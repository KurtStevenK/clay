/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycolordisplayeditor.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_COLOR_DISPLAY_EDITOR            (clay_color_display_editor_get_type ())
#define CLAY_COLOR_DISPLAY_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_DISPLAY_EDITOR, ClayColorDisplayEditor))
#define CLAY_COLOR_DISPLAY_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_DISPLAY_EDITOR, ClayColorDisplayEditorClass))
#define CLAY_IS_COLOR_DISPLAY_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_DISPLAY_EDITOR))
#define CLAY_IS_COLOR_DISPLAY_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_DISPLAY_EDITOR))
#define CLAY_COLOR_DISPLAY_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_DISPLAY_EDITOR, ClayColorDisplayEditorClass))


typedef struct _ClayColorDisplayEditorClass  ClayColorDisplayEditorClass;

struct _ClayColorDisplayEditor
{
  GtkBox                 parent_instance;

  Clay                  *clay;
  ClayColorDisplayStack *stack;
  ClayColorConfig       *config;
  ClayColorManaged      *managed;

  GtkWidget             *src_list;
  GtkWidget             *dest_list;
  ClayColorDisplay      *selected;

  GtkWidget             *add_button;
  GtkWidget             *remove_button;
  GtkWidget             *up_button;
  GtkWidget             *down_button;

  GtkWidget             *config_frame;
  GtkWidget             *config_box;
  GtkWidget             *config_widget;

  GtkWidget             *reset_button;
};

struct _ClayColorDisplayEditorClass
{
  GtkBoxClass parent_class;
};


GType       clay_color_display_editor_get_type (void);

GtkWidget * clay_color_display_editor_new      (Clay                  *clay,
                                                ClayColorDisplayStack *stack,
                                                ClayColorConfig       *config,
                                                ClayColorManaged      *managed);
