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

#include "clayauxitem.h"


#define CLAY_SAMPLE_POINT_POSITION_UNDEFINED G_MININT


#define CLAY_TYPE_SAMPLE_POINT            (clay_sample_point_get_type ())
#define CLAY_SAMPLE_POINT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SAMPLE_POINT, ClaySamplePoint))
#define CLAY_SAMPLE_POINT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SAMPLE_POINT, ClaySamplePointClass))
#define CLAY_IS_SAMPLE_POINT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SAMPLE_POINT))
#define CLAY_IS_SAMPLE_POINT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SAMPLE_POINT))
#define CLAY_SAMPLE_POINT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SAMPLE_POINT, ClaySamplePointClass))


typedef struct _ClaySamplePointPrivate ClaySamplePointPrivate;
typedef struct _ClaySamplePointClass   ClaySamplePointClass;

struct _ClaySamplePoint
{
  ClayAuxItem             parent_instance;

  ClaySamplePointPrivate *priv;
};

struct _ClaySamplePointClass
{
  ClayAuxItemClass  parent_class;
};


GType             clay_sample_point_get_type      (void);

ClaySamplePoint * clay_sample_point_new           (guint32            sample_point_ID);

void              clay_sample_point_get_position  (ClaySamplePoint   *sample_point,
                                                   gint              *position_x,
                                                   gint              *position_y);
void              clay_sample_point_set_position  (ClaySamplePoint   *sample_point,
                                                   gint               position_x,
                                                   gint               position_y);

ClayColorPickMode clay_sample_point_get_pick_mode (ClaySamplePoint   *sample_point);
void              clay_sample_point_set_pick_mode (ClaySamplePoint   *sample_point,
                                                   ClayColorPickMode  pick_mode);
