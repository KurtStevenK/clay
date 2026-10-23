/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasgrid.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "display-types.h"

#include "core/claygrid.h"
#include "core/clayimage.h"

#include "claycanvas-style.h"
#include "claycanvasgrid.h"
#include "claycanvasitem-utils.h"
#include "claydisplayshell.h"
#include "claydisplayshell-scale.h"


enum
{
  PROP_0,
  PROP_GRID,
  PROP_GRID_STYLE
};


typedef struct _ClayCanvasGridPrivate ClayCanvasGridPrivate;

struct _ClayCanvasGridPrivate
{
  ClayGrid *grid;
  gboolean  grid_style;
};

#define GET_PRIVATE(grid) \
        ((ClayCanvasGridPrivate *) clay_canvas_grid_get_instance_private ((ClayCanvasGrid *) (grid)))


/*  local function prototypes  */

static void             clay_canvas_grid_finalize     (GObject        *object);
static void             clay_canvas_grid_set_property (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void             clay_canvas_grid_get_property (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);
static void             clay_canvas_grid_draw         (ClayCanvasItem *item,
                                                       cairo_t        *cr);
static cairo_region_t * clay_canvas_grid_get_extents  (ClayCanvasItem *item);
static void             clay_canvas_grid_stroke       (ClayCanvasItem *item,
                                                       cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasGrid, clay_canvas_grid,
                            CLAY_TYPE_CANVAS_ITEM)

#define parent_class clay_canvas_grid_parent_class


static void
clay_canvas_grid_class_init (ClayCanvasGridClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayCanvasItemClass *item_class   = CLAY_CANVAS_ITEM_CLASS (klass);

  object_class->finalize     = clay_canvas_grid_finalize;
  object_class->set_property = clay_canvas_grid_set_property;
  object_class->get_property = clay_canvas_grid_get_property;

  item_class->draw           = clay_canvas_grid_draw;
  item_class->get_extents    = clay_canvas_grid_get_extents;
  item_class->stroke         = clay_canvas_grid_stroke;

  g_object_class_install_property (object_class, PROP_GRID,
                                   g_param_spec_object ("grid", NULL, NULL,
                                                        CLAY_TYPE_GRID,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_GRID_STYLE,
                                   g_param_spec_boolean ("grid-style",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_canvas_grid_init (ClayCanvasGrid *grid)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (grid);

  private->grid = g_object_new (CLAY_TYPE_GRID, NULL);
}

static void
clay_canvas_grid_finalize (GObject *object)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->grid);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_canvas_grid_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GRID:
      {
        ClayGrid *grid = g_value_get_object (value);
        if (grid)
          clay_config_sync (G_OBJECT (grid), G_OBJECT (private->grid), 0);
      }
      break;
    case PROP_GRID_STYLE:
      private->grid_style = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_grid_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GRID:
      g_value_set_object (value, private->grid);
      break;
    case PROP_GRID_STYLE:
      g_value_set_boolean (value, private->grid_style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_grid_draw (ClayCanvasItem *item,
                       cairo_t        *cr)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (item);
  ClayDisplayShell      *shell   = clay_canvas_item_get_shell (item);
  gdouble                xspacing, yspacing;
  gdouble                xoffset, yoffset;
  gboolean               vert, horz;
  gdouble                dx1, dy1, dx2, dy2;
  gint                   x1, y1, x2, y2;
  gdouble                dx, dy;
  gint                   x, y;

#define CROSSHAIR 2

  clay_grid_get_spacing (private->grid, &xspacing, &yspacing);
  clay_grid_get_offset  (private->grid, &xoffset,  &yoffset);

  g_return_if_fail (xspacing >= 0.0 &&
                    yspacing >= 0.0);

  xspacing *= shell->scale_x;
  yspacing *= shell->scale_y;

  xoffset  *= shell->scale_x;
  yoffset  *= shell->scale_y;

  /*  skip grid drawing when the space between grid lines starts
   *  disappearing, see bug #599267.
   */
  vert = (xspacing >= 2.0);
  horz = (yspacing >= 2.0);

  if (! vert && ! horz)
    return;

  cairo_clip_extents (cr, &dx1, &dy1, &dx2, &dy2);

  x1 = floor (dx1) - 1;
  y1 = floor (dy1) - 1;
  x2 = ceil  (dx2) + 1;
  y2 = ceil  (dy2) + 1;

  if (! clay_display_shell_get_infinite_canvas (shell))
    {
      GeglRectangle bounds;

      clay_display_shell_scale_get_image_unrotated_bounds (
        shell,
        &bounds.x, &bounds.y, &bounds.width, &bounds.height);

      if (! gegl_rectangle_intersect (&bounds,
                                      &bounds,
                                      GEGL_RECTANGLE (x1,      y1,
                                                      x2 - x1, y2 - y1)))
        {
          return;
        }

      x1 = bounds.x;
      y1 = bounds.y;
      x2 = bounds.x + bounds.width;
      y2 = bounds.y + bounds.height;
    }

  switch (clay_grid_get_style (private->grid))
    {
    case CLAY_GRID_INTERSECTIONS:
      x1 -= CROSSHAIR;
      y1 -= CROSSHAIR;
      x2 += CROSSHAIR;
      y2 += CROSSHAIR;
      break;

    case CLAY_GRID_DOTS:
    case CLAY_GRID_ON_OFF_DASH:
    case CLAY_GRID_DOUBLE_DASH:
    case CLAY_GRID_SOLID:
      break;
    }

  xoffset = fmod (xoffset - shell->offset_x - x1, xspacing);
  yoffset = fmod (yoffset - shell->offset_y - y1, yspacing);

  if (xoffset < 0.0)
    xoffset += xspacing;

  if (yoffset < 0.0)
    yoffset += yspacing;

  switch (clay_grid_get_style (private->grid))
    {
    case CLAY_GRID_DOTS:
      if (vert && horz)
        {
          for (dx = x1 + xoffset; dx <= x2; dx += xspacing)
            {
              x = RINT (dx);

              for (dy = y1 + yoffset; dy <= y2; dy += yspacing)
                {
                  y = RINT (dy);

                  cairo_move_to (cr, x,       y + 0.5);
                  cairo_line_to (cr, x + 1.0, y + 0.5);
                }
            }
        }
      break;

    case CLAY_GRID_INTERSECTIONS:
      if (vert && horz)
        {
          for (dx = x1 + xoffset; dx <= x2; dx += xspacing)
            {
              x = RINT (dx);

              for (dy = y1 + yoffset; dy <= y2; dy += yspacing)
                {
                  y = RINT (dy);

                  cairo_move_to (cr, x + 0.5, y - CROSSHAIR);
                  cairo_line_to (cr, x + 0.5, y + CROSSHAIR + 1.0);

                  cairo_move_to (cr, x - CROSSHAIR, y + 0.5);
                  cairo_line_to (cr, x + CROSSHAIR + 1.0, y + 0.5);
                }
            }
        }
      break;

    case CLAY_GRID_ON_OFF_DASH:
    case CLAY_GRID_DOUBLE_DASH:
    case CLAY_GRID_SOLID:
      if (vert)
        {
          for (dx = x1 + xoffset; dx < x2; dx += xspacing)
            {
              x = RINT (dx);
              
              cairo_move_to (cr, x + 0.5, y1);
              cairo_line_to (cr, x + 0.5, y2);
            }
        }

      if (horz)
        {
          for (dy = y1 + yoffset; dy < y2; dy += yspacing)
            {
              y = RINT (dy);
              
              cairo_move_to (cr, x1, y + 0.5);
              cairo_line_to (cr, x2, y + 0.5);
            }
        }
      break;
    }

  _clay_canvas_item_stroke (item, cr);
}

static cairo_region_t *
clay_canvas_grid_get_extents (ClayCanvasItem *item)
{
  ClayDisplayShell      *shell = clay_canvas_item_get_shell (item);
  ClayImage             *image = clay_canvas_item_get_image (item);
  cairo_rectangle_int_t  rectangle;

  if (! image)
    return NULL;

  if (! clay_display_shell_get_infinite_canvas (shell))
    {
      
      gdouble x1, y1;
      gdouble x2, y2;
      gint    w, h;

      w = clay_image_get_width  (image);
      h = clay_image_get_height (image);

      clay_canvas_item_transform_xy_f (item, 0, 0, &x1, &y1);
      clay_canvas_item_transform_xy_f (item, w, h, &x2, &y2);

      rectangle.x      = floor (x1);
      rectangle.y      = floor (y1);
      rectangle.width  = ceil (x2) - rectangle.x;
      rectangle.height = ceil (y2) - rectangle.y;
    }
  else
    {
      clay_canvas_item_untransform_viewport (item,
                                             &rectangle.x,
                                             &rectangle.y,
                                             &rectangle.width,
                                             &rectangle.height);
    }

  return cairo_region_create_rectangle (&rectangle);
}

static void
clay_canvas_grid_stroke (ClayCanvasItem *item,
                         cairo_t        *cr)
{
  ClayCanvasGridPrivate *private = GET_PRIVATE (item);

  if (private->grid_style)
    {
      ClayDisplayShell *shell = clay_canvas_item_get_shell (item);

      clay_canvas_set_grid_style (clay_canvas_item_get_canvas (item), cr,
                                  private->grid,
                                  shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
  else
    {
      CLAY_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

ClayCanvasItem *
clay_canvas_grid_new (ClayDisplayShell *shell,
                      ClayGrid         *grid)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);
  g_return_val_if_fail (grid == NULL || CLAY_IS_GRID (grid), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_GRID,
                       "shell", shell,
                       "grid",  grid,
                       NULL);
}
