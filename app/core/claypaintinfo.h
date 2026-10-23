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

#include "clayviewable.h"


#define CLAY_TYPE_PAINT_INFO            (clay_paint_info_get_type ())
#define CLAY_PAINT_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINT_INFO, ClayPaintInfo))
#define CLAY_PAINT_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINT_INFO, ClayPaintInfoClass))
#define CLAY_IS_PAINT_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINT_INFO))
#define CLAY_IS_PAINT_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINT_INFO))
#define CLAY_PAINT_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINT_INFO, ClayPaintInfoClass))


typedef struct _ClayPaintInfoClass ClayPaintInfoClass;

struct _ClayPaintInfo
{
  ClayViewable      parent_instance;

  Clay             *clay;

  GType             paint_type;
  GType             paint_options_type;

  gchar            *blurb;

  ClayPaintOptions *paint_options;
};

struct _ClayPaintInfoClass
{
  ClayViewableClass  parent_class;
};


GType           clay_paint_info_get_type     (void);

ClayPaintInfo * clay_paint_info_new          (Clay          *clay,
                                              GType          paint_type,
                                              GType          paint_options_type,
                                              const gchar   *identifier,
                                              const gchar   *blurb,
                                              const gchar   *icon_name);

void            clay_paint_info_set_standard (Clay          *clay,
                                              ClayPaintInfo *paint_info);
ClayPaintInfo * clay_paint_info_get_standard (Clay          *clay);
