/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasguide.c
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

#include "claycanvas-style.h"
#include "claycanvasguide.h"
#include "claydisplayshell.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_STYLE
};


typedef struct _ClayCanvasGuidePrivate ClayCanvasGuidePrivate;

struct _ClayCanvasGuidePrivate
{
  ClayOrientationType  orientation;
  gint                 position;

  ClayGuideStyle       style;
};

#define GET_PRIVATE(guide) \
        ((ClayCanvasGuidePrivate *) clay_canvas_guide_get_instance_private ((ClayCanvasGuide *) (guide)))


/*  local function prototypes  */

static void             clay_canvas_guide_set_property (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void             clay_canvas_guide_get_property (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);
static void             clay_canvas_guide_draw         (ClayCanvasItem *item,
                                                        cairo_t        *cr);
static cairo_region_t * clay_canvas_guide_get_extents  (ClayCanvasItem *item);
static void             clay_canvas_guide_stroke       (ClayCanvasItem *item,
                                                        cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (ClayCanvasGuide, clay_canvas_guide,
                            CLAY_TYPE_CANVAS_ITEM)

#define parent_class clay_canvas_guide_parent_class


static void
clay_canvas_guide_class_init (ClayCanvasGuideClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  ClayCanvasItemClass *item_class   = CLAY_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = clay_canvas_guide_set_property;
  object_class->get_property = clay_canvas_guide_get_property;

  item_class->draw           = clay_canvas_guide_draw;
  item_class->get_extents    = clay_canvas_guide_get_extents;
  item_class->stroke         = clay_canvas_guide_stroke;

  g_object_class_install_property (object_class, PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation", NULL, NULL,
                                                      CLAY_TYPE_ORIENTATION_TYPE,
                                                      CLAY_ORIENTATION_HORIZONTAL,
                                                      CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_int ("position", NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE, 0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_STYLE,
                                   g_param_spec_enum ("style", NULL, NULL,
                                                      CLAY_TYPE_GUIDE_STYLE,
                                                      CLAY_GUIDE_STYLE_NONE,
                                                      CLAY_PARAM_READWRITE));
}

static void
clay_canvas_guide_init (ClayCanvasGuide *guide)
{
}

static void
clay_canvas_guide_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      private->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      private->position = g_value_get_int (value);
      break;
    case PROP_STYLE:
      private->style = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_guide_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayCanvasGuidePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, private->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, private->position);
      break;
    case PROP_STYLE:
      g_value_set_enum (value, private->style);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_canvas_guide_transform (ClayCanvasItem *item,
                             gdouble        *x1,
                             gdouble        *y1,
                             gdouble        *x2,
                             gdouble        *y2)
{
  ClayCanvasGuidePrivate *private = GET_PRIVATE (item);
  GtkWidget              *canvas  = clay_canvas_item_get_canvas (item);
  GtkAllocation           allocation;
  gint                    max_outside;
  gint                    x, y;

  gtk_widget_get_allocation (canvas, &allocation);

  max_outside = allocation.width + allocation.height;

  *x1 = -max_outside;
  *y1 = -max_outside;
  *x2 = allocation.width  + max_outside;
  *y2 = allocation.height + max_outside;

  switch (private->orientation)
    {
    case CLAY_ORIENTATION_HORIZONTAL:
      clay_canvas_item_transform_xy (item, 0, private->position, &x, &y);
      *y1 = *y2 = y + 0.5;
      break;

    case CLAY_ORIENTATION_VERTICAL:
      clay_canvas_item_transform_xy (item, private->position, 0, &x, &y);
      *x1 = *x2 = x + 0.5;
      break;

    case CLAY_ORIENTATION_UNKNOWN:
      return;
    }
}

static void
clay_canvas_guide_draw (ClayCanvasItem *item,
                        cairo_t        *cr)
{
  gdouble x1, y1;
  gdouble x2, y2;

  clay_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  cairo_move_to (cr, x1, y1);
  cairo_line_to (cr, x2, y2);

  _clay_canvas_item_stroke (item, cr);
}

static cairo_region_t *
clay_canvas_guide_get_extents (ClayCanvasItem *item)
{
  cairo_rectangle_int_t rectangle;
  gdouble               x1, y1;
  gdouble               x2, y2;

  clay_canvas_guide_transform (item, &x1, &y1, &x2, &y2);

  rectangle.x      = MIN (x1, x2) - 1.5;
  rectangle.y      = MIN (y1, y2) - 1.5;
  rectangle.width  = ABS (x2 - x1) + 3.0;
  rectangle.height = ABS (y2 - y1) + 3.0;

  return cairo_region_create_rectangle (&rectangle);
}

static void
clay_canvas_guide_stroke (ClayCanvasItem *item,
                          cairo_t        *cr)
{
  ClayCanvasGuidePrivate *private = GET_PRIVATE (item);

  if (private->style != CLAY_GUIDE_STYLE_NONE)
    {
      ClayDisplayShell *shell = clay_canvas_item_get_shell (item);

      clay_canvas_set_guide_style (clay_canvas_item_get_canvas (item), cr,
                                   private->style,
                                   clay_canvas_item_get_highlight (item),
                                   shell->offset_x, shell->offset_y);
      cairo_stroke (cr);
    }
  else
    {
      CLAY_CANVAS_ITEM_CLASS (parent_class)->stroke (item, cr);
    }
}

ClayCanvasItem *
clay_canvas_guide_new (ClayDisplayShell    *shell,
                       ClayOrientationType  orientation,
                       gint                 position,
                       ClayGuideStyle       style)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (CLAY_TYPE_CANVAS_GUIDE,
                       "shell",        shell,
                       "orientation",  orientation,
                       "position",     position,
                       "style",        style,
                       NULL);
}

void
clay_canvas_guide_set (ClayCanvasItem      *guide,
                       ClayOrientationType  orientation,
                       gint                 position)
{
  g_return_if_fail (CLAY_IS_CANVAS_GUIDE (guide));

  clay_canvas_item_begin_change (guide);

  g_object_set (guide,
                "orientation", orientation,
                "position",    position,
                NULL);

  clay_canvas_item_end_change (guide);
}
