/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginprocedure.h
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

#include "pdb/clayprocedure.h"


#define CLAY_TYPE_PLUG_IN_PROCEDURE            (clay_plug_in_procedure_get_type ())
#define CLAY_PLUG_IN_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUG_IN_PROCEDURE, ClayPlugInProcedure))
#define CLAY_PLUG_IN_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUG_IN_PROCEDURE, ClayPlugInProcedureClass))
#define CLAY_IS_PLUG_IN_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUG_IN_PROCEDURE))
#define CLAY_IS_PLUG_IN_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUG_IN_PROCEDURE))
#define CLAY_PLUG_IN_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PLUG_IN_PROCEDURE, ClayPlugInProcedureClass))


typedef struct _ClayPlugInProcedureClass ClayPlugInProcedureClass;

struct _ClayPlugInProcedure
{
  ClayProcedure        parent_instance;

  /*  common members  */
  GFile               *file;
  GFile               *root_folder;

  GQuark               help_domain;
  gchar               *help_uri;
  GFile               *help_file;

  gchar               *menu_label;
  GList               *menu_paths;
  gchar               *help_id_with_domain;
  ClayIconType         icon_type;
  gint                 icon_data_length;
  guint8              *icon_data;
  gchar               *image_types;
  ClayPlugInImageType  image_types_val;
  gchar               *insensitive_reason;
  gint                 sensitivity_mask;
  gint64               mtime;
  gboolean             installed_during_init;

  /*  file proc specific members  */
  gboolean             file_proc;
  gboolean             generic_file_proc; /* not returning an image. */
  gchar               *extensions;
  gchar               *meta_extensions;
  gchar               *prefixes;
  gchar               *magics;
  gint                 priority;
  gchar               *mime_types;
  gboolean             handles_remote;
  gboolean             handles_raw;
  gboolean             handles_vector;
  gboolean             batch_interpreter;
  gchar               *batch_interpreter_name;
  GSList              *extensions_list;
  GSList              *meta_extensions_list;
  GSList              *prefixes_list;
  GSList              *magics_list;
  GSList              *mime_types_list;
  gchar               *thumb_loader;
};

struct _ClayPlugInProcedureClass
{
  ClayProcedureClass parent_class;

  /*  virtual functions  */
  GFile * (* get_file)        (ClayPlugInProcedure *procedure);

  /*  signals  */
  void    (* menu_path_added) (ClayPlugInProcedure *procedure,
                               const gchar         *menu_path);
};


GType           clay_plug_in_procedure_get_type            (void);

ClayProcedure * clay_plug_in_procedure_new                 (ClayPDBProcType      proc_type,
                                                            GFile               *file,
                                                            GFile               *root_folder);

ClayPlugInProcedure * clay_plug_in_procedure_find          (GSList              *list,
                                                            const gchar         *proc_name);

GFile       * clay_plug_in_procedure_get_file              (ClayPlugInProcedure *proc);
GFile       * clay_plug_in_procedure_get_root_folder       (ClayPlugInProcedure *proc);

void          clay_plug_in_procedure_set_help_domain       (ClayPlugInProcedure *proc,
                                                            const gchar         *help_domain);
const gchar * clay_plug_in_procedure_get_help_domain       (ClayPlugInProcedure *proc);

gboolean      clay_plug_in_procedure_set_help_uri          (ClayPlugInProcedure *proc,
                                                            const gchar         *help_uri,
                                                            GError             **error);

gboolean      clay_plug_in_procedure_set_menu_label        (ClayPlugInProcedure *proc,
                                                            const gchar         *menu_label,
                                                            GError             **error);

gboolean      clay_plug_in_procedure_add_menu_path         (ClayPlugInProcedure *proc,
                                                            const gchar         *menu_path,
                                                            GError             **error);

gboolean      clay_plug_in_procedure_set_icon              (ClayPlugInProcedure *proc,
                                                            ClayIconType         type,
                                                            const guint8        *data,
                                                            gint                 data_length,
                                                            GError             **error);
gboolean      clay_plug_in_procedure_take_icon             (ClayPlugInProcedure *proc,
                                                            ClayIconType         type,
                                                            guint8              *data,
                                                            gint                 data_length,
                                                            GError             **error);

void          clay_plug_in_procedure_set_image_types       (ClayPlugInProcedure *proc,
                                                            const gchar         *image_types);
void          clay_plug_in_procedure_set_sensitivity_mask  (ClayPlugInProcedure *proc,
                                                            gint                 sensitivity_mask);

void          clay_plug_in_procedure_set_file_proc         (ClayPlugInProcedure *proc,
                                                            const gchar         *extensions,
                                                            const gchar         *meta_extensions,
                                                            const gchar         *prefixes,
                                                            const gchar         *magics);
gchar       * clay_plug_in_procedure_get_save_extensions   (ClayPlugInProcedure *proc,
                                                            gboolean             one_full);
gchar       * clay_plug_in_procedure_get_export_extensions (ClayPlugInProcedure *proc,
                                                            gboolean             one_full);
gchar       * clay_plug_in_procedure_get_open_extensions   (ClayPlugInProcedure *proc);

void          clay_plug_in_procedure_set_generic_file_proc (ClayPlugInProcedure *proc,
                                                            gboolean             is_generic_file_proc);
void          clay_plug_in_procedure_set_priority          (ClayPlugInProcedure *proc,
                                                            gint                 priority);
void          clay_plug_in_procedure_set_mime_types        (ClayPlugInProcedure *proc,
                                                            const gchar         *mime_ypes);
void          clay_plug_in_procedure_set_handles_remote    (ClayPlugInProcedure *proc);
void          clay_plug_in_procedure_set_handles_raw       (ClayPlugInProcedure *proc);
void          clay_plug_in_procedure_set_handles_vector    (ClayPlugInProcedure *proc);
void          clay_plug_in_procedure_set_thumb_loader      (ClayPlugInProcedure *proc,
                                                            const gchar         *thumbnailer);
void          clay_plug_in_procedure_set_batch_interpreter (ClayPlugInProcedure *proc,
                                                            const gchar         *name);

void          clay_plug_in_procedure_handle_return_values  (ClayPlugInProcedure *proc,
                                                            Clay                *clay,
                                                            ClayProgress        *progress,
                                                            ClayValueArray      *return_vals);

gboolean      clay_plug_in_procedure_is_xcf_load           (ClayPlugInProcedure *file_proc);
