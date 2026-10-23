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


#define CLAY_COLOR_FRAME_ROWS 6


#define CLAY_TYPE_COLOR_FRAME            (clay_color_frame_get_type ())
#define CLAY_COLOR_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_FRAME, ClayColorFrame))
#define CLAY_COLOR_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_FRAME, ClayColorFrameClass))
#define CLAY_IS_COLOR_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_FRAME))
#define CLAY_IS_COLOR_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_FRAME))
#define CLAY_COLOR_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_FRAME, ClayColorFrameClass))


typedef struct _ClayColorFrameClass ClayColorFrameClass;

struct _ClayColorFrame
{
  ClayFrame           parent_instance;

  Clay               *clay;

  gboolean            sample_valid;
  gboolean            sample_average;
  GeglColor          *color;
  gint                x;
  gint                y;

  ClayColorPickMode   pick_mode;

  PangoEllipsizeMode  ellipsize;

  gboolean            has_number;
  gint                number;

  gboolean            has_color_area;
  gboolean            has_coords;

  GtkWidget          *combo;
  GtkWidget          *color_area;
  GtkWidget          *coords_box_x;
  GtkWidget          *coords_box_y;
  GtkWidget          *coords_label_x;
  GtkWidget          *coords_label_y;
  GtkWidget          *name_labels[CLAY_COLOR_FRAME_ROWS];
  GtkWidget          *value_labels[CLAY_COLOR_FRAME_ROWS];

  PangoLayout        *number_layout;

  ClayImage          *image;

  ClayColorConfig    *config;
  ClayColorProfile   *view_profile;
  ClayColorRenderingIntent
                      simulation_intent;
};

struct _ClayColorFrameClass
{
  ClayFrameClass      parent_class;
};


GType       clay_color_frame_get_type           (void);

GtkWidget * clay_color_frame_new                (Clay               *clay);

void        clay_color_frame_set_mode           (ClayColorFrame     *frame,
                                                 ClayColorPickMode   mode);

void        clay_color_frame_set_ellipsize      (ClayColorFrame     *frame,
                                                 PangoEllipsizeMode  ellipsize);

void        clay_color_frame_set_has_number     (ClayColorFrame     *frame,
                                                 gboolean            has_number);
void        clay_color_frame_set_number         (ClayColorFrame     *frame,
                                                 gint                number);

void        clay_color_frame_set_has_color_area (ClayColorFrame     *frame,
                                                 gboolean            has_color_area);
void        clay_color_frame_set_has_coords     (ClayColorFrame     *frame,
                                                 gboolean            has_coords);

void        clay_color_frame_set_color          (ClayColorFrame     *frame,
                                                 gboolean            sample_average,
                                                 GeglColor          *color,
                                                 gint                x,
                                                 gint                y);
void        clay_color_frame_set_invalid        (ClayColorFrame     *frame);

void        clay_color_frame_set_color_config   (ClayColorFrame     *frame,
                                                 ClayColorConfig    *config);

void        clay_color_frame_set_image          (ClayColorFrame     *frame,
                                                 ClayImage          *image);
