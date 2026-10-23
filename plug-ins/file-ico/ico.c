/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Clay Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

/* #define ICO_DBG */

#include "ico.h"
#include "ani.h"
#include "ani-export.h"
#include "ani-load.h"
#include "ico-export.h"
#include "ico-load.h"

#include "libclay/stdplugins-intl.h"

#define LOAD_PROC           "file-ico-load"
#define LOAD_CUR_PROC       "file-cur-load"
#define LOAD_ANI_PROC       "file-ani-load"
#define LOAD_THUMB_PROC     "file-ico-load-thumb"
#define LOAD_ANI_THUMB_PROC "file-ani-load-thumb"
#define EXPORT_PROC         "file-ico-export"
#define EXPORT_CUR_PROC     "file-cur-export"
#define EXPORT_ANI_PROC     "file-ani-export"


typedef struct _Ico      Ico;
typedef struct _IcoClass IcoClass;

struct _Ico
{
  ClayPlugIn      parent_instance;
};

struct _IcoClass
{
  ClayPlugInClass parent_class;
};


#define ICO_TYPE  (ico_get_type ())
#define ICO(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ICO_TYPE, Ico))

GType                   ico_get_type         (void);

static GList          * ico_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * ico_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * ico_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * ani_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer              run_data);
static ClayValueArray * ico_load_thumb       (ClayProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);
static ClayValueArray * ani_load_thumb       (ClayProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);
static ClayValueArray * ico_export           (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              ClayImage            *image,
                                              GFile                *file,
                                              ClayExportOptions    *options,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);
static ClayValueArray * cur_export           (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              ClayImage            *image,
                                              GFile                *file,
                                              ClayExportOptions    *options,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);
static ClayValueArray * ani_export           (ClayProcedure        *procedure,
                                              ClayRunMode           run_mode,
                                              ClayImage            *image,
                                              GFile                *file,
                                              ClayExportOptions    *options,
                                              ClayMetadata         *metadata,
                                              ClayProcedureConfig  *config,
                                              gpointer              run_data);


G_DEFINE_TYPE (Ico, ico, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (ICO_TYPE)
DEFINE_STD_SET_I18N


static void
ico_class_init (IcoClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = ico_query_procedures;
  plug_in_class->create_procedure = ico_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
ico_init (Ico *ico)
{
}

static GList *
ico_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_ANI_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_CUR_PROC));
  list = g_list_append (list, g_strdup (LOAD_ANI_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));
  list = g_list_append (list, g_strdup (EXPORT_CUR_PROC));
  list = g_list_append (list, g_strdup (EXPORT_ANI_PROC));

  return list;
}

static ClayProcedure *
ico_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            ico_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows icon"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        "Loads files of Windows ICO file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ico");
      /* We do not set magics here, since that interferes with certain types
         of TGA images. */

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_CUR_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            ico_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows cursor"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        "Loads files of Windows CUR file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "2002-2022");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/vnd.microsoft.icon");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "cur");
      /* We do not set magics here, since that interferes with certain types
         of TGA images. */

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_ANI_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            ani_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows animated cursor"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        _("Loads files of Windows ANI file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "application/x-navi-animation");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ani");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "8,string,ACON");

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_ANI_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                ico_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Loads a preview from a Windows ICO or CUR files",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Sven Neumann <sven@gimp.org>",
                                      "2005");
    }
  else if (! strcmp (name, LOAD_ANI_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                ani_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        _("Loads a preview from a Windows ANI files"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann, James Huang, "
                                      "Alex S.",
                                      "Dom Lachowicz, "
                                      "Sven Neumann <sven@gimp.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, ico_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows icon"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        "Exports files in Windows ICO file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>",
                                      "Christian Kreibich <christian@whoop.org>",
                                      "2002");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           "Microsoft Windows icon");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-ico");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ico");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA   |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);
    }
  else if (! strcmp (name, EXPORT_CUR_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, cur_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows cursor"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        "Exports files in Windows CUR file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "Nikc M.",
                                      "2002-2022");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           "Microsoft Windows cursor");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/vnd.microsoft.icon");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "cur");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA   |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      clay_procedure_add_int32_array_argument (procedure, "hot-spot-x",
                                               "Hot spot X",
                                               "X coordinates of hot spot (one per layer)",
                                               G_PARAM_READWRITE);
      clay_procedure_add_int32_array_argument (procedure, "hot-spot-y",
                                               "Hot spot Y",
                                               "Y coordinates of hot spot (one per layer)",
                                               G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_ANI_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, ani_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("Microsoft Windows animated cursor"));
      clay_procedure_set_icon_name (procedure, CLAY_ICON_BRUSH);

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in Windows ANI file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "Christian Kreibich <christian@whoop.org>, "
                                      "James Huang, Alex S.",
                                      "2007-2022");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           "Microsoft Windows animated cursor");
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "application/x-navi-animation");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "ani");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA   |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      clay_procedure_add_string_argument (procedure, "cursor-name",
                                          "Cursor Name",
                                          _("Cursor Name (Optional)"),
                                          NULL,
                                          G_PARAM_READWRITE);

      clay_procedure_add_string_argument (procedure, "author-name",
                                          "Cursor Author",
                                          _("Cursor Author (Optional)"),
                                          NULL,
                                          G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "default-delay",
                                       "Default delay",
                                       "Default delay between frames "
                                       "in jiffies (1/60 of a second)",
                                       0, G_MAXINT, 8,
                                       G_PARAM_READWRITE);

      clay_procedure_add_int32_array_argument (procedure, "hot-spot-x",
                                               "Hot spot X",
                                               "X coordinates of hot spot (one per layer)",
                                               G_PARAM_READWRITE);
      clay_procedure_add_int32_array_argument (procedure, "hot-spot-y",
                                               "Hot spot Y",
                                               "Y coordinates of hot spot (one per layer)",
                                               G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
ico_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = ico_load_image (file, NULL, -1, &error);

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
ani_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = ani_load_image (file, FALSE,
                          NULL, NULL, &error);

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
ico_load_thumb (ClayProcedure       *procedure,
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

  image = ico_load_thumbnail_image (file,
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
ani_load_thumb (ClayProcedure        *procedure,
                GFile                *file,
                gint                  size,
                ClayProcedureConfig  *config,
                gpointer              run_data)
{
  ClayValueArray *return_vals;
  gint            width;
  gint            height;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  width  = size;
  height = size;

  image = ani_load_image (file, TRUE,
                          &width, &height, &error);

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
ico_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status;
  ClayExportReturn   export = CLAY_EXPORT_IGNORE;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  export = clay_export_options_get_image (options, &image);
  status = ico_export_image (file, image, procedure, config, run_mode, &error);

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayValueArray *
cur_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status;
  ClayExportReturn   export       = CLAY_EXPORT_IGNORE;
  GError            *error        = NULL;
  ClayArray         *x_array      = NULL;
  ClayArray         *y_array      = NULL;
  const gint32      *hot_spot_x   = NULL;
  const gint32      *hot_spot_y   = NULL;
  gint32            *new_hot_spot_x   = NULL;
  gint32            *new_hot_spot_y   = NULL;
  gsize              n_hot_spot_x = 0;
  gsize              n_hot_spot_y = 0;

  gegl_init (NULL, NULL);

  g_object_get (config,
                "hot-spot-x", &x_array,
                "hot-spot-y", &y_array,
                NULL);

  hot_spot_x = clay_int32_array_get_values (x_array, &n_hot_spot_x);
  hot_spot_y = clay_int32_array_get_values (y_array, &n_hot_spot_y);

  export = clay_export_options_get_image (options, &image);
  status = cur_export_image (file, image, procedure, config, run_mode,
                             &n_hot_spot_x, hot_spot_x, &new_hot_spot_x,
                             &n_hot_spot_y, hot_spot_y, &new_hot_spot_y,
                             &error);

  if (status == CLAY_PDB_SUCCESS)
    {
      clay_int32_array_set_values (x_array, new_hot_spot_x, n_hot_spot_x, FALSE);
      clay_int32_array_set_values (y_array, new_hot_spot_y, n_hot_spot_y, FALSE);
      g_object_set (config,
                    "hot-spot-x", x_array,
                    "hot-spot-y", y_array,
                    NULL);
      g_free (new_hot_spot_x);
      g_free (new_hot_spot_y);
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayValueArray *
ani_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status;
  ClayExportReturn   export         = CLAY_EXPORT_IGNORE;
  GError            *error          = NULL;
  gchar             *inam           = NULL;
  gchar             *iart           = NULL;
  gint               jif_rate       = 0;
  ClayArray         *x_array        = NULL;
  ClayArray         *y_array        = NULL;
  const gint32      *hot_spot_x     = NULL;
  const gint32      *hot_spot_y     = NULL;
  gint32            *new_hot_spot_x = NULL;
  gint32            *new_hot_spot_y = NULL;
  gsize              n_hot_spot_x   = 0;
  gsize              n_hot_spot_y   = 0;
  AniFileHeader      header;
  AniSaveInfo        ani_info;

  gegl_init (NULL, NULL);

  g_object_get (config,
                "cursor-name",   &inam,
                "author-name",   &iart,
                "default-delay", &jif_rate,
                "hot-spot-x",    &x_array,
                "hot-spot-y",    &y_array,
                NULL);

  /* Jiffies (1/60th of a second) used if rate chunk not present. */
  header.jif_rate = jif_rate;
  ani_info.inam = inam;
  ani_info.iart = iart;

  hot_spot_x = clay_int32_array_get_values (x_array, &n_hot_spot_x);
  hot_spot_y = clay_int32_array_get_values (y_array, &n_hot_spot_y);

  export = clay_export_options_get_image (options, &image);
  status = ani_export_image (file, image, procedure, config, run_mode,
                             &n_hot_spot_x, hot_spot_x, &new_hot_spot_x,
                             &n_hot_spot_y, hot_spot_y, &new_hot_spot_y,
                             &header, &ani_info, &error);

  if (status == CLAY_PDB_SUCCESS)
    {
      clay_int32_array_set_values (x_array, new_hot_spot_x, n_hot_spot_x, FALSE);
      clay_int32_array_set_values (y_array, new_hot_spot_y, n_hot_spot_y, FALSE);
      g_object_set (config,
                    "cursor-name",   NULL,
                    "author-name",   NULL,
                    "default-delay", header.jif_rate,
                    "hot-spot-x",    x_array,
                    "hot-spot-y",    y_array,
                    NULL);
    }

  g_free (new_hot_spot_x);
  g_free (new_hot_spot_y);
  g_free (ani_info.inam);
  g_free (ani_info.iart);
  clay_array_free (x_array);
  clay_array_free (y_array);

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  return clay_procedure_new_return_values (procedure, status, error);
}

gint
ico_rowstride (gint width,
               gint bpp)
{
  switch (bpp)
    {
    case 1:
      if ((width % 32) == 0)
        return width / 8;
      else
        return 4 * (width/32 + 1);
      break;

    case 4:
      if ((width % 8) == 0)
        return width / 2;
      else
        return 4 * (width/8 + 1);
      break;

    case 8:
      if ((width % 4) == 0)
        return width;
      else
        return 4 * (width/4 + 1);
      break;

    case 24:
      if (((width*3) % 4) == 0)
        return width * 3;
      else
        return 4 * (width*3/4+1);

    case 32:
      return width * 4;

    default:
      g_warning ("invalid bitrate: %d\n", bpp);
      g_assert_not_reached ();
      return width * (bpp/8);
    }
}

guint8 *
ico_alloc_map (gint  width,
               gint  height,
               gint  bpp,
               gint *length)
{
  gint    len = 0;
  guint8 *map = NULL;

  len = ico_rowstride (width, bpp) * height;

  *length = len;
  map = g_new0 (guint8, len);

  return map;
}

static gint
ico_sort_key_compare (const void *a,
                      const void *b)
{
  const IcoSortKey *ka = (const IcoSortKey *) a;
  const IcoSortKey *kb = (const IcoSortKey *) b;
  gint              diff;

  diff = ka->depth - kb->depth;
  if (diff != 0)
    return diff;

  diff = kb->width - ka->width;
  if (diff != 0)
    return diff;

  return kb->height - ka->height;
}

void
ico_sort_keys (IcoSortKey *keys,
               gint        n_keys)
{
  qsort (keys, n_keys, sizeof (IcoSortKey), ico_sort_key_compare);
}
