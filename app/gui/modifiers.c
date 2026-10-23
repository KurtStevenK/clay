/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * modifiers.c
 * Copyright (C) 2022 Jehan
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

#include "display/claymodifiersmanager.h"

#include "widgets/claywidgets-utils.h"

#include "dialogs/dialogs.h"

#include "modifiers.h"
#include "clay-log.h"

#include "clay-intl.h"


enum
{
  MODIFIERS_INFO = 1,
  HIDE_DOCKS,
  SINGLE_WINDOW_MODE,
  SHOW_TABS,
  TABS_POSITION,
  LAST_TIP_SHOWN
};


static GFile * modifiers_file (Clay *clay);


/*  private variables  */

static gboolean   modifiersrc_deleted = FALSE;


/*  public functions  */

void
modifiers_init (Clay *clay)
{
  ClayDisplayConfig    *display_config;
  GFile                *file;
  ClayModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  display_config = CLAY_DISPLAY_CONFIG (clay->config);
  if (display_config->modifiers_manager != NULL)
    return;

  manager = clay_modifiers_manager_new ();
  g_object_set (display_config, "modifiers-manager", manager, NULL);
  g_object_unref (manager);

  file = modifiers_file (clay);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  clay_config_deserialize_file (CLAY_CONFIG (manager), file, NULL, &error);

  if (error)
    {
      /* File not existing is considered a normal event, not an error.
       * It can happen for instance the first time you run Clay. When
       * this happens, we ignore the error. The ClayModifiersManager
       * object will simply use default modifiers.
       */
      if (error->domain != CLAY_CONFIG_ERROR ||
          error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
          clay_config_file_backup_on_error (file, "modifiersrc", NULL);
        }

      g_clear_error (&error);
    }

  g_object_unref (file);
}

void
modifiers_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
}

void
modifiers_restore (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
}

void
modifiers_save (Clay     *clay,
                gboolean  always_save)
{
  ClayDisplayConfig    *display_config;
  GFile                *file;
  ClayModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (modifiersrc_deleted && ! always_save)
    return;

  display_config = CLAY_DISPLAY_CONFIG (clay->config);
  g_return_if_fail (CLAY_IS_DISPLAY_CONFIG (display_config));

  manager = CLAY_MODIFIERS_MANAGER (display_config->modifiers_manager);
  g_return_if_fail (manager != NULL);
  g_return_if_fail (CLAY_IS_MODIFIERS_MANAGER (manager));
  file = modifiers_file (clay);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  clay_config_serialize_to_file (CLAY_CONFIG (manager), file,
                                 "Clay modifiersrc\n\n"
                                 "This file stores modifiers configuration. "
                                 "You are not supposed to edit it manually, "
                                 "but of course you can do. The modifiersrc "
                                 "will be entirely rewritten every time you "
                                 "quit Clay. If this file isn't found, "
                                 "defaults are used.",
                                 NULL, NULL, &error);
  if (error != NULL)
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);

  modifiersrc_deleted = FALSE;
}

gboolean
modifiers_clear (Clay    *clay,
                 GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = modifiers_file (clay);

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
      modifiersrc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

static GFile *
modifiers_file (Clay *clay)
{
  const gchar *basename;
  GFile       *file;

  basename = g_getenv ("CLAY_TESTING_MODIFIERSRC_NAME");
  if (! basename)
    basename = "modifiersrc";

  file = clay_directory_file (basename, NULL);

  return file;
}
