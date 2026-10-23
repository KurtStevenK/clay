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

#include "config.h"

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "config/claygeglconfig.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "clay.h"
#include "claycontainer.h"
#include "claydatafactory.h"
#include "claydrawable.h"
#include "clayimage.h"
#include "clayimage-colormap.h"
#include "clayimage-private.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claypalette.h"

#include "clay-intl.h"


typedef struct
{
  GeglBuffer *buffer;
  const Babl *format;

  /* Shared by jobs. */
  gboolean   *found;
  GRWLock    *lock;
} IndexUsedJobData;


/*  local function prototype  */

static void   clay_image_colormap_set_palette_entry    (ClayImage        *image,
                                                        GeglColor        *color,
                                                        gint              index);
static void   clay_image_colormap_thread_is_index_used (IndexUsedJobData *data,
                                                        gint              index);


/*  public functions  */

void
clay_image_colormap_init (ClayImage *image)
{
  ClayImagePrivate *private;
  ClayContainer    *palettes;
  gchar            *palette_name;
  gchar            *palette_id;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette == NULL);

  palette_name = g_strdup_printf (_("Colormap of Image #%d (%s)"),
                                  clay_image_get_id (image),
                                  clay_image_get_display_name (image));
  palette_id = g_strdup_printf ("clay-indexed-image-palette-%d",
                                clay_image_get_id (image));

  private->palette = CLAY_PALETTE (clay_palette_new (NULL, palette_name));

  clay_image_colormap_update_formats (image);

  clay_palette_set_columns (private->palette, 16);

  clay_data_set_image (CLAY_DATA (private->palette), image, TRUE, FALSE);

  palettes = clay_data_factory_get_container (image->clay->palette_factory);

  clay_container_add (palettes, CLAY_OBJECT (private->palette));

  g_free (palette_name);
  g_free (palette_id);
}

void
clay_image_colormap_dispose (ClayImage *image)
{
  ClayImagePrivate *private;
  ClayContainer    *palettes;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (CLAY_IS_PALETTE (private->palette));

  palettes = clay_data_factory_get_container (image->clay->palette_factory);

  clay_container_remove (palettes, CLAY_OBJECT (private->palette));
}

void
clay_image_colormap_free (ClayImage *image)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (CLAY_IS_PALETTE (private->palette));

  g_clear_object (&private->palette);

  /* don't touch the image's babl_palettes because we might still have
   * buffers with that palette on the undo stack, and on undoing the
   * image back to indexed, we must have exactly these palettes around
   */
}

void
clay_image_colormap_update_formats (ClayImage *image)
{
  ClayImagePrivate *private;
  const Babl       *space;
  const Babl       *format;
  gchar            *format_name;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  space = clay_image_get_layer_space (image);

  format_name = g_strdup_printf ("-clay-indexed-format-%d",
                                 clay_image_get_id (image));

  babl_new_palette_with_space (format_name, space,
                               &private->babl_palette_rgb,
                               &private->babl_palette_rgba);

  format = clay_babl_format (CLAY_RGB, private->precision, FALSE, space),
  g_free (format_name);

  if (private->palette && clay_palette_get_n_colors (private->palette) > 0)
    {
      guchar *colormap;
      gint    n_colors;

      clay_palette_restrict_format (private->palette, format, FALSE);

      colormap = _clay_image_get_colormap (image, &n_colors);
      g_return_if_fail (colormap != NULL);

      babl_palette_set_palette (private->babl_palette_rgb,
                                format, colormap, n_colors);
      babl_palette_set_palette (private->babl_palette_rgba,
                                format, colormap, n_colors);

      g_free (colormap);
    }
}

const Babl *
clay_image_colormap_get_rgb_format (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->babl_palette_rgb;
}

const Babl *
clay_image_colormap_get_rgba_format (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->babl_palette_rgba;
}

ClayPalette *
clay_image_get_colormap_palette (ClayImage *image)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  return CLAY_IMAGE_GET_PRIVATE (image)->palette;
}

void
clay_image_set_colormap_palette (ClayImage   *image,
                                 ClayPalette *palette,
                                 gboolean     push_undo)
{
  ClayImagePrivate *private;
  ClayPaletteEntry *entry;
  gint              n_colors, i;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (palette != NULL);
  n_colors = clay_palette_get_n_colors (palette);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    clay_image_undo_push_image_colormap (image, C_("undo-type", "Set Colormap"));

  if (!private->palette)
    clay_image_colormap_init (image);

  clay_data_freeze (CLAY_DATA (private->palette));

  while ((entry = clay_palette_get_entry (private->palette, 0)))
    clay_palette_delete_entry (private->palette, entry);

  for (i = 0; i < n_colors; i++)
    {
      ClayPaletteEntry *entry = clay_palette_get_entry (palette, i);

      clay_image_colormap_set_palette_entry (image, entry->color, i);
    }

  clay_data_thaw (CLAY_DATA (private->palette));

  clay_image_colormap_changed (image, -1);
}

guchar *
_clay_image_get_colormap (ClayImage *image,
                          gint      *n_colors)
{
  ClayImagePrivate *private;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    return NULL;

  g_return_val_if_fail (private->palette->format != NULL, NULL);

  return clay_palette_get_colormap (private->palette, private->palette->format, n_colors);
}

void
_clay_image_set_colormap (ClayImage    *image,
                          const guchar *colormap,
                          gint          n_colors,
                          gboolean      push_undo)
{
  ClayImagePrivate *private;
  const Babl       *space;
  const Babl       *format;

  g_return_if_fail (CLAY_IS_IMAGE (image));
  g_return_if_fail (colormap != NULL || n_colors == 0);
  g_return_if_fail (n_colors >= 0 && n_colors <= 256);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (private->palette == NULL)
    clay_image_colormap_init (image);

  space  = clay_image_get_layer_space (image);
  format = clay_babl_format (CLAY_RGB, private->precision, FALSE, space);

  clay_palette_set_colormap (private->palette, format, colormap, n_colors, push_undo);
  clay_image_colormap_changed (image, -1);
}

void
clay_image_unset_colormap (ClayImage *image,
                           gboolean   push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  if (push_undo)
    clay_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Unset Colormap"));

  if (private->palette)
    {
      clay_image_colormap_dispose (image);
      clay_image_colormap_free (image);
    }

  clay_image_colormap_changed (image, -1);
}

gboolean
clay_image_colormap_is_index_used (ClayImage *image,
                                   gint       color_index)
{
  GList       *layers;
  GList       *iter;
  GThreadPool *pool;
  GRWLock      lock;
  gboolean     found = FALSE;
  gint         num_processors;

  g_rw_lock_init (&lock);
  num_processors = CLAY_GEGL_CONFIG (image->clay->config)->num_processors;
  layers         = clay_image_get_layer_list (image);

  pool = g_thread_pool_new_full ((GFunc) clay_image_colormap_thread_is_index_used,
                                 GINT_TO_POINTER (color_index),
                                 (GDestroyNotify) g_free,
                                 num_processors, TRUE, NULL);
  for (iter = layers; iter; iter = g_list_next (iter))
    {
      IndexUsedJobData *job_data;

      job_data = g_malloc (sizeof (IndexUsedJobData ));
      job_data->buffer = clay_drawable_get_buffer (iter->data);
      job_data->format = clay_drawable_get_format_without_alpha (iter->data);
      job_data->found  = &found;
      job_data->lock   = &lock;

      g_thread_pool_push (pool, job_data, NULL);
    }

  g_thread_pool_free (pool, FALSE, TRUE);
  g_rw_lock_clear (&lock);
  g_list_free (layers);

  return found;
}

GeglColor *
clay_image_get_colormap_entry (ClayImage *image,
                               gint       color_index)
{
  ClayImagePrivate *private;
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_val_if_fail (private->palette != NULL, NULL);
  g_return_val_if_fail (color_index >= 0 &&
                        color_index < clay_palette_get_n_colors (private->palette),
                        NULL);

  entry = clay_palette_get_entry (private->palette, color_index);

  g_return_val_if_fail (entry != NULL, NULL);

  return entry->color;
}

void
clay_image_set_colormap_entry (ClayImage *image,
                               gint       color_index,
                               GeglColor *color,
                               gboolean   push_undo)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (color_index >= 0 &&
                    color_index < clay_palette_get_n_colors (private->palette));
  g_return_if_fail (GEGL_IS_COLOR (color));

  if (push_undo)
    clay_image_undo_push_image_colormap (image,
                                         C_("undo-type", "Change Colormap entry"));

  clay_image_colormap_set_palette_entry (image, color, color_index);

  clay_image_colormap_changed (image, color_index);
}

void
clay_image_add_colormap_entry (ClayImage *image,
                               GeglColor *color)
{
  ClayImagePrivate *private;

  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_IMAGE_GET_PRIVATE (image);

  g_return_if_fail (private->palette != NULL);
  g_return_if_fail (clay_palette_get_n_colors (private->palette) < 256);
  g_return_if_fail (GEGL_IS_COLOR (color));

  clay_image_undo_push_image_colormap (image,
                                       C_("undo-type", "Add Color to Colormap"));

  clay_image_colormap_set_palette_entry (image, color,
                                         clay_palette_get_n_colors (private->palette));

  clay_image_colormap_changed (image, -1);
}

gboolean
clay_image_delete_colormap_entry (ClayImage *image,
                                  gint       color_index,
                                  gboolean   push_undo)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), FALSE);

  if (! clay_image_colormap_is_index_used (image, color_index))
    {
      ClayImagePrivate *private;
      ClayPaletteEntry *entry;
      GList            *layers;
      GList            *iter;

      if (push_undo)
        {
          clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP,
                                       C_("undo-type", "Delete Colormap entry"));

          clay_image_undo_push_image_colormap (image, NULL);
        }

      private = CLAY_IMAGE_GET_PRIVATE (image);
      layers  = clay_image_get_layer_list (image);

      for (iter = layers; iter; iter = g_list_next (iter))
        {
          if (push_undo)
            clay_image_undo_push_drawable_mod (image, NULL, iter->data, TRUE);

          clay_gegl_shift_index (clay_drawable_get_buffer (iter->data), NULL,
                                 clay_drawable_get_format (iter->data),
                                 color_index, -1);
        }

      entry = clay_palette_get_entry (private->palette, color_index);
      clay_palette_delete_entry (private->palette, entry);

      g_list_free (layers);

      if (push_undo)
        clay_image_undo_group_end (image);

      clay_image_colormap_changed (image, -1);

      return TRUE;
    }

  return FALSE;
}


/*  private functions  */

static void
clay_image_colormap_set_palette_entry (ClayImage *image,
                                       GeglColor *color,
                                       gint       index)
{
  ClayImagePrivate *private   = CLAY_IMAGE_GET_PRIVATE (image);
  GeglColor        *new_color = gegl_color_new ("black");
  const Babl       *space;
  const Babl       *format;
  guint8            rgb[3];
  gchar             name[64];
  gint              i;

  g_return_if_fail (GEGL_IS_COLOR (color));

  /* Adding black entries if needed. */
  for (i = clay_palette_get_n_colors (private->palette);
       i <= index;
       i++)
    {
      g_snprintf (name, sizeof (name), "#%d", i);
      clay_palette_add_entry (private->palette, index, name, new_color);
    }

  space  = clay_image_get_layer_space (image);
  format = clay_babl_format (CLAY_RGB, private->precision, FALSE, space);

  /* For image colormap, we force the color to be in the target format (which at
   * time of writing is only "R'G'B' u8" for the target space).
   */
  gegl_color_get_pixel (color, format, rgb);
  gegl_color_set_pixel (new_color, format, rgb);
  clay_palette_set_entry (private->palette, index, name, new_color);
  g_object_unref (new_color);
}

static void
clay_image_colormap_thread_is_index_used (IndexUsedJobData *data,
                                          gint              index)
{
  g_rw_lock_reader_lock (data->lock);
  if (*data->found)
    {
      g_rw_lock_reader_unlock (data->lock);
      return;
    }
  g_rw_lock_reader_unlock (data->lock);
  if (clay_gegl_is_index_used (data->buffer, NULL, data->format, index))
    {
      g_rw_lock_writer_lock (data->lock);
      *data->found = TRUE;
      g_rw_lock_writer_unlock (data->lock);
    }
}
