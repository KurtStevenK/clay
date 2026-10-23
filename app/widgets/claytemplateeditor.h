/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytemplateeditor.h
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


#define CLAY_TYPE_TEMPLATE_EDITOR            (clay_template_editor_get_type ())
#define CLAY_TEMPLATE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEMPLATE_EDITOR, ClayTemplateEditor))
#define CLAY_TEMPLATE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEMPLATE_EDITOR, ClayTemplateEditorClass))
#define CLAY_IS_TEMPLATE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEMPLATE_EDITOR))
#define CLAY_IS_TEMPLATE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEMPLATE_EDITOR))
#define CLAY_TEMPLATE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEMPLATE_EDITOR, ClayTemplateEditorClass))


typedef struct _ClayTemplateEditorClass ClayTemplateEditorClass;

struct _ClayTemplateEditor
{
  GtkBox  parent_instance;
};

struct _ClayTemplateEditorClass
{
  GtkBoxClass   parent_class;
};


GType          clay_template_editor_get_type      (void);

GtkWidget    * clay_template_editor_new           (ClayTemplate       *template,
                                                   Clay               *clay,
                                                   gboolean            edit_template);

ClayTemplate * clay_template_editor_get_template  (ClayTemplateEditor *editor);

void           clay_template_editor_show_advanced (ClayTemplateEditor *editor,
                                                   gboolean            expanded);
GtkWidget    * clay_template_editor_get_size_se   (ClayTemplateEditor *editor);
GtkWidget    * clay_template_editor_get_resolution_se
                                                  (ClayTemplateEditor *editor);
GtkWidget    * clay_template_editor_get_resolution_chain
                                                  (ClayTemplateEditor *editor);
