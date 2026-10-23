/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydasheditor.h
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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


#define CLAY_TYPE_DASH_EDITOR            (clay_dash_editor_get_type ())
#define CLAY_DASH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DASH_EDITOR, ClayDashEditor))
#define CLAY_DASH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DASH_EDITOR, ClayDashEditorClass))
#define CLAY_IS_DASH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DASH_EDITOR))
#define CLAY_IS_DASH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DASH_EDITOR))
#define CLAY_DASH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DASH_EDITOR, ClayDashEditorClass))


typedef struct _ClayDashEditorClass  ClayDashEditorClass;

struct _ClayDashEditor
{
  GtkDrawingArea     parent_instance;

  ClayStrokeOptions *stroke_options;
  gdouble            dash_length;

  /* GUI stuff */
  gint               n_segments;
  gboolean          *segments;

  /* coordinates of the first block main dash pattern */
  gint               x0;
  gint               y0;
  gint               block_width;
  gint               block_height;

  gboolean           edit_mode;
  gint               edit_button_x0;
};

struct _ClayDashEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       clay_dash_editor_get_type    (void);

GtkWidget * clay_dash_editor_new         (ClayStrokeOptions *stroke_options);

void        clay_dash_editor_shift_left  (ClayDashEditor    *editor);
void        clay_dash_editor_shift_right (ClayDashEditor    *editor);
