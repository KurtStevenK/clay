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

#include "core/claytooloptions.h"


#define CLAY_TYPE_PATH_OPTIONS            (clay_path_options_get_type ())
#define CLAY_PATH_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATH_OPTIONS, ClayPathOptions))
#define CLAY_PATH_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATH_OPTIONS, ClayPathOptionsClass))
#define CLAY_IS_PATH_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATH_OPTIONS))
#define CLAY_IS_PATH_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATH_OPTIONS))
#define CLAY_PATH_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATH_OPTIONS, ClayPathOptionsClass))


typedef struct _ClayPathOptions      ClayPathOptions;
typedef struct _ClayToolOptionsClass ClayPathOptionsClass;

struct _ClayPathOptions
{
  ClayToolOptions    parent_instance;

  ClayPathMode       edit_mode;
  gboolean           polygonal;
  gboolean           auto_connect;

  /* vector layer */
  gboolean           enable_fill;
  gboolean           enable_stroke;
  ClayFillOptions   *fill_options;
  ClayStrokeOptions *stroke_options;

  ClayCustomStyle    fill_style;
  GeglColor         *fill_foreground;
  ClayPattern       *fill_pattern;
  gboolean           fill_antialias;

  ClayCustomStyle    stroke_style;
  GeglColor         *stroke_foreground;
  ClayPattern       *stroke_pattern;
  gboolean           stroke_antialias;
  gdouble            stroke_width;
  ClayUnit          *stroke_unit;
  ClayCapStyle       stroke_cap_style;
  ClayJoinStyle      stroke_join_style;
  gdouble            stroke_miter_limit;
  gdouble            stroke_dash_offset;

  /*  options gui  */
  GtkWidget         *to_selection_button;
  GtkWidget         *vector_layer_button;
};


GType       clay_path_options_get_type (void);

GtkWidget * clay_path_options_gui      (ClayToolOptions *tool_options);
