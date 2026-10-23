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


#define CLAY_TYPE_UNDO_EDITOR            (clay_undo_editor_get_type ())
#define CLAY_UNDO_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_UNDO_EDITOR, ClayUndoEditor))
#define CLAY_UNDO_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_UNDO_EDITOR, ClayUndoEditorClass))
#define CLAY_IS_UNDO_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_UNDO_EDITOR))
#define CLAY_IS_UNDO_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_UNDO_EDITOR))
#define CLAY_UNDO_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_UNDO_EDITOR, ClayUndoEditorClass))


typedef struct _ClayUndoEditorClass ClayUndoEditorClass;

struct _ClayUndoEditor
{
  ClayImageEditor  parent_instance;

  ClayContext     *context;
  ClayContainer   *container;
  GtkWidget       *view;
  ClayViewSize     view_size;

  ClayUndo        *base_item;

  GtkWidget       *undo_button;
  GtkWidget       *redo_button;
  GtkWidget       *clear_button;
};

struct _ClayUndoEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_undo_editor_get_type  (void);

GtkWidget * clay_undo_editor_new       (ClayCoreConfig  *config,
                                        ClayMenuFactory *menu_factory);
