/* gimpparasite.c: Copyright 1998 Jay Cox <jaycox@gimp.org>
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
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clay-parasites.h"
#include "clayparasitelist.h"


gboolean
clay_parasite_validate (Clay                *clay,
                        const ClayParasite  *parasite,
                        GError             **error)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}

void
clay_parasite_attach (Clay               *clay,
                      const ClayParasite *parasite)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (parasite != NULL);

  clay_parasite_list_add (clay->parasites, parasite);
}

void
clay_parasite_detach (Clay        *clay,
                      const gchar *name)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (name != NULL);

  clay_parasite_list_remove (clay->parasites, name);
}

const ClayParasite *
clay_parasite_find (Clay        *clay,
                    const gchar *name)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return clay_parasite_list_find (clay->parasites, name);
}

static void
list_func (const gchar    *key,
           ClayParasite   *parasite,
           gchar        ***current)
{
  *(*current)++ = g_strdup (key);
}

gchar **
clay_parasite_list (Clay *clay)
{
  gint    count;
  gchar **list;
  gchar **current;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  count = clay_parasite_list_length (clay->parasites);

  list = current = g_new0 (gchar *, count + 1);

  clay_parasite_list_foreach (clay->parasites, (GHFunc) list_func, &current);

  return list;
}


/*  FIXME: this doesn't belong here  */

void
clay_parasite_shift_parent (ClayParasite *parasite)
{
  g_return_if_fail (parasite != NULL);

  parasite->flags = (parasite->flags >> 8);
}


/*  parasiterc functions  */

void
clay_parasiterc_load (Clay *clay)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = clay_directory_file ("parasiterc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_deserialize_file (CLAY_CONFIG (clay->parasites),
                                      file, NULL, &error))
    {
      if (error->code != CLAY_CONFIG_ERROR_OPEN_ENOENT)
        clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);

      g_error_free (error);
    }

  g_object_unref (file);
}

void
clay_parasiterc_save (Clay *clay)
{
  const gchar *header =
    "Clay parasiterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of parasiterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_PARASITE_LIST (clay->parasites));

  file = clay_directory_file ("parasiterc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (clay->parasites),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}
