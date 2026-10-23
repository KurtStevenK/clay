/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claypickable.h
 * Copyright (C) 2004  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PICKABLE (clay_pickable_get_type ())
G_DECLARE_INTERFACE (ClayPickable,
                     clay_pickable,
                     CLAY, PICKABLE,
                     GObject)


struct _ClayPickableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void            (* flush)                   (ClayPickable        *pickable);
  ClayImage     * (* get_image)               (ClayPickable        *pickable);
  const Babl    * (* get_format)              (ClayPickable        *pickable);
  const Babl    * (* get_format_with_alpha)   (ClayPickable        *pickable);
  GeglBuffer    * (* get_buffer)              (ClayPickable        *pickable);
  GeglBuffer    * (* get_buffer_with_effects) (ClayPickable        *pickable);
  gboolean        (* get_pixel_at)            (ClayPickable        *pickable,
                                               gint                 x,
                                               gint                 y,
                                               const Babl          *format,
                                               gpointer             pixel);
  gdouble         (* get_opacity_at)          (ClayPickable        *pickable,
                                               gint                 x,
                                               gint                 y);
  void            (* get_pixel_average)       (ClayPickable        *pickable,
                                               const GeglRectangle *rect,
                                               const Babl          *format,
                                               gpointer             pixel);
};


void            clay_pickable_flush                   (ClayPickable        *pickable);
ClayImage     * clay_pickable_get_image               (ClayPickable        *pickable);
const Babl    * clay_pickable_get_format              (ClayPickable        *pickable);
const Babl    * clay_pickable_get_format_with_alpha   (ClayPickable        *pickable);
GeglBuffer    * clay_pickable_get_buffer              (ClayPickable        *pickable);
GeglBuffer    * clay_pickable_get_buffer_with_effects (ClayPickable        *pickable);
gboolean        clay_pickable_get_pixel_at            (ClayPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y,
                                                       const Babl          *format,
                                                       gpointer             pixel);
GeglColor     * clay_pickable_get_color_at            (ClayPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y);
gdouble         clay_pickable_get_opacity_at          (ClayPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y);
void            clay_pickable_get_pixel_average       (ClayPickable        *pickable,
                                                       const GeglRectangle *rect,
                                                       const Babl          *format,
                                                       gpointer             pixel);

gboolean        clay_pickable_pick_color              (ClayPickable        *pickable,
                                                       gint                 x,
                                                       gint                 y,
                                                       gboolean             sample_average,
                                                       gdouble              average_radius,
                                                       gpointer             pixel,
                                                       GeglColor          **color);
