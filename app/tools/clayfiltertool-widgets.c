/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfiltertool-widgets.c
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

#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "core/claycontainer.h"
#include "core/claydrawablefilter.h"
#include "core/clayitem.h"

#include "display/claydisplay.h"
#include "display/claytoolfocus.h"
#include "display/claytoolgyroscope.h"
#include "display/claytoolline.h"
#include "display/claytooltransformgrid.h"
#include "display/claytoolwidgetgroup.h"

#include "clayfilteroptions.h"
#include "clayfiltertool.h"
#include "clayfiltertool-widgets.h"


typedef struct _Controller Controller;

struct _Controller
{
  ClayFilterTool     *filter_tool;
  ClayControllerType  controller_type;
  ClayToolWidget     *widget;
  GCallback           creator_callback;
  gpointer            creator_data;
};


/*  local function prototypes  */

static Controller * clay_filter_tool_controller_new          (void);
static void         clay_filter_tool_controller_free         (Controller                 *controller);

static void         clay_filter_tool_set_line                (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     x1,
                                                              gdouble                     y1,
                                                              gdouble                     x2,
                                                              gdouble                     y2);
static void         clay_filter_tool_line_changed            (ClayToolWidget             *widget,
                                                              Controller                 *controller);

static void         clay_filter_tool_set_slider_line         (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     x1,
                                                              gdouble                     y1,
                                                              gdouble                     x2,
                                                              gdouble                     y2,
                                                              const ClayControllerSlider *sliders,
                                                              gint                        n_sliders);
static void         clay_filter_tool_slider_line_changed     (ClayToolWidget             *widget,
                                                              Controller                 *controller);

static void         clay_filter_tool_set_transform_grid      (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              const ClayMatrix3          *transform);
static void         clay_filter_tool_transform_grid_changed  (ClayToolWidget             *widget,
                                                              Controller                 *controller);

static void         clay_filter_tool_set_transform_grids     (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              const ClayMatrix3          *transforms,
                                                              gint                        n_transforms);
static void         clay_filter_tool_transform_grids_changed (ClayToolWidget             *widget,
                                                              Controller                 *controller);

static void         clay_filter_tool_set_gyroscope           (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              gdouble                     yaw,
                                                              gdouble                     pitch,
                                                              gdouble                     roll,
                                                              gdouble                     zoom,
                                                              gboolean                    invert);
static void         clay_filter_tool_gyroscope_changed       (ClayToolWidget             *widget,
                                                              Controller                 *controller);

static void         clay_filter_tool_set_focus               (Controller                 *controller,
                                                              GeglRectangle              *area,
                                                              ClayLimitType               type,
                                                              gdouble                     x,
                                                              gdouble                     y,
                                                              gdouble                     radius,
                                                              gdouble                     aspect_ratio,
                                                              gdouble                     angle,
                                                              gdouble                     inner_limit,
                                                              gdouble                     midpoint);
static void         clay_filter_tool_focus_changed           (ClayToolWidget             *widget,
                                                              Controller                 *controller);


/*  public functions  */

ClayToolWidget *
clay_filter_tool_create_widget (ClayFilterTool     *filter_tool,
                                ClayControllerType  controller_type,
                                const gchar        *status_title,
                                GCallback           callback,
                                gpointer            callback_data,
                                GCallback          *set_func,
                                gpointer           *set_func_data)
{
  ClayTool         *tool;
  ClayDisplayShell *shell;
  Controller       *controller;

  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (filter_tool->config != NULL, NULL);

  tool = CLAY_TOOL (filter_tool);

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = clay_display_get_shell (tool->display);

  controller = clay_filter_tool_controller_new ();

  controller->filter_tool      = filter_tool;
  controller->controller_type  = controller_type;
  controller->creator_callback = callback;
  controller->creator_data     = callback_data;

  switch (controller_type)
    {
    case CLAY_CONTROLLER_TYPE_LINE:
      controller->widget = clay_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (clay_filter_tool_line_changed),
                        controller);

      *set_func      = (GCallback) clay_filter_tool_set_line;
      *set_func_data = controller;
      break;

    case CLAY_CONTROLLER_TYPE_SLIDER_LINE:
      controller->widget = clay_tool_line_new (shell, 100, 100, 500, 500);

      g_object_set (controller->widget,
                    "status-title", status_title,
                    NULL);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (clay_filter_tool_slider_line_changed),
                        controller);

      *set_func      = (GCallback) clay_filter_tool_set_slider_line;
      *set_func_data = controller;
      break;

    case CLAY_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        ClayMatrix3   transform;
        gint          off_x, off_y;
        GeglRectangle area;
        gdouble       x1, y1;
        gdouble       x2, y2;

        clay_matrix3_identity (&transform);

        clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        x1 = off_x + area.x;
        y1 = off_y + area.y;
        x2 = x1 + area.width;
        y2 = y1 + area.height;

        controller->widget = clay_tool_transform_grid_new (shell, &transform,
                                                           x1, y1, x2, y2);

        g_object_set (controller->widget,
                      "pivot-x",                 (x1 + x2) / 2.0,
                      "pivot-y",                 (y1 + y2) / 2.0,
                      "inside-function",         CLAY_TRANSFORM_FUNCTION_MOVE,
                      "outside-function",        CLAY_TRANSFORM_FUNCTION_ROTATE,
                      "use-corner-handles",      TRUE,
                      "use-perspective-handles", TRUE,
                      "use-side-handles",        TRUE,
                      "use-shear-handles",       TRUE,
                      "use-pivot-handle",        TRUE,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (clay_filter_tool_transform_grid_changed),
                          controller);

        *set_func      = (GCallback) clay_filter_tool_set_transform_grid;
        *set_func_data = controller;
      }
      break;

    case CLAY_CONTROLLER_TYPE_TRANSFORM_GRIDS:
      {
        controller->widget = clay_tool_widget_group_new (shell);

        clay_tool_widget_group_set_auto_raise (
          CLAY_TOOL_WIDGET_GROUP (controller->widget), TRUE);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (clay_filter_tool_transform_grids_changed),
                          controller);

        *set_func      = (GCallback) clay_filter_tool_set_transform_grids;
        *set_func_data = controller;
      }
      break;

    case CLAY_CONTROLLER_TYPE_GYROSCOPE:
      {
        GeglRectangle area;
        gint          off_x, off_y;

        clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

        controller->widget = clay_tool_gyroscope_new (shell);

        g_object_set (controller->widget,
                      "speed",   1.0 / MAX (area.width, area.height),
                      "pivot-x", off_x + area.x + area.width  / 2.0,
                      "pivot-y", off_y + area.y + area.height / 2.0,
                      NULL);

        g_signal_connect (controller->widget, "changed",
                          G_CALLBACK (clay_filter_tool_gyroscope_changed),
                          controller);

        *set_func      = (GCallback) clay_filter_tool_set_gyroscope;
        *set_func_data = controller;
      }
      break;

    case CLAY_CONTROLLER_TYPE_FOCUS:
      controller->widget = clay_tool_focus_new (shell);

      g_signal_connect (controller->widget, "changed",
                        G_CALLBACK (clay_filter_tool_focus_changed),
                        controller);

      *set_func      = (GCallback) clay_filter_tool_set_focus;
      *set_func_data = controller;
      break;
    }

  g_object_add_weak_pointer (G_OBJECT (controller->widget),
                            (gpointer) &controller->widget);

  g_object_set_data_full (filter_tool->config,
                          "clay-filter-tool-controller", controller,
                          (GDestroyNotify) clay_filter_tool_controller_free);

  return controller->widget;
}

static void
clay_filter_tool_reset_transform_grid (ClayToolWidget *widget,
                                       ClayFilterTool *filter_tool)
{
  ClayMatrix3   *transform;
  gint           off_x, off_y;
  GeglRectangle  area;
  gdouble        x1, y1;
  gdouble        x2, y2;
  gdouble        pivot_x, pivot_y;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 = off_x + area.x;
  y1 = off_y + area.y;
  x2 = x1 + area.width;
  y2 = y1 + area.height;

  clay_matrix3_transform_point (transform,
                                (x1 + x2) / 2.0, (y1 + y2) / 2.0,
                                &pivot_x, &pivot_y);

  g_object_set (widget,
                "pivot-x", pivot_x,
                "pivot-y", pivot_y,
                NULL);

  g_free (transform);
}

void
clay_filter_tool_reset_widget (ClayFilterTool *filter_tool,
                               ClayToolWidget *widget)
{
  Controller *controller;

  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (CLAY_IS_TOOL_WIDGET (widget));
  g_return_if_fail (filter_tool->config != NULL);

  controller = g_object_get_data (filter_tool->config,
                                  "clay-filter-tool-controller");

  g_return_if_fail (controller != NULL);

  switch (controller->controller_type)
    {
    case CLAY_CONTROLLER_TYPE_TRANSFORM_GRID:
      {
        g_signal_handlers_block_by_func (controller->widget,
                                         clay_filter_tool_transform_grid_changed,
                                         controller);

        clay_filter_tool_reset_transform_grid (controller->widget, filter_tool);

        g_signal_handlers_unblock_by_func (controller->widget,
                                           clay_filter_tool_transform_grid_changed,
                                           controller);
      }
      break;

    case CLAY_CONTROLLER_TYPE_TRANSFORM_GRIDS:
      {
        g_signal_handlers_block_by_func (controller->widget,
                                         clay_filter_tool_transform_grids_changed,
                                         controller);

        clay_container_foreach (
          clay_tool_widget_group_get_children (
            CLAY_TOOL_WIDGET_GROUP (controller->widget)),
          (GFunc) clay_filter_tool_reset_transform_grid,
          filter_tool);

        g_signal_handlers_unblock_by_func (controller->widget,
                                           clay_filter_tool_transform_grids_changed,
                                           controller);
      }
      break;

    default:
      break;
    }
}


/*  private functions  */

static Controller *
clay_filter_tool_controller_new (void)
{
  return g_slice_new0 (Controller);
}

static void
clay_filter_tool_controller_free (Controller *controller)
{
  if (controller->widget)
    {
      g_signal_handlers_disconnect_by_data (controller->widget, controller);

      g_clear_weak_pointer (&controller->widget);
    }

  g_slice_free (Controller, controller);
}

static void
clay_filter_tool_set_line (Controller    *controller,
                           GeglRectangle *area,
                           gdouble        x1,
                           gdouble        y1,
                           gdouble        x2,
                           gdouble        y2)
{
  ClayTool       *tool;
  ClayFilterTool *filter_tool;
  ClayDrawable   *drawable;

  if (! controller->widget)
    return;

  filter_tool = controller->filter_tool;
  tool        = CLAY_TOOL (filter_tool);
  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_line_changed,
                                     controller);
}

static void
clay_filter_tool_line_changed (ClayToolWidget *widget,
                               Controller     *controller)
{
  ClayFilterTool             *filter_tool = controller->filter_tool;
  ClayControllerLineCallback  line_callback;
  gdouble                     x1, y1, x2, y2;
  gint                        off_x, off_y;
  GeglRectangle               area;

  line_callback = (ClayControllerLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  line_callback (controller->creator_data,
                 &area, x1, y1, x2, y2);
}

static void
clay_filter_tool_set_slider_line (Controller                 *controller,
                                  GeglRectangle              *area,
                                  gdouble                     x1,
                                  gdouble                     y1,
                                  gdouble                     x2,
                                  gdouble                     y2,
                                  const ClayControllerSlider *sliders,
                                  gint                        n_sliders)
{
  ClayTool       *tool;
  ClayFilterTool *filter_tool;
  ClayDrawable   *drawable;

  if (! controller->widget)
    return;

  filter_tool = controller->filter_tool;
  tool        = CLAY_TOOL (filter_tool);
  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      x1 += off_x + area->x;
      y1 += off_y + area->y;
      x2 += off_x + area->x;
      y2 += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_slider_line_changed,
                                   controller);

  g_object_set (controller->widget,
                "x1", x1,
                "y1", y1,
                "x2", x2,
                "y2", y2,
                NULL);

  clay_tool_line_set_sliders (CLAY_TOOL_LINE (controller->widget),
                              sliders, n_sliders);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_slider_line_changed,
                                     controller);
}

static void
clay_filter_tool_slider_line_changed (ClayToolWidget *widget,
                                      Controller     *controller)
{
  ClayFilterTool                   *filter_tool = controller->filter_tool;
  ClayControllerSliderLineCallback  slider_line_callback;
  gdouble                           x1, y1, x2, y2;
  const ClayControllerSlider       *sliders;
  gint                              n_sliders;
  gint                              off_x, off_y;
  GeglRectangle                     area;

  slider_line_callback =
    (ClayControllerSliderLineCallback) controller->creator_callback;

  g_object_get (widget,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  sliders = clay_tool_line_get_sliders (CLAY_TOOL_LINE (controller->widget),
                                        &n_sliders);

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x1 -= off_x + area.x;
  y1 -= off_y + area.y;
  x2 -= off_x + area.x;
  y2 -= off_y + area.y;

  slider_line_callback (controller->creator_data,
                        &area, x1, y1, x2, y2, sliders, n_sliders);
}

static void
clay_filter_tool_set_transform_grid (Controller        *controller,
                                     GeglRectangle     *area,
                                     const ClayMatrix3 *transform)
{
  ClayTool       *tool;
  ClayFilterTool *filter_tool;
  ClayDrawable   *drawable;
  gdouble         x1 = area->x;
  gdouble         y1 = area->y;
  gdouble         x2 = area->x + area->width;
  gdouble         y2 = area->y + area->height;
  ClayMatrix3     matrix;

  if (! controller->widget)
    return;

  filter_tool = controller->filter_tool;
  tool        = CLAY_TOOL (filter_tool);
  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      x1 += off_x;
      y1 += off_y;
      x2 += off_x;
      y2 += off_y;
    }

  clay_matrix3_identity (&matrix);
  clay_matrix3_translate (&matrix, -x1, -y1);
  clay_matrix3_mult (transform, &matrix);
  clay_matrix3_translate (&matrix, +x1, +y1);

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_transform_grid_changed,
                                   controller);

  g_object_set (controller->widget,
                "transform", &matrix,
                "x1",        x1,
                "y1",        y1,
                "x2",        x2,
                "y2",        y2,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_transform_grid_changed,
                                     controller);
}

static void
clay_filter_tool_transform_grid_changed (ClayToolWidget *widget,
                                         Controller     *controller)
{
  ClayFilterTool                      *filter_tool = controller->filter_tool;
  ClayControllerTransformGridCallback  transform_grid_callback;
  gint                                 off_x, off_y;
  GeglRectangle                        area;
  ClayMatrix3                         *transform;
  ClayMatrix3                          matrix;

  transform_grid_callback =
    (ClayControllerTransformGridCallback) controller->creator_callback;

  g_object_get (widget,
                "transform", &transform,
                NULL);

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  clay_matrix3_identity (&matrix);
  clay_matrix3_translate (&matrix, +(off_x + area.x), +(off_y + area.y));
  clay_matrix3_mult (transform, &matrix);
  clay_matrix3_translate (&matrix, -(off_x + area.x), -(off_y + area.y));

  transform_grid_callback (controller->creator_data,
                           &area, &matrix);

  g_free (transform);
}

static void
clay_filter_tool_set_transform_grids (Controller        *controller,
                                      GeglRectangle     *area,
                                      const ClayMatrix3 *transforms,
                                      gint               n_transforms)
{
  ClayTool         *tool;
  ClayFilterTool   *filter_tool;
  ClayDisplayShell *shell;
  ClayDrawable     *drawable;
  ClayContainer    *grids;
  ClayToolWidget   *grid = NULL;
  gdouble           x1   = area->x;
  gdouble           y1   = area->y;
  gdouble           x2   = area->x + area->width;
  gdouble           y2   = area->y + area->height;
  ClayMatrix3       matrix;
  gint              i;

  if (! controller->widget)
    return;

  filter_tool = controller->filter_tool;
  tool        = CLAY_TOOL (filter_tool);
  shell       = clay_display_get_shell (tool->display);
  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_transform_grids_changed,
                                   controller);

  if (drawable)
    {
      gint off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      x1 += off_x;
      y1 += off_y;
      x2 += off_x;
      y2 += off_y;
    }

  grids = clay_tool_widget_group_get_children (
    CLAY_TOOL_WIDGET_GROUP (controller->widget));

  if (n_transforms > clay_container_get_n_children (grids))
    {
      clay_matrix3_identity (&matrix);

      for (i = clay_container_get_n_children (grids); i < n_transforms; i++)
        {
          gdouble pivot_x;
          gdouble pivot_y;

          grid = clay_tool_transform_grid_new (shell, &matrix, x1, y1, x2, y2);

          if (i > 0 && ! memcmp (&transforms[i], &transforms[i - 1],
                                 sizeof (ClayMatrix3)))
            {
              g_object_get (clay_container_get_last_child (grids),
                            "pivot-x", &pivot_x,
                            "pivot-y", &pivot_y,
                            NULL);
            }
          else
            {
              pivot_x = (x1 + x2) / 2.0;
              pivot_y = (y1 + y2) / 2.0;

              clay_matrix3_transform_point (&transforms[i],
                                            pivot_x, pivot_y,
                                            &pivot_x, &pivot_y);
            }

          g_object_set (grid,
                        "pivot-x",                 pivot_x,
                        "pivot-y",                 pivot_y,
                        "inside-function",         CLAY_TRANSFORM_FUNCTION_MOVE,
                        "outside-function",        CLAY_TRANSFORM_FUNCTION_ROTATE,
                        "use-corner-handles",      TRUE,
                        "use-perspective-handles", TRUE,
                        "use-side-handles",        TRUE,
                        "use-shear-handles",       TRUE,
                        "use-pivot-handle",        TRUE,
                        NULL);

          clay_container_add (grids, CLAY_OBJECT (grid));

          g_object_unref (grid);
        }

      clay_tool_widget_set_focus (grid, TRUE);
    }
  else
    {
      while (clay_container_get_n_children (grids) > n_transforms)
        clay_container_remove (grids, clay_container_get_last_child (grids));
    }

  for (i = 0; i < n_transforms; i++)
    {
      clay_matrix3_identity (&matrix);
      clay_matrix3_translate (&matrix, -x1, -y1);
      clay_matrix3_mult (&transforms[i], &matrix);
      clay_matrix3_translate (&matrix, +x1, +y1);

      g_object_set (clay_container_get_child_by_index (grids, i),
                    "transform", &matrix,
                    "x1",        x1,
                    "y1",        y1,
                    "x2",        x2,
                    "y2",        y2,
                    NULL);
    }

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_transform_grids_changed,
                                     controller);
}

static void
clay_filter_tool_transform_grids_changed (ClayToolWidget *widget,
                                          Controller     *controller)
{
  ClayFilterTool                       *filter_tool = controller->filter_tool;
  ClayControllerTransformGridsCallback  transform_grids_callback;
  ClayContainer                        *grids;
  gint                                  off_x, off_y;
  GeglRectangle                         area;
  ClayMatrix3                          *transforms;
  gint                                  n_transforms;
  gint                                  i;

  transform_grids_callback =
    (ClayControllerTransformGridsCallback) controller->creator_callback;

  grids = clay_tool_widget_group_get_children (
    CLAY_TOOL_WIDGET_GROUP (controller->widget));

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  n_transforms = clay_container_get_n_children (grids);
  transforms   = g_new (ClayMatrix3, n_transforms);

  for (i = 0; i < n_transforms; i++)
    {
      ClayMatrix3 *transform;

      g_object_get (clay_container_get_child_by_index (grids, i),
                    "transform", &transform,
                    NULL);

      clay_matrix3_identity (&transforms[i]);
      clay_matrix3_translate (&transforms[i],
                              +(off_x + area.x), +(off_y + area.y));
      clay_matrix3_mult (transform, &transforms[i]);
      clay_matrix3_translate (&transforms[i],
                              -(off_x + area.x), -(off_y + area.y));

      g_free (transform);
    }

  transform_grids_callback (controller->creator_data,
                            &area, transforms, n_transforms);

  g_free (transforms);
}

static void
clay_filter_tool_set_gyroscope (Controller    *controller,
                                GeglRectangle *area,
                                gdouble        yaw,
                                gdouble        pitch,
                                gdouble        roll,
                                gdouble        zoom,
                                gboolean       invert)
{
  if (! controller->widget)
    return;

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_gyroscope_changed,
                                   controller);

  g_object_set (controller->widget,
                "yaw",    yaw,
                "pitch",  pitch,
                "roll",   roll,
                "zoom",   zoom,
                "invert", invert,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_gyroscope_changed,
                                     controller);
}

static void
clay_filter_tool_gyroscope_changed (ClayToolWidget *widget,
                                    Controller     *controller)
{
  ClayFilterTool                  *filter_tool = controller->filter_tool;
  ClayControllerGyroscopeCallback  gyroscope_callback;
  gint                             off_x, off_y;
  GeglRectangle                    area;
  gdouble                          yaw;
  gdouble                          pitch;
  gdouble                          roll;
  gdouble                          zoom;
  gboolean                         invert;

  gyroscope_callback =
    (ClayControllerGyroscopeCallback) controller->creator_callback;

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  g_object_get (widget,
                "yaw",    &yaw,
                "pitch",  &pitch,
                "roll",   &roll,
                "zoom",   &zoom,
                "invert", &invert,
                NULL);

  gyroscope_callback (controller->creator_data,
                      &area, yaw, pitch, roll, zoom, invert);
}

static void
clay_filter_tool_set_focus (Controller    *controller,
                            GeglRectangle *area,
                            ClayLimitType  type,
                            gdouble        x,
                            gdouble        y,
                            gdouble        radius,
                            gdouble        aspect_ratio,
                            gdouble        angle,
                            gdouble        inner_limit,
                            gdouble        midpoint)
{
  ClayTool       *tool;
  ClayFilterTool *filter_tool;
  ClayDrawable   *drawable;

  if (! controller->widget)
    return;

  filter_tool = controller->filter_tool;
  tool        = CLAY_TOOL (filter_tool);
  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (drawable)
    {
      gint off_x, off_y;

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      x += off_x + area->x;
      y += off_y + area->y;
    }

  g_signal_handlers_block_by_func (controller->widget,
                                   clay_filter_tool_focus_changed,
                                   controller);

  g_object_set (controller->widget,
                "type",         type,
                "x",            x,
                "y",            y,
                "radius",       radius,
                "aspect-ratio", aspect_ratio,
                "angle",        angle,
                "inner-limit",  inner_limit,
                "midpoint",     midpoint,
                NULL);

  g_signal_handlers_unblock_by_func (controller->widget,
                                     clay_filter_tool_focus_changed,
                                     controller);
}

static void
clay_filter_tool_focus_changed (ClayToolWidget *widget,
                                Controller     *controller)
{
  ClayFilterTool              *filter_tool = controller->filter_tool;
  ClayControllerFocusCallback  focus_callback;
  ClayLimitType                type;
  gdouble                      x,  y;
  gdouble                      radius;
  gdouble                      aspect_ratio;
  gdouble                      angle;
  gdouble                      inner_limit;
  gdouble                      midpoint;
  gint                         off_x, off_y;
  GeglRectangle                area;

  focus_callback = (ClayControllerFocusCallback) controller->creator_callback;

  g_object_get (widget,
                "type",         &type,
                "x",            &x,
                "y",            &y,
                "radius",       &radius,
                "aspect-ratio", &aspect_ratio,
                "angle",        &angle,
                "inner-limit",  &inner_limit,
                "midpoint",     &midpoint,
                NULL);

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  x -= off_x + area.x;
  y -= off_y + area.y;

  focus_callback (controller->creator_data,
                  &area,
                  type,
                  x,
                  y,
                  radius,
                  aspect_ratio,
                  angle,
                  inner_limit,
                  midpoint);
}
