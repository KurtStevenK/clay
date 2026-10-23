/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayitemcombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2006 Simon Budig <simon@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_ITEM_COMBO_BOX_H__
#define __CLAY_ITEM_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_DRAWABLE_COMBO_BOX    (clay_drawable_combo_box_get_type ())
#define CLAY_DRAWABLE_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_COMBO_BOX, ClayDrawableComboBox))
#define CLAY_IS_DRAWABLE_COMBO_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_COMBO_BOX))

#define CLAY_TYPE_CHANNEL_COMBO_BOX     (clay_channel_combo_box_get_type ())
#define CLAY_CHANNEL_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CHANNEL_COMBO_BOX, ClayChannelComboBox))
#define CLAY_IS_CHANNEL_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CHANNEL_COMBO_BOX))

#define CLAY_TYPE_LAYER_COMBO_BOX       (clay_layer_combo_box_get_type ())
#define CLAY_LAYER_COMBO_BOX(obj)       (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER_COMBO_BOX, ClayLayerComboBox))
#define CLAY_IS_LAYER_COMBO_BOX(obj)    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER_COMBO_BOX))

#define CLAY_TYPE_PATH_COMBO_BOX     (clay_path_combo_box_get_type ())
#define CLAY_PATH_COMBO_BOX(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATH_COMBO_BOX, ClayPathComboBox))
#define CLAY_IS_PATH_COMBO_BOX(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATH_COMBO_BOX))


GType       clay_drawable_combo_box_get_type (void);
GType       clay_channel_combo_box_get_type  (void);
GType       clay_layer_combo_box_get_type    (void);
GType       clay_path_combo_box_get_type     (void);

/**
 * ClayItemConstraintFunc:
 * @image:
 * @item:
 * @data: (closure):
 */
typedef gboolean (* ClayItemConstraintFunc) (ClayImage *image,
                                             ClayItem  *item,
                                             gpointer   data);

GtkWidget * clay_drawable_combo_box_new (ClayItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * clay_channel_combo_box_new  (ClayItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * clay_layer_combo_box_new    (ClayItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);
GtkWidget * clay_path_combo_box_new     (ClayItemConstraintFunc constraint,
                                         gpointer               data,
                                         GDestroyNotify         data_destroy);


G_END_DECLS

#endif /* __CLAY_ITEM_COMBO_BOX_H__ */
