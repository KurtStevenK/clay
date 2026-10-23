/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageParasiteView
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_IMAGE_PARASITE_VIEW            (clay_image_parasite_view_get_type ())
#define CLAY_IMAGE_PARASITE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_PARASITE_VIEW, ClayImageParasiteView))
#define CLAY_IMAGE_PARASITE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_PARASITE_VIEW, ClayImageParasiteViewClass))
#define CLAY_IS_IMAGE_PARASITE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_PARASITE_VIEW))
#define CLAY_IS_IMAGE_PARASITE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_PARASITE_VIEW))
#define CLAY_IMAGE_PARASITE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_PARASITE_VIEW, ClayImageParasiteViewClass))


typedef struct _ClayImageParasiteViewClass ClayImageParasiteViewClass;

struct _ClayImageParasiteView
{
  GtkBox     parent_instance;

  ClayImage *image;
  gchar     *parasite;
};

struct _ClayImageParasiteViewClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* update) (ClayImageParasiteView *view);
};


GType                clay_image_parasite_view_get_type     (void);

GtkWidget          * clay_image_parasite_view_new          (ClayImage   *image,
                                                            const gchar *parasite);
ClayImage          * clay_image_parasite_view_get_image    (ClayImageParasiteView *view);
const ClayParasite * clay_image_parasite_view_get_parasite (ClayImageParasiteView *view);
