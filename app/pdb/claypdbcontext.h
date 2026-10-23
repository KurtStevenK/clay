/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claypdbcontext.h
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

#include "core/claycontext.h"


#define CLAY_TYPE_PDB_CONTEXT            (clay_pdb_context_get_type ())
#define CLAY_PDB_CONTEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PDB_CONTEXT, ClayPDBContext))
#define CLAY_PDB_CONTEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PDB_CONTEXT, ClayPDBContextClass))
#define CLAY_IS_PDB_CONTEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PDB_CONTEXT))
#define CLAY_IS_PDB_CONTEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PDB_CONTEXT))
#define CLAY_PDB_CONTEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PDB_CONTEXT, ClayPDBContextClass))


typedef struct _ClayPDBContext      ClayPDBContext;
typedef struct _ClayPDBContextClass ClayPDBContextClass;

struct _ClayPDBContext
{
  ClayContext             parent_instance;

  gboolean                antialias;
  gboolean                feather;
  gdouble                 feather_radius_x;
  gdouble                 feather_radius_y;
  gboolean                sample_merged;
  ClaySelectCriterion     sample_criterion;
  gdouble                 sample_threshold;
  gboolean                sample_transparent;
  gboolean                diagonal_neighbors;

  ClayInterpolationType   interpolation;
  ClayTransformDirection  transform_direction;
  ClayTransformResize     transform_resize;

  ClayContainer          *paint_options_list;
  ClayStrokeOptions      *stroke_options;

  GeglDistanceMetric      distance_metric;
};

struct _ClayPDBContextClass
{
  ClayContextClass  parent_class;
};


GType               clay_pdb_context_get_type           (void);

ClayContext       * clay_pdb_context_new                (Clay           *clay,
                                                         ClayContext    *parent,
                                                         gboolean        set_parent);

ClayContainer     * clay_pdb_context_get_paint_options_list
                                                        (ClayPDBContext *context);
ClayPaintOptions  * clay_pdb_context_get_paint_options  (ClayPDBContext *context,
                                                         const gchar    *name);
GList             * clay_pdb_context_get_brush_options  (ClayPDBContext *context);

ClayStrokeOptions * clay_pdb_context_get_stroke_options (ClayPDBContext *context);
