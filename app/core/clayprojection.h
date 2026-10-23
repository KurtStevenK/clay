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

#include "clayobject.h"


#define CLAY_TYPE_PROJECTION            (clay_projection_get_type ())
#define CLAY_PROJECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PROJECTION, ClayProjection))
#define CLAY_PROJECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PROJECTION, ClayProjectionClass))
#define CLAY_IS_PROJECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PROJECTION))
#define CLAY_IS_PROJECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PROJECTION))
#define CLAY_PROJECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PROJECTION, ClayProjectionClass))


typedef struct _ClayProjectionPrivate ClayProjectionPrivate;
typedef struct _ClayProjectionClass   ClayProjectionClass;

struct _ClayProjection
{
  ClayObject             parent_instance;

  ClayProjectionPrivate *priv;
};

struct _ClayProjectionClass
{
  ClayObjectClass  parent_class;

  void (* update) (ClayProjection *proj,
                   gboolean        now,
                   gint            x,
                   gint            y,
                   gint            width,
                   gint            height);
};


GType            clay_projection_get_type          (void);

ClayProjection * clay_projection_new               (ClayProjectable   *projectable);

void             clay_projection_set_priority      (ClayProjection    *projection,
                                                    gint               priority);
gint             clay_projection_get_priority      (ClayProjection    *projection);

void             clay_projection_set_priority_rect (ClayProjection    *proj,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

void             clay_projection_stop_rendering    (ClayProjection    *proj);

void             clay_projection_flush             (ClayProjection    *proj);
void             clay_projection_flush_now         (ClayProjection    *proj,
                                                    gboolean           direct);
void             clay_projection_finish_draw       (ClayProjection    *proj);

gint64           clay_projection_estimate_memsize  (ClayImageBaseType  type,
                                                    ClayComponentType  component_type,
                                                    gint               width,
                                                    gint               height);

gboolean         clay_projection_has_alpha         (ClayProjection    *proj);
