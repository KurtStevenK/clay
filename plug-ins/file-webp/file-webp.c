/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-webp - WebP file format plug-in for the Clay
 * Copyright (C) 2015  Nathan Osman
 * Copyright (C) 2016  Ben Touchette
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#include <webp/encode.h>

#include "file-webp-dialog.h"
#include "file-webp-load.h"
#include "file-webp-export.h"
#include "file-webp.h"

#include "libclay/stdplugins-intl.h"


typedef struct _Webp      Webp;
typedef struct _WebpClass WebpClass;

struct _Webp
{
  ClayPlugIn      parent_instance;
};

struct _WebpClass
{
  ClayPlugInClass parent_class;
};


#define WEBP_TYPE  (webp_get_type ())
#define WEBP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WEBP_TYPE, Webp))

GType                           webp_get_type         (void);

static GList                  * webp_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure          * webp_create_procedure (ClayPlugIn            *plug_in,
                                                       const gchar           *name);

static ClayValueArray         * webp_load             (ClayProcedure         *procedure,
                                                       ClayRunMode            run_mode,
                                                       GFile                 *file,
                                                       ClayMetadata          *metadata,
                                                       ClayMetadataLoadFlags *flags,
                                                       ClayProcedureConfig   *config,
                                                       gpointer               run_data);
static ClayValueArray         * webp_export           (ClayProcedure         *procedure,
                                                       ClayRunMode            run_mode,
                                                       ClayImage             *image,
                                                       GFile                 *file,
                                                       ClayExportOptions     *options,
                                                       ClayMetadata          *metadata,
                                                       ClayProcedureConfig   *config,
                                                       gpointer               run_data);

static ClayExportCapabilities   export_edit_options   (ClayProcedure        *procedure,
                                                       ClayProcedureConfig  *config,
                                                       ClayExportOptions    *options,
                                                       gpointer              create_data);


G_DEFINE_TYPE (Webp, webp, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (WEBP_TYPE)
DEFINE_STD_SET_I18N


static void
webp_class_init (WebpClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = webp_query_procedures;
  plug_in_class->create_procedure = webp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
webp_init (Webp *webp)
{
}

static GList *
webp_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
webp_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           webp_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("WebP image"));

      clay_procedure_set_documentation (procedure,
                                        "Loads images in the WebP file format",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/webp");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "webp");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "8,string,WEBP");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             TRUE, webp_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "*");

      clay_procedure_set_menu_label (procedure, _("WebP image"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in the WebP image format"),
                                        _("Exports files in the WebP image format"),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Nathan Osman, Ben Touchette",
                                      "(C) 2015-2016 Nathan Osman, "
                                      "(C) 2016 Ben Touchette",
                                      "2015,2016");

      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("WebP"));
      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/webp");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "webp");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              0, export_edit_options, NULL, NULL);

      clay_procedure_add_choice_argument (procedure, "preset",
                                          _("Source _type"),
                                          _("WebP encoder preset"),
                                          clay_choice_new_with_values ("default", WEBP_PRESET_DEFAULT, _("Default"), NULL,
                                                                       "picture", WEBP_PRESET_PICTURE, _("Picture"), NULL,
                                                                       "photo",   WEBP_PRESET_PHOTO,   _("Photo"),   NULL,
                                                                       "drawing", WEBP_PRESET_DRAWING, _("Drawing"), NULL,
                                                                       "icon",    WEBP_PRESET_ICON,    _("Icon"),    NULL,
                                                                       "text",    WEBP_PRESET_TEXT,    _("Text"),    NULL,
                                                                       NULL),
                                          "default",
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "lossless",
                                           _("L_ossless"),
                                           _("Use lossless encoding"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_double_argument (procedure, "quality",
                                          _("Image _quality"),
                                          _("Quality of the image"),
                                          0, 100, 90,
                                          G_PARAM_READWRITE);

      clay_procedure_add_double_argument (procedure, "alpha-quality",
                                          _("Alpha q_uality"),
                                          _("Quality of the image's alpha channel"),
                                          0, 100, 100,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "use-sharp-yuv",
                                           _("Use Sharp YU_V"),
                                           /* TRANSLATORS: \xe2\x86\x92 is a Unicode
                                            * "Rightward Arrow" in UTF-8 encoding.
                                            */
                                           _("Use sharper (but slower) RGB\xe2\x86\x92YUV conversion"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "animation-loop",
                                           _("Loop _forever"),
                                           _("Loop animation infinitely"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "minimize-size",
                                           _("_Minimize output size (slower)"),
                                           _("Minimize output file size"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "keyframe-distance",
                                       _("Max distance between _key-frames"),
                                       _("Maximum distance between keyframes"),
                                       0, G_MAXINT, 50,
                                       G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "default-delay",
                                       _("_Default delay between frames"),
                                       _("Default delay (in milliseconds) to use when timestamps"
                                         " for frames are not available or forced."),
                                       0, G_MAXINT, 200,
                                       G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "force-delay",
                                           _("Use default dela_y for all frames"),
                                           _("Force default delay on all frames"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "animation",
                                           _("Export a_nimation"),
                                           _("Use layers for animation"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_export_procedure_set_support_exif      (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
      clay_export_procedure_set_support_iptc      (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
      clay_export_procedure_set_support_xmp       (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
      clay_export_procedure_set_support_profile   (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
      clay_export_procedure_set_support_thumbnail (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
    }

  return procedure;
}

static ClayValueArray *
webp_load (ClayProcedure         *procedure,
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

  image = load_image (file, FALSE, flags, &error);

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
webp_export (ClayProcedure        *procedure,
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
  gint               n_drawables;
  gboolean           animation;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);

      if (! save_dialog (image, procedure, G_OBJECT (config)))
        return clay_procedure_new_return_values (procedure,
                                                 CLAY_PDB_CANCEL,
                                                 NULL);
    }

  g_object_get (config,
                "animation", &animation,
                NULL);

  if (status == CLAY_PDB_SUCCESS)
    export = clay_export_options_get_image (options, &image);

  drawables   = clay_image_list_layers (image);
  n_drawables = g_list_length (drawables);

  if (animation)
    {
      if (! save_animation (file, image, n_drawables, drawables, G_OBJECT (config),
                            &error))
        {
          status = CLAY_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      if (! save_layer (file, image, drawables->data, G_OBJECT (config),
                        &error))
        {
          status = CLAY_PDB_EXECUTION_ERROR;
        }
    }

  if (status == CLAY_PDB_SUCCESS && metadata)
    {
      gboolean save_xmp;

      /* WebP doesn't support iptc natively and sets it via xmp */
      g_object_get (config,
                    "include-xmp", &save_xmp,
                    NULL);
      g_object_set (config,
                    "include-iptc", save_xmp,
                    NULL);

      clay_metadata_set_bits_per_sample (metadata, 8);
    }

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}

static ClayExportCapabilities
export_edit_options (ClayProcedure       *procedure,
                     ClayProcedureConfig *config,
                     ClayExportOptions   *options,
                     gpointer             create_data)
{
  ClayExportCapabilities capabilities;
  gboolean               animation;

  g_object_get (G_OBJECT (config),
                "animation", &animation,
                NULL);

  capabilities = (CLAY_EXPORT_CAN_HANDLE_RGB     |
                  CLAY_EXPORT_CAN_HANDLE_GRAY    |
                  CLAY_EXPORT_CAN_HANDLE_INDEXED |
                  CLAY_EXPORT_CAN_HANDLE_ALPHA);

  if (animation)
    capabilities |= CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION;

  return capabilities;
}
