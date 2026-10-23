/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayunit.h
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_UNIT_H__
#define __CLAY_UNIT_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/**
 * CLAY_TYPE_UNIT:
 *
 * #CLAY_TYPE_UNIT is a class representing units used for measuring
 * dimensions in Clay.
 **/

#define CLAY_TYPE_UNIT (clay_unit_get_type ())
G_DECLARE_FINAL_TYPE (ClayUnit, clay_unit, CLAY, UNIT, GObject)


gint32        clay_unit_get_id                       (ClayUnit    *unit);

const gchar * clay_unit_get_name                     (ClayUnit    *unit);
gdouble       clay_unit_get_factor                   (ClayUnit    *unit);
gint          clay_unit_get_digits                   (ClayUnit    *unit);
gint          clay_unit_get_scaled_digits            (ClayUnit    *unit,
                                                      gdouble      resolution);
const gchar * clay_unit_get_symbol                   (ClayUnit    *unit);
const gchar * clay_unit_get_abbreviation             (ClayUnit    *unit);

gboolean      clay_unit_get_deletion_flag            (ClayUnit    *unit);
void          clay_unit_set_deletion_flag            (ClayUnit    *unit,
                                                      gboolean     deletion_flag);

ClayUnit    * clay_unit_get_by_id                    (gint         unit_id);
ClayUnit    * clay_unit_pixel                        (void);
ClayUnit    * clay_unit_mm                           (void);
ClayUnit    * clay_unit_inch                         (void);
ClayUnit    * clay_unit_point                        (void);
ClayUnit    * clay_unit_pica                         (void);
ClayUnit    * clay_unit_percent                      (void);

gboolean      clay_unit_is_built_in                  (ClayUnit    *unit);
gboolean      clay_unit_is_metric                    (ClayUnit    *unit);


gchar       * clay_unit_format_string                (const gchar *format,
                                                      ClayUnit    *unit);

gdouble       clay_pixels_to_units                   (gdouble      pixels,
                                                      ClayUnit    *unit,
                                                      gdouble      resolution);
gdouble       clay_units_to_pixels                   (gdouble      value,
                                                      ClayUnit    *unit,
                                                      gdouble      resolution);
gdouble       clay_units_to_points                   (gdouble      value,
                                                      ClayUnit    *unit,
                                                      gdouble       resolution);

/*
 * CLAY_TYPE_PARAM_UNIT
 */

#define CLAY_VALUE_HOLDS_UNIT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_UNIT))

#define CLAY_TYPE_PARAM_UNIT           (clay_param_unit_get_type ())
#define CLAY_IS_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_UNIT))


GType        clay_param_unit_get_type             (void);

GParamSpec * clay_param_spec_unit                 (const gchar *name,
                                                   const gchar *nick,
                                                   const gchar *blurb,
                                                   gboolean     allow_pixel,
                                                   gboolean     allow_percent,
                                                   ClayUnit    *default_value,
                                                   GParamFlags  flags);

gboolean     clay_param_spec_unit_pixel_allowed   (GParamSpec  *pspec);
gboolean     clay_param_spec_unit_percent_allowed (GParamSpec  *pspec);

G_END_DECLS

#endif /* __CLAY_UNIT_H__ */
