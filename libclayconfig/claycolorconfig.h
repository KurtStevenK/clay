/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayColorConfig class
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
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

#if !defined (__CLAY_CONFIG_H_INSIDE__) && !defined (CLAY_CONFIG_COMPILATION)
#error "Only <libclayconfig/clayconfig.h> can be included directly."
#endif

#ifndef __CLAY_COLOR_CONFIG_H__
#define __CLAY_COLOR_CONFIG_H__


#define CLAY_TYPE_COLOR_CONFIG (clay_color_config_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorConfig, clay_color_config, CLAY, COLOR_CONFIG, GObject)


ClayColorManagementMode
                   clay_color_config_get_mode                     (ClayColorConfig  *config);

ClayColorRenderingIntent
                   clay_color_config_get_display_intent           (ClayColorConfig  *config);
gboolean           clay_color_config_get_display_bpc              (ClayColorConfig  *config);
gboolean           clay_color_config_get_display_optimize         (ClayColorConfig  *config);
gboolean           clay_color_config_get_display_profile_from_gdk (ClayColorConfig  *config);

ClayColorRenderingIntent
                   clay_color_config_get_simulation_intent        (ClayColorConfig  *config);
gboolean           clay_color_config_get_simulation_bpc           (ClayColorConfig  *config);
gboolean           clay_color_config_get_simulation_optimize      (ClayColorConfig  *config);
gboolean           clay_color_config_get_simulation_gamut_check   (ClayColorConfig  *config);
GeglColor        * clay_color_config_get_out_of_gamut_color       (ClayColorConfig  *config);

ClayColorProfile * clay_color_config_get_rgb_color_profile        (ClayColorConfig  *config,
                                                                   GError          **error);
ClayColorProfile * clay_color_config_get_gray_color_profile       (ClayColorConfig  *config,
                                                                   GError          **error);
ClayColorProfile * clay_color_config_get_cmyk_color_profile       (ClayColorConfig  *config,
                                                                   GError          **error);
ClayColorProfile * clay_color_config_get_display_color_profile    (ClayColorConfig  *config,
                                                                   GError          **error);
ClayColorProfile * clay_color_config_get_simulation_color_profile (ClayColorConfig  *config,
                                                                   GError          **error);


#endif /* CLAY_COLOR_CONFIG_H__ */
