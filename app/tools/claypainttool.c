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

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claydrawable.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylinklayer.h"
#include "core/claypaintinfo.h"
#include "core/clayprojection.h"
#include "core/claytoolinfo.h"

#include "paint/claypaintcore.h"
#include "paint/claypaintoptions.h"

#include "path/clayvectorlayer.h"

#include "widgets/claydevices.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-selection.h"
#include "display/claydisplayshell-utils.h"

#include "claycoloroptions.h"
#include "claypaintoptions-gui.h"
#include "claypainttool.h"
#include "claypainttool-paint.h"
#include "claysourcetool.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


static void   clay_paint_tool_constructed    (GObject               *object);
static void   clay_paint_tool_finalize       (GObject               *object);

static void   clay_paint_tool_control        (ClayTool              *tool,
                                              ClayToolAction         action,
                                              ClayDisplay           *display);
static void   clay_paint_tool_button_press   (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonPressType    press_type,
                                              ClayDisplay           *display);
static void   clay_paint_tool_button_release (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonReleaseType  release_type,
                                              ClayDisplay           *display);
static void   clay_paint_tool_motion         (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);
static void   clay_paint_tool_modifier_key   (ClayTool              *tool,
                                              GdkModifierType        key,
                                              gboolean               press,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);
static void   clay_paint_tool_cursor_update  (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);
static void   clay_paint_tool_oper_update    (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              ClayDisplay           *display);

static void   clay_paint_tool_draw           (ClayDrawTool          *draw_tool);

static void
          clay_paint_tool_real_paint_prepare (ClayPaintTool         *paint_tool,
                                              ClayDisplay           *display);

static ClayCanvasItem *
              clay_paint_tool_get_outline    (ClayPaintTool         *paint_tool,
                                              ClayDisplay           *display,
                                              gdouble                x,
                                              gdouble                y);

static gboolean  clay_paint_tool_check_alpha (ClayPaintTool         *paint_tool,
                                              ClayDrawable          *drawable,
                                              ClayDisplay           *display,
                                              GError               **error);

static void   clay_paint_tool_hard_notify    (ClayPaintOptions      *options,
                                              const GParamSpec      *pspec,
                                              ClayPaintTool         *paint_tool);
static void   clay_paint_tool_cursor_notify  (ClayDisplayConfig     *config,
                                              GParamSpec            *pspec,
                                              ClayPaintTool         *paint_tool);


G_DEFINE_TYPE (ClayPaintTool, clay_paint_tool, CLAY_TYPE_COLOR_TOOL)

#define parent_class clay_paint_tool_parent_class


static void
clay_paint_tool_class_init (ClayPaintToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = clay_paint_tool_constructed;
  object_class->finalize     = clay_paint_tool_finalize;

  tool_class->control        = clay_paint_tool_control;
  tool_class->button_press   = clay_paint_tool_button_press;
  tool_class->button_release = clay_paint_tool_button_release;
  tool_class->motion         = clay_paint_tool_motion;
  tool_class->modifier_key   = clay_paint_tool_modifier_key;
  tool_class->cursor_update  = clay_paint_tool_cursor_update;
  tool_class->oper_update    = clay_paint_tool_oper_update;

  draw_tool_class->draw      = clay_paint_tool_draw;

  klass->paint_prepare       = clay_paint_tool_real_paint_prepare;
}

static void
clay_paint_tool_init (ClayPaintTool *paint_tool)
{
  ClayTool *tool = CLAY_TOOL (paint_tool);

  clay_tool_control_set_motion_mode    (tool->control, CLAY_MOTION_MODE_EXACT);
  clay_tool_control_set_scroll_lock    (tool->control, TRUE);
  clay_tool_control_set_action_opacity (tool->control,
                                        "context-opacity-set");

  paint_tool->active          = TRUE;
  paint_tool->pick_colors     = FALSE;
  paint_tool->can_multi_paint = FALSE;
  paint_tool->draw_line       = FALSE;

  paint_tool->show_cursor   = TRUE;
  paint_tool->draw_brush    = TRUE;
  paint_tool->snap_brush    = FALSE;
  paint_tool->draw_fallback = FALSE;
  paint_tool->fallback_size = 0.0;
  paint_tool->draw_circle   = FALSE;
  paint_tool->circle_size   = 0.0;

  paint_tool->status      = _("Click to paint");
  paint_tool->status_line = _("Click to draw the line");
  paint_tool->status_ctrl = _("%s to pick a color");

  paint_tool->core        = NULL;
}

static void
clay_paint_tool_constructed (GObject *object)
{
  ClayTool          *tool       = CLAY_TOOL (object);
  ClayPaintTool     *paint_tool = CLAY_PAINT_TOOL (object);
  ClayPaintOptions  *options    = CLAY_PAINT_TOOL_GET_OPTIONS (tool);
  ClayDisplayConfig *display_config;
  ClayPaintInfo     *paint_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_TOOL_INFO (tool->tool_info));
  clay_assert (CLAY_IS_PAINT_INFO (tool->tool_info->paint_info));

  display_config = CLAY_DISPLAY_CONFIG (tool->tool_info->clay->config);

  paint_info = tool->tool_info->paint_info;

  clay_assert (g_type_is_a (paint_info->paint_type, CLAY_TYPE_PAINT_CORE));

  paint_tool->core = g_object_new (paint_info->paint_type,
                                   "undo-desc", paint_info->blurb,
                                   NULL);

  g_signal_connect_object (options, "notify::hard",
                           G_CALLBACK (clay_paint_tool_hard_notify),
                           paint_tool, 0);

  clay_paint_tool_hard_notify (options, NULL, paint_tool);

  paint_tool->show_cursor = display_config->show_paint_tool_cursor;
  paint_tool->draw_brush  = display_config->show_brush_outline;
  paint_tool->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (clay_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (clay_paint_tool_cursor_notify),
                           paint_tool, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (clay_paint_tool_cursor_notify),
                           paint_tool, 0);
}

static void
clay_paint_tool_finalize (GObject *object)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (object);

  if (paint_tool->core)
    {
      g_object_unref (paint_tool->core);
      paint_tool->core = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_paint_tool_control (ClayTool       *tool,
                         ClayToolAction  action,
                         ClayDisplay    *display)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_paint_core_cleanup (paint_tool->core);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_paint_tool_button_press (ClayTool            *tool,
                              const ClayCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              ClayButtonPressType  press_type,
                              ClayDisplay         *display)
{
  ClayDrawTool     *draw_tool  = CLAY_DRAW_TOOL (tool);
  ClayPaintTool    *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayPaintOptions *options    = CLAY_PAINT_TOOL_GET_OPTIONS (tool);
  ClayGuiConfig    *config     = CLAY_GUI_CONFIG (display->clay->config);
  ClayDisplayShell *shell      = clay_display_get_shell (display);
  ClayImage        *image      = clay_display_get_image (display);
  GList            *drawables;
  GList            *iter;
  gboolean          constrain;
  GError           *error = NULL;

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
      return;
    }

  drawables  = clay_image_get_selected_drawables (image);
  if (drawables == NULL)
    {
      clay_tool_message_literal (tool, display,
                                 _("No selected drawables."));

      return;
    }
  else if (! paint_tool->can_multi_paint)
    {
      if (g_list_length (drawables) != 1)
        {
          clay_tool_message_literal (tool, display,
                                     _("Cannot paint on multiple layers. Select only one layer."));

          g_list_free (drawables);
          return;
        }
    }

  for (iter = drawables; iter; iter = iter->next)
    {
      ClayDrawable *drawable = iter->data;

      if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
        {
          clay_tool_message_literal (tool, display,
                                     _("Cannot paint on layer groups."));
          g_list_free (drawables);

          return;
        }

      if (! clay_paint_tool_check_alpha (paint_tool, drawable, display, &error))
        {
          GtkWidget *options_gui;
          GtkWidget *mode_box;

          clay_tool_message_literal (tool, display, error->message);

          options_gui = clay_tools_get_tool_options_gui (CLAY_TOOL_OPTIONS (options));
          mode_box    = clay_paint_options_gui_get_paint_mode_box (options_gui);

          if (gtk_widget_is_sensitive (mode_box))
            {
              clay_tools_show_tool_options (display->clay);
              clay_widget_blink (mode_box);
            }

          g_clear_error (&error);
          g_list_free (drawables);

          return;
        }

      if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
          ! config->edit_non_visible)
        {
          clay_tool_message_literal (tool, display,
                                     _("A selected layer is not visible."));
          g_list_free (drawables);

          return;
        }
    }

  if (clay_draw_tool_is_active (draw_tool))
    clay_draw_tool_stop (draw_tool);

  if (tool->display            &&
      tool->display != display &&
      clay_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  constrain = (state & clay_get_constrain_behavior_mask ()) != 0;

  if (! clay_paint_tool_paint_start (paint_tool,
                                     display, coords, time, constrain,
                                     &error))
    {
      clay_tool_message_literal (tool, display,
                                 error ? error->message : _("Unknown error"));
      g_clear_error (&error);
      return;
    }

  tool->display   = display;
  tool->drawables = drawables;

  /*  pause the current selection  */
  clay_display_shell_selection_pause (shell);

  clay_draw_tool_start (draw_tool, display);

  clay_tool_control_activate (tool->control);
}

static void
clay_paint_tool_button_release (ClayTool              *tool,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type,
                                ClayDisplay           *display)
{
  ClayPaintTool    *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayDisplayShell *shell      = clay_display_get_shell (display);
  ClayImage        *image      = clay_display_get_image (display);
  gboolean          cancel;

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                      state, release_type,
                                                      display);
      return;
    }

  cancel = (release_type == CLAY_BUTTON_RELEASE_CANCEL);

  clay_paint_tool_paint_end (paint_tool, time, cancel);

  clay_tool_control_halt (tool->control);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  /*  resume the current selection  */
  clay_display_shell_selection_resume (shell);

  clay_image_flush (image);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_paint_tool_motion (ClayTool         *tool,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        ClayDisplay      *display)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);

  CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    return;

  if (! paint_tool->snap_brush)
    clay_draw_tool_pause  (CLAY_DRAW_TOOL (tool));

  clay_paint_tool_paint_motion (paint_tool, coords, time);

  if (! paint_tool->snap_brush)
    clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_paint_tool_modifier_key (ClayTool        *tool,
                              GdkModifierType  key,
                              gboolean         press,
                              GdkModifierType  state,
                              ClayDisplay     *display)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayDrawTool  *draw_tool  = CLAY_DRAW_TOOL (tool);

  if (paint_tool->pick_colors && ! paint_tool->draw_line)
    {
      if ((state & clay_get_all_modifiers_mask ()) ==
          clay_get_constrain_behavior_mask ())
        {
          if (! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
            {
              ClayToolInfo *info = clay_get_tool_info (display->clay,
                                                       "clay-color-picker-tool");

              if (CLAY_IS_TOOL_INFO (info))
                {
                  if (clay_draw_tool_is_active (draw_tool))
                    clay_draw_tool_stop (draw_tool);

                  clay_color_tool_enable (CLAY_COLOR_TOOL (tool),
                                          CLAY_COLOR_OPTIONS (info->tool_options));

                  switch (CLAY_COLOR_TOOL (tool)->pick_target)
                    {
                    case CLAY_COLOR_PICK_TARGET_FOREGROUND:
                      clay_tool_push_status (tool, display,
                                             _("Click in any image to pick the "
                                               "foreground color"));
                      break;

                    case CLAY_COLOR_PICK_TARGET_BACKGROUND:
                      clay_tool_push_status (tool, display,
                                             _("Click in any image to pick the "
                                               "background color"));
                      break;

                    default:
                      break;
                    }
                }
            }
        }
      else
        {
          if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
            {
              clay_tool_pop_status (tool, display);
              clay_color_tool_disable (CLAY_COLOR_TOOL (tool));
            }
        }
    }
}

static void
clay_paint_tool_cursor_update (ClayTool         *tool,
                               const ClayCoords *coords,
                               GdkModifierType   state,
                               ClayDisplay      *display)
{
  ClayPaintTool      *paint_tool = CLAY_PAINT_TOOL (tool);
  ClayGuiConfig      *config     = CLAY_GUI_CONFIG (display->clay->config);
  ClayCursorModifier  modifier;
  ClayCursorModifier  toggle_modifier;
  ClayCursorModifier  old_modifier;
  ClayCursorModifier  old_toggle_modifier;

  modifier        = tool->control->cursor_modifier;
  toggle_modifier = tool->control->toggle_cursor_modifier;

  old_modifier        = modifier;
  old_toggle_modifier = toggle_modifier;

  if (! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      ClayImage    *image     = clay_display_get_image (display);
      GList        *drawables = clay_image_get_selected_drawables (image);
      GList        *iter;

      if (! drawables)
        return;

      for (iter = drawables; iter; iter = iter->next)
        {
          ClayDrawable *drawable = iter->data;

          if (clay_viewable_get_children (CLAY_VIEWABLE (drawable))               ||
              clay_item_is_content_locked (CLAY_ITEM (drawable), NULL)            ||
              ! clay_paint_tool_check_alpha (paint_tool, drawable, display, NULL) ||
              ! (clay_item_is_visible (CLAY_ITEM (drawable)) ||
                 config->edit_non_visible))
            {
              gboolean constrain_only;

              /* Allow pixel-locked layers to be set as sources */
              constrain_only = (state & clay_get_constrain_behavior_mask () &&
                                ! (state & clay_get_extend_selection_mask ()));

              if (! (CLAY_IS_SOURCE_TOOL (tool) && constrain_only))
                {
                  modifier        = CLAY_CURSOR_MODIFIER_BAD;
                  toggle_modifier = CLAY_CURSOR_MODIFIER_BAD;
                  break;
                }
            }
        }

      g_list_free (drawables);

      if (! paint_tool->show_cursor &&
          modifier != CLAY_CURSOR_MODIFIER_BAD)
        {
          if (paint_tool->draw_brush)
            clay_tool_set_cursor (tool, display,
                                  CLAY_CURSOR_NONE,
                                  CLAY_TOOL_CURSOR_NONE,
                                  CLAY_CURSOR_MODIFIER_NONE);
          else
            clay_tool_set_cursor (tool, display,
                                  CLAY_CURSOR_SINGLE_DOT,
                                  CLAY_TOOL_CURSOR_NONE,
                                  CLAY_CURSOR_MODIFIER_NONE);
          return;
        }

      clay_tool_control_set_cursor_modifier        (tool->control,
                                                    modifier);
      clay_tool_control_set_toggle_cursor_modifier (tool->control,
                                                    toggle_modifier);
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);

  /*  reset old stuff here so we are not interfering with the modifiers
   *  set by our subclasses
   */
  clay_tool_control_set_cursor_modifier        (tool->control,
                                                old_modifier);
  clay_tool_control_set_toggle_cursor_modifier (tool->control,
                                                old_toggle_modifier);
}

static void
clay_paint_tool_oper_update (ClayTool         *tool,
                             const ClayCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             ClayDisplay      *display)
{
  ClayPaintTool    *paint_tool    = CLAY_PAINT_TOOL (tool);
  ClayDrawTool     *draw_tool     = CLAY_DRAW_TOOL (tool);
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (tool);
  ClayPaintCore    *core          = paint_tool->core;
  ClayDisplayShell *shell         = clay_display_get_shell (display);
  ClayImage        *image         = clay_display_get_image (display);
  GList            *drawables;
  gchar            *status        = NULL;

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
      return;
    }

  clay_draw_tool_pause (draw_tool);

  if (clay_draw_tool_is_active (draw_tool) &&
      draw_tool->display != display)
    clay_draw_tool_stop (draw_tool);

  if (tool->display            &&
      tool->display != display &&
      clay_display_get_image (tool->display) == image)
    {
      /*  if this is a different display, but the same image, HACK around
       *  in tool internals AFTER stopping the current draw_tool, so
       *  straight line drawing works across different views of the
       *  same image.
       */

      tool->display = display;
    }

  drawables = clay_image_get_selected_drawables (image);

  if ((g_list_length (drawables) == 1 ||
       (g_list_length (drawables) > 1 && paint_tool->can_multi_paint)) &&
      proximity)
    {
      gboolean  constrain_mask = clay_get_constrain_behavior_mask ();

      core->cur_coords = *coords;

      if (display == tool->display && (state & CLAY_PAINT_TOOL_LINE_MASK))
        {
          /*  If shift is down and this is not the first paint stroke,
           *  draw a line.
           */
          gchar   *status_help;
          gdouble  offset_angle;
          gdouble  xres, yres;

          clay_display_shell_get_constrained_line_params (shell,
                                                          &offset_angle,
                                                          &xres, &yres);

          clay_paint_core_round_line (core, paint_options,
                                      (state & constrain_mask) != 0,
                                      offset_angle, xres, yres);

          status_help = clay_suggest_modifiers (paint_tool->status_line,
                                                constrain_mask & ~state,
                                                NULL,
                                                _("%s for constrained angles"),
                                                NULL);

          status = clay_display_shell_get_line_status (shell, status_help,
                                                       ". ",
                                                       core->last_coords.x,
                                                       core->last_coords.y,
                                                       core->cur_coords.x,
                                                       core->cur_coords.y);
          g_free (status_help);
          paint_tool->draw_line = TRUE;
        }
      else
        {
          GdkModifierType  modifiers = 0;

          /* HACK: A paint tool may set status_ctrl to NULL to indicate that
           * it ignores the Ctrl modifier (temporarily or permanently), so
           * it should not be suggested.  This is different from how
           * clay_suggest_modifiers() would interpret this parameter.
           */
          if (paint_tool->status_ctrl != NULL)
            modifiers |= constrain_mask;

          /* suggest drawing lines only after the first point is set
           */
          if (display == tool->display)
            modifiers |= CLAY_PAINT_TOOL_LINE_MASK;

          status = clay_suggest_modifiers (paint_tool->status,
                                           modifiers & ~state,
                                           _("%s for a straight line"),
                                           paint_tool->status_ctrl,
                                           NULL);
          paint_tool->draw_line = FALSE;
        }

      paint_tool->cursor_x = core->cur_coords.x;
      paint_tool->cursor_y = core->cur_coords.y;

      if (! clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_start (draw_tool, display);
    }
  else if (clay_draw_tool_is_active (draw_tool))
    {
      clay_draw_tool_stop (draw_tool);
    }

  if (status != NULL)
    {
      clay_tool_push_status (tool, display, "%s", status);
      g_free (status);
    }
  else
    {
      clay_tool_pop_status (tool, display);
    }

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);

  clay_draw_tool_resume (draw_tool);
  g_list_free (drawables);
}

static void
clay_paint_tool_draw (ClayDrawTool *draw_tool)
{
  ClayPaintTool *paint_tool = CLAY_PAINT_TOOL (draw_tool);

  if (paint_tool->active &&
      ! clay_color_tool_is_enabled (CLAY_COLOR_TOOL (draw_tool)))
    {
      ClayPaintCore  *core       = paint_tool->core;
      ClayCanvasItem *outline    = NULL;
      gboolean        line_drawn = FALSE;
      gdouble         cur_x, cur_y;

      if (clay_paint_tool_paint_is_active (paint_tool) &&
          paint_tool->snap_brush)
        {
          cur_x = paint_tool->paint_x;
          cur_y = paint_tool->paint_y;
        }
      else
        {
          cur_x = paint_tool->cursor_x;
          cur_y = paint_tool->cursor_y;

          if (paint_tool->draw_line &&
              ! clay_tool_control_is_active (CLAY_TOOL (draw_tool)->control))
            {
              ClayCanvasGroup *group;
              gdouble          last_x, last_y;

              last_x = core->last_coords.x;
              last_y = core->last_coords.y;

              group = clay_draw_tool_add_stroke_group (draw_tool);
              clay_draw_tool_push_group (draw_tool, group);

              clay_draw_tool_add_handle (draw_tool,
                                         CLAY_HANDLE_CIRCLE,
                                         last_x, last_y,
                                         CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                         CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                         CLAY_HANDLE_ANCHOR_CENTER);

              clay_draw_tool_add_line (draw_tool,
                                       last_x, last_y,
                                       cur_x, cur_y);

              clay_draw_tool_add_handle (draw_tool,
                                         CLAY_HANDLE_CIRCLE,
                                         cur_x, cur_y,
                                         CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                         CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                         CLAY_HANDLE_ANCHOR_CENTER);

              clay_draw_tool_pop_group (draw_tool);

              line_drawn = TRUE;
            }
        }

      clay_paint_tool_set_draw_fallback (paint_tool, FALSE, 0.0);
      clay_paint_tool_set_draw_circle (paint_tool, FALSE, 0.0);

      if (paint_tool->draw_brush)
        outline = clay_paint_tool_get_outline (paint_tool,
                                               draw_tool->display,
                                               cur_x, cur_y);

      if (outline)
        {
          clay_draw_tool_add_item (draw_tool, outline);
          g_object_unref (outline);
        }
      else if (paint_tool->draw_fallback)
        {
          /* Lets make a sensible fallback cursor
           *
           * Sensible cursor is
           *  * crossed to indicate draw point
           *  * reactive to options alterations
           *  * not a full circle that would be in the way
           */
          gint size = (gint) paint_tool->fallback_size;

#define TICKMARK_ANGLE 48
#define ROTATION_ANGLE G_PI / 4

          /* marks for indicating full size */
          clay_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          clay_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI / 2 - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          clay_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + G_PI - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);

          clay_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  ROTATION_ANGLE + 3 * G_PI / 2 - (2.0 * G_PI) / (TICKMARK_ANGLE * 2),
                                  (2.0 * G_PI) / TICKMARK_ANGLE);
        }
      else if (paint_tool->draw_circle)
        {
          gint size = (gint) paint_tool->circle_size;

          /* draw an indicatory circle */
          clay_draw_tool_add_arc (draw_tool,
                                  FALSE,
                                  cur_x - (size / 2.0),
                                  cur_y - (size / 2.0),
                                  size, size,
                                  0.0, (2.0 * G_PI));
        }

      if (! outline                   &&
          ! paint_tool->draw_fallback &&
          ! line_drawn                &&
          ! paint_tool->show_cursor   &&
          ! paint_tool->draw_circle)
        {
          /* I am not sure this case can/should ever happen since now we
           * always set the CLAY_CURSOR_SINGLE_DOT when neither pointer
           * nor outline options are checked. Yet let's imagine any
           * weird case where brush outline is wanted, without pointer
           * cursor, yet we fail to draw the outline while neither
           * circle nor fallbacks are requested (it depends on per-class
           * implementation of get_outline()).
           *
           * In such a case, we don't want to leave the user without any
           * indication so we draw a fallback crosshair.
           */
          if (paint_tool->draw_brush)
            clay_draw_tool_add_handle (draw_tool,
                                       CLAY_HANDLE_CIRCLE,
                                       cur_x, cur_y,
                                       CLAY_TOOL_HANDLE_SIZE_CROSSHAIR,
                                       CLAY_TOOL_HANDLE_SIZE_CROSSHAIR,
                                       CLAY_HANDLE_ANCHOR_CENTER);
        }
    }

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);
}

static void
clay_paint_tool_real_paint_prepare (ClayPaintTool *paint_tool,
                                    ClayDisplay   *display)
{
  ClayDisplayShell *shell = clay_display_get_shell (display);

  clay_paint_core_set_show_all (paint_tool->core, shell->show_all);
}

static ClayCanvasItem *
clay_paint_tool_get_outline (ClayPaintTool *paint_tool,
                             ClayDisplay   *display,
                             gdouble        x,
                             gdouble        y)
{
  if (CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline)
    return CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->get_outline (paint_tool,
                                                                display, x, y);

  return NULL;
}

static gboolean
clay_paint_tool_check_alpha (ClayPaintTool  *paint_tool,
                             ClayDrawable   *drawable,
                             ClayDisplay    *display,
                             GError        **error)
{
  ClayPaintToolClass *klass = CLAY_PAINT_TOOL_GET_CLASS (paint_tool);

  if (klass->is_alpha_only && klass->is_alpha_only (paint_tool, drawable))
    {
      ClayLayer *locked_layer = NULL;

      if (! clay_drawable_has_alpha (drawable))
        {
          g_set_error_literal (
            error, CLAY_ERROR, CLAY_FAILED,
            _("The selected drawable does not have an alpha channel."));

          return FALSE;
        }

        if (CLAY_IS_LAYER (drawable) &&
            clay_layer_is_alpha_locked (CLAY_LAYER (drawable),
                                        &locked_layer))
        {
          g_set_error_literal (
            error, CLAY_ERROR, CLAY_FAILED,
            _("The selected layer's alpha channel is locked."));

          if (error)
            clay_tools_blink_lock_box (display->clay, CLAY_ITEM (locked_layer));

          return FALSE;
        }
    }

  return TRUE;
}

static void
clay_paint_tool_hard_notify (ClayPaintOptions *options,
                             const GParamSpec *pspec,
                             ClayPaintTool    *paint_tool)
{
  if (paint_tool->active)
    {
      ClayTool *tool = CLAY_TOOL (paint_tool);

      clay_tool_control_set_precision (tool->control,
                                       options->hard ?
                                       CLAY_CURSOR_PRECISION_PIXEL_CENTER :
                                       CLAY_CURSOR_PRECISION_SUBPIXEL);
    }
}

static void
clay_paint_tool_cursor_notify (ClayDisplayConfig *config,
                               GParamSpec        *pspec,
                               ClayPaintTool     *paint_tool)
{
  clay_draw_tool_pause (CLAY_DRAW_TOOL (paint_tool));

  paint_tool->show_cursor = config->show_paint_tool_cursor;
  paint_tool->draw_brush  = config->show_brush_outline;
  paint_tool->snap_brush  = config->snap_brush_outline;

  clay_draw_tool_resume (CLAY_DRAW_TOOL (paint_tool));
}

void
clay_paint_tool_set_active (ClayPaintTool *tool,
                            gboolean       active)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  if (active != tool->active)
    {
      ClayPaintOptions *options = CLAY_PAINT_TOOL_GET_OPTIONS (tool);

      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

      tool->active = active;

      if (active)
        clay_paint_tool_hard_notify (options, NULL, tool);

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
}

/**
 * clay_paint_tool_enable_color_picker:
 * @tool:   a #ClayPaintTool
 * @target: the #ClayColorPickTarget to set
 *
 * This is a convenience function used from the init method of paint
 * tools that want the color picking functionality. The @mode that is
 * set here is used to decide what cursor modifier to draw and if the
 * picked color goes to the foreground or background color.
 **/
void
clay_paint_tool_enable_color_picker (ClayPaintTool       *tool,
                                     ClayColorPickTarget  target)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  tool->pick_colors = TRUE;

  CLAY_COLOR_TOOL (tool)->pick_target = target;
}

/**
 * clay_paint_tool_enable_multi_paint:
 * @tool: a #ClayPaintTool
 *
 * This is a convenience function used from the init method of paint
 * tools that want to allow painting with several drawables.
 **/
void
clay_paint_tool_enable_multi_paint (ClayPaintTool *tool)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  tool->can_multi_paint = TRUE;
}

void
clay_paint_tool_set_draw_fallback (ClayPaintTool *tool,
                                   gboolean       draw_fallback,
                                   gint           fallback_size)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  tool->draw_fallback = draw_fallback;
  tool->fallback_size = fallback_size;
}

void
clay_paint_tool_set_draw_circle (ClayPaintTool *tool,
                                 gboolean       draw_circle,
                                 gint           circle_size)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  tool->draw_circle = draw_circle;
  tool->circle_size = circle_size;
}


/**
 * clay_paint_tool_force_draw:
 * @tool:
 * @force:
 *
 * If @force is %TRUE, the brush, or a fallback, or circle will be
 * drawn, regardless of the Preferences settings. This can be used for
 * code such as when modifying brush size or shape on-canvas with a
 * visual feedback, temporarily bypassing the user setting.
 */
void
clay_paint_tool_force_draw (ClayPaintTool *tool,
                            gboolean       force)
{
  ClayDisplayConfig *display_config;

  g_return_if_fail (CLAY_IS_PAINT_TOOL (tool));

  display_config = CLAY_DISPLAY_CONFIG (CLAY_TOOL (tool)->tool_info->clay->config);

  if (force)
    tool->draw_brush = TRUE;
  else
    tool->draw_brush  = display_config->show_brush_outline;
}
