/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayForegroundSelectTool
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-mask.h"
#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/claychannel-select.h"
#include "core/claydrawable-foreground-extract.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/clayprogress.h"
#include "core/clayscanconvert.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claycanvasbufferpreview.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolgui.h"

#include "clayforegroundselecttool.h"
#include "clayforegroundselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


#define FAR_OUTSIDE -10000


typedef struct _StrokeUndo StrokeUndo;

struct _StrokeUndo
{
  GeglBuffer          *saved_trimap;
  gint                 trimap_x;
  gint                 trimap_y;
  ClayMattingDrawMode  draw_mode;
  gint                 stroke_width;
};


static void   clay_foreground_select_tool_finalize       (GObject          *object);

static gboolean  clay_foreground_select_tool_initialize  (ClayTool         *tool,
                                                          ClayDisplay      *display,
                                                          GError          **error);
static void   clay_foreground_select_tool_control        (ClayTool         *tool,
                                                          ClayToolAction    action,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_button_press   (ClayTool         *tool,
                                                          const ClayCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          ClayButtonPressType press_type,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_button_release (ClayTool         *tool,
                                                          const ClayCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          ClayButtonReleaseType release_type,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_motion         (ClayTool         *tool,
                                                          const ClayCoords *coords,
                                                          guint32           time,
                                                          GdkModifierType   state,
                                                          ClayDisplay      *display);
static gboolean  clay_foreground_select_tool_key_press   (ClayTool         *tool,
                                                          GdkEventKey      *kevent,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_modifier_key   (ClayTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_active_modifier_key
                                                         (ClayTool         *tool,
                                                          GdkModifierType   key,
                                                          gboolean          press,
                                                          GdkModifierType   state,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_oper_update    (ClayTool         *tool,
                                                          const ClayCoords *coords,
                                                          GdkModifierType   state,
                                                          gboolean          proximity,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_cursor_update  (ClayTool         *tool,
                                                          const ClayCoords *coords,
                                                          GdkModifierType   state,
                                                          ClayDisplay      *display);
static const gchar * clay_foreground_select_tool_can_undo
                                                         (ClayTool         *tool,
                                                          ClayDisplay      *display);
static const gchar * clay_foreground_select_tool_can_redo
                                                         (ClayTool         *tool,
                                                          ClayDisplay      *display);
static gboolean   clay_foreground_select_tool_undo       (ClayTool         *tool,
                                                          ClayDisplay      *display);
static gboolean   clay_foreground_select_tool_redo       (ClayTool         *tool,
                                                          ClayDisplay      *display);
static void   clay_foreground_select_tool_options_notify (ClayTool         *tool,
                                                          ClayToolOptions  *options,
                                                          const GParamSpec *pspec);

static void   clay_foreground_select_tool_draw           (ClayDrawTool     *draw_tool);

static void   clay_foreground_select_tool_confirm        (ClayPolygonSelectTool *poly_sel,
                                                          ClayDisplay           *display);

static void   clay_foreground_select_tool_halt           (ClayForegroundSelectTool *fg_select);
static void   clay_foreground_select_tool_commit         (ClayForegroundSelectTool *fg_select);

static void   clay_foreground_select_tool_set_trimap     (ClayForegroundSelectTool *fg_select);
static void   clay_foreground_select_tool_set_preview    (ClayForegroundSelectTool *fg_select);
static void   clay_foreground_select_tool_preview        (ClayForegroundSelectTool *fg_select);

static void   clay_foreground_select_tool_stroke_paint   (ClayForegroundSelectTool *fg_select);
static void   clay_foreground_select_tool_cancel_paint   (ClayForegroundSelectTool *fg_select);

static void   clay_foreground_select_tool_response       (ClayToolGui              *gui,
                                                          gint                      response_id,
                                                          ClayForegroundSelectTool *fg_select);
static void   clay_foreground_select_tool_preview_toggled(GtkToggleButton          *button,
                                                          ClayForegroundSelectTool *fg_select);

static void   clay_foreground_select_tool_update_gui     (ClayForegroundSelectTool *fg_select);

static gboolean  clay_foreground_select_tool_has_strokes (ClayForegroundSelectTool *fg_select);

static StrokeUndo * clay_foreground_select_undo_new      (GeglBuffer               *trimap,
                                                          GArray                   *stroke,
                                                          ClayMattingDrawMode       draw_mode,
                                                          gint                      stroke_width);
static void         clay_foreground_select_undo_pop      (StrokeUndo               *undo,
                                                          GeglBuffer               *trimap);
static void         clay_foreground_select_undo_free     (StrokeUndo               *undo);


G_DEFINE_TYPE (ClayForegroundSelectTool, clay_foreground_select_tool,
               CLAY_TYPE_POLYGON_SELECT_TOOL)

#define parent_class clay_foreground_select_tool_parent_class


void
clay_foreground_select_tool_register (ClayToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (CLAY_TYPE_FOREGROUND_SELECT_TOOL,
                CLAY_TYPE_FOREGROUND_SELECT_OPTIONS,
                clay_foreground_select_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-foreground-select-tool",
                _("Foreground Select"),
                _("Foreground Select Tool: Select a region containing foreground objects"),
                N_("F_oreground Select"), NULL,
                NULL, CLAY_HELP_TOOL_FOREGROUND_SELECT,
                CLAY_ICON_TOOL_FOREGROUND_SELECT,
                data);
}

static void
clay_foreground_select_tool_class_init (ClayForegroundSelectToolClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass              *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass          *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);
  ClayPolygonSelectToolClass *polygon_select_tool_class;

  polygon_select_tool_class = CLAY_POLYGON_SELECT_TOOL_CLASS (klass);

  object_class->finalize             = clay_foreground_select_tool_finalize;

  tool_class->initialize             = clay_foreground_select_tool_initialize;
  tool_class->control                = clay_foreground_select_tool_control;
  tool_class->button_press           = clay_foreground_select_tool_button_press;
  tool_class->button_release         = clay_foreground_select_tool_button_release;
  tool_class->motion                 = clay_foreground_select_tool_motion;
  tool_class->key_press              = clay_foreground_select_tool_key_press;
  tool_class->modifier_key           = clay_foreground_select_tool_modifier_key;
  tool_class->active_modifier_key    = clay_foreground_select_tool_active_modifier_key;
  tool_class->oper_update            = clay_foreground_select_tool_oper_update;
  tool_class->cursor_update          = clay_foreground_select_tool_cursor_update;
  tool_class->can_undo               = clay_foreground_select_tool_can_undo;
  tool_class->can_redo               = clay_foreground_select_tool_can_redo;
  tool_class->undo                   = clay_foreground_select_tool_undo;
  tool_class->redo                   = clay_foreground_select_tool_redo;
  tool_class->options_notify         = clay_foreground_select_tool_options_notify;

  draw_tool_class->draw              = clay_foreground_select_tool_draw;

  polygon_select_tool_class->confirm = clay_foreground_select_tool_confirm;
}

static void
clay_foreground_select_tool_init (ClayForegroundSelectTool *fg_select)
{
  ClayTool *tool = CLAY_TOOL (fg_select);

  clay_tool_control_set_motion_mode (tool->control, CLAY_MOTION_MODE_EXACT);
  clay_tool_control_set_scroll_lock (tool->control, FALSE);
  clay_tool_control_set_preserve    (tool->control, FALSE);
  clay_tool_control_set_dirty_mask  (tool->control,
                                     CLAY_DIRTY_IMAGE_SIZE |
                                     CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_precision   (tool->control,
                                     CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor (tool->control,
                                     CLAY_TOOL_CURSOR_FREE_SELECT);

  clay_tool_control_set_action_size (tool->control,
                                     "tools-foreground-select-brush-size-set");

  fg_select->state = MATTING_STATE_FREE_SELECT;
  fg_select->grayscale_preview = NULL;
}

static void
clay_foreground_select_tool_finalize (GObject *object)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (object);

  g_clear_object (&fg_select->gui);
  fg_select->preview_toggle = NULL;

  if (fg_select->stroke)
    g_warning ("%s: stroke should be NULL at this point", G_STRLOC);

  if (fg_select->mask)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  if (fg_select->trimap)
    g_warning ("%s: mask should be NULL at this point", G_STRLOC);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_foreground_select_tool_initialize (ClayTool     *tool,
                                        ClayDisplay  *display,
                                        GError      **error)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  ClayGuiConfig            *config    = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage                *image     = clay_display_get_image (display);
  ClayDisplayShell         *shell     = clay_display_get_shell (display);
  GList                    *drawables = clay_image_get_selected_drawables (image);
  ClayDrawable             *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                             _("Cannot select from multiple layers."));
      else
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }
  drawable = drawables->data;
  g_list_free (drawables);

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("The active layer is not visible."));
      return FALSE;
    }

  tool->display = display;

  /*  enable double click for the FreeSelectTool, because it may have been
   *  disabled if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in clay_foreground_select_tool_set_trimap().
   */
  clay_tool_control_set_wants_double_click (tool->control, TRUE);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  if (! fg_select->gui)
    {
      fg_select->gui =
        clay_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Dialog for foreground select"),
                           NULL, NULL,
                           clay_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_Select"), GTK_RESPONSE_APPLY,

                           NULL);

      clay_tool_gui_set_auto_overlay (fg_select->gui, TRUE);

      g_signal_connect (fg_select->gui, "response",
                        G_CALLBACK (clay_foreground_select_tool_response),
                        fg_select);

      fg_select->preview_toggle =
        gtk_check_button_new_with_mnemonic (_("_Preview mask"));
      gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (fg_select->gui)),
                          fg_select->preview_toggle, FALSE, FALSE, 0);
      gtk_widget_set_visible (fg_select->preview_toggle, TRUE);

      g_signal_connect (fg_select->preview_toggle, "toggled",
                        G_CALLBACK (clay_foreground_select_tool_preview_toggled),
                        fg_select);
    }

  clay_tool_gui_set_description (fg_select->gui,
                                 _("Select foreground pixels"));

  clay_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        FALSE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, FALSE);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                FALSE);

  clay_tool_gui_set_shell (fg_select->gui, shell);
  clay_tool_gui_set_viewable (fg_select->gui, CLAY_VIEWABLE (drawable));

  clay_tool_gui_show (fg_select->gui);

  return TRUE;
}

static void
clay_foreground_select_tool_control (ClayTool       *tool,
                                     ClayToolAction  action,
                                     ClayDisplay    *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_foreground_select_tool_halt (fg_select);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_foreground_select_tool_commit (fg_select);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_foreground_select_tool_button_press (ClayTool            *tool,
                                          const ClayCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          ClayButtonPressType  press_type,
                                          ClayDisplay         *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  ClayDrawTool             *draw_tool = CLAY_DRAW_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else
    {
      ClayVector2 point = clay_vector2_new (coords->x, coords->y);

      clay_draw_tool_pause (draw_tool);

      if (clay_draw_tool_is_active (draw_tool) && draw_tool->display != display)
        clay_draw_tool_stop (draw_tool);

      clay_tool_control_activate (tool->control);

      fg_select->last_coords = *coords;

      g_return_if_fail (fg_select->stroke == NULL);
      fg_select->stroke = g_array_new (FALSE, FALSE, sizeof (ClayVector2));

      g_array_append_val (fg_select->stroke, point);

      if (! clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_start (draw_tool, display);

      clay_draw_tool_resume (draw_tool);
    }
}

static void
clay_foreground_select_tool_button_release (ClayTool              *tool,
                                            const ClayCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            ClayButtonReleaseType  release_type,
                                            ClayDisplay           *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
  else
    {
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      clay_tool_control_halt (tool->control);

      if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
        {
          clay_foreground_select_tool_cancel_paint (fg_select);
        }
      else
        {
          clay_foreground_select_tool_stroke_paint (fg_select);

          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            clay_foreground_select_tool_preview (fg_select);
          else
            clay_foreground_select_tool_set_trimap (fg_select);
        }

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static void
clay_foreground_select_tool_motion (ClayTool         *tool,
                                    const ClayCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    ClayDisplay      *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
  else
    {
      ClayVector2 *last = &g_array_index (fg_select->stroke,
                                          ClayVector2,
                                          fg_select->stroke->len - 1);

      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      fg_select->last_coords = *coords;

      if (last->x != (gint) coords->x || last->y != (gint) coords->y)
        {
          ClayVector2 point = clay_vector2_new (coords->x, coords->y);

          g_array_append_val (fg_select->stroke, point);
        }

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static gboolean
clay_foreground_select_tool_key_press (ClayTool    *tool,
                                       GdkEventKey *kevent,
                                       ClayDisplay *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
    }
  else
    {
      if (display != tool->display)
        return FALSE;

      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                          TRUE);
          else
            clay_foreground_select_tool_response (fg_select->gui,
                                                  GTK_RESPONSE_APPLY, fg_select);
          return TRUE;

        case GDK_KEY_Escape:
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            clay_foreground_select_tool_response (fg_select->gui,
                                                  GTK_RESPONSE_CANCEL, fg_select);
          else
            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fg_select->preview_toggle),
                                          FALSE);
          return TRUE;

        default:
          return FALSE;
        }
    }
}

static void
clay_foreground_select_tool_modifier_key (ClayTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          ClayDisplay     *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                    display);
    }
  else
    {
#if 0
      if (key == clay_get_toggle_behavior_mask ())
        {
          ClayForegroundSelectOptions *options;

          options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

          g_object_set (options,
                        "background", ! options->background,
                        NULL);
        }
#endif
    }
}

static void
clay_foreground_select_tool_active_modifier_key (ClayTool        *tool,
                                                 GdkModifierType  key,
                                                 gboolean         press,
                                                 GdkModifierType  state,
                                                 ClayDisplay     *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press,
                                                           state, display);
    }
}

static void
clay_foreground_select_tool_oper_update (ClayTool         *tool,
                                         const ClayCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         ClayDisplay      *display)
{
  ClayForegroundSelectTool    *fg_select    = CLAY_FOREGROUND_SELECT_TOOL (tool);
  ClayForegroundSelectOptions *options;
  const gchar                 *status_stage = NULL;
  const gchar                 *status_mode  = NULL;

  options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      if (CLAY_SELECTION_TOOL (tool)->function == SELECTION_SELECT)
        {
          gint n_points;

          clay_polygon_select_tool_get_points (CLAY_POLYGON_SELECT_TOOL (tool),
                                               NULL, &n_points);

          if (n_points > 2)
            {
              status_mode = _("Roughly outline the object to extract");
              status_stage = _("press Enter to refine.");
            }
          else
            {
              status_stage = _("Roughly outline the object to extract");
            }
        }
    }
  else
    {
      ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

      clay_draw_tool_pause (draw_tool);

      if (proximity)
        {
          fg_select->last_coords = *coords;
        }
      else
        {
          fg_select->last_coords.x = FAR_OUTSIDE;
          fg_select->last_coords.y = FAR_OUTSIDE;
        }

      clay_draw_tool_resume (draw_tool);

      if (options->draw_mode == CLAY_MATTING_DRAW_MODE_FOREGROUND)
        status_mode = _("Selecting foreground");
      else if (options->draw_mode == CLAY_MATTING_DRAW_MODE_BACKGROUND)
        status_mode = _("Selecting background");
      else
        status_mode = _("Selecting unknown");

      if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
        status_stage = _("press Enter to preview.");
      else
        status_stage = _("press Escape to exit preview or Enter to apply.");
    }

  if (proximity && status_stage)
    {
      if (status_mode)
        clay_tool_replace_status (tool, display, "%s, %s", status_mode, status_stage);
      else
        clay_tool_replace_status (tool, display, "%s", status_stage);
    }
}

static void
clay_foreground_select_tool_cursor_update (ClayTool         *tool,
                                           const ClayCoords *coords,
                                           GdkModifierType   state,
                                           ClayDisplay      *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      switch (CLAY_SELECTION_TOOL (tool)->function)
        {
        case SELECTION_MOVE_MASK:
        case SELECTION_MOVE:
        case SELECTION_MOVE_COPY:
        case SELECTION_ANCHOR:
          return;
        default:
          break;
        }
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
clay_foreground_select_tool_can_undo (ClayTool    *tool,
                                      ClayDisplay *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->undo_stack)
    {
      StrokeUndo  *undo = fg_select->undo_stack->data;
      const gchar *desc;

      if (clay_enum_get_value (CLAY_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static const gchar *
clay_foreground_select_tool_can_redo (ClayTool    *tool,
                                      ClayDisplay *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);

  if (fg_select->redo_stack)
    {
      StrokeUndo  *undo = fg_select->redo_stack->data;
      const gchar *desc;

      if (clay_enum_get_value (CLAY_TYPE_MATTING_DRAW_MODE, undo->draw_mode,
                               NULL, NULL, &desc, NULL))
        {
          return desc;
        }
    }

  return NULL;
}

static gboolean
clay_foreground_select_tool_undo (ClayTool    *tool,
                                  ClayDisplay *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->undo_stack->data;

  clay_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->undo_stack = g_list_remove (fg_select->undo_stack, undo);
  fg_select->redo_stack = g_list_prepend (fg_select->redo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    clay_foreground_select_tool_preview (fg_select);
  else
    clay_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static gboolean
clay_foreground_select_tool_redo (ClayTool    *tool,
                                  ClayDisplay *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  StrokeUndo               *undo      = fg_select->redo_stack->data;

  clay_foreground_select_undo_pop (undo, fg_select->trimap);

  fg_select->redo_stack = g_list_remove (fg_select->redo_stack, undo);
  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    clay_foreground_select_tool_preview (fg_select);
  else
    clay_foreground_select_tool_set_trimap (fg_select);

  return TRUE;
}

static void
clay_foreground_select_tool_options_notify (ClayTool         *tool,
                                            ClayToolOptions  *options,
                                            const GParamSpec *pspec)
{
  ClayForegroundSelectTool    *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  ClayForegroundSelectOptions *fg_options;

  fg_options = CLAY_FOREGROUND_SELECT_OPTIONS (options);

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "mask-color") ||
      ! strcmp (pspec->name, "preview-mode"))
    {
      if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
        {
          clay_foreground_select_tool_set_trimap (fg_select);
        }
      else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          clay_foreground_select_tool_set_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "engine"))
    {
      if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
        {
          clay_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "iterations"))
    {
      if (fg_options->engine == CLAY_MATTING_ENGINE_GLOBAL &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          clay_foreground_select_tool_preview (fg_select);
        }
    }
  else if (! strcmp (pspec->name, "levels") ||
           ! strcmp (pspec->name, "active-levels"))
    {
      if (fg_options->engine == CLAY_MATTING_ENGINE_LEVIN &&
          fg_select->state   == MATTING_STATE_PREVIEW_MASK)
        {
          clay_foreground_select_tool_preview (fg_select);
        }
    }
}

static void
clay_foreground_select_tool_get_area (GeglBuffer *mask,
                                      gint       *x1,
                                      gint       *y1,
                                      gint       *x2,
                                      gint       *y2)
{
  gint width;
  gint height;

  clay_gegl_mask_bounds (mask, x1, y1, x2, y2);

  width  = *x2 - *x1;
  height = *y2 - *y1;

  *x1 = MAX (*x1 - width  / 2, 0);
  *y1 = MAX (*y1 - height / 2, 0);
  *x2 = MIN (*x2 + width  / 2, clay_item_get_width  (CLAY_ITEM (mask)));
  *y2 = MIN (*y2 + height / 2, clay_item_get_height (CLAY_ITEM (mask)));
}

static void
clay_foreground_select_tool_draw (ClayDrawTool *draw_tool)
{
  ClayTool                    *tool      = CLAY_TOOL (draw_tool);
  ClayForegroundSelectTool    *fg_select = CLAY_FOREGROUND_SELECT_TOOL (tool);
  ClayForegroundSelectOptions *options;

  options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
      return;
    }
  else
    {
      gint    x      = fg_select->last_coords.x;
      gint    y      = fg_select->last_coords.y;
      gdouble radius = options->stroke_width / 2.0f;

      if (fg_select->stroke)
        {
          ClayDisplayShell *shell = clay_display_get_shell (draw_tool->display);

          clay_draw_tool_add_pen (draw_tool,
                                  (const ClayVector2 *) fg_select->stroke->data,
                                  fg_select->stroke->len,
                                  CLAY_CONTEXT (options),
                                  CLAY_ACTIVE_COLOR_FOREGROUND,
                                  options->stroke_width * shell->scale_y);
        }

      /*  warn if the user is drawing outside of the working area  */
      if (FALSE)
        {
          gint x1, y1;
          gint x2, y2;

          clay_foreground_select_tool_get_area (fg_select->mask,
                                                &x1, &y1, &x2, &y2);

          if (x < x1 + radius || x > x2 - radius ||
              y < y1 + radius || y > y2 - radius)
            {
              clay_draw_tool_add_rectangle (draw_tool, FALSE,
                                            x1, y1,
                                            x2 - x1, y2 - y1);
            }
        }

      if (x > FAR_OUTSIDE && y > FAR_OUTSIDE)
        clay_draw_tool_add_arc (draw_tool, FALSE,
                                x - radius, y - radius,
                                2 * radius, 2 * radius,
                                0.0, 2.0 * G_PI);

      if (fg_select->grayscale_preview)
        clay_draw_tool_add_preview (draw_tool, fg_select->grayscale_preview);
    }
}

static void
clay_foreground_select_tool_confirm (ClayPolygonSelectTool *poly_sel,
                                     ClayDisplay           *display)
{
  ClayForegroundSelectTool *fg_select = CLAY_FOREGROUND_SELECT_TOOL (poly_sel);
  ClayImage                *image     = clay_display_get_image (display);
  GList                    *drawables = clay_image_get_selected_drawables (image);
  ClayDrawable             *drawable;
  ClayItem                 *item;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  item     = CLAY_ITEM (drawable);
  g_list_free (drawables);

  if (drawable && fg_select->state == MATTING_STATE_FREE_SELECT)
    {
      ClayScanConvert   *scan_convert = clay_scan_convert_new ();
      const ClayVector2 *points;
      gint               n_points;

      clay_polygon_select_tool_get_points (poly_sel, &points, &n_points);

      clay_scan_convert_add_polyline (scan_convert, n_points, points, TRUE);

      fg_select->trimap =
        gegl_buffer_new (GEGL_RECTANGLE (clay_item_get_offset_x (item),
                                         clay_item_get_offset_y (item),
                                         clay_item_get_width    (item),
                                         clay_item_get_height   (item)),
                         clay_image_get_mask_format (image));

      clay_scan_convert_render_value (scan_convert, fg_select->trimap,
                                      0, 0, 0.5);
      clay_scan_convert_free (scan_convert);

      fg_select->grayscale_preview =
          clay_canvas_buffer_preview_new (clay_display_get_shell (display),
                                          fg_select->trimap);

      clay_foreground_select_tool_set_trimap (fg_select);
    }
}

static void
clay_foreground_select_tool_halt (ClayForegroundSelectTool *fg_select)
{
  ClayTool     *tool      = CLAY_TOOL (fg_select);
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (fg_select);

  if (draw_tool->preview)
    clay_draw_tool_remove_preview (draw_tool, fg_select->grayscale_preview);

  g_clear_object (&fg_select->grayscale_preview);
  g_clear_object (&fg_select->trimap);
  g_clear_object (&fg_select->mask);

  if (fg_select->undo_stack)
    {
      g_list_free_full (fg_select->undo_stack,
                        (GDestroyNotify) clay_foreground_select_undo_free);
      fg_select->undo_stack = NULL;
    }

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) clay_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  if (tool->display)
    clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                 NULL, 0, 0, NULL, FALSE);

  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_FREE_SELECT);
  clay_tool_control_set_toggle_tool_cursor (tool->control,
                                            CLAY_TOOL_CURSOR_FREE_SELECT);

  clay_tool_control_set_toggled (tool->control, FALSE);

  /*  set precision to SUBPIXEL, because it may have been changed to
   *  PIXEL_CENTER if the tool has switched to MATTING_STATE_PAINT_TRIMAP,
   *  in clay_foreground_select_tool_set_trimap().
   */
  clay_tool_control_set_precision (tool->control,
                                   CLAY_CURSOR_PRECISION_SUBPIXEL);

  fg_select->state = MATTING_STATE_FREE_SELECT;

  /*  update the undo actions / menu items  */
  if (tool->display)
    clay_image_flush (clay_display_get_image (tool->display));

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (fg_select->gui)
    clay_tool_gui_hide (fg_select->gui);
}

static void
clay_foreground_select_tool_commit (ClayForegroundSelectTool *fg_select)
{
  ClayTool             *tool    = CLAY_TOOL (fg_select);
  ClaySelectionOptions *options = CLAY_SELECTION_TOOL_GET_OPTIONS (fg_select);

  if (tool->display                                 &&
      fg_select->state != MATTING_STATE_FREE_SELECT &&
      clay_foreground_select_tool_has_strokes (fg_select))
    {
      ClayImage *image = clay_display_get_image (tool->display);

      if (fg_select->state != MATTING_STATE_PREVIEW_MASK)
        clay_foreground_select_tool_preview (fg_select);

      clay_channel_select_buffer (clay_image_get_mask (image),
                                  C_("command", "Foreground Select"),
                                  fg_select->mask,
                                  0, /* x offset */
                                  0, /* y offset */
                                  options->operation,
                                  options->feather,
                                  options->feather_radius,
                                  options->feather_radius);

      clay_image_flush (image);
    }
}

static void
clay_foreground_select_tool_set_trimap (ClayForegroundSelectTool *fg_select)
{
  ClayTool                    *tool = CLAY_TOOL (fg_select);
  ClayForegroundSelectOptions *options;

  g_return_if_fail (fg_select->trimap != NULL);

  options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  clay_polygon_select_tool_halt (CLAY_POLYGON_SELECT_TOOL (fg_select));

  if (options->preview_mode == CLAY_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        clay_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   fg_select->trimap, 0, 0,
                                   options->mask_color, TRUE);
    }
  else
    {
      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                        fg_select->trimap, NULL);

          clay_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_PAINTBRUSH);
  clay_tool_control_set_toggle_tool_cursor (tool->control,
                                            CLAY_TOOL_CURSOR_PAINTBRUSH);

  clay_tool_control_set_toggled (tool->control, FALSE);

  /* disable double click in paint trimap state */
  clay_tool_control_set_wants_double_click (tool->control, FALSE);

  /* set precision to PIXEL_CENTER in paint trimap state */
  clay_tool_control_set_precision (tool->control,
                                   CLAY_CURSOR_PRECISION_PIXEL_CENTER);

  fg_select->state = MATTING_STATE_PAINT_TRIMAP;

  clay_foreground_select_tool_update_gui (fg_select);
}

static void
clay_foreground_select_tool_set_preview (ClayForegroundSelectTool *fg_select)
{

  ClayTool                    *tool = CLAY_TOOL (fg_select);
  ClayForegroundSelectOptions *options;

  g_return_if_fail (fg_select->mask != NULL);

  options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->preview_mode == CLAY_MATTING_PREVIEW_MODE_ON_COLOR)
    {
      if (fg_select->grayscale_preview)
        clay_canvas_item_set_visible (fg_select->grayscale_preview, FALSE);

      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   fg_select->mask, 0, 0,
                                   options->mask_color, TRUE);
    }
  else
    {
      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);

      if (fg_select->grayscale_preview)
        {
          g_object_set (fg_select->grayscale_preview, "buffer",
                    fg_select->mask, NULL);
          clay_canvas_item_set_visible (fg_select->grayscale_preview, TRUE);
        }
    }

  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_PAINTBRUSH);
  clay_tool_control_set_toggle_tool_cursor (tool->control,
                                            CLAY_TOOL_CURSOR_PAINTBRUSH);

  clay_tool_control_set_toggled (tool->control, FALSE);

  fg_select->state = MATTING_STATE_PREVIEW_MASK;

  clay_foreground_select_tool_update_gui (fg_select);
}

static void
clay_foreground_select_tool_preview (ClayForegroundSelectTool *fg_select)
{
  ClayTool                    *tool      = CLAY_TOOL (fg_select);
  ClayForegroundSelectOptions *options;
  ClayImage                   *image     = clay_display_get_image (tool->display);
  GList                       *drawables = clay_image_get_selected_drawables (image);
  ClayDrawable                *drawable;

  g_return_if_fail (g_list_length (drawables) == 1);

  drawable = drawables->data;
  g_list_free (drawables);

  options  = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (tool);

  g_clear_object (&fg_select->mask);

  fg_select->mask = clay_drawable_foreground_extract (drawable,
                                                      options->engine,
                                                      options->iterations,
                                                      options->levels,
                                                      options->active_levels,
                                                      fg_select->trimap,
                                                      CLAY_PROGRESS (fg_select));

  clay_foreground_select_tool_set_preview (fg_select);
}

static void
clay_foreground_select_tool_stroke_paint (ClayForegroundSelectTool *fg_select)
{
  ClayForegroundSelectOptions *options;
  ClayScanConvert             *scan_convert;
  StrokeUndo                  *undo;
  gint                         width;
  gdouble                      opacity;

  options = CLAY_FOREGROUND_SELECT_TOOL_GET_OPTIONS (fg_select);

  g_return_if_fail (fg_select->stroke != NULL);

  width = ROUND ((gdouble) options->stroke_width);

  if (fg_select->redo_stack)
    {
      g_list_free_full (fg_select->redo_stack,
                        (GDestroyNotify) clay_foreground_select_undo_free);
      fg_select->redo_stack = NULL;
    }

  undo = clay_foreground_select_undo_new (fg_select->trimap,
                                          fg_select->stroke,
                                          options->draw_mode, width);
  if (! undo)
    {
      g_array_free (fg_select->stroke, TRUE);
      fg_select->stroke = NULL;
      return;
    }

  fg_select->undo_stack = g_list_prepend (fg_select->undo_stack, undo);

  scan_convert = clay_scan_convert_new ();

  if (fg_select->stroke->len == 1)
    {
      ClayVector2 points[2];

      points[0] = points[1] = ((ClayVector2 *) fg_select->stroke->data)[0];

      points[1].x += 0.01;
      points[1].y += 0.01;

      clay_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
    }
  else
    {
      clay_scan_convert_add_polyline (scan_convert,
                                      fg_select->stroke->len,
                                      (ClayVector2 *) fg_select->stroke->data,
                                      FALSE);
    }

  clay_scan_convert_stroke (scan_convert,
                            width,
                            CLAY_JOIN_ROUND, CLAY_CAP_ROUND, 10.0,
                            0.0, NULL);

  if (options->draw_mode == CLAY_MATTING_DRAW_MODE_FOREGROUND)
    opacity = 1.0;
  else if (options->draw_mode == CLAY_MATTING_DRAW_MODE_BACKGROUND)
    opacity = 0.0;
  else
    opacity = 0.5;

  clay_scan_convert_compose_value (scan_convert, fg_select->trimap,
                                   0, 0,
                                   opacity);

  clay_scan_convert_free (scan_convert);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;

  /*  update the undo actions / menu items  */
  clay_image_flush (clay_display_get_image (CLAY_TOOL (fg_select)->display));
}

static void
clay_foreground_select_tool_cancel_paint (ClayForegroundSelectTool *fg_select)
{
  g_return_if_fail (fg_select->stroke != NULL);

  g_array_free (fg_select->stroke, TRUE);
  fg_select->stroke = NULL;
}

static void
clay_foreground_select_tool_response (ClayToolGui              *gui,
                                      gint                      response_id,
                                      ClayForegroundSelectTool *fg_select)
{
  ClayTool *tool = CLAY_TOOL (fg_select);

  switch (response_id)
    {
    case GTK_RESPONSE_APPLY:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_foreground_select_tool_preview_toggled (GtkToggleButton          *button,
                                             ClayForegroundSelectTool *fg_select)
{
  if (fg_select->state != MATTING_STATE_FREE_SELECT)
    {
      if (gtk_toggle_button_get_active (button))
        {
          if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
            clay_foreground_select_tool_preview (fg_select);
        }
      else
        {
          if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
            clay_foreground_select_tool_set_trimap (fg_select);
        }
    }
}

static void
clay_foreground_select_tool_update_gui (ClayForegroundSelectTool *fg_select)
{
  if (fg_select->state == MATTING_STATE_PAINT_TRIMAP)
    {
      clay_tool_gui_set_description (fg_select->gui, _("Paint mask"));
    }
  else if (fg_select->state == MATTING_STATE_PREVIEW_MASK)
    {
      clay_tool_gui_set_description (fg_select->gui, _("Preview"));
    }

  clay_tool_gui_set_response_sensitive (fg_select->gui, GTK_RESPONSE_APPLY,
                                        TRUE);
  gtk_widget_set_sensitive (fg_select->preview_toggle, TRUE);
}

static gboolean
clay_foreground_select_tool_has_strokes (ClayForegroundSelectTool *fg_select)
{
  GList *iter;

  if (fg_select->undo_stack == NULL)
    return FALSE;

  /* Check if undo_stack has any foreground strokes, as these are the
     only ones that can create a selection */
  for (iter = fg_select->undo_stack; iter; iter = iter->next)
    {
      StrokeUndo *undo = iter->data;

      if (undo && undo->draw_mode == CLAY_MATTING_DRAW_MODE_FOREGROUND)
        return TRUE;
    }

  return FALSE;
}

static StrokeUndo *
clay_foreground_select_undo_new (GeglBuffer          *trimap,
                                 GArray              *stroke,
                                 ClayMattingDrawMode draw_mode,
                                 gint                stroke_width)

{
  StrokeUndo          *undo;
  const GeglRectangle *extent;
  gint                 x1, y1, x2, y2;
  gint                 width, height;
  gint                 i;

  extent = gegl_buffer_get_extent (trimap);

  x1 = G_MAXINT;
  y1 = G_MAXINT;
  x2 = G_MININT;
  y2 = G_MININT;

  for (i = 0; i < stroke->len; i++)
    {
      ClayVector2 *point = &g_array_index (stroke, ClayVector2, i);

      x1 = MIN (x1, floor (point->x));
      y1 = MIN (y1, floor (point->y));
      x2 = MAX (x2, ceil (point->x));
      y2 = MAX (y2, ceil (point->y));
    }

  x1 -= (stroke_width + 1) / 2;
  y1 -= (stroke_width + 1) / 2;
  x2 += (stroke_width + 1) / 2;
  y2 += (stroke_width + 1) / 2;

  x1 = MAX (x1, extent->x);
  y1 = MAX (y1, extent->y);
  x2 = MIN (x2, extent->x + extent->width);
  y2 = MIN (y2, extent->x + extent->height);

  width  = x2 - x1;
  height = y2 - y1;

  if (width <= 0 || height <= 0)
    return NULL;

  undo = g_slice_new0 (StrokeUndo);
  undo->saved_trimap = gegl_buffer_new (GEGL_RECTANGLE (x1, y1, width, height),
                                        gegl_buffer_get_format (trimap));

  clay_gegl_buffer_copy (
    trimap,             GEGL_RECTANGLE (x1, y1, width, height),
    GEGL_ABYSS_NONE,
    undo->saved_trimap, NULL);

  undo->trimap_x = x1;
  undo->trimap_y = y1;

  undo->draw_mode    = draw_mode;
  undo->stroke_width = stroke_width;

  return undo;
}

static void
clay_foreground_select_undo_pop (StrokeUndo *undo,
                                 GeglBuffer *trimap)
{
  GeglBuffer *buffer;
  gint        width, height;

  buffer = clay_gegl_buffer_dup (undo->saved_trimap, NULL);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  clay_gegl_buffer_copy (trimap,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         undo->saved_trimap, NULL);

  clay_gegl_buffer_copy (buffer,
                         GEGL_RECTANGLE (undo->trimap_x, undo->trimap_y,
                                         width, height),
                         GEGL_ABYSS_NONE,
                         trimap, NULL);

  g_object_unref (buffer);
}

static void
clay_foreground_select_undo_free (StrokeUndo *undo)
{
  g_clear_object (&undo->saved_trimap);

  g_slice_free (StrokeUndo, undo);
}
