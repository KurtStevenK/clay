/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolrotategrid.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "display-types.h"

#include "core/clay-transform-utils.h"
#include "core/clay-utils.h"

#include "claydisplayshell.h"
#include "claytoolrotategrid.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_ANGLE
};


struct _ClayToolRotateGridPrivate
{
  gdouble  angle;

  gboolean rotate_grab;
  gdouble  real_angle;
  gdouble  last_x;
  gdouble  last_y;
};


/*  local function prototypes  */

static void   clay_tool_rotate_grid_set_property (GObject             *object,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
static void   clay_tool_rotate_grid_get_property (GObject             *object,
                                                  guint                property_id,
                                                  GValue              *value,
                                                  GParamSpec          *pspec);

static gint   clay_tool_rotate_grid_button_press (ClayToolWidget      *widget,
                                                  const ClayCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  ClayButtonPressType  press_type);
static void   clay_tool_rotate_grid_motion       (ClayToolWidget      *widget,
                                                  const ClayCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolRotateGrid, clay_tool_rotate_grid,
                            CLAY_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class clay_tool_rotate_grid_parent_class


static void
clay_tool_rotate_grid_class_init (ClayToolRotateGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->set_property    = clay_tool_rotate_grid_set_property;
  object_class->get_property    = clay_tool_rotate_grid_get_property;

  widget_class->button_press    = clay_tool_rotate_grid_button_press;
  widget_class->motion          = clay_tool_rotate_grid_motion;

  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("angle",
                                                        NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_tool_rotate_grid_init (ClayToolRotateGrid *grid)
{
  grid->private = clay_tool_rotate_grid_get_instance_private (grid);
}

static void
clay_tool_rotate_grid_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayToolRotateGrid        *grid    = CLAY_TOOL_ROTATE_GRID (object);
  ClayToolRotateGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ANGLE:
      private->angle = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_rotate_grid_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayToolRotateGrid        *grid    = CLAY_TOOL_ROTATE_GRID (object);
  ClayToolRotateGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ANGLE:
      g_value_set_double (value, private->angle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
clay_tool_rotate_grid_button_press (ClayToolWidget      *widget,
                                    const ClayCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    ClayButtonPressType  press_type)
{
  ClayToolRotateGrid        *grid    = CLAY_TOOL_ROTATE_GRID (widget);
  ClayToolRotateGridPrivate *private = grid->private;
  ClayTransformHandle        handle;

  handle = CLAY_TOOL_WIDGET_CLASS (parent_class)->button_press (widget,
                                                                coords, time,
                                                                state,
                                                                press_type);

  if (handle == CLAY_TRANSFORM_HANDLE_ROTATION)
    {
      private->rotate_grab = TRUE;
      private->real_angle  = private->angle;
      private->last_x      = coords->x;
      private->last_y      = coords->y;
    }
  else
    {
      private->rotate_grab = FALSE;
    }

  return handle;
}

void
clay_tool_rotate_grid_motion (ClayToolWidget   *widget,
                              const ClayCoords *coords,
                              guint32           time,
                              GdkModifierType   state)
{
  ClayToolRotateGrid        *grid    = CLAY_TOOL_ROTATE_GRID (widget);
  ClayToolRotateGridPrivate *private = grid->private;
  gdouble                    angle1, angle2, angle;
  gdouble                    pivot_x, pivot_y;
  gdouble                    x1, y1, x2, y2;
  gboolean                   constrain;
  ClayMatrix3                transform;

  if (! private->rotate_grab)
    {
      gdouble old_pivot_x;
      gdouble old_pivot_y;

      g_object_get (widget,
                    "pivot-x", &old_pivot_x,
                    "pivot-y", &old_pivot_y,
                    NULL);

      g_object_freeze_notify (G_OBJECT (widget));

      CLAY_TOOL_WIDGET_CLASS (parent_class)->motion (widget,
                                                     coords, time, state);

      g_object_get (widget,
                    "pivot-x", &pivot_x,
                    "pivot-y", &pivot_y,
                    NULL);

      if (old_pivot_x != pivot_x ||
          old_pivot_y != pivot_y)
        {
          clay_matrix3_identity (&transform);
          clay_transform_matrix_rotate_center (&transform,
                                               pivot_x, pivot_y,
                                               private->angle);

          g_object_set (widget,
                        "transform", &transform,
                        NULL);
       }

      g_object_thaw_notify (G_OBJECT (widget));

      return;
    }

  g_object_get (widget,
                "pivot-x",          &pivot_x,
                "pivot-y",          &pivot_y,
                "constrain-rotate", &constrain,
                NULL);

  x1 = coords->x       - pivot_x;
  x2 = private->last_x - pivot_x;
  y1 = pivot_y - coords->y;
  y2 = pivot_y - private->last_y;

  /*  find the first angle  */
  angle1 = atan2 (y1, x1);

  /*  find the angle  */
  angle2 = atan2 (y2, x2);

  angle = angle2 - angle1;

  if (angle > G_PI || angle < -G_PI)
    angle = angle2 - ((angle1 < 0) ? 2.0 * G_PI + angle1 : angle1 - 2.0 * G_PI);

  /*  increment the transform tool's angle  */
  private->real_angle += angle;

  /*  limit the angle to between -180 and 180 degrees  */
  if (private->real_angle < - G_PI)
    {
      private->real_angle += 2.0 * G_PI;
    }
  else if (private->real_angle > G_PI)
    {
      private->real_angle -= 2.0 * G_PI;
    }

  /*  constrain the angle to 15-degree multiples if ctrl is held down  */
#define FIFTEEN_DEG (G_PI / 12.0)

  if (constrain)
    {
      angle = FIFTEEN_DEG * (gint) ((private->real_angle +
                                     FIFTEEN_DEG / 2.0) / FIFTEEN_DEG);
    }
  else
    {
      angle = private->real_angle;
    }

  clay_matrix3_identity (&transform);
  clay_transform_matrix_rotate_center (&transform, pivot_x, pivot_y, angle);

  g_object_set (widget,
                "transform", &transform,
                "angle",     angle,
                NULL);

  private->last_x = coords->x;
  private->last_y = coords->y;
}


/*  public functions  */

ClayToolWidget *
clay_tool_rotate_grid_new (ClayDisplayShell *shell,
                           gdouble           x1,
                           gdouble           y1,
                           gdouble           x2,
                           gdouble           y2,
                           gdouble           pivot_x,
                           gdouble           pivot_y,
                           gdouble           angle)
{
  ClayMatrix3 transform;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  clay_matrix3_identity (&transform);
  clay_transform_matrix_rotate_center (&transform, pivot_x, pivot_y, angle);

  return g_object_new (CLAY_TYPE_TOOL_ROTATE_GRID,
                       "shell",     shell,
                       "transform", &transform,
                       "x1",        x1,
                       "y1",        y1,
                       "x2",        x2,
                       "y2",        y2,
                       "pivot-x",   pivot_x,
                       "pivot-y",   pivot_y,
                       "angle",     angle,
                       NULL);
}
