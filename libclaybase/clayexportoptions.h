/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayexportoptions.h
 * Copyright (C) 2024 Alx Sa.
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif


#ifndef __CLAY_EXPORT_OPTIONS_H__
#define __CLAY_EXPORT_OPTIONS_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_EXPORT_OPTIONS               (clay_export_options_get_type ())
#define CLAY_VALUE_HOLDS_EXPORT_OPTIONS(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_EXPORT_OPTIONS))
G_DECLARE_FINAL_TYPE (ClayExportOptions, clay_export_options, CLAY, EXPORT_OPTIONS, GObject)



/*
 * CLAY_TYPE_PARAM_EXPORT_OPTIONS
 */

#define CLAY_TYPE_PARAM_EXPORT_OPTIONS           (clay_param_export_options_get_type ())
#define CLAY_IS_PARAM_SPEC_EXPORT_OPTIONS(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_EXPORT_OPTIONS))


GType        clay_param_export_options_get_type (void);

GParamSpec * clay_param_spec_export_options     (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 GParamFlags   flags);


G_END_DECLS

#endif /* __CLAY_EXPORT_OPTIONS_H__ */
