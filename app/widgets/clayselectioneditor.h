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

#include "clayimageeditor.h"


#define CLAY_TYPE_SELECTION_EDITOR            (clay_selection_editor_get_type ())
#define CLAY_SELECTION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SELECTION_EDITOR, ClaySelectionEditor))
#define CLAY_SELECTION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SELECTION_EDITOR, ClaySelectionEditorClass))
#define CLAY_IS_SELECTION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SELECTION_EDITOR))
#define CLAY_IS_SELECTION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SELECTION_EDITOR))
#define CLAY_SELECTION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SELECTION_EDITOR, ClaySelectionEditorClass))


typedef struct _ClaySelectionEditorClass ClaySelectionEditorClass;

struct _ClaySelectionEditor
{
  ClayImageEditor  parent_instance;

  GtkWidget       *view;

  GtkWidget       *all_button;
  GtkWidget       *none_button;
  GtkWidget       *invert_button;
  GtkWidget       *save_button;
  GtkWidget       *path_button;
  GtkWidget       *stroke_button;
};

struct _ClaySelectionEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_selection_editor_get_type (void);

GtkWidget * clay_selection_editor_new      (ClayMenuFactory *menu_factory);
