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


#define CLAY_TYPE_TEXT_OPTIONS            (clay_text_options_get_type ())
#define CLAY_TEXT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_OPTIONS, ClayTextOptions))
#define CLAY_TEXT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_OPTIONS, ClayTextOptionsClass))
#define CLAY_IS_TEXT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_OPTIONS))
#define CLAY_IS_TEXT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_OPTIONS))
#define CLAY_TEXT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEXT_OPTIONS, ClayTextOptionsClass))


typedef struct _ClayTextOptions      ClayTextOptions;
typedef struct _ClayToolOptionsClass ClayTextOptionsClass;

struct _ClayTextOptions
{
  ClayToolOptions           tool_options;

  ClayUnit                 *unit;
  gdouble                   font_size;
  gboolean                  antialias;
  ClayTextHintStyle         hint_style;
  gchar                    *language;
  ClayTextDirection         base_dir;
  ClayTextJustification     justify;
  gdouble                   indent;
  gdouble                   line_spacing;
  gdouble                   letter_spacing;
  ClayTextBoxMode           box_mode;

  ClayTextOutline           outline;
  ClayCustomStyle           outline_style;
  GeglColor                *outline_foreground;
  ClayPattern              *outline_pattern;
  gdouble                   outline_width;
  ClayUnit                 *outline_unit;
  ClayTextOutlineDirection  outline_direction;
  ClayCapStyle              outline_cap_style;
  ClayJoinStyle             outline_join_style;
  gdouble                   outline_miter_limit;
  gboolean                  outline_antialias;
  gdouble                   outline_dash_offset;
  GArray                   *outline_dash_info;

  ClayViewType              font_view_type;
  ClayViewSize              font_view_size;

  gboolean                  use_editor;
  gboolean                  show_on_canvas;

  /*  options gui  */
  GtkWidget                *size_entry;
};


GType       clay_text_options_get_type     (void);

void        clay_text_options_connect_text (ClayTextOptions *options,
                                            ClayText        *text);

GtkWidget * clay_text_options_gui          (ClayToolOptions *tool_options);

GtkWidget * clay_text_options_editor_new   (GtkWindow       *parent,
                                            Clay            *clay,
                                            ClayTextOptions *options,
                                            ClayMenuFactory *menu_factory,
                                            const gchar     *title,
                                            ClayText        *text,
                                            ClayTextBuffer  *text_buffer,
                                            gdouble          xres,
                                            gdouble          yres);
