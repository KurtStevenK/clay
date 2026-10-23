/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextEditor
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


#define CLAY_TYPE_TEXT_EDITOR    (clay_text_editor_get_type ())
#define CLAY_TEXT_EDITOR(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_EDITOR, ClayTextEditor))
#define CLAY_IS_TEXT_EDITOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_EDITOR))


typedef struct _ClayTextEditorClass  ClayTextEditorClass;

struct _ClayTextEditor
{
  ClayDialog         parent_instance;

  /*<  private  >*/
  ClayTextDirection     base_dir;
  gchar                *font_name;

  GtkWidget            *view;
  GtkWidget            *font_toggle;
  GtkFileChooserNative *file_dialog;
  ClayUIManager        *ui_manager;
  GtkCssProvider       *font_css;
};

struct _ClayTextEditorClass
{
  ClayDialogClass   parent_class;

  void (* text_changed) (ClayTextEditor *editor);
  void (* dir_changed)  (ClayTextEditor *editor);
};


GType               clay_text_editor_get_type      (void);
GtkWidget         * clay_text_editor_new           (const gchar       *title,
                                                    GtkWindow         *parent,
                                                    Clay              *clay,
                                                    ClayMenuFactory   *menu_factory,
                                                    ClayText          *text,
                                                    ClayTextBuffer    *text_buffer,
                                                    gdouble            xres,
                                                    gdouble            yres);

void                clay_text_editor_set_text      (ClayTextEditor    *editor,
                                                    const gchar       *text,
                                                    gint               len);
gchar             * clay_text_editor_get_text      (ClayTextEditor    *editor);

void                clay_text_editor_set_direction (ClayTextEditor    *editor,
                                                    ClayTextDirection  base_dir);
ClayTextDirection   clay_text_editor_get_direction (ClayTextEditor    *editor);

void                clay_text_editor_set_font_name (ClayTextEditor    *editor,
                                                    const gchar       *font_name);
const gchar       * clay_text_editor_get_font_name (ClayTextEditor    *editor);
