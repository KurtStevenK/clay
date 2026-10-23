/*
 * pat plug-in version 1.01
 * Loads/exports version 1 Clay .pat files, by Tim Newsome <drz@frody.bloke.com>
 *
 * xyz.kainzmayer.ClayDesign
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

#include "config.h"

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define EXPORT_PROC    "file-pat-export"
#define PLUG_IN_BINARY "file-pat"


typedef struct _Pat      Pat;
typedef struct _PatClass PatClass;

struct _Pat
{
  ClayPlugIn      parent_instance;
};

struct _PatClass
{
  ClayPlugInClass parent_class;
};


#define PAT_TYPE  (pat_get_type ())
#define PAT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PAT_TYPE, Pat))

GType                   pat_get_type         (void);

static GList          * pat_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * pat_create_procedure (ClayPlugIn           *plug_in,
                                              const gchar          *name);

static ClayValueArray * pat_export           (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              ClayImage            *image,
                                              GFile                *file,
                                              ClayExportOptions    *options,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);

static gboolean         save_dialog          (ClayImage            *image,
                                              ClayProcedure        *procedure,
                                              GObject              *config);


G_DEFINE_TYPE (Pat, pat, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (PAT_TYPE)
DEFINE_STD_SET_I18N


static void
pat_class_init (PatClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = pat_query_procedures;
  plug_in_class->create_procedure = pat_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
pat_init (Pat *pat)
{
}

static GList *
pat_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static ClayProcedure *
pat_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, pat_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Clay pattern"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_PATTERN);

      clay_procedure_set_documentation (procedure,
                                        _("Exports Clay pattern file (.PAT)"),
                                        _("New Clay patterns can be created "
                                          "by exporting them in the "
                                          "appropriate place with this plug-in."),
                                        EXPORT_PROC);
      clay_procedure_set_attribution (procedure,
                                      "Tim Newsome",
                                      "Tim Newsome",
                                      "1997");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("Pattern"));
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-clay-pat");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "pat");
      clay_file_procedure_set_handles_remote (CLAY_FILE_PROCEDURE (procedure),
                                              TRUE);

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      clay_procedure_add_string_argument (procedure, "description",
                                          _("_Description"),
                                          _("Short description of the pattern"),
                                          _("Clay Pattern"),
                                          CLAY_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
pat_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;
  ClayExportReturn   export = CLAY_EXPORT_IGNORE;
  ClayLayer        **layers;
  gchar             *description;
  GError            *error  = NULL;

  g_object_get (config,
                "description", &description,
                NULL);

  if (! description || ! strlen (description))
    {
      gchar *name = g_path_get_basename (clay_file_get_utf8_name (file));

      if (g_str_has_suffix (name, ".pat"))
        name[strlen (name) - 4] = '\0';

      if (strlen (name))
        g_object_set (config,
                      "description", name,
                      NULL);

      g_free (name);
    }

  g_free (description);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (image, procedure, G_OBJECT (config)))
        status = CLAY_PDB_CANCEL;
    }

  export = clay_export_options_get_image (options, &image);
  layers = clay_image_get_layers (image);

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayProcedure   *procedure;
      ClayValueArray  *save_retvals;

      g_object_get (config,
                    "description", &description,
                    NULL);

      procedure    = clay_pdb_lookup_procedure (clay_get_pdb (),
                                                "file-pat-export-internal");
      save_retvals = clay_procedure_run (procedure,
                                         "image",     image,
                                         "drawables", (ClayDrawable **) layers,
                                         "file",      file,
                                         "name",      description,
                                         NULL);

      if (CLAY_VALUES_GET_ENUM (save_retvals, 0) != CLAY_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-pat-export-internal' "
                       "failed: %s",
                       clay_pdb_get_last_error (clay_get_pdb ()));

          status = CLAY_PDB_EXECUTION_ERROR;
        }

      clay_value_array_unref (save_retvals);
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_free (layers);

  return clay_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_dialog (ClayImage     *image,
             ClayProcedure *procedure,
             GObject       *config)
{
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *real_entry;
  gboolean   run;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  entry = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                            "description", CLAY_TYPE_LABEL_ENTRY);
  real_entry = clay_label_entry_get_entry (CLAY_LABEL_ENTRY (entry));
  gtk_entry_set_max_length (GTK_ENTRY (real_entry), 256);
  gtk_entry_set_width_chars (GTK_ENTRY (real_entry), 20);
  gtk_entry_set_activates_default (GTK_ENTRY (real_entry), TRUE);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
