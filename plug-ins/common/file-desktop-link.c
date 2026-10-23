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
 * Desktop Entry Specification
 * http://standards.freedesktop.org/desktop-entry-spec/latest/
 */

#include "config.h"

#include <string.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>

#include "libclay/stdplugins-intl.h"


#define LOAD_PROC      "file-desktop-link-load"
#define PLUG_IN_BINARY "file-desktop-link"
#define PLUG_IN_ROLE   "clay-file-desktop-link"


typedef struct _Desktop      Desktop;
typedef struct _DesktopClass DesktopClass;

struct _Desktop
{
  ClayPlugIn      parent_instance;
};

struct _DesktopClass
{
  ClayPlugInClass parent_class;
};


#define DESKTOP_TYPE  (desktop_get_type ())
#define DESKTOP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESKTOP_TYPE, Desktop))

GType                   desktop_get_type         (void);

static GList          * desktop_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * desktop_create_procedure (ClayPlugIn            *plug_in,
                                                  const gchar           *name);

static ClayValueArray * desktop_load             (ClayProcedure         *procedure,
                                                  ClayRunMode            run_mode,
                                                  GFile                 *file,
                                                  ClayMetadata          *metadata,
                                                  ClayMetadataLoadFlags *flags,
                                                  ClayProcedureConfig   *config,
                                                  gpointer               run_data);

static ClayImage      * load_image               (GFile                 *file,
                                                  ClayRunMode            run_mode,
                                                  GError               **error);


G_DEFINE_TYPE (Desktop, desktop, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (DESKTOP_TYPE)
DEFINE_STD_SET_I18N


static void
desktop_class_init (DesktopClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = desktop_query_procedures;
  plug_in_class->create_procedure = desktop_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
desktop_init (Desktop *desktop)
{
}

static GList *
desktop_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (LOAD_PROC));
}

static ClayProcedure *
desktop_create_procedure (ClayPlugIn  *plug_in,
                          const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           desktop_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Desktop Link"));

      clay_procedure_set_documentation (procedure,
                                        "Follows a link to an image in a "
                                        ".desktop file",
                                        "Opens a .desktop file and if it is "
                                        "a link, it asks Clay to open the "
                                        "file the link points to.",
                                        LOAD_PROC);

      clay_procedure_set_attribution (procedure,
                                      "Sven Neumann",
                                      "Sven Neumann",
                                      "2006");

      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "desktop");
    }

  return procedure;
}

static ClayValueArray *
desktop_load (ClayProcedure         *procedure,
              ClayRunMode            run_mode,
              GFile                 *file,
              ClayMetadata          *metadata,
              ClayMetadataLoadFlags *flags,
              ClayProcedureConfig   *config,
              gpointer               run_data)
{
  ClayValueArray *return_values;
  ClayImage      *image;
  GError         *error  = NULL;

  image = load_image (file, run_mode, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_values = clay_procedure_new_return_values (procedure,
                                                    CLAY_PDB_SUCCESS,
                                                    NULL);

  CLAY_VALUES_SET_IMAGE (return_values, 1, image);

  return return_values;
}

static ClayImage *
load_image (GFile        *file,
            ClayRunMode   run_mode,
            GError      **load_error)
{
  GKeyFile  *key_file = g_key_file_new ();
  ClayImage *image    = NULL;
  gchar     *filename = NULL;
  gchar     *group    = NULL;
  gchar     *value    = NULL;
  GError    *error    = NULL;

  filename = g_file_get_path (file);

  if (! g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, &error))
    goto out;

  group = g_key_file_get_start_group (key_file);
  if (! group || strcmp (group, G_KEY_FILE_DESKTOP_GROUP) != 0)
    goto out;

  value = g_key_file_get_value (key_file,
                                group, G_KEY_FILE_DESKTOP_KEY_TYPE, &error);
  if (! value || strcmp (value, G_KEY_FILE_DESKTOP_TYPE_LINK) != 0)
    goto out;

  g_free (value);

  value = g_key_file_get_value (key_file,
                                group, G_KEY_FILE_DESKTOP_KEY_URL, &error);
  if (value)
    image = clay_file_load (run_mode, g_file_new_for_uri (value));

 out:
  if (error)
    {
      g_set_error (load_error, error->domain, error->code,
                   _("Error loading desktop file '%s': %s"),
                   clay_filename_to_utf8 (filename), error->message);
      g_error_free (error);
    }

  g_free (value);
  g_free (group);
  g_free (filename);
  g_key_file_free (key_file);

  return image;
}
