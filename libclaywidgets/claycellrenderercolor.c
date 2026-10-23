/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycellrenderercolor.c
 * Copyright (C) 2004,2007  Sven Neuman <sven1@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claycairo-utils.h"
#include "claycellrenderercolor.h"


/**
 * SECTION: claycellrenderercolor
 * @title: ClayCellRendererColor
 * @short_description: A #GtkCellRenderer to display a #GeglColor color.
 *
 * A #GtkCellRenderer to display a #GeglColor color.
 **/


#define DEFAULT_ICON_SIZE  GTK_ICON_SIZE_MENU


enum
{
  PROP_0,
  PROP_COLOR,
  PROP_OPAQUE,
  PROP_SIZE
};


struct _ClayCellRendererColor
{
  GtkCellRenderer  parent_instance;

  GeglColor       *color;
  gboolean         opaque;
  GtkIconSize      size;
  gint             border;
};


static void clay_cell_renderer_color_finalize     (GObject            *object);
static void clay_cell_renderer_color_get_property (GObject            *object,
                                                   guint               param_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static void clay_cell_renderer_color_set_property (GObject            *object,
                                                   guint               param_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void clay_cell_renderer_color_get_size     (GtkCellRenderer    *cell,
                                                   GtkWidget          *widget,
                                                   const GdkRectangle *rectangle,
                                                   gint               *x_offset,
                                                   gint               *y_offset,
                                                   gint               *width,
                                                   gint               *height);
static void clay_cell_renderer_color_render       (GtkCellRenderer    *cell,
                                                   cairo_t            *cr,
                                                   GtkWidget          *widget,
                                                   const GdkRectangle *background_area,
                                                   const GdkRectangle *cell_area,
                                                   GtkCellRendererState flags);



G_DEFINE_TYPE (ClayCellRendererColor, clay_cell_renderer_color, GTK_TYPE_CELL_RENDERER)

#define parent_class clay_cell_renderer_color_parent_class


static void
clay_cell_renderer_color_class_init (ClayCellRendererColorClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->finalize     = clay_cell_renderer_color_finalize;
  object_class->get_property = clay_cell_renderer_color_get_property;
  object_class->set_property = clay_cell_renderer_color_set_property;

  cell_class->get_size       = clay_cell_renderer_color_get_size;
  cell_class->render         = clay_cell_renderer_color_render;

  g_object_class_install_property (object_class, PROP_COLOR,
                                   clay_param_spec_color ("color",
                                                          "Color",
                                                          "The displayed color",
                                                          TRUE, NULL,
                                                          G_PARAM_READWRITE |
                                                          G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_OPAQUE,
                                   g_param_spec_boolean ("opaque",
                                                         "Opaque",
                                                         "Whether to show transparency",
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     "Icon Size",
                                                     "The cell's size",
                                                     0, G_MAXINT,
                                                     DEFAULT_ICON_SIZE,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
clay_cell_renderer_color_init (ClayCellRendererColor *cell)
{
  cell->color = gegl_color_new ("black");
}

static void
clay_cell_renderer_color_finalize (GObject *object)
{
  ClayCellRendererColor *renderer = CLAY_CELL_RENDERER_COLOR (object);

  g_clear_object (&renderer->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_cell_renderer_color_get_property (GObject    *object,
                                       guint       param_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayCellRendererColor *renderer = CLAY_CELL_RENDERER_COLOR (object);

  switch (param_id)
    {
    case PROP_COLOR:
      g_clear_object (&renderer->color);
      renderer->color = gegl_color_duplicate (g_value_get_object (value));
      break;
    case PROP_OPAQUE:
      g_value_set_boolean (value, renderer->opaque);
      break;
    case PROP_SIZE:
      g_value_set_int (value, renderer->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
clay_cell_renderer_color_set_property (GObject      *object,
                                       guint         param_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayCellRendererColor *renderer = CLAY_CELL_RENDERER_COLOR (object);

  switch (param_id)
    {
    case PROP_COLOR:
      g_set_object (&renderer->color, g_value_get_object (value));
      break;
    case PROP_OPAQUE:
      renderer->opaque = g_value_get_boolean (value);
      break;
    case PROP_SIZE:
      renderer->size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
clay_cell_renderer_color_get_size (GtkCellRenderer    *cell,
                                   GtkWidget          *widget,
                                   const GdkRectangle *cell_area,
                                   gint               *x_offset,
                                   gint               *y_offset,
                                   gint               *width,
                                   gint               *height)
{
  ClayCellRendererColor *renderer = CLAY_CELL_RENDERER_COLOR (cell);
  gint                   calc_width;
  gint                   calc_height;
  gfloat                 xalign;
  gfloat                 yalign;
  gint                   xpad;
  gint                   ypad;

  gtk_icon_size_lookup (renderer->size, &calc_width, &calc_height);
  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  if (cell_area && calc_width > 0 && calc_height > 0)
    {
      if (x_offset)
        {
          *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        1.0 - xalign : xalign) *
                       (cell_area->width - calc_width));
          *x_offset = MAX (*x_offset, 0) + xpad;
        }
      if (y_offset)
        {
          *y_offset = (yalign * (cell_area->height - calc_height));
          *y_offset = MAX (*y_offset, 0) + ypad;
        }
    }
  else
    {
      if (x_offset)
        *x_offset = 0;
      if (y_offset)
        *y_offset = 0;
    }

  if (width)
    *width  = calc_width  + 2 * xpad;
  if (height)
    *height = calc_height + 2 * ypad;
}

static void
clay_cell_renderer_color_render (GtkCellRenderer      *cell,
                                 cairo_t              *cr,
                                 GtkWidget            *widget,
                                 const GdkRectangle   *background_area,
                                 const GdkRectangle   *cell_area,
                                 GtkCellRendererState  flags)
{
  ClayCellRendererColor *renderer = CLAY_CELL_RENDERER_COLOR (cell);
  GdkRectangle           rect;
  gint                   xpad;
  gint                   ypad;

  clay_cell_renderer_color_get_size (cell, widget, cell_area,
                                     &rect.x,
                                     &rect.y,
                                     &rect.width,
                                     &rect.height);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  rect.x      += cell_area->x + xpad;
  rect.y      += cell_area->y + ypad;
  rect.width  -= 2 * xpad;
  rect.height -= 2 * ypad;

  if (rect.width > 2 && rect.height > 2)
    {
      GtkStyleContext *context = gtk_widget_get_style_context (widget);
      GdkRGBA          color;
      gdouble          rgba[4];

      cairo_rectangle (cr,
                       rect.x + 1, rect.y + 1,
                       rect.width - 2, rect.height - 2);

      clay_cairo_set_source_color (cr, renderer->color, NULL, FALSE,
                                   widget);
      cairo_fill (cr);

      gegl_color_get_pixel (renderer->color, babl_format ("R'G'B'A double"), rgba);
      if (! renderer->opaque && rgba[3] < 1.0)
        {
          cairo_pattern_t *pattern;

          cairo_move_to (cr, rect.x + 1,              rect.y + rect.height - 1);
          cairo_line_to (cr, rect.x + rect.width - 1, rect.y + rect.height - 1);
          cairo_line_to (cr, rect.x + rect.width - 1, rect.y + 1);
          cairo_close_path (cr);

          pattern = clay_cairo_checkerboard_create (cr,
                                                    CLAY_CHECK_SIZE_SM,
                                                    NULL, NULL);
          cairo_set_source (cr, pattern);
          cairo_pattern_destroy (pattern);

          cairo_fill_preserve (cr);

          clay_cairo_set_source_color (cr, renderer->color, NULL, FALSE,
                                       widget);
          cairo_fill (cr);
        }

      /* draw border */
      cairo_rectangle (cr,
                       rect.x + 0.5, rect.y + 0.5,
                       rect.width - 1, rect.height - 1);

      cairo_set_line_width (cr, 1);
      gtk_style_context_get_color (context,
                                   gtk_cell_renderer_get_state (cell, widget, flags),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_stroke (cr);
    }
}

/**
 * clay_cell_renderer_color_new:
 *
 * Creates a #GtkCellRenderer that displays a color.
 *
 * Returns: a new #ClayCellRendererColor
 *
 * Since: 2.2
 **/
GtkCellRenderer *
clay_cell_renderer_color_new (void)
{
  return g_object_new (CLAY_TYPE_CELL_RENDERER_COLOR, NULL);
}
