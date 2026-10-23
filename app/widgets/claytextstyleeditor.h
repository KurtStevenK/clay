/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextStyleEditor
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_TEXT_STYLE_EDITOR            (clay_text_style_editor_get_type ())
#define CLAY_TEXT_STYLE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_STYLE_EDITOR, ClayTextStyleEditor))
#define CLAY_TEXT_STYLE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_STYLE_EDITOR, ClayTextStyleEditorClass))
#define CLAY_IS_TEXT_STYLE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_STYLE_EDITOR))
#define CLAY_IS_TEXT_STYLE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_STYLE_EDITOR))
#define CLAY_TEXT_STYLE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEXT_STYLE_EDITOR, ClayTextStyleEditorClass))


typedef struct _ClayTextStyleEditorClass ClayTextStyleEditorClass;

struct _ClayTextStyleEditor
{
  GtkBox          parent_instance;

  Clay           *clay;
  ClayContext    *context;

  ClayText       *text; /* read-only for default values */
  ClayTextBuffer *buffer;

  ClayContainer  *fonts;
  gdouble         resolution_x;
  gdouble         resolution_y;

  GtkWidget      *dnd_handle;

  GtkWidget      *upper_hbox;
  GtkWidget      *lower_hbox;
  GtkWidget      *restore_position_button;

  GtkWidget      *font_entry;
  GtkWidget      *size_entry;

  GtkWidget      *color_button;

  GtkWidget      *clear_button;

  GtkWidget      *baseline_spinbutton;
  GtkAdjustment  *baseline_adjustment;

  GtkWidget      *kerning_spinbutton;
  GtkAdjustment  *kerning_adjustment;

  GList          *toggles;

  guint           update_idle_id;
};

struct _ClayTextStyleEditorClass
{
  GtkBoxClass  parent_class;
};


GType       clay_text_style_editor_get_type  (void);

GtkWidget * clay_text_style_editor_new       (Clay                 *clay,
                                              ClayText             *text,
                                              ClayTextBuffer       *buffer,
                                              ClayContainer        *fonts,
                                              gdouble               resolution_x,
                                              gdouble               resolution_y);

GList     * clay_text_style_editor_list_tags (ClayTextStyleEditor  *editor,
                                              GList               **remove_tags);

void        clay_text_style_show_restore_position_button
                                             (ClayTextStyleEditor  *editor,
                                              gboolean              show);
