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

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay-transform-utils.h"
#include "core/clayimage.h"

#include "paint/clayperspectiveclone.h"
#include "paint/clayperspectivecloneoptions.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewablebox.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasgroup.h"
#include "display/claydisplay.h"
#include "display/claytooltransformgrid.h"

#include "clayperspectiveclonetool.h"
#include "claycloneoptions-gui.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3,
  PIVOT_X,
  PIVOT_Y
};


static void     clay_perspective_clone_tool_constructed    (GObject          *object);

static gboolean clay_perspective_clone_tool_initialize     (ClayTool         *tool,
                                                            ClayDisplay      *display,
                                                            GError          **error);

static gboolean clay_perspective_clone_tool_has_display    (ClayTool         *tool,
                                                            ClayDisplay      *display);
static ClayDisplay *
                clay_perspective_clone_tool_has_image      (ClayTool         *tool,
                                                            ClayImage        *image);
static void     clay_perspective_clone_tool_control        (ClayTool         *tool,
                                                            ClayToolAction    action,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_button_press   (ClayTool         *tool,
                                                            const ClayCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            ClayButtonPressType  press_type,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_button_release (ClayTool         *tool,
                                                            const ClayCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            ClayButtonReleaseType  release_type,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_motion         (ClayTool         *tool,
                                                            const ClayCoords *coords,
                                                            guint32           time,
                                                            GdkModifierType   state,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_modifier_key   (ClayTool         *tool,
                                                            GdkModifierType   key,
                                                            gboolean          press,
                                                            GdkModifierType   state,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_cursor_update  (ClayTool         *tool,
                                                            const ClayCoords *coords,
                                                            GdkModifierType   state,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_oper_update    (ClayTool         *tool,
                                                            const ClayCoords *coords,
                                                            GdkModifierType   state,
                                                            gboolean          proximity,
                                                            ClayDisplay      *display);
static void     clay_perspective_clone_tool_options_notify (ClayTool         *tool,
                                                            ClayToolOptions  *options,
                                                            const GParamSpec *pspec);

static void     clay_perspective_clone_tool_draw           (ClayDrawTool             *draw_tool);

static void     clay_perspective_clone_tool_halt           (ClayPerspectiveCloneTool *clone_tool);
static void     clay_perspective_clone_tool_bounds         (ClayPerspectiveCloneTool *clone_tool,
                                                            ClayDisplay              *display);
static void     clay_perspective_clone_tool_prepare        (ClayPerspectiveCloneTool *clone_tool);
static void     clay_perspective_clone_tool_recalc_matrix  (ClayPerspectiveCloneTool *clone_tool,
                                                            ClayToolWidget           *widget);

static void     clay_perspective_clone_tool_widget_changed (ClayToolWidget           *widget,
                                                            ClayPerspectiveCloneTool *clone_tool);
static void     clay_perspective_clone_tool_widget_status  (ClayToolWidget           *widget,
                                                            const gchar              *status,
                                                            ClayPerspectiveCloneTool *clone_tool);

static GtkWidget *
                clay_perspective_clone_options_gui         (ClayToolOptions *tool_options);


G_DEFINE_TYPE (ClayPerspectiveCloneTool, clay_perspective_clone_tool,
               CLAY_TYPE_BRUSH_TOOL)

#define parent_class clay_perspective_clone_tool_parent_class


void
clay_perspective_clone_tool_register (ClayToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (CLAY_TYPE_PERSPECTIVE_CLONE_TOOL,
                CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS,
                clay_perspective_clone_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_PATTERN,
                "clay-perspective-clone-tool",
                _("Perspective Clone"),
                _("Perspective Clone Tool: Clone from an image source "
                  "after applying a perspective transformation"),
                N_("_Perspective Clone"), NULL,
                NULL, CLAY_HELP_TOOL_PERSPECTIVE_CLONE,
                CLAY_ICON_TOOL_PERSPECTIVE_CLONE,
                data);
}

static void
clay_perspective_clone_tool_class_init (ClayPerspectiveCloneToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = clay_perspective_clone_tool_constructed;

  tool_class->initialize     = clay_perspective_clone_tool_initialize;
  tool_class->has_display    = clay_perspective_clone_tool_has_display;
  tool_class->has_image      = clay_perspective_clone_tool_has_image;
  tool_class->control        = clay_perspective_clone_tool_control;
  tool_class->button_press   = clay_perspective_clone_tool_button_press;
  tool_class->button_release = clay_perspective_clone_tool_button_release;
  tool_class->motion         = clay_perspective_clone_tool_motion;
  tool_class->modifier_key   = clay_perspective_clone_tool_modifier_key;
  tool_class->cursor_update  = clay_perspective_clone_tool_cursor_update;
  tool_class->oper_update    = clay_perspective_clone_tool_oper_update;
  tool_class->options_notify = clay_perspective_clone_tool_options_notify;

  draw_tool_class->draw      = clay_perspective_clone_tool_draw;
}

static void
clay_perspective_clone_tool_init (ClayPerspectiveCloneTool *clone_tool)
{
  ClayTool *tool = CLAY_TOOL (clone_tool);

  clay_tool_control_set_action_object_2 (tool->control,
                                         "context-pattern-select-set");

  clay_matrix3_identity (&clone_tool->transform);
  clay_paint_tool_enable_multi_paint (CLAY_PAINT_TOOL (clone_tool));
}

static void
clay_perspective_clone_tool_constructed (GObject *object)
{
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    clay_paint_tool_set_active (CLAY_PAINT_TOOL (object), FALSE);
}

static gboolean
clay_perspective_clone_tool_initialize (ClayTool     *tool,
                                        ClayDisplay  *display,
                                        GError      **error)
{
  ClayPerspectiveCloneTool *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayImage                *image      = clay_display_get_image (display);
  GList                    *drawables;

  if (! CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = clay_image_get_selected_drawables (image);
  if (g_list_length (drawables) == 0)
    {
      clay_tool_message_literal (tool, display, _("No selected drawables."));

      g_list_free (drawables);

      return FALSE;
    }

  if (display != tool->display)
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);
      gint              i;

      tool->display   = display;
      tool->drawables = drawables;

      /*  Find the transform bounds initializing */
      clay_perspective_clone_tool_bounds (clone_tool, display);

      clay_perspective_clone_tool_prepare (clone_tool);

      /*  Recalculate the transform tool  */
      clay_perspective_clone_tool_recalc_matrix (clone_tool, NULL);

      clone_tool->widget =
        clay_tool_transform_grid_new (shell,
                                      &clone_tool->transform,
                                      clone_tool->x1,
                                      clone_tool->y1,
                                      clone_tool->x2,
                                      clone_tool->y2);

      g_object_set (clone_tool->widget,
                    "pivot-x",                 (clone_tool->x1 + clone_tool->x2) / 2.0,
                    "pivot-y",                 (clone_tool->y1 + clone_tool->y2) / 2.0,
                    "inside-function",         CLAY_TRANSFORM_FUNCTION_MOVE,
                    "outside-function",        CLAY_TRANSFORM_FUNCTION_ROTATE,
                    "use-corner-handles",      TRUE,
                    "use-perspective-handles", TRUE,
                    "use-side-handles",        TRUE,
                    "use-shear-handles",       TRUE,
                    "use-pivot-handle",        TRUE,
                    NULL);

      g_signal_connect (clone_tool->widget, "changed",
                        G_CALLBACK (clay_perspective_clone_tool_widget_changed),
                        clone_tool);
      g_signal_connect (clone_tool->widget, "status",
                        G_CALLBACK (clay_perspective_clone_tool_widget_status),
                        clone_tool);

      /*  start drawing the bounding box and handles...  */
      if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
        clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

      clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);

      /*  Save the current transformation info  */
      for (i = 0; i < TRANS_INFO_SIZE; i++)
        clone_tool->old_trans_info[i] = clone_tool->trans_info[i];
    }
  else
    {
      g_list_free (drawables);
    }

  return TRUE;
}

static gboolean
clay_perspective_clone_tool_has_display (ClayTool    *tool,
                                         ClayDisplay *display)
{
  ClayPerspectiveCloneTool *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);

  return (display == clone_tool->src_display ||
          CLAY_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static ClayDisplay *
clay_perspective_clone_tool_has_image (ClayTool  *tool,
                                       ClayImage *image)
{
  ClayPerspectiveCloneTool *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayDisplay              *display;

  display = CLAY_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && clone_tool->src_display)
    {
      if (image && clay_display_get_image (clone_tool->src_display) == image)
        display = clone_tool->src_display;

      /*  NULL image means any display  */
      if (! image)
        display = clone_tool->src_display;
    }

  return display;
}

static void
clay_perspective_clone_tool_control (ClayTool       *tool,
                                     ClayToolAction  action,
                                     ClayDisplay    *display)
{
  ClayPerspectiveCloneTool *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_perspective_clone_tool_halt (clone_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_perspective_clone_tool_button_press (ClayTool            *tool,
                                          const ClayCoords    *coords,
                                          guint32              time,
                                          GdkModifierType      state,
                                          ClayButtonPressType  press_type,
                                          ClayDisplay         *display)
{
  ClayPaintTool               *paint_tool  = CLAY_PAINT_TOOL (tool);
  ClayPerspectiveCloneTool    *clone_tool  = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPerspectiveClone        *clone       = CLAY_PERSPECTIVE_CLONE (paint_tool->core);
  ClaySourceCore              *source_core = CLAY_SOURCE_CORE (clone);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          clay_tool_widget_hover (clone_tool->widget, coords, state, TRUE);

          if (clay_tool_widget_button_press (clone_tool->widget, coords,
                                             time, state, press_type))
            {
              clone_tool->grab_widget = clone_tool->widget;
            }
        }

      clay_tool_control_activate (tool->control);
    }
  else
    {
      GdkModifierType extend_mask = clay_get_extend_selection_mask ();
      GdkModifierType toggle_mask = clay_get_toggle_behavior_mask ();
      gdouble         nnx, nny;

      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          source_core->set_source = TRUE;

          clone_tool->src_display = display;
        }
      else
        {
          source_core->set_source = FALSE;
        }

      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);

      /* Set the coordinates for the reference cross */
      clay_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static void
clay_perspective_clone_tool_button_release (ClayTool              *tool,
                                            const ClayCoords      *coords,
                                            guint32                time,
                                            GdkModifierType        state,
                                            ClayButtonReleaseType  release_type,
                                            ClayDisplay           *display)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  switch (options->clone_mode)
    {
    case CLAY_PERSPECTIVE_CLONE_MODE_ADJUST:
      clay_tool_control_halt (tool->control);

      if (clone_tool->grab_widget)
        {
          clay_tool_widget_button_release (clone_tool->grab_widget,
                                           coords, time, state, release_type);
          clone_tool->grab_widget = NULL;
        }
      break;

    case CLAY_PERSPECTIVE_CLONE_MODE_PAINT:
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
      break;
    }
}

static void
clay_perspective_clone_tool_motion (ClayTool         *tool,
                                    const ClayCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    ClayDisplay      *display)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPaintTool               *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayPerspectiveClone        *clone      = CLAY_PERSPECTIVE_CLONE (paint_tool->core);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->grab_widget)
        {
          clay_tool_widget_motion (clone_tool->grab_widget, coords, time, state);
        }
    }
  else if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_PAINT)
    {
      gdouble nnx, nny;

      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);

      /* Set the coordinates for the reference cross */
      clay_perspective_clone_get_source_point (clone,
                                               coords->x, coords->y,
                                               &nnx, &nny);

      clone_tool->src_x = floor (nnx);
      clone_tool->src_y = floor (nny);

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

static void
clay_perspective_clone_tool_modifier_key (ClayTool        *tool,
                                          GdkModifierType  key,
                                          gboolean         press,
                                          GdkModifierType  state,
                                          ClayDisplay     *display)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_PAINT &&
      key == clay_get_toggle_behavior_mask ())
    {
      if (press)
        {
          clone_tool->saved_precision =
            clay_tool_control_get_precision (tool->control);
          clay_tool_control_set_precision (tool->control,
                                           CLAY_CURSOR_PRECISION_PIXEL_CENTER);
        }
      else
        {
          clay_tool_control_set_precision (tool->control,
                                           clone_tool->saved_precision);
        }
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
clay_perspective_clone_tool_cursor_update (ClayTool         *tool,
                                           const ClayCoords *coords,
                                           GdkModifierType   state,
                                           ClayDisplay      *display)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPerspectiveCloneOptions *options;
  ClayImage                   *image;
  ClayToolClass               *tool_class;
  ClayCursorType               cursor      = CLAY_CURSOR_MOUSE;
  ClayToolCursorType           tool_cursor = CLAY_TOOL_CURSOR_NONE;
  ClayCursorModifier           modifier    = CLAY_CURSOR_MODIFIER_NONE;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  image = clay_display_get_image (display);

  if (clay_image_coords_in_active_pickable (image, coords,
                                            FALSE, FALSE, TRUE))
    {
      cursor = CLAY_CURSOR_MOUSE;
    }

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              clay_tool_widget_get_cursor (clone_tool->widget,
                                           coords, state,
                                           &cursor, &tool_cursor, &modifier);
            }
        }
    }
  else
    {
      GdkModifierType extend_mask = clay_get_extend_selection_mask ();
      GdkModifierType toggle_mask = clay_get_toggle_behavior_mask ();

      if ((state & (toggle_mask | extend_mask)) == toggle_mask)
        {
          cursor = CLAY_CURSOR_CROSSHAIR_SMALL;
        }
      else if (! CLAY_SOURCE_OPTIONS (options)->src_drawables)
        {
          modifier = CLAY_CURSOR_MODIFIER_BAD;
        }

      tool_cursor = CLAY_TOOL_CURSOR_CLONE;
    }

  clay_tool_control_set_cursor          (tool->control, cursor);
  clay_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  /*  If we are in adjust mode, skip the ClayBrushClass when chaining up.
   *  This ensures that the cursor will be set regardless of
   *  ClayBrushTool::show_cursor (see bug #354933).
   */
  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    tool_class = CLAY_TOOL_CLASS (g_type_class_peek_parent (parent_class));
  else
    tool_class = CLAY_TOOL_CLASS (parent_class);

  tool_class->cursor_update (tool, coords, state, display);
}

static void
clay_perspective_clone_tool_oper_update (ClayTool         *tool,
                                         const ClayCoords *coords,
                                         GdkModifierType   state,
                                         gboolean          proximity,
                                         ClayDisplay      *display)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          if (display == tool->display)
            {
              clay_tool_widget_hover (clone_tool->widget, coords, state,
                                      proximity);
            }
        }
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);

      if (proximity)
        {
          ClayPaintCore        *core        = CLAY_PAINT_TOOL (tool)->core;
          ClayPerspectiveClone *clone       = CLAY_PERSPECTIVE_CLONE (core);
          ClaySourceCore       *source_core = CLAY_SOURCE_CORE (core);

          if (CLAY_SOURCE_OPTIONS (options)->src_drawables == NULL)
            {
              clay_tool_replace_status (tool, display,
                                        _("Ctrl-Click to set a clone source"));
            }
          else
            {
              clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

              g_object_get (options,
                            "src-x", &clone_tool->src_x,
                            "src-y", &clone_tool->src_y,
                            NULL);

              if (! source_core->first_stroke)
                {
                  if (CLAY_SOURCE_OPTIONS (options)->align_mode ==
                      CLAY_SOURCE_ALIGN_YES)
                    {
                      gdouble nnx, nny;

                      /* Set the coordinates for the reference cross */
                      clay_perspective_clone_get_source_point (clone,
                                                               coords->x,
                                                               coords->y,
                                                               &nnx, &nny);

                      clone_tool->src_x = floor (nnx);
                      clone_tool->src_y = floor (nny);
                    }
                }

              clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
            }
        }
    }
}

static void
clay_perspective_clone_tool_options_notify (ClayTool         *tool,
                                            ClayToolOptions  *options,
                                            const GParamSpec *pspec)
{
  ClayPerspectiveCloneTool    *clone_tool = CLAY_PERSPECTIVE_CLONE_TOOL (tool);
  ClayPaintTool               *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayPerspectiveCloneOptions *clone_options;

  clone_options = CLAY_PERSPECTIVE_CLONE_OPTIONS (options);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "clone-mode"))
    {
      ClayPerspectiveClone *clone;

      clone = CLAY_PERSPECTIVE_CLONE (CLAY_PAINT_TOOL (tool)->core);

      clay_draw_tool_pause (CLAY_DRAW_TOOL (clone_tool));

      if (clone_options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_PAINT)
        {
          clay_perspective_clone_set_transform (clone, &clone_tool->transform);

          clay_paint_tool_set_active (paint_tool, TRUE);
        }
      else
        {
          clay_paint_tool_set_active (paint_tool, FALSE);

          clay_tool_control_set_precision (tool->control,
                                           CLAY_CURSOR_PRECISION_SUBPIXEL);

          /*  start drawing the bounding box and handles...  */
          if (tool->display &&
              ! clay_draw_tool_is_active (CLAY_DRAW_TOOL (clone_tool)))
            {
              clay_draw_tool_start (CLAY_DRAW_TOOL (clone_tool), tool->display);
            }
        }

      clay_draw_tool_resume (CLAY_DRAW_TOOL (clone_tool));
    }
}

static void
clay_perspective_clone_tool_draw (ClayDrawTool *draw_tool)
{
  ClayTool                    *tool        = CLAY_TOOL (draw_tool);
  ClayPerspectiveCloneTool    *clone_tool  = CLAY_PERSPECTIVE_CLONE_TOOL (draw_tool);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  if (options->clone_mode == CLAY_PERSPECTIVE_CLONE_MODE_ADJUST)
    {
      if (clone_tool->widget)
        {
          ClayCanvasItem *item = clay_tool_widget_get_item (clone_tool->widget);

          clay_draw_tool_add_item (draw_tool, item);
        }
    }
  else
    {
      ClayCanvasGroup *stroke_group;

      stroke_group = clay_draw_tool_add_stroke_group (draw_tool);

      /*  draw the bounding box  */
      clay_draw_tool_push_group (draw_tool, stroke_group);

      clay_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0],
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1]);
      clay_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X1],
                               clone_tool->trans_info[Y1],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      clay_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X3],
                               clone_tool->trans_info[Y3]);
      clay_draw_tool_add_line (draw_tool,
                               clone_tool->trans_info[X2],
                               clone_tool->trans_info[Y2],
                               clone_tool->trans_info[X0],
                               clone_tool->trans_info[Y0]);

      clay_draw_tool_pop_group (draw_tool);
    }

  if (CLAY_SOURCE_OPTIONS (options)->src_drawables && clone_tool->src_display)
    {
      ClayDisplay *tmp_display;

      tmp_display = draw_tool->display;
      draw_tool->display = clone_tool->src_display;

      clay_draw_tool_add_handle (draw_tool,
                                 CLAY_HANDLE_CROSS,
                                 clone_tool->src_x + 0.5,
                                 clone_tool->src_y + 0.5,
                                 CLAY_TOOL_HANDLE_SIZE_CROSS,
                                 CLAY_TOOL_HANDLE_SIZE_CROSS,
                                 CLAY_HANDLE_ANCHOR_CENTER);

      draw_tool->display = tmp_display;
    }

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
clay_perspective_clone_tool_halt (ClayPerspectiveCloneTool *clone_tool)
{
  ClayTool                    *tool = CLAY_TOOL (clone_tool);
  ClayPerspectiveCloneOptions *options;

  options = CLAY_PERSPECTIVE_CLONE_TOOL_GET_OPTIONS (tool);

  clone_tool->src_display = NULL;

  g_object_set (options,
                "src-drawables", NULL,
                NULL);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  g_clear_object (&clone_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
clay_perspective_clone_tool_bounds (ClayPerspectiveCloneTool *clone_tool,
                                    ClayDisplay              *display)
{
  ClayImage *image = clay_display_get_image (display);

  clone_tool->x1 = 0;
  clone_tool->y1 = 0;
  clone_tool->x2 = clay_image_get_width  (image);
  clone_tool->y2 = clay_image_get_height (image);
}

static void
clay_perspective_clone_tool_prepare (ClayPerspectiveCloneTool *clone_tool)
{
  clone_tool->trans_info[PIVOT_X] = (gdouble) (clone_tool->x1 + clone_tool->x2) / 2.0;
  clone_tool->trans_info[PIVOT_Y] = (gdouble) (clone_tool->y1 + clone_tool->y2) / 2.0;

  clone_tool->trans_info[X0] = clone_tool->x1;
  clone_tool->trans_info[Y0] = clone_tool->y1;
  clone_tool->trans_info[X1] = clone_tool->x2;
  clone_tool->trans_info[Y1] = clone_tool->y1;
  clone_tool->trans_info[X2] = clone_tool->x1;
  clone_tool->trans_info[Y2] = clone_tool->y2;
  clone_tool->trans_info[X3] = clone_tool->x2;
  clone_tool->trans_info[Y3] = clone_tool->y2;
}

static void
clay_perspective_clone_tool_recalc_matrix (ClayPerspectiveCloneTool *clone_tool,
                                           ClayToolWidget           *widget)
{
  clay_matrix3_identity (&clone_tool->transform);
  clay_transform_matrix_perspective (&clone_tool->transform,
                                     clone_tool->x1,
                                     clone_tool->y1,
                                     clone_tool->x2 - clone_tool->x1,
                                     clone_tool->y2 - clone_tool->y1,
                                     clone_tool->trans_info[X0],
                                     clone_tool->trans_info[Y0],
                                     clone_tool->trans_info[X1],
                                     clone_tool->trans_info[Y1],
                                     clone_tool->trans_info[X2],
                                     clone_tool->trans_info[Y2],
                                     clone_tool->trans_info[X3],
                                     clone_tool->trans_info[Y3]);

  if (widget)
    g_object_set (widget,
                  "transform", &clone_tool->transform,
                  "x1",        (gdouble) clone_tool->x1,
                  "y1",        (gdouble) clone_tool->y1,
                  "x2",        (gdouble) clone_tool->x2,
                  "y2",        (gdouble) clone_tool->y2,
                  "pivot-x",   clone_tool->trans_info[PIVOT_X],
                  "pivot-y",   clone_tool->trans_info[PIVOT_Y],
                  NULL);
}

static void
clay_perspective_clone_tool_widget_changed (ClayToolWidget           *widget,
                                            ClayPerspectiveCloneTool *clone_tool)
{
  ClayMatrix3 *transform;

  g_object_get (widget,
                "transform", &transform,
                "pivot-x",   &clone_tool->trans_info[PIVOT_X],
                "pivot-y",   &clone_tool->trans_info[PIVOT_Y],
                NULL);

  clay_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y1,
                                &clone_tool->trans_info[X0],
                                &clone_tool->trans_info[Y0]);
  clay_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y1,
                                &clone_tool->trans_info[X1],
                                &clone_tool->trans_info[Y1]);
  clay_matrix3_transform_point (transform,
                                clone_tool->x1, clone_tool->y2,
                                &clone_tool->trans_info[X2],
                                &clone_tool->trans_info[Y2]);
  clay_matrix3_transform_point (transform,
                                clone_tool->x2, clone_tool->y2,
                                &clone_tool->trans_info[X3],
                                &clone_tool->trans_info[Y3]);

  g_free (transform);

  clay_perspective_clone_tool_recalc_matrix (clone_tool, NULL);
}

static void
clay_perspective_clone_tool_widget_status (ClayToolWidget           *widget,
                                           const gchar              *status,
                                           ClayPerspectiveCloneTool *clone_tool)
{
  ClayTool *tool = CLAY_TOOL (clone_tool);

  if (status)
    {
      clay_tool_replace_status (tool, tool->display, "%s", status);
    }
  else
    {
      clay_tool_pop_status (tool, tool->display);
    }
}


/*  tool options stuff  */

static GtkWidget *
clay_perspective_clone_options_gui (ClayToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = clay_clone_options_gui (tool_options);
  GtkWidget *mode;

  /* radio buttons to set if you are modifying the perspective plane
   * or painting
   */
  mode = clay_prop_enum_radio_box_new (config, "clone-mode", 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), mode, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), mode, 0);

  return vbox;
}
