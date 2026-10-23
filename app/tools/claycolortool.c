/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"

#include "core/clay.h"
#include "core/claydata.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-color.h"
#include "core/clayimage-pick-item.h"
#include "core/clayimage-sample-points.h"
#include "core/claymarshal.h"
#include "core/claysamplepoint.h"

#include "widgets/claycolormapeditor.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydockable.h"
#include "widgets/claydockcontainer.h"
#include "widgets/claypaletteeditor.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"

#include "claycoloroptions.h"
#include "claycolortool.h"
#include "claysamplepointtool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


enum
{
  PICKED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   clay_color_tool_finalize       (GObject               *object);

static void   clay_color_tool_button_press   (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonPressType    press_type,
                                              ClayDisplay           *display);
static void   clay_color_tool_button_release (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonReleaseType  release_type,
                                              ClayDisplay           *display);
static void   clay_color_tool_motion         (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);
static void   clay_color_tool_oper_update    (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              GdkModifierType        state,
                                              gboolean               proximity,
                                              ClayDisplay           *display);
static void   clay_color_tool_cursor_update  (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);

static void   clay_color_tool_draw           (ClayDrawTool          *draw_tool);

static gboolean
              clay_color_tool_real_can_pick  (ClayColorTool         *color_tool,
                                              const ClayCoords      *coords,
                                              ClayDisplay           *display);
static gboolean   clay_color_tool_real_pick  (ClayColorTool         *color_tool,
                                              const ClayCoords      *coords,
                                              ClayDisplay           *display,
                                              const Babl           **sample_format,
                                              gpointer               pixel,
                                              GeglColor            **color);
static void   clay_color_tool_real_picked    (ClayColorTool         *color_tool,
                                              const ClayCoords      *coords,
                                              ClayDisplay           *display,
                                              ClayColorPickState     pick_state,
                                              const Babl            *sample_format,
                                              gpointer               pixel,
                                              GeglColor             *color);

static gboolean   clay_color_tool_can_pick   (ClayColorTool         *tool,
                                              const ClayCoords      *coords,
                                              ClayDisplay           *display);
static void   clay_color_tool_pick           (ClayColorTool         *tool,
                                              const ClayCoords      *coords,
                                              ClayDisplay           *display,
                                              ClayColorPickState     pick_state);


G_DEFINE_TYPE (ClayColorTool, clay_color_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_color_tool_parent_class

static guint clay_color_tool_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_tool_class_init (ClayColorToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class   = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_class   = CLAY_DRAW_TOOL_CLASS (klass);

  clay_color_tool_signals[PICKED] =
    g_signal_new ("picked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorToolClass, picked),
                  NULL, NULL,
                  clay_marshal_VOID__POINTER_OBJECT_ENUM_POINTER_POINTER_BOXED,
                  G_TYPE_NONE, 6,
                  G_TYPE_POINTER,
                  CLAY_TYPE_DISPLAY,
                  CLAY_TYPE_COLOR_PICK_STATE,
                  G_TYPE_POINTER,
                  G_TYPE_POINTER,
                  GEGL_TYPE_COLOR);

  object_class->finalize     = clay_color_tool_finalize;

  tool_class->button_press   = clay_color_tool_button_press;
  tool_class->button_release = clay_color_tool_button_release;
  tool_class->motion         = clay_color_tool_motion;
  tool_class->oper_update    = clay_color_tool_oper_update;
  tool_class->cursor_update  = clay_color_tool_cursor_update;

  draw_class->draw           = clay_color_tool_draw;

  klass->can_pick            = clay_color_tool_real_can_pick;
  klass->pick                = clay_color_tool_real_pick;
  klass->picked              = clay_color_tool_real_picked;
}

static void
clay_color_tool_init (ClayColorTool *color_tool)
{
  ClayTool *tool = CLAY_TOOL (color_tool);

  clay_tool_control_set_action_size (tool->control,
                                     "tools-color-average-radius-set");
}

static void
clay_color_tool_finalize (GObject *object)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (object);

  g_clear_object (&color_tool->options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_color_tool_button_press (ClayTool            *tool,
                              const ClayCoords    *coords,
                              guint32              time,
                              GdkModifierType      state,
                              ClayButtonPressType  press_type,
                              ClayDisplay         *display)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          clay_sample_point_tool_start_edit (tool, display,
                                             color_tool->sample_point);
        }
      else if (clay_color_tool_can_pick (color_tool, coords, display))
        {
          clay_color_tool_pick (color_tool, coords, display,
                                CLAY_COLOR_PICK_STATE_START);

          clay_tool_control_activate (tool->control);
        }
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
}

static void
clay_color_tool_button_release (ClayTool              *tool,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type,
                                ClayDisplay           *display)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      clay_tool_control_halt (tool->control);

      if (! color_tool->sample_point &&
          clay_color_tool_can_pick (color_tool, coords, display))
        {
          clay_color_tool_pick (color_tool, coords, display,
                                CLAY_COLOR_PICK_STATE_END);
        }
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
clay_color_tool_motion (ClayTool         *tool,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        ClayDisplay      *display)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (! color_tool->sample_point)
        {
          clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

          color_tool->can_pick = clay_color_tool_can_pick (color_tool,
                                                           coords, display);
          color_tool->center_x = coords->x;
          color_tool->center_y = coords->y;

          if (color_tool->can_pick)
            {
              clay_color_tool_pick (color_tool, coords, display,
                                    CLAY_COLOR_PICK_STATE_UPDATE);
            }

          clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
        }
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static void
clay_color_tool_oper_update (ClayTool         *tool,
                             const ClayCoords *coords,
                             GdkModifierType   state,
                             gboolean          proximity,
                             ClayDisplay      *display)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      ClayDrawTool     *draw_tool    = CLAY_DRAW_TOOL (tool);
      ClayDisplayShell *shell        = clay_display_get_shell (display);
      ClaySamplePoint  *sample_point = NULL;

      clay_draw_tool_pause (draw_tool);

      if (! draw_tool->widget &&

          clay_draw_tool_is_active (draw_tool) &&
          (draw_tool->display != display ||
           ! proximity))
        {
          clay_draw_tool_stop (draw_tool);
        }

      if (clay_display_shell_get_show_sample_points (shell) &&
          proximity)
        {
          ClayImage *image         = clay_display_get_image (display);
          gint       snap_distance = display->config->snap_distance;

          sample_point =
            clay_image_pick_sample_point (image,
                                          coords->x, coords->y,
                                          FUNSCALEX (shell, snap_distance),
                                          FUNSCALEY (shell, snap_distance));
        }

      color_tool->sample_point = sample_point;

      color_tool->can_pick = clay_color_tool_can_pick (color_tool,
                                                       coords, display);
      color_tool->center_x = coords->x;
      color_tool->center_y = coords->y;

      if (! draw_tool->widget &&

          ! clay_draw_tool_is_active (draw_tool) &&
          proximity)
        {
          clay_draw_tool_start (draw_tool, display);
        }

      clay_draw_tool_resume (draw_tool);
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
clay_color_tool_cursor_update (ClayTool         *tool,
                               const ClayCoords *coords,
                               GdkModifierType   state,
                               ClayDisplay      *display)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          clay_tool_set_cursor (tool, display,
                                CLAY_CURSOR_MOUSE,
                                CLAY_TOOL_CURSOR_COLOR_PICKER,
                                CLAY_CURSOR_MODIFIER_MOVE);
        }
      else
        {
          ClayCursorModifier modifier = CLAY_CURSOR_MODIFIER_BAD;

          if (clay_color_tool_can_pick (color_tool, coords, display))
            {
              switch (color_tool->pick_target)
                {
                case CLAY_COLOR_PICK_TARGET_NONE:
                  modifier = CLAY_CURSOR_MODIFIER_NONE;
                  break;
                case CLAY_COLOR_PICK_TARGET_FOREGROUND:
                  modifier = CLAY_CURSOR_MODIFIER_FOREGROUND;
                  break;
                case CLAY_COLOR_PICK_TARGET_BACKGROUND:
                  modifier = CLAY_CURSOR_MODIFIER_BACKGROUND;
                  break;
                case CLAY_COLOR_PICK_TARGET_PALETTE:
                  modifier = CLAY_CURSOR_MODIFIER_PLUS;
                  break;
                }
            }

          clay_tool_set_cursor (tool, display,
                                CLAY_CURSOR_COLOR_PICKER,
                                CLAY_TOOL_CURSOR_COLOR_PICKER,
                                modifier);
        }
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
clay_color_tool_draw (ClayDrawTool *draw_tool)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (draw_tool);

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  if (color_tool->enabled)
    {
      if (color_tool->sample_point)
        {
          ClayImage      *image = clay_display_get_image (draw_tool->display);
          ClayCanvasItem *item;
          gint            index;
          gint            x;
          gint            y;

          clay_sample_point_get_position (color_tool->sample_point, &x, &y);

          index = g_list_index (clay_image_get_sample_points (image),
                                color_tool->sample_point) + 1;

          item = clay_draw_tool_add_sample_point (draw_tool, x, y, index);
          clay_canvas_item_set_highlight (item, TRUE);
        }
      else if (color_tool->can_pick && color_tool->options->sample_average)
        {
          gdouble radius = color_tool->options->average_radius;

          clay_draw_tool_add_rectangle (draw_tool,
                                        FALSE,
                                        color_tool->center_x - radius,
                                        color_tool->center_y - radius,
                                        2 * radius + 1,
                                        2 * radius + 1);
        }
    }
}

static gboolean
clay_color_tool_real_can_pick (ClayColorTool    *color_tool,
                               const ClayCoords *coords,
                               ClayDisplay      *display)
{
  ClayDisplayShell *shell = clay_display_get_shell (display);
  ClayImage        *image = clay_display_get_image (display);

  return clay_image_coords_in_active_pickable (image, coords,
                                               shell->show_all,
                                               color_tool->options->sample_merged,
                                               FALSE);
}

static gboolean
clay_color_tool_real_pick (ClayColorTool     *color_tool,
                           const ClayCoords  *coords,
                           ClayDisplay       *display,
                           const Babl       **sample_format,
                           gpointer           pixel,
                           GeglColor        **color)
{
  ClayDisplayShell *shell     = clay_display_get_shell (display);
  ClayImage        *image     = clay_display_get_image (display);
  GList            *drawables = clay_image_get_selected_drawables (image);

  g_return_val_if_fail (drawables != NULL, FALSE);
  g_return_val_if_fail (color != NULL && GEGL_IS_COLOR (*color), FALSE);

  return clay_image_pick_color (image, drawables,
                                coords->x, coords->y,
                                shell->show_all,
                                color_tool->options->sample_merged,
                                color_tool->options->sample_average,
                                color_tool->options->average_radius,
                                sample_format,
                                pixel,
                                color);
}

static void
clay_color_tool_real_picked (ClayColorTool      *color_tool,
                             const ClayCoords   *coords,
                             ClayDisplay        *display,
                             ClayColorPickState  pick_state,
                             const Babl         *sample_format,
                             gpointer            pixel,
                             GeglColor          *color)
{
  ClayTool          *tool  = CLAY_TOOL (color_tool);
  ClayDisplayShell  *shell = clay_display_get_shell (display);
  ClayImageWindow   *image_window;
  ClayDialogFactory *dialog_factory;
  ClayContext       *context;

  g_return_if_fail (GEGL_IS_COLOR (color));

  image_window   = clay_display_shell_get_window (shell);
  dialog_factory = clay_dock_container_get_dialog_factory (CLAY_DOCK_CONTAINER (image_window));

  /*  use this tool's own options here (NOT color_tool->options)  */
  context = CLAY_CONTEXT (clay_tool_get_options (tool));

  if (color_tool->pick_target == CLAY_COLOR_PICK_TARGET_FOREGROUND ||
      color_tool->pick_target == CLAY_COLOR_PICK_TARGET_BACKGROUND)
    {
      GtkWidget *widget;

      widget = clay_dialog_factory_find_widget (dialog_factory,
                                                "clay-indexed-palette");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          ClayImage *image  = clay_display_get_image (display);

          if (babl_format_is_palette (sample_format))
            {
              guchar *index  = pixel;

              clay_colormap_editor_set_index (CLAY_COLORMAP_EDITOR (editor),
                                              *index, NULL);
            }
          else if (clay_image_get_base_type (image) == CLAY_INDEXED)
            {
              /* When Sample merged is set, we don't have the index
               * information and it is possible to pick colors out of
               * the colormap (with compositing). In such a case, the
               * sample format will not be a palette format even though
               * the image is indexed. Still search if the color exists
               * in the colormap.
               * Note that even if it does, we might still pick the
               * wrong color, since several indexes may contain the same
               * color and we can't know for sure which is the right
               * one.
               */
              gint index = clay_colormap_editor_get_index (CLAY_COLORMAP_EDITOR (editor),
                                                           color);
              if (index > -1)
                clay_colormap_editor_set_index (CLAY_COLORMAP_EDITOR (editor),
                                                index, NULL);
            }
        }

      widget = clay_dialog_factory_find_widget (dialog_factory,
                                                "clay-palette-editor");
      if (widget)
        {
          GtkWidget *editor = gtk_bin_get_child (GTK_BIN (widget));
          gint       index;

          index = clay_palette_editor_get_index (CLAY_PALETTE_EDITOR (editor),
                                                 color);
          if (index != -1)
            clay_palette_editor_set_index (CLAY_PALETTE_EDITOR (editor),
                                           index, NULL);
        }
    }

  switch (color_tool->pick_target)
    {
    case CLAY_COLOR_PICK_TARGET_NONE:
      break;

    case CLAY_COLOR_PICK_TARGET_FOREGROUND:
      clay_context_set_foreground (context, color);
      break;

    case CLAY_COLOR_PICK_TARGET_BACKGROUND:
      clay_context_set_background (context, color);
      break;

    case CLAY_COLOR_PICK_TARGET_PALETTE:
      {
        GdkMonitor *monitor = clay_widget_get_monitor (GTK_WIDGET (shell));
        GtkWidget  *dockable;

        dockable =
          clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (display->clay)),
                                                     display->clay,
                                                     dialog_factory,
                                                     monitor,
                                                     "clay-palette-editor");

        if (dockable)
          {
            GtkWidget *palette_editor;
            ClayData  *data;

            /* don't blink like mad when updating */
            if (pick_state != CLAY_COLOR_PICK_STATE_START)
              clay_widget_blink_cancel (dockable);

            palette_editor = gtk_bin_get_child (GTK_BIN (dockable));

            data = clay_data_editor_get_data (CLAY_DATA_EDITOR (palette_editor));

            if (! data)
              {
                data = CLAY_DATA (clay_context_get_palette (context));

                clay_data_editor_set_data (CLAY_DATA_EDITOR (palette_editor),
                                           data);
              }

            clay_palette_editor_pick_color (CLAY_PALETTE_EDITOR (palette_editor),
                                            color, pick_state);
          }
      }
      break;
    }
}

static gboolean
clay_color_tool_can_pick (ClayColorTool    *tool,
                          const ClayCoords *coords,
                          ClayDisplay      *display)
{
  ClayColorToolClass *klass;

  klass = CLAY_COLOR_TOOL_GET_CLASS (tool);

  return klass->can_pick && klass->can_pick (tool, coords, display);
}

static void
clay_color_tool_pick (ClayColorTool      *tool,
                      const ClayCoords   *coords,
                      ClayDisplay        *display,
                      ClayColorPickState  pick_state)
{
  ClayColorToolClass *klass;
  const Babl         *sample_format;
  gdouble             pixel[4];
  GeglColor          *color;

  klass = CLAY_COLOR_TOOL_GET_CLASS (tool);
  color = gegl_color_new ("black");

  if (klass->pick &&
      klass->pick (tool, coords, display, &sample_format, pixel, &color))
    g_signal_emit (tool, clay_color_tool_signals[PICKED], 0,
                   coords, display, pick_state,
                   sample_format, pixel, color);

  g_object_unref (color);
}


/*  public functions  */

void
clay_color_tool_enable (ClayColorTool    *color_tool,
                        ClayColorOptions *options)
{
  ClayTool *tool;

  g_return_if_fail (CLAY_IS_COLOR_TOOL (color_tool));
  g_return_if_fail (CLAY_IS_COLOR_OPTIONS (options));

  tool = CLAY_TOOL (color_tool);

  if (clay_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to enable ClayColorTool while it is active.");
      return;
    }

  g_set_object (&color_tool->options, options);

  /*  color picking doesn't snap, see bug #768058  */
  color_tool->saved_snap_to = clay_tool_control_get_snap_to (tool->control);
  clay_tool_control_set_snap_to (tool->control, FALSE);

  color_tool->enabled = TRUE;
}

void
clay_color_tool_disable (ClayColorTool *color_tool)
{
  ClayTool *tool;

  g_return_if_fail (CLAY_IS_COLOR_TOOL (color_tool));

  tool = CLAY_TOOL (color_tool);

  if (clay_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to disable ClayColorTool while it is active.");
      return;
    }

  g_clear_object (&color_tool->options);

  clay_tool_control_set_snap_to (tool->control, color_tool->saved_snap_to);
  color_tool->saved_snap_to = FALSE;

  color_tool->enabled = FALSE;
}

gboolean
clay_color_tool_is_enabled (ClayColorTool *color_tool)
{
  return color_tool->enabled;
}
