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
 * dbbrowser
 * 0.08 26th sept 97  by Thomas NOEL <thomas@minet.net>
 */

/*
 * This plugin gives you the list of available procedure, with the
 * name, description and parameters for each procedure.
 * You can do regexp search (by name and by description)
 * Useful for scripts development.
 *
 * NOTE :
 * this is only a exercice for me (my first "plug-in" (extension))
 * so it's very (very) dirty.
 * Btw, hope it gives you some ideas about clay possibilities.
 *
 * The core of the plugin is not here. See dbbrowser_utils (shared
 * with script-fu-console).
 *
 * TODO
 * - bug fixes... (my method : rewrite from scratch :)
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-dbbrowser"
#define PLUG_IN_BINARY "procedure-browser"
#define PLUG_IN_ROLE   "clay-procedure-browser"


typedef struct _Browser      Browser;
typedef struct _BrowserClass BrowserClass;

struct _Browser
{
  ClayPlugIn parent_instance;
};

struct _BrowserClass
{
  ClayPlugInClass parent_class;
};


/* Declare local functions.
 */

#define BROWSER_TYPE  (browser_get_type ())
#define BROWSER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BROWSER_TYPE, Browser))

GType                   browser_get_type         (void);

static GList          * browser_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * browser_create_procedure (ClayPlugIn           *plug_in,
                                                  const gchar          *name);

static ClayValueArray * browser_run              (ClayProcedure        *procedure,
                                                  ClayProcedureConfig  *config,
                                                  gpointer              run_data);


G_DEFINE_TYPE (Browser, browser, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (BROWSER_TYPE)
DEFINE_STD_SET_I18N


static void
browser_class_init (BrowserClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = browser_query_procedures;
  plug_in_class->create_procedure = browser_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
browser_init (Browser *browser)
{
}

static GList *
browser_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static ClayProcedure *
browser_create_procedure (ClayPlugIn  *plug_in,
                          const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PLUGIN,
                                      browser_run, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Procedure _Browser"));
      clay_procedure_add_menu_path (procedure, "<Image>/Help/[Programming]");

      clay_procedure_set_documentation (procedure,
                                        _("List available procedures in the PDB"),
                                        NULL,
                                        PLUG_IN_PROC);
      clay_procedure_set_attribution (procedure,
                                      "Thomas Noel",
                                      "Thomas Noel",
                                      "23th june 1997");

      clay_procedure_add_enum_argument (procedure, "run-mode",
                                        "Run mode",
                                        "The run mode",
                                        CLAY_TYPE_RUN_MODE,
                                        CLAY_RUN_INTERACTIVE,
                                        G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
browser_run (ClayProcedure        *procedure,
             ClayProcedureConfig  *config,
             gpointer              run_data)
{
  ClayRunMode run_mode;

  g_object_get (config, "run-mode", &run_mode, NULL);
  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      {
        GtkWidget *dialog;

        clay_ui_init (PLUG_IN_BINARY);

        dialog =
          clay_proc_browser_dialog_new (_("Procedure Browser"), PLUG_IN_BINARY,
                                        clay_standard_help_func, PLUG_IN_PROC,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

          clay_window_set_transient (GTK_WINDOW (dialog));

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
      }
      break;

    case CLAY_RUN_WITH_LAST_VALS:
    case CLAY_RUN_NONINTERACTIVE:
        {
          GError *error = NULL;

          g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                       _("Procedure %s allows only interactive invocation."),
                       clay_procedure_get_name (procedure));

          return clay_procedure_new_return_values (procedure,
                                                   CLAY_PDB_CALLING_ERROR,
                                                   error);
        }
      break;

    default:
      break;
    }

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}
