/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __CLAY_BASE_TYPES_H__
#define __CLAY_BASE_TYPES_H__


#include <libclaycolor/claycolortypes.h>
#include <libclaymath/claymathtypes.h>

#include <libclaybase/claybaseenums.h>


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/* XXX FIXME move these to a separate file */

#ifdef CLAY_DISABLE_DEPRECATION_WARNINGS
#define CLAY_DEPRECATED
#define CLAY_DEPRECATED_FOR(f)
#define CLAY_UNAVAILABLE(maj,min)
#else
#define CLAY_DEPRECATED G_DEPRECATED
#define CLAY_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f)
#define CLAY_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min)
#endif


typedef struct _ClayChoice        ClayChoice;
typedef struct _ClayParasite      ClayParasite;
typedef struct _ClayEnumDesc      ClayEnumDesc;
typedef struct _ClayExportOptions ClayExportOptions;
typedef struct _ClayFlagsDesc     ClayFlagsDesc;
typedef struct _ClayUnit          ClayUnit;
typedef struct _ClayValueArray    ClayValueArray;

typedef struct _ClayMetadata      ClayMetadata;


/**
 * ClayEnumDesc:
 * @value:      An enum value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for enum values. See clay_enum_set_value_descriptions().
 **/
struct _ClayEnumDesc
{
  gint         value;
  const gchar *value_desc;
  const gchar *value_help;
};

/**
 * ClayFlagsDesc:
 * @value:      A flag value.
 * @value_desc: The value's description.
 * @value_help: The value's help text.
 *
 * This structure is used to register translatable descriptions and
 * help texts for flag values. See clay_flags_set_value_descriptions().
 **/
struct _ClayFlagsDesc
{
  guint        value;
  const gchar *value_desc;
  const gchar *value_help;
};


void                  clay_type_set_translation_domain  (GType                type,
                                                         const gchar         *domain);
const gchar         * clay_type_get_translation_domain  (GType                type);

void                  clay_type_set_translation_context (GType                type,
                                                         const gchar         *context);
const gchar         * clay_type_get_translation_context (GType                type);

void                  clay_enum_set_value_descriptions  (GType                enum_type,
                                                         const ClayEnumDesc  *descriptions);
const ClayEnumDesc  * clay_enum_get_value_descriptions  (GType                enum_type);

void                  clay_flags_set_value_descriptions (GType                flags_type,
                                                         const ClayFlagsDesc *descriptions);
const ClayFlagsDesc * clay_flags_get_value_descriptions (GType                flags_type);


G_END_DECLS

#endif  /* __CLAY_BASE_TYPES_H__ */
