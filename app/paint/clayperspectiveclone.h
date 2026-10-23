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

#include "clayclone.h"


#define CLAY_TYPE_PERSPECTIVE_CLONE            (clay_perspective_clone_get_type ())
#define CLAY_PERSPECTIVE_CLONE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PERSPECTIVE_CLONE, ClayPerspectiveClone))
#define CLAY_PERSPECTIVE_CLONE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PERSPECTIVE_CLONE, ClayPerspectiveCloneClass))
#define CLAY_IS_PERSPECTIVE_CLONE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PERSPECTIVE_CLONE))
#define CLAY_IS_PERSPECTIVE_CLONE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PERSPECTIVE_CLONE))
#define CLAY_PERSPECTIVE_CLONE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PERSPECTIVE_CLONE, ClayPerspectiveCloneClass))


typedef struct _ClayPerspectiveCloneClass ClayPerspectiveCloneClass;

struct _ClayPerspectiveClone
{
  ClayClone      parent_instance;

  gdouble        src_x_fv;     /* source coords in front_view perspective */
  gdouble        src_y_fv;

  gdouble        dest_x_fv;    /* destination coords in front_view perspective */
  gdouble        dest_y_fv;

  ClayMatrix3    transform;
  ClayMatrix3    transform_inv;

  GeglNode      *node;
  GeglNode      *crop;
  GeglNode      *transform_node;
  GeglNode      *src_node;
  GeglNode      *dest_node;
};

struct _ClayPerspectiveCloneClass
{
  ClayCloneClass  parent_class;
};


void    clay_perspective_clone_register      (Clay                      *clay,
                                              ClayPaintRegisterCallback  callback);

GType   clay_perspective_clone_get_type         (void);

void    clay_perspective_clone_set_transform    (ClayPerspectiveClone   *clone,
                                                 ClayMatrix3            *transform);
void    clay_perspective_clone_get_source_point (ClayPerspectiveClone   *clone,
                                                 gdouble                 x,
                                                 gdouble                 y,
                                                 gdouble                *newx,
                                                 gdouble                *newy);
