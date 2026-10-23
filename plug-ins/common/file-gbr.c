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

/*
 * gbr plug-in version 1.00
 * Loads/exports version 2 Clay .gbr files, by Tim Newsome <drz@frody.bloke.com>
 * Some bits stolen from the .99.7 source tree.
 *
 * Added in GBR version 1 support after learning that there wasn't a
 * tool to read them.
 * July 6, 1998 by Seth Burgess <sjburges@gimp.org>
 *
 * Dec 17, 2000
 * Load and save Clay brushes in GRAY or RGBA.  jtl + neo
 *
 *
 * TODO: Give some better error reporting on not opening files/bad headers
 *       etc.
 */

#include "config.h"

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define EXPORT_PROC    "file-gbr-export"
#define PLUG_IN_BINARY "file-gbr"


typedef struct _Gbr      Gbr;
typedef struct _GbrClass GbrClass;

struct _Gbr
{
  ClayPlugIn      parent_instance;
};

struct _GbrClass
{
  ClayPlugInClass parent_class;
};


#define GBR_TYPE  (gbr_get_type ())
#define GBR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GBR_TYPE, Gbr))

GType                   gbr_get_type         (void);

static GList          * gbr_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * gbr_create_procedure (ClayPlugIn           *plug_in,
                                              const gchar          *name);

static ClayValueArray * gbr_export           (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              ClayImage            *image,
                                              GFile                *file,
                                              ClayExportOptions    *options,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);

static gboolean         save_dialog          (ClayProcedure        *procedure,
                                              GObject              *config,
                                              ClayImage            *image);


G_DEFINE_TYPE (Gbr, gbr, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (GBR_TYPE)
DEFINE_STD_SET_I18N


static void
gbr_class_init (GbrClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = gbr_query_procedures;
  plug_in_class->create_procedure = gbr_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
gbr_init (Gbr *gbr)
{
}

static GList *
gbr_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (EXPORT_PROC));
}

static ClayProcedure *
gbr_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, gbr_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Clay brush"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("Brush"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in the Clay brush "
                                          "file format"),
                                        NULL, EXPORT_PROC);
      clay_procedure_set_attribution (procedure,
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "Tim Newsome, Jens Lautenbacher, "
                                      "Sven Neumann",
                                      "1997-2000");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-clay-gbr");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "gbr");
      clay_file_procedure_set_handles_remote (CLAY_FILE_PROCEDURE (procedure),
                                              TRUE);

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      clay_procedure_add_int_argument (procedure, "spacing",
                                       _("Sp_acing"),
                                       _("Spacing of the brush"),
                                       1, 1000, 10,
                                       CLAY_PARAM_READWRITE);

      clay_procedure_add_string_argument (procedure, "description",
                                          _("_Description"),
                                          _("Short description of the brush"),
                                          _("Clay Brush"),
                                          CLAY_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
gbr_export (ClayProcedure        *procedure,
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

      if (g_str_has_suffix (name, ".gbr"))
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

      if (! save_dialog (procedure, G_OBJECT (config), image))
        status = CLAY_PDB_CANCEL;
    }

  export = clay_export_options_get_image (options, &image);
  layers = clay_image_get_layers (image);

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayProcedure   *procedure;
      ClayValueArray  *save_retvals;
      gint             spacing;

      g_object_get (config,
                    "description", &description,
                    "spacing",     &spacing,
                    NULL);

      procedure    = clay_pdb_lookup_procedure (clay_get_pdb (),
                                                "file-gbr-export-internal");
      save_retvals = clay_procedure_run (procedure,
                                         "image",     image,
                                         "drawables", (ClayDrawable **) layers,
                                         "file",      file,
                                         "spacing",   spacing,
                                         "name",      description,
                                         NULL);

      g_free (description);

      if (CLAY_VALUES_GET_ENUM (save_retvals, 0) != CLAY_PDB_SUCCESS)
        {
          g_set_error (&error, 0, 0,
                       "Running procedure 'file-gbr-export-internal' "
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
save_dialog (ClayProcedure *procedure,
             GObject       *config,
             ClayImage     *image)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
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

  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                        "spacing", 1.0);

  vbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                         "gbr-vbox", "description", "spacing",
                                         NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "gbr-vbox", NULL);
  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
