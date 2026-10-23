/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * file-icns.c
 * Copyright (C) 2004 Brion Vibber <brion@pobox.com>
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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

/* #define ICNS_DBG */

#include "file-icns.h"
#include "file-icns-load.h"
#include "file-icns-export.h"

#include "libclay/stdplugins-intl.h"

#define LOAD_PROC           "file-icns-load"
#define LOAD_THUMB_PROC     "file-icns-load-thumb"
#define EXPORT_PROC         "file-icns-export"


typedef struct _Icns      Icns;
typedef struct _IcnsClass IcnsClass;

struct _Icns
{
  ClayPlugIn      parent_instance;
};

struct _IcnsClass
{
  ClayPlugInClass parent_class;
};


#define ICNS_TYPE (icns_get_type ())
#define ICNS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICNS_TYPE, Icns))

GType                   icns_get_type         (void);

static GList          * icns_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * icns_create_procedure (ClayPlugIn            *plug_in,
                                               const gchar           *name);

static ClayValueArray * icns_load             (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               GFile                 *file,
                                               ClayMetadata          *metadata,
                                               ClayMetadataLoadFlags *flags,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);
static ClayValueArray * icns_load_thumb       (ClayProcedure         *procedure,
                                               GFile                 *file,
                                               gint                   size,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);
static ClayValueArray * icns_export           (ClayProcedure         *procedure,
                                               ClayRunMode            run_mode,
                                               ClayImage             *image,
                                               GFile                 *file,
                                               ClayExportOptions     *options,
                                               ClayMetadata          *metadata,
                                               ClayProcedureConfig   *config,
                                               gpointer               run_data);


G_DEFINE_TYPE (Icns, icns, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (ICNS_TYPE)
DEFINE_STD_SET_I18N

static void
icns_class_init (IcnsClass *klass)
{
  ClayPlugInClass *plug_in_class  = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = icns_query_procedures;
  plug_in_class->create_procedure = icns_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
icns_init (Icns *icns)
{
}

static GList *
icns_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
icns_create_procedure (ClayPlugIn  *plug_in,
                       const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           icns_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Icns"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files in Apple Icon Image format"),
                                        _("Loads Apple Icon Image files."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "icns");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,\x69\x63\x6E\x73");

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                icns_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Loads a preview from an Apple Icon Image file",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, icns_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Apple Icon Image"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        "Exports files in Apple Icon Image file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Brion Vibber <brion@pobox.com>",
                                      "Brion Vibber <brion@pobox.com>",
                                      "2004");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           "Apple Icon Image");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-icns");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "icns");

      clay_export_procedure_set_support_profile (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static ClayValueArray *
icns_load (ClayProcedure         *procedure,
           ClayRunMode            run_mode,
           GFile                 *file,
           ClayMetadata          *metadata,
           ClayMetadataLoadFlags *flags,
           ClayProcedureConfig   *config,
           gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error       = NULL;

  gegl_init (NULL, NULL);

  image = icns_load_image (file, NULL, &error);

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
icns_load_thumb (ClayProcedure       *procedure,
                 GFile               *file,
                 gint                 size,
                 ClayProcedureConfig *config,
                 gpointer             run_data)
{
  ClayValueArray *return_vals;
  gint            width;
  gint            height;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = icns_load_thumbnail_image (file,
                                     &width, &height, 0, &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);
  CLAY_VALUES_SET_INT   (return_vals, 2, width);
  CLAY_VALUES_SET_INT   (return_vals, 3, height);

  clay_value_array_truncate (return_vals, 4);

  return return_vals;
}

static ClayValueArray *
icns_export (ClayProcedure        *procedure,
             ClayRunMode           run_mode,
             ClayImage            *image,
             GFile                *file,
             ClayExportOptions    *options,
             ClayMetadata         *metadata,
             ClayProcedureConfig  *config,
             gpointer              run_data)
{
  ClayPDBStatusType  status;
  GError            *error = NULL;

  gegl_init (NULL, NULL);

  status = icns_save_image (file, image, procedure, config, run_mode, &error);

  return clay_procedure_new_return_values (procedure, status, error);
}

/* Buffer should point to *at least 5 byte buffer*! */
void
fourcc_get_string (gchar *fourcc,
                   gchar *buf)
{
  buf = fourcc;
  buf[4] = 0;
}
