/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The Clay Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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

#include <string.h>  /*  strlen  */

#include <glib.h>

#include "libclay/clay.h"

#include "clayhelp.h"

#include "libclay/stdplugins-intl.h"


/*  defines  */

#define CLAY_HELP_EXT_PROC        "extension-clay-help"
#define CLAY_HELP_TEMP_EXT_PROC   "extension-clay-help-temp"


typedef struct _Help      Help;
typedef struct _HelpClass HelpClass;

struct _Help
{
  ClayPlugIn parent_instance;
};

struct _HelpClass
{
  ClayPlugInClass parent_class;
};


/*  forward declarations  */

#define HELP_TYPE  (help_get_type ())
#define HELP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HELP_TYPE, Help))

GType                   help_get_type          (void);

static GList          * help_query_procedures  (ClayPlugIn           *plug_in);
static ClayProcedure  * help_create_procedure  (ClayPlugIn           *plug_in,
                                                const gchar          *name);

static ClayValueArray * help_run               (ClayProcedure        *procedure,
                                                ClayProcedureConfig  *config,
                                                gpointer              run_data);
static ClayValueArray * help_temp_run          (ClayProcedure        *procedure,
                                                ClayProcedureConfig  *config,
                                                gpointer              run_data);

static void             help_temp_proc_install (ClayPlugIn           *plug_in);
static void             help_load              (const gchar          *help_domain,
                                                const gchar          *help_locales,
                                                const gchar          *help_id);

static ClayHelpProgress * help_load_progress_new (void);


static GMainLoop *main_loop = NULL;

G_DEFINE_TYPE (Help, help, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (HELP_TYPE)
DEFINE_STD_SET_I18N


static void
help_class_init (HelpClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = help_query_procedures;
  plug_in_class->create_procedure = help_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
help_init (Help *help)
{
}

static GList *
help_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (CLAY_HELP_EXT_PROC));
}

static ClayProcedure *
help_create_procedure (ClayPlugIn  *plug_in,
                       const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, CLAY_HELP_EXT_PROC))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PERSISTENT,
                                      help_run, NULL, NULL);

      clay_procedure_set_attribution (procedure,
                                      "Sven Neumann <sven@gimp.org>, "
                                      "Michael Natterer <mitch@gimp.org>, "
                                      "Henrik Brix Andersen <brix@gimp.org>",
                                      "Sven Neumann, Michael Natterer & Henrik Brix Andersen",
                                      "1999-2008");

      clay_procedure_add_string_array_argument (procedure, "domain-names",
                                                "Domain Names",
                                                "Domain names",
                                                G_PARAM_READWRITE);

      clay_procedure_add_string_array_argument (procedure, "domain-uris",
                                                "Domain URIs",
                                                "Domain URIs",
                                                G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
help_run (ClayProcedure        *procedure,
          ClayProcedureConfig  *config,
          gpointer              run_data)
{
  ClayPDBStatusType   status       = CLAY_PDB_SUCCESS;
  gchar             **domain_names = NULL;
  gchar             **domain_uris  = NULL;

  g_object_get (config,
                "domain-names", &domain_names,
                "domain-uris",  &domain_uris,
                NULL);
  if (! clay_help_init ((const gchar **) domain_names,
                        (const gchar **) domain_uris))
    status = CLAY_PDB_CALLING_ERROR;

  g_strfreev (domain_names);
  g_strfreev (domain_uris);

  if (status == CLAY_PDB_SUCCESS)
    {
      ClayPlugIn *plug_in = clay_procedure_get_plug_in (procedure);

      main_loop = g_main_loop_new (NULL, FALSE);

      help_temp_proc_install (plug_in);

      clay_procedure_persistent_ready (procedure);
      clay_plug_in_persistent_enable (plug_in);

      g_main_loop_run (main_loop);

      g_main_loop_unref (main_loop);
      main_loop = NULL;

      clay_plug_in_remove_temp_procedure (plug_in, CLAY_HELP_TEMP_EXT_PROC);
    }

  return clay_procedure_new_return_values (procedure, status, NULL);
}

static void
help_temp_proc_install (ClayPlugIn *plug_in)
{
  ClayProcedure *procedure;

  procedure = clay_procedure_new (plug_in, CLAY_HELP_TEMP_EXT_PROC,
                                  CLAY_PDB_PROC_TYPE_TEMPORARY,
                                  help_temp_run, NULL, NULL);

  clay_procedure_set_attribution (procedure,
                                  "Sven Neumann <sven@gimp.org>, "
                                  "Michael Natterer <mitch@gimp.org>"
                                  "Henrik Brix Andersen <brix@gimp.org",
                                  "Sven Neumann, Michael Natterer & "
                                  "Henrik Brix Andersen",
                                  "1999-2008");

  clay_procedure_add_string_argument (procedure, "help-proc",
                                      "The procedure of the browser to use",
                                      "The procedure of the browser to use. "
                                      "This argument is now ignored and considered deprecated.",
                                      NULL,
                                      G_PARAM_READWRITE);

  clay_procedure_add_string_argument (procedure, "help-domain",
                                      "Help domain to use",
                                      "Help domain to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  clay_procedure_add_string_argument (procedure, "help-locales",
                                      "Language to use",
                                      "Language to use",
                                      NULL,
                                      G_PARAM_READWRITE);

  clay_procedure_add_string_argument (procedure, "help-id",
                                      "Help ID to open",
                                      "Help ID to open",
                                      NULL,
                                      G_PARAM_READWRITE);

  clay_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
}

static ClayValueArray *
help_temp_run (ClayProcedure        *procedure,
               ClayProcedureConfig  *config,
               gpointer              run_data)
{
  ClayPDBStatusType  status       = CLAY_PDB_SUCCESS;
  gchar             *help_domain  = NULL;
  gchar             *help_locales = NULL;
  gchar             *help_id      = NULL;

  g_object_get (config,
                "help-domain",  &help_domain,
                "help-locales", &help_locales,
                "help-id",      &help_id,
                NULL);

  if (help_domain == NULL)
    help_domain = g_strdup (CLAY_HELP_DEFAULT_DOMAIN);

  if (help_id == NULL)
    help_id = g_strdup (CLAY_HELP_DEFAULT_ID);

  if (status == CLAY_PDB_SUCCESS)
    help_load (help_domain, help_locales, help_id);

  g_free (help_domain);
  g_free (help_locales);
  g_free (help_id);

  return clay_procedure_new_return_values (procedure, status, NULL);
}

static void
help_load (const gchar *help_domain,
           const gchar *help_locales,
           const gchar *help_id)
{
  ClayHelpDomain *domain;

  domain = clay_help_lookup_domain (help_domain);
  if (domain)
    {
      ClayHelpProgress *progress = NULL;
      GList            *locales;
      gchar            *uri;
      gboolean          fatal_error;

      locales = clay_help_parse_locales (help_locales);

      if (! g_str_has_prefix (domain->help_uri, "file:"))
        progress = help_load_progress_new ();

      uri = clay_help_domain_map (domain, locales, help_id,
                                  progress, NULL, &fatal_error);

      if (progress)
        clay_help_progress_free (progress);

      g_list_free_full (locales, (GDestroyNotify) g_free);

      if (uri)
        {
          g_app_info_launch_default_for_uri (uri, NULL, NULL);
          g_free (uri);
        }
      else if (fatal_error)
        {
          g_main_loop_quit (main_loop);
        }
    }
}

static void
help_load_progress_start (const gchar *message,
                          gboolean     cancelable,
                          gpointer     user_data)
{
  clay_progress_init (message);
}

static void
help_load_progress_update (gdouble  value,
                           gpointer user_data)
{
  clay_progress_update (value);
}

static void
help_load_progress_end (gpointer user_data)
{
  clay_progress_end ();
}

static ClayHelpProgress *
help_load_progress_new (void)
{
  static const ClayHelpProgressVTable vtable =
  {
    help_load_progress_start,
    help_load_progress_end,
    help_load_progress_update
  };

  return clay_help_progress_new (&vtable, NULL);
}
