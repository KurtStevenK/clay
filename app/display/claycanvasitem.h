/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasitem.h
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

#pragma once

#include "core/clayobject.h"


#define CLAY_TYPE_CANVAS_ITEM            (clay_canvas_item_get_type ())
#define CLAY_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CANVAS_ITEM, ClayCanvasItem))
#define CLAY_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CANVAS_ITEM, ClayCanvasItemClass))
#define CLAY_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CANVAS_ITEM))
#define CLAY_IS_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CANVAS_ITEM))
#define CLAY_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CANVAS_ITEM, ClayCanvasItemClass))


typedef struct _ClayCanvasItemPrivate ClayCanvasItemPrivate;
typedef struct _ClayCanvasItemClass   ClayCanvasItemClass;

struct _ClayCanvasItem
{
  ClayObject             parent_instance;

  ClayCanvasItemPrivate *private;
};

struct _ClayCanvasItemClass
{
  ClayObjectClass  parent_class;

  /*  signals  */
  void             (* update)      (ClayCanvasItem   *item,
                                    cairo_region_t   *region);

  /*  virtual functions  */
  void             (* draw)        (ClayCanvasItem   *item,
                                    cairo_t          *cr);
  cairo_region_t * (* get_extents) (ClayCanvasItem   *item);

  void             (* stroke)      (ClayCanvasItem   *item,
                                    cairo_t          *cr);
  void             (* fill)        (ClayCanvasItem   *item,
                                    cairo_t          *cr);

  gboolean         (* hit)         (ClayCanvasItem   *item,
                                    gdouble           x,
                                    gdouble           y);
};


GType            clay_canvas_item_get_type         (void);

ClayDisplayShell * clay_canvas_item_get_shell      (ClayCanvasItem   *item);
ClayImage      * clay_canvas_item_get_image        (ClayCanvasItem   *item);
GtkWidget      * clay_canvas_item_get_canvas       (ClayCanvasItem   *item);

void             clay_canvas_item_draw             (ClayCanvasItem   *item,
                                                    cairo_t          *cr);
cairo_region_t * clay_canvas_item_get_extents      (ClayCanvasItem   *item);

gboolean         clay_canvas_item_hit              (ClayCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y);

void             clay_canvas_item_set_visible      (ClayCanvasItem   *item,
                                                    gboolean          visible);
gboolean         clay_canvas_item_get_visible      (ClayCanvasItem   *item);

void             clay_canvas_item_set_line_cap     (ClayCanvasItem   *item,
                                                    cairo_line_cap_t  line_cap);

void             clay_canvas_item_set_highlight    (ClayCanvasItem   *item,
                                                    gboolean          highlight);
gboolean         clay_canvas_item_get_highlight    (ClayCanvasItem   *item);

void             clay_canvas_item_begin_change     (ClayCanvasItem   *item);
void             clay_canvas_item_end_change       (ClayCanvasItem   *item);

void             clay_canvas_item_suspend_stroking (ClayCanvasItem   *item);
void             clay_canvas_item_resume_stroking  (ClayCanvasItem   *item);

void             clay_canvas_item_suspend_filling  (ClayCanvasItem   *item);
void             clay_canvas_item_resume_filling   (ClayCanvasItem   *item);

void             clay_canvas_item_transform        (ClayCanvasItem   *item,
                                                    cairo_t          *cr);
void             clay_canvas_item_transform_xy     (ClayCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gint             *tx,
                                                    gint             *ty);
void             clay_canvas_item_transform_xy_f   (ClayCanvasItem   *item,
                                                    gdouble           x,
                                                    gdouble           y,
                                                    gdouble          *tx,
                                                    gdouble          *ty);
gdouble          clay_canvas_item_transform_distance
                                                   (ClayCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
gdouble          clay_canvas_item_transform_distance_square
                                                   (ClayCanvasItem   *item,
                                                    gdouble           x1,
                                                    gdouble           y1,
                                                    gdouble           x2,
                                                    gdouble           y2);
void             clay_canvas_item_untransform_viewport
                                                   (ClayCanvasItem   *item,
                                                    gint             *x,
                                                    gint             *y,
                                                    gint             *w,
                                                    gint             *h);


/*  protected  */

void             _clay_canvas_item_update          (ClayCanvasItem   *item,
                                                    cairo_region_t   *region);
gboolean         _clay_canvas_item_needs_update    (ClayCanvasItem   *item);
void             _clay_canvas_item_stroke          (ClayCanvasItem   *item,
                                                    cairo_t          *cr);
void             _clay_canvas_item_fill            (ClayCanvasItem   *item,
                                                    cairo_t          *cr);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayCanvasItem, g_object_unref);
