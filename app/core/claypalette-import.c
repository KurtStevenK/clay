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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "claychannel.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "claygradient.h"
#include "clayimage.h"
#include "clayimage-colormap.h"
#include "claypalette.h"
#include "claypalette-import.h"
#include "claypalette-load.h"
#include "claypickable.h"

#include "clay-intl.h"


#define MAX_IMAGE_COLORS (10000 * 2)


/*  create a palette from a gradient  ****************************************/

ClayPalette *
clay_palette_import_from_gradient (ClayGradient                *gradient,
                                   ClayContext                 *context,
                                   gboolean                     reverse,
                                   ClayGradientBlendColorSpace  blend_color_space,
                                   const gchar                 *palette_name,
                                   gint                         n_colors)
{
  ClayPalette         *palette;
  ClayGradientSegment *seg = NULL;
  gdouble              dx, cur_x;
  gint                 i;

  g_return_val_if_fail (CLAY_IS_GRADIENT (gradient), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);

  palette = CLAY_PALETTE (clay_palette_new (context, palette_name));

  dx = 1.0 / (n_colors - 1);

  for (i = 0, cur_x = 0; i < n_colors; i++, cur_x += dx)
    {
      GeglColor *color = NULL;

      seg = clay_gradient_get_color_at (gradient, context,
                                        seg, cur_x, reverse, blend_color_space,
                                        &color);

      g_return_val_if_fail (color != NULL, palette);

      clay_palette_add_entry (palette, -1, NULL, color);

      g_clear_object (&color);
    }

  return palette;
}


/*  create a palette from a non-indexed image  *******************************/

typedef struct _ImgColors ImgColors;

struct _ImgColors
{
  guint  count;
  guint  r_adj;
  guint  g_adj;
  guint  b_adj;
  guchar r;
  guchar g;
  guchar b;
};

static gint count_color_entries = 0;

static GHashTable *
clay_palette_import_store_colors (GHashTable *table,
                                  guchar     *colors,
                                  guchar     *colors_real,
                                  gint        n_colors)
{
  gpointer   found_color = NULL;
  ImgColors *new_color;
  guint      key_colors = colors[0] * 256 * 256 + colors[1] * 256 + colors[2];

  if (table == NULL)
    {
      table = g_hash_table_new (g_direct_hash, g_direct_equal);
      count_color_entries = 0;
    }
  else
    {
      found_color = g_hash_table_lookup (table, GUINT_TO_POINTER (key_colors));
    }

  if (found_color == NULL)
    {
      if (count_color_entries > MAX_IMAGE_COLORS)
        {
          /* Don't add any more new ones */
          return table;
        }

      count_color_entries++;

      new_color = g_slice_new (ImgColors);

      new_color->count = 1;
      new_color->r_adj = 0;
      new_color->g_adj = 0;
      new_color->b_adj = 0;
      new_color->r     = colors[0];
      new_color->g     = colors[1];
      new_color->b     = colors[2];

      g_hash_table_insert (table, GUINT_TO_POINTER (key_colors), new_color);
    }
  else
    {
      new_color = found_color;

      if (new_color->count < (G_MAXINT - 1))
        new_color->count++;

      /* Now do the adjustments ...*/
      new_color->r_adj += (colors_real[0] - colors[0]);
      new_color->g_adj += (colors_real[1] - colors[1]);
      new_color->b_adj += (colors_real[2] - colors[2]);

      /* Boundary conditions */
      if(new_color->r_adj > (G_MAXINT - 255))
        new_color->r_adj /= new_color->count;

      if(new_color->g_adj > (G_MAXINT - 255))
        new_color->g_adj /= new_color->count;

      if(new_color->b_adj > (G_MAXINT - 255))
        new_color->b_adj /= new_color->count;
    }

  return table;
}

static void
clay_palette_import_create_list (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
  GSList    **list      = user_data;
  ImgColors  *color_tab = value;

  *list = g_slist_prepend (*list, color_tab);
}

static gint
clay_palette_import_sort_colors (gconstpointer a,
                                 gconstpointer b)
{
  const ImgColors *s1 = a;
  const ImgColors *s2 = b;

  if(s1->count > s2->count)
    return -1;
  if(s1->count < s2->count)
    return 1;

  return 0;
}

static void
clay_palette_import_create_image_palette (gpointer data,
                                          gpointer user_data)
{
  ClayPalette *palette   = user_data;
  ImgColors   *color_tab = data;
  const Babl  *format;
  gint         n_colors;
  gchar       *lab;
  GeglColor   *color;
  guint8       rgb[4];

  n_colors = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (palette),
                                                 "import-n-colors"));

  if (clay_palette_get_n_colors (palette) >= n_colors)
    return;

  format = clay_palette_get_restriction (palette);

  /* TRANSLATORS: the "%s" is an item title and "%u" is the number of
     occurrences for this item. */
  lab = g_strdup_printf (_("%s (occurs %u)"),
                         _("Untitled"),
                         color_tab->count);

  /* Adjust the colors to the mean of the the sample */
  color = gegl_color_new (NULL);
  rgb[0] = (guchar) color_tab->r + (color_tab->r_adj / color_tab->count);
  rgb[1] = (guchar) color_tab->g + (color_tab->g_adj / color_tab->count);
  rgb[2] = (guchar) color_tab->b + (color_tab->b_adj / color_tab->count);
  /* TODO: We only receive the RGB values, so if the format has alpha, it
   * becomes transparent when used outside of a palette. For now, we'll set
   * alpha to 255, but in the future, a less RGB-specific implementation
   * would be ideal */
  if (babl_format_has_alpha (format))
    rgb[3] = 255;

  gegl_color_set_pixel (color, format, rgb);

  clay_palette_add_entry (palette, -1, lab, color);

  g_object_unref (color);
  g_free (lab);
}

static ClayPalette *
clay_palette_import_make_palette (GHashTable  *table,
                                  const Babl  *format,
                                  const gchar *palette_name,
                                  ClayContext *context,
                                  gint         n_colors)
{
  ClayPalette *palette;
  GSList      *list = NULL;
  GSList      *iter;

  palette = CLAY_PALETTE (clay_palette_new (context, palette_name));

  if (! table)
    return palette;

  clay_palette_restrict_format (palette, format, FALSE);
  g_hash_table_foreach (table, clay_palette_import_create_list, &list);
  list = g_slist_sort (list, clay_palette_import_sort_colors);

  g_object_set_data (G_OBJECT (palette), "import-n-colors",
                     GINT_TO_POINTER (n_colors));

  g_slist_foreach (list, clay_palette_import_create_image_palette, palette);

  g_object_set_data (G_OBJECT (palette), "import-n-colors", NULL);

  /*  Free up used memory
   *  Note the same structure is on both the hash list and the sorted
   *  list. So only delete it once.
   */
  g_hash_table_destroy (table);

  for (iter = list; iter; iter = iter->next)
    g_slice_free (ImgColors, iter->data);

  g_slist_free (list);

  clay_palette_restrict_format (palette, NULL, FALSE);

  return palette;
}

static GHashTable *
clay_palette_import_extract (ClayImage     *image,
                             GHashTable    *colors,
                             const Babl   **format,
                             ClayPickable  *pickable,
                             gint           pickable_off_x,
                             gint           pickable_off_y,
                             gboolean       selection_only,
                             gint           x,
                             gint           y,
                             gint           width,
                             gint           height,
                             gint           n_colors,
                             gint           threshold)
{
  GeglBuffer         *buffer;
  GeglBufferIterator *iter;
  GeglRectangle      *mask_roi = NULL;
  GeglRectangle       rect     = { x, y, width, height };
  gint                bpp;
  gint                mask_bpp = 0;

  buffer = clay_pickable_get_buffer (pickable);
  if (*format == NULL)
    *format = babl_format_with_space ("R'G'B'A u8", clay_image_get_layer_space (image));

  iter = gegl_buffer_iterator_new (buffer, &rect, 0, *format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  bpp = babl_format_get_bytes_per_pixel (*format);

  if (selection_only &&
      ! clay_channel_is_empty (clay_image_get_mask (image)))
    {
      ClayDrawable *mask = CLAY_DRAWABLE (clay_image_get_mask (image));
      const Babl   *selection_format;

      rect.x = x + pickable_off_x;
      rect.y = y + pickable_off_y;

      buffer = clay_drawable_get_buffer (mask);
      selection_format = babl_format ("Y u8");

      gegl_buffer_iterator_add (iter, buffer, &rect, 0, selection_format,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      mask_roi = &iter->items[1].roi;
      mask_bpp = babl_format_get_bytes_per_pixel (selection_format);
    }

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *data      = iter->items[0].data;
      const guchar *mask_data = NULL;
      gint          length    = iter->length;

      if (mask_roi)
        mask_data = iter->items[1].data;

      while (length--)
        {
          /*  ignore unselected, and completely transparent pixels  */
          if ((! mask_data || *mask_data) && data[ALPHA])
            {
              guchar rgba[MAX_CHANNELS]     = { 0, };
              guchar rgb_real[MAX_CHANNELS] = { 0, };

              memcpy (rgba, data, 4);
              memcpy (rgb_real, rgba, 4);

              rgba[0] = (rgba[0] / threshold) * threshold;
              rgba[1] = (rgba[1] / threshold) * threshold;
              rgba[2] = (rgba[2] / threshold) * threshold;

              colors = clay_palette_import_store_colors (colors,
                                                         rgba, rgb_real,
                                                         n_colors);
            }

          data += bpp;

          if (mask_data)
            mask_data += mask_bpp;
        }
    }

  return colors;
}

ClayPalette *
clay_palette_import_from_image (ClayImage   *image,
                                ClayContext *context,
                                const gchar *palette_name,
                                gint         n_colors,
                                gint         threshold,
                                gboolean     selection_only)
{
  GHashTable *colors;
  const Babl *format = NULL;
  gint        x, y;
  gint        width, height;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);
  g_return_val_if_fail (threshold > 0, NULL);

  clay_pickable_flush (CLAY_PICKABLE (image));

  if (selection_only)
    {
      clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                        &x, &y, &width, &height);
    }
  else
    {
      x      = 0;
      y      = 0;
      width  = clay_image_get_width  (image);
      height = clay_image_get_height (image);
    }

  colors = clay_palette_import_extract (image,
                                        NULL, &format,
                                        CLAY_PICKABLE (image),
                                        0, 0,
                                        selection_only,
                                        x, y, width, height,
                                        n_colors, threshold);

  return clay_palette_import_make_palette (colors, format, palette_name,
                                           context, n_colors);
}


/*  create a palette from an indexed image  **********************************/

ClayPalette *
clay_palette_import_from_indexed_image (ClayImage   *image,
                                        ClayContext *context,
                                        const gchar *palette_name)
{
  ClayPalette  *palette;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (clay_image_get_base_type (image) == CLAY_INDEXED, NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);

  palette = CLAY_PALETTE (clay_data_duplicate (CLAY_DATA (clay_image_get_colormap_palette (image))));

  clay_object_set_name (CLAY_OBJECT (palette), palette_name);

  return palette;
}


/*  create a palette from a drawable  ****************************************/

ClayPalette *
clay_palette_import_from_drawables (GList       *drawables,
                                    ClayContext *context,
                                    const gchar *palette_name,
                                    gint         n_colors,
                                    gint         threshold,
                                    gboolean     selection_only)
{
  GHashTable *colors = NULL;
  const Babl *format = NULL;
  GList      *iter;
  gint        x, y;
  gint        width, height;
  gint        off_x, off_y;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (n_colors > 1, NULL);
  g_return_val_if_fail (threshold > 0, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      ClayDrawable *drawable = iter->data;

      g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
      g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);

      if (selection_only)
        {
          if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                          &x, &y, &width, &height))
            return NULL;
        }
      else
        {
          x      = 0;
          y      = 0;
          width  = clay_item_get_width  (CLAY_ITEM (drawable));
          height = clay_item_get_height (CLAY_ITEM (drawable));
        }

      clay_item_get_offset (CLAY_ITEM (drawable), &off_x, &off_y);

      colors =
        clay_palette_import_extract (clay_item_get_image (CLAY_ITEM (drawable)),
                                     colors, &format,
                                     CLAY_PICKABLE (drawable),
                                     off_x, off_y,
                                     selection_only,
                                     x, y, width, height,
                                     n_colors, threshold);
    }

  return clay_palette_import_make_palette (colors, format, palette_name,
                                           context, n_colors);
}


/*  create a palette from a file  **********************************/

ClayPalette *
clay_palette_import_from_file (ClayContext  *context,
                               GFile        *file,
                               const gchar  *palette_name,
                               GError      **error)
{
  GList        *palette_list = NULL;
  GInputStream *input;
  GError       *my_error = NULL;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (palette_name != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));
  if (! input)
    {
      g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   clay_file_get_utf8_name (file), my_error->message);
      g_clear_error (&my_error);
      return NULL;
    }

  switch (clay_palette_load_detect_format (file, input))
    {
    case CLAY_PALETTE_FILE_FORMAT_GPL:
      palette_list = clay_palette_load (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_ACT:
      palette_list = clay_palette_load_act (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_RIFF_PAL:
      palette_list = clay_palette_load_riff (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_PSP_PAL:
      palette_list = clay_palette_load_psp (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_ACO:
      palette_list = clay_palette_load_aco (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_ACB:
      palette_list = clay_palette_load_acb (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_ASE:
      palette_list = clay_palette_load_ase (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_CSS:
      palette_list = clay_palette_load_css (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_SBZ:
      palette_list = clay_palette_load_sbz (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_PROCREATE:
      palette_list = clay_palette_load_procreate (context, file, input, error);
      break;

    case CLAY_PALETTE_FILE_FORMAT_KRITA:
      palette_list = clay_palette_load_krita (context, file, input, error);
      break;

    default:
      g_set_error (error,
                   CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                   _("Unknown type of palette file: %s"),
                   clay_file_get_utf8_name (file));
      break;
    }

  g_object_unref (input);

  if (palette_list)
    {
      ClayPalette *palette = g_object_ref (palette_list->data);

      clay_object_set_name (CLAY_OBJECT (palette), palette_name);

      g_list_free_full (palette_list, (GDestroyNotify) g_object_unref);

      return palette;
    }

  return NULL;
}
