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


/*
 * CLAY_TYPE_PARAM_STRING
 */

#define CLAY_TYPE_PARAM_STRING           (clay_param_string_get_type ())
#define CLAY_PARAM_SPEC_STRING(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_STRING, ClayParamSpecString))
#define CLAY_IS_PARAM_SPEC_STRING(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_STRING))

typedef struct _ClayParamSpecString ClayParamSpecString;

struct _ClayParamSpecString
{
  GParamSpecString parent_instance;

  guint            allow_non_utf8 : 1;
  guint            non_empty      : 1;
};

GType        clay_param_string_get_type (void);

GParamSpec * clay_param_spec_string     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         gboolean     allow_non_utf8,
                                         gboolean     null_ok,
                                         gboolean     non_empty,
                                         const gchar *default_value,
                                         GParamFlags  flags);


/*
 * CLAY_TYPE_PARAM_ENUM
 */

#define CLAY_TYPE_PARAM_ENUM           (clay_param_enum_get_type ())
#define CLAY_PARAM_SPEC_ENUM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_ENUM, ClayParamSpecEnum))

#define CLAY_IS_PARAM_SPEC_ENUM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_ENUM))

typedef struct _ClayParamSpecEnum ClayParamSpecEnum;

struct _ClayParamSpecEnum
{
  GParamSpecEnum  parent_instance;

  GSList         *excluded_values;
};

GType        clay_param_enum_get_type     (void);

GParamSpec * clay_param_spec_enum         (const gchar       *name,
                                           const gchar       *nick,
                                           const gchar       *blurb,
                                           GType              enum_type,
                                           gint               default_value,
                                           GParamFlags        flags);

void   clay_param_spec_enum_exclude_value (ClayParamSpecEnum *espec,
                                           gint               value);


/*  include the declaration of the remaining paramspecs, they are
 *  identical app/ and libclay/.
 */
#define CLAY_COMPILATION
#include "../../libclay/clayparamspecs.h"
#undef CLAY_COMPILATION
