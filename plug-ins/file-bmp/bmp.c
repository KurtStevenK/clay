/* bmp.c                                          */
/* Version 0.52                                   */
/* This is a File input and output filter for the */
/* Clay. It loads and exports images in the       */
/* windows(TM) bitmap format.                     */
/* Some Parts that deal with the interaction with */
/* Clay are taken from the GIF plugin by          */
/* Peter Mattis & Spencer Kimball and from the    */
/* PCX plugin by Francisco Bustamante.            */
/*                                                */
/* Alexander.Schulz@stud.uni-karlsruhe.de         */

/* Changes:   28.11.1997 Noninteractive operation */
/*            16.03.1998 Endian-independent!!     */
/*            21.03.1998 Little Bug-fix           */
/*            06.04.1998 Bugfix in Padding        */
/*            11.04.1998 Arch. cleanup (-Wall)    */
/*                       Parses gtkrc             */
/*            14.04.1998 Another Bug in Padding   */
/*            28.04.1998 RLE-Encoding rewritten   */
/*            29.10.1998 Changes by Tor Lillqvist */
/*                       <tml@iki.fi> to support  */
/*                       16 and 32 bit images     */
/*            28.11.1998 Bug in RLE-read-padding  */
/*                       fixed.                   */
/*            19.12.1999 Resolution support added */
/*            06.05.2000 Overhaul for 16&24-bit   */
/*                       plus better OS/2 code    */
/*                       by njl195@zepler.org.uk  */
/*            29.06.2006 Full support for 16/32   */
/*                       bits bitmaps and support */
/*                       for alpha channel        */
/*                       by p.filiciak@zax.pl     */

/*
 * xyz.kainzmayer.ClayDesign
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
 * ----------------------------------------------------------------------------
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "bmp.h"
#include "bmp-load.h"
#include "bmp-export.h"

#include "libclay/stdplugins-intl.h"


typedef struct _Bmp      Bmp;
typedef struct _BmpClass BmpClass;

struct _Bmp
{
  ClayPlugIn      parent_instance;
};

struct _BmpClass
{
  ClayPlugInClass parent_class;
};


#define BMP_TYPE  (bmp_get_type ())
#define BMP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BMP_TYPE, Bmp))

GType                   bmp_get_type         (void);

static GList          * bmp_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * bmp_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * bmp_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * bmp_export           (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              ClayImage             *image,
                                              GFile                 *file,
                                              ClayExportOptions     *options,
                                              ClayMetadata          *metadata,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);



G_DEFINE_TYPE (Bmp, bmp, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (BMP_TYPE)
DEFINE_STD_SET_I18N


static void
bmp_class_init (BmpClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = bmp_query_procedures;
  plug_in_class->create_procedure = bmp_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
bmp_init (Bmp *bmp)
{
}

static GList *
bmp_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_PROC));
  list = g_list_append (list, g_strdup (EXPORT_PROC));

  return list;
}

static ClayProcedure *
bmp_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           bmp_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("Windows BMP image"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads files of Windows BMP file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alexander Schulz",
                                      "Alexander Schulz",
                                      "1997");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/bmp");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "bmp");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,BM");
    }
  else if (! strcmp (name, EXPORT_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, bmp_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "INDEXED, GRAY, RGB*");

      clay_procedure_set_menu_label (procedure, _("Windows BMP image"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("BMP"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in Windows BMP file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Alexander Schulz",
                                      "Alexander Schulz",
                                      "1997");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/bmp");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "bmp");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB   |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY  |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA |
                                              CLAY_EXPORT_CAN_HANDLE_INDEXED,
                                              NULL, NULL, NULL);

      clay_procedure_add_boolean_argument (procedure, "use-rle",
                                           _("Ru_n-Length Encoded"),
                                           _("Use run-length-encoding compression "
                                             "(only valid for 4 and 8-bit indexed images)"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "write-color-space",
                                           _("_Write color space information"),
                                           _("Whether or not to write BITMAPV5HEADER "
                                             "color space data"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "rgb-format",
                                          _("R_GB format"),
                                          _("Export format for RGB images"),
                                          clay_choice_new_with_values ("rgb-565",   RGB_565,   _("16 bit (R5 G6 B5)"),    NULL,
                                                                       "rgba-5551", RGBA_5551, _("16 bit (A1 R5 G5 B5)"), NULL,
                                                                       "rgb-555",   RGB_555,   _("16 bit (X1 R5 G5 B5)"), NULL,
                                                                       "rgb-888",   RGB_888,   _("24 bit (R8 G8 B8)"),    NULL,
                                                                       "rgba-8888", RGBA_8888, _("32 bit (A8 R8 G8 B8)"), NULL,
                                                                       "rgbx-8888", RGBX_8888, _("32 bit (X8 R8 G8 B8)"),  NULL,
                                                                       NULL),
                                          "rgb-888",
                                          G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
bmp_load (ClayProcedure         *procedure,
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

  image = load_image (file, &error);

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
bmp_export (ClayProcedure        *procedure,
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
    clay_ui_init (PLUG_IN_BINARY);

  export    = clay_export_options_get_image (options, &image);
  drawables = clay_image_list_layers (image);

  status = export_image (file, image, drawables->data, run_mode,
                         procedure, G_OBJECT (config),
                         &error);

  if (export == CLAY_EXPORT_EXPORT)
    clay_image_delete (image);

  g_list_free (drawables);
  return clay_procedure_new_return_values (procedure, status, error);
}
