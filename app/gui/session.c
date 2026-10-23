/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Session-managment stuff
 * Copyright (C) 1998 Sven Neumann <sven@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "gui-types.h"

#include "config/clayconfig-file.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clayerror.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claysessioninfo.h"
#include "widgets/claywidgets-utils.h"

#include "dialogs/dialogs.h"

#include "session.h"
#include "clay-log.h"

#include "clay-intl.h"


enum
{
  SESSION_INFO = 1,
  HIDE_DOCKS,
  SINGLE_WINDOW_MODE,
  SHOW_TABS,
  TABS_POSITION,
  LAST_TIP_SHOWN
};


static GFile * session_file (Clay *clay);


/*  private variables  */

static gboolean   sessionrc_deleted = FALSE;


/*  public functions  */

void
session_init (Clay *clay)
{
  GFile      *file;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = session_file (clay);

  scanner = clay_scanner_new_file (file, &error);

  if (! scanner && error->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = clay_sysconf_directory_file ("sessionrc", NULL);

      scanner = clay_scanner_new_file (file, NULL);
    }

  if (! scanner)
    {
      g_clear_error (&error);
      g_object_unref (file);
      return;
    }

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  g_scanner_scope_add_symbol (scanner, 0, "session-info",
                              GINT_TO_POINTER (SESSION_INFO));
  g_scanner_scope_add_symbol (scanner, 0,  "hide-docks",
                              GINT_TO_POINTER (HIDE_DOCKS));
  g_scanner_scope_add_symbol (scanner, 0,  "single-window-mode",
                              GINT_TO_POINTER (SINGLE_WINDOW_MODE));
  g_scanner_scope_add_symbol (scanner, 0,  "show-tabs",
                              GINT_TO_POINTER (SHOW_TABS));
  g_scanner_scope_add_symbol (scanner, 0,  "tabs-position",
                              GINT_TO_POINTER (TABS_POSITION));
  g_scanner_scope_add_symbol (scanner, 0,  "last-tip-shown",
                              GINT_TO_POINTER (LAST_TIP_SHOWN));

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
          if (scanner->value.v_symbol == GINT_TO_POINTER (SESSION_INFO))
            {
              ClayDialogFactory      *factory      = NULL;
              ClaySessionInfo        *info         = NULL;
              gchar                  *factory_name = NULL;
              gchar                  *entry_name   = NULL;
              ClayDialogFactoryEntry *entry        = NULL;

              token = G_TOKEN_STRING;

              if (! clay_scanner_parse_string (scanner, &factory_name))
                break;

              /* In versions <= Clay 2.6 there was a "toolbox", a
               * "dock", a "display" and a "toplevel" factory. These
               * are now merged to a single clay_dialog_factory_get_singleton (). We
               * need the legacy name though, so keep it around.
               */
              factory = clay_dialog_factory_get_singleton ();

              info = clay_session_info_new ();

              /* Clay 2.6 has the entry name as part of the
               * session-info header, so try to get it
               */
              clay_scanner_parse_string (scanner, &entry_name);
              if (entry_name)
                {
                  /* Previously, ClayDock was a toplevel. That is why
                   * versions <= Clay 2.6 has "dock" as the entry name. We
                   * want "dock" to be interpreted as 'dock window'
                   * however so have some special-casing for that. When
                   * the entry name is "dock" the factory name is either
                   * "dock" or "toolbox".
                   */
                  if (strcmp (entry_name, "dock") == 0)
                    {
                      entry =
                        clay_dialog_factory_find_entry (factory,
                                                        (strcmp (factory_name, "toolbox") == 0 ?
                                                         "clay-toolbox-window" :
                                                         "clay-dock-window"));
                    }
                  else
                    {
                      entry = clay_dialog_factory_find_entry (factory,
                                                              entry_name);
                    }
                }

              /* We're done with these now */
              g_free (factory_name);
              g_free (entry_name);

              /* We can get the factory entry either now (the Clay <=
               * 2.6 way), or when we deserialize (the Clay 2.8 way)
               */
              if (entry)
                {
                  clay_session_info_set_factory_entry (info, entry);
                }

              /* Always try to deserialize */
              if (clay_config_deserialize (CLAY_CONFIG (info), scanner, 1, NULL))
                {
                  /* Make sure we got a factory entry either the 2.6
                   * or 2.8 way
                   */
                  if (clay_session_info_get_factory_entry (info))
                    {
                      CLAY_LOG (DIALOG_FACTORY,
                                "successfully parsed and added session info %p",
                                info);

                      clay_dialog_factory_add_session_info (factory, info);
                    }
                  else
                    {
                      CLAY_LOG (DIALOG_FACTORY,
                                "failed to parse session info %p, not adding",
                                info);
                    }

                  g_object_unref (info);
                }
              else
                {
                  g_object_unref (info);

                  /* set token to left paren so we won't set another
                   * error below, clay_config_deserialize() already did
                   */
                  token = G_TOKEN_LEFT_PAREN;
                  goto error;
                }
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (HIDE_DOCKS))
            {
              gboolean hide_docks;

              token = G_TOKEN_IDENTIFIER;

              if (! clay_scanner_parse_boolean (scanner, &hide_docks))
                break;

              g_object_set (clay->config,
                            "hide-docks", hide_docks,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (SINGLE_WINDOW_MODE))
            {
              gboolean single_window_mode;

              token = G_TOKEN_IDENTIFIER;

              if (! clay_scanner_parse_boolean (scanner, &single_window_mode))
                break;

              g_object_set (clay->config,
                            "single-window-mode", single_window_mode,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (SHOW_TABS))
          {
            gboolean show_tabs;

            token = G_TOKEN_IDENTIFIER;

            if (! clay_scanner_parse_boolean (scanner, &show_tabs))
              break;

            g_object_set (clay->config,
                          "show-tabs", show_tabs,
                          NULL);
          }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (TABS_POSITION))
            {
              gint tabs_position;

              token = G_TOKEN_INT;

              if (! clay_scanner_parse_int (scanner, &tabs_position))
                break;

              g_object_set (clay->config,
                            "tabs-position", tabs_position,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (LAST_TIP_SHOWN))
            {
              gint last_tip_shown;

              token = G_TOKEN_INT;

              if (! clay_scanner_parse_int (scanner, &last_tip_shown))
                break;

              g_object_set (clay->config,
                            "last-tip-shown", last_tip_shown,
                            NULL);
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

 error:

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  if (error)
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      clay_config_file_backup_on_error (file, "sessionrc", NULL);
    }

  clay_scanner_unref (scanner);
  g_object_unref (file);

  dialogs_load_recent_docks (clay);
}

void
session_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
}

void
session_restore (Clay       *clay,
                 GdkMonitor *monitor)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  clay_dialog_factory_restore (clay_dialog_factory_get_singleton (),
                               monitor);

  /* make sure ClayImageWindow acts upon hide-docks at the right time,
   * see bug #678043.
   */
  if (CLAY_GUI_CONFIG (clay->config)->single_window_mode &&
      CLAY_GUI_CONFIG (clay->config)->hide_docks)
    {
      g_object_notify (G_OBJECT (clay->config), "hide-docks");
    }
}

void
session_save (Clay     *clay,
              gboolean  always_save)
{
  ClayConfigWriter *writer;
  GFile            *file;
  GError           *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (sessionrc_deleted && ! always_save)
    return;

  file = session_file (clay);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  writer =
    clay_config_writer_new_from_file (file,
                                      TRUE,
                                      "Clay sessionrc\n\n"
                                      "This file takes session-specific info "
                                      "(that is info, you want to keep between "
                                      "two Clay sessions).  You are not supposed "
                                      "to edit it manually, but of course you "
                                      "can do.  The sessionrc will be entirely "
                                      "rewritten every time you quit Clay.  "
                                      "If this file isn't found, defaults are "
                                      "used.",
                                      NULL);
  g_object_unref (file);

  if (!writer)
    return;

  clay_dialog_factory_save (clay_dialog_factory_get_singleton (), writer);
  clay_config_writer_linefeed (writer);

  clay_config_writer_open (writer, "hide-docks");
  clay_config_writer_identifier (writer,
                                 CLAY_GUI_CONFIG (clay->config)->hide_docks ?
                                 "yes" : "no");
  clay_config_writer_close (writer);

  clay_config_writer_open (writer, "single-window-mode");
  clay_config_writer_identifier (writer,
                                 CLAY_GUI_CONFIG (clay->config)->single_window_mode ?
                                 "yes" : "no");
  clay_config_writer_close (writer);

  clay_config_writer_open (writer, "show-tabs");
  clay_config_writer_printf (writer,
                             CLAY_GUI_CONFIG (clay->config)->show_tabs ?
                             "yes" : "no");
  clay_config_writer_close (writer);

  clay_config_writer_open (writer, "tabs-position");
  clay_config_writer_printf (writer, "%d",
                             CLAY_GUI_CONFIG (clay->config)->tabs_position);
  clay_config_writer_close (writer);

  clay_config_writer_open (writer, "last-tip-shown");
  clay_config_writer_printf (writer, "%d",
                             CLAY_GUI_CONFIG (clay->config)->last_tip_shown);
  clay_config_writer_close (writer);

  if (! clay_config_writer_finish (writer, "end of sessionrc", &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  dialogs_save_recent_docks (clay);

  sessionrc_deleted = FALSE;
}

gboolean
session_clear (Clay    *clay,
               GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = session_file (clay);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
    }
  else
    {
      sessionrc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}


static GFile *
session_file (Clay *clay)
{
  const gchar *basename;
  gchar       *filename;
  GFile       *file;

  basename = g_getenv ("CLAY_TESTING_SESSIONRC_NAME");
  if (! basename)
    basename = "sessionrc";

  if (clay->session_name)
    filename = g_strconcat (basename, ".", clay->session_name, NULL);
  else
    filename = g_strdup (basename);

  file = clay_directory_file (filename, NULL);

  g_free (filename);

  return file;
}
