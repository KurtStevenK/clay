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

#include "paint/claypaintoptions.h"


#define CLAY_TYPE_BUCKET_FILL_OPTIONS            (clay_bucket_fill_options_get_type ())
#define CLAY_BUCKET_FILL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BUCKET_FILL_OPTIONS, ClayBucketFillOptions))
#define CLAY_BUCKET_FILL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BUCKET_FILL_OPTIONS, ClayBucketFillOptionsClass))
#define CLAY_IS_BUCKET_FILL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BUCKET_FILL_OPTIONS))
#define CLAY_IS_BUCKET_FILL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BUCKET_FILL_OPTIONS))
#define CLAY_BUCKET_FILL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BUCKET_FILL_OPTIONS, ClayBucketFillOptionsClass))


typedef struct _ClayBucketFillOptions        ClayBucketFillOptions;
typedef struct _ClayBucketFillOptionsPrivate ClayBucketFillOptionsPrivate;
typedef struct _ClayPaintOptionsClass        ClayBucketFillOptionsClass;

struct _ClayBucketFillOptions
{
  ClayPaintOptions              paint_options;

  ClayBucketFillMode            fill_mode;
  ClayBucketFillArea            fill_area;
  gboolean                      fill_transparent;
  gboolean                      sample_merged;
  gboolean                      diagonal_neighbors;
  gboolean                      antialias;
  gboolean                      feather;
  gdouble                       feather_radius;
  gdouble                       threshold;

  GtkWidget                    *line_art_busy_box;
  ClayLineArtSource             line_art_source;
  gdouble                       line_art_threshold;
  gint                          line_art_max_grow;

  gboolean                      line_art_automatic_closure;
  gint                          line_art_max_gap_length;

  gboolean                      line_art_stroke;
  gchar                        *line_art_stroke_tool;
  ClayStrokeOptions            *stroke_options;

  gboolean                      fill_as_line_art;
  gdouble                       fill_as_line_art_threshold;

  ClaySelectCriterion           fill_criterion;

  ClayBucketFillOptionsPrivate *priv;
};


GType       clay_bucket_fill_options_get_type (void);

GtkWidget * clay_bucket_fill_options_gui      (ClayToolOptions *tool_options);
