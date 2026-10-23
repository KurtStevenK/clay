/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayvaluearray.h ported from GValueArray
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_VALUE_ARRAY_H__
#define __CLAY_VALUE_ARRAY_H__

G_BEGIN_DECLS

/**
 * CLAY_TYPE_VALUE_ARRAY:
 *
 * The type ID of the "ClayValueArray" type which is a boxed type,
 * used to pass around pointers to ClayValueArrays.
 *
 * Since: 2.10
 */
#define CLAY_TYPE_VALUE_ARRAY               (clay_value_array_get_type ())
#define CLAY_VALUE_HOLDS_VALUE_ARRAY(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_VALUE_ARRAY))


GType            clay_value_array_get_type              (void);

ClayValueArray * clay_value_array_new                   (gint                   n_prealloced);
ClayValueArray * clay_value_array_new_from_types        (gchar                **error_msg,
                                                         GType                  first_type,
                                                         ...);
ClayValueArray * clay_value_array_new_from_types_valist (gchar                **error_msg,
                                                         GType                  first_type,
                                                         va_list                va_args);
ClayValueArray * clay_value_array_new_from_values       (const GValue          *values,
                                                         gint                   n_values);

ClayValueArray * clay_value_array_copy                  (const ClayValueArray  *value_array);

ClayValueArray * clay_value_array_ref                   (ClayValueArray        *value_array);
void             clay_value_array_unref                 (ClayValueArray        *value_array);

gint             clay_value_array_length                (const ClayValueArray  *value_array);

GValue         * clay_value_array_index                 (const ClayValueArray  *value_array,
                                                         gint                   index);
GeglColor     ** clay_value_array_get_color_array       (const ClayValueArray  *value_array,
                                                         gint                   index);
GObject       ** clay_value_array_get_core_object_array (const ClayValueArray  *value_array,
                                                         gint                   index);

ClayValueArray * clay_value_array_prepend               (ClayValueArray        *value_array,
                                                         const GValue          *value);
ClayValueArray * clay_value_array_append                (ClayValueArray        *value_array,
                                                         const GValue          *value);
ClayValueArray * clay_value_array_insert                (ClayValueArray        *value_array,
                                                         gint                   index,
                                                         const GValue          *value);

ClayValueArray * clay_value_array_remove                (ClayValueArray        *value_array,
                                                         gint                   index);
void             clay_value_array_truncate              (ClayValueArray        *value_array,
                                                         gint                   n_values);


/*
 * CLAY_TYPE_PARAM_VALUE_ARRAY
 */

#define CLAY_TYPE_PARAM_VALUE_ARRAY           (clay_param_value_array_get_type ())
#define CLAY_IS_PARAM_SPEC_VALUE_ARRAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_VALUE_ARRAY))


GType        clay_param_value_array_get_type              (void);

GParamSpec * clay_param_spec_value_array                  (const gchar    *name,
                                                           const gchar    *nick,
                                                           const gchar    *blurb,
                                                           GParamSpec     *element_spec,
                                                           GParamFlags     flags);

GParamSpec * clay_param_spec_value_array_get_element_spec (GParamSpec *pspec);


G_END_DECLS

#endif /* __CLAY_VALUE_ARRAY_H__ */
