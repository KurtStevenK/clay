/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Cineon/DPX file plugin
 * reading and writing code Copyright (C) 1997 Peter Kirchgessner
 * e-mail: peter@kirchgessner.net, WWW: http://www.kirchgessner.net
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

#include <math.h>
#include <string.h>
#include <errno.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "cineon-lib.h"
#include "dpx-lib.h"

#include "libclay/stdplugins-intl.h"


#define LOAD_PROC       "file-cin-load"
#define EXPORT_PROC     "file-cin-export"
#define LOAD_DPX_PROC   "file-dpx-load"
#define EXPORT_DPX_PROC "file-dpx-export"
#define PLUG_IN_BINARY  "file-cin"
#define PLUG_IN_ROLE    "clay-file-cin"


typedef struct _Cineon      Cineon;
typedef struct _CineonClass CineonClass;

struct _Cineon
{
  ClayPlugIn      parent_instance;
};

struct _CineonClass
{
  ClayPlugInClass parent_class;
};


#define CINEON_TYPE  (cineon_get_type ())
#define CINEON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CINEON_TYPE, Cineon))

GType                   cineon_get_type         (void);

static GList          * cineon_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * cineon_create_procedure (ClayPlugIn            *plug_in,
                                                 const gchar           *name);

static ClayValueArray * cineon_load             (ClayProcedure         *procedure,
                                                 ClayRunMode            run_mode,
                                                 GFile                 *file,
                                                 ClayMetadata          *metadata,
                                                 ClayMetadataLoadFlags *flags,
                                                 ClayProcedureConfig   *config,
                                                 gpointer               run_data);

static ClayValueArray * dpx_load                (ClayProcedure         *procedure,
                                                 ClayRunMode            run_mode,
                                                 GFile                 *file,
                                                 ClayMetadata          *metadata,
                                                 ClayMetadataLoadFlags *flags,
                                                 ClayProcedureConfig   *config,
                                                 gpointer               run_data);

static ClayImage      * load_image              (GFile                 *file,
                                                 GObject               *config,
                                                 gboolean               is_cineon,
                                                 GError               **error);



G_DEFINE_TYPE (Cineon, cineon, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (CINEON_TYPE)
DEFINE_STD_SET_I18N


static void
cineon_class_init (CineonClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = cineon_query_procedures;
  plug_in_class->create_procedure = cineon_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
cineon_init (Cineon *cineon)
{
}

static GList *
cineon_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_DPX_PROC));

  return list;
}

static ClayProcedure *
cineon_create_procedure (ClayPlugIn  *plug_in,
                         const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           cineon_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("Kodak Cineon"));

      clay_procedure_set_documentation (procedure,
                                        _("Load file of the Kodak Cineon file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "David Hodson",
                                      "David Hodson",
                                      "1999 - 2002");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/cineon");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "cin");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,\xD7\x5F\x2A\x80,"
                                      "0,string,\x80\x2A\x5F\xD7");

    }
  else if (! strcmp (name, LOAD_DPX_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           dpx_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure,
                                     _("DPX"));

      clay_procedure_set_documentation (procedure,
                                        _("Load file of the DPX file format"),
                                        _("Load file of the Digital Picture "
                                          "Exchange file format"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "David Hodson",
                                      "David Hodson",
                                      "1999 - 2002");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/dpx");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "dpx");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,\x53\x44\x50\x58,"
                                      "0,string,\x58\x50\x44\x53");

    }

  return procedure;
}

static ClayValueArray *
cineon_load (ClayProcedure         *procedure,
             ClayRunMode            run_mode,
             GFile                 *file,
             ClayMetadata          *metadata,
             ClayMetadataLoadFlags *flags,
             ClayProcedureConfig   *config,
             gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image = NULL;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), TRUE, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
dpx_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image = NULL;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_image (file, G_OBJECT (config), FALSE, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayImage *
load_image (GFile    *file,
            GObject  *config,
            gboolean  is_cineon,
            GError  **error)
{
  ClayImage *image = NULL;

  if (is_cineon)
    image = cineon_open (file, error);
  else
    image = dpx_open (file, error);

  return image;
}
