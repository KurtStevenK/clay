/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "core/clayobject.h"


#define CLAY_TYPE_TEXT            (clay_text_get_type ())
#define CLAY_TEXT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT, ClayText))
#define CLAY_TEXT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT, ClayTextClass))
#define CLAY_IS_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT))
#define CLAY_IS_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT))
#define CLAY_TEXT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEXT, ClayTextClass))


typedef struct _ClayTextClass  ClayTextClass;

struct _ClayText
{
  ClayObject                parent_instance;

  gchar                    *text;
  gchar                    *markup;
  ClayFont                 *font;
  ClayUnit                 *unit;
  gdouble                   font_size;
  gboolean                  antialias;
  ClayTextHintStyle         hint_style;
  gboolean                  kerning;
  gchar                    *language;
  ClayTextDirection         base_dir;
  GeglColor                *color;
  ClayCustomStyle           outline_style;
  ClayPattern              *outline_pattern;
  GeglColor                *outline_foreground;
  gdouble                   outline_width;
  ClayUnit                 *outline_unit;
  ClayTextOutlineDirection  outline_direction;
  ClayCapStyle              outline_cap_style;
  ClayJoinStyle             outline_join_style;
  gdouble                   outline_miter_limit;
  gboolean                  outline_antialias;
  gdouble                   outline_dash_offset;
  GArray                   *outline_dash_info;
  ClayTextOutline           outline;
  ClayTextJustification     justify;
  gdouble                   indent;
  gdouble                   line_spacing;
  gdouble                   letter_spacing;
  ClayTextBoxMode           box_mode;
  gdouble                   box_width;
  gdouble                   box_height;
  ClayUnit                 *box_unit;
  ClayMatrix2               transformation;
  gdouble                   offset_x;
  gdouble                   offset_y;

  gdouble                   border;
  Clay                     *clay;
};

struct _ClayTextClass
{
  ClayObjectClass        parent_class;

  void (* changed) (ClayText *text);
};


GType  clay_text_get_type           (void);

void   clay_text_get_transformation (ClayText    *text,
                                     ClayMatrix3 *matrix);
