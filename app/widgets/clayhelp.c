/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayhelp.c
 * Copyright (C) 1999-2004 Michael Natterer <mitch@gimp.org>
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

#include <string.h>
#ifdef _WIN32
#define strdup _strdup
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"
#include "core/clay-utils.h"

#include "pdb/claypdb.h"
#include "pdb/clayprocedure.h"

#include "plug-in/clayplugin.h"
#include "plug-in/claypluginmanager-help-domain.h"
#include "plug-in/claytemporaryprocedure.h"

#include "clayhelp.h"
#include "clayhelp-ids.h"
#include "claylanguagecombobox.h"
#include "claymessagebox.h"
#include "claymessagedialog.h"
#include "claymessagedialog.h"
#include "claywidgets-utils.h"

#include "clay-log.h"
#include "clay-intl.h"


typedef struct _ClayIdleHelp ClayIdleHelp;

struct _ClayIdleHelp
{
  Clay         *clay;
  ClayProgress *progress;
  gchar        *help_domain;
  gchar        *help_locales;
  gchar        *help_id;
};


/*  local function prototypes  */

static gboolean   clay_idle_help          (ClayIdleHelp  *idle_help);
static void       clay_idle_help_free     (ClayIdleHelp  *idle_help);

static void       clay_help_call          (Clay          *clay,
                                           ClayProgress  *progress,
                                           const gchar   *help_domain,
                                           const gchar   *help_locales,
                                           const gchar   *help_id);

static void       clay_help_get_help_domains         (Clay    *clay,
                                                      gchar ***domain_names,
                                                      gchar ***domain_uris);
static gchar    * clay_help_get_default_domain_uri   (Clay    *clay);
static gchar    * clay_help_get_locales              (Clay    *clay);

static GFile    * clay_help_get_user_manual_basedir  (void);

/*  public functions  */

void
clay_help_show (Clay         *clay,
                ClayProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_id)
{
  ClayGuiConfig *config;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  config = CLAY_GUI_CONFIG (clay->config);

  if (config->use_help)
    {
      ClayIdleHelp *idle_help = g_slice_new0 (ClayIdleHelp);

      idle_help->clay     = clay;
      idle_help->progress = progress;

      if (help_domain && strlen (help_domain))
        idle_help->help_domain = g_strdup (help_domain);

      idle_help->help_locales = clay_help_get_locales (clay);

      if (help_id && strlen (help_id))
        idle_help->help_id = g_strdup (help_id);

      CLAY_LOG (HELP, "request for help-id '%s' from help-domain '%s'",
                help_id     ? help_id      : "(null)",
                help_domain ? help_domain  : "(null)");

      g_idle_add ((GSourceFunc) clay_idle_help, idle_help);
    }
}

gboolean
clay_help_user_manual_is_installed (Clay *clay)
{
  GFile    *basedir;
  gboolean  found = FALSE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  /*  if CLAY2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("CLAY2_HELP_URI"))
    return TRUE;

  basedir = clay_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gchar       *locales = clay_help_get_locales (clay);
      const gchar *s       = locales;
      const gchar *p;

      for (p = strchr (s, ':'); p && !found; p = strchr (s, ':'))
        {
          gchar *locale = g_strndup (s, p - s);
          GFile *file1  = g_file_get_child (basedir, locale);
          GFile *file2  = g_file_get_child (file1, "clay-help.xml");

          found = (g_file_query_file_type (file2, G_FILE_QUERY_INFO_NONE,
                                           NULL) == G_FILE_TYPE_REGULAR);

          g_object_unref (file1);
          g_object_unref (file2);
          g_free (locale);

          s = p + 1;
        }

      g_free (locales);

      if (! found)
        {
          GFile *file1  = g_file_get_child (basedir, "en");
          GFile *file2  = g_file_get_child (file1, "clay-help.xml");

          found = (g_file_query_file_type (file2, G_FILE_QUERY_INFO_NONE,
                                           NULL) == G_FILE_TYPE_REGULAR);

          g_object_unref (file1);
          g_object_unref (file2);
        }
    }

  g_object_unref (basedir);

  return found;
}

void
clay_help_user_manual_changed (Clay *clay)
{
  ClayProcedure *procedure;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /*  Check if a help parser is running  */
  procedure = clay_pdb_lookup_procedure (clay->pdb, "extension-clay-help-temp");

  if (CLAY_IS_TEMPORARY_PROCEDURE (procedure))
    {
      clay_plug_in_close (CLAY_TEMPORARY_PROCEDURE (procedure)->plug_in, TRUE);
    }
}

GList *
clay_help_get_installed_languages (void)
{
  GList *manuals = NULL;
  GFile *basedir;

  /*  if CLAY2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("CLAY2_HELP_URI"))
    basedir = g_file_new_for_uri (g_getenv ("CLAY2_HELP_URI"));
  else
    basedir = clay_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      GFileEnumerator *enumerator;

      enumerator = g_file_enumerate_children (basedir,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                              G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                              G_FILE_QUERY_INFO_NONE,
                                              NULL, NULL);

      if (enumerator)
        {
          GFileInfo *info;

          while ((info = g_file_enumerator_next_file (enumerator,
                                                      NULL, NULL)))
            {
              if (g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
                {
                  GFile *locale_dir;
                  GFile *file;

                  locale_dir = g_file_enumerator_get_child (enumerator, info);
                  file  = g_file_get_child (locale_dir, "clay-help.xml");
                  if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE,
                                              NULL) == G_FILE_TYPE_REGULAR)
                    {
                      manuals = g_list_prepend (manuals,
                                                g_strdup (g_file_info_get_name (info)));
                    }
                  g_object_unref (locale_dir);
                  g_object_unref (file);
                }
              g_object_unref (info);
            }
          g_object_unref (enumerator);
        }
    }
  g_object_unref (basedir);

  return manuals;
}

/*  private functions  */

static gboolean
clay_idle_help (ClayIdleHelp *idle_help)
{
  ClayGuiConfig *config = CLAY_GUI_CONFIG (idle_help->clay->config);

  if (! idle_help->help_domain       &&
      ! config->user_manual_online   &&
      ! clay_help_user_manual_is_installed (idle_help->clay))
    {
      clay_idle_help_free (idle_help);

      return FALSE;
    }

  clay_help_call (idle_help->clay,
                  idle_help->progress,
                  idle_help->help_domain,
                  idle_help->help_locales,
                  idle_help->help_id);

  clay_idle_help_free (idle_help);

  return FALSE;
}

static void
clay_idle_help_free (ClayIdleHelp *idle_help)
{
  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (ClayIdleHelp, idle_help);
}

static void
clay_help_call (Clay         *clay,
                ClayProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_locales,
                const gchar  *help_id)
{
  ClayProcedure *procedure;

  /*  Check if a help parser is already running  */
  procedure = clay_pdb_lookup_procedure (clay->pdb, "extension-clay-help-temp");

  if (! procedure)
    {
      ClayValueArray  *args         = NULL;
      gchar          **help_domains = NULL;
      gchar          **help_uris    = NULL;
      GError          *error        = NULL;

      procedure = clay_pdb_lookup_procedure (clay->pdb, "extension-clay-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      clay_help_get_help_domains (clay, &help_domains, &help_uris);

      args = clay_procedure_get_arguments (procedure);
      clay_value_array_truncate (args, 2);

      g_value_take_boxed (clay_value_array_index (args, 0), help_domains);
      g_value_take_boxed (clay_value_array_index (args, 1), help_uris);

      clay_procedure_execute_async (procedure, clay,
                                    clay_get_user_context (clay), progress,
                                    args, NULL, &error);

      clay_value_array_unref (args);

      if (error)
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  /*  Check if the help parser started properly  */
  procedure = clay_pdb_lookup_procedure (clay->pdb, "extension-clay-help-temp");

  if (procedure)
    {
      ClayValueArray *return_vals;
      GError         *error = NULL;

      CLAY_LOG (HELP, "Calling help: %s %s %s",
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        clay_pdb_execute_procedure_by_name (clay->pdb,
                                            clay_get_user_context (clay),
                                            progress, &error,
                                            "extension-clay-help-temp",
                                            G_TYPE_STRING, "",
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      clay_value_array_unref (return_vals);

      if (error)
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }
}

static void
clay_help_get_help_domains (Clay    *clay,
                            gchar ***domain_names,
                            gchar ***domain_uris)
{
  gchar **plug_in_domains = NULL;
  gchar **plug_in_uris    = NULL;
  gint    i, n_domains;

  n_domains = clay_plug_in_manager_get_help_domains (clay->plug_in_manager,
                                                     &plug_in_domains,
                                                     &plug_in_uris);

  *domain_names = g_new0 (gchar *, n_domains + 2);
  *domain_uris  = g_new0 (gchar *, n_domains + 2);

  (*domain_names)[0] = g_strdup ("https://www.gimp.org/help");
  (*domain_uris)[0]  = clay_help_get_default_domain_uri (clay);

  for (i = 0; i < n_domains; i++)
    {
      (*domain_names)[i + 1] = plug_in_domains[i];
      (*domain_uris)[i + 1]  = plug_in_uris[i];
    }

  g_free (plug_in_domains);
  g_free (plug_in_uris);
}

static gchar *
clay_help_get_default_domain_uri (Clay *clay)
{
  ClayGuiConfig *config = CLAY_GUI_CONFIG (clay->config);
  GFile         *dir;
  gchar         *uri;

  if (g_getenv ("CLAY2_HELP_URI"))
    return g_strdup (g_getenv ("CLAY2_HELP_URI"));

  if (config->user_manual_online)
    return g_strdup (config->user_manual_online_uri);

  dir = clay_help_get_user_manual_basedir ();
  uri = g_file_get_uri (dir);
  g_object_unref (dir);

  return uri;
}

static gchar *
clay_help_get_locales (Clay *clay)
{
  ClayGuiConfig  *config = CLAY_GUI_CONFIG (clay->config);
  gchar         **names;
  gchar          *locales      = NULL;
  GList          *locales_list = NULL;
  GList          *iter;
  gint            i;

  if (config->help_locales && strlen (config->help_locales))
    return g_strdup (config->help_locales);

  /* Process locales. */
  names = (gchar **) g_get_language_names ();
  for (i = 0; names[i]; i++)
    {
      gchar *locale = g_strdup (names[i]);
      gchar *c;

      /* We don't care about encoding in context of our help system. */
      c = strchr (locale, '.');
      if (c)
        *c = '\0';

      /* We don't care about variants either. */
      c = strchr (locale, '@');
      if (c)
        *c = '\0';

      /* Apparently some systems (i.e. Windows) would return a value as
       * IETF language tag, which is a different format from POSIX
       * locale; especially it would separate the lang and the region
       * with an hyphen instead of an underscore.
       * Actually the difference is much deeper, and IETF language tags
       * can have extended language subtags, a script subtag, variants,
       * moreover using different codes.
       * We'd actually need to look into this in details (TODO).
       * this dirty hack should do for easy translation at least (like
       * "en-GB" -> "en_GB).
       * Cf. bug 777754.
       */
      c = strchr (locale, '-');
      if (c)
        *c = '_';

      if (locale && *locale &&
          ! g_list_find_custom (locales_list, locale,
                                (GCompareFunc) g_strcmp0))
        {
          gchar *base;

          /* Adding this locale. */
          locales_list = g_list_prepend (locales_list, locale);

          /* Adding the base language as well. */
          base = strdup (locale);
          c = strchr (base, '_');
          if (c)
            *c = '\0';

          if (base && *base &&
              ! g_list_find_custom (locales_list, base,
                                    (GCompareFunc) g_strcmp0))
            {
              locales_list = g_list_prepend (locales_list, base);
            }
          else
            {
              g_free (base);
            }
        }
      else
        {
          g_free (locale);
        }
    }

  locales_list = g_list_reverse (locales_list);

  /* Finally generate the colon-separated value. */
  if (locales_list)
    {
      locales = g_strdup (locales_list->data);
      for (iter = locales_list->next; iter; iter = iter->next)
        {
          gchar *temp = locales;
          locales = g_strconcat (temp, ":", iter->data, NULL);
          g_free (temp);
        }
    }

  g_list_free_full (locales_list, g_free);

  return locales;
}

static GFile *
clay_help_get_user_manual_basedir (void)
{
  GFile *user_dir;
  GFile *sys_dir;

  /* 1. Help is on user settings (e.g. for AppImage, macOS .app) */
  user_dir = clay_directory_file ("help", NULL);
  if (g_file_query_exists (user_dir, NULL))
    {
      return user_dir;
    }

  /* 2. Fallback: help is on user installation */
  g_object_unref (user_dir);
  sys_dir = clay_data_directory_file ("help", NULL);
  return sys_dir;
}

