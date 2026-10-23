/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claycolorprofile.h
 * Copyright (C) 2014  Michael Natterer <mitch@gimp.org>
 *                     Elle Stone <ellestone@ninedegreesbelow.com>
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

#if !defined (__CLAY_COLOR_H_INSIDE__) && !defined (CLAY_COLOR_COMPILATION)
#error "Only <libclaycolor/claycolor.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_PROFILE_H__
#define __CLAY_COLOR_PROFILE_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_COLOR_PROFILE (clay_color_profile_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorProfile, clay_color_profile, CLAY, COLOR_PROFILE, GObject)


ClayColorProfile * clay_color_profile_new_rgb_srgb          (void);
ClayColorProfile * clay_color_profile_new_rgb_srgb_linear   (void);
ClayColorProfile * clay_color_profile_new_rgb_adobe         (void);

ClayColorProfile * clay_color_profile_new_d65_gray_srgb_trc (void);
ClayColorProfile * clay_color_profile_new_d65_gray_linear   (void);
ClayColorProfile * clay_color_profile_new_d50_gray_lab_trc  (void);

ClayColorProfile *
       clay_color_profile_new_srgb_trc_from_color_profile   (ClayColorProfile  *profile);
ClayColorProfile *
       clay_color_profile_new_linear_from_color_profile     (ClayColorProfile  *profile);

ClayColorProfile * clay_color_profile_new_from_file         (GFile             *file,
                                                             GError           **error);

ClayColorProfile * clay_color_profile_new_from_icc_profile  (const guint8      *data,
                                                             gsize              length,
                                                             GError           **error);
ClayColorProfile * clay_color_profile_new_from_lcms_profile (gpointer           lcms_profile,
                                                             GError           **error);

gboolean           clay_color_profile_save_to_file          (ClayColorProfile  *profile,
                                                             GFile             *file,
                                                             GError           **error);

const guint8     * clay_color_profile_get_icc_profile       (ClayColorProfile  *profile,
                                                             gsize             *length);
gpointer           clay_color_profile_get_lcms_profile      (ClayColorProfile  *profile);

const gchar      * clay_color_profile_get_description       (ClayColorProfile  *profile);
const gchar      * clay_color_profile_get_manufacturer      (ClayColorProfile  *profile);
const gchar      * clay_color_profile_get_model             (ClayColorProfile  *profile);
const gchar      * clay_color_profile_get_copyright         (ClayColorProfile  *profile);

const gchar      * clay_color_profile_get_label             (ClayColorProfile  *profile);
const gchar      * clay_color_profile_get_summary           (ClayColorProfile  *profile);

gboolean           clay_color_profile_is_equal              (ClayColorProfile  *profile1,
                                                             ClayColorProfile  *profile2);

gboolean           clay_color_profile_is_rgb                (ClayColorProfile  *profile);
gboolean           clay_color_profile_is_gray               (ClayColorProfile  *profile);
gboolean           clay_color_profile_is_cmyk               (ClayColorProfile  *profile);

gboolean           clay_color_profile_is_linear             (ClayColorProfile  *profile);

const Babl       * clay_color_profile_get_space             (ClayColorProfile  *profile,
                                                             ClayColorRenderingIntent intent,
                                                             GError           **error);
const Babl       * clay_color_profile_get_format            (ClayColorProfile  *profile,
                                                             const Babl        *format,
                                                             ClayColorRenderingIntent intent,
                                                             GError           **error);

const Babl       * clay_color_profile_get_lcms_format       (const Babl        *format,
                                                             guint32           *lcms_format);


G_END_DECLS

#endif  /* __CLAY_COLOR_PROFILE_H__ */
