/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycoloreditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include "clayeditor.h"


#define CLAY_TYPE_COLOR_EDITOR            (clay_color_editor_get_type ())
#define CLAY_COLOR_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_EDITOR, ClayColorEditor))
#define CLAY_COLOR_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_EDITOR, ClayColorEditorClass))
#define CLAY_IS_COLOR_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_EDITOR))
#define CLAY_IS_COLOR_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_EDITOR))
#define CLAY_COLOR_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_EDITOR, ClayColorEditorClass))


typedef struct _ClayColorEditorClass ClayColorEditorClass;

struct _ClayColorEditor
{
  ClayEditor        parent_instance;

  ClayContext      *context;
  ClayDisplay      *active_display;
  ClayDisplayShell *active_shell;
  ClayImage        *active_image;
  gboolean          edit_bg;

  GtkWidget        *hbox;
  GtkWidget        *notebook;
  GtkWidget        *fg_bg;
  GtkWidget        *hex_entry;
};

struct _ClayColorEditorClass
{
  ClayEditorClass  parent_class;
};


GType       clay_color_editor_get_type (void);

GtkWidget * clay_color_editor_new      (ClayContext *context);
