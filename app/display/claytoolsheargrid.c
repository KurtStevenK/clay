/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolsheargrid.c
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
#include "claytoolsheargrid.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_SHEAR_X,
  PROP_SHEAR_Y
};


struct _ClayToolShearGridPrivate
{
  ClayOrientationType orientation;
  gdouble             shear_x;
  gdouble             shear_y;

  gdouble             last_x;
  gdouble             last_y;
};


/*  local function prototypes  */

static void     clay_tool_shear_grid_set_property (GObject             *object,
                                                   guint                property_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void     clay_tool_shear_grid_get_property (GObject             *object,
                                                   guint                property_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

static gint     clay_tool_shear_grid_button_press (ClayToolWidget      *widget,
                                                   const ClayCoords    *coords,
                                                   guint32              time,
                                                   GdkModifierType      state,
                                                   ClayButtonPressType  press_type);
static void     clay_tool_shear_grid_motion        (ClayToolWidget      *widget,
                                                    const ClayCoords    *coords,
                                                    guint32              time,
                                                    GdkModifierType      state);


G_DEFINE_TYPE_WITH_PRIVATE (ClayToolShearGrid, clay_tool_shear_grid,
                            CLAY_TYPE_TOOL_TRANSFORM_GRID)

#define parent_class clay_tool_shear_grid_parent_class


static void
clay_tool_shear_grid_class_init (ClayToolShearGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayToolWidgetClass *widget_class = CLAY_TOOL_WIDGET_CLASS (klass);

  object_class->set_property    = clay_tool_shear_grid_set_property;
  object_class->get_property    = clay_tool_shear_grid_get_property;

  widget_class->button_press    = clay_tool_shear_grid_button_press;
  widget_class->motion          = clay_tool_shear_grid_motion;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      NULL, NULL,
                                                      CLAY_TYPE_ORIENTATION_TYPE,
                                                      CLAY_ORIENTATION_UNKNOWN,
                                                      CLAY_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHEAR_X,
                                   g_param_spec_double ("shear-x",
                                                        NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SHEAR_Y,
                                   g_param_spec_double ("shear-y",
                                                        NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_tool_shear_grid_init (ClayToolShearGrid *grid)
{
  grid->private = clay_tool_shear_grid_get_instance_private (grid);

  g_object_set (grid,
                "inside-function",         CLAY_TRANSFORM_FUNCTION_SHEAR,
                "outside-function",        CLAY_TRANSFORM_FUNCTION_SHEAR,
                "use-corner-handles",      FALSE,
                "use-perspective-handles", FALSE,
                "use-side-handles",        FALSE,
                "use-shear-handles",       FALSE,
                "use-center-handle",       FALSE,
                "use-pivot-handle",        FALSE,
                NULL);
}

static void
clay_tool_shear_grid_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayToolShearGrid        *grid    = CLAY_TOOL_SHEAR_GRID (object);
  ClayToolShearGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_SHEAR_X:
      private->shear_x = g_value_get_double (value);
      break;
    case PROP_SHEAR_Y:
      private->shear_y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tool_shear_grid_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayToolShearGrid        *grid    = CLAY_TOOL_SHEAR_GRID (object);
  ClayToolShearGridPrivate *private = grid->private;

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_SHEAR_X:
      g_value_set_double (value, private->shear_x);
      break;
    case PROP_SHEAR_Y:
      g_value_set_double (value, private->shear_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint
clay_tool_shear_grid_button_press (ClayToolWidget      *widget,
                                   const ClayCoords    *coords,
                                   guint32              time,
                                   GdkModifierType      state,
                                   ClayButtonPressType  press_type)
{
  ClayToolShearGrid        *grid    = CLAY_TOOL_SHEAR_GRID (widget);
  ClayToolShearGridPrivate *private = grid->private;

  private->last_x = coords->x;
  private->last_y = coords->y;

  return 1;
}

void
clay_tool_shear_grid_motion (ClayToolWidget   *widget,
                             const ClayCoords *coords,
                             guint32           time,
                             GdkModifierType   state)
{
  ClayToolShearGrid        *grid    = CLAY_TOOL_SHEAR_GRID (widget);
  ClayToolShearGridPrivate *private = grid->private;
  gdouble                   diffx   = coords->x - private->last_x;
  gdouble                   diffy   = coords->y - private->last_y;
  gdouble                   amount  = 0.0;
  ClayMatrix3               transform;
  ClayMatrix3              *t;
  gdouble                   x1, y1;
  gdouble                   x2, y2;
  gdouble                   tx1, ty1;
  gdouble                   tx2, ty2;
  gdouble                   tx3, ty3;
  gdouble                   tx4, ty4;
  gdouble                   current_x;
  gdouble                   current_y;

  g_object_get (widget,
                "transform", &t,
                "x1",        &x1,
                "y1",        &y1,
                "x2",        &x2,
                "y2",        &y2,
                NULL);

  clay_matrix3_transform_point (t, x1, y1, &tx1, &ty1);
  clay_matrix3_transform_point (t, x2, y1, &tx2, &ty2);
  clay_matrix3_transform_point (t, x1, y2, &tx3, &ty3);
  clay_matrix3_transform_point (t, x2, y2, &tx4, &ty4);

  g_free (t);

  current_x = coords->x;
  current_y = coords->y;

  diffx = current_x - private->last_x;
  diffy = current_y - private->last_y;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */
  if (private->orientation == CLAY_ORIENTATION_UNKNOWN)
    {
#define MIN_MOVE 5

      if (ABS (diffx) > MIN_MOVE || ABS (diffy) > MIN_MOVE)
        {
          if (ABS (diffx) > ABS (diffy))
            {
              private->orientation = CLAY_ORIENTATION_HORIZONTAL;
              private->shear_x     = 0.0;
            }
          else
            {
              private->orientation = CLAY_ORIENTATION_VERTICAL;
              private->shear_y     = 0.0;
            }
        }
      /*  set the current coords to the last ones  */
      else
        {
          current_x = private->last_x;
          current_y = private->last_y;
        }
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (private->orientation == CLAY_ORIENTATION_HORIZONTAL)
    {
      if (current_y > (ty1 + ty3) / 2)
        private->shear_x += diffx;
      else
        private->shear_x -= diffx;

      amount = private->shear_x;
    }
  else if (private->orientation == CLAY_ORIENTATION_VERTICAL)
    {
      if (current_x > (tx1 + tx2) / 2)
        private->shear_y += diffy;
      else
        private->shear_y -= diffy;

      amount = private->shear_y;
    }

  clay_matrix3_identity (&transform);
  clay_transform_matrix_shear (&transform,
                               x1, y1, x2 - x1, y2 - y1,
                               private->orientation, amount);

  g_object_set (widget,
                "transform",   &transform,
                "orientation", private->orientation,
                "shear-x",     private->shear_x,
                "shear_y",     private->shear_y,
                NULL);

  private->last_x = current_x;
  private->last_y = current_y;
}


/*  public functions  */

ClayToolWidget *
clay_tool_shear_grid_new (ClayDisplayShell    *shell,
                          gdouble              x1,
                          gdouble              y1,
                          gdouble              x2,
                          gdouble              y2,
                          ClayOrientationType  orientation,
                          gdouble              shear_x,
                          gdouble              shear_y)
{
  ClayMatrix3 transform;
  gdouble     amount;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  if (orientation == CLAY_ORIENTATION_HORIZONTAL)
    amount = shear_x;
  else
    amount = shear_y;

  clay_matrix3_identity (&transform);
  clay_transform_matrix_shear (&transform,
                               x1, y1, x2 - x1, y2 - y1,
                               orientation, amount);

  return g_object_new (CLAY_TYPE_TOOL_SHEAR_GRID,
                       "shell",       shell,
                       "transform",   &transform,
                       "x1",          x1,
                       "y1",          y1,
                       "x2",          x2,
                       "y2",          y2,
                       "orientation", orientation,
                       "shear-x",     shear_x,
                       "shear-y",     shear_y,
                       NULL);
}
