/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphelp.c
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"
#include "core/gimp-utils.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "plug-in/gimpplugin.h"
#include "plug-in/gimppluginmanager-help-domain.h"
#include "plug-in/gimptemporaryprocedure.h"

#include "gimphelp.h"
#include "gimphelp-ids.h"
#include "gimplanguagecombobox.h"
#include "gimpmessagebox.h"
#include "gimpmessagedialog.h"
#include "gimpmessagedialog.h"
#include "gimpwidgets-utils.h"

#include "gimp-log.h"
#include "gimp-intl.h"


typedef struct _GimpIdleHelp GimpIdleHelp;

struct _GimpIdleHelp
{
  Gimp         *gimp;
  GimpProgress *progress;
  gchar        *help_domain;
  gchar        *help_locales;
  gchar        *help_id;
};


/*  local function prototypes  */

static gboolean   gimp_idle_help          (GimpIdleHelp  *idle_help);
static void       gimp_idle_help_free     (GimpIdleHelp  *idle_help);

static void       gimp_help_call          (Gimp          *gimp,
                                           GimpProgress  *progress,
                                           const gchar   *help_domain,
                                           const gchar   *help_locales,
                                           const gchar   *help_id);

static void       gimp_help_get_help_domains         (Gimp    *gimp,
                                                      gchar ***domain_names,
                                                      gchar ***domain_uris);
static gchar    * gimp_help_get_default_domain_uri   (Gimp    *gimp);
static gchar    * gimp_help_get_locales              (Gimp    *gimp);

static GFile    * gimp_help_get_user_manual_basedir  (void);

/*  public functions  */

void
gimp_help_show (Gimp         *gimp,
                GimpProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_id)
{
  GimpGuiConfig *config;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  config = GIMP_GUI_CONFIG (gimp->config);

  if (config->use_help)
    {
      GimpIdleHelp *idle_help = g_slice_new0 (GimpIdleHelp);

      idle_help->gimp     = gimp;
      idle_help->progress = progress;

      if (help_domain && strlen (help_domain))
        idle_help->help_domain = g_strdup (help_domain);

      idle_help->help_locales = gimp_help_get_locales (gimp);

      if (help_id && strlen (help_id))
        idle_help->help_id = g_strdup (help_id);

      GIMP_LOG (HELP, "request for help-id '%s' from help-domain '%s'",
                help_id     ? help_id      : "(null)",
                help_domain ? help_domain  : "(null)");

      g_idle_add ((GSourceFunc) gimp_idle_help, idle_help);
    }
}

gboolean
gimp_help_user_manual_is_installed (Gimp *gimp)
{
  GFile    *basedir;
  gboolean  found = FALSE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  /*  if GIMP2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("GIMP2_HELP_URI"))
    return TRUE;

  basedir = gimp_help_get_user_manual_basedir ();

  if (g_file_query_file_type (basedir, G_FILE_QUERY_INFO_NONE, NULL) ==
      G_FILE_TYPE_DIRECTORY)
    {
      gchar       *locales = gimp_help_get_locales (gimp);
      const gchar *s       = locales;
      const gchar *p;

      for (p = strchr (s, ':'); p && !found; p = strchr (s, ':'))
        {
          gchar *locale = g_strndup (s, p - s);
          GFile *file1  = g_file_get_child (basedir, locale);
          GFile *file2  = g_file_get_child (file1, "gimp-help.xml");

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
          GFile *file2  = g_file_get_child (file1, "gimp-help.xml");

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
gimp_help_user_manual_changed (Gimp *gimp)
{
  GimpProcedure *procedure;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  Check if a help parser is running  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (GIMP_IS_TEMPORARY_PROCEDURE (procedure))
    {
      gimp_plug_in_close (GIMP_TEMPORARY_PROCEDURE (procedure)->plug_in, TRUE);
    }
}

GList *
gimp_help_get_installed_languages (void)
{
  GList *manuals = NULL;
  GFile *basedir;

  /*  if GIMP2_HELP_URI is set, assume that the manual can be found there  */
  if (g_getenv ("GIMP2_HELP_URI"))
    basedir = g_file_new_for_uri (g_getenv ("GIMP2_HELP_URI"));
  else
    basedir = gimp_help_get_user_manual_basedir ();

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
                  file  = g_file_get_child (locale_dir, "gimp-help.xml");
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
gimp_idle_help (GimpIdleHelp *idle_help)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (idle_help->gimp->config);

  if (! idle_help->help_domain       &&
      ! config->user_manual_online   &&
      ! gimp_help_user_manual_is_installed (idle_help->gimp))
    {
      gimp_idle_help_free (idle_help);

      return FALSE;
    }

  gimp_help_call (idle_help->gimp,
                  idle_help->progress,
                  idle_help->help_domain,
                  idle_help->help_locales,
                  idle_help->help_id);

  gimp_idle_help_free (idle_help);

  return FALSE;
}

static void
gimp_idle_help_free (GimpIdleHelp *idle_help)
{
  g_free (idle_help->help_domain);
  g_free (idle_help->help_locales);
  g_free (idle_help->help_id);

  g_slice_free (GimpIdleHelp, idle_help);
}

static void
gimp_help_call (Gimp         *gimp,
                GimpProgress *progress,
                const gchar  *help_domain,
                const gchar  *help_locales,
                const gchar  *help_id)
{
  GimpProcedure *procedure;

  /*  Check if a help parser is already running  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (! procedure)
    {
      GimpValueArray  *args         = NULL;
      gchar          **help_domains = NULL;
      gchar          **help_uris    = NULL;
      GError          *error        = NULL;

      procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help");

      if (! procedure)
        /*  FIXME: error msg  */
        return;

      gimp_help_get_help_domains (gimp, &help_domains, &help_uris);

      args = gimp_procedure_get_arguments (procedure);
      gimp_value_array_truncate (args, 2);

      g_value_take_boxed (gimp_value_array_index (args, 0), help_domains);
      g_value_take_boxed (gimp_value_array_index (args, 1), help_uris);

      gimp_procedure_execute_async (procedure, gimp,
                                    gimp_get_user_context (gimp), progress,
                                    args, NULL, &error);

      gimp_value_array_unref (args);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }

  /*  Check if the help parser started properly  */
  procedure = gimp_pdb_lookup_procedure (gimp->pdb, "extension-gimp-help-temp");

  if (procedure)
    {
      GimpValueArray *return_vals;
      GError         *error = NULL;

      GIMP_LOG (HELP, "Calling help: %s %s %s",
                help_domain  ? help_domain  : "(null)",
                help_locales ? help_locales : "(null)",
                help_id      ? help_id      : "(null)");

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb,
                                            gimp_get_user_context (gimp),
                                            progress, &error,
                                            "extension-gimp-help-temp",
                                            G_TYPE_STRING, "",
                                            G_TYPE_STRING, help_domain,
                                            G_TYPE_STRING, help_locales,
                                            G_TYPE_STRING, help_id,
                                            G_TYPE_NONE);

      gimp_value_array_unref (return_vals);

      if (error)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          g_error_free (error);
        }
    }
}

static void
gimp_help_get_help_domains (Gimp    *gimp,
                            gchar ***domain_names,
                            gchar ***domain_uris)
{
  gchar **plug_in_domains = NULL;
  gchar **plug_in_uris    = NULL;
  gint    i, n_domains;

  n_domains = gimp_plug_in_manager_get_help_domains (gimp->plug_in_manager,
                                                     &plug_in_domains,
                                                     &plug_in_uris);

  *domain_names = g_new0 (gchar *, n_domains + 2);
  *domain_uris  = g_new0 (gchar *, n_domains + 2);

  (*domain_names)[0] = g_strdup ("https://www.gimp.org/help");
  (*domain_uris)[0]  = gimp_help_get_default_domain_uri (gimp);

  for (i = 0; i < n_domains; i++)
    {
      (*domain_names)[i + 1] = plug_in_domains[i];
      (*domain_uris)[i + 1]  = plug_in_uris[i];
    }

  g_free (plug_in_domains);
  g_free (plug_in_uris);
}

static gchar *
gimp_help_get_default_domain_uri (Gimp *gimp)
{
  GimpGuiConfig *config = GIMP_GUI_CONFIG (gimp->config);
  GFile         *dir;
  gchar         *uri;

  if (g_getenv ("GIMP2_HELP_URI"))
    return g_strdup (g_getenv ("GIMP2_HELP_URI"));

  if (config->user_manual_online)
    return g_strdup (config->user_manual_online_uri);

  dir = gimp_help_get_user_manual_basedir ();
  uri = g_file_get_uri (dir);
  g_object_unref (dir);

  return uri;
}

static gchar *
gimp_help_get_locales (Gimp *gimp)
{
  GimpGuiConfig  *config = GIMP_GUI_CONFIG (gimp->config);
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
gimp_help_get_user_manual_basedir (void)
{
  GFile *user_dir;
  GFile *sys_dir;

  /* 1. Help is on user settings (e.g. for AppImage, macOS .app) */
  user_dir = gimp_directory_file ("help", NULL);
  if (g_file_query_exists (user_dir, NULL))
    {
      return user_dir;
    }

  /* 2. Fallback: help is on user installation */
  g_object_unref (user_dir);
  sys_dir = gimp_data_directory_file ("help", NULL);
  return sys_dir;
}

