/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimage-color-profile.h
 * Copyright (C) 2015-2018 Michael Natterer <mitch@gimp.org>
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


#define CLAY_ICC_PROFILE_PARASITE_NAME            "icc-profile"
#define CLAY_SIMULATION_ICC_PROFILE_PARASITE_NAME "simulation-icc-profile"


gboolean             clay_image_get_use_srgb_profile   (ClayImage           *image,
                                                        gboolean            *hidden_profile);
void                 clay_image_set_use_srgb_profile   (ClayImage           *image,
                                                        gboolean             use_srgb);

ClayColorProfile   * _clay_image_get_hidden_profile    (ClayImage           *image);
void                 _clay_image_set_hidden_profile    (ClayImage           *image,
                                                        ClayColorProfile    *profile,
                                                        gboolean             push_undo);

gboolean             clay_image_validate_icc_parasite  (ClayImage           *image,
                                                        const ClayParasite  *icc_parasite,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const ClayParasite * clay_image_get_icc_parasite       (ClayImage           *image);
void                 clay_image_set_icc_parasite       (ClayImage           *image,
                                                        const ClayParasite  *icc_parasite,
                                                        const gchar         *profile_type);

gboolean             clay_image_validate_icc_profile   (ClayImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
const guint8       * clay_image_get_icc_profile        (ClayImage           *image,
                                                        gsize               *length);
gboolean             clay_image_set_icc_profile        (ClayImage           *image,
                                                        const guint8        *data,
                                                        gsize                length,
                                                        const gchar         *profile_type,
                                                        GError             **error);

gboolean             clay_image_validate_color_profile (ClayImage           *image,
                                                        ClayColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);
ClayColorProfile   * clay_image_get_color_profile      (ClayImage           *image);
gboolean             clay_image_set_color_profile      (ClayImage           *image,
                                                        ClayColorProfile    *profile,
                                                        GError             **error);

ClayColorProfile   * clay_image_get_simulation_profile (ClayImage           *image);
gboolean             clay_image_set_simulation_profile (ClayImage           *image,
                                                        ClayColorProfile    *profile);

ClayColorRenderingIntent
                     clay_image_get_simulation_intent  (ClayImage           *image);
void                 clay_image_set_simulation_intent  (ClayImage                *image,
                                                        ClayColorRenderingIntent intent);

gboolean             clay_image_get_simulation_bpc     (ClayImage           *image);
void                 clay_image_set_simulation_bpc     (ClayImage           *image,
                                                        gboolean             bpc);

gboolean             clay_image_validate_color_profile_by_format
                                                       (const Babl          *format,
                                                        ClayColorProfile    *profile,
                                                        gboolean            *is_builtin,
                                                        GError             **error);

ClayColorProfile   * clay_image_get_builtin_color_profile
                                                       (ClayImage           *image);

gboolean             clay_image_assign_color_profile   (ClayImage           *image,
                                                        ClayColorProfile    *dest_profile,
                                                        ClayProgress        *progress,
                                                        GError             **error);

gboolean             clay_image_convert_color_profile  (ClayImage           *image,
                                                        ClayColorProfile    *dest_profile,
                                                        ClayColorRenderingIntent  intent,
                                                        gboolean             bpc,
                                                        ClayProgress        *progress,
                                                        GError             **error);

void                 clay_image_import_color_profile   (ClayImage           *image,
                                                        ClayContext         *context,
                                                        ClayProgress        *progress,
                                                        gboolean             interactive);

ClayColorTransform * clay_image_get_color_transform_to_srgb_u8
                                                       (ClayImage           *image);

ClayColorTransform * clay_image_get_color_transform_to_srgb_double
                                                       (ClayImage           *image);
ClayColorTransform * clay_image_get_color_transform_from_srgb_double
                                                       (ClayImage           *image);


/*  internal API, to be called only from clayimage.c  */

void                 _clay_image_free_color_profile    (ClayImage           *image);
void                 _clay_image_free_color_transforms (ClayImage           *image);
void                 _clay_image_update_color_profile  (ClayImage           *image,
                                                        const ClayParasite  *icc_parasite);
void                 _clay_image_update_simulation_profile
                                                       (ClayImage           *image,
                                                        const ClayParasite  *icc_parasite);
