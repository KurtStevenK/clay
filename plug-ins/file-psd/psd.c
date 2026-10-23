/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Clay PSD Plug-in
 * Copyright 2007 by John Marshall
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

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "psd.h"
#include "psd-load.h"
#include "psd-export.h"
#include "psd-thumb-load.h"

#include "libclay/stdplugins-intl.h"


typedef struct _Psd      Psd;
typedef struct _PsdClass PsdClass;

struct _Psd
{
  ClayPlugIn      parent_instance;
};

struct _PsdClass
{
  ClayPlugInClass parent_class;
};


#define PSD_TYPE  (psd_get_type ())
#define PSD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), PSD_TYPE, Psd))

GType                   psd_get_type         (void);

static GList          * psd_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * psd_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * psd_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer              run_data);
static ClayValueArray * psd_load_thumb       (ClayProcedure         *procedure,
                                              GFile                 *file,
                                              gint                   size,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * psd_export           (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              ClayImage             *image,
                                              GFile                 *file,
                                              ClayExportOptions     *options,
                                              ClayMetadata          *metadata,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * psd_load_metadata    (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * psd_export_metadata  (ClayProcedure         *procedure,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);


G_DEFINE_TYPE (Psd, psd, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (PSD_TYPE)
DEFINE_STD_SET_I18N


static void
psd_class_init (PsdClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = psd_query_procedures;
  plug_in_class->create_procedure = psd_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
psd_init (Psd *psd)
{
}

static GList *
psd_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (LOAD_MERGED_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PSB_PROC));
  list = g_list_append (list, g_strdup (LOAD_METADATA_PROC));
  list = g_list_append (list, g_strdup (EXPORT_METADATA_PROC));

  return list;
}

static ClayProcedure *
psd_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Photoshop image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads images from the Photoshop "
                                          "PSD and PSB file formats"),
                                        _("This plug-in loads images in Adobe "
                                          "Photoshop (TM) native PSD and PSB format."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_MERGED_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           psd_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Photoshop image (merged)"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads images from the Photoshop "
                                          "PSD and PSB file formats"),
                                        _("This plug-in loads the merged image "
                                          "data in Adobe Photoshop (TM) native "
                                          "PSD and PSB format."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Ell",
                                      "Ell",
                                      "2018");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-psd");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "psd, psb");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,8BPS");
      clay_file_procedure_set_priority (CLAY_FILE_PROCEDURE (procedure), +1);

      clay_load_procedure_set_thumbnail_loader (CLAY_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = clay_thumbnail_procedure_new (plug_in, name,
                                                CLAY_PDB_PROC_TYPE_PLUGIN,
                                                psd_load_thumb, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        _("Loads thumbnails from the "
                                          "Photoshop PSD file format"),
                                        _("This plug-in loads thumbnail images "
                                          "from Adobe Photoshop (TM) native "
                                          "PSD format files."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");
    }
  else if (! strcmp (name, EXPORT_PROC) ||
           ! strcmp (name, EXPORT_PSB_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, psd_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      if (! strcmp (name, EXPORT_PROC))
        {
          clay_procedure_set_menu_label (procedure, _("Photoshop image"));
          clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                               _("Photoshop image"));

          clay_procedure_set_documentation (procedure,
                                            _("Exports files in the Photoshop (TM) "
                                              "PSD file format"),
                                            _("This plug-in exports files of Adobe "
                                              "Photoshop (TM) native PSD format. "
                                              "These files may be of any image type "
                                              "supported by Clay, with or without "
                                              "layers, layer masks, aux channels "
                                              "and guides."),
                                            name);
          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              "image/x-psd");
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              "psd");
        }
      else
        {
          clay_procedure_set_menu_label (procedure, _("Photoshop Large image"));
          clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                               _("Photoshop Large image"));

          clay_procedure_set_documentation (procedure,
                                            _("Exports files in the Photoshop (TM) "
                                              "Large PSB file format"),
                                            _("This plug-in exports files of Adobe "
                                              "Photoshop (TM) Large native PSB format. "
                                              "These files may be of any image type "
                                              "supported by Clay, with or without "
                                              "layers, layer masks, aux channels "
                                              "and guides."),
                                            name);
          clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                              "image/x-psb");
          clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                              "psb");
        }

      clay_procedure_set_attribution (procedure,
                                      "Monigotes",
                                      "Monigotes",
                                      "2000");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA   |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS  |
                                              CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS,
                                              NULL, NULL, NULL);

      clay_procedure_add_boolean_argument (procedure, "clippingpath",
                                           _("Assign a Clipping _Path"),
                                           _("Select a path to be the "
                                           "clipping path"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_string_argument (procedure, "clippingpathname",
                                          _("Clipping Path _Name"),
                                          _("Clipping path name\n"
                                            "(ignored if no clipping path)"),
                                          NULL,
                                          G_PARAM_READWRITE);

      clay_procedure_add_double_argument (procedure, "clippingpathflatness",
                                          _("Path _Flatness"),
                                          _("Clipping path flatness in device pixels\n"
                                            "(ignored if no clipping path)"),
                                          0.0, 100.0, 0.2,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "cmyk",
                                           _("Export as _CMYK"),
                                           _("Export a CMYK PSD image using the soft-proofing color profile"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "duotone",
                                           _("Export as _Duotone"),
                                           _("Export as a Duotone PSD file if Duotone color space information "
                                           "was attached to the image when originally imported."),
                                           FALSE,
                                           G_PARAM_READWRITE);

      /* TODO: Remove condition when PSB metadata is supported by Exiv2 */
      if (! strcmp (name, EXPORT_PROC))
        {
          clay_export_procedure_set_support_exif      (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
          clay_export_procedure_set_support_iptc      (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
          clay_export_procedure_set_support_xmp       (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
          clay_export_procedure_set_support_profile   (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
          clay_export_procedure_set_support_thumbnail (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
        }
    }
  else if (! strcmp (name, LOAD_METADATA_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           psd_load_metadata, NULL, NULL);

      clay_procedure_set_documentation (procedure,
                                        "Loads Photoshop-format metadata "
                                        "from other file formats.",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "John Marshall",
                                      "John Marshall",
                                      "2007");
      clay_procedure_add_int_argument (procedure, "size",
                                       "Metadata size",
                                       NULL,
                                       0, G_MAXINT, 0,
                                       G_PARAM_READWRITE);
      clay_procedure_add_image_argument (procedure, "image",
                                         "image", "The image",
                                         FALSE,
                                         CLAY_PARAM_READWRITE);
      clay_procedure_add_boolean_argument (procedure, "metadata-type",
                                           "Metadata type",
                                           "If the metadata contains image or "
                                           "layer PSD resources.",
                                           FALSE,
                                           G_PARAM_READWRITE);
      clay_procedure_add_boolean_argument (procedure, "cmyk",
                                           "CMYK",
                                           "If the layer metadata needs to be "
                                           "converted from CMYK colorspace.",
                                           FALSE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_METADATA_PROC))
    {
      procedure = clay_procedure_new (plug_in, name,
                                      CLAY_PDB_PROC_TYPE_PLUGIN,
                                      psd_export_metadata, NULL, NULL);
      clay_procedure_set_documentation (procedure,
                                        "Exports Photoshop-format metadata "
                                        "to other file formats.",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Frank Teklote",
                                      "Frank Teklote",
                                      "2026");
      clay_procedure_add_file_argument (procedure, "file",
                                        "File",
                                        "Output file for exported metadata",
                                        CLAY_FILE_CHOOSER_ACTION_SAVE,
                                        FALSE, NULL,
                                        G_PARAM_READWRITE);
      clay_procedure_add_image_argument (procedure, "image",
                                         "image",
                                         "The image",
                                         FALSE,
                                         G_PARAM_READWRITE);
      clay_procedure_add_boolean_argument (procedure, "metadata-type",
                                           "Metadata type",
                                           "Whether the metadata contains "
                                           "image or layer PSD resources.",
                                           FALSE,
                                           G_PARAM_READWRITE);
      clay_procedure_add_boolean_argument (procedure, "cmyk",
                                           "Export as CMYK",
                                           "Export a CMYK PSD metadata using the "
                                           "soft-proofing color profile",
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
psd_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray *return_vals;
  gboolean        resolution_loaded = FALSE;
  gboolean        profile_loaded    = FALSE;
  ClayImage      *image;
  ClayParasite   *parasite = NULL;
  GError         *error = NULL;
  PSDSupport      unsupported_features;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
    case CLAY_RUN_WITH_LAST_VALS:
      clay_ui_init (PLUG_IN_BINARY);
      break;
    default:
      break;
    }

  image = load_image (file,
                      strcmp (clay_procedure_get_name (procedure),
                              LOAD_MERGED_PROC) == 0,
                      &resolution_loaded,
                      &profile_loaded,
                      &unsupported_features,
                      &error);

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  /* If image was Duotone, notify user of compatibility */
  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      parasite = clay_image_get_parasite (image, PSD_PARASITE_DUOTONE_DATA);
      if (parasite)
        unsupported_features.duotone_mode = TRUE;

      if (unsupported_features.duotone_mode ||
          unsupported_features.show_gui)
        load_dialog (_("Import PSD"), &unsupported_features);

      if (parasite)
        clay_parasite_free (parasite);
    }

  if (resolution_loaded)
    *flags &= ~CLAY_METADATA_LOAD_RESOLUTION;

  if (profile_loaded)
    *flags &= ~CLAY_METADATA_LOAD_COLORSPACE;

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
psd_load_thumb (ClayProcedure       *procedure,
                GFile               *file,
                gint                 size,
                ClayProcedureConfig *config,
                gpointer             run_data)
{
  ClayValueArray *return_vals;
  gint            width  = 0;
  gint            height = 0;
  ClayImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  image = load_thumbnail_image (file, &width, &height, &error);

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
psd_export (ClayProcedure        *procedure,
            ClayRunMode           run_mode,
            ClayImage            *image,
            GFile                *file,
            ClayExportOptions    *options,
            ClayMetadata         *metadata,
            ClayProcedureConfig  *config,
            gpointer              run_data)
{
  ClayPDBStatusType  status = CLAY_PDB_SUCCESS;
  ClayExportReturn   export = CLAY_EXPORT_IGNORE;
  GList             *drawables;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (image, procedure, G_OBJECT (config)))
        return clay_procedure_new_return_values (procedure, CLAY_PDB_CANCEL,
                                                 NULL);
    }

  export    = clay_export_options_get_image (options, &image);
  drawables = clay_image_list_layers (image);

  if (export_image (file, image, procedure, G_OBJECT (config), &error))
    {
      if (metadata)
        clay_metadata_set_bits_per_sample (metadata, 8);
    }
  else
    {
      status = CLAY_PDB_EXECUTION_ERROR;
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayValueArray *
psd_load_metadata (ClayProcedure         *procedure,
                   ClayRunMode            run_mode,
                   GFile                 *file,
                   ClayMetadata          *metadata,
                   ClayMetadataLoadFlags *flags,
                   ClayProcedureConfig   *config,
                   gpointer               run_data)
{
  ClayValueArray *return_vals;
  ClayImage      *image;
  gint            data_length;
  PSDSupport      unsupported_features;
  gboolean        is_layer = FALSE;
  gboolean        is_cmyk  = FALSE;
  GError         *error    = NULL;

  gegl_init (NULL, NULL);

  /* Retrieve image */
  g_object_get (config,
                "image",         &image,
                "size",          &data_length,
                "metadata-type", &is_layer,
                "cmyk",          &is_cmyk,
                NULL);

  image = load_image_metadata (file, data_length, image, is_layer, is_cmyk,
                               &unsupported_features, &error);

  /* Check for unsupported layers */

  if (is_layer && unsupported_features.show_gui)
    {
      /* Metadata doesn't store rasterized versions of fill layers,
       * (unlike PSDs) so we can't display them for now.
       */
      if (unsupported_features.fill_layer)
        {
          unsupported_features.psd_metadata = TRUE;
          unsupported_features.fill_layer   = FALSE;
        }

      switch (run_mode)
        {
        case CLAY_RUN_INTERACTIVE:
          clay_ui_init (PLUG_IN_BINARY);
          load_dialog (_("Import PSD metadata"), &unsupported_features);
          break;
        default:
          g_printerr ("[%s] %s\n", "file-psd-load-metadata",
                      _("Metadata fill layers are not supported "
                        "and will show up as empty layers."));
          break;
        }

    }

  if (! image)
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  g_object_unref (image);

  return return_vals;
}

static ClayValueArray *
psd_export_metadata (ClayProcedure       *procedure,
                     ClayProcedureConfig *config,
                     gpointer             run_data)
{
  GFile             *file     = NULL;
  ClayImage         *image    = NULL;
  gboolean           is_layer = FALSE;
  gboolean           cmyk     = FALSE;
  ClayPDBStatusType  status   = CLAY_PDB_EXECUTION_ERROR;
  GError            *error    = NULL;

  gegl_init (NULL, NULL);

  g_object_get (config,
                "file",          &file,
                "image",         &image,
                "metadata-type", &is_layer,
                "cmyk",          &cmyk,
                NULL);

  if (export_image_metadata (file, image, is_layer, cmyk, &error))
    status = CLAY_PDB_SUCCESS;

  if (file)
    g_object_unref (file);

  if (image)
    g_object_unref (image);

  return clay_procedure_new_return_values (procedure, status, error);
}
