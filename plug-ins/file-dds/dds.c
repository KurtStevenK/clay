/*
 * DDS Clay plugin
 *
 * Copyright (C) 2004-2012 Shawn Kirst <skirst@gmail.com>,
 * with parts (C) 2003 Arne Reuter <homepage@arnereuter.de> where specified.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include <libclay/stdplugins-intl.h>

#include "dds.h"
#include "ddsread.h"
#include "ddswrite.h"
#include "misc.h"


#define LOAD_PROC                "file-dds-load"
#define EXPORT_PROC              "file-dds-export"


typedef struct _Dds      Dds;
typedef struct _DdsClass DdsClass;

struct _Dds
{
  ClayPlugIn      parent_instance;
};

struct _DdsClass
{
  ClayPlugInClass parent_class;
};


#define DDS_TYPE  (dds_get_type ())
#define DDS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DDS_TYPE, Dds))

GType                   dds_get_type         (void);

static GList          * dds_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * dds_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * dds_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * dds_export           (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              ClayImage             *image,
                                              GFile                 *file,
                                              ClayExportOptions     *options,
                                              ClayMetadata          *metadata,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);


G_DEFINE_TYPE (Dds, dds, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (DDS_TYPE)
DEFINE_STD_SET_I18N


static void
dds_class_init (DdsClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = dds_query_procedures;
  plug_in_class->create_procedure = dds_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
dds_init (Dds *dds)
{
}

static GList *
dds_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
dds_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           dds_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("DDS image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files in DDS image format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/vnd.ms-dds,image/x-dds,image/dds");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "dds");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,DDS");

      clay_procedure_add_boolean_argument (procedure, "load-mipmaps",
                                           _("Load _mipmaps"),
                                           _("Load mipmaps if present"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "flip-image",
                                           _("Flip image _vertically"),
                                           _("Flip the image vertically on import"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      ClayChoice *choice;

      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, dds_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB");

      clay_procedure_set_menu_label (procedure, _("DDS image"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("DDS"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in DDS image format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Shawn Kirst",
                                      "Shawn Kirst",
                                      "2008");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/dds");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "dds");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB     |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY    |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA   |
                                              CLAY_EXPORT_CAN_HANDLE_LAYERS,
                                              NULL, NULL, NULL);

      choice = clay_choice_new_with_values ("none",   DDS_COMPRESS_NONE,   _("None"),                  NULL,
                                            "bc1",    DDS_COMPRESS_BC1,    _("BC1 / DXT1"),            NULL,
                                            "bc2",    DDS_COMPRESS_BC2,    _("BC2 / DXT3"),            NULL,
                                            "bc3",    DDS_COMPRESS_BC3,    _("BC3 / DXT5"),            NULL,
                                            "bc3n",   DDS_COMPRESS_BC3N,   _("BC3nm / DXT5nm"),        NULL,
                                            "bc4",    DDS_COMPRESS_BC4,    _("BC4 / ATI1 (3Dc+)"),     NULL,
                                            "bc5",    DDS_COMPRESS_BC5,    _("BC5 / ATI2 (3Dc)"),      NULL,
                                            "bc7",    DDS_COMPRESS_BC7,    "BC7",                      NULL,
                                            "rxgb",   DDS_COMPRESS_RXGB,   _("RXGB (DXT5)"),           NULL,
                                            "aexp",   DDS_COMPRESS_AEXP,   _("Alpha Exponent (DXT5)"), NULL,
                                            "ycocg",  DDS_COMPRESS_YCOCG,  _("YCoCg (DXT5)"),          NULL,
                                            "ycocgs", DDS_COMPRESS_YCOCGS, _("YCoCg scaled (DXT5)"),   NULL,
                                            NULL);
      clay_choice_add_deprecated (choice, "bc3, ", DDS_COMPRESS_BC3, "bc3", NULL);

      clay_procedure_add_choice_argument (procedure, "compression-format",
                                          _("Compressio_n"),
                                          _("Compression format"),
                                          choice,
                                          "none",
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "perceptual-metric",
                                           _("Use percept_ual error metric"),
                                           _("Use a perceptual error metric during compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      choice = clay_choice_new_with_values ("default", DDS_FORMAT_DEFAULT, _("Default"), NULL,
                                            "rgb8",    DDS_FORMAT_RGB8,    _("RGB8"),    NULL,
                                            "rgba8",   DDS_FORMAT_RGBA8,   _("RGBA8"),   NULL,
                                            "bgr8",    DDS_FORMAT_BGR8,    _("BGR8"),    NULL,
                                            "abgr8",   DDS_FORMAT_ABGR8,   _("ABGR8"),   NULL,
                                            "r5g6b5",  DDS_FORMAT_R5G6B5,  _("R5G6B5"),  NULL,
                                            "rgba4",   DDS_FORMAT_RGBA4,   _("RGBA4"),   NULL,
                                            "rgb5a1",  DDS_FORMAT_RGB5A1,  _("RGB5A1"),  NULL,
                                            "rgb10a2", DDS_FORMAT_RGB10A2, _("RGB10A2"), NULL,
                                            "r3g3b2",  DDS_FORMAT_R3G3B2,  _("R3G3B2"),  NULL,
                                            "a8",      DDS_FORMAT_A8,      _("A8"),      NULL,
                                            "l8",      DDS_FORMAT_L8,      _("L8"),      NULL,
                                            "l8a8",    DDS_FORMAT_L8A8,    _("L8A8"),    NULL,
                                            "aexp",    DDS_FORMAT_AEXP,    _("AEXP"),    NULL,
                                            "ycocg",   DDS_FORMAT_YCOCG,   _("YCOCG"),   NULL,
                                            NULL);
      clay_choice_add_deprecated (choice, "abgr8, ", DDS_FORMAT_ABGR8, "abgr8", NULL);

      clay_procedure_add_choice_argument (procedure, "format",
                                          _("_Format"),
                                          _("Pixel format"),
                                          choice,
                                          "default",
                                          G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "save-type",
                                          _("Sav_e type"),
                                          _("How to export the image"),
                                          clay_choice_new_with_values ("layer",  DDS_SAVE_SELECTED_LAYER, _("Selected layer"),     NULL,
                                                                       "canvas", DDS_SAVE_VISIBLE_LAYERS, _("All visible layers"), NULL,
                                                                       "cube",   DDS_SAVE_CUBEMAP,        _("As cube map"),        NULL,
                                                                       "volume", DDS_SAVE_VOLUMEMAP,      _("As volume map"),      NULL,
                                                                       "array",  DDS_SAVE_ARRAY,          _("As texture array"),   NULL,
                                                                       NULL),
                                          "layer",
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "flip-image",
                                           _("Flip image _vertically on export"),
                                           _("Flip the image vertically on export"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "transparent-color",
                                           _("Set _transparent color"),
                                           _("Make an indexed color transparent"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "transparent-index",
                                       _("Transparent inde_x"),
                                       _("Index of transparent color or -1 to disable "
                                         "(for indexed images only)."),
                                       0, 255, 0,
                                       G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "mipmaps",
                                          _("_Mipmaps"),
                                          _("How to handle mipmaps"),
                                          clay_choice_new_with_values ("none",     DDS_MIPMAP_NONE,     _("No mipmaps"),           NULL,
                                                                       "generate", DDS_MIPMAP_GENERATE, _("Generate mipmaps"),     NULL,
                                                                       "existing", DDS_MIPMAP_EXISTING, _("Use existing mipmaps"), NULL,
                                                                       NULL),
                                          "none",
                                          G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "mipmap-filter",
                                          _("F_ilter"),
                                          _("Filtering to use when generating mipmaps"),
                                          clay_choice_new_with_values ("default",   DDS_MIPMAP_FILTER_DEFAULT,   _("Default"),     NULL,
                                                                       "nearest",   DDS_MIPMAP_FILTER_NEAREST,   _("Nearest"),     NULL,
                                                                       "box",       DDS_MIPMAP_FILTER_BOX,       _("Box"),         NULL,
                                                                       "triangle",  DDS_MIPMAP_FILTER_TRIANGLE,  _("Triangle"),    NULL,
                                                                       "quadratic", DDS_MIPMAP_FILTER_QUADRATIC, _("Quadratic"),   NULL,
                                                                       "bspline",   DDS_MIPMAP_FILTER_BSPLINE,   _("B-Spline"),    NULL,
                                                                       "mitchell",  DDS_MIPMAP_FILTER_MITCHELL,  _("Mitchell"),    NULL,
                                                                       "catrom",    DDS_MIPMAP_FILTER_CATROM,    _("Catmull-Rom"), NULL,
                                                                       "lanczos",   DDS_MIPMAP_FILTER_LANCZOS,   _("Lanczos"),     NULL,
                                                                       "kaiser",    DDS_MIPMAP_FILTER_KAISER,    _("Kaiser"),      NULL,
                                                                       NULL),
                                          "default",
                                          G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "mipmap-wrap",
                                          _("_Wrap mode"),
                                          _("Wrap mode to use when generating mipmaps"),
                                          clay_choice_new_with_values ("default", DDS_MIPMAP_WRAP_DEFAULT, _("Default"), NULL,
                                                                       "mirror",  DDS_MIPMAP_WRAP_MIRROR,  _("Mirror"),  NULL,
                                                                       "repeat",  DDS_MIPMAP_WRAP_REPEAT,  _("Repeat"),  NULL,
                                                                       "clamp",   DDS_MIPMAP_WRAP_CLAMP,   _("Clamp"),   NULL,
                                                                       NULL),
                                          "default",
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "gamma-correct",
                                           _("Appl_y gamma correction"),
                                           _("Use gamma correct mipmap filtering"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "srgb",
                                           _("Use sRG_B colorspace"),
                                           _("Use sRGB colorspace for gamma correction"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_double_argument (procedure, "gamma",
                                          _("_Gamma"),
                                          _("Gamma value to use for gamma correction (e.g. 2.2)"),
                                          0.0, 10.0, 0.0,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "preserve-alpha-coverage",
                                           _("Preserve al_pha test coverage"),
                                           _("Preserve alpha test coverage for alpha "
                                             "channel maps"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_double_argument (procedure, "alpha-test-threshold",
                                          _("Alp_ha test threshold"),
                                          _("Alpha test threshold value for which alpha test "
                                            "coverage should be preserved"),
                                          0.0, 1.0, 0.5,
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
dds_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray      *return_vals;
  ClayPDBStatusType    status;
  ClayImage           *image;
  GError              *error = NULL;

  gegl_init (NULL, NULL);

  status = read_dds (file, &image, run_mode == CLAY_RUN_INTERACTIVE,
                     procedure, config, &error);

  if (status != CLAY_PDB_SUCCESS)
    return clay_procedure_new_return_values (procedure, status, error);

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
dds_export (ClayProcedure        *procedure,
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
  ClayLayer        **drawables;
  GError            *error  = NULL;
  gdouble            gamma;

  gegl_init (NULL, NULL);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    clay_ui_init ("dds");

  export    = clay_export_options_get_image (options, &image);
  drawables = clay_image_get_selected_layers (image);

  g_object_get (config,
                "gamma", &gamma,
                NULL);

  /* clay_gamma () got removed and was always returning 2.2 anyway.
   * XXX Review this piece of code if we expect gamma value could be
   * parameterized.
   */
  if (gamma < 1e-04f)
    g_object_set (config,
                  "gamma", 2.2,
                  NULL);

  /* TODO: support multiple-layers selection, especially as DDS has
   * DDS_SAVE_SELECTED_LAYER option support.
   */
  status = write_dds (file, image, CLAY_DRAWABLE (drawables[0]),
                      run_mode == CLAY_RUN_INTERACTIVE,
                      procedure, config,
                      export == CLAY_EXPORT_EXPORT);

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}
