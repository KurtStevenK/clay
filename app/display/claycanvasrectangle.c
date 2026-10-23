/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasrectangle.c
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

#include "display-types.h"

#include "claycanvasrectangle.h"
#include "claydisplayshell.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_FILLED
};


typedef struct _ClayCanvasRectanglePrivate ClayCanvasRectanglePrivate;

struct _ClayCanvasRectanglePrivate
{
  gdouble  x;
  gdouble  y;
  gdouble  width;
  gdouble  height;
  gboolean filled;
};

#define GET_PRIVATE(rectangle) \
        ((ClayCanvasRectanglePrivate *) clay_canvas_rectangle_get_instance_private ((ClayCanvasRectangle *) (rectangle)))


/*  local function prototypes  */

static void             clay_canvas_rectangle_set_property (GObject        *object,
                                                            guint           property_id,
                                                            const GValue   *value,
                                                            GParamSpec     *pspec);
static void             clay_canvas_rectangle_get_property (GObject        *object,
                                                            guint           property_id,
                                                            GValue         *value,
                                                            GParamSpec     *pspec);
static void             clay_canvas_rectangle_draw         (ClayCanvasItem *item,
                                                            cairo_t        *cr);
static cairo_region_t * clay_canvas_rectangle_get_extents  (ClayCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasRectangle, clay_canvas_rectangle,
                            CLAY_TYPE_CANVAS_ITEM)

#define parent_class clay_canvas_rectangle_parent_class


static void
clay_canvas_rectangle_class_init (ClayCanvasRectangleClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayCanvasItemClass *item_class   = CLAY_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = clay_canvas_rectangle_set_property;
  object_class->get_property = clay_canvas_rectangle_get_property;

  item_class->draw           = clay_canvas_rectangle_draw;
  item_class->get_extents    = clay_canvas_rectangle_get_extents;

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE, 0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE, 0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_double ("width", NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE, 0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_double ("height", NULL, NULL,
                                                        -CLAY_MAX_IMAGE_SIZE,
                                                        CLAY_MAX_IMAGE_SIZE, 0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILLED,
                                   g_param_spec_boolean ("filled", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_canvas_rectangle_init (ClayCanvasRectangle *rectangle)
{
}

static void
clay_canvas_rectangle_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayCanvasRectanglePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;
    case PROP_FILLED:
      private->filled = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_rectangle_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayCanvasRectanglePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;
    case PROP_FILLED:
      g_value_set_boolean (value, private->filled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_rectangle_transform (ClayCanvasItem *item,
                                 gdouble        *x,
                                 gdouble        *y,
                                 gdouble        *w,
                                 gdouble        *h)
{
  ClayCanvasRectanglePrivate *private = GET_PRIVATE (item);
  gdouble                     x1, y1;
  gdouble                     x2, y2;

  clay_canvas_item_transform_xy_f (item,
                                   MIN (private->x,
                                        private->x + private->width),
                                   MIN (private->y,
                                        private->y + private->height),
                                   &x1, &y1);
  clay_canvas_item_transform_xy_f (item,
                                   MAX (private->x,
                                        private->x + private->width),
                                   MAX (private->y,
                                        private->y + private->height),
                                   &x2, &y2);

  x1 = floor (x1);
  y1 = floor (y1);
  x2 = ceil (x2);
  y2 = ceil (y2);

  if (private->filled)
    {
      *x = x1;
      *y = y1;
      *w = x2 - x1;
      *h = y2 - y1;
    }
  else
    {
      *x = x1 + 0.5;
      *y = y1 + 0.5;
      *w = x2 - 0.5 - *x;
      *h = y2 - 0.5 - *y;

      *w = MAX (0.0, *w);
      *h = MAX (0.0, *h);
    }
}

static void
clay_canvas_rectangle_draw (ClayCanvasItem *item,
                            cairo_t        *cr)
{
  ClayCanvasRectanglePrivate *private = GET_PRIVATE (item);
  gdouble                     x, y;
  gdouble                     w, h;

  clay_canvas_rectangle_transform (item, &x, &y, &w, &h);

  cairo_rectangle (cr, x, y, w, h);

  if (private->filled)
    _clay_canvas_item_fill (item, cr);
  else
    _clay_canvas_item_stroke (item, cr);
}

static cairo_region_t *
clay_canvas_rectangle_get_extents (ClayCanvasItem *item)
{
  ClayCanvasRectanglePrivate *private = GET_PRIVATE (item);
  cairo_rectangle_int_t       rectangle;
  gdouble                     x, y;
  gdouble                     w, h;

  clay_canvas_rectangle_transform (item, &x, &y, &w, &h);

  if (private->filled)
    {
      rectangle.x      = floor (x - 1.0);
      rectangle.y      = floor (y - 1.0);
      rectangle.width  = ceil (w + 2.0);
      rectangle.height = ceil (h + 2.0);

      return cairo_region_create_rectangle (&rectangle);
    }
  else if (w > 64 && h > 64)
    {
      cairo_region_t *region;

      /* left */
      rectangle.x      = floor (x - 1.5);
      rectangle.y      = floor (y - 1.5);
      rectangle.width  = 3.0;
      rectangle.height = ceil (h + 3.0);

      region = cairo_region_create_rectangle (&rectangle);

      /* right */
      rectangle.x      = floor (x + w - 1.5);

      cairo_region_union_rectangle (region, &rectangle);

      /* top */
      rectangle.x      = floor (x - 1.5);
      rectangle.y      = floor (y - 1.5);
      rectangle.width  = ceil (w + 3.0);
      rectangle.height = 3.0;

      cairo_region_union_rectangle (region, &rectangle);

      /* bottom */
      rectangle.y      = floor (y + h - 1.5);

      cairo_region_union_rectangle (region, &rectangle);

      return region;
    }
  else
    {
      rectangle.x      = floor (x - 1.5);
      rectangle.y      = floor (y - 1.5);
      rectangle.width  = ceil (w + 3.0);
      rectangle.height = ceil (h + 3.0);

      return cairo_region_create_rectangle (&rectangle);
    }
}

ClayCanvasItem *
clay_canvas_rectangle_new (ClayDisplayShell *shell,
                           gdouble           x,
                           gdouble           y,
                           gdouble           width,
                           gdouble           height,
                           gboolean          filled)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_RECTANGLE,
                       "shell",  shell,
                       "x",      x,
                       "y",      y,
                       "width",  width,
                       "height", height,
                       "filled", filled,
                       NULL);
}

void
clay_canvas_rectangle_set (ClayCanvasItem *rectangle,
                           gdouble         x,
                           gdouble         y,
                           gdouble         width,
                           gdouble         height)
{
  g_return_if_fail (CLAY_IS_CANVAS_RECTANGLE (rectangle));

  clay_canvas_item_begin_change (rectangle);

  g_object_set (rectangle,
                "x",      x,
                "y",      y,
                "width",  width,
                "height", height,
                NULL);

  clay_canvas_item_end_change (rectangle);
}
