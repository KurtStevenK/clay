/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymodules.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <gio/gio.h>

#include "libclaybase/claybase.h"
#include "libclaymodule/claymodule.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "clay.h"
#include "clay-modules.h"

#include "clay-intl.h"


void
clay_modules_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (! clay->no_interface)
    {
      clay->module_db = clay_module_db_new (clay->be_verbose);
      clay->write_modulerc = FALSE;
    }
}

void
clay_modules_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_clear_object (&clay->module_db);
}

void
clay_modules_load (Clay *clay)
{
  GFile    *file;
  GScanner *scanner;
  gchar    *module_load_inhibit = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->no_interface)
    return;

  clay_module_db_set_verbose (clay->module_db, clay->be_verbose);

  file = clay_directory_file ("modulerc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  scanner = clay_scanner_new_file (file, NULL);
  g_object_unref (file);

  if (scanner)
    {
      GTokenType  token;
      GError     *error = NULL;

#define MODULE_LOAD_INHIBIT 1

      g_scanner_scope_add_symbol (scanner, 0, "module-load-inhibit",
                                  GINT_TO_POINTER (MODULE_LOAD_INHIBIT));

      token = G_TOKEN_LEFT_PAREN;

      while (g_scanner_peek_next_token (scanner) == token)
        {
          token = g_scanner_get_next_token (scanner);

          switch (token)
            {
            case G_TOKEN_LEFT_PAREN:
              token = G_TOKEN_SYMBOL;
              break;

            case G_TOKEN_SYMBOL:
              if (scanner->value.v_symbol == GINT_TO_POINTER (MODULE_LOAD_INHIBIT))
                {
                  token = G_TOKEN_STRING;

                  if (! clay_scanner_parse_string_no_validate (scanner,
                                                               &module_load_inhibit))
                    goto error;
                }
              token = G_TOKEN_RIGHT_PAREN;
              break;

            case G_TOKEN_RIGHT_PAREN:
              token = G_TOKEN_LEFT_PAREN;
              break;

            default: /* do nothing */
              break;
            }
        }

#undef MODULE_LOAD_INHIBIT

      if (token != G_TOKEN_LEFT_PAREN)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
        }

    error:

      if (error)
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          g_clear_error (&error);
        }

      clay_scanner_unref (scanner);
    }

  if (module_load_inhibit)
    {
      clay_module_db_set_load_inhibit (clay->module_db, module_load_inhibit);
      g_free (module_load_inhibit);
    }

  clay_module_db_load (clay->module_db, clay->config->module_path);
}

void
clay_modules_unload (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (! clay->no_interface && clay->write_modulerc)
    {
      ClayConfigWriter *writer;
      GString          *str;
      GListModel       *modules = G_LIST_MODEL (clay->module_db);
      guint             i;
      const gchar      *p;
      GFile            *file;
      GError           *error = NULL;

      str = g_string_new (NULL);
      for (i = 0; i < g_list_model_get_n_items (modules); i++)
        {
          ClayModule *module;

          module = g_list_model_get_item (modules, i);
          if (! clay_module_get_auto_load (module))
            {
              GFile *file = clay_module_get_file (module);
              gchar *path = g_file_get_path (file);

              g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
              g_string_append (str, path);

              g_free (path);
            }

          g_clear_object (&module);
        }
      if (str->len > 0)
        p = str->str + 1;
      else
        p = "";

      file = clay_directory_file ("modulerc", NULL);

      if (clay->be_verbose)
        g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

      writer = clay_config_writer_new_from_file (file,
                                                 TRUE,
                                                 "Clay modulerc",
                                                 &error);
      g_object_unref (file);

      if (writer)
        {
          clay_config_writer_open (writer, "module-load-inhibit");
          clay_config_writer_string (writer, p);
          clay_config_writer_close (writer);

          clay_config_writer_finish (writer, "end of modulerc", &error);

          clay->write_modulerc = FALSE;
        }

      g_string_free (str, TRUE);

      if (error)
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          g_clear_error (&error);
        }
    }
}

void
clay_modules_refresh (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (! clay->no_interface)
    {
      clay_module_db_refresh (clay->module_db, clay->config->module_path);
    }
}
