/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <gdk/gdkkeysyms.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"

#include "core/clay.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-arrange.h"
#include "core/clayimage-pick-item.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claypickable.h"
#include "core/claypickable-auto-shrink.h"

#include "path/claypath.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"

#include "clayalignoptions.h"
#include "clayaligntool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


#define EPSILON  3   /* move distance after which we do a box-select */


/*  local function prototypes  */

static void     clay_align_tool_constructed    (GObject               *object);

static void     clay_align_tool_control        (ClayTool              *tool,
                                                ClayToolAction         action,
                                                ClayDisplay           *display);
static void     clay_align_tool_button_press   (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonPressType    press_type,
                                                ClayDisplay           *display);
static void     clay_align_tool_button_release (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayButtonReleaseType  release_type,
                                                ClayDisplay           *display);
static void     clay_align_tool_motion         (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                guint32                time,
                                                GdkModifierType        state,
                                                ClayDisplay           *display);
static gboolean clay_align_tool_key_press      (ClayTool              *tool,
                                                GdkEventKey           *kevent,
                                                ClayDisplay           *display);
static void     clay_align_tool_oper_update    (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                gboolean               proximity,
                                                ClayDisplay           *display);
static void     clay_align_tool_status_update  (ClayTool              *tool,
                                                ClayDisplay           *display,
                                                GdkModifierType        state,
                                                gboolean               proximity);
static void     clay_align_tool_cursor_update  (ClayTool              *tool,
                                                const ClayCoords      *coords,
                                                GdkModifierType        state,
                                                ClayDisplay           *display);
const gchar *   clay_align_tool_can_undo       (ClayTool              *tool,
                                                ClayDisplay           *display);
static gboolean clay_align_tool_undo           (ClayTool              *tool,
                                                ClayDisplay           *display);

static void     clay_align_tool_draw           (ClayDrawTool          *draw_tool);
static void     clay_align_tool_redraw         (ClayAlignTool         *align_tool);

static void     clay_align_tool_align          (ClayAlignTool         *align_tool,
                                                ClayAlignmentType      align_type);

static gint     clay_align_tool_undo_idle      (gpointer               data);
static void    clay_align_tool_display_changed (ClayContext           *context,
                                                ClayDisplay           *display,
                                                ClayAlignTool         *align_tool);


G_DEFINE_TYPE (ClayAlignTool, clay_align_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_align_tool_parent_class


void
clay_align_tool_register (ClayToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (CLAY_TYPE_ALIGN_TOOL,
                CLAY_TYPE_ALIGN_OPTIONS,
                clay_align_options_gui,
                0,
                "clay-align-tool",
                _("Align and Distribute"),
                _("Alignment Tool: Align or arrange layers and other objects"),
                N_("_Align and Distribute"), "Q",
                NULL, CLAY_HELP_TOOL_ALIGN,
                CLAY_ICON_TOOL_ALIGN,
                data);
}

static void
clay_align_tool_class_init (ClayAlignToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = clay_align_tool_constructed;

  tool_class->control        = clay_align_tool_control;
  tool_class->button_press   = clay_align_tool_button_press;
  tool_class->button_release = clay_align_tool_button_release;
  tool_class->motion         = clay_align_tool_motion;
  tool_class->key_press      = clay_align_tool_key_press;
  tool_class->oper_update    = clay_align_tool_oper_update;
  tool_class->cursor_update  = clay_align_tool_cursor_update;
  tool_class->can_undo       = clay_align_tool_can_undo;
  tool_class->undo           = clay_align_tool_undo;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = clay_align_tool_draw;
}

static void
clay_align_tool_init (ClayAlignTool *align_tool)
{
  ClayTool *tool = CLAY_TOOL (align_tool);

  clay_tool_control_set_snap_to     (tool->control, FALSE);
  clay_tool_control_set_precision   (tool->control,
                                     CLAY_CURSOR_PRECISION_PIXEL_BORDER);
  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_MOVE);

  align_tool->function = ALIGN_TOOL_REF_IDLE;
}

static void
clay_align_tool_constructed (GObject *object)
{
  ClayAlignTool    *align_tool = CLAY_ALIGN_TOOL (object);
  ClayAlignOptions *options;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  options = CLAY_ALIGN_TOOL_GET_OPTIONS (align_tool);

  g_signal_connect_object (options, "align-button-clicked",
                           G_CALLBACK (clay_align_tool_align),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (options, "notify::align-reference",
                           G_CALLBACK (clay_align_tool_redraw),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (options, "notify::align-contents",
                           G_CALLBACK (clay_align_tool_redraw),
                           align_tool, G_CONNECT_SWAPPED);
  g_signal_connect_object (clay_get_user_context (CLAY_CONTEXT (options)->clay),
                           "display-changed",
                           G_CALLBACK (clay_align_tool_display_changed),
                           align_tool, 0);
}

static void
clay_align_tool_control (ClayTool       *tool,
                         ClayToolAction  action,
                         ClayDisplay    *display)
{
  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_align_tool_button_press (ClayTool            *tool,
                              const ClayCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              ClayButtonPressType  press_type,
                              ClayDisplay         *display)
{
  ClayAlignTool *align_tool  = CLAY_ALIGN_TOOL (tool);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  /*  If the tool was being used in another image... reset it  */
  if (display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

  tool->display = display;

  clay_tool_control_activate (tool->control);

  align_tool->x2 = align_tool->x1 = coords->x;
  align_tool->y2 = align_tool->y1 = coords->y;

  if (! clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

/* some rather complex logic here.  If the user clicks without modifiers,
 * then we start a new list, and use the first object in it as reference.
 * If the user clicks using Shift, or draws a rubber-band box, then
 * we add objects to the list, but do not specify which one should
 * be used as reference.
 */
static void
clay_align_tool_button_release (ClayTool              *tool,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type,
                                ClayDisplay           *display)
{
  ClayAlignTool    *align_tool = CLAY_ALIGN_TOOL (tool);
  ClayAlignOptions *options    = CLAY_ALIGN_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell *shell      = clay_display_get_shell (display);
  ClayImage        *image      = clay_display_get_image (display);
  GdkModifierType   extend_mask;

  extend_mask = clay_get_extend_selection_mask ();

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  clay_tool_control_halt (tool->control);

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      align_tool->x2 = align_tool->x1;
      align_tool->y2 = align_tool->y1;

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
      return;
    }

  if (state & GDK_MOD1_MASK)
    {
      ClayGuide *guide = NULL;

      if (clay_display_shell_get_show_guides (shell))
        {
          gint snap_distance = display->config->snap_distance;

          guide = clay_image_pick_guide (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      clay_align_options_pick_guide (options, guide, (gboolean) state & extend_mask);
    }
  else
    {
      GObject *object = NULL;

      /* Check if a layer is fully included in the rubber-band rectangle.
       * Don't verify for too small rectangles.
       */
      /* FIXME: look for paths too */
      if (hypot (coords->x - align_tool->x1,
                 coords->y - align_tool->y1) > EPSILON)
        {
          gint   X0 = MIN (coords->x, align_tool->x1);
          gint   X1 = MAX (coords->x, align_tool->x1);
          gint   Y0 = MIN (coords->y, align_tool->y1);
          gint   Y1 = MAX (coords->y, align_tool->y1);
          GList *all_layers;
          GList *list;

          all_layers = clay_image_get_layer_list (image);

          for (list = all_layers; list; list = g_list_next (list))
            {
              ClayLayer *layer = list->data;
              gint       x0, y0, x1, y1;

              if (! clay_item_get_visible (CLAY_ITEM (layer)))
                continue;

              clay_item_get_offset (CLAY_ITEM (layer), &x0, &y0);
              x1 = x0 + clay_item_get_width  (CLAY_ITEM (layer));
              y1 = y0 + clay_item_get_height (CLAY_ITEM (layer));

              if (x0 < X0 || y0 < Y0 || x1 > X1 || y1 > Y1)
                continue;

              object = G_OBJECT (layer);
              break;
            }

          g_list_free (all_layers);
        }

      if (object == NULL)
        {
          ClayPath    *path;
          ClayGuide   *guide;
          ClayLayer   *layer;
          GObject     *previously_picked;
          gint         snap_distance = display->config->snap_distance;

          previously_picked = clay_align_options_get_reference (options, FALSE);


          if ((path = clay_image_pick_path (image,
                                               coords->x, coords->y,
                                               FUNSCALEX (shell, snap_distance),
                                               FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (path);
            }
          else if (clay_display_shell_get_show_guides (shell) &&
                   (guide = clay_image_pick_guide (image,
                                                   coords->x, coords->y,
                                                   FUNSCALEX (shell, snap_distance),
                                                   FUNSCALEY (shell, snap_distance))))
            {
              object = G_OBJECT (guide);
            }
          else if ((layer = clay_image_pick_layer (image, coords->x, coords->y,
                                                   previously_picked && CLAY_IS_LAYER (previously_picked)? CLAY_LAYER (previously_picked) : NULL)))
            {
              object = G_OBJECT (layer);
            }
        }

      if (object)
        clay_align_options_pick_reference (options, object);
    }

  align_tool->x2 = align_tool->x1;
  align_tool->y2 = align_tool->y1;

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_align_tool_motion (ClayTool         *tool,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        ClayDisplay      *display)
{
  ClayAlignTool *align_tool = CLAY_ALIGN_TOOL (tool);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  align_tool->x2 = coords->x;
  align_tool->y2 = coords->y;

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static gboolean
clay_align_tool_key_press (ClayTool    *tool,
                           GdkEventKey *kevent,
                           ClayDisplay *display)
{
  if (display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Escape:
          clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
          return TRUE;

        default:
          break;
        }
    }

  return FALSE;
}

static void
clay_align_tool_oper_update (ClayTool         *tool,
                             const ClayCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             ClayDisplay      *display)
{
  ClayAlignTool    *align_tool    = CLAY_ALIGN_TOOL (tool);
  ClayAlignOptions *options       = CLAY_ALIGN_TOOL_GET_OPTIONS (align_tool);
  ClayDisplayShell *shell         = clay_display_get_shell (display);
  ClayImage        *image         = clay_display_get_image (display);
  gint              snap_distance = display->config->snap_distance;

  state &= clay_get_all_modifiers_mask ();

  align_tool->function = ALIGN_TOOL_NO_ACTION;

  if (clay_image_pick_path (image,
                            coords->x, coords->y,
                            FUNSCALEX (shell, snap_distance),
                            FUNSCALEY (shell, snap_distance)))
    {
      if (options->align_reference == CLAY_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_PATH;
    }
  else if (clay_display_shell_get_show_guides (shell) &&
           clay_image_pick_guide (image,
                                  coords->x, coords->y,
                                  FUNSCALEX (shell, snap_distance),
                                  FUNSCALEY (shell, snap_distance)))
    {
      if (state == (clay_get_extend_selection_mask () | GDK_MOD1_MASK))
        align_tool->function = ALIGN_TOOL_ALIGN_ADD_GUIDE;
      else if (state == GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_PICK_GUIDE;
      else if (options->align_reference == CLAY_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_GUIDE;
    }
  else if (clay_image_pick_layer_by_bounds (image, coords->x, coords->y))
    {
      if (options->align_reference == CLAY_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_PICK_LAYER;
    }
  else
    {
      if (state & GDK_MOD1_MASK)
        align_tool->function = ALIGN_TOOL_ALIGN_IDLE;
      else if (options->align_reference == CLAY_ALIGN_REFERENCE_PICK)
        align_tool->function = ALIGN_TOOL_REF_IDLE;
    }

  clay_align_tool_status_update (tool, display, state, proximity);

  if (! clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

static void
clay_align_tool_cursor_update (ClayTool         *tool,
                               const ClayCoords *coords,
                               GdkModifierType   state,
                               ClayDisplay      *display)
{
  ClayAlignTool      *align_tool  = CLAY_ALIGN_TOOL (tool);
  ClayToolCursorType  tool_cursor = CLAY_TOOL_CURSOR_NONE;
  ClayCursorModifier  modifier    = CLAY_CURSOR_MODIFIER_NONE;

  switch (align_tool->function)
    {
    case ALIGN_TOOL_REF_IDLE:
    case ALIGN_TOOL_ALIGN_IDLE:
      tool_cursor = CLAY_TOOL_CURSOR_RECT_SELECT;
      break;

    case ALIGN_TOOL_REF_PICK_LAYER:
      tool_cursor = CLAY_TOOL_CURSOR_HAND;
      break;

    case ALIGN_TOOL_ALIGN_ADD_GUIDE:
      modifier = CLAY_CURSOR_MODIFIER_PLUS;
    case ALIGN_TOOL_REF_PICK_GUIDE:
    case ALIGN_TOOL_ALIGN_PICK_GUIDE:
      tool_cursor = CLAY_TOOL_CURSOR_MOVE;
      break;

    case ALIGN_TOOL_REF_PICK_PATH:
      tool_cursor = CLAY_TOOL_CURSOR_PATHS;
      break;

    case ALIGN_TOOL_REF_DRAG_BOX:
    case ALIGN_TOOL_NO_ACTION:
      break;
    }

  clay_tool_control_set_cursor          (tool->control, CLAY_CURSOR_MOUSE);
  clay_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
clay_align_tool_can_undo (ClayTool    *tool,
                          ClayDisplay *display)
{
  return _("Arrange Objects");
}

static gboolean
clay_align_tool_undo (ClayTool    *tool,
                      ClayDisplay *display)
{
  g_idle_add ((GSourceFunc) clay_align_tool_undo_idle, (gpointer) tool);

  return FALSE;
}

static void
clay_align_tool_status_update (ClayTool        *tool,
                               ClayDisplay     *display,
                               GdkModifierType  state,
                               gboolean         proximity)
{
  ClayAlignTool    *align_tool = CLAY_ALIGN_TOOL (tool);
  gchar            *status     = NULL;
  GdkModifierType   extend_mask;

  extend_mask = clay_get_extend_selection_mask ();

  clay_tool_pop_status (tool, display);

  if (proximity)
    {
      switch (align_tool->function)
        {
        case ALIGN_TOOL_REF_IDLE:
          status = g_strdup (_("Click on a layer, path or guide, "
                               "or Click-Drag to pick a reference"));
          break;
        case ALIGN_TOOL_REF_PICK_LAYER:
          status = g_strdup (_("Click to pick this layer as reference"));
          break;
        case ALIGN_TOOL_REF_PICK_GUIDE:
          status = clay_suggest_modifiers (_("Click to pick this guide as reference"),
                                           GDK_MOD1_MASK & ~state,
                                           NULL, NULL, NULL);
          break;
        case ALIGN_TOOL_REF_PICK_PATH:
          status = g_strdup (_("Click to pick this path as reference"));
          break;

        case ALIGN_TOOL_REF_DRAG_BOX:
          break;

        case ALIGN_TOOL_ALIGN_IDLE:
          status = g_strdup (_("Click on a guide to add it to objects to align, "
                               "click anywhere else to unselect all guides"));
          break;
        case ALIGN_TOOL_ALIGN_PICK_GUIDE:
          status = clay_suggest_modifiers (_("Click to select this guide for alignment"),
                                           extend_mask & ~state,
                                           NULL, NULL, NULL);
          break;
        case ALIGN_TOOL_ALIGN_ADD_GUIDE:
          status = g_strdup (_("Click to add this guide to the list of objects to align"));
          break;

        case ALIGN_TOOL_NO_ACTION:
          break;
        }
    }

  if (status)
    {
      clay_tool_push_status (tool, display, "%s", status);
      g_free (status);
    }
}

static void
clay_align_tool_draw (ClayDrawTool *draw_tool)
{
  ClayAlignTool    *align_tool = CLAY_ALIGN_TOOL (draw_tool);
  ClayAlignOptions *options    = CLAY_ALIGN_TOOL_GET_OPTIONS (align_tool);
  GObject          *reference;
  GList            *objects;
  GList            *iter;
  gint              x, y, w, h;

  /* draw rubber-band rectangle */
  x = MIN (align_tool->x2, align_tool->x1);
  y = MIN (align_tool->y2, align_tool->y1);
  w = MAX (align_tool->x2, align_tool->x1) - x;
  h = MAX (align_tool->y2, align_tool->y1) - y;

  if (w != 0 && h != 0)
    clay_draw_tool_add_rectangle (draw_tool, FALSE, x, y, w, h);

  /* Draw handles on the reference object. */
  reference = clay_align_options_get_reference (options, FALSE);
  if (CLAY_IS_ITEM (reference))
    {
      ClayItem *item = CLAY_ITEM (reference);
      gint      off_x, off_y;

      clay_item_bounds (item, &x, &y, &w, &h);

      clay_item_get_offset (item, &off_x, &off_y);
      x += off_x;
      y += off_y;

      if (clay_align_options_align_contents (options) &&
          CLAY_IS_PICKABLE (reference))
        {
          gint shrunk_x;
          gint shrunk_y;

          if (clay_pickable_auto_shrink (CLAY_PICKABLE (reference),
                                         0, 0,
                                         clay_item_get_width  (CLAY_ITEM (reference)),
                                         clay_item_get_height (CLAY_ITEM (reference)),
                                         &shrunk_x, &shrunk_y, &w, &h) == CLAY_AUTO_SHRINK_SHRINK)
            {
              x += shrunk_x;
              y += shrunk_y;
            }
        }

      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 x, y,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_NORTH_WEST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 x + w, y,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_NORTH_EAST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 x, y + h,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_SOUTH_WEST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 x + w, y + h,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_SOUTH_EAST);
    }
  else if (CLAY_IS_GUIDE (reference))
    {
      ClayGuide *guide = CLAY_GUIDE (reference);
      ClayImage *image = clay_display_get_image (draw_tool->display);
      gint       x, y;
      gint       w, h;

      switch (clay_guide_get_orientation (guide))
        {
        case CLAY_ORIENTATION_VERTICAL:
          x = clay_guide_get_position (guide);
          h = clay_image_get_height (image);
          clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                     x, h,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_HANDLE_ANCHOR_SOUTH);
          clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                     x, 0,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_HANDLE_ANCHOR_NORTH);
          break;

        case CLAY_ORIENTATION_HORIZONTAL:
          y = clay_guide_get_position (guide);
          w = clay_image_get_width (image);
          clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                     w, y,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_HANDLE_ANCHOR_EAST);
          clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                     0, y,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_TOOL_HANDLE_SIZE_SMALL,
                                     CLAY_HANDLE_ANCHOR_WEST);
          break;

        default:
          break;
        }
    }
  else if (CLAY_IS_IMAGE (reference))
    {
      ClayImage *image  = CLAY_IMAGE (reference);
      gint       width  = clay_image_get_width (image);
      gint       height = clay_image_get_height (image);

      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 0, 0,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_NORTH_WEST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 width, 0,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_NORTH_EAST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 0, height,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_SOUTH_WEST);
      clay_draw_tool_add_handle (draw_tool, CLAY_HANDLE_FILLED_SQUARE,
                                 width, height,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_TOOL_HANDLE_SIZE_SMALL,
                                 CLAY_HANDLE_ANCHOR_SOUTH_EAST);
    }

  /* Highlight picked guides, similarly to how they are highlighted in Move
   * tool.
   */
  objects = clay_align_options_get_objects (options);
  for (iter = objects; iter; iter = iter->next)
    {
      if (CLAY_IS_GUIDE (iter->data))
        {
          ClayGuide      *guide = iter->data;
          ClayCanvasItem *item;
          ClayGuideStyle  style;

          style = clay_guide_get_style (guide);

          item = clay_draw_tool_add_guide (draw_tool,
                                           clay_guide_get_orientation (guide),
                                           clay_guide_get_position (guide),
                                           style);
          clay_canvas_item_set_highlight (item, TRUE);
        }
    }
}

static void
clay_align_tool_redraw (ClayAlignTool *align_tool)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (align_tool);

  clay_draw_tool_pause (draw_tool);

  if (! clay_draw_tool_is_active (draw_tool) && draw_tool->display)
    clay_draw_tool_start (draw_tool, draw_tool->display);

  clay_draw_tool_resume (draw_tool);
}

static void
clay_align_tool_align (ClayAlignTool     *align_tool,
                       ClayAlignmentType  align_type)
{
  ClayAlignOptions *options = CLAY_ALIGN_TOOL_GET_OPTIONS (align_tool);
  ClayImage        *image;
  GObject          *reference_object = NULL;
  GList            *objects;
  GList            *list;
  gdouble           align_x = 0.0;
  gdouble           align_y = 0.0;

  /* if nothing is selected, just return */
  objects = clay_align_options_get_objects (options);
  if (! objects)
    return;

  image = clay_context_get_image (clay_get_user_context (CLAY_CONTEXT (options)->clay));
  list  = objects;

  if (align_type < CLAY_ARRANGE_HFILL)
    {
      reference_object = clay_align_options_get_reference (options, TRUE);
      if (! reference_object)
        return;
    }

  clay_draw_tool_pause (CLAY_DRAW_TOOL (align_tool));

  clay_align_options_get_pivot (options, &align_x, &align_y);
  clay_image_arrange_objects (image, list,
                              align_x, align_y,
                              reference_object,
                              align_type,
                              clay_align_options_align_contents (options));

  clay_draw_tool_resume (CLAY_DRAW_TOOL (align_tool));

  clay_image_flush (image);
  g_list_free (objects);
}

static gint
clay_align_tool_undo_idle (gpointer data)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (data);
  ClayDisplay  *display   = draw_tool->display;

  /* This makes sure that we properly undraw then redraw the various handles and
   * highlighted guides after undoing.
   */
  clay_draw_tool_stop (draw_tool);
  clay_draw_tool_start (draw_tool, display);

  return FALSE; /* remove idle */
}

static void
clay_align_tool_display_changed (ClayContext   *context,
                                 ClayDisplay   *display,
                                 ClayAlignTool *align_tool)
{
  ClayTool     *tool      = CLAY_TOOL (align_tool);
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (align_tool);

  clay_draw_tool_pause (draw_tool);

  if (display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

  tool->display = display;

  if (! clay_draw_tool_is_active (draw_tool) && display)
    clay_draw_tool_start (draw_tool, display);

  clay_draw_tool_resume (draw_tool);
}
