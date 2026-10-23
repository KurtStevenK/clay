/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-babl.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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


void                clay_babl_init                         (void);
void                clay_babl_init_fishes                  (ClayInitStatusFunc  status_callback);

const gchar       * clay_babl_format_get_description       (const Babl         *format);
ClayColorProfile  * clay_babl_format_get_color_profile     (const Babl         *format);
ClayColorProfile  * clay_babl_get_builtin_color_profile    (ClayImageBaseType   base_type,
                                                            ClayTRCType         trc);

ClayImageBaseType   clay_babl_format_get_base_type         (const Babl         *format);
ClayComponentType   clay_babl_format_get_component_type    (const Babl         *format);
ClayPrecision       clay_babl_format_get_precision         (const Babl         *format);
ClayTRCType         clay_babl_format_get_trc               (const Babl         *format);

ClayComponentType   clay_babl_component_type               (ClayPrecision       precision);
ClayTRCType         clay_babl_trc                          (ClayPrecision       precision);
ClayPrecision       clay_babl_precision                    (ClayComponentType   component,
                                                            ClayTRCType         trc);

gboolean            clay_babl_is_valid                     (ClayImageBaseType   base_type,
                                                            ClayPrecision       precision);
gboolean            clay_babl_is_bounded                   (ClayPrecision       precision);

const Babl        * clay_babl_format                       (ClayImageBaseType   base_type,
                                                            ClayPrecision       precision,
                                                            gboolean            with_alpha,
                                                            const Babl         *space);
const Babl        * clay_babl_mask_format                  (ClayPrecision       precision);
const Babl        * clay_babl_component_format             (ClayImageBaseType   base_type,
                                                            ClayPrecision       precision,
                                                            gint                index);

const Babl        * clay_babl_format_change_component_type (const Babl          *format,
                                                            ClayComponentType    component);
const Babl        * clay_babl_format_change_trc            (const Babl          *format,
                                                            ClayTRCType          trc);

gchar            ** clay_babl_print_color                  (GeglColor           *color);
