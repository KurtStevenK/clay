/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clay-templates.h"
#include "claylist.h"
#include "claytemplate.h"


/* functions to load and save the clay templates files */

void
clay_templates_load (Clay *clay)
{
  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_LIST (clay->templates));

  file = clay_directory_file ("templaterc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_deserialize_file (CLAY_CONFIG (clay->templates),
                                      file, NULL, &error))
    {
      if (error->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&error);
          g_object_unref (file);

          if (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"))
            {
              gchar *path;
              path = g_build_filename (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"),
                                       "etc", "templaterc", NULL);
              file = g_file_new_for_path (path);
              g_free (path);
            }
          else
            {
              file = clay_sysconf_directory_file ("templaterc", NULL);
            }

          if (! clay_config_deserialize_file (CLAY_CONFIG (clay->templates),
                                              file, NULL, &error))
            {
              clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR,
                                    error->message);
            }
        }
      else
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
        }

      g_clear_error (&error);
    }

  clay_list_reverse (CLAY_LIST (clay->templates));

  g_object_unref (file);
}

void
clay_templates_save (Clay *clay)
{
  const gchar *header =
    "Clay templaterc\n"
    "\n"
    "This file will be entirely rewritten each time you exit.";
  const gchar *footer =
    "end of templaterc";

  GFile  *file;
  GError *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_LIST (clay->templates));

  file = clay_directory_file ("templaterc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  if (! clay_config_serialize_to_file (CLAY_CONFIG (clay->templates),
                                       file,
                                       header, footer, NULL,
                                       &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_unref (file);
}


/*  just like clay_list_get_child_by_name() but matches case-insensitive
 *  and dpi/ppi-insensitive
 */
static ClayObject *
clay_templates_migrate_get_child_by_name (ClayContainer *container,
                                          const gchar   *name)
{
  ClayList   *list   = CLAY_LIST (container);
  ClayObject *retval = NULL;
  GList      *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      ClayObject *object = glist->data;
      gchar      *str1   = g_ascii_strdown (clay_object_get_name (object), -1);
      gchar      *str2   = g_ascii_strdown (name, -1);

      if (! strcmp (str1, str2))
        {
          retval = object;
        }
      else
        {
          gchar *dpi = strstr (str1, "dpi");

          if (dpi)
            {
              memcpy (dpi, "ppi", 3);

              g_print ("replaced: %s\n", str1);

              if (! strcmp (str1, str2))
                retval = object;
            }
        }

      g_free (str1);
      g_free (str2);
    }

  return retval;
}

/**
 * clay_templates_migrate:
 * @olddir: the old user directory
 *
 * Migrating the templaterc from Clay 2.0 to Clay 2.2 needs this special
 * hack since we changed the way that units are handled. This function
 * merges the user's templaterc with the systemwide templaterc. The goal
 * is to replace the unit for a couple of default templates with "pixels".
 **/
void
clay_templates_migrate (const gchar *olddir)
{
  ClayContainer *templates = clay_list_new (CLAY_TYPE_TEMPLATE, TRUE);
  GFile         *file      = clay_directory_file ("templaterc", NULL);

  if (clay_config_deserialize_file (CLAY_CONFIG (templates), file,
                                    NULL, NULL))
    {
      GFile *sysconf_file;

      sysconf_file = clay_sysconf_directory_file ("templaterc", NULL);

      if (olddir && (strstr (olddir, "2.0") || strstr (olddir, "2.2")))
        {
          /* We changed the spelling of a couple of template names:
           *
           * - from upper to lower case between 2.0 and 2.2
           * - from "dpi" to "ppi" between 2.2 and 2.4
           */
          ClayContainerClass *class = CLAY_CONTAINER_GET_CLASS (templates);
          gpointer            func  = class->get_child_by_name;

          class->get_child_by_name = clay_templates_migrate_get_child_by_name;

          clay_config_deserialize_file (CLAY_CONFIG (templates),
                                        sysconf_file, NULL, NULL);

          class->get_child_by_name = func;
        }
      else
        {
          clay_config_deserialize_file (CLAY_CONFIG (templates),
                                        sysconf_file, NULL, NULL);
        }

      g_object_unref (sysconf_file);

      clay_list_reverse (CLAY_LIST (templates));

      clay_config_serialize_to_file (CLAY_CONFIG (templates), file,
                                     NULL, NULL, NULL, NULL);
    }

  g_object_unref (file);
}
