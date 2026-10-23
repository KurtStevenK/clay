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
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include <libclay/stdplugins-intl.h>

#include "dds.h"
#include "ddswrite.h"
#include "dxt.h"
#include "endian_rw.h"
#include "imath.h"
#include "mipmap.h"
#include "misc.h"


static gboolean      write_image        (FILE                *fp,
                                         ClayImage           *image,
                                         ClayDrawable        *drawable,
                                         ClayProcedureConfig *config);
static gboolean      save_dialog        (ClayImage           *image,
                                         ClayDrawable        *drawable,
                                         ClayProcedure       *procedure,
                                         ClayProcedureConfig *config);

static const gchar * check_comp_format  (guint32              format);

static const gchar *cubemap_face_names[4][6] =
{
  {
    "positive x", "negative x",
    "positive y", "negative y",
    "positive z", "negative z"
  },
  {
    "pos x", "neg x",
    "pos y", "neg y",
    "pos z", "neg z",
  },
  {
    "+x", "-x",
    "+y", "-y",
    "+z", "-z"
  },
  {
    "right", "left",
    "top", "bottom",
    "back", "front"
  }
};

static ClayImage *global_image          = NULL;
static ClayLayer *cubemap_faces[6];
static gboolean   is_cubemap            = FALSE;
static gboolean   is_volume             = FALSE;
static gboolean   is_array              = FALSE;
static gboolean   is_mipmap_chain_valid = FALSE;

static struct
{
  gint         format;
  DXGI_FORMAT  dxgi_format;
  gint         bpp;
  gboolean     alpha;
  guint        rmask;
  guint        gmask;
  guint        bmask;
  guint        amask;
} format_info[] =
{
  { DDS_FORMAT_RGB8,    DXGI_FORMAT_UNKNOWN,           3, FALSE, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000},
  { DDS_FORMAT_RGBA8,   DXGI_FORMAT_B8G8R8A8_UNORM,    4, TRUE,  0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000},
  { DDS_FORMAT_BGR8,    DXGI_FORMAT_UNKNOWN,           4, FALSE, 0x000000ff, 0x0000ff00, 0x00ff0000, 0x00000000},
  { DDS_FORMAT_ABGR8,   DXGI_FORMAT_R8G8B8A8_UNORM,    4, TRUE,  0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000},
  { DDS_FORMAT_R5G6B5,  DXGI_FORMAT_B5G6R5_UNORM,      2, FALSE, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000},
  { DDS_FORMAT_RGBA4,   DXGI_FORMAT_B4G4R4A4_UNORM,    2, TRUE,  0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000},
  { DDS_FORMAT_RGB5A1,  DXGI_FORMAT_B5G5R5A1_UNORM,    2, TRUE,  0x00007c00, 0x000003e0, 0x0000001f, 0x00008000},
  { DDS_FORMAT_RGB10A2, DXGI_FORMAT_R10G10B10A2_UNORM, 4, TRUE,  0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000},
  { DDS_FORMAT_R3G3B2,  DXGI_FORMAT_UNKNOWN,           1, FALSE, 0x000000e0, 0x0000001c, 0x00000003, 0x00000000},
  { DDS_FORMAT_A8,      DXGI_FORMAT_A8_UNORM,          1, FALSE, 0x00000000, 0x00000000, 0x00000000, 0x000000ff},
  { DDS_FORMAT_L8,      DXGI_FORMAT_R8_UNORM,          1, FALSE, 0x000000ff, 0x00000000, 0x00000000, 0x00000000},
  { DDS_FORMAT_L8A8,    DXGI_FORMAT_UNKNOWN,           2, TRUE,  0x000000ff, 0x00000000, 0x00000000, 0x0000ff00},
  { DDS_FORMAT_AEXP,    DXGI_FORMAT_B8G8R8A8_UNORM,    4, TRUE,  0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000},
  { DDS_FORMAT_YCOCG,   DXGI_FORMAT_B8G8R8A8_UNORM,    4, TRUE,  0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}
};


static gboolean
check_mipmaps (gint savetype)
{
  GList         *layers;
  GList         *list;
  gint           num_layers;
  gint           i, j;
  gint           w, h;
  gint           mipw, miph;
  gint           num_mipmaps;
  gint           num_surfaces = 0;
  gint           min_surfaces = 1;
  gint           max_surfaces = 1;
  gboolean       valid        = TRUE;
  ClayImageType  type;

  /* not handling volume maps for the moment... */
  if (savetype == DDS_SAVE_VOLUMEMAP)
    return 0;

  if (savetype == DDS_SAVE_CUBEMAP)
    {
      min_surfaces = 6;
      max_surfaces = 6;
    }
  else if (savetype == DDS_SAVE_ARRAY)
    {
      min_surfaces = 2;
      max_surfaces = G_MAXINT;
    }

  layers = clay_image_list_layers (global_image);
  num_layers = g_list_length (layers);

  w = clay_image_get_width (global_image);
  h = clay_image_get_height (global_image);

  num_mipmaps = get_num_mipmaps (w, h);

  type = clay_drawable_type (layers->data);

  for (list = layers; list; list = g_list_next (list))
    {
      if (type != clay_drawable_type (list->data))
        return 0;

      if ((clay_drawable_get_width  (list->data) == w) &&
          (clay_drawable_get_height (list->data) == h))
        ++num_surfaces;
    }

  if ((num_surfaces < min_surfaces) ||
      (num_surfaces > max_surfaces) ||
      (num_layers != (num_surfaces * num_mipmaps)))
    return 0;

  for (i = 0; valid && i < num_layers; i += num_mipmaps)
    {
      ClayDrawable *drawable = g_list_nth_data (layers, i);

      if ((clay_drawable_get_width  (drawable) != w) ||
          (clay_drawable_get_height (drawable) != h))
        {
          valid = FALSE;
          break;
        }

      for (j = 1; j < num_mipmaps; ++j)
        {
          drawable = g_list_nth_data (layers, i + j);

          mipw = w >> j;
          miph = h >> j;
          if (mipw < 1) mipw = 1;
          if (miph < 1) miph = 1;
          if ((clay_drawable_get_width  (drawable) != mipw) ||
              (clay_drawable_get_height (drawable) != miph))
            {
              valid = FALSE;
              break;
            }
        }
    }

  return valid;
}

static gboolean
check_cubemap (ClayImage *image)
{
  ClayLayer    **layers;
  gint           num_layers;
  gboolean       cubemap = TRUE;
  gint           i, j, k;
  gint           w, h;
  gchar         *layer_name;
  ClayImageType  type;

  layers     = clay_image_get_layers (image);
  num_layers = clay_core_object_array_get_length ((GObject **) layers);

  if (num_layers < 6)
    {
      g_free (layers);
      return FALSE;
    }

  /* Check for a valid cubemap with mipmap layers */
  if (num_layers > 6)
    {
      /* Check that mipmap layers are in order for a cubemap */
      if (! check_mipmaps (DDS_SAVE_CUBEMAP))
        return FALSE;

      /* Invalidate cubemap faces */
      for (i = 0; i < 6; ++i)
        cubemap_faces[i] = NULL;

      /* Find the mipmap level 0 layers */
      w = clay_image_get_width (image);
      h = clay_image_get_height (image);

      for (i = 0; i < num_layers; ++i)
        {
          ClayDrawable *drawable = CLAY_DRAWABLE (layers[i]);

          if ((clay_drawable_get_width  (drawable) != w) ||
              (clay_drawable_get_height (drawable) != h))
            continue;

          layer_name = (gchar *) clay_item_get_name (CLAY_ITEM (drawable));
          for (j = 0; j < 6; ++j)
            {
              for (k = 0; k < 4; ++k)
                {
                  if (strstr (layer_name, cubemap_face_names[k][j]))
                    {
                      if (cubemap_faces[j] == NULL)
                        {
                          cubemap_faces[j] = CLAY_LAYER (drawable);
                          break;
                        }
                    }
                }
            }
        }

      /* Check for 6 valid faces */
      for (i = 0; i < 6; ++i)
        {
          if (cubemap_faces[i] == NULL)
            {
              cubemap = FALSE;
              break;
            }
        }

      /* Make sure all faces are of the same type */
      if (cubemap)
        {
          type = clay_drawable_type (CLAY_DRAWABLE (cubemap_faces[0]));
          for (i = 1; i < 6 && cubemap; ++i)
            {
              if (clay_drawable_type (CLAY_DRAWABLE (cubemap_faces[i])) != type)
                cubemap = FALSE;
            }
        }
    }

  if (num_layers == 6)
    {
      /* Invalidate cubemap faces */
      for (i = 0; i < 6; ++i)
        cubemap_faces[i] = NULL;

      for (i = 0; i < num_layers; ++i)
        {
          layer_name = (gchar *) clay_item_get_name (CLAY_ITEM (layers[i]));

          for (j = 0; j < 6; ++j)
            {
              for (k = 0; k < 4; ++k)
                {
                  if (strstr (layer_name, cubemap_face_names[k][j]))
                    {
                      if (cubemap_faces[j] == NULL)
                        {
                          cubemap_faces[j] = layers[i];
                          break;
                        }
                    }
                }
            }
        }

      /* Check for 6 valid faces */
      for (i = 0; i < 6; ++i)
        {
          if (cubemap_faces[i] == NULL)
            {
              cubemap = FALSE;
              break;
            }
        }

      /* Make sure all faces are of the same size */
      if (cubemap)
        {
          w = clay_drawable_get_width (CLAY_DRAWABLE (cubemap_faces[0]));
          h = clay_drawable_get_height (CLAY_DRAWABLE (cubemap_faces[0]));

          for (i = 1; i < 6 && cubemap; ++i)
            {
              if ((clay_drawable_get_width  (CLAY_DRAWABLE (cubemap_faces[i])) != w) ||
                  (clay_drawable_get_height (CLAY_DRAWABLE (cubemap_faces[i])) != h))
                cubemap = FALSE;
            }
        }

      /* Make sure all faces are of the same type */
      if (cubemap)
        {
          type = clay_drawable_type (CLAY_DRAWABLE (cubemap_faces[0]));
          for (i = 1; i < 6 && cubemap; ++i)
            {
              if (clay_drawable_type (CLAY_DRAWABLE (cubemap_faces[i])) != type)
                cubemap = FALSE;
            }
        }
    }
  g_free (layers);

  return cubemap;
}

static gboolean
check_volume (ClayImage *image)
{
  GList         *layers;
  GList         *list;
  gint           num_layers;
  gboolean       volume = FALSE;
  gint           i;
  gint           w, h;
  ClayImageType  type;

  layers = clay_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (num_layers > 1)
    {
      volume = TRUE;

      /* Make sure all layers are of the same size */
      w = clay_drawable_get_width  (layers->data);
      h = clay_drawable_get_height (layers->data);

      for (i = 1, list = layers->next;
           i < num_layers && volume;
           ++i, list = g_list_next (list))
        {
          if ((clay_drawable_get_width  (list->data) != w) ||
              (clay_drawable_get_height (list->data) != h))
            volume = FALSE;
        }

      if (volume)
        {
          /* Make sure all layers are of the same type */
          type = clay_drawable_type (layers->data);

          for (i = 1, list = layers->next;
               i < num_layers && volume;
               ++i, list = g_list_next (list))
            {
              if (clay_drawable_type (list->data) != type)
                volume = FALSE;
            }
        }
    }

  return volume;
}

static gboolean
check_array (ClayImage *image)
{
  GList         *layers;
  gint           num_layers;
  gboolean       array = FALSE;
  gint           i;
  gint           w, h;
  ClayImageType  type;

  if (check_mipmaps (DDS_SAVE_ARRAY))
    return 1;

  layers = clay_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (num_layers > 1)
    {
      GList *list;

      array = TRUE;

      /* Make sure all layers are of the same size */
      w = clay_drawable_get_width  (layers->data);
      h = clay_drawable_get_height (layers->data);

      for (i = 1, list = g_list_next (layers);
           i < num_layers && array;
           ++i, list = g_list_next (list))
        {
          if ((clay_drawable_get_width  (list->data)  != w) ||
              (clay_drawable_get_height (list->data) != h))
            array = FALSE;
        }

      if (array)
        {
          /* Make sure all layers are of the same type */
          type = clay_drawable_type (layers->data);

          for (i = 1, list = g_list_next (layers);
               i < num_layers;
               ++i, list = g_list_next (list))
            {
              if (clay_drawable_type (list->data) != type)
                {
                  array = FALSE;
                  break;
                }
            }
        }
    }

  g_list_free (layers);

  return array;
}

static int
get_array_size (ClayImage *image)
{
  GList *layers;
  GList *list;
  gint   num_layers;
  gint   i;
  gint   w, h;
  gint   elements = 0;

  layers = clay_image_list_layers (image);
  num_layers = g_list_length (layers);

  w = clay_image_get_width  (image);
  h = clay_image_get_height (image);

  for (i = 0, list = layers;
       i < num_layers;
       ++i, list = g_list_next (list))
    {
      if ((clay_drawable_get_width  (list->data) == w) &&
          (clay_drawable_get_height (list->data) == h))
        {
          elements++;
        }
    }

  g_list_free (layers);

  return elements;
}

ClayPDBStatusType
write_dds (GFile               *file,
           ClayImage           *image,
           ClayDrawable        *drawable,
           gboolean             interactive,
           ClayProcedure       *procedure,
           ClayProcedureConfig *config,
           gboolean             is_duplicate_image)
{
  FILE         *fp;
  ClayParasite *parasite = NULL;
  gint          rc       = 0;
  gint          compression;
  gint          mipmaps;
  gint          savetype;

  savetype = clay_procedure_config_get_choice_id (config, "save-type");
  mipmaps  = clay_procedure_config_get_choice_id (config, "mipmaps");

  global_image = image;

  is_mipmap_chain_valid = check_mipmaps (savetype);

  is_cubemap = check_cubemap (image);
  is_volume  = check_volume  (image);
  is_array   = check_array   (image);

  /* Check for imported DDS original settings */
  parasite = clay_image_get_parasite (image, "dds-import-settings");
  if (parasite)
    {
      gchar   *parasite_data;
      guint32  parasite_size;
      gint     version     = 0;
      gint     comp_format = 0;
      gint     d3d9_format = 0;
      gint     dxgi_format = 0;
      guint    flags       = 0;
      guint    n_mipmaps   = 0;
      guint    n_params    = 0;

      parasite_data = (gchar *) clay_parasite_get_data (parasite,
                                                        &parasite_size);
      parasite_data = g_strndup (parasite_data, parasite_size);

#ifndef _UCRT
      n_params = sscanf (parasite_data, "%d %d %d %d %d %d", &version,
                         &comp_format, &d3d9_format, &dxgi_format, &flags,
                         &n_mipmaps);
#else
      n_params = sscanf_s (parasite_data, "%d %d %d %d %d %d", &version,
                           &comp_format, &d3d9_format, &dxgi_format, &flags,
                           &n_mipmaps);
#endif
      if (n_params == 6)
        {
          const gchar *config_comp_format;

          config_comp_format = check_comp_format (comp_format);
          g_object_set (config,
                        "compression-format", config_comp_format,
                        NULL);
        }
      g_free (parasite_data);

      clay_image_detach_parasite (image, "dds-import-settings");
      g_free (parasite);
    }

  compression = clay_procedure_config_get_choice_id (config,
                                                     "compression-format");

  if (interactive)
    {
      if (! is_mipmap_chain_valid &&
          mipmaps == DDS_MIPMAP_EXISTING)
        {
          g_object_set (config,
                        "mipmaps", "none",
                        NULL);
        }

      if (! save_dialog (image, drawable, procedure, config))
        return CLAY_PDB_CANCEL;
    }
  else
    {
      if ((savetype == DDS_SAVE_CUBEMAP) && (! is_cubemap))
        {
          g_message ("DDS: Cannot save image as cube map");
          return CLAY_PDB_EXECUTION_ERROR;
        }

      if ((savetype == DDS_SAVE_VOLUMEMAP) && (! is_volume))
        {
          g_message ("DDS: Cannot save image as volume map");
          return CLAY_PDB_EXECUTION_ERROR;
        }

      if ((savetype == DDS_SAVE_VOLUMEMAP) && (compression != DDS_COMPRESS_NONE))
        {
          g_message ("DDS: Cannot save volume map with compression");
          return CLAY_PDB_EXECUTION_ERROR;
        }

      if ((mipmaps == DDS_MIPMAP_EXISTING) && (! is_mipmap_chain_valid))
        {
          g_message ("DDS: Cannot save with existing mipmaps as the mipmap chain is incomplete");
          return CLAY_PDB_EXECUTION_ERROR;
        }
    }

  /* Open up the file to write */
  fp = g_fopen (g_file_peek_path (file), "wb");

  if (! fp)
    {
      g_message ("Error opening %s", g_file_peek_path (file));
      return CLAY_PDB_EXECUTION_ERROR;
    }

  clay_progress_init_printf (_("Saving: %s"), clay_file_get_utf8_name (file));

  /* If destructive changes are going to happen to the image,
   * make sure we send a duplicate of it to write_image()
   */
  if (! is_duplicate_image)
    {
      ClayImage     *duplicate_image = clay_image_duplicate (image);
      ClayDrawable **drawables;

      drawables = clay_image_get_selected_drawables (duplicate_image);
      rc = write_image (fp, duplicate_image, drawables[0], config);
      clay_image_delete (duplicate_image);
      g_free (drawables);
    }
  else
    {
      rc = write_image (fp, image, drawable, config);
    }

  fclose (fp);

  return rc ? CLAY_PDB_SUCCESS : CLAY_PDB_EXECUTION_ERROR;
}

static void
swap_rb (guchar *pixels,
         guint   n,
         gint    bpp)
{
  guint  i;
  guchar t;

  for (i = 0; i < n; ++i)
    {
      t = pixels[bpp * i + 0];
      pixels[bpp * i + 0] = pixels[bpp * i + 2];
      pixels[bpp * i + 2] = t;
    }
}

static void
convert_pixels (guchar *dst,
                guchar *src,
                gint    format,
                gint    w,
                gint    h,
                gint    d,
                gint    bpp,
                guchar *palette,
                gint    mipmaps)
{
  guint  i, num_pixels;
  guchar r, g, b, a;

  if (d > 0)
    num_pixels = get_volume_mipmapped_size (w, h, d, 1, 0, mipmaps, DDS_COMPRESS_NONE);
  else
    num_pixels = get_mipmapped_size (w, h, 1, 0, mipmaps, DDS_COMPRESS_NONE);

  for (i = 0; i < num_pixels; ++i)
    {
      if (bpp == 1)
        {
          if (palette)
            {
              r = palette[3 * src[i] + 0];
              g = palette[3 * src[i] + 1];
              b = palette[3 * src[i] + 2];
            }
          else
            r = g = b = src[i];

          if (format == DDS_FORMAT_A8)
            a = src[i];
          else
            a = 255;
        }
      else if (bpp == 2)
        {
          r = g = b = src[2 * i];
          a = src[2 * i + 1];
        }
      else if (bpp == 3)
        {
          b = src[3 * i + 0];
          g = src[3 * i + 1];
          r = src[3 * i + 2];
          a = 255;
        }
      else
        {
          b = src[4 * i + 0];
          g = src[4 * i + 1];
          r = src[4 * i + 2];
          a = src[4 * i + 3];
        }

      switch (format)
        {
        case DDS_FORMAT_RGB8:
          dst[3 * i + 0] = b;
          dst[3 * i + 1] = g;
          dst[3 * i + 2] = r;
          break;
        case DDS_FORMAT_RGBA8:
          dst[4 * i + 0] = b;
          dst[4 * i + 1] = g;
          dst[4 * i + 2] = r;
          dst[4 * i + 3] = a;
          break;
        case DDS_FORMAT_BGR8:
          dst[4 * i + 0] = r;
          dst[4 * i + 1] = g;
          dst[4 * i + 2] = b;
          dst[4 * i + 3] = 0;
          break;
        case DDS_FORMAT_ABGR8:
          dst[4 * i + 0] = r;
          dst[4 * i + 1] = g;
          dst[4 * i + 2] = b;
          dst[4 * i + 3] = a;
          break;
        case DDS_FORMAT_R5G6B5:
          PUTL16 (&dst[2 * i],
            (mul8bit (r, 31) << 11) |
            (mul8bit (g, 63) <<  5) |
            (mul8bit (b, 31)      ));
          break;
        case DDS_FORMAT_RGBA4:
          PUTL16 (&dst[2 * i],
            (mul8bit (a, 15) << 12) |
            (mul8bit (r, 15) <<  8) |
            (mul8bit (g, 15) <<  4) |
            (mul8bit (b, 15)      ));
          break;
        case DDS_FORMAT_RGB5A1:
          PUTL16 (&dst[2 * i],
            (((a >> 7) & 0x01) << 15) |
            (mul8bit (r, 31)   << 10) |
            (mul8bit (g, 31)   <<  5) |
            (mul8bit (b, 31)        ));
          break;
        case DDS_FORMAT_RGB10A2:
          PUTL32 (&dst[4 * i],
            ((guint) ((a >> 6) & 0x003) << 30) |
            ((guint) ((b << 2) & 0x3ff) << 20) |
            ((guint) ((g << 2) & 0x3ff) << 10) |
            ((guint) ((r << 2) & 0x3ff)      ));
          break;
        case DDS_FORMAT_R3G3B2:
          dst[i] =
            (mul8bit (r, 7) << 5) |
            (mul8bit (g, 7) << 2) |
            (mul8bit (b, 3)     );
          break;
        case DDS_FORMAT_A8:
          dst[i] = a;
          break;
        case DDS_FORMAT_L8:
          dst[i] =
            ((r * 54 + g * 182 + b * 20) + 128) >> 8;
          break;
        case DDS_FORMAT_L8A8:
          dst[2 * i + 0] =
            ((r * 54 + g * 182 + b * 20) + 128) >> 8;
          dst[2 * i + 1] = a;
          break;
        case DDS_FORMAT_YCOCG:
          dst[4 * i] = a;
          encode_ycocg (&dst[4 * i], r, g, b);
          break;
        case DDS_FORMAT_AEXP:
          encode_alpha_exponent (&dst[4 * i], r, g, b, a);
          break;
        default:
          break;
        }
    }
}

static void
get_mipmap_chain (guchar       *dst,
                  gint          w,
                  gint          h,
                  gint          bpp,
                  ClayImage    *image,
                  ClayDrawable *drawable)
{
  GList      *layers;
  GList      *list;
  gint        num_layers;
  GeglBuffer *buffer;
  const Babl *format;
  gint        i;
  gint        idx = 0;
  gint        offset;
  gint        mipw, miph;

  if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  layers = clay_image_list_layers (image);
  num_layers = g_list_length (layers);

  for (i = 0, list = layers;
       i < num_layers;
       ++i, list = g_list_next (list))
    {
      if (list->data == drawable)
        {
          idx = i;
          break;
        }
    }

  if (i == num_layers)
    return;

  offset = 0;

  while (get_next_mipmap_dimensions (&mipw, &miph, w, h))
    {
      buffer = clay_drawable_get_buffer (g_list_nth_data (layers, ++idx));

      if ((gegl_buffer_get_width (buffer)  != mipw) ||
          (gegl_buffer_get_height (buffer) != miph))
        {
          g_object_unref (buffer);
          return;
        }

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, mipw, miph), 1.0, format,
                       dst + offset, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      g_object_unref (buffer);

      /* BGRX or BGRA needed */
      if (bpp >= 3)
        swap_rb (dst + offset, mipw * miph, bpp);

      offset += (mipw * miph * bpp);
      w = mipw;
      h = miph;
    }
}

static void
write_layer (FILE                *fp,
             ClayImage           *image,
             ClayDrawable        *drawable,
             ClayProcedureConfig *config,
             gint                 w,
             gint                 h,
             gint                 bpp,
             gint                 fmtbpp,
             gint                 num_mipmaps)
{
  GeglBuffer        *buffer;
  const Babl        *format;
  ClayImageBaseType  basetype;
  ClayImageType      type;
  guchar            *src;
  guchar            *dst;
  guchar            *fmtdst;
  guchar            *tmp;
  guchar            *palette = NULL;
  gint               i, c;
  gint               x, y;
  gint               size;
  gint               fmtsize;
  gint               offset  = 0;
  gint               colors;
  gint               compression;
  gint               mipmaps;
  gint               pixel_format;
  gboolean           perceptual_metric;
  gint               flags   = 0;

  g_object_get (config,
                "perceptual-metric",  &perceptual_metric,
                NULL);
  compression  = clay_procedure_config_get_choice_id (config, "compression-format");
  pixel_format = clay_procedure_config_get_choice_id (config, "format");
  mipmaps      = clay_procedure_config_get_choice_id (config, "mipmaps");

  basetype = clay_image_get_base_type (image);
  type = clay_drawable_type (drawable);

  buffer = clay_drawable_get_buffer (drawable);

  src = g_malloc (w * h * bpp);

  if (basetype == CLAY_INDEXED)
    format = clay_drawable_get_format (drawable);
  else if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (basetype == CLAY_INDEXED)
    {
      palette = clay_palette_get_colormap (clay_image_get_palette (image), babl_format ("R'G'B' u8"), &colors, NULL);

      if (type == CLAY_INDEXEDA_IMAGE)
        {
          tmp = g_malloc (w * h);
          for (i = 0; i < w * h; ++i)
            tmp[i] = src[2 * i];
          g_free (src);
          src = tmp;
          bpp = 1;
        }
    }

  /* We want and assume BGRA ordered pixels for bpp >= 3 from here on  */
  if (bpp >= 3 && compression != DDS_COMPRESS_BC7)
    swap_rb (src, w * h, bpp);

  if (compression == DDS_COMPRESS_BC3N)
    {
      if (bpp != 4)
        {
          fmtsize = w * h * 4;
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGBA8, w, h, 0, bpp,
                          palette, 1);
          g_free (src);
          src = fmtdst;
          bpp = 4;
        }

      for (y = 0; y < h; ++y)
        {
          for (x = 0; x < w; ++x)
            {
              /* set alpha to red (x) */
              src[y * (w * 4) + (x * 4) + 3] =
                src[y * (w * 4) + (x * 4) + 2];
              /* set red to 1 */
              src[y * (w * 4) + (x * 4) + 2] = 255;
            }
        }
    }

  /* RXGB (Doom3) */
  if (compression == DDS_COMPRESS_RXGB)
    {
      if (bpp != 4)
        {
          fmtsize = w * h * 4;
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGBA8, w, h, 0, bpp,
                          palette, 1);
          g_free (src);
          src = fmtdst;
          bpp = 4;
        }

      for (y = 0; y < h; ++y)
        {
          for (x = 0; x < w; ++x)
            {
              /* swap red and alpha */
              c = src[y * (w * 4) + (x * 4) + 3];
              src[y * (w * 4) + (x * 4) + 3] =
                src[y * (w * 4) + (x * 4) + 2];
              src[y * (w * 4) + (x * 4) + 2] = c;
            }
        }
    }

  if (compression == DDS_COMPRESS_YCOCG ||
      compression == DDS_COMPRESS_YCOCGS) /* convert to YCoCG */
    {
      fmtsize = w * h * 4;
      fmtdst = g_malloc (fmtsize);
      convert_pixels (fmtdst, src, DDS_FORMAT_YCOCG, w, h, 0, bpp,
                      palette, 1);
      g_free (src);
      src = fmtdst;
      bpp = 4;
    }

  if (compression == DDS_COMPRESS_AEXP)
    {
      fmtsize = w * h * 4;
      fmtdst = g_malloc (fmtsize);
      convert_pixels (fmtdst, src, DDS_FORMAT_AEXP, w, h, 0, bpp,
                      palette, 1);
      g_free (src);
      src = fmtdst;
      bpp = 4;
    }

  if (compression == DDS_COMPRESS_NONE)
    {
      if (num_mipmaps > 1)
        {
          /* Pre-convert indexed images to RGB if not exporting as indexed */
          if (pixel_format > DDS_FORMAT_DEFAULT && basetype == CLAY_INDEXED)
            {
              fmtsize = get_mipmapped_size (w, h, 3, 0, num_mipmaps, DDS_COMPRESS_NONE);
              fmtdst = g_malloc (fmtsize);
              convert_pixels (fmtdst, src, DDS_FORMAT_RGB8, w, h, 0, bpp,
                              palette, 1);
              g_free (src);
              src = fmtdst;
              bpp = 3;
              palette = NULL;
            }

          size = get_mipmapped_size (w, h, bpp, 0, num_mipmaps,
                                     DDS_COMPRESS_NONE);
          dst = g_malloc (size);
          if (mipmaps == DDS_MIPMAP_GENERATE)
            {
              gint     mipmap_filter;
              gint     mipmap_wrap;
              gboolean gamma_correct;
              gboolean srgb;
              gdouble  gamma;
              gboolean preserve_alpha_coverage;
              gdouble  alpha_test_threshold;

              g_object_get (config,
                            "gamma-correct",           &gamma_correct,
                            "srgb",                    &srgb,
                            "gamma",                   &gamma,
                            "preserve-alpha-coverage", &preserve_alpha_coverage,
                            "alpha-test-threshold",    &alpha_test_threshold,
                            NULL);
              mipmap_filter = clay_procedure_config_get_choice_id (config,
                                                                   "mipmap-filter");
              mipmap_wrap   = clay_procedure_config_get_choice_id (config,
                                                                   "mipmap-wrap");

              generate_mipmaps (dst, src, w, h, bpp, palette != NULL,
                                num_mipmaps,
                                mipmap_filter,
                                mipmap_wrap,
                                gamma_correct + srgb,
                                gamma,
                                preserve_alpha_coverage,
                                alpha_test_threshold);
            }
          else
            {
              memcpy (dst, src, w * h * bpp);
              get_mipmap_chain (dst + (w * h * bpp), w, h, bpp, image, drawable);
            }

          if (pixel_format > DDS_FORMAT_DEFAULT)
            {
              fmtsize = get_mipmapped_size (w, h, fmtbpp, 0, num_mipmaps,
                                            DDS_COMPRESS_NONE);
              fmtdst = g_malloc (fmtsize);

              convert_pixels (fmtdst, dst, pixel_format, w, h, 0, bpp,
                              palette, num_mipmaps);

              g_free (dst);
              dst = fmtdst;
              bpp = fmtbpp;
            }

          for (i = 0; i < num_mipmaps; ++i)
            {
              size = get_mipmapped_size (w, h, bpp, i, 1, DDS_COMPRESS_NONE);
              fwrite (dst + offset, 1, size, fp);
              offset += size;
            }

          g_free (dst);
        }
      else
        {
          if (pixel_format > DDS_FORMAT_DEFAULT)
            {
              fmtdst = g_malloc (h * w * fmtbpp);
              convert_pixels (fmtdst, src, pixel_format, w, h, 0, bpp,
                              palette, 1);
              g_free (src);
              src = fmtdst;
              bpp = fmtbpp;
            }

          fwrite (src, 1, h * w * bpp, fp);
        }
    }
  else
    {
      size = get_mipmapped_size (w, h, bpp, 0, num_mipmaps, compression);

      dst = g_malloc (size);

      if (basetype == CLAY_INDEXED)
        {
          fmtsize = get_mipmapped_size (w, h, 3, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
          fmtdst = g_malloc (fmtsize);
          convert_pixels (fmtdst, src, DDS_FORMAT_RGB8, w, h, 0, bpp,
                          palette, num_mipmaps);
          g_free (src);
          src = fmtdst;
          bpp = 3;
        }

      if (num_mipmaps > 1)
        {
          fmtsize = get_mipmapped_size (w, h, bpp, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
          fmtdst = g_malloc (fmtsize);
          if (mipmaps == DDS_MIPMAP_GENERATE)
            {
              gint     mipmap_filter;
              gint     mipmap_wrap;
              gboolean gamma_correct;
              gboolean srgb;
              gdouble  gamma;
              gboolean preserve_alpha_coverage;
              gdouble  alpha_test_threshold;

              g_object_get (config,
                            "gamma-correct",           &gamma_correct,
                            "srgb",                    &srgb,
                            "gamma",                   &gamma,
                            "preserve-alpha-coverage", &preserve_alpha_coverage,
                            "alpha-test-threshold",    &alpha_test_threshold,
                            NULL);
              mipmap_filter = clay_procedure_config_get_choice_id (config,
                                                                   "mipmap-filter");
              mipmap_wrap   = clay_procedure_config_get_choice_id (config,
                                                                   "mipmap-wrap");

              generate_mipmaps (fmtdst, src, w, h, bpp, 0, num_mipmaps,
                                mipmap_filter,
                                mipmap_wrap,
                                gamma_correct + srgb,
                                gamma,
                                preserve_alpha_coverage,
                                alpha_test_threshold);
            }
          else
            {
              memcpy (fmtdst, src, w * h * bpp);
              get_mipmap_chain (fmtdst + (w * h * bpp), w, h, bpp, image, drawable);
            }

          g_free (src);
          src = fmtdst;
        }

      flags = 0;
      if (perceptual_metric)
        flags |= DXT_PERCEPTUAL;

      dxt_compress (dst, src, compression, w, h, bpp, num_mipmaps, flags);

      fwrite (dst, 1, size, fp);

      g_free (dst);
    }

  g_free (src);

  g_object_unref (buffer);
}

static void
write_volume_mipmaps (FILE                *fp,
                      ClayImage           *image,
                      ClayProcedureConfig *config,
                      GList               *layers,
                      gint                 w,
                      gint                 h,
                      gint                 d,
                      gint                 bpp,
                      gint                 fmtbpp,
                      gint                 num_mipmaps)
{
  GList             *list;
  gint               i;
  gint               size;
  gint               offset;
  gint               colors;
  guchar            *src;
  guchar            *dst;
  guchar            *tmp;
  guchar            *fmtdst;
  guchar            *palette = 0;
  GeglBuffer        *buffer;
  const Babl        *format;
  ClayImageBaseType  type;
  gint               compression;
  gint               pixel_format;
  gint               mipmap_filter;
  gint               mipmap_wrap;
  gboolean           gamma_correct;
  gboolean           srgb;
  gdouble            gamma;

  g_object_get (config,
                "gamma-correct",      &gamma_correct,
                "srgb",               &srgb,
                "gamma",              &gamma,
                NULL);
  compression = clay_procedure_config_get_choice_id (config,
                                                     "compression-format");
  pixel_format = clay_procedure_config_get_choice_id (config, "format");
  mipmap_filter = clay_procedure_config_get_choice_id (config,
                                                       "mipmap-filter");
  mipmap_wrap = clay_procedure_config_get_choice_id (config, "mipmap-wrap");

  type = clay_image_get_base_type (image);

  if (compression != DDS_COMPRESS_NONE)
    return;

  src = g_malloc (w * h * bpp * d);

  if (bpp == 1)
    format = babl_format ("Y' u8");
  else if (bpp == 2)
    format = babl_format ("Y'A u8");
  else if (bpp == 3)
    format = babl_format ("R'G'B' u8");
  else
    format = babl_format ("R'G'B'A u8");

  if (clay_image_get_base_type (image) == CLAY_INDEXED)
    palette = clay_palette_get_colormap (clay_image_get_palette (image), babl_format ("R'G'B' u8"), &colors, NULL);

  offset = 0;
  for (i = 0, list = layers;
       i < d;
       ++i, list = g_list_next (list))
    {
      buffer = clay_drawable_get_buffer (list->data);
      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0, format,
                       src + offset, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      offset += (w * h * bpp);
      g_object_unref (buffer);
    }

  if (clay_drawable_type (layers->data) == CLAY_INDEXEDA_IMAGE)
    {
      tmp = g_malloc (w * h * d);
      for (i = 0; i < w * h * d; ++i)
        tmp[i] = src[2 * i];
      g_free (src);
      src = tmp;
      bpp = 1;
    }

  /* We want and assume BGRA ordered pixels for bpp >= 3 from here on */
  if (bpp >= 3 && compression != DDS_COMPRESS_BC7)
    swap_rb (src, w * h * d, bpp);

  /* Pre-convert indexed images to RGB if not exporting as indexed */
  if (pixel_format > DDS_FORMAT_DEFAULT && type == CLAY_INDEXED)
    {
      size = get_volume_mipmapped_size (w, h, d, 3, 0, num_mipmaps,
                                        DDS_COMPRESS_NONE);
      dst = g_malloc (size);
      convert_pixels (dst, src, DDS_FORMAT_RGB8, w, h, d, bpp, palette, 1);
      g_free (src);
      src = dst;
      bpp = 3;
      palette = NULL;
    }

  size = get_volume_mipmapped_size (w, h, d, bpp, 0, num_mipmaps,
                                    compression);

  dst = g_malloc (size);

  offset = get_volume_mipmapped_size (w, h, d, bpp, 0, 1,
                                      compression);

  generate_volume_mipmaps (dst, src, w, h, d, bpp,
                           palette != NULL, num_mipmaps,
                           mipmap_filter,
                           mipmap_wrap,
                           gamma_correct + srgb,
                           gamma);

  if (pixel_format > DDS_FORMAT_DEFAULT)
    {
      size = get_volume_mipmapped_size (w, h, d, fmtbpp, 0, num_mipmaps,
                                        compression);
      offset = get_volume_mipmapped_size (w, h, d, fmtbpp, 0, 1,
                                          compression);
      fmtdst = g_malloc (size);

      convert_pixels (fmtdst, dst, pixel_format, w, h, d, bpp,
                      palette, num_mipmaps);
      g_free (dst);
      dst = fmtdst;
    }

  fwrite (dst + offset, 1, size, fp);

  g_free (src);
  g_free (dst);
}

static gboolean
write_image (FILE                *fp,
             ClayImage           *image,
             ClayDrawable        *drawable,
             ClayProcedureConfig *config)
{
  ClayImageType      drawable_type;
  ClayImageBaseType  basetype;
  gint               i, w, h;
  gint               bpp        = 0;
  gint               fmtbpp     = 0;
  gboolean           has_alpha  = FALSE;
  gboolean           is_dx10    = FALSE;
  guint              fourcc     = 0;
  gint               array_size = 1;
  gint               num_mipmaps;
  guchar             hdr[DDS_HEADERSIZE];
  guchar             hdr10[DDS_HEADERSIZE_DX10];
  guint              flags = 0, pflags = 0, caps = 0, caps2 = 0, size = 0;
  guint              rmask = 0, gmask = 0, bmask = 0, amask = 0;
  DXGI_FORMAT        dxgi_format = DXGI_FORMAT_UNKNOWN;
  gint32             num_layers;
  GList             *layers, *list;
  guchar            *cmap;
  gint               colors;
  gint               compression, mipmaps;
  gint               savetype, pixel_format;
  gint               transindex;
  gboolean           flip_export;

  g_object_get (config,
                "transparent-index",  &transindex,
                "flip-image",         &flip_export,
                NULL);
  savetype     = clay_procedure_config_get_choice_id (config, "save-type");
  compression  = clay_procedure_config_get_choice_id (config, "compression-format");
  pixel_format = clay_procedure_config_get_choice_id (config, "format");
  mipmaps      = clay_procedure_config_get_choice_id (config, "mipmaps");

  if (flip_export)
    clay_image_flip (image, CLAY_ORIENTATION_VERTICAL);

  layers = clay_image_list_layers (image);
  num_layers = g_list_length (layers);

  if (mipmaps == DDS_MIPMAP_EXISTING)
    drawable = layers->data;

  if (savetype == DDS_SAVE_SELECTED_LAYER)
    {
      w = clay_drawable_get_width  (drawable);
      h = clay_drawable_get_height (drawable);
    }
  else
    {
      w = clay_image_get_width  (image);
      h = clay_image_get_height (image);
    }

  basetype = clay_image_get_base_type (image);
  drawable_type = clay_drawable_type (drawable);

  switch (drawable_type)
    {
    case CLAY_RGB_IMAGE:      bpp = 3; break;
    case CLAY_RGBA_IMAGE:     bpp = 4; break;
    case CLAY_GRAY_IMAGE:     bpp = 1; break;
    case CLAY_GRAYA_IMAGE:    bpp = 2; break;
    case CLAY_INDEXED_IMAGE:  bpp = 1; break;
    case CLAY_INDEXEDA_IMAGE: bpp = 2; break;
    default:                  break;
    }

  /* Get uncompressed format data */
  if (pixel_format > DDS_FORMAT_DEFAULT)
    {
      for (i = 0; ; ++i)
        {
          if (format_info[i].format == pixel_format)
            {
              fmtbpp = format_info[i].bpp;
              has_alpha = format_info[i].alpha;
              rmask = format_info[i].rmask;
              gmask = format_info[i].gmask;
              bmask = format_info[i].bmask;
              amask = format_info[i].amask;
              dxgi_format = format_info[i].dxgi_format;
              break;
            }
        }
    }
  else if (bpp == 1)
    {
      if (basetype == CLAY_INDEXED)
        {
          fmtbpp = 1;
          has_alpha = FALSE;
          rmask = bmask = gmask = amask = 0;
        }
      else
        {
          fmtbpp = 1;
          has_alpha = FALSE;
          rmask = 0x000000ff;
          gmask = bmask = amask = 0;
          dxgi_format = DXGI_FORMAT_R8_UNORM;
        }
    }
  else if (bpp == 2)
    {
      if (basetype == CLAY_INDEXED)
        {
          fmtbpp = 1;
          has_alpha = FALSE;
          rmask = gmask = bmask = amask = 0;
        }
      else
        {
          fmtbpp = 2;
          has_alpha = TRUE;
          rmask = 0x000000ff;
          gmask = 0x00000000;
          bmask = 0x00000000;
          amask = 0x0000ff00;
        }
    }
  else if (bpp == 3)
    {
      fmtbpp = 3;
      rmask = 0x00ff0000;
      gmask = 0x0000ff00;
      bmask = 0x000000ff;
      amask = 0x00000000;
    }
  else
    {
      fmtbpp = 4;
      has_alpha = TRUE;
      rmask = 0x00ff0000;
      gmask = 0x0000ff00;
      bmask = 0x000000ff;
      amask = 0xff000000;
      dxgi_format = DXGI_FORMAT_B8G8R8A8_UNORM;
    }

  /* Write header */
  memset (hdr, 0, DDS_HEADERSIZE);

  PUTL32 (hdr,       FOURCC ('D','D','S',' '));
  PUTL32 (hdr + 4,   124);  /* Header size */
  PUTL32 (hdr + 12,  h);
  PUTL32 (hdr + 16,  w);
  PUTL32 (hdr + 76,  32);  /* Pixel Format size */

  if (compression == DDS_COMPRESS_NONE)
    {
      PUTL32 (hdr + 88,  fmtbpp << 3);
      PUTL32 (hdr + 92,  rmask);
      PUTL32 (hdr + 96,  gmask);
      PUTL32 (hdr + 100, bmask);
      PUTL32 (hdr + 104, amask);
    }

  /* Put some custom info in the reserved area to identify the origin of the image */
  PUTL32 (hdr + 32, FOURCC ('G','I','M','P'));
  PUTL32 (hdr + 36, FOURCC ('-','D','D','S'));
  PUTL32 (hdr + 40, DDS_PLUGIN_VERSION);

  flags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;

  caps = DDSCAPS_TEXTURE;
  if (mipmaps)
    {
      flags |= DDSD_MIPMAPCOUNT;
      caps  |= (DDSCAPS_COMPLEX | DDSCAPS_MIPMAP);
      num_mipmaps = get_num_mipmaps (w, h);
    }
  else
    {
      num_mipmaps = 1;
    }

  if ((savetype == DDS_SAVE_CUBEMAP) && is_cubemap)
    {
      caps  |= DDSCAPS_COMPLEX;
      caps2 |= (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_ALL_FACES);
    }
  else if ((savetype == DDS_SAVE_VOLUMEMAP) && is_volume)
    {
      PUTL32 (hdr + 24, num_layers);  /* Depth */
      flags |= DDSD_DEPTH;
      caps  |= DDSCAPS_COMPLEX;
      caps2 |= DDSCAPS2_VOLUME;
    }

  PUTL32 (hdr + 28,  num_mipmaps);
  PUTL32 (hdr + 108, caps);
  PUTL32 (hdr + 112, caps2);

  if (compression == DDS_COMPRESS_NONE)  /* Write uncompressed data */
    {
      flags |= DDSD_PITCH;

      if (pixel_format > DDS_FORMAT_DEFAULT)
        {
          if (pixel_format == DDS_FORMAT_A8)
            pflags |= DDPF_ALPHA;
          else
            {
              if (((fmtbpp == 1) || (pixel_format == DDS_FORMAT_L8A8)) &&
                  (pixel_format != DDS_FORMAT_R3G3B2))
                pflags |= DDPF_LUMINANCE;
              else
                pflags |= DDPF_RGB;
            }
        }
      else
        {
          if (bpp == 1 || bpp == 2)
            {
              if (basetype == CLAY_INDEXED)
                pflags |= DDPF_PALETTEINDEXED8;
              else
                pflags |= DDPF_LUMINANCE;
            }
          else
            {
              pflags |= DDPF_RGB;
            }
        }

      if (has_alpha)
        pflags |= DDPF_ALPHAPIXELS;

      PUTL32 (hdr + 8,  flags);
      PUTL32 (hdr + 20, w * fmtbpp);  /* Pitch */
      PUTL32 (hdr + 80, pflags);

      /* Write extra FourCC info specific to Clay DDS. When the image
       * is read again we use this information to decode the pixels.
       */
      if (pixel_format == DDS_FORMAT_AEXP)
        {
          PUTL32 (hdr + 44, FOURCC ('A','E','X','P'));
        }
      else if (pixel_format == DDS_FORMAT_YCOCG)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','1'));
        }
    }
  else  /* Write compressed data */
    {
      flags |= DDSD_LINEARSIZE;
      pflags = DDPF_FOURCC;

      switch (compression)
        {
        case DDS_COMPRESS_BC1:
          fourcc = FOURCC ('D','X','T','1');
          dxgi_format = DXGI_FORMAT_BC1_UNORM;
          break;

        case DDS_COMPRESS_BC2:
          fourcc = FOURCC ('D','X','T','3');
          dxgi_format = DXGI_FORMAT_BC2_UNORM;
          break;

        case DDS_COMPRESS_BC3:
        case DDS_COMPRESS_BC3N:
        case DDS_COMPRESS_YCOCG:
        case DDS_COMPRESS_YCOCGS:
        case DDS_COMPRESS_AEXP:
          fourcc = FOURCC ('D','X','T','5');
          dxgi_format = DXGI_FORMAT_BC3_UNORM;
          break;

        case DDS_COMPRESS_RXGB:
          fourcc = FOURCC ('R','X','G','B');
          dxgi_format = DXGI_FORMAT_BC3_UNORM;
          break;

        case DDS_COMPRESS_BC4:
          fourcc = FOURCC ('A','T','I','1');
          dxgi_format = DXGI_FORMAT_BC4_UNORM;
          /*is_dx10 = TRUE;*/
          break;

        case DDS_COMPRESS_BC5:
          fourcc = FOURCC ('A','T','I','2');
          dxgi_format = DXGI_FORMAT_BC5_UNORM;
          /*is_dx10 = TRUE;*/
          break;

        case DDS_COMPRESS_BC7:
          dxgi_format = DXGI_FORMAT_BC7_UNORM;
          is_dx10     = TRUE;
        }

      if ((compression == DDS_COMPRESS_BC3N) ||
          (compression == DDS_COMPRESS_RXGB))
        {
          pflags |= DDPF_NORMAL;
        }

      PUTL32 (hdr + 8,  flags);
      PUTL32 (hdr + 80, pflags);
      PUTL32 (hdr + 84, fourcc);

      /* Linear size */
      size = ((w + 3) >> 2) * ((h + 3) >> 2);
      if ((compression == DDS_COMPRESS_BC1) ||
          (compression == DDS_COMPRESS_BC4))
        size *= 8;
      else
        size *= 16;

      PUTL32 (hdr + 20, size);

      /*
       * write extra fourcc info - this is special to Clay DDS. When the image
       * is read by the plugin, we can detect the added information to decode
       * the pixels
       */
      if (compression == DDS_COMPRESS_AEXP)
        {
          PUTL32 (hdr + 44, FOURCC ('A','E','X','P'));
        }
      else if (compression == DDS_COMPRESS_YCOCG)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','1'));
        }
      else if (compression == DDS_COMPRESS_YCOCGS)
        {
          PUTL32 (hdr + 44, FOURCC ('Y','C','G','2'));
        }
    }

  /* Texture arrays always require a DX10 header */
  if (savetype == DDS_SAVE_ARRAY)
    is_dx10 = TRUE;

  /* Upgrade to DX10 header when desired */
  if (is_dx10)
    {
      array_size = ((savetype == DDS_SAVE_SELECTED_LAYER ||
                     savetype == DDS_SAVE_VISIBLE_LAYERS) ?
                    1 : get_array_size (image));

      PUTL32 (hdr10 +  0, dxgi_format);
      PUTL32 (hdr10 +  4, D3D10_RESOURCE_DIMENSION_TEXTURE2D);
      PUTL32 (hdr10 +  8, 0);
      PUTL32 (hdr10 + 12, array_size);
      PUTL32 (hdr10 + 16, 0);

      /* Update main header accordingly */
      PUTL32 (hdr + 80, pflags | DDPF_FOURCC);
      PUTL32 (hdr + 84, FOURCC ('D','X','1','0'));
    }

  fwrite (hdr, DDS_HEADERSIZE, 1, fp);

  if (is_dx10)
    fwrite (hdr10, DDS_HEADERSIZE_DX10, 1, fp);

  /* Write palette for indexed images */
  if ((basetype == CLAY_INDEXED) &&
      (pixel_format == DDS_FORMAT_DEFAULT) &&
      (compression == DDS_COMPRESS_NONE))
    {
      const guchar zero[4] = {0, 0, 0, 0};
      cmap = clay_palette_get_colormap (clay_image_get_palette (image), babl_format ("R'G'B' u8"), &colors, NULL);

      for (i = 0; i < colors; ++i)
        {
          fwrite (&cmap[3 * i], 1, 3, fp);
          if (i == transindex)
            fputc (0, fp);
          else
            fputc (255, fp);
        }

      /* Pad unused palette space with zeroes */
      for (; i < 256; ++i)
        fwrite (zero, 1, 4, fp);
    }

  if (savetype == DDS_SAVE_CUBEMAP)  /* Write cubemap layers */
    {
      for (i = 0; i < 6; ++i)
        {
          write_layer (fp, image, CLAY_DRAWABLE (cubemap_faces[i]), config,
                       w, h, bpp, fmtbpp,
                       num_mipmaps);
          clay_progress_update ((float)(i + 1) / 6.0);
        }
    }
  else if (savetype == DDS_SAVE_VOLUMEMAP)  /* Write volume slices */
    {
      for (i = 0, list = layers;
           i < num_layers;
           ++i, list = g_list_next (list))
        {
          write_layer (fp, image, list->data, config,
                       w, h, bpp, fmtbpp, 1);
          clay_progress_update ((float)i / (float)num_layers);
        }

      if (num_mipmaps > 1)
        write_volume_mipmaps (fp, image, config, layers, w, h, num_layers,
                              bpp, fmtbpp, num_mipmaps);
    }
  else if (savetype == DDS_SAVE_ARRAY)  /* Write array entries */
    {
      for (i = 0, list = layers;
           i < num_layers;
           ++i, list = g_list_next (list))
        {
          if ((clay_drawable_get_width  (list->data) == w) &&
              (clay_drawable_get_height (list->data) == h))
            {
              write_layer (fp, image, list->data, config,
                           w, h, bpp, fmtbpp, num_mipmaps);
            }

          clay_progress_update ((float)i / (float)num_layers);
        }
    }
  else
    {
      if (savetype == DDS_SAVE_VISIBLE_LAYERS)
        drawable = CLAY_DRAWABLE (clay_image_merge_visible_layers (image,
                                                                   CLAY_CLIP_TO_IMAGE));
      write_layer (fp, image, drawable, config,
                   w, h, bpp, fmtbpp, num_mipmaps);
    }

  clay_progress_update (1.0);

  return TRUE;
}


static void
config_notify (ClayProcedureConfig *config,
               const GParamSpec    *pspec,
               ClayProcedureDialog *dialog)
{
  if (! strcmp (pspec->name, "compression-format"))
    {
      gint compression;

      compression = clay_procedure_config_get_choice_id (config,
                                                         "compression-format");

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "format",
                                           compression == DDS_COMPRESS_NONE,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "perceptual-metric",
                                           compression != DDS_COMPRESS_NONE,
                                           NULL, NULL, FALSE);
    }
  else if (! strcmp (pspec->name, "save-type"))
    {
      GParamSpec *cspec;
      ClayChoice *choice;
      gint        savetype;

      savetype = clay_procedure_config_get_choice_id (config, "save-type");

      switch (savetype)
        {
        case DDS_SAVE_SELECTED_LAYER:
        case DDS_SAVE_VISIBLE_LAYERS:
        case DDS_SAVE_CUBEMAP:
        case DDS_SAVE_ARRAY:
          clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                               "compression-format", TRUE,
                                               NULL, NULL, FALSE);
          break;

        case DDS_SAVE_VOLUMEMAP:
          g_object_set (config,
                        "compression-format", "none",
                        NULL);
          clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                               "compression-format", FALSE,
                                               NULL, NULL, FALSE);
          break;
        }

      cspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "mipmaps");
      choice = clay_param_spec_choice_get_choice (cspec);
      clay_choice_set_sensitive (choice, "existing", check_mipmaps (savetype));
    }
  else if (! strcmp (pspec->name, "mipmaps"))
    {
      gint     mipmaps;
      gboolean gamma_correct;
      gboolean srgb;
      gboolean preserve_alpha_coverage;

      g_object_get (config,
                    "gamma-correct",           &gamma_correct,
                    "srgb",                    &srgb,
                    "preserve-alpha-coverage", &preserve_alpha_coverage,
                    NULL);
      mipmaps = clay_procedure_config_get_choice_id (config, "mipmaps");

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "mipmap-filter",
                                           mipmaps == DDS_MIPMAP_GENERATE,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "mipmap-wrap",
                                           mipmaps == DDS_MIPMAP_GENERATE,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "gamma-correct",
                                           mipmaps == DDS_MIPMAP_GENERATE,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "srgb",
                                           ((mipmaps == DDS_MIPMAP_GENERATE) &&
                                            gamma_correct),
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "gamma",
                                           ((mipmaps == DDS_MIPMAP_GENERATE) &&
                                            gamma_correct && ! srgb),
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "preserve-alpha-coverage",
                                           mipmaps == DDS_MIPMAP_GENERATE,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "alpha-test-threshold",
                                           ((mipmaps == DDS_MIPMAP_GENERATE) &&
                                            preserve_alpha_coverage),
                                           NULL, NULL, FALSE);
    }
  else if (! strcmp (pspec->name, "transparent-color"))
    {
      GtkWidget *transparent_check;
      gboolean   transparent_color;

      g_object_get (config,
                    "transparent-color", &transparent_color,
                    NULL);

      transparent_check = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                                            "transparent-color",
                                                            G_TYPE_NONE);

      if ((transparent_check != NULL) &&
          gtk_widget_get_sensitive (transparent_check))
        clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                             "transparent-index",
                                             transparent_color,
                                             NULL, NULL, FALSE);
    }
  else if (! strcmp (pspec->name, "gamma-correct"))
    {
      gboolean gamma_correct;
      gboolean srgb;

      g_object_get (config,
                    "gamma-correct", &gamma_correct,
                    "srgb",          &srgb,
                    NULL);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "srgb", gamma_correct,
                                           NULL, NULL, FALSE);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "gamma",
                                           (gamma_correct && ! srgb),
                                           NULL, NULL, FALSE);
    }
  else if (! strcmp (pspec->name, "srgb"))
    {
      gboolean gamma_correct;
      gboolean srgb;

      g_object_get (config,
                    "gamma-correct", &gamma_correct,
                    "srgb",          &srgb,
                    NULL);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "gamma",
                                           (gamma_correct && ! srgb),
                                           NULL, NULL, FALSE);
    }
  else if (! strcmp (pspec->name, "preserve-alpha-coverage"))
    {
      gboolean preserve_alpha_coverage;

      g_object_get (config,
                    "preserve-alpha-coverage", &preserve_alpha_coverage,
                    NULL);

      clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                           "alpha-test-threshold",
                                           preserve_alpha_coverage,
                                           NULL, NULL, FALSE);
    }
}


static gboolean
save_dialog (ClayImage           *image,
             ClayDrawable        *drawable,
             ClayProcedure       *procedure,
             ClayProcedureConfig *config)
{
  GtkWidget         *dialog;
  GParamSpec        *cspec;
  ClayChoice        *choice;
  ClayImageBaseType  base_type;
  gboolean           run;

  base_type = clay_image_get_base_type (image);

  if (is_cubemap || is_volume || is_array)
    g_object_set (config,
                  "save-type", "layer",
                  NULL);

  dialog = clay_export_procedure_dialog_new (CLAY_EXPORT_PROCEDURE (procedure),
                                             CLAY_PROCEDURE_CONFIG (config),
                                             image);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "transparent-color",
                                    G_TYPE_NONE);
  clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog),
                                       "transparent-color",
                                       base_type == CLAY_INDEXED,
                                       NULL, NULL, FALSE);

  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "transparency-frame",
                                    "transparent-color", FALSE,
                                    "transparent-index");

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                   "mipmap-options-label",
                                   _("Mipmap Options"),
                                   FALSE, FALSE);

  clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                  "mipmap-options-box",
                                  "mipmap-filter", "mipmap-wrap",
                                  "gamma-correct", "srgb", "gamma",
                                  "preserve-alpha-coverage",
                                  "alpha-test-threshold", NULL);

  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "mipmap-options-frame",
                                    "mipmap-options-label", FALSE,
                                    "mipmap-options-box");

  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "save-type", G_TYPE_NONE);
  cspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "save-type");
  choice = clay_param_spec_choice_get_choice (cspec);
  clay_choice_set_sensitive (choice, "cube",   is_cubemap);
  clay_choice_set_sensitive (choice, "volume", is_volume);
  clay_choice_set_sensitive (choice, "array",  is_array);

  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "mipmaps", G_TYPE_NONE);
  cspec  = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "mipmaps");
  choice = clay_param_spec_choice_get_choice (cspec);
  clay_choice_set_sensitive (choice, "existing",
                             ! (is_volume || is_cubemap) && is_mipmap_chain_valid);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "compression-format", "perceptual-metric",
                              "format", "save-type", "flip-image",
                              "mipmaps", "transparency-frame",
                              "mipmap-options-frame", NULL);

  config_notify (CLAY_PROCEDURE_CONFIG (config),
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "compression-format"),
                 CLAY_PROCEDURE_DIALOG (dialog));

  config_notify (CLAY_PROCEDURE_CONFIG (config),
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "mipmaps"),
                 CLAY_PROCEDURE_DIALOG (dialog));

  config_notify (CLAY_PROCEDURE_CONFIG (config),
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "save-type"),
                 CLAY_PROCEDURE_DIALOG (dialog));

  config_notify (CLAY_PROCEDURE_CONFIG (config),
                 g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                               "transparent-color"),
                 CLAY_PROCEDURE_DIALOG (dialog));

  g_signal_connect (CLAY_PROCEDURE_CONFIG (config), "notify",
                    G_CALLBACK (config_notify),
                    CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  g_signal_handlers_disconnect_by_func (CLAY_PROCEDURE_CONFIG (config),
                                        config_notify,
                                        CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static const gchar *
check_comp_format (guint32 format)
{
  switch (format)
    {
    case DDS_COMPRESS_BC1:
      return "bc1";

    case DDS_COMPRESS_BC2:
      return "bc2";

    case DDS_COMPRESS_BC3:
      return "bc3";

    case DDS_COMPRESS_BC4:
      return "bc4";

    case DDS_COMPRESS_BC5:
      return "bc5";

    case DDS_COMPRESS_BC6H:
    case DDS_COMPRESS_BC7:
      return "bc7";

    default:
      return "none";
    }
}
