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

#include "claydata.h"


#define CLAY_TYPE_PATTERN            (clay_pattern_get_type ())
#define CLAY_PATTERN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATTERN, ClayPattern))
#define CLAY_PATTERN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATTERN, ClayPatternClass))
#define CLAY_IS_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATTERN))
#define CLAY_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATTERN))
#define CLAY_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATTERN, ClayPatternClass))


typedef struct _ClayPatternClass ClayPatternClass;

struct _ClayPattern
{
  ClayData     parent_instance;

  ClayTempBuf *mask;
};

struct _ClayPatternClass
{
  ClayDataClass  parent_class;
};


GType         clay_pattern_get_type      (void);

ClayData    * clay_pattern_new           (ClayContext *context,
                                          const gchar *name);
ClayData    * clay_pattern_get_standard  (ClayContext *context);

ClayTempBuf * clay_pattern_get_mask      (ClayPattern *pattern);
GeglBuffer  * clay_pattern_create_buffer (ClayPattern *pattern);
