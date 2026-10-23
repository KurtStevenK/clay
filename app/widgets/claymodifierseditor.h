/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymodifierseditor.h
 * Copyright (C) 2022 Jehan
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


#define CLAY_TYPE_MODIFIERS_EDITOR            (clay_modifiers_editor_get_type ())
#define CLAY_MODIFIERS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MODIFIERS_EDITOR, ClayModifiersEditor))
#define CLAY_MODIFIERS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MODIFIERS_EDITOR, ClayModifiersEditorClass))
#define CLAY_IS_MODIFIERS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MODIFIERS_EDITOR))
#define CLAY_IS_MODIFIERS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MODIFIERS_EDITOR))
#define CLAY_MODIFIERS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MODIFIERS_EDITOR, ClayModifiersEditorClass))


typedef struct _ClayModifiersEditorPrivate ClayModifiersEditorPrivate;
typedef struct _ClayModifiersEditorClass   ClayModifiersEditorClass;

struct _ClayModifiersEditor
{
  ClayFrame                  parent_instance;

  ClayModifiersEditorPrivate *priv;
};

struct _ClayModifiersEditorClass
{
  ClayFrameClass             parent_class;
};


GType       clay_modifiers_editor_get_type (void);

GtkWidget * clay_modifiers_editor_new      (ClayModifiersManager *manager,
                                            Clay                 *clay);

void        clay_modifiers_editor_clear    (ClayModifiersEditor  *editor);
