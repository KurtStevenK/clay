/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayimagecombobox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_IMAGE_COMBO_BOX_H__
#define __CLAY_IMAGE_COMBO_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * ClayImageConstraintFunc:
 * @image:
 * @data: (closure):
 */
typedef gboolean (* ClayImageConstraintFunc) (ClayImage *image,
                                              gpointer   data);


#define CLAY_TYPE_IMAGE_COMBO_BOX       (clay_image_combo_box_get_type ())
G_DECLARE_FINAL_TYPE (ClayImageComboBox, clay_image_combo_box, CLAY, IMAGE_COMBO_BOX, ClayIntComboBox)


GtkWidget * clay_image_combo_box_new (ClayImageConstraintFunc  constraint,
                                      gpointer                 data,
                                      GDestroyNotify           data_destroy);


G_END_DECLS

#endif /* __CLAY_IMAGE_COMBO_BOX_H__ */
