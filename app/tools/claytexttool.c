/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextTool
 * Copyright (C) 2002-2010  Sven Neumann <sven@gimp.org>
 *                          Daniel Eddeland <danedde@svn.gnome.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/clayasyncset.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/claydrawable.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/clayprojection.h"
#include "core/clay-palettes.h"
#include "core/clayimage-pick-item.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/claylayer-floating-selection.h"
#include "core/claylayermask.h"
#include "core/claylist.h"
#include "core/clayrasterizable.h"
#include "core/claytoolinfo.h"
#include "core/clayundostack.h"

#include "menus/menus.h"

#include "text/claytext.h"
#include "text/claytext-path.h"
#include "text/claytextlayer.h"
#include "text/claytextlayout.h"
#include "text/claytextundo.h"

#include "path/claystroke.h"
#include "path/claypath.h"
#include "path/claypath-warp.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydockcontainer.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymenufactory.h"
#include "widgets/claytextbuffer.h"
#include "widgets/claytextstyleeditor.h"
#include "widgets/clayuimanager.h"
#include "widgets/clayviewabledialog.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasgroup.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolrectangle.h"

#include "claytextoptions.h"
#include "claytexttool.h"
#include "claytexttool-editor.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


#define TEXT_UNDO_TIMEOUT 3
#define MIN_DRAG_SIZE     3 /* in screen pixels */

/*  local function prototypes  */

static void      clay_text_tool_constructed     (GObject           *object);
static void      clay_text_tool_finalize        (GObject           *object);

static void      clay_text_tool_control         (ClayTool          *tool,
                                                 ClayToolAction     action,
                                                 ClayDisplay       *display);
static void      clay_text_tool_button_press    (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 ClayButtonPressType  press_type,
                                                 ClayDisplay       *display);
static void      clay_text_tool_button_release  (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 ClayButtonReleaseType release_type,
                                                 ClayDisplay       *display);
static void      clay_text_tool_motion          (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 guint32            time,
                                                 GdkModifierType    state,
                                                 ClayDisplay       *display);
static gboolean  clay_text_tool_key_press       (ClayTool          *tool,
                                                 GdkEventKey       *kevent,
                                                 ClayDisplay       *display);
static gboolean  clay_text_tool_key_release     (ClayTool          *tool,
                                                 GdkEventKey       *kevent,
                                                 ClayDisplay       *display);
static void      clay_text_tool_oper_update     (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 GdkModifierType    state,
                                                 gboolean           proximity,
                                                 ClayDisplay       *display);
static void      clay_text_tool_cursor_update   (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 GdkModifierType    state,
                                                 ClayDisplay       *display);
static ClayUIManager * clay_text_tool_get_popup (ClayTool          *tool,
                                                 const ClayCoords  *coords,
                                                 GdkModifierType    state,
                                                 ClayDisplay       *display,
                                                 const gchar      **ui_path);

static void      clay_text_tool_draw            (ClayDrawTool      *draw_tool);
static void      clay_text_tool_draw_selection  (ClayDrawTool      *draw_tool);

static gboolean  clay_text_tool_start           (ClayTextTool      *text_tool,
                                                 ClayDisplay       *display,
                                                 ClayLayer         *layer,
                                                 GError           **error);
static void      clay_text_tool_halt            (ClayTextTool      *text_tool);

static void      clay_text_tool_frame_item      (ClayTextTool      *text_tool);

static void      clay_text_tool_rectangle_response
                                                (ClayToolRectangle *rectangle,
                                                 gint               response_id,
                                                 ClayTextTool      *text_tool);
static void      clay_text_tool_rectangle_change_complete
                                                (ClayToolRectangle *rectangle,
                                                 ClayTextTool      *text_tool);

static void      clay_text_tool_connect         (ClayTextTool      *text_tool,
                                                 ClayTextLayer     *layer,
                                                 ClayText          *text);

static void      clay_text_tool_layer_notify    (ClayTextLayer     *layer,
                                                 const GParamSpec  *pspec,
                                                 ClayTextTool      *text_tool);
static void      clay_text_tool_proxy_notify    (ClayText          *text,
                                                 const GParamSpec  *pspec,
                                                 ClayTextTool      *text_tool);

static void      clay_text_tool_text_notify     (ClayText          *text,
                                                 const GParamSpec  *pspec,
                                                 ClayTextTool      *text_tool);
static void      clay_text_tool_text_changed    (ClayText          *text,
                                                 ClayTextTool      *text_tool);

static void
    clay_text_tool_fonts_async_set_empty_notify (ClayAsyncSet      *async_set,
                                                 GParamSpec        *pspec,
                                                 ClayTextTool      *text_tool);

static void      clay_text_tool_apply_list      (ClayTextTool      *text_tool,
                                                 GList             *pspecs);

static void      clay_text_tool_create_layer    (ClayTextTool      *text_tool,
                                                 ClayText          *text);

static void      clay_text_tool_layer_changed   (ClayImage         *image,
                                                 ClayTextTool      *text_tool);
static void      clay_text_tool_set_image       (ClayTextTool      *text_tool,
                                                 ClayImage         *image);
static gboolean  clay_text_tool_set_drawable    (ClayTextTool      *text_tool,
                                                 ClayDrawable      *drawable,
                                                 gboolean           confirm);

static void      clay_text_tool_block_drawing   (ClayTextTool      *text_tool);
static void      clay_text_tool_unblock_drawing (ClayTextTool      *text_tool);

static void    clay_text_tool_buffer_begin_edit (ClayTextBuffer    *buffer,
                                                 ClayTextTool      *text_tool);
static void    clay_text_tool_buffer_end_edit   (ClayTextBuffer    *buffer,
                                                 ClayTextTool      *text_tool);

static void    clay_text_tool_buffer_color_applied
                                                (ClayTextBuffer    *buffer,
                                                 GeglColor         *color,
                                                 ClayTextTool      *text_tool);


G_DEFINE_TYPE (ClayTextTool, clay_text_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_text_tool_parent_class


void
clay_text_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_TEXT_TOOL,
                CLAY_TYPE_TEXT_OPTIONS,
                clay_text_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_FONT       |
                CLAY_CONTEXT_PROP_MASK_PALETTE /* for the color popup's palette tab */,
                "clay-text-tool",
                _("Text"),
                _("Text Tool: Create or edit text layers"),
                N_("Te_xt"), "T",
                NULL, CLAY_HELP_TOOL_TEXT,
                CLAY_ICON_TOOL_TEXT,
                data);
}

static void
clay_text_tool_class_init (ClayTextToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->constructed    = clay_text_tool_constructed;
  object_class->finalize       = clay_text_tool_finalize;

  tool_class->control          = clay_text_tool_control;
  tool_class->button_press     = clay_text_tool_button_press;
  tool_class->motion           = clay_text_tool_motion;
  tool_class->button_release   = clay_text_tool_button_release;
  tool_class->key_press        = clay_text_tool_key_press;
  tool_class->key_release      = clay_text_tool_key_release;
  tool_class->oper_update      = clay_text_tool_oper_update;
  tool_class->cursor_update    = clay_text_tool_cursor_update;
  tool_class->get_popup        = clay_text_tool_get_popup;
  tool_class->is_destructive   = FALSE;

  draw_tool_class->draw        = clay_text_tool_draw;
}

static void
clay_text_tool_init (ClayTextTool *text_tool)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  text_tool->buffer = clay_text_buffer_new ();

  g_signal_connect (text_tool->buffer, "begin-user-action",
                    G_CALLBACK (clay_text_tool_buffer_begin_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "end-user-action",
                    G_CALLBACK (clay_text_tool_buffer_end_edit),
                    text_tool);
  g_signal_connect (text_tool->buffer, "color-applied",
                    G_CALLBACK (clay_text_tool_buffer_color_applied),
                    text_tool);

  text_tool->handle_rectangle_change_complete = TRUE;

  clay_text_tool_editor_init (text_tool);

  clay_tool_control_set_scroll_lock          (tool->control, TRUE);
  clay_tool_control_set_handle_empty_image   (tool->control, TRUE);
  clay_tool_control_set_wants_click          (tool->control, TRUE);
  clay_tool_control_set_wants_double_click   (tool->control, TRUE);
  clay_tool_control_set_wants_triple_click   (tool->control, TRUE);
  clay_tool_control_set_wants_all_key_events (tool->control, TRUE);
  clay_tool_control_set_active_modifiers     (tool->control,
                                              CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision            (tool->control,
                                              CLAY_CURSOR_PRECISION_PIXEL_BORDER);
  clay_tool_control_set_tool_cursor          (tool->control,
                                              CLAY_TOOL_CURSOR_TEXT);
  clay_tool_control_set_action_object_1      (tool->control,
                                              "context-font-select-set");
}

static void
clay_text_tool_constructed (GObject *object)
{
  ClayTextTool    *text_tool = CLAY_TEXT_TOOL (object);
  ClayTextOptions *options   = CLAY_TEXT_TOOL_GET_OPTIONS (text_tool);
  ClayTool        *tool      = CLAY_TOOL (text_tool);
  ClayAsyncSet    *async_set;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  text_tool->proxy = g_object_new (CLAY_TYPE_TEXT, "clay", tool->tool_info->clay, NULL);

  clay_text_options_connect_text (options, text_tool->proxy);

  g_signal_connect_object (text_tool->proxy, "notify",
                           G_CALLBACK (clay_text_tool_proxy_notify),
                           text_tool, 0);

  async_set =
    clay_data_factory_get_async_set (tool->tool_info->clay->font_factory);

  g_signal_connect_object (async_set,
                           "notify::empty",
                           G_CALLBACK (clay_text_tool_fonts_async_set_empty_notify),
                           text_tool, 0);
}

static void
clay_text_tool_finalize (GObject *object)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (object);

  g_clear_object (&text_tool->proxy);
  g_clear_object (&text_tool->buffer);

  clay_text_tool_editor_finalize (text_tool);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_text_tool_remove_empty_text_layer (ClayTextTool *text_tool)
{
  ClayTextLayer *text_layer = text_tool->layer;

  if (text_layer && clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (text_layer)))
    {
      ClayText *text = clay_text_layer_get_text (text_layer);

      if (text && text->box_mode == CLAY_TEXT_BOX_DYNAMIC &&
          (! text->text || text->text[0] == '\0') &&
          (! text->markup || text->markup[0] == '\0'))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (text_layer));

          if (text_tool->image == image)
            g_signal_handlers_block_by_func (image,
                                             clay_text_tool_layer_changed,
                                             text_tool);

          clay_image_remove_layer (image, CLAY_LAYER (text_layer), TRUE, NULL);
          clay_image_flush (image);

          if (text_tool->image == image)
            g_signal_handlers_unblock_by_func (image,
                                               clay_text_tool_layer_changed,
                                               text_tool);
        }
    }
}

static void
clay_text_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_text_tool_halt (text_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_text_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayTextTool      *text_tool = CLAY_TEXT_TOOL (tool);
  ClayImage         *image     = clay_display_get_image (display);
  ClayText          *text      = text_tool->text;
  ClayToolRectangle *rectangle;

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  if (tool->display && tool->display != display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

  if (! text_tool->widget)
    {
      GError *error = NULL;

      if (! clay_text_tool_start (text_tool, display, NULL, &error))
        {
          clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));

          clay_tool_message_literal (tool, display, error->message);

          g_clear_error (&error);

          return;
        }

      clay_tool_widget_hover (text_tool->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      clay_tool_rectangle_set_function (CLAY_TOOL_RECTANGLE (text_tool->widget),
                                        CLAY_TOOL_RECTANGLE_CREATING);
    }

  rectangle = CLAY_TOOL_RECTANGLE (text_tool->widget);

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    {
      clay_tool_control_activate (tool->control);

      /* clicking anywhere while a preedit is going on aborts the
       * preedit, this is ugly but at least leaves everything in
       * a consistent state
       */
      if (text_tool->preedit_active)
        clay_text_tool_abort_im_context (text_tool);
      else
        clay_text_tool_reset_im_context (text_tool);

      text_tool->button_held = FALSE;

      if (text_tool->moving && text_tool->layer)
        {
          ClayItem *item = CLAY_ITEM (text_tool->layer);

          text_tool->move_start_x = clay_item_get_offset_x (item);
          text_tool->move_start_y = clay_item_get_offset_y (item);
        }

      if (clay_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          clay_tool_rectangle_set_function (rectangle,
                                            CLAY_TOOL_RECTANGLE_DEAD);
        }
      else if (clay_tool_widget_button_press (text_tool->widget, coords,
                                              time, state, press_type))
        {
          text_tool->grab_widget = text_tool->widget;
        }

      /*  bail out now if the user user clicked on a handle of an
       *  existing rectangle, but not inside an existing framed layer
       */
      if (clay_tool_rectangle_get_function (rectangle) !=
          CLAY_TOOL_RECTANGLE_CREATING)
        {
          if (text_tool->layer)
            {
              ClayItem *item = CLAY_ITEM (text_tool->layer);
              gdouble   x    = coords->x - clay_item_get_offset_x (item);
              gdouble   y    = coords->y - clay_item_get_offset_y (item);

              if (x < 0 || x >= clay_item_get_width  (item) ||
                  y < 0 || y >= clay_item_get_height (item))
                {
                  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
                  return;
                }
            }
          else
            {
              clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
              return;
            }
        }

      /* if the the click is not related to the currently edited text
       * layer in any way, try to pick a text layer
       */
      if (! text_tool->moving &&
          clay_tool_rectangle_get_function (rectangle) ==
          CLAY_TOOL_RECTANGLE_CREATING)
        {
          ClayTextLayer *text_layer;

          text_layer = clay_image_pick_text_layer (image, coords->x, coords->y);

          if (text_layer && text_layer != text_tool->layer)
            {
              GList *selection = g_list_prepend (NULL, text_layer);

              if (text_tool->image == image)
                g_signal_handlers_block_by_func (image,
                                                 clay_text_tool_layer_changed,
                                                 text_tool);

              clay_image_set_selected_layers (image, selection);
              g_list_free (selection);

              /* override shift+click due to the change in layers */
              state = state & ~clay_get_extend_selection_mask ();

              if (text_tool->image == image)
                g_signal_handlers_unblock_by_func (image,
                                                   clay_text_tool_layer_changed,
                                                   text_tool);
            }
        }
    }

  if (clay_image_coords_in_active_pickable (image, coords, FALSE, FALSE, FALSE))
    {
      GList        *drawables = clay_image_get_selected_drawables (image);
      ClayDrawable *drawable  = NULL;
      gdouble       x         = coords->x;
      gdouble       y         = coords->y;

      if (g_list_length (drawables) == 1)
        {
          ClayItem *item = CLAY_ITEM (drawables->data);

          x = coords->x - clay_item_get_offset_x (item);
          y = coords->y - clay_item_get_offset_y (item);

          drawable = drawables->data;
        }
      g_list_free (drawables);

      /* If we clicked on a layer mask attached TO a text layer,
       * try to pick the underlying text layer instead (issue #16514)) */
      if (drawable && CLAY_IS_LAYER_MASK (drawable))
        {
          ClayLayer *layer = clay_layer_mask_get_layer (CLAY_LAYER_MASK (drawable));

          if (CLAY_IS_TEXT_LAYER (layer))
            drawable = CLAY_DRAWABLE (layer);
        }

      /*  did the user click on a text layer?  */
      if (drawable &&
          clay_text_tool_set_drawable (text_tool, drawable, TRUE))
        {
          if (press_type == CLAY_BUTTON_PRESS_NORMAL)
            {
              /*  if we clicked on a text layer while the tool was idle
               *  (didn't show a rectangle), frame the layer and switch to
               *  selecting instead of drawing a new rectangle
               */
              if (clay_tool_rectangle_get_function (rectangle) ==
                  CLAY_TOOL_RECTANGLE_CREATING)
                {
                  clay_tool_rectangle_set_function (rectangle,
                                                    CLAY_TOOL_RECTANGLE_DEAD);

                  clay_text_tool_frame_item (text_tool);
                }

              if (text_tool->text && text_tool->text != text)
                {
                  if (text_tool->style_overlay)
                    {
                      gtk_widget_destroy (text_tool->style_overlay);
                      text_tool->style_overlay = NULL;
                      text_tool->style_editor  = NULL;
                    }

                  clay_text_tool_editor_start (text_tool);
                }
            }

          if (text_tool->text && ! text_tool->moving)
            {
              /* Shift+click: select from (previous cursor) to (cursor) */
              if (press_type == CLAY_BUTTON_PRESS_NORMAL &&
                  state & clay_get_extend_selection_mask ())
                {
                  clay_text_tool_editor_motion (text_tool, x, y);
                }
              else
                {
                  clay_text_tool_editor_button_press (text_tool, x, y, press_type);
                }

              text_tool->button_held = TRUE;
            }
          else
            {
              text_tool->button_held = FALSE;
            }

          clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));

          return;
        }
    }

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    {
      /*  create a new text layer  */
      text_tool->text_box_fixed = FALSE;
      text_tool->button_held    = FALSE;

      /* make sure the text tool has an image, even if the user didn't click
       * inside the active drawable, in particular, so that the text style
       * editor picks the correct resolution.
       */
      clay_text_tool_set_image (text_tool, image);

      clay_text_tool_connect (text_tool, NULL, NULL);
      clay_text_tool_editor_start (text_tool);
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_text_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayTextTool      *text_tool = CLAY_TEXT_TOOL (tool);
  ClayToolRectangle *rectangle = CLAY_TOOL_RECTANGLE (text_tool->widget);

  clay_tool_control_halt (tool->control);

  if (text_tool->button_held)
    {
      /*  if we were still doing a click-drag select, finish it and
       *  ignore rectangle-change-complete.
       */

      /*  need to block "end-user-action" on the text buffer, because
       *  GtkTextBuffer considers copying text to the clipboard an
       *  undo-relevant user action, which is clearly a bug, but what
       *  can we do...
       */
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_end_edit,
                                       text_tool);

      clay_text_tool_editor_button_release (text_tool);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_begin_edit,
                                         text_tool);

      text_tool->button_held = FALSE;

      text_tool->handle_rectangle_change_complete = FALSE;

      /*  there is no cancelling of selections yet  */
      if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
        release_type = CLAY_BUTTON_RELEASE_NORMAL;
    }
  else if (text_tool->moving)
    {
      if (release_type == CLAY_BUTTON_RELEASE_CANCEL &&
          text_tool->layer)
        {
          /*  the user has canceled the move, restore the layer to
           *  its original position.
           */
          ClayItem  *item  = CLAY_ITEM (text_tool->layer);
          ClayImage *image = clay_item_get_image (item);

          clay_item_translate (item,
                               text_tool->move_start_x - clay_item_get_offset_x (item),
                               text_tool->move_start_y - clay_item_get_offset_y (item),
                               FALSE);

          clay_projection_flush (clay_image_get_projection (image));
        }

      /*  fall through and let rectangle-change-complete do its job of
       *  setting text layer's new position.
       */
    }
  else if (clay_tool_rectangle_get_function (rectangle) ==
           CLAY_TOOL_RECTANGLE_DEAD)
    {
      /*  the user clicked in dead space (like between the corner and
       *  edge handles, so completely ignore that.
       */

      text_tool->handle_rectangle_change_complete = FALSE;
    }
  else if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      /*  user has canceled the rectangle resizing, fall through
       *  and let the rectangle handle restoring the previous size
       */
    }
  else
    {
      gdouble x1, y1;
      gdouble x2, y2;

      /*  otherwise the user has clicked outside of any text layer in
       *  order to create a new text, fall through and let
       *  rectangle-change-complete do its job of setting the new text
       *  layer's size.
       */

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      if (release_type == CLAY_BUTTON_RELEASE_CLICK ||
          (x2 - x1) < MIN_DRAG_SIZE                 ||
          (y2 - y1) < MIN_DRAG_SIZE)
        {
          /*  unless the rectangle is unreasonably small to hold any
           *  real text (the user has either just clicked or just made
           *  a rectangle of a few pixels), so set the text box to
           *  dynamic and ignore rectangle-change-complete.
           */

          g_object_set (text_tool->proxy,
                        "box-mode", CLAY_TEXT_BOX_DYNAMIC,
                        NULL);

          text_tool->handle_rectangle_change_complete = FALSE;
        }
    }

  if (text_tool->grab_widget)
    {
      clay_tool_widget_button_release (text_tool->grab_widget,
                                       coords, time, state, release_type);
      text_tool->grab_widget = NULL;
    }

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
clay_text_tool_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (tool);

  if (! text_tool->button_held)
    {
      if (text_tool->grab_widget)
        {
          clay_tool_widget_motion (text_tool->grab_widget,
                                   coords, time, state);
        }

      if (text_tool->moving && text_tool->layer)
        {
          ClayItem  *item  = CLAY_ITEM (text_tool->layer);
          ClayImage *image = clay_item_get_image (item);
          gdouble    x1, y1;

          g_object_get (text_tool->widget,
                        "x1", &x1,
                        "y1", &y1,
                        NULL);

          clay_item_translate (item,
                               (gint) x1 - clay_item_get_offset_x (item),
                               (gint) y1 - clay_item_get_offset_y (item),
                               FALSE);

          clay_projection_flush (clay_image_get_projection (image));
        }
    }
  else
    {
      ClayItem *item = CLAY_ITEM (text_tool->layer);
      gdouble   x    = coords->x - clay_item_get_offset_x (item);
      gdouble   y    = coords->y - clay_item_get_offset_y (item);

      clay_text_tool_editor_motion (text_tool, x, y);
    }
}

static gboolean
clay_text_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (tool);

  if (display == tool->display)
    return clay_text_tool_editor_key_press (text_tool, kevent);

  return FALSE;
}

static gboolean
clay_text_tool_key_release (ClayTool    *tool,
                            GdkEventKey *kevent,
                            ClayDisplay *display)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (tool);

  if (display == tool->display)
    return clay_text_tool_editor_key_release (text_tool, kevent);

  return FALSE;
}

static void
clay_text_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayTextTool      *text_tool = CLAY_TEXT_TOOL (tool);
  ClayToolRectangle *rectangle = CLAY_TOOL_RECTANGLE (text_tool->widget);

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                               proximity, display);

  text_tool->moving = (text_tool->widget &&
                       clay_tool_rectangle_get_function (rectangle) ==
                       CLAY_TOOL_RECTANGLE_MOVING &&
                       (state & GDK_MOD1_MASK));
}

static void
clay_text_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayTextTool      *text_tool = CLAY_TEXT_TOOL (tool);
  ClayToolRectangle *rectangle = CLAY_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle && tool->display == display)
    {
      if (clay_tool_rectangle_point_in_rectangle (rectangle,
                                                  coords->x,
                                                  coords->y) &&
          ! text_tool->moving)
        {
          clay_tool_set_cursor (tool, display,
                                (ClayCursorType) GDK_XTERM,
                                clay_tool_control_get_tool_cursor (tool->control),
                                CLAY_CURSOR_MODIFIER_NONE);
        }
      else
        {
          CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
    }
  else
    {
      ClayAsyncSet *async_set;

      async_set =
        clay_data_factory_get_async_set (tool->tool_info->clay->font_factory);

      if (clay_async_set_is_empty (async_set))
        {
          CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                         display);
        }
      else
        {
          clay_tool_set_cursor (tool, display,
                                clay_tool_control_get_cursor (tool->control),
                                clay_tool_control_get_tool_cursor (tool->control),
                                CLAY_CURSOR_MODIFIER_BAD);
        }
    }
}

static ClayUIManager *
clay_text_tool_get_popup (ClayTool         *tool,
                          const ClayCoords *coords,
                          GdkModifierType   state,
                          ClayDisplay      *display,
                          const gchar     **ui_path)
{
  ClayTextTool      *text_tool = CLAY_TEXT_TOOL (tool);
  ClayToolRectangle *rectangle = CLAY_TOOL_RECTANGLE (text_tool->widget);

  if (rectangle &&
      clay_tool_rectangle_point_in_rectangle (rectangle,
                                              coords->x,
                                              coords->y))
    {
      ClayMenuFactory *menu_factory;
      ClayUIManager   *ui_manager;

      menu_factory = menus_get_global_menu_factory (tool->tool_info->clay);
      ui_manager   = clay_menu_factory_get_manager (menu_factory, "<TextTool>", text_tool);

      clay_ui_manager_update (ui_manager, text_tool);

      *ui_path = "/text-tool-popup";

      return ui_manager;
    }

  return NULL;
}

static void
clay_text_tool_draw (ClayDrawTool *draw_tool)
{
  ClayTextTool *text_tool = CLAY_TEXT_TOOL (draw_tool);

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (! text_tool->text  ||
      ! text_tool->layer ||
      ! text_tool->layer->text)
    {
      clay_text_tool_editor_update_im_cursor (text_tool);

      return;
    }

  clay_text_tool_ensure_layout (text_tool);

  if (gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (text_tool->buffer)))
    {
      /* If the text buffer has a selection, highlight the selected letters */

      clay_text_tool_draw_selection (draw_tool);
    }
  else
    {
      /* If the text buffer has no selection, draw the text cursor */

      ClayCanvasItem   *item;
      PangoRectangle    cursor_rect;
      ClayVector2       corners[4];
      gint              off_x, off_y;
      gboolean          overwrite;
      ClayTextDirection direction;

      if (clay_text_tool_editor_get_cursor_rect (text_tool,
                                                 text_tool->overwrite_mode,
                                                 &cursor_rect, corners))
        {
          gint c;

          clay_item_get_offset (CLAY_ITEM (text_tool->layer), &off_x, &off_y);

          for (c = 0; c < 4; c++)
            {
              corners[c].x += off_x;
              corners[c].y += off_y;
            }

          item = clay_draw_tool_add_lines (draw_tool, corners, 4, NULL, FALSE);
          clay_canvas_item_set_highlight (item, TRUE);

          clay_text_tool_editor_update_im_cursor (text_tool);
          return;
        }

      clay_item_get_offset (CLAY_ITEM (text_tool->layer), &off_x, &off_y);
      cursor_rect.x += off_x;
      cursor_rect.y += off_y;

      overwrite = text_tool->overwrite_mode && cursor_rect.width != 0;

      direction = clay_text_tool_get_direction (text_tool);

      item = clay_draw_tool_add_text_cursor (draw_tool, &cursor_rect,
                                             overwrite, direction);
      clay_canvas_item_set_highlight (item, TRUE);
    }

  clay_text_tool_editor_update_im_cursor (text_tool);
}

static void
clay_text_tool_draw_selection (ClayDrawTool *draw_tool)
{
  ClayTextTool     *text_tool = CLAY_TEXT_TOOL (draw_tool);
  GtkTextBuffer    *buffer    = GTK_TEXT_BUFFER (text_tool->buffer);
  ClayCanvasGroup  *group;
  PangoLayout      *layout;
  gint              offset_x;
  gint              offset_y;
  gint              width;
  gint              height;
  gint              off_x, off_y;
  PangoLayoutIter  *iter;
  GtkTextIter       sel_start, sel_end;
  gint              min, max;
  gint              i;
  ClayTextDirection direction;

  group = clay_draw_tool_add_stroke_group (draw_tool);
  clay_canvas_item_set_highlight (CLAY_CANVAS_ITEM (group), TRUE);

  gtk_text_buffer_get_selection_bounds (buffer, &sel_start, &sel_end);

  min = clay_text_buffer_get_iter_index (text_tool->buffer, &sel_start, TRUE);
  max = clay_text_buffer_get_iter_index (text_tool->buffer, &sel_end, TRUE);

  layout = clay_text_layout_get_pango_layout (text_tool->layout);

  clay_text_layout_get_offsets (text_tool->layout, &offset_x, &offset_y);

  clay_text_layout_get_size (text_tool->layout, &width, &height);

  clay_item_get_offset (CLAY_ITEM (text_tool->layer), &off_x, &off_y);
  offset_x += off_x;
  offset_y += off_y;

  direction = clay_text_tool_get_direction (text_tool);

  iter = pango_layout_get_iter (layout);

  clay_draw_tool_push_group (draw_tool, group);

  do
    {
      if (! pango_layout_iter_get_run (iter))
        continue;

      i = pango_layout_iter_get_index (iter);

      if (i >= min && i < max)
        {
          PangoRectangle rect;
          gint           ytop, ybottom;

          pango_layout_iter_get_char_extents (iter, &rect);
          pango_layout_iter_get_line_yrange (iter, &ytop, &ybottom);

          rect.y      = ytop;
          rect.height = ybottom - ytop;

          pango_extents_to_pixels (&rect, NULL);

          /*  A rotated or sheared glyph extent is a quad, not an axis-aligned
           *  rectangle, so draw the four mapped corners directly.  Only the
           *  horizontal directions are handled this way; the vertical ones do
           *  their own coordinate juggling below and combining that with an
           *  arbitrary matrix is a separate problem.
           */
          if (clay_text_layout_is_transformed (text_tool->layout) &&
              (direction == CLAY_TEXT_DIRECTION_LTR ||
               direction == CLAY_TEXT_DIRECTION_RTL))
            {
              ClayVector2 corners[4];
              gint        c;

              clay_text_layout_transform_quad (text_tool->layout, &rect,
                                               corners);

              for (c = 0; c < 4; c++)
                {
                  corners[c].x += offset_x;
                  corners[c].y += offset_y;
                }

              clay_draw_tool_add_lines (draw_tool, corners, 4, NULL, FALSE);
              continue;
            }

          clay_text_layout_transform_rect (text_tool->layout, &rect);

          switch (direction)
            {
            case CLAY_TEXT_DIRECTION_LTR:
            case CLAY_TEXT_DIRECTION_RTL:
              rect.x += offset_x;
              rect.y += offset_y;
              clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.x, rect.y,
                                            rect.width, rect.height);
              break;
            case CLAY_TEXT_DIRECTION_TTB_RTL:
            case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              rect.y = offset_x - rect.y + width;
              rect.x = offset_y + rect.x;
              clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            -rect.height, rect.width);
              break;
            case CLAY_TEXT_DIRECTION_TTB_LTR:
            case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              rect.y = offset_x + rect.y;
              rect.x = offset_y - rect.x + height;
              clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                            rect.y, rect.x,
                                            rect.height, -rect.width);
              break;
            }
        }
    }
  while (pango_layout_iter_next_char (iter));

  clay_draw_tool_pop_group (draw_tool);

  pango_layout_iter_free (iter);
}

static gboolean
clay_text_tool_start (ClayTextTool  *text_tool,
                      ClayDisplay   *display,
                      ClayLayer     *layer,
                      GError       **error)
{
  ClayTool         *tool  = CLAY_TOOL (text_tool);
  ClayDisplayShell *shell = clay_display_get_shell (display);
  ClayToolWidget   *widget;
  ClayAsyncSet     *async_set;

  async_set =
    clay_data_factory_get_async_set (tool->tool_info->clay->font_factory);

  if (! clay_async_set_is_empty (async_set))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Fonts are still loading"));

      return FALSE;
    }

  tool->display = display;

  text_tool->widget = widget = clay_tool_rectangle_new (shell);

  g_object_set (widget,
                "force-narrow-mode", TRUE,
                "status-title",      _("Text box: "),
                NULL);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), widget);

  g_signal_connect (widget, "response",
                    G_CALLBACK (clay_text_tool_rectangle_response),
                    text_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (clay_text_tool_rectangle_change_complete),
                    text_tool);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);

  if (layer)
    {
      clay_text_tool_frame_item (text_tool);
      clay_text_tool_editor_start (text_tool);
      clay_text_tool_editor_position (text_tool);
    }

  return TRUE;
}

static void
clay_text_tool_halt (ClayTextTool *text_tool)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  clay_text_tool_editor_halt (text_tool);
  clay_text_tool_clear_layout (text_tool);
  clay_text_tool_set_drawable (text_tool, NULL, FALSE);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&text_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
clay_text_tool_frame_item (ClayTextTool *text_tool)
{
  g_return_if_fail (CLAY_IS_LAYER (text_tool->layer));

  text_tool->handle_rectangle_change_complete = FALSE;

  clay_tool_rectangle_frame_item (CLAY_TOOL_RECTANGLE (text_tool->widget),
                                  CLAY_ITEM (text_tool->layer));

  /* Update crop of any filters applied to text */
  if (text_tool->layer)
  {
    GList         *list;
    ClayDrawable  *drawable = CLAY_DRAWABLE (text_tool->layer);
    ClayContainer *filters  = clay_drawable_get_filters (drawable);

    for (list = CLAY_LIST (filters)->queue->tail;
         list; list = g_list_previous (list))
      {
        ClayDrawableFilter *filter = list->data;

        /* TODO: Handle partial layer effect */
        clay_drawable_filter_set_region (filter, CLAY_FILTER_REGION_SELECTION);
        clay_drawable_filter_set_region (filter, CLAY_FILTER_REGION_DRAWABLE);
      }
    if (list)
      g_list_free (list);
  }

  text_tool->handle_rectangle_change_complete = TRUE;
}

static void
clay_text_tool_rectangle_response (ClayToolRectangle *rectangle,
                                   gint               response_id,
                                   ClayTextTool      *text_tool)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  /* this happens when a newly created rectangle gets canceled,
   * we have to shut down the tool
   */
  if (response_id == CLAY_TOOL_WIDGET_RESPONSE_CANCEL)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
}

static void
clay_text_tool_rectangle_change_complete (ClayToolRectangle *rectangle,
                                          ClayTextTool      *text_tool)
{
  clay_text_tool_editor_position (text_tool);

  if (text_tool->handle_rectangle_change_complete)
    {
      ClayItem *item = CLAY_ITEM (text_tool->layer);
      gdouble   x1, y1;
      gdouble   x2, y2;
      gboolean  location_changed;

      if (! item)
        {
          /* we can't set properties for the text layer, because it
           * isn't created until some text has been inserted, so we
           * need to make a special note that will remind us what to
           * do when we actually create the layer
           */
          text_tool->text_box_fixed = TRUE;

          return;
        }

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      /* Indicates if the live preview of the text has been moved */
      location_changed = ((gint) x1 != text_tool->move_start_x ||
                          (gint) y1 != text_tool->move_start_y);

      if ((x2 - x1) != clay_item_get_width  (item) ||
          (y2 - y1) != clay_item_get_height (item))
        {
          ClayUnit *box_unit = text_tool->proxy->box_unit;
          gdouble   xres, yres;
          gboolean  push_undo = TRUE;
          ClayUndo *undo;

          clay_image_get_resolution (text_tool->image, &xres, &yres);

          g_object_set (text_tool->proxy,
                        "box-mode",   CLAY_TEXT_BOX_FIXED,
                        "box-width",  clay_pixels_to_units (x2 - x1,
                                                            box_unit, xres),
                        "box-height", clay_pixels_to_units (y2 - y1,
                                                            box_unit, yres),
                        NULL);

          undo = clay_image_undo_can_compress (text_tool->image,
                                               CLAY_TYPE_UNDO_STACK,
                                               CLAY_UNDO_GROUP_TEXT);

          if (undo &&
              clay_undo_get_age (undo) <= TEXT_UNDO_TIMEOUT &&
              g_object_get_data (G_OBJECT (undo), "reshape-text-layer") == (gpointer) item)
            push_undo = FALSE;

          if (push_undo)
            {
              clay_image_undo_group_start (text_tool->image, CLAY_UNDO_GROUP_TEXT,
                                           _("Reshape Text Layer"));

              undo = clay_image_undo_can_compress (text_tool->image, CLAY_TYPE_UNDO_STACK,
                                                   CLAY_UNDO_GROUP_TEXT);

              if (undo)
                g_object_set_data (G_OBJECT (undo), "reshape-text-layer",
                                   (gpointer) item);
            }

          clay_text_tool_block_drawing (text_tool);

          clay_item_translate (item,
                               x1 - clay_item_get_offset_x (item),
                               y1 - clay_item_get_offset_y (item),
                               push_undo);
          clay_text_tool_apply (text_tool, push_undo);

          clay_text_tool_unblock_drawing (text_tool);

          if (push_undo)
            clay_image_undo_group_end (text_tool->image);
        }
      else if ((text_tool->moving && location_changed) ||
               (x1 != clay_item_get_offset_x (item)    ||
                y1 != clay_item_get_offset_y (item)))
        {
          clay_text_tool_block_drawing (text_tool);

          clay_image_undo_group_start (text_tool->image, CLAY_UNDO_GROUP_TEXT,
                                       _("Move Text Layer"));

          if (location_changed)
            clay_item_translate (item,
                                 text_tool->move_start_x - clay_item_get_offset_x (item),
                                 text_tool->move_start_y - clay_item_get_offset_y (item),
                                 FALSE);

          clay_text_tool_apply (text_tool, TRUE);

          clay_item_translate (item,
                               x1 - clay_item_get_offset_x (item),
                               y1 - clay_item_get_offset_y (item),
                               TRUE);

          clay_image_undo_group_end (text_tool->image);

          clay_text_tool_unblock_drawing (text_tool);

          clay_image_flush (text_tool->image);
        }
    }
}

static void
clay_text_tool_connect (ClayTextTool  *text_tool,
                        ClayTextLayer *layer,
                        ClayText      *text)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  g_return_if_fail (text == NULL || (layer != NULL && layer->text == text));

  if (text_tool->text != text)
    {
      ClayTextOptions *options = CLAY_TEXT_TOOL_GET_OPTIONS (tool);

      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_end_edit,
                                       text_tool);

      if (text_tool->text)
        {
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                clay_text_tool_text_notify,
                                                text_tool);
          g_signal_handlers_disconnect_by_func (text_tool->text,
                                                clay_text_tool_text_changed,
                                                text_tool);

          if (text_tool->pending)
            clay_text_tool_apply (text_tool, TRUE);

          g_clear_object (&text_tool->text);

          g_object_set (text_tool->proxy,
                        "text",   NULL,
                        "markup", NULL,
                        NULL);
          clay_text_buffer_set_text (text_tool->buffer, NULL);

          clay_text_tool_clear_layout (text_tool);
        }

      clay_context_define_property (CLAY_CONTEXT (options),
                                    CLAY_CONTEXT_PROP_FOREGROUND,
                                    text != NULL);

      if (text)
        {
          if (text->unit != text_tool->proxy->unit)
            clay_size_entry_set_unit (CLAY_SIZE_ENTRY (options->size_entry),
                                      text->unit);

          clay_config_sync (G_OBJECT (text), G_OBJECT (text_tool->proxy), 0);

          /*  The proxy carries style -- font, size, colour, justification --
           *  forward from one text layer to the next, and
           *  clay_text_tool_create_layer() builds a new layer by duplicating
           *  it.  The transformation is not style, it is per-layer geometry,
           *  and clay_config_sync() above has just copied this layer's matrix
           *  into the proxy along with everything else.  Left there, every new
           *  text layer created afterwards would be born carrying the matrix
           *  of whichever layer was last edited.
           *
           *  This is safe here specifically because text_tool->text is still
           *  NULL at this point -- it is assigned below -- so
           *  clay_text_tool_proxy_notify() early-returns and these resets are
           *  not queued up and written back to the layer being connected.
           */
          {
            ClayMatrix2 identity;

            clay_matrix2_identity (&identity);

            g_object_set (text_tool->proxy,
                          "transformation", &identity,
                          "offset-x",       0.0,
                          "offset-y",       0.0,
                          NULL);
          }

          if (text->markup)
            clay_text_buffer_set_markup (text_tool->buffer, text->markup);
          else
            clay_text_buffer_set_text (text_tool->buffer, text->text);

          clay_text_tool_clear_layout (text_tool);

          text_tool->text = g_object_ref (text);

          g_signal_connect (text, "notify",
                            G_CALLBACK (clay_text_tool_text_notify),
                            text_tool);
          g_signal_connect (text, "changed",
                            G_CALLBACK (clay_text_tool_text_changed),
                            text_tool);
        }

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  if (text_tool->layer != layer)
    {
      if (text_tool->layer)
        {
          g_signal_handlers_disconnect_by_func (text_tool->layer,
                                                clay_text_tool_layer_notify,
                                                text_tool);

          /*  don't try to remove the layer if it is not attached,
           *  which can happen if we got here because the layer was
           *  somehow deleted from the image (like by the user in the
           *  layers dialog).
           */
          if (clay_item_is_attached (CLAY_ITEM (text_tool->layer)))
            clay_text_tool_remove_empty_text_layer (text_tool);
        }

      text_tool->layer = layer;

      if (layer)
        {
          g_signal_connect_object (text_tool->layer, "notify",
                                   G_CALLBACK (clay_text_tool_layer_notify),
                                   text_tool, 0);
        }
    }
}

static void
clay_text_tool_layer_notify (ClayTextLayer    *layer,
                             const GParamSpec *pspec,
                             ClayTextTool     *text_tool)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  if (! strcmp (pspec->name, "modified"))
    {
      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
        clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "text"))
    {
      if (! layer->text)
        clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
    }
  else if (! strcmp (pspec->name, "offset-x") ||
           ! strcmp (pspec->name, "offset-y"))
    {
      if (clay_item_is_attached (CLAY_ITEM (layer)))
        {
          clay_text_tool_block_drawing (text_tool);

          clay_text_tool_frame_item (text_tool);

          clay_text_tool_unblock_drawing (text_tool);
        }
    }
}

static gboolean
clay_text_tool_apply_idle (ClayTextTool *text_tool)
{
  text_tool->idle_id = 0;

  clay_text_tool_apply (text_tool, TRUE);

  clay_text_tool_unblock_drawing (text_tool);

  return G_SOURCE_REMOVE;
}

static void
clay_text_tool_proxy_notify (ClayText         *text,
                             const GParamSpec *pspec,
                             ClayTextTool     *text_tool)
{
  if (! text_tool->text)
    return;

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE &&
      pspec->owner_type == CLAY_TYPE_TEXT)
    {
      if (text_tool->preedit_active)
        {
          /* if there is a preedit going on, don't queue pending
           * changes to be idle-applied with undo; instead, flush the
           * pending queue (happens only when preedit starts), and
           * apply the changes to text_tool->text directly. Preedit
           * will *always* end by removing the preedit string, and if
           * the preedit was committed, it will insert the resulting
           * text, which will not trigger this if() any more.
           */

          GList *list = NULL;

         /* if there are pending changes, apply them before applying
           * preedit stuff directly (bypassing undo)
           */
          if (text_tool->pending)
            {
              clay_text_tool_block_drawing (text_tool);
              clay_text_tool_apply (text_tool, TRUE);
              clay_text_tool_unblock_drawing (text_tool);
            }

          clay_text_tool_block_drawing (text_tool);

          list = g_list_append (list, (gpointer) pspec);
          clay_text_tool_apply_list (text_tool, list);
          g_list_free (list);

          clay_text_tool_frame_item (text_tool);

          clay_image_flush (clay_item_get_image (CLAY_ITEM (text_tool->layer)));

          clay_text_tool_unblock_drawing (text_tool);
        }
      else
        {
          /* else queue the property change for normal processing,
           * including undo
           */

          text_tool->pending = g_list_append (text_tool->pending,
                                              (gpointer) pspec);

          if (! text_tool->idle_id)
            {
              clay_text_tool_block_drawing (text_tool);

              text_tool->idle_id =
                g_idle_add_full (G_PRIORITY_LOW,
                                 (GSourceFunc) clay_text_tool_apply_idle,
                                 text_tool,
                                 NULL);
            }
        }
    }
}

static void
clay_text_tool_text_notify (ClayText         *text,
                            const GParamSpec *pspec,
                            ClayTextTool     *text_tool)
{
  g_return_if_fail (text == text_tool->text);

  /* an undo cancels all preedit operations */
  if (text_tool->preedit_active)
    clay_text_tool_abort_im_context (text_tool);

  clay_text_tool_block_drawing (text_tool);

  if ((pspec->flags & G_PARAM_READWRITE) == G_PARAM_READWRITE)
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (text), pspec->name, &value);

      g_signal_handlers_block_by_func (text_tool->proxy,
                                       clay_text_tool_proxy_notify,
                                       text_tool);

      g_object_set_property (G_OBJECT (text_tool->proxy), pspec->name, &value);

      g_signal_handlers_unblock_by_func (text_tool->proxy,
                                         clay_text_tool_proxy_notify,
                                         text_tool);

      g_value_unset (&value);
    }

  /* if the text has changed, (probably because of an undo), we put
   * the new text into the text buffer
   */
  if (strcmp (pspec->name, "text")   == 0 ||
      strcmp (pspec->name, "markup") == 0)
    {
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_begin_edit,
                                       text_tool);
      g_signal_handlers_block_by_func (text_tool->buffer,
                                       clay_text_tool_buffer_end_edit,
                                       text_tool);

      if (text->markup)
        clay_text_buffer_set_markup (text_tool->buffer, text->markup);
      else
        clay_text_buffer_set_text (text_tool->buffer, text->text);

      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_end_edit,
                                         text_tool);
      g_signal_handlers_unblock_by_func (text_tool->buffer,
                                         clay_text_tool_buffer_begin_edit,
                                         text_tool);
    }

  clay_text_tool_unblock_drawing (text_tool);
}

static void
clay_text_tool_text_changed (ClayText     *text,
                             ClayTextTool *text_tool)
{
  clay_text_tool_block_drawing (text_tool);

  /* we need to redraw the rectangle in any case because whatever
   * changes to the text can change its size
   */
  clay_text_tool_frame_item (text_tool);

  clay_text_tool_unblock_drawing (text_tool);
}

static void
clay_text_tool_fonts_async_set_empty_notify (ClayAsyncSet *async_set,
                                             GParamSpec   *pspec,
                                             ClayTextTool *text_tool)
{
  ClayTool *tool = CLAY_TOOL (text_tool);

  if (! clay_async_set_is_empty (async_set) && tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
}

static void
clay_text_tool_apply_list (ClayTextTool *text_tool,
                           GList        *pspecs)
{
  GObject *src  = G_OBJECT (text_tool->proxy);
  GObject *dest = G_OBJECT (text_tool->text);
  GList   *list;

  g_signal_handlers_block_by_func (dest,
                                   clay_text_tool_text_notify,
                                   text_tool);
  g_signal_handlers_block_by_func (dest,
                                   clay_text_tool_text_changed,
                                   text_tool);

  g_object_freeze_notify (dest);

  for (list = pspecs; list; list = g_list_next (list))
    {
      const GParamSpec *pspec;
      GValue            value = G_VALUE_INIT;

      /*  look ahead and compress changes  */
      if (list->next && list->next->data == list->data)
        continue;

      pspec = list->data;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (src,  pspec->name, &value);
      g_object_set_property (dest, pspec->name, &value);

      g_value_unset (&value);
    }

  g_object_thaw_notify (dest);

  g_signal_handlers_unblock_by_func (dest,
                                     clay_text_tool_text_notify,
                                     text_tool);
  g_signal_handlers_unblock_by_func (dest,
                                     clay_text_tool_text_changed,
                                     text_tool);
}

static void
clay_text_tool_create_layer (ClayTextTool *text_tool,
                             ClayText     *text)
{
  ClayTool  *tool  = CLAY_TOOL (text_tool);
  ClayImage *image = clay_display_get_image (tool->display);
  ClayLayer *layer;
  gdouble    x1, y1;
  gdouble    x2, y2;

  clay_text_tool_block_drawing (text_tool);

  if (text)
    {
      text = clay_config_duplicate (CLAY_CONFIG (text));
    }
  else
    {
      gchar *string;

      if (clay_text_buffer_has_markup (text_tool->buffer))
        {
          string = clay_text_buffer_get_markup (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "markup",   string,
                        "box-mode", CLAY_TEXT_BOX_DYNAMIC,
                        NULL);
        }
      else
        {
          string = clay_text_buffer_get_text (text_tool->buffer);

          g_object_set (text_tool->proxy,
                        "text",     string,
                        "box-mode", CLAY_TEXT_BOX_DYNAMIC,
                        NULL);
        }

      g_free (string);

      text = clay_config_duplicate (CLAY_CONFIG (text_tool->proxy));
    }

  layer = clay_text_layer_new (image, text);

  g_object_unref (text);

  if (! layer)
    {
      clay_text_tool_unblock_drawing (text_tool);
      return;
    }

  clay_text_tool_connect (text_tool, CLAY_TEXT_LAYER (layer), text);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  if (clay_image_get_floating_selection (image))
    {
      g_signal_handlers_block_by_func (image,
                                       clay_text_tool_layer_changed,
                                       text_tool);

      floating_sel_anchor (clay_image_get_floating_selection (image));

      g_signal_handlers_unblock_by_func (image,
                                         clay_text_tool_layer_changed,
                                         text_tool);
    }

  g_object_get (text_tool->widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (text_tool->text_box_fixed == FALSE)
    {
      if (text_tool->text &&
          (text_tool->text->base_dir == CLAY_TEXT_DIRECTION_TTB_RTL ||
           text_tool->text->base_dir == CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          x1 -= clay_item_get_width (CLAY_ITEM (layer));
        }
    }
  clay_item_set_offset (CLAY_ITEM (layer), x1, y1);

  clay_image_add_layer (image, layer,
                        CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  if (text_tool->text_box_fixed)
    {
      ClayUnit *box_unit = text_tool->proxy->box_unit;
      gdouble   xres, yres;

      clay_image_get_resolution (image, &xres, &yres);

      g_object_set (text_tool->proxy,
                    "box-mode",   CLAY_TEXT_BOX_FIXED,
                    "box-width",  clay_pixels_to_units (x2 - x1,
                                                        box_unit, xres),
                    "box-height", clay_pixels_to_units (y2 - y1,
                                                        box_unit, yres),
                    NULL);

      clay_text_tool_apply (text_tool, TRUE); /* unblocks drawing */
    }
  else
    {
      clay_text_tool_frame_item (text_tool);
    }

  clay_image_undo_group_end (image);

  clay_image_flush (image);

  clay_text_tool_set_drawable (text_tool, CLAY_DRAWABLE (layer), FALSE);

  clay_text_tool_unblock_drawing (text_tool);
}

#define  RESPONSE_NEW 1

static void
clay_text_tool_confirm_response (GtkWidget    *widget,
                                 gint          response_id,
                                 ClayTextTool *text_tool)
{
  ClayTextLayer *layer = text_tool->layer;

  gtk_widget_destroy (widget);

  if (layer && layer->text)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          clay_text_tool_create_layer (text_tool, layer->text);
          break;

        case GTK_RESPONSE_ACCEPT:
          clay_text_tool_connect (text_tool, layer, layer->text);

          /*  cause the text layer to be rerendered  */
          g_object_notify (G_OBJECT (text_tool->proxy), "markup");

          clay_text_tool_editor_start (text_tool);
          break;

        default:
          break;
        }
    }
}

static void
clay_text_tool_confirm_dialog (ClayTextTool *text_tool)
{
  ClayTool         *tool  = CLAY_TOOL (text_tool);
  ClayDisplayShell *shell = clay_display_get_shell (tool->display);
  GtkWidget        *dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;

  g_return_if_fail (text_tool->layer != NULL);

  if (text_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (text_tool->confirm_dialog));
      return;
    }

  dialog = clay_viewable_dialog_new (g_list_prepend (NULL, text_tool->layer),
                                     CLAY_CONTEXT (clay_tool_get_options (tool)),
                                     _("Confirm Text Editing"),
                                     "clay-text-tool-confirm",
                                     CLAY_ICON_LAYER_TEXT_LAYER,
                                     _("Confirm Text Editing"),
                                     GTK_WIDGET (shell),
                                     clay_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     _("_Cancel"),           GTK_RESPONSE_CANCEL,
                                     _("_Edit Anyway"),      GTK_RESPONSE_ACCEPT,

                                     NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_NEW,
                                            GTK_RESPONSE_ACCEPT,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (clay_text_tool_confirm_response),
                    text_tool);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("The text layer you picked was rasterized. "
                           "Editing its text will discard any modifications."));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  gtk_widget_set_visible (dialog, TRUE);

  text_tool->confirm_dialog = dialog;
  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_nullify_pointer),
                            &text_tool->confirm_dialog);
}

static void
clay_text_tool_layer_changed (ClayImage    *image,
                              ClayTextTool *text_tool)
{
  GList *layers = clay_image_get_selected_layers (image);

  if (g_list_length (layers) != 1 || layers->data != text_tool->layer)
    {
      ClayTool    *tool    = CLAY_TOOL (text_tool);
      ClayDisplay *display = tool->display;

      if (display)
        {
          ClayLayer *layer = NULL;

          clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

          if (g_list_length (layers) == 1)
            layer = layers->data;

          /* The tool can only be started when a single layer is
           * selected and this is a text layer.
           */
          if (layer &&
              clay_text_tool_set_drawable (text_tool, CLAY_DRAWABLE (layer),
                                           FALSE) &&
              CLAY_LAYER (text_tool->layer) == layer)
            {
              GError *error = NULL;

              if (! clay_text_tool_start (text_tool, display, layer, &error))
                {
                  clay_text_tool_set_drawable (text_tool, NULL, FALSE);

                  clay_tool_message_literal (tool, display, error->message);

                  g_clear_error (&error);

                  return;
                }
            }
        }
    }
}

static void
clay_text_tool_set_image (ClayTextTool *text_tool,
                          ClayImage    *image)
{
  if (text_tool->image == image)
    return;

  if (text_tool->image)
    {
      g_signal_handlers_disconnect_by_func (text_tool->image,
                                            clay_text_tool_layer_changed,
                                            text_tool);
    }

  g_set_weak_pointer (&text_tool->image, image);

  if (image)
    {
      ClayTextOptions *options = CLAY_TEXT_TOOL_GET_OPTIONS (text_tool);
      gdouble          xres;
      gdouble          yres;

      g_signal_connect_object (text_tool->image, "selected-layers-changed",
                               G_CALLBACK (clay_text_tool_layer_changed),
                               text_tool, 0);

      clay_image_get_resolution (image, &xres, &yres);
      clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (options->size_entry), 0,
                                      yres, FALSE);
    }
}

static gboolean
clay_text_tool_set_drawable (ClayTextTool *text_tool,
                             ClayDrawable *drawable,
                             gboolean      confirm)
{
  ClayImage *image = NULL;

  if (text_tool->confirm_dialog)
    gtk_widget_destroy (text_tool->confirm_dialog);

  if (drawable)
    image = clay_item_get_image (CLAY_ITEM (drawable));

  clay_text_tool_set_image (text_tool, image);

  if (CLAY_IS_TEXT_LAYER (drawable) && CLAY_TEXT_LAYER (drawable)->text)
    {
      ClayTextLayer *layer = CLAY_TEXT_LAYER (drawable);

      if (layer == text_tool->layer && layer->text == text_tool->text)
        return TRUE;

      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
        {
          if (confirm)
            {
              clay_text_tool_connect (text_tool, layer, NULL);
              clay_text_tool_confirm_dialog (text_tool);
              return TRUE;
            }
        }
      else
        {
          clay_text_tool_connect (text_tool, layer, layer->text);
          return TRUE;
        }
    }

  clay_text_tool_connect (text_tool, NULL, NULL);

  return FALSE;
}

static void
clay_text_tool_block_drawing (ClayTextTool *text_tool)
{
  if (text_tool->drawing_blocked == 0)
    {
      clay_draw_tool_pause (CLAY_DRAW_TOOL (text_tool));

      clay_text_tool_clear_layout (text_tool);
    }

  text_tool->drawing_blocked++;
}

static void
clay_text_tool_unblock_drawing (ClayTextTool *text_tool)
{
  g_return_if_fail (text_tool->drawing_blocked > 0);

  text_tool->drawing_blocked--;

  if (text_tool->drawing_blocked == 0)
    clay_draw_tool_resume (CLAY_DRAW_TOOL (text_tool));
}

static void
clay_text_tool_buffer_begin_edit (ClayTextBuffer *buffer,
                                  ClayTextTool   *text_tool)
{
  clay_text_tool_block_drawing (text_tool);
}

static void
clay_text_tool_buffer_end_edit (ClayTextBuffer *buffer,
                                ClayTextTool   *text_tool)
{
  if (text_tool->text)
    {
      gchar *string;

      if (clay_text_buffer_has_markup (buffer))
        {
          string = clay_text_buffer_get_markup (buffer);

          g_object_set (text_tool->proxy,
                        "markup", string,
                        NULL);
        }
      else
        {
          string = clay_text_buffer_get_text (buffer);

          g_object_set (text_tool->proxy,
                        "text", string,
                        NULL);
        }

      g_free (string);
    }
  else
    {
      clay_text_tool_create_layer (text_tool, NULL);
    }

  clay_text_tool_unblock_drawing (text_tool);
}

static void
clay_text_tool_buffer_color_applied (ClayTextBuffer *buffer,
                                     GeglColor      *color,
                                     ClayTextTool   *text_tool)
{
  clay_palettes_add_color_history (CLAY_TOOL (text_tool)->tool_info->clay, color);
}


/*  public functions  */

void
clay_text_tool_clear_layout (ClayTextTool *text_tool)
{
  g_clear_object (&text_tool->layout);
}

gboolean
clay_text_tool_ensure_layout (ClayTextTool *text_tool)
{
  if (! text_tool->layout && text_tool->text)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (text_tool->layer));
      gdouble    xres;
      gdouble    yres;
      GError    *error = NULL;

      clay_image_get_resolution (image, &xres, &yres);

      text_tool->layout = clay_text_layout_new (text_tool->layer->text,
                                                image, xres, yres, &error);
      if (error)
        {
          clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  return text_tool->layout != NULL;
}

void
clay_text_tool_apply (ClayTextTool *text_tool,
                      gboolean      push_undo)
{
  const GParamSpec *pspec = NULL;
  ClayImage        *image;
  ClayTextLayer    *layer;
  GList            *list;
  gboolean          undo_group = FALSE;

  if (text_tool->idle_id)
    {
      g_source_remove (text_tool->idle_id);
      text_tool->idle_id = 0;

      clay_text_tool_unblock_drawing (text_tool);
    }

  g_return_if_fail (text_tool->text != NULL);
  g_return_if_fail (text_tool->layer != NULL);

  layer = text_tool->layer;
  image = clay_item_get_image (CLAY_ITEM (layer));

  g_return_if_fail (layer->text == text_tool->text);

  /*  Walk over the list of changes and figure out if we are changing
   *  a single property or need to push a full text undo.
   */
  for (list = text_tool->pending;
       list && list->next && list->next->data == list->data;
       list = list->next)
    /* do nothing */;

  if (g_list_length (list) == 1)
    pspec = list->data;

  /*  If we are changing a single property, we don't need to push
   *  an undo if all of the following is true:
   *   - the redo stack is empty
   *   - the last item on the undo stack is a text undo
   *   - the last undo changed the same text property on the same layer
   *   - the last undo happened less than TEXT_UNDO_TIMEOUT seconds ago
   */
  if (pspec)
    {
      ClayUndo *undo = clay_image_undo_can_compress (image, CLAY_TYPE_TEXT_UNDO,
                                                     CLAY_UNDO_TEXT_LAYER);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (layer))
        {
          ClayTextUndo *text_undo = CLAY_TEXT_UNDO (undo);

          if (text_undo->pspec == pspec)
            {
              if (clay_undo_get_age (undo) < TEXT_UNDO_TIMEOUT)
                {
                  ClayTool    *tool = CLAY_TOOL (text_tool);
                  ClayContext *context;

                  context = CLAY_CONTEXT (clay_tool_get_options (tool));

                  push_undo = FALSE;
                  clay_undo_reset_age (undo);
                  clay_undo_refresh_preview (undo, context);
                }
            }
        }
    }

  if (push_undo)
    {
      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
        {
          undo_group = TRUE;
          clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TEXT, NULL);

          /*  see comment in clay_text_layer_set()  */
          clay_image_undo_push_drawable_mod (image, NULL,
                                             CLAY_DRAWABLE (layer), TRUE);
        }

      clay_image_undo_push_text_layer (image, NULL, layer, pspec);
    }

  clay_text_tool_apply_list (text_tool, list);

  g_list_free (text_tool->pending);
  text_tool->pending = NULL;

  if (undo_group)
    {
      clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));
      clay_image_undo_group_end (image);
    }

  clay_text_tool_frame_item (text_tool);

  clay_image_flush (image);
}

gboolean
clay_text_tool_set_layer (ClayTextTool *text_tool,
                          ClayLayer    *layer)
{
  g_return_val_if_fail (CLAY_IS_TEXT_TOOL (text_tool), FALSE);
  g_return_val_if_fail (layer == NULL || CLAY_IS_LAYER (layer), FALSE);

  if (layer == CLAY_LAYER (text_tool->layer))
    return TRUE;

  /*  FIXME this function works, and I have no clue why: first we set
   *  the drawable, then we HALT the tool and start() it without
   *  re-setting the drawable. Why this works perfectly anyway when
   *  double clicking a text layer in the layers dialog... no idea.
   */
  if (clay_text_tool_set_drawable (text_tool, CLAY_DRAWABLE (layer), TRUE))
    {
      ClayTool    *tool = CLAY_TOOL (text_tool);
      ClayItem    *item = CLAY_ITEM (layer);
      ClayContext *context;
      ClayDisplay *display;

      context = clay_get_user_context (tool->tool_info->clay);
      display = clay_context_get_display (context);

      if (! display ||
          clay_display_get_image (display) != clay_item_get_image (item))
        {
          GList *list;

          display = NULL;

          for (list = clay_get_display_iter (tool->tool_info->clay);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (clay_display_get_image (display) == clay_item_get_image (item))
                {
                  clay_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      if (tool->display)
        clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

      if (display)
        {
          GError *error = NULL;

          if (! clay_text_tool_start (text_tool, display, layer, &error))
            {
              clay_text_tool_set_drawable (text_tool, NULL, FALSE);

              clay_tool_message_literal (tool, display, error->message);

              g_clear_error (&error);

              return FALSE;
            }

          g_list_free (tool->drawables);
          tool->drawables = g_list_prepend (NULL, CLAY_DRAWABLE (layer));
        }
    }

  return TRUE;
}

gboolean
clay_text_tool_get_has_text_selection (ClayTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  return gtk_text_buffer_get_has_selection (buffer);
}

void
clay_text_tool_delete_selection (ClayTextTool *text_tool)
{
  GtkTextBuffer *buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
    }
}

void
clay_text_tool_cut_clipboard (ClayTextTool *text_tool)
{
  ClayDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));

  shell = clay_display_get_shell (CLAY_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_cut_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                 clipboard, TRUE);
}

void
clay_text_tool_copy_clipboard (ClayTextTool *text_tool)
{
  ClayDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));

  shell = clay_display_get_shell (CLAY_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  /*  need to block "end-user-action" on the text buffer, because
   *  GtkTextBuffer considers copying text to the clipboard an
   *  undo-relevant user action, which is clearly a bug, but what
   *  can we do...
   */
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   clay_text_tool_buffer_begin_edit,
                                   text_tool);
  g_signal_handlers_block_by_func (text_tool->buffer,
                                   clay_text_tool_buffer_end_edit,
                                   text_tool);

  gtk_text_buffer_copy_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                  clipboard);

  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     clay_text_tool_buffer_end_edit,
                                     text_tool);
  g_signal_handlers_unblock_by_func (text_tool->buffer,
                                     clay_text_tool_buffer_begin_edit,
                                     text_tool);
}

void
clay_text_tool_paste_clipboard (ClayTextTool *text_tool)
{
  ClayDisplayShell *shell;
  GtkClipboard     *clipboard;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));

  shell = clay_display_get_shell (CLAY_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  gtk_text_buffer_paste_clipboard (GTK_TEXT_BUFFER (text_tool->buffer),
                                   clipboard, NULL, TRUE);
}

void
clay_text_tool_toggle_tag (ClayTextTool *text_tool,
                           GtkTextTag   *tag)
{
  GtkTextBuffer *buffer;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));
  g_return_if_fail (GTK_IS_TEXT_BUFFER (text_tool->buffer));

  buffer = GTK_TEXT_BUFFER (text_tool->buffer);

  if (gtk_text_buffer_get_has_selection (buffer))
    {
      GtkTextIter start;
      GtkTextIter end;
      GtkTextIter iter;
      gboolean    is_tag_active = FALSE;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);

      iter = start;
      while (! gtk_text_iter_equal (&iter, &end))
        {
          if (gtk_text_iter_has_tag (&iter, tag))
            {
              is_tag_active = TRUE;
              break;
            }
          gtk_text_iter_forward_char (&iter);
        }

      gtk_text_buffer_begin_user_action (buffer);

      if (is_tag_active)
        gtk_text_buffer_remove_tag (buffer, tag, &start, &end);
      else
        gtk_text_buffer_apply_tag (buffer, tag, &start, &end);

      gtk_text_buffer_end_user_action (buffer);
    }
}

void
clay_text_tool_paste_clipboard_unformatted (ClayTextTool *text_tool)
{
  ClayDisplayShell *shell;
  GtkClipboard     *clipboard;
  gchar            *unformatted_text;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));

  shell = clay_display_get_shell (CLAY_TOOL (text_tool)->display);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (shell),
                                        GDK_SELECTION_CLIPBOARD);

  unformatted_text = gtk_clipboard_wait_for_text (clipboard);

  if (unformatted_text)
    {
      /* First delete text in the current selection (possibly empty),
       * in order to allow overwriting. */
      gtk_text_buffer_delete_selection (GTK_TEXT_BUFFER (text_tool->buffer),
                                        TRUE, TRUE);

      clay_text_buffer_insert (text_tool->buffer, unformatted_text);
    }

  g_free (unformatted_text);
}

void
clay_text_tool_create_path (ClayTextTool *text_tool)
{
  ClayPath *path;

  g_return_if_fail (CLAY_IS_TEXT_TOOL (text_tool));

  if (! text_tool->text || ! text_tool->image)
    return;

  path = clay_text_path_new (text_tool->image, text_tool->text);

  if (text_tool->layer)
    {
      gint x, y;

      clay_item_get_offset (CLAY_ITEM (text_tool->layer), &x, &y);
      clay_item_translate (CLAY_ITEM (path), x, y, FALSE);
    }

  clay_image_add_path (text_tool->image, path,
                          CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  clay_image_flush (text_tool->image);
}

gboolean
clay_text_tool_create_path_warped (ClayTextTool  *text_tool,
                                   GError       **error)
{
  GList             *paths0;
  ClayPath          *path;
  gdouble            box_width;
  gdouble            box_height;
  ClayTextDirection  dir;
  gdouble            offset = 0.0;

  g_return_val_if_fail (CLAY_IS_TEXT_TOOL (text_tool), FALSE);

  if (! text_tool->text || ! text_tool->image || ! text_tool->layer)
    {
      if (! text_tool->text)
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                             _("Text is required."));
      if (! text_tool->image)
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                             _("No image."));
      if (! text_tool->layer)
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                             _("No layer."));
      return FALSE;
    }

  box_width  = clay_item_get_width  (CLAY_ITEM (text_tool->layer));
  box_height = clay_item_get_height (CLAY_ITEM (text_tool->layer));

  paths0 = clay_image_get_selected_paths (text_tool->image);
  if (g_list_length (paths0) != 1)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Exactly one path must be selected."));
      return FALSE;
    }

  path = clay_text_path_new (text_tool->image, text_tool->text);

  offset = 0;
  dir = clay_text_tool_get_direction (text_tool);
  switch (dir)
    {
    case CLAY_TEXT_DIRECTION_LTR:
    case CLAY_TEXT_DIRECTION_RTL:
      offset = 0.5 * box_height;
      break;
    case CLAY_TEXT_DIRECTION_TTB_RTL:
    case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
    case CLAY_TEXT_DIRECTION_TTB_LTR:
    case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      {
        ClayStroke *stroke = NULL;

        while ((stroke = clay_path_stroke_get_next (path, stroke)))
          {
            clay_stroke_rotate (stroke, 0, 0, 270);
            clay_stroke_translate (stroke, 0, box_width);
          }
      }
      offset = 0.5 * box_width;
      break;
    }

  clay_path_warp_path (paths0->data, path, offset);

  clay_item_set_visible (CLAY_ITEM (path), TRUE, FALSE);

  clay_image_add_path (text_tool->image, path,
                       CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  clay_image_flush (text_tool->image);

  return TRUE;
}

ClayTextDirection
clay_text_tool_get_direction  (ClayTextTool *text_tool)
{
  ClayTextOptions *options = CLAY_TEXT_TOOL_GET_OPTIONS (text_tool);
  return options->base_dir;
}

void
clay_text_tool_restore_on_canvas_editor_position (ClayTextTool *text_tool)
{
  ClayTextStyleEditor *editor = CLAY_TEXT_STYLE_EDITOR (text_tool->style_editor);

  if (text_tool->layer)
    clay_text_layer_set_style_overlay_position (text_tool->layer, FALSE, 0, 0);

  clay_text_tool_editor_position (text_tool);
  clay_text_style_show_restore_position_button (editor, FALSE);
}
