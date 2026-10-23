/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayunitstore.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_UNIT_STORE_H__
#define __CLAY_UNIT_STORE_H__

G_BEGIN_DECLS


enum
{
  CLAY_UNIT_STORE_UNIT,
  CLAY_UNIT_STORE_UNIT_FACTOR,
  CLAY_UNIT_STORE_UNIT_DIGITS,
  CLAY_UNIT_STORE_UNIT_NAME,
  CLAY_UNIT_STORE_UNIT_SYMBOL,
  CLAY_UNIT_STORE_UNIT_ABBREVIATION,
  CLAY_UNIT_STORE_UNIT_SHORT_FORMAT,
  CLAY_UNIT_STORE_UNIT_LONG_FORMAT,
  CLAY_UNIT_STORE_UNIT_COLUMNS,
  CLAY_UNIT_STORE_FIRST_VALUE = CLAY_UNIT_STORE_UNIT_COLUMNS
};


#define CLAY_TYPE_UNIT_STORE (clay_unit_store_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayUnitStore, clay_unit_store, CLAY, UNIT_STORE, GObject)

struct _ClayUnitStoreClass
{
  GObjectClass  parent_class;

  /* Padding for future expansion */
  void (*_clay_reserved0) (void);
  void (*_clay_reserved1) (void);
  void (*_clay_reserved2) (void);
  void (*_clay_reserved3) (void);
  void (*_clay_reserved4) (void);
  void (*_clay_reserved5) (void);
  void (*_clay_reserved6) (void);
  void (*_clay_reserved7) (void);
  void (*_clay_reserved8) (void);
  void (*_clay_reserved9) (void);
};


ClayUnitStore * clay_unit_store_new              (gint           num_values);

void            clay_unit_store_set_has_pixels   (ClayUnitStore *store,
                                                  gboolean       has_pixels);
gboolean        clay_unit_store_get_has_pixels   (ClayUnitStore *store);

void            clay_unit_store_set_has_percent  (ClayUnitStore *store,
                                                  gboolean       has_percent);
gboolean        clay_unit_store_get_has_percent  (ClayUnitStore *store);

void            clay_unit_store_set_pixel_value  (ClayUnitStore *store,
                                                  gint           index,
                                                  gdouble        value);
void            clay_unit_store_set_pixel_values (ClayUnitStore *store,
                                                  gdouble        first_value,
                                                  ...);
void            clay_unit_store_set_resolution   (ClayUnitStore *store,
                                                  gint           index,
                                                  gdouble        resolution);
void            clay_unit_store_set_resolutions  (ClayUnitStore *store,
                                                  gdouble        first_resolution,
                                                  ...);
gdouble         clay_unit_store_get_nth_value    (ClayUnitStore *store,
                                                  ClayUnit      *unit,
                                                  gint           index);
void            clay_unit_store_get_values       (ClayUnitStore *store,
                                                  ClayUnit      *unit,
                                                  gdouble       *first_value,
                                                  ...);

void            _clay_unit_store_sync_units      (ClayUnitStore *store);


G_END_DECLS

#endif  /* __CLAY_UNIT_STORE_H__ */
