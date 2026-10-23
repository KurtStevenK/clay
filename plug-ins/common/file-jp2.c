/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jp2.c -- JPEG 2000 file format plug-in
 * Copyright (C) 2009 Aurimas Juška <aurimas.juska@gmail.com>
 * Copyright (C) 2004 Florian Traverse <florian.traverse@cpe.fr>
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
 * Portions of this plug-in code (color conversion, etc.) were imported
 * from the OpenJPEG project covered under the following GNU GPL
 * compatible license:
 *
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"

#include <openjpeg.h>


#define LOAD_JP2_PROC      "file-jp2-load"
#define LOAD_J2K_PROC      "file-j2k-load"
#define EXPORT_JP2_PROC    "file-jp2-export"
#define EXPORT_J2K_PROC    "file-j2k-export"
#define PLUG_IN_BINARY     "file-jp2-load"
#define PLUG_IN_ROLE       "clay-file-jp2-load"


typedef struct _Jp2      Jp2;
typedef struct _Jp2Class Jp2Class;

struct _Jp2
{
  ClayPlugIn      parent_instance;
};

struct _Jp2Class
{
  ClayPlugInClass parent_class;
};


#define JP2_TYPE  (jp2_get_type ())
#define JP2 (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), JP2_TYPE, Jp2))

GType                   jp2_get_type         (void);

static GList          * jp2_query_procedures (ClayPlugIn            *plug_in);
static ClayProcedure  * jp2_create_procedure (ClayPlugIn            *plug_in,
                                              const gchar           *name);

static ClayValueArray * jp2_load             (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              GFile                 *file,
                                              ClayMetadata          *metadata,
                                              ClayMetadataLoadFlags *flags,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);
static ClayValueArray * jp2_export           (ClayProcedure         *procedure,
                                              ClayRunMode            run_mode,
                                              ClayImage             *image,
                                              GFile                 *file,
                                              ClayExportOptions     *options,
                                              ClayMetadata          *metadata,
                                              ClayProcedureConfig   *config,
                                              gpointer               run_data);

static ClayImage      * load_image           (ClayProcedure         *procedure,
                                              GObject               *config,
                                              GFile                 *file,
                                              OPJ_CODEC_FORMAT       format,
                                              OPJ_COLOR_SPACE        color_space,
                                              gboolean               interactive,
                                              gboolean              *profile_loaded,
                                              GError               **error);
static ClayPDBStatusType  export_image       (GFile                 *file,
                                              ClayImage             *image,
                                              ClayDrawable          *drawable,
                                              ClayRunMode            run_mode,
                                              ClayProcedure         *procedure,
                                              GObject               *config,
                                              GError               **error);

static OPJ_COLOR_SPACE  open_dialog          (ClayProcedure         *procedure,
                                              GObject               *config,
                                              GFile                 *file,
                                              OPJ_CODEC_FORMAT       format,
                                              gint                   num_components,
                                              GError               **error);
static gboolean         export_dialog        (ClayProcedure         *procedure,
                                              ClayProcedureConfig   *config,
                                              ClayImage             *image);

/* Helper Method */
static gint             get_bpc              (ClayImage             *image);


G_DEFINE_TYPE (Jp2, jp2, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (JP2_TYPE)
DEFINE_STD_SET_I18N


static void
jp2_class_init (Jp2Class *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = jp2_query_procedures;
  plug_in_class->create_procedure = jp2_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
jp2_init (Jp2 *jp2)
{
}

static GList *
jp2_query_procedures (ClayPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_JP2_PROC));
  list = g_list_append (list, g_strdup (LOAD_J2K_PROC));
  list = g_list_append (list, g_strdup (EXPORT_JP2_PROC));
  list = g_list_append (list, g_strdup (EXPORT_J2K_PROC));

  return list;
}

static ClayProcedure *
jp2_create_procedure (ClayPlugIn  *plug_in,
                      const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_JP2_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           jp2_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("JPEG 2000 image"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000"));

      clay_procedure_set_documentation (procedure,
                                        "Loads JPEG 2000 images.",
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Aurimas Juška",
                                      "Aurimas Juška, Florian Traverse",
                                      "2009");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/jp2");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "jp2");

      /* XXX: more complete magic number would be:
       * "0,string,\x00\x00\x00\x0C\x6A\x50\x20\x20\x0D\x0A\x87\x0A"
       * But the '\0' character makes problem in a 0-terminated string
       * obviously, as well as some other space characters, it would
       * seem. The below smaller version seems ok and not interfering
       * with other formats.
       */
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "3,string,\x0CjP");

      clay_procedure_add_choice_aux_argument (procedure, "colorspace",
                                              _("Color s_pace"),
                                              _("Color space"),
                                              clay_choice_new_with_values ("srgb",      OPJ_CLRSPC_SRGB,     _("sRGB"),      NULL,
                                                                           "grayscale", OPJ_CLRSPC_GRAY,     _("Grayscale"), NULL,
                                                                           "ycbcr",     OPJ_CLRSPC_SYCC,     _("YCbCr"),     NULL,
                                                                           "xvycc",     OPJ_CLRSPC_EYCC,     _("xvYCC"),     NULL,
                                                                           "cmyk",      OPJ_CLRSPC_CMYK,     _("CMYK"),      NULL,
                                                                           "unknown",   OPJ_CLRSPC_UNKNOWN,  _("Unknown"),   NULL,
                                                                           NULL),
                                              "unknown",
                                              G_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_J2K_PROC))
    {
      procedure = clay_load_procedure_new (plug_in, name,
                                           CLAY_PDB_PROC_TYPE_PLUGIN,
                                           jp2_load, NULL, NULL);

      clay_procedure_set_menu_label (procedure, _("JPEG 2000 codestream"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000"));

      clay_procedure_set_documentation (procedure,
                                        _("Loads JPEG 2000 codestream."),
                                        _("Loads JPEG 2000 codestream. "
                                          "If the color space is set to "
                                          "\"unknown\", we will try to guess, "
                                          "which is only possible for few "
                                          "spaces (such as grayscale). Most "
                                          "such calls will fail. You are rather "
                                          "expected to know the color space of "
                                          "your data."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Jehan",
                                      "Jehan",
                                      "2009");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-jp2-codestream");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "j2k,j2c,jpc");
      clay_file_procedure_set_magics (CLAY_FILE_PROCEDURE (procedure),
                                      "0,string,\xff\x4f\xff\x51\x00");

      clay_procedure_add_choice_argument (procedure, "colorspace",
                                          _("Color s_pace"),
                                          _("Color space"),
                                          clay_choice_new_with_values ("srgb",      OPJ_CLRSPC_SRGB,     _("sRGB"),      NULL,
                                                                       "grayscale", OPJ_CLRSPC_GRAY,     _("Grayscale"), NULL,
                                                                       "ycbcr",     OPJ_CLRSPC_SYCC,     _("YCbCr"),     NULL,
                                                                       "xvycc",     OPJ_CLRSPC_EYCC,     _("xvYCC"),     NULL,
                                                                       "cmyk",      OPJ_CLRSPC_CMYK,     _("CMYK"),      NULL,
                                                                       "unknown",   OPJ_CLRSPC_UNKNOWN,  _("Unknown"),   NULL,
                                                                       NULL),
                                          "unknown",
                                          G_PARAM_READWRITE);
    }
  else if (! strcmp (name, EXPORT_JP2_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, jp2_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "GRAY, RGB*");

      clay_procedure_set_menu_label (procedure, _("JPEG 2000 image"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in JPEG 2000 file format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Advance Software",
                                      "Advance Software",
                                      "2025");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/jp2");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "jp2");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB   |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY  |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      clay_procedure_add_double_argument (procedure, "quality",
                                          _("_Quality"),
                                          _("Quality of exported image"),
                                          1.0, 100.0, 90.0,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "ict",
                                           _("IC_T compression"),
                                           _("Use Irreversible Color Transformation "
                                             "compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "resolution",
                                       _("_Number of resolutions"),
                                       _("Equivalent to the number of "
                                         "DWT decompositions plus one"),
                                       1, 12, 6,
                                       G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "progression-order",
                                          _("Pro_gression order"),
                                          NULL,
                                          clay_choice_new_with_values ("lrcp", OPJ_LRCP, _("LRCP"), NULL,
                                                                       "rlcp", OPJ_RLCP, _("RLCP"), NULL,
                                                                       "rpcl", OPJ_RPCL, _("RPCL"), NULL,
                                                                       "pcrl", OPJ_PCRL, _("PCRL"), NULL,
                                                                       "cprl", OPJ_CPRL, _("CPRL"), NULL,
                                                                       NULL),
                                          "lrcp",
                                          G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "compliance",
                                          _("Co_mpliance profile"),
                                          _("Set to ensure compliance with Digital Cinema Specifications"),
                                          clay_choice_new_with_values ("standard",     0,               _("Standard Profile"),  NULL,
                                                                       "cinema-2k-24", OPJ_CINEMA2K_24, _("Cinema2K (24 FPS)"), NULL,
                                                                       "cinema-2k-48", OPJ_CINEMA2K_48, _("Cinema2K (48 FPS)"), NULL,
                                                                       "cinema-4k-24", OPJ_CINEMA4K_24, _("Cinema4K (24 FPS)"), NULL,
                                                                       NULL),
                                          "standard",
                                          G_PARAM_READWRITE);

      /* CMYK JPEG 2000 export support was added in OpenJPEG 2.5.3 */
#if ((OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR == 5 && OPJ_VERSION_BUILD >= 3) || \
     (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR > 5) || OPJ_VERSION_MAJOR > 2)
      clay_procedure_add_boolean_argument (procedure, "cmyk",
                                           _("E_xport as CMYK"),
                                           _("Create a CMYK JPEG 2000 image using the "
                                             "soft-proofing color profile"),
                                           FALSE,
                                           G_PARAM_READWRITE);
#endif

      clay_export_procedure_set_support_comment (CLAY_EXPORT_PROCEDURE (procedure), TRUE);
    }
  else if (! strcmp (name, EXPORT_J2K_PROC))
    {
      procedure = clay_export_procedure_new (plug_in, name,
                                             CLAY_PDB_PROC_TYPE_PLUGIN,
                                             FALSE, jp2_export, NULL, NULL);

      clay_procedure_set_image_types (procedure, "GRAY, RGB*");

      clay_procedure_set_menu_label (procedure, _("JPEG 2000 codestream"));
      clay_file_procedure_set_format_name (CLAY_FILE_PROCEDURE (procedure),
                                           _("JPEG 2000 codestream"));

      clay_procedure_set_documentation (procedure,
                                        _("Exports files in JPEG 2000 codestream format"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Advance Software",
                                      "Advance Software",
                                      "2025");

      clay_file_procedure_set_mime_types (CLAY_FILE_PROCEDURE (procedure),
                                          "image/x-jp2-codestream");
      clay_file_procedure_set_extensions (CLAY_FILE_PROCEDURE (procedure),
                                          "j2k,j2c,jpc");

      clay_export_procedure_set_capabilities (CLAY_EXPORT_PROCEDURE (procedure),
                                              CLAY_EXPORT_CAN_HANDLE_RGB   |
                                              CLAY_EXPORT_CAN_HANDLE_GRAY  |
                                              CLAY_EXPORT_CAN_HANDLE_ALPHA,
                                              NULL, NULL, NULL);

      clay_procedure_add_double_argument (procedure, "quality",
                                          _("_Quality"),
                                          _("Quality of exported image"),
                                          1.0, 100.0, 90.0,
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "ict",
                                           _("IC_T compression"),
                                           _("Use Irreversible Color Transformation "
                                             "compression"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "resolution",
                                       _("_Number of Resolutions"),
                                       _("Equivalent to the number of "
                                         "DWT decompositions plus one"),
                                       1, 12, 6,
                                       G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "progression-order",
                                          _("Pro_gression order"),
                                          NULL,
                                          clay_choice_new_with_values ("lrcp", OPJ_LRCP, _("LRCP"), NULL,
                                                                       "rlcp", OPJ_RLCP, _("RLCP"), NULL,
                                                                       "rpcl", OPJ_RPCL, _("RPCL"), NULL,
                                                                       "pcrl", OPJ_PCRL, _("PCRL"), NULL,
                                                                       "cprl", OPJ_CPRL, _("CPRL"), NULL,
                                                                       NULL),
                                          "lrcp",
                                          G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "compliance",
                                          _("Co_mpliance profile"),
                                          _("Set to ensure compliance with Digital Cinema Specifications"),
                                          clay_choice_new_with_values ("standard",     0,               _("Standard Profile"),  NULL,
                                                                       "cinema-2k-24", OPJ_CINEMA2K_24, _("Cinema2K (24 FPS)"), NULL,
                                                                       "cinema-2k-48", OPJ_CINEMA2K_48, _("Cinema2K (48 FPS)"), NULL,
                                                                       "cinema-4k-24", OPJ_CINEMA4K_24, _("Cinema4K (24 FPS)"), NULL,
                                                                       NULL),
                                          "standard",
                                          G_PARAM_READWRITE);

      /* CMYK JPEG 2000 export support was added in OpenJPEG 2.5.3 */
#if ((OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR == 5 && OPJ_VERSION_BUILD >= 3) || \
     (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR > 5) || OPJ_VERSION_MAJOR > 2)
      clay_procedure_add_boolean_argument (procedure, "cmyk",
                                           _("E_xport as CMYK"),
                                           _("Create a CMYK JPEG 2000 image using the "
                                             "soft-proofing color profile"),
                                           FALSE,
                                           G_PARAM_READWRITE);
#endif
    }

  return procedure;
}

static ClayValueArray *
jp2_load (ClayProcedure         *procedure,
          ClayRunMode            run_mode,
          GFile                 *file,
          ClayMetadata          *metadata,
          ClayMetadataLoadFlags *flags,
          ClayProcedureConfig   *config,
          gpointer               run_data)
{
  ClayValueArray    *return_vals;
  ClayImage         *image          = NULL;
  OPJ_COLOR_SPACE    color_space    = OPJ_CLRSPC_UNKNOWN;
  gboolean           interactive;
  gboolean           profile_loaded = FALSE;
  ClayPDBStatusType  status         = CLAY_PDB_SUCCESS;
  GError            *error          = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case CLAY_RUN_INTERACTIVE:
      clay_ui_init (PLUG_IN_BINARY);
      interactive = TRUE;
      break;

    default:
      if (! strcmp (clay_procedure_get_name (procedure), LOAD_J2K_PROC))
        color_space =
          clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                               "colorspace");
      interactive = FALSE;
      break;
    }

  if (! strcmp (clay_procedure_get_name (procedure), LOAD_JP2_PROC))
    {
      image = load_image (procedure, G_OBJECT (config), file, OPJ_CODEC_JP2,
                          color_space, interactive, &profile_loaded,
                          &error);
    }
  else
    {
      image = load_image (procedure, G_OBJECT (config), file, OPJ_CODEC_J2K,
                          color_space, interactive, &profile_loaded,
                          &error);
    }

  if (! image)
    {
      status = error ? CLAY_PDB_EXECUTION_ERROR : CLAY_PDB_CANCEL;

      return clay_procedure_new_return_values (procedure, status, error);
    }

  if (profile_loaded)
    *flags &= ~CLAY_METADATA_LOAD_COLORSPACE;

  return_vals = clay_procedure_new_return_values (procedure,
                                                  CLAY_PDB_SUCCESS,
                                                  NULL);

  CLAY_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static ClayValueArray *
jp2_export (ClayProcedure        *procedure,
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
  GError            *error  = NULL;
  GList             *drawables;

  gegl_init (NULL, NULL);

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      clay_ui_init (PLUG_IN_BINARY);

      if (! export_dialog (procedure, config, image))
        status = CLAY_PDB_CANCEL;
    }

  if (status != CLAY_PDB_CANCEL)
    {
      export    = clay_export_options_get_image (options, &image);
      drawables = clay_image_list_layers (image);

      status = export_image (file, image, drawables->data, run_mode,
                             procedure, G_OBJECT (config),
                             &error);

      if (export == CLAY_EXPORT_EXPORT)
        clay_image_delete (image);

      g_list_free (drawables);
    }

  return clay_procedure_new_return_values (procedure, status, error);
}

static void
sycc_to_rgb (int  offset,
             int  upb,
             int  y,
             int  cb,
             int  cr,
             int *out_r,
             int *out_g,
             int *out_b)
{
  int r, g, b;

  cb -= offset;
  cr -= offset;
  r = y + (int) (1.402 * (float) cr);

  if (r < 0)
    r = 0;
  else if (r > upb)
    r = upb;
  *out_r = r;

  g = y - (int) (0.344 * (float) cb + 0.714 * (float) cr);
  if (g < 0)
    g = 0;
  else if (g > upb)
    g = upb;
  *out_g = g;

  b = y + (int) (1.772 * (float) cb);
  if (b < 0)
    b = 0;
  else if (b > upb)
    b = upb;
  *out_b = b;
}

static gboolean
sycc420_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b, *nr, *ng, *nb;
  const int *y, *cb, *cr, *ny;
  size_t maxw, maxh, max, offx, loopmaxw, offy, loopmaxh;
  int offset, upb;
  size_t i;

  upb = (int) img->comps[0].prec;
  offset = 1 << (upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc (sizeof (int) * max);
  d1 = g = (int *) malloc (sizeof (int) * max);
  d2 = b = (int *) malloc (sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc420_to_rgb()");
      goto out;
    }

  /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
  offx = img->x0 & 1U;
  loopmaxw = maxw - offx;
  /* if img->y0 is odd, then first line shall use Cb/Cr = 0 */
  offy = img->y0 & 1U;
  loopmaxh = maxh - offy;

  if (offy > 0U)
    {
      size_t j;

      for (j = 0; j < maxw; ++j)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
        }
    }

  for (i = 0U; i < (loopmaxh & ~(size_t) 1U); i += 2U)
    {
      size_t j;

      ny = y + maxw;
      nr = r + maxw;
      ng = g + maxw;
      nb = b + maxw;

      if (offx > 0U)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;
        }

      for (j = 0; j < (loopmaxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          ++cb;
          ++cr;
        }

      if (j < loopmaxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *ny, *cb, *cr, nr, ng, nb);
          ++ny;
          ++nr;
          ++ng;
          ++nb;

          ++cb;
          ++cr;
        }

      y += maxw;
      r += maxw;
      g += maxw;
      b += maxw;
    }

  if (i < loopmaxh)
    {
      size_t j;

      for (j = 0U; j < (maxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          ++cb;
          ++cr;
        }

      if (j < maxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
        }
    }

  free (img->comps[0].data);
  img->comps[0].data = d0;
  free (img->comps[1].data);
  img->comps[1].data = d1;
  free (img->comps[2].data);
  img->comps[2].data = d2;

  img->comps[1].w = img->comps[2].w = img->comps[0].w;
  img->comps[1].h = img->comps[2].h = img->comps[0].h;
  img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
  img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
  img->color_space = OPJ_CLRSPC_SRGB;

  return TRUE;

 out:
  free (r);
  free (g);
  free (b);
  return FALSE;
}

static gboolean
sycc422_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b;
  const int *y, *cb, *cr;
  size_t maxw, maxh, max, offx, loopmaxw;
  int offset, upb;
  size_t i;

  upb = (int) img->comps[0].prec;
  offset = 1 <<(upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc (sizeof (int) * max);
  d1 = g = (int *) malloc (sizeof (int) * max);
  d2 = b = (int *) malloc (sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc422_to_rgb()");
      goto out;
    }

  /* if img->x0 is odd, then first column shall use Cb/Cr = 0 */
  offx = img->x0 & 1U;
  loopmaxw = maxw - offx;

  for (i = 0U; i < maxh; ++i)
    {
      size_t j;

      if (offx > 0U)
        {
          sycc_to_rgb (offset, upb, *y, 0, 0, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
        }

      for (j = 0U; j < (loopmaxw & ~(size_t) 1U); j += 2U)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;

          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
          ++cb;
          ++cr;
        }

      if (j < loopmaxw)
        {
          sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
          ++y;
          ++r;
          ++g;
          ++b;
          ++cb;
          ++cr;
        }
    }

  free (img->comps[0].data);
  img->comps[0].data = d0;
  free (img->comps[1].data);
  img->comps[1].data = d1;
  free (img->comps[2].data);
  img->comps[2].data = d2;

  img->comps[1].w = img->comps[2].w = img->comps[0].w;
  img->comps[1].h = img->comps[2].h = img->comps[0].h;
  img->comps[1].dx = img->comps[2].dx = img->comps[0].dx;
  img->comps[1].dy = img->comps[2].dy = img->comps[0].dy;
  img->color_space = OPJ_CLRSPC_SRGB;

  return (TRUE);

 out:
  free (r);
  free (g);
  free (b);
  return (FALSE);
}

static gboolean
sycc444_to_rgb (opj_image_t *img)
{
  int *d0, *d1, *d2, *r, *g, *b;
  const int *y, *cb, *cr;
  size_t maxw, maxh, max, i;
  int offset, upb;

  upb = (int) img->comps[0].prec;
  offset = 1 << (upb - 1);
  upb = (1 << upb) - 1;

  maxw = (size_t) img->comps[0].w;
  maxh = (size_t) img->comps[0].h;
  max = maxw * maxh;

  y = img->comps[0].data;
  cb = img->comps[1].data;
  cr = img->comps[2].data;

  d0 = r = (int *) malloc(sizeof (int) * max);
  d1 = g = (int *) malloc(sizeof (int) * max);
  d2 = b = (int *) malloc(sizeof (int) * max);

  if (r == NULL || g == NULL || b == NULL)
    {
      g_warning ("malloc() failed in sycc444_to_rgb()");
      goto out;
    }

  for (i = 0U; i < max; ++i)
    {
      sycc_to_rgb (offset, upb, *y, *cb, *cr, r, g, b);
      ++y;
      ++cb;
      ++cr;
      ++r;
      ++g;
      ++b;
    }

  for (gint i = 0; i < max; i++)
    {
      img->comps[0].data[i] = d0[i];
      img->comps[1].data[1] = d1[i];
      img->comps[2].data[1] = d2[i];
    }
  g_free (d0);
  g_free (d1);
  g_free (d2);

  img->color_space = OPJ_CLRSPC_SRGB;
  return TRUE;

 out:
  free (r);
  free (g);
  free (b);
  return FALSE;
}

static gboolean
color_sycc_to_rgb (opj_image_t *img)
{
  if (img->numcomps < 3)
    {
      img->color_space = OPJ_CLRSPC_GRAY;
      return TRUE;
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 2) &&
           (img->comps[2].dx == 2) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 2) &&
           (img->comps[2].dy == 2))
    {
      /* horizontal and vertical sub-sample */
      return sycc420_to_rgb (img);
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 2) &&
           (img->comps[2].dx == 2) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 1) &&
           (img->comps[2].dy == 1))
    {
      /* horizontal sub-sample only */
      return sycc422_to_rgb (img);
    }
  else if ((img->comps[0].dx == 1) &&
           (img->comps[1].dx == 1) &&
           (img->comps[2].dx == 1) &&
           (img->comps[0].dy == 1) &&
           (img->comps[1].dy == 1) &&
           (img->comps[2].dy == 1))
    {
      /* no sub-sample */
      return sycc444_to_rgb (img);
    }
  else
    {
      g_warning ("Cannot convert in color_sycc_to_rgb()");
      return FALSE;
    }
}

static gboolean
color_cmyk_to_rgb (opj_image_t *image)
{
  float C, M, Y, K;
  float sC, sM, sY, sK;
  unsigned int w, h, max, i;

  w = image->comps[0].w;
  h = image->comps[0].h;

  if ((image->numcomps < 4) ||
      (image->comps[0].dx != image->comps[1].dx) ||
      (image->comps[0].dx != image->comps[2].dx) ||
      (image->comps[0].dx != image->comps[3].dx) ||
      (image->comps[0].dy != image->comps[1].dy) ||
      (image->comps[0].dy != image->comps[2].dy) ||
      (image->comps[0].dy != image->comps[3].dy))
    {
      g_warning ("Cannot convert in color_cmyk_to_rgb()");
      return FALSE;
    }

  max = w * h;

  sC = 1.0f / (float) ((1 << image->comps[0].prec) - 1);
  sM = 1.0f / (float) ((1 << image->comps[1].prec) - 1);
  sY = 1.0f / (float) ((1 << image->comps[2].prec) - 1);
  sK = 1.0f / (float) ((1 << image->comps[3].prec) - 1);

  for (i = 0; i < max; ++i)
    {
      /* CMYK values from 0 to 1 */
      C = (float) (image->comps[0].data[i]) * sC;
      M = (float) (image->comps[1].data[i]) * sM;
      Y = (float) (image->comps[2].data[i]) * sY;
      K = (float) (image->comps[3].data[i]) * sK;

      /* Invert all CMYK values */
      C = 1.0f - C;
      M = 1.0f - M;
      Y = 1.0f - Y;
      K = 1.0f - K;

      /* CMYK -> RGB : RGB results from 0 to 255 */
      image->comps[0].data[i] = (int) (255.0f * C * K); /* R */
      image->comps[1].data[i] = (int) (255.0f * M * K); /* G */
      image->comps[2].data[i] = (int) (255.0f * Y * K); /* B */
    }

  image->comps[0].prec = 8;
  image->comps[1].prec = 8;
  image->comps[2].prec = 8;
  image->numcomps -= 1;
  image->color_space = OPJ_CLRSPC_SRGB;

  for (i = 3; i < image->numcomps; ++i)
    {
      memcpy(&(image->comps[i]), &(image->comps[i + 1]),
             sizeof (image->comps[i]));
    }

  /* Restore the count so the OpenJPEG destroy function works
   * properly */
  image->numcomps += 1;

  return TRUE;
}

/*
 * This code has been adopted from sjpx_openjpeg.c of ghostscript
 */
static gboolean
color_esycc_to_rgb (opj_image_t *image)
{
  int y, cb, cr, sign1, sign2, val;
  unsigned int w, h, max, i;
  int flip_value;
  int max_value;

  flip_value = (1 << (image->comps[0].prec - 1));
  max_value = (1 << image->comps[0].prec) - 1;

  if ((image->numcomps < 3) ||
      (image->comps[0].dx != image->comps[1].dx) ||
      (image->comps[0].dx != image->comps[2].dx) ||
      (image->comps[0].dy != image->comps[1].dy) ||
      (image->comps[0].dy != image->comps[2].dy))
    {
      g_warning ("Cannot convert in color_esycc_to_rgb()");
      return FALSE;
    }

  w = image->comps[0].w;
  h = image->comps[0].h;

  sign1 = (int)image->comps[1].sgnd;
  sign2 = (int)image->comps[2].sgnd;

  max = w * h;

  for (i = 0; i < max; ++i)
    {

      y = image->comps[0].data[i];
      cb = image->comps[1].data[i];
      cr = image->comps[2].data[i];

      if (! sign1)
        cb -= flip_value;

      if (! sign2)
        cr -= flip_value;

      val = (int) ((float) y - (float) 0.0000368 *
                   (float) cb + (float) 1.40199 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if (val < 0)
        val = 0;
      image->comps[0].data[i] = val;

      val = (int) ((float) 1.0003 * (float) y - (float) 0.344125 *
                   (float) cb - (float) 0.7141128 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if(val < 0)
        val = 0;
      image->comps[1].data[i] = val;

      val = (int) ((float) 0.999823 * (float) y + (float) 1.77204 *
                   (float) cb - (float) 0.000008 * (float) cr + (float) 0.5);

      if (val > max_value)
        val = max_value;
      else if (val < 0)
        val = 0;
      image->comps[2].data[i] = val;
    }

  image->color_space = OPJ_CLRSPC_SRGB;
  return TRUE;
}

/*
 * get_valid_precision() converts given precision to standard precision
 * of clay i.e. 8, 16, 32
 * e.g 12-bit to 16-bit , 24-bit to 32-bit
*/
static gint
get_valid_precision (gint precision_actual)
{
  if (precision_actual <= 8)
    return 8;
  else if (precision_actual <= 16)
    return 16;
  else
    return 32;
}

static ClayPrecision
get_image_precision (gint     precision,
                     gboolean linear)
{
  switch (precision)
    {
      case 32:
        if (linear)
          return CLAY_PRECISION_U32_LINEAR;
        return CLAY_PRECISION_U32_NON_LINEAR;
      case 16:
        if (linear)
          return CLAY_PRECISION_U16_LINEAR;
        return CLAY_PRECISION_U16_NON_LINEAR;
      default:
         if (linear)
          return CLAY_PRECISION_U8_LINEAR;
        return CLAY_PRECISION_U8_NON_LINEAR;
    }
}

static OPJ_COLOR_SPACE
open_dialog (ClayProcedure    *procedure,
             GObject          *config,
             GFile            *file,
             OPJ_CODEC_FORMAT  format,
             gint              num_components,
             GError          **error)
{
  const gchar     *title;
  GtkWidget       *dialog;
  gboolean         run;
  GParamSpec      *cspec;
  ClayChoice      *choice;
  OPJ_COLOR_SPACE  color_space = OPJ_CLRSPC_SRGB;

  if (format == OPJ_CODEC_J2K)
    /* Not having color information is expected. */
    title = _("Opening JPEG 2000 codestream");
  else
    /* Unexpected, but let's be a bit flexible and ask. */
    title = _("JPEG 2000 image with no color space");

  clay_ui_init (PLUG_IN_BINARY);

  dialog = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _(title));

  cspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "colorspace");
  choice = clay_param_spec_choice_get_choice (cspec);

  if (num_components == 3)
    {
      /* Can be RGB, YUV and YCC. */
      clay_choice_set_sensitive (choice, "grayscale", FALSE);
      clay_choice_set_sensitive (choice, "cmyk", FALSE);
    }
  else if (num_components == 4)
    {
      /* Can be RGB, YUV and YCC with alpha or CMYK. */
      clay_choice_set_sensitive (choice, "grayscale", FALSE);
    }
  else
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported JPEG 2000%s '%s' with %d components."),
                   (format == OPJ_CODEC_J2K) ? " codestream" : "",
                   clay_file_get_utf8_name (file), num_components);

      g_object_set (config, "colorspace", "unknown", NULL);
    }

  if (num_components == 3 || num_components == 4)
    {
      /* By default, RGB is active. */
      clay_choice_set_sensitive (choice, "unknown", FALSE);
      g_object_set (config, "colorspace", "srgb", NULL);

      clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                                  NULL);

      gtk_widget_set_visible (dialog, TRUE);

      run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

      if (! run)
        /* Do not set an error here. The import was simply canceled.
         * No error occurred. */
        color_space = OPJ_CLRSPC_UNKNOWN;
      else
        color_space =
          clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                               "colorspace");
    }

  gtk_widget_destroy (dialog);

  return color_space;
}

static gboolean
export_dialog (ClayProcedure         *procedure,
               ClayProcedureConfig   *config,
               ClayImage             *image)
{
  GtkWidget        *dialog;
#if ((OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR == 5 && OPJ_VERSION_BUILD >= 3) || \
     (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR > 5) || OPJ_VERSION_MAJOR > 2)
  GtkWidget        *profile_label;
  ClayColorProfile *cmyk_profile = NULL;
#endif
  gboolean          run;

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  /* Quality as a ClayScaleEntry. */
  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                        "quality", 1.0);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), "quality", "ict",
                              NULL);

#if ((OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR == 5 && OPJ_VERSION_BUILD >= 3) || \
     (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR > 5) || OPJ_VERSION_MAJOR > 2)
  /* Profile label. */
  profile_label = clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                                   "profile-label", _("No soft-proofing profile"),
                                                   FALSE, FALSE);
  gtk_label_set_xalign (GTK_LABEL (profile_label), 0.0);
  gtk_label_set_ellipsize (GTK_LABEL (profile_label), PANGO_ELLIPSIZE_END);
  clay_label_set_attributes (GTK_LABEL (profile_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  clay_help_set_help_data (profile_label,
                           _("Name of the color profile used for CMYK export."), NULL);
  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "cmyk-frame", "cmyk", FALSE,
                                    "profile-label");
  cmyk_profile = clay_image_get_simulation_profile (image);
  if (cmyk_profile)
    {
      if (clay_color_profile_is_cmyk (cmyk_profile))
        {
          gchar *label_text;

          label_text = g_strdup_printf (_("Profile: %s"),
                                        clay_color_profile_get_label (cmyk_profile));
          gtk_label_set_text (GTK_LABEL (profile_label), label_text);
          clay_label_set_attributes (GTK_LABEL (profile_label),
                                     PANGO_ATTR_STYLE, PANGO_STYLE_NORMAL,
                                     -1);
          g_free (label_text);
        }
      g_object_unref (cmyk_profile);
    }

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), "cmyk-frame", NULL);
#endif

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), "resolution",
                              "progression-order", "compliance", NULL);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return run;
}

static ClayImage *
load_image (ClayProcedure     *procedure,
            GObject           *config,
            GFile             *file,
            OPJ_CODEC_FORMAT   format,
            OPJ_COLOR_SPACE    color_space,
            gboolean           interactive,
            gboolean          *profile_loaded,
            GError           **error)
{
  opj_stream_t        *stream     = NULL;
  opj_codec_t         *codec      = NULL;
  opj_dparameters_t    parameters;
  opj_image_t         *image      = NULL;
  ClayColorProfile    *profile    = NULL;
  ClayImage           *clay_image = NULL;
  ClayLayer           *layer;
  GeglBuffer          *buffer     = NULL;
  guchar              *pixels     = NULL;
  gsize                pixels_size;
  ClayImageType        image_type;
  ClayImageBaseType    base_type;
  gint                 width;
  gint                 height;
  gint                 num_components;
  gint                 i, j, k, it;
  const Babl          *file_format;
  gint                 bpp;
  ClayPrecision        image_precision;
  guint                signed_offset;
  gint                 precision_actual, precision_scaled;
  gint                 temp;
  gboolean             linear = FALSE;
  unsigned char       *c      = NULL;
  gint                 n_threads;

  clay_progress_init_printf (_("Opening '%s'"),
                             clay_file_get_utf8_name (file));

  stream = opj_stream_create_default_file_stream (g_file_peek_path (file), OPJ_TRUE);

  if (! stream)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Could not open '%s' for reading"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  codec = opj_create_decompress (format);
  if (! codec)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Failed to initialize decoder for '%s', out of memory?"),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  n_threads = clay_get_num_processors ();
  if (n_threads >= 2 && ! opj_codec_set_threads (codec, n_threads))
    {
      g_warning ("Couldn't set number of threads on decoder for '%s'.",
                 clay_file_get_utf8_name (file));
    }

  opj_set_default_decoder_parameters (&parameters);
  if (opj_setup_decoder (codec, &parameters) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't set parameters on decoder for '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  if (opj_read_header (stream, codec, &image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't read JP2 header from '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  if (opj_decode (codec, stream, image) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decode JP2 image in '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  if (opj_end_decompress (codec, stream) != OPJ_TRUE)
    {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Couldn't decompress JP2 image in '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  if (image->icc_profile_buf)
    {
      if (image->icc_profile_len)
        {
          profile = clay_color_profile_new_from_icc_profile (image->icc_profile_buf,
                                                             image->icc_profile_len,
                                                             error);
          if (! profile)
            goto out;

          *profile_loaded = TRUE;

          if (image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
              image->color_space == OPJ_CLRSPC_UNKNOWN)
            {
              if (clay_color_profile_is_rgb (profile))
                image->color_space = OPJ_CLRSPC_SRGB;
              else if (clay_color_profile_is_gray (profile))
                image->color_space = OPJ_CLRSPC_GRAY;
              else if (clay_color_profile_is_cmyk (profile))
                image->color_space = OPJ_CLRSPC_CMYK;
            }
        }
      else
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't decode CIELAB JP2 image in '%s'."),
                       clay_file_get_utf8_name (file));
          goto out;
        }

      free (image->icc_profile_buf);
      image->icc_profile_buf = NULL;
      image->icc_profile_len = 0;
    }

  num_components = image->numcomps;

  if ((image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
       image->color_space == OPJ_CLRSPC_UNKNOWN) && ! interactive)
      image->color_space = color_space;

  if (image->color_space == OPJ_CLRSPC_UNSPECIFIED ||
      image->color_space == OPJ_CLRSPC_UNKNOWN)
    {
      /* Sometimes the color space is not set at this point, which
       * sucks. This happens always with codestream images (.j2c or
       * .j2k) which are meant to be embedded by other files.
       *
       * It might also happen with JP2 in case the header does not have
       * color space and the ICC profile is absent (though this may mean
       * that the JP2 is broken, but let's be flexible and allow manual
       * fallback).
       * Assuming RGB/RGBA space is bogus since this format can handle
       * so much more. Therefore we instead pop-up a dialog asking one
       * to specify the color space in interactive mode.
       */
      if (num_components == 1 || num_components == 2)
        {
          /* Only possibility is gray. */
          image->color_space = OPJ_CLRSPC_GRAY;
        }
      else if (num_components == 5)
        {
          /* Can only be CMYK with Alpha. */
          image->color_space = OPJ_CLRSPC_CMYK;
        }
      else if (interactive)
        {
          image->color_space = open_dialog (procedure, config, file, format,
                                            num_components, error);

          if (image->color_space == OPJ_CLRSPC_UNKNOWN)
            goto out;
        }
      else /* ! interactive */
        {
          /* API call where color space was set to UNKNOWN. We don't
           * want to guess or assume anything. It is much better to just
           * fail. It is the responsibility of the developer to know its
           * data when loading it in a script.
           */
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Unknown color space in JP2 codestream '%s'."),
                       clay_file_get_utf8_name (file));
          goto out;
        }
    }

  if (image->color_space == OPJ_CLRSPC_SYCC)
    {
      if (! color_sycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert YCbCr JP2 image '%s' to RGB."),
                       clay_file_get_utf8_name (file));
          goto out;
        }
    }
  else if (image->color_space == OPJ_CLRSPC_CMYK)
    {
      if (! color_cmyk_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert CMYK JP2 image in '%s' to RGB."),
                       clay_file_get_utf8_name (file));
          goto out;
        }

      num_components--;
    }
  else if (image->color_space == OPJ_CLRSPC_EYCC)
    {
      if (! color_esycc_to_rgb (image))
        {
          g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                       _("Couldn't convert xvYCC JP2 image in '%s' to RGB."),
                       clay_file_get_utf8_name (file));
          goto out;
        }
    }

  /* At this point, the image should be converted to Gray or RGB. */
  if (image->color_space == OPJ_CLRSPC_GRAY)
    {
      base_type  = CLAY_GRAY;
      image_type = CLAY_GRAY_IMAGE;

      if (num_components >= 2)
        {
          image_type     = CLAY_GRAYA_IMAGE;
          num_components = 2;
        }
    }
  else if (image->color_space == OPJ_CLRSPC_SRGB)
    {
      base_type  = CLAY_RGB;
      image_type = CLAY_RGB_IMAGE;

      if (num_components >= 4)
        {
          image_type     = CLAY_RGBA_IMAGE;
          num_components = 4;
        }
    }
  else
    {
      /* If not gray or RGB, this is an image we cannot handle. */
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   _("Unsupported color space in JP2 image '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }

  width = image->comps[0].w;
  height = image->comps[0].h;

  if (profile)
    linear = clay_color_profile_is_linear (profile);

  precision_actual = image->comps[0].prec;

  precision_scaled = get_valid_precision (precision_actual);
  image_precision = get_image_precision (precision_scaled, linear);

  signed_offset = 0;
  if (image->comps[0].sgnd)
    {
      switch (precision_scaled)
        {
          case 32:
            signed_offset = G_MAXINT32 - 1;
            break;
          case 16:
            signed_offset = G_MAXINT16 - 1;
            break;
          default:
            signed_offset = G_MAXINT8 - 1;
        }
    }

  clay_image = clay_image_new_with_precision (width, height,
                                              base_type, image_precision);

  if (profile)
    clay_image_set_color_profile (clay_image, profile);

  layer = clay_layer_new (clay_image,
                          _("Background"),
                          width, height,
                          image_type,
                          100,
                          clay_image_get_default_new_layer_mode (clay_image));
  clay_image_insert_layer (clay_image, layer, NULL, 0);

  file_format = clay_drawable_get_format (CLAY_DRAWABLE (layer));
  bpp = babl_format_get_bytes_per_pixel (file_format);

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));

  if (! g_size_checked_mul (&pixels_size, width, (bpp * (precision_scaled / 8))))
    {
      g_set_error (error, CLAY_PLUG_IN_ERROR, 0,
                   _("Defined row size is too large in JP2 image '%s'."),
                   clay_file_get_utf8_name (file));
      goto out;
    }
  pixels = g_new0 (guchar, pixels_size);

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < num_components; j++)
        {
          int shift = precision_scaled - precision_actual;

          for (k = 0; k < width; k++)
            {
              if (shift >= 0)
                temp = (image->comps[j].data[i * width + k] + signed_offset) << shift;
              else /* precision_actual > 32 */
                temp = (image->comps[j].data[i * width + k] + signed_offset) >> (- shift);

              c = (unsigned char *) &temp;
              for (it = 0; it < (precision_scaled / 8); it++)
                {
                  pixels[k * bpp + j * (precision_scaled / 8) + it] =  c[it];
                }
            }
        }

        gegl_buffer_set (buffer, GEGL_RECTANGLE (0, i, width, 1), 0,
                         file_format, pixels, GEGL_AUTO_ROWSTRIDE);
    }
  clay_progress_update (1.0);

 out:
  if (pixels)
    g_free (pixels);
  if (buffer)
    g_object_unref (buffer);
  if (profile)
    g_object_unref (profile);
  if (image)
    opj_image_destroy (image);
  if (codec)
    opj_destroy_codec (codec);
  if (stream)
    opj_stream_destroy (stream);

  return clay_image;
}

static ClayPDBStatusType
export_image (GFile          *file,
              ClayImage      *image,
              ClayDrawable   *drawable,
              ClayRunMode     run_mode,
              ClayProcedure  *procedure,
              GObject        *config,
              GError        **error)
{
  /* OpenJPEG parameters */
  opj_cparameters_t      parameters;
  opj_codec_t           *codec;
  opj_image_t           *jp2_image;
  opj_image_cmptparm_t   cmptparm[4];
  opj_stream_t          *stream = NULL;
  OPJ_BOOL               success;
  OPJ_COLOR_SPACE        color_space;
  /* Clay variables */
  ClayPDBStatusType      status  = CLAY_PDB_SUCCESS;
  guchar                *pixels  = NULL;
  GeglBuffer            *buffer  = NULL;
  const Babl            *format;
  gchar                 *path;
  gint                   channels;
  ClayImageType          type;
  gint                   width;
  gint                   height;
  gint                   precision;
  gint                   bpc;
  gint                   tile_height;
  /* Plug-in parameters */
  gdouble                quality;
  gboolean               ict;
  gint                   resolution;
  gint                   prog_order;
  gint                   compliance;
  gboolean               save_comment;
  gchar                 *comment = NULL;
  gboolean               cmyk    = FALSE;

  g_object_get (config,
                "quality",         &quality,
                "ict",             &ict,
                "resolution",      &resolution,
#if ((OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR == 5 && OPJ_VERSION_BUILD >= 3) || \
     (OPJ_VERSION_MAJOR == 2 && OPJ_VERSION_MINOR > 5) || OPJ_VERSION_MAJOR > 2)
                "cmyk",            &cmyk,
#endif
                NULL);
  if (! strcmp (clay_procedure_get_name (procedure), EXPORT_JP2_PROC))
    g_object_get (config,
                  "include-comment", &save_comment,
                  "clay-comment",    &comment,
                  NULL);
  else
    save_comment = FALSE;

  compliance =
    clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                         "compliance");
  prog_order =
    clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                         "progression-order");

  buffer      = clay_drawable_get_buffer (drawable);
  tile_height = clay_tile_height ();

  type      = clay_drawable_type (drawable);
  width     = clay_drawable_get_width (drawable);
  height    = clay_drawable_get_height (drawable);
  precision = get_bpc (image);

  switch (type)
    {
    case CLAY_RGBA_IMAGE:
      if (precision == 8)
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B'A u16");

      channels    = 4;
      color_space = OPJ_CLRSPC_SRGB;
      break;

    case CLAY_RGB_IMAGE:
      if (precision == 8)
        format = babl_format ("R'G'B' u8");
      else
        format = babl_format ("R'G'B' u16");

      channels    = 3;
      color_space = OPJ_CLRSPC_SRGB;
      break;

    case CLAY_GRAYA_IMAGE:
      if (precision == 8)
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y'A u16");

      channels    = 2;
      color_space = OPJ_CLRSPC_GRAY;
      break;

    case CLAY_GRAY_IMAGE:
      if (precision == 8)
        format = babl_format ("Y' u8");
      else
        format = babl_format ("Y' u16");

      channels    = 1;
      color_space = OPJ_CLRSPC_GRAY;
      break;

    default:
      g_assert_not_reached ();
    }

  if (cmyk)
    {
      if (precision == 8)
        format = babl_format ("CMYK u8");
      else
        format = babl_format ("CMYK u16");

      channels    = 4;
      color_space = OPJ_CLRSPC_CMYK;
    }

  clay_progress_init_printf (_("Exporting '%s'"),
                             clay_file_get_utf8_name (file));

  /* fetch the image */
  bpc    = precision / 8;
  pixels = g_new0 (guchar, tile_height * width * channels * bpc);

  /* Initialize OpenJPEG */
  opj_set_default_encoder_parameters (&parameters);

  if (! strcmp (clay_procedure_get_name (procedure), EXPORT_JP2_PROC))
    {
      parameters.cod_format = 1;
      codec                 = opj_create_compress (OPJ_CODEC_JP2);
    }
  else
    {
      parameters.cod_format = 0;
      codec                 = opj_create_compress (OPJ_CODEC_J2K);
    }

  memset (&cmptparm[0], 0, 4 * sizeof (opj_image_cmptparm_t));

  for (gint i = 0; i < channels; i++)
    {
      cmptparm[i].prec = precision;
      cmptparm[i].sgnd = 0;
      cmptparm[i].dx   = parameters.subsampling_dx;
      cmptparm[i].dy   = parameters.subsampling_dy;
      cmptparm[i].w    = width;
      cmptparm[i].h    = height;
    }

  jp2_image = opj_image_create (channels, &cmptparm[0], color_space);

  if (! jp2_image)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  jp2_image->x0 = parameters.image_offset_x0;
  jp2_image->y0 = parameters.image_offset_y0;
  jp2_image->x1 = jp2_image->x0 + (width - 1) * parameters.subsampling_dx + 1;
  jp2_image->y1 = jp2_image->y0 + (height - 1) * parameters.subsampling_dy + 1;

  for (gint i = 0; i < height; i += tile_height)
    {
      gint tile_pixels;
      gint rect_height = (tile_height < (height - i)) ?
                          tile_height : (height - i);

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, i, width, rect_height), 1.0,
                       format, pixels,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      tile_pixels = rect_height * width;
      for (gint j = 0; j < tile_pixels; j++)
        {
          gint    index  = (i * width) + j;
          gint    offset = j * (channels * bpc);
          guint16 pix16;

          for (gint k = 0; k < (channels * bpc); k += bpc)
            {
              if (precision == 8)
                {
                  jp2_image->comps[k].data[index] = pixels[offset + k];
                }
              else if (precision == 16)
                {
                  pix16 = ((gushort) pixels[offset + k + 1] << 8) |
                           pixels[offset + k];

                  jp2_image->comps[k / bpc].data[index] = pix16;
                }
            }
        }
    }

  /* Other parameters */
  parameters.irreversible  = (gint) ict;
  parameters.numresolution = resolution;
  parameters.prog_order    = prog_order;

  if (compliance == OPJ_CINEMA2K_24)
    {
      parameters.rsiz          = OPJ_PROFILE_CINEMA_2K;
      parameters.max_comp_size = OPJ_CINEMA_24_COMP;
      parameters.max_cs_size   = OPJ_CINEMA_24_CS;
    }
  else if (compliance == OPJ_CINEMA2K_48)
    {
      parameters.rsiz          = OPJ_PROFILE_CINEMA_2K;
      parameters.max_comp_size = OPJ_CINEMA_48_COMP;
      parameters.max_cs_size   = OPJ_CINEMA_48_CS;
    }
  else if (compliance == OPJ_CINEMA4K_24)
    {
      parameters.rsiz = OPJ_PROFILE_CINEMA_4K;
    }

  if (save_comment && comment && strlen (comment))
    {
      parameters.cp_comment = g_malloc0 (strlen (comment) + 1);
#ifndef _UCRT
      strncpy (parameters.cp_comment, comment, strlen (comment));
#else
      strncpy_s (parameters.cp_comment, sizeof (parameters.cp_comment),
                 comment, strlen (comment));
#endif
    }

  if (! cmyk)
    parameters.tcp_mct = (channels >= 3) ? 1 : 0;

  /* Quality of 0 is lossless */
  parameters.tcp_distoratio[0] = (quality == 100.0) ? 0 : quality;
  parameters.tcp_numlayers     = 1;
  if (parameters.tcp_distoratio[0] > 0.0f)
    parameters.cp_fixed_quality = 1;

  /* Encoding */
  if (! opj_setup_encoder (codec, &parameters, jp2_image))
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  path = g_file_get_path (file);
  memcpy (parameters.outfile, path, strlen (path));

  stream = opj_stream_create_default_file_stream (parameters.outfile,
                                                  OPJ_FALSE);
  if (! stream)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  success = opj_start_compress (codec, jp2_image, stream);
  if (! success)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  success = success && opj_encode (codec, stream);
  if (! success)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  success = success && opj_end_compress (codec, stream);
  if (! success)
    {
      status = CLAY_PDB_EXECUTION_ERROR;
      goto out;
    }

  out:
  if (stream)
    opj_stream_destroy (stream);
  if (codec)
    opj_destroy_codec (codec);
  if (jp2_image)
    opj_image_destroy (jp2_image);

  if (buffer)
    g_object_unref (buffer);
  if (pixels)
    g_free (pixels);

  return status;
}

/* Helper Methods */

static gint
get_bpc (ClayImage *image)
{
  /* OpenJPEG only supports up to 16 bpc */
  switch (clay_image_get_precision (image))
    {
    case CLAY_PRECISION_U8_LINEAR:
    case CLAY_PRECISION_U8_NON_LINEAR:
    case CLAY_PRECISION_U8_PERCEPTUAL:
      return 8;

    default:
      return 16;
    }
}
