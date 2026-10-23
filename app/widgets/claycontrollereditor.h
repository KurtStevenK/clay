/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontrollereditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CONTROLLER_EDITOR            (clay_controller_editor_get_type ())
#define CLAY_CONTROLLER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTROLLER_EDITOR, ClayControllerEditor))
#define CLAY_CONTROLLER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTROLLER_EDITOR, ClayControllerEditorClass))
#define CLAY_IS_CONTROLLER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTROLLER_EDITOR))
#define CLAY_IS_CONTROLLER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTROLLER_EDITOR))
#define CLAY_CONTROLLER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTROLLER_EDITOR, ClayControllerEditorClass))


typedef struct _ClayControllerEditorClass ClayControllerEditorClass;

struct _ClayControllerEditor
{
  GtkBox              parent_instance;

  ClayControllerInfo *info;
  ClayContext        *context;

  GtkTreeSelection   *sel;

  GtkWidget          *grab_button;
  GtkWidget          *edit_button;
  GtkWidget          *delete_button;

  GtkWidget          *edit_dialog;
  GtkTreeSelection   *edit_sel;
};

struct _ClayControllerEditorClass
{
  GtkBoxClass   parent_class;
};


GType       clay_controller_editor_get_type (void);

GtkWidget * clay_controller_editor_new      (ClayControllerInfo *info,
                                             ClayContext        *context);
