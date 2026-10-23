/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvascanvasboundary.c
 * Copyright (C) 2019 Ell
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

#include "core/clayimage.h"

#include "claycanvas-style.h"
#include "claycanvascanvasboundary.h"
#include "claydisplayshell.h"


enum
{
  PROP_0,
  PROP_IMAGE
};


typedef struct _ClayCanvasCanvasBoundaryPrivate ClayCanvasCanvasBoundaryPrivate;

struct _ClayCanvasCanvasBoundaryPrivate
{
  ClayImage *image;
};

#define GET_PRIVATE(canvas_boundary) \
        ((ClayCanvasCanvasBoundaryPrivate *) clay_canvas_canvas_boundary_get_instance_private ((ClayCanvasCanvasBoundary *) (canvas_boundary)))


/*  local function prototypes  */

static void             clay_canvas_canvas_boundary_set_property (GObject        *object,
                                                                  guint           property_id,
                                                                  const GValue   *value,
                                                                  GParamSpec     *pspec);
static void             clay_canvas_canvas_boundary_get_property (GObject        *object,
                                                                  guint           property_id,
                                                                  GValue         *value,
                                                                  GParamSpec     *pspec);
static void             clay_canvas_canvas_boundary_finalize     (GObject        *object);
static void             clay_canvas_canvas_boundary_draw         (ClayCanvasItem *item,
                                                                  cairo_t        *cr);
static cairo_region_t * clay_canvas_canvas_boundary_get_extents  (ClayCanvasItem *item);
static void             clay_canvas_canvas_boundary_stroke       (ClayCanvasItem *item,
                                                                  cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasCanvasBoundary, clay_canvas_canvas_boundary,
                            CLAY_TYPE_CANVAS_RECTANGLE)

#define parent_class clay_canvas_canvas_boundary_parent_class


static void
clay_canvas_canvas_boundary_class_init (ClayCanvasCanvasBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayCanvasItemClass *item_class   = CLAY_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = clay_canvas_canvas_boundary_set_property;
  object_class->get_property = clay_canvas_canvas_boundary_get_property;
  object_class->finalize     = clay_canvas_canvas_boundary_finalize;

  item_class->draw           = clay_canvas_canvas_boundary_draw;
  item_class->get_extents    = clay_canvas_canvas_boundary_get_extents;
  item_class->stroke         = clay_canvas_canvas_boundary_stroke;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_canvas_canvas_boundary_init (ClayCanvasCanvasBoundary *canvas_boundary)
{
}

static void
clay_canvas_canvas_boundary_finalize (GObject *object)
{
  ClayCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  g_clear_weak_pointer (&private->image);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_canvas_canvas_boundary_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  ClayCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_set_weak_pointer (&private->image, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_canvas_boundary_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ClayCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_canvas_boundary_draw (ClayCanvasItem *item,
                                  cairo_t        *cr)
{
  ClayCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->image)
    CLAY_CANVAS_ITEM_CLASS (parent_class)->draw (item, cr);
}

static cairo_region_t *
clay_canvas_canvas_boundary_get_extents (ClayCanvasItem *item)
{
  ClayCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->image)
    return CLAY_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

  return NULL;
}

static void
clay_canvas_canvas_boundary_stroke (ClayCanvasItem *item,
                                    cairo_t        *cr)
{
  ClayDisplayShell *shell = clay_canvas_item_get_shell (item);

  clay_canvas_set_canvas_style (clay_canvas_item_get_canvas (item), cr,
                                shell->offset_x, shell->offset_y);
  cairo_stroke (cr);
}

ClayCanvasItem *
clay_canvas_canvas_boundary_new (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_CANVAS_BOUNDARY,
                       "shell", shell,
                       NULL);
}

void
clay_canvas_canvas_boundary_set_image (ClayCanvasCanvasBoundary *boundary,
                                       ClayImage                *image)
{
  ClayCanvasCanvasBoundaryPrivate *private;

  g_return_if_fail (CLAY_IS_CANVAS_CANVAS_BOUNDARY (boundary));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  private = GET_PRIVATE (boundary);

  if (image != private->image)
    {
      clay_canvas_item_begin_change (CLAY_CANVAS_ITEM (boundary));

      if (image)
        {
          g_object_set (boundary,
                        "x",      (gdouble) 0,
                        "y",      (gdouble) 0,
                        "width",  (gdouble) clay_image_get_width  (image),
                        "height", (gdouble) clay_image_get_height (image),
                        NULL);
        }

      g_object_set (boundary,
                    "image", image,
                    NULL);

      clay_canvas_item_end_change (CLAY_CANVAS_ITEM (boundary));
    }
  else if (image && image == private->image)
    {
      gint    lx, ly, lw, lh;
      gdouble x, y, w ,h;

      lx = 0;
      ly = 0;
      lw = clay_image_get_width  (image);
      lh = clay_image_get_height (image);

      g_object_get (boundary,
                    "x",      &x,
                    "y",      &y,
                    "width",  &w,
                    "height", &h,
                    NULL);

      if (lx != (gint) x ||
          ly != (gint) y ||
          lw != (gint) w ||
          lh != (gint) h)
        {
          clay_canvas_item_begin_change (CLAY_CANVAS_ITEM (boundary));

          g_object_set (boundary,
                        "x",      (gdouble) lx,
                        "y",      (gdouble) ly,
                        "width",  (gdouble) lw,
                        "height", (gdouble) lh,
                        NULL);

          clay_canvas_item_end_change (CLAY_CANVAS_ITEM (boundary));
        }
    }
}
