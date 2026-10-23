/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayDisplayOptions
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#define CLAY_TYPE_DISPLAY_OPTIONS            (clay_display_options_get_type ())
#define CLAY_TYPE_DISPLAY_OPTIONS_FULLSCREEN (clay_display_options_fullscreen_get_type ())
#define CLAY_TYPE_DISPLAY_OPTIONS_NO_IMAGE   (clay_display_options_no_image_get_type ())

#define CLAY_DISPLAY_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DISPLAY_OPTIONS, ClayDisplayOptions))
#define CLAY_DISPLAY_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DISPLAY_OPTIONS, ClayDisplayOptionsClass))
#define CLAY_IS_DISPLAY_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DISPLAY_OPTIONS))
#define CLAY_IS_DISPLAY_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DISPLAY_OPTIONS))
#define CLAY_DISPLAY_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DISPLAY_OPTIONS, ClayDisplayOptionsClass))


typedef struct _ClayDisplayOptionsClass ClayDisplayOptionsClass;

struct _ClayDisplayOptions
{
  GObject                parent_instance;

  /*  ClayImageWindow options  */
  gboolean               show_menubar;
  gboolean               show_statusbar;

  /*  ClayDisplayShell options  */
  gboolean               show_rulers;
  gboolean               show_scrollbars;
  gboolean               show_selection;
  gboolean               show_layer_boundary;
  gboolean               show_canvas_boundary;
  gboolean               show_guides;
  gboolean               show_grid;
  gboolean               show_sample_points;

  gboolean               snap_to_guides;
  gboolean               snap_to_grid;
  gboolean               snap_to_canvas;
  gboolean               snap_to_path;
  gboolean               snap_to_bbox;
  gboolean               snap_to_equidistance;

  ClayCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;
  gboolean               padding_mode_set;
  gboolean               padding_in_show_all;
};

struct _ClayDisplayOptionsClass
{
  GObjectClass           parent_class;
};


GType  clay_display_options_get_type            (void);
GType  clay_display_options_fullscreen_get_type (void);
GType  clay_display_options_no_image_get_type   (void);
