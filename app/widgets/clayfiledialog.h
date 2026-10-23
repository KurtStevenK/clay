/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfiledialog.h
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


#define CLAY_TYPE_FILE_DIALOG            (clay_file_dialog_get_type ())
#define CLAY_FILE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILE_DIALOG, ClayFileDialog))
#define CLAY_FILE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILE_DIALOG, ClayFileDialogClass))
#define CLAY_IS_FILE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILE_DIALOG))
#define CLAY_IS_FILE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FILE_DIALOG))
#define CLAY_FILE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FILE_DIALOG, ClayFileDialogClass))


typedef struct _ClayFileDialogClass  ClayFileDialogClass;

struct _ClayFileDialog
{
  GtkFileChooserDialog    parent_instance;

  GBytes                 *window_handle;

  Clay                   *clay;
  ClayImage              *image;

  ClayPlugInProcedure    *file_proc;

  GtkWidget              *thumb_box;
  GtkWidget              *extra_vbox;
  GtkWidget              *proc_expander;
  GtkWidget              *proc_view;
  GtkWidget              *progress;

  gboolean                busy;
  gboolean                canceled;

  gchar                  *help_id;
  gchar                  *ok_button_label;
  gchar                  *automatic_help_id;
  gchar                  *automatic_label;
  gchar                  *file_filter_label;

  ClayFileProcedureGroup  file_proc_group;
  GSList                 *file_procs;
  GSList                 *file_procs_all_images;

  gboolean                show_all_files;
};

struct _ClayFileDialogClass
{
  GtkFileChooserDialogClass  parent_class;

  GFile * (* get_default_folder) (ClayFileDialog *dialog);

  void    (* save_state)         (ClayFileDialog *dialog,
                                  const gchar    *state_name);
  void    (* load_state)         (ClayFileDialog *dialog,
                                  const gchar    *state_name);
};


GType   clay_file_dialog_get_type           (void);

void    clay_file_dialog_add_extra_widget   (ClayFileDialog      *dialog,
                                             GtkWidget           *widget,
                                             gboolean             expand,
                                             gboolean             fill,
                                             guint                padding);

void    clay_file_dialog_set_sensitive      (ClayFileDialog      *dialog,
                                             gboolean             sensitive);

void    clay_file_dialog_set_file_proc      (ClayFileDialog      *dialog,
                                             ClayPlugInProcedure *file_proc);

GFile * clay_file_dialog_get_default_folder (ClayFileDialog      *dialog);

void    clay_file_dialog_save_state         (ClayFileDialog      *dialog,
                                             const gchar         *state_name);
void    clay_file_dialog_load_state         (ClayFileDialog      *dialog,
                                             const gchar         *state_name);
