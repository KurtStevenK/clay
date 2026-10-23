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

#ifndef  __CLAY_MYBRUSH_OPTIONS_H__
#define  __CLAY_MYBRUSH_OPTIONS_H__


#include "claypaintoptions.h"


#define CLAY_TYPE_MYBRUSH_OPTIONS            (clay_mybrush_options_get_type ())
#define CLAY_MYBRUSH_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MYBRUSH_OPTIONS, ClayMybrushOptions))
#define CLAY_MYBRUSH_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MYBRUSH_OPTIONS, ClayMybrushOptionsClass))
#define CLAY_IS_MYBRUSH_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MYBRUSH_OPTIONS))
#define CLAY_IS_MYBRUSH_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MYBRUSH_OPTIONS))
#define CLAY_MYBRUSH_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MYBRUSH_OPTIONS, ClayMybrushOptionsClass))


typedef struct _ClayMybrushOptionsClass ClayMybrushOptionsClass;

struct _ClayMybrushOptions
{
  ClayPaintOptions  parent_instance;

  gdouble           view_zoom;
  gdouble           view_rotation;

  gdouble           radius;
  gdouble           opaque;
  gdouble           hardness;
  gdouble           gain;
  gdouble           pigment;
  gboolean          pigment_use;
  gdouble           posterize;
  gdouble           posterize_num;
  gboolean          eraser;
  gboolean          no_erasing;
};

struct _ClayMybrushOptionsClass
{
  ClayPaintOptionsClass  parent_class;
};


GType   clay_mybrush_options_get_type (void);


#endif  /*  __CLAY_MYBRUSH_OPTIONS_H__  */
