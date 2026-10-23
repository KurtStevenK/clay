/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayimagechooser.h
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

#ifndef __CLAY_IMAGE_CHOOSER_H__
#define __CLAY_IMAGE_CHOOSER_H__

G_BEGIN_DECLS

#define CLAY_TYPE_IMAGE_CHOOSER (clay_image_chooser_get_type ())
G_DECLARE_FINAL_TYPE (ClayImageChooser, clay_image_chooser, CLAY, IMAGE_CHOOSER, GtkBox)


GtkWidget *    clay_image_chooser_new          (const gchar       *title,
                                                const gchar       *label,
                                                ClayImage         *image);

ClayImage *    clay_image_chooser_get_image    (ClayImageChooser  *chooser);
void           clay_image_chooser_set_image    (ClayImageChooser  *chooser,
                                                ClayImage         *image);

GtkWidget *    clay_image_chooser_get_label    (ClayImageChooser  *widget);

G_END_DECLS

#endif /* __CLAY_IMAGE_CHOOSER_H__ */
