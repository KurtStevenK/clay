/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


ClayLayer * clay_layer_new                  (ClayImage        *image,
                                             gint              width,
                                             gint              height,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             ClayLayerMode     mode);

ClayLayer * clay_layer_new_from_buffer      (ClayBuffer       *buffer,
                                             ClayImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             ClayLayerMode     mode);
ClayLayer * clay_layer_new_from_gegl_buffer (GeglBuffer       *buffer,
                                             ClayImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             ClayLayerMode     mode,
                                             ClayColorProfile *buffer_profile);
ClayLayer * clay_layer_new_from_pixbuf      (GdkPixbuf        *pixbuf,
                                             ClayImage        *dest_image,
                                             const Babl       *format,
                                             const gchar      *name,
                                             gdouble           opacity,
                                             ClayLayerMode     mode);
