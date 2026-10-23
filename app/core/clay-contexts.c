/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clay-contexts.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clayerror.h"
#include "clay-contexts.h"
#include "claycontext.h"

#include "config/clayconfig-file.h"

#include "clay-intl.h"


void
clay_contexts_init (Clay *clay)
{
  ClayContext *context;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /*  the default context contains the user's saved preferences
   *
   *  TODO: load from disk
   */
  context = clay_context_new (clay, "Default", NULL);
  clay_set_default_context (clay, context);
  g_object_unref (context);

  /*  the initial user_context is a straight copy of the default context
   */
  context = clay_context_new (clay, "User", context);
  clay_set_user_context (clay, context);
  g_object_unref (context);
}

void
clay_contexts_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_set_user_context (clay, NULL);
  clay_set_default_context (clay, NULL);
}

gboolean
clay_contexts_load (Clay    *clay,
                    GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_directory_file ("contextrc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  success = clay_config_deserialize_file (CLAY_CONFIG (clay_get_user_context (clay)),
                                          file,
                                          NULL, &my_error);

  g_object_unref (file);

  if (! success)
    {
      if (my_error->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&my_error);
          success = TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
        }
    }

  return success;
}

gboolean
clay_contexts_save (Clay    *clay,
                    GError **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = clay_directory_file ("contextrc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  success = clay_config_serialize_to_file (CLAY_CONFIG (clay_get_user_context (clay)),
                                           file,
                                           "Clay user context",
                                           "end of user context",
                                           NULL, error);

  g_object_unref (file);

  return success;
}

gboolean
clay_contexts_clear (Clay    *clay,
                     GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  file = clay_directory_file ("contextrc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}
