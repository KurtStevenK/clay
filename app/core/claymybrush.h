/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymybrush.h
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


#define CLAY_TYPE_MYBRUSH            (clay_mybrush_get_type ())
#define CLAY_MYBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MYBRUSH, ClayMybrush))
#define CLAY_MYBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MYBRUSH, ClayMybrushClass))
#define CLAY_IS_MYBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MYBRUSH))
#define CLAY_IS_MYBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MYBRUSH))
#define CLAY_MYBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MYBRUSH, ClayMybrushClass))


typedef struct _ClayMybrushPrivate ClayMybrushPrivate;
typedef struct _ClayMybrushClass   ClayMybrushClass;

struct _ClayMybrush
{
  ClayData            parent_instance;

  ClayMybrushPrivate *priv;
};

struct _ClayMybrushClass
{
  ClayDataClass  parent_class;
};


GType         clay_mybrush_get_type             (void);

ClayData    * clay_mybrush_new                  (ClayContext *context,
                                                 const gchar *name);
ClayData    * clay_mybrush_get_standard         (ClayContext *context);

const gchar * clay_mybrush_get_brush_json       (ClayMybrush *brush);

gdouble       clay_mybrush_get_radius           (ClayMybrush *brush);
gdouble       clay_mybrush_get_opaque           (ClayMybrush *brush);
gdouble       clay_mybrush_get_hardness         (ClayMybrush *brush);
gdouble       clay_mybrush_get_gain             (ClayMybrush *brush);
gdouble       clay_mybrush_get_pigment          (ClayMybrush *brush);
gdouble       clay_mybrush_get_posterize        (ClayMybrush *brush);
gdouble       clay_mybrush_get_posterize_num    (ClayMybrush *brush);
gdouble       clay_mybrush_get_offset_by_random (ClayMybrush *brush);
gboolean      clay_mybrush_get_is_eraser        (ClayMybrush *brush);
