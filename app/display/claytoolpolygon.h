/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolpolygon.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "claytoolwidget.h"


#define CLAY_TYPE_TOOL_POLYGON            (clay_tool_polygon_get_type ())
#define CLAY_TOOL_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_POLYGON, ClayToolPolygon))
#define CLAY_TOOL_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_POLYGON, ClayToolPolygonClass))
#define CLAY_IS_TOOL_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_POLYGON))
#define CLAY_IS_TOOL_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_POLYGON))
#define CLAY_TOOL_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_POLYGON, ClayToolPolygonClass))


typedef struct _ClayToolPolygon        ClayToolPolygon;
typedef struct _ClayToolPolygonPrivate ClayToolPolygonPrivate;
typedef struct _ClayToolPolygonClass   ClayToolPolygonClass;

struct _ClayToolPolygon
{
  ClayToolWidget          parent_instance;

  ClayToolPolygonPrivate *private;
};

struct _ClayToolPolygonClass
{
  ClayToolWidgetClass  parent_class;

  /*  signals  */
  void (* change_complete) (ClayToolPolygon *polygon);
};


GType            clay_tool_polygon_get_type   (void);

ClayToolWidget * clay_tool_polygon_new        (ClayDisplayShell   *shell);

gboolean         clay_tool_polygon_is_closed  (ClayToolPolygon    *polygon);
void             clay_tool_polygon_get_points (ClayToolPolygon    *polygon,
                                               const ClayVector2 **points,
                                               gint               *n_points);
