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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "gegl/clay-babl.h"

#include "clay-memsize.h"
#include "clayimage.h"
#include "clayimage-undo-push.h"
#include "claypalette.h"
#include "claypalette-load.h"
#include "claypalette-save.h"
#include "claysavable.h"
#include "claytagged.h"
#include "claytempbuf.h"

#include "clay-intl.h"


#define RGB_EPSILON 1e-6

enum
{
  ENTRY_CHANGED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void          clay_palette_tagged_iface_init   (ClayTaggedInterface  *iface);
static void          clay_palette_savable_iface_init  (ClaySavableInterface *iface);

static void          clay_palette_finalize            (GObject              *object);

static gint64        clay_palette_get_memsize         (ClayObject           *object,
                                                       gint64               *gui_size);

static void          clay_palette_get_preview_size    (ClayViewable         *viewable,
                                                       gint                  size,
                                                       gboolean              popup,
                                                       gboolean              dot_for_dot,
                                                       gint                 *width,
                                                       gint                 *height);
static gboolean      clay_palette_get_popup_size      (ClayViewable         *viewable,
                                                       gint                  width,
                                                       gint                  height,
                                                       gboolean              dot_for_dot,
                                                       gint                 *popup_width,
                                                       gint                 *popup_height);
static ClayTempBuf * clay_palette_get_new_preview     (ClayViewable         *viewable,
                                                       ClayContext          *context,
                                                       gint                  width,
                                                       gint                  height,
                                                       gint                  scale_factor,
                                                       GeglColor            *fg_color);
static gchar       * clay_palette_get_description     (ClayViewable         *viewable,
                                                       gchar               **tooltip);
static const gchar * clay_palette_get_extension       (ClayData             *data);
static void          clay_palette_copy                (ClayData             *data,
                                                       ClayData             *src_data);
static void          clay_palette_real_entry_changed  (ClayPalette         *palette,
                                                       gint                 index);

static void          clay_palette_entry_free          (ClayPaletteEntry     *entry);
static gint64        clay_palette_entry_get_memsize   (ClayPaletteEntry     *entry,
                                                       gint64               *gui_size);
static gchar       * clay_palette_get_checksum        (ClayTagged           *tagged);

static void          clay_palette_savable_save        (ClaySavable          *savable,
                                                       ClaySaveState        *state);

static void          clay_palette_image_space_updated (ClayImage            *image,
                                                       ClayPalette          *palette);
static void          clay_palette_notify_image        (ClayPalette          *palette,
                                                       const GParamSpec     *unused,
                                                       gpointer              unused_user_data);


G_DEFINE_TYPE_WITH_CODE (ClayPalette, clay_palette, CLAY_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_TAGGED,
                                                clay_palette_tagged_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_palette_savable_iface_init))

#define parent_class clay_palette_parent_class

static guint signals[LAST_SIGNAL] = { 0 };

static void
clay_palette_class_init (ClayPaletteClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayDataClass     *data_class        = CLAY_DATA_CLASS (klass);

  signals[ENTRY_CHANGED] = g_signal_new ("entry-changed",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_FIRST,
                                         G_STRUCT_OFFSET (ClayPaletteClass, entry_changed),
                                         NULL, NULL, NULL,
                                         G_TYPE_NONE, 1,
                                         G_TYPE_INT);

  object_class->finalize            = clay_palette_finalize;

  clay_object_class->get_memsize    = clay_palette_get_memsize;

  viewable_class->default_icon_name = "gtk-select-color";
  viewable_class->default_name      = _("Palette");
  viewable_class->get_preview_size  = clay_palette_get_preview_size;
  viewable_class->get_popup_size    = clay_palette_get_popup_size;
  viewable_class->get_new_preview   = clay_palette_get_new_preview;
  viewable_class->get_description   = clay_palette_get_description;

  data_class->save                  = clay_palette_save;
  data_class->get_extension         = clay_palette_get_extension;
  data_class->copy                  = clay_palette_copy;

  klass->entry_changed              = clay_palette_real_entry_changed;
}

static void
clay_palette_tagged_iface_init (ClayTaggedInterface *iface)
{
  iface->get_checksum = clay_palette_get_checksum;
}

static void
clay_palette_savable_iface_init (ClaySavableInterface *iface)
{
  iface->save = clay_palette_savable_save;
}

static void
clay_palette_init (ClayPalette *palette)
{
  palette->colors    = NULL;
  palette->n_colors  = 0;
  palette->n_columns = 0;
  palette->image     = NULL;
  palette->format    = NULL;

  g_signal_connect (palette, "notify::image",
                    G_CALLBACK (clay_palette_notify_image),
                    NULL);
}

static void
clay_palette_finalize (GObject *object)
{
  ClayPalette *palette = CLAY_PALETTE (object);

  if (palette->colors)
    {
      g_list_free_full (palette->colors,
                        (GDestroyNotify) clay_palette_entry_free);
      palette->colors = NULL;
    }

  g_clear_weak_pointer (&palette->image);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_palette_get_memsize (ClayObject *object,
                          gint64     *gui_size)
{
  ClayPalette *palette = CLAY_PALETTE (object);
  gint64       memsize = 0;

  memsize += clay_g_list_get_memsize_foreach (palette->colors,
                                              (ClayMemsizeFunc)
                                              clay_palette_entry_get_memsize,
                                              gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_palette_get_preview_size (ClayViewable *viewable,
                               gint          size,
                               gboolean      popup,
                               gboolean      dot_for_dot,
                               gint         *width,
                               gint         *height)
{
  *width  = size;
  *height = 1 + size / 2;
}

static gboolean
clay_palette_get_popup_size (ClayViewable *viewable,
                             gint          width,
                             gint          height,
                             gboolean      dot_for_dot,
                             gint         *popup_width,
                             gint         *popup_height)
{
  ClayPalette *palette = CLAY_PALETTE (viewable);
  gint         p_width;
  gint         p_height;

  if (! palette->n_colors)
    return FALSE;

  if (palette->n_columns)
    p_width = palette->n_columns;
  else
    p_width = MIN (palette->n_colors, 16);

  p_height = MAX (1, palette->n_colors / p_width);

  if (p_width * 4 > width || p_height * 4 > height)
    {
      *popup_width  = p_width  * 4;
      *popup_height = p_height * 4;

      return TRUE;
    }

  return FALSE;
}

static ClayTempBuf *
clay_palette_get_new_preview (ClayViewable *viewable,
                              ClayContext  *context,
                              gint          width,
                              gint          height,
                              gint          scale_factor,
                              GeglColor    *fg_color G_GNUC_UNUSED)
{
  ClayPalette *palette  = CLAY_PALETTE (viewable);
  ClayTempBuf *temp_buf;
  guchar      *buf;
  guchar      *b;
  GList       *list;
  gint         columns;
  gint         rows;
  gint         cell_size;
  gint         x, y;

  width  *= scale_factor;
  height *= scale_factor;

  temp_buf = clay_temp_buf_new (width, height, babl_format ("R'G'B' u8"));
  memset (clay_temp_buf_get_data (temp_buf), 255, width * height * 3);

  if (palette->n_columns > 1)
    cell_size = MAX (4, width / palette->n_columns);
  else
    cell_size = 4;

  columns = width  / cell_size;
  rows    = height / cell_size;

  buf = clay_temp_buf_get_data (temp_buf);
  b   = g_new (guchar, width * 3);

  list = palette->colors;

  for (y = 0; y < rows && list; y++)
    {
      gint i;

      memset (b, 255, width * 3);

      for (x = 0; x < columns && list; x++)
        {
          ClayPaletteEntry *entry = list->data;

          list = g_list_next (list);

          gegl_color_get_pixel (entry->color, babl_format ("R'G'B' u8"), &b[x * cell_size * 3]);

          for (i = 1; i < cell_size; i++)
            {
              b[(x * cell_size + i) * 3 + 0] = b[(x * cell_size) * 3 + 0];
              b[(x * cell_size + i) * 3 + 1] = b[(x * cell_size) * 3 + 1];
              b[(x * cell_size + i) * 3 + 2] = b[(x * cell_size) * 3 + 2];
            }
        }

      for (i = 0; i < cell_size; i++)
        memcpy (buf + ((y * cell_size + i) * width) * 3, b, width * 3);
    }

  g_free (b);

  return temp_buf;
}

static gchar *
clay_palette_get_description (ClayViewable  *viewable,
                              gchar        **tooltip)
{
  ClayPalette *palette = CLAY_PALETTE (viewable);

  return g_strdup_printf ("%s (%d)",
                          clay_object_get_name (palette),
                          palette->n_colors);
}

ClayData *
clay_palette_new (ClayContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  return g_object_new (CLAY_TYPE_PALETTE,
                       "name", name,
                       NULL);
}

ClayData *
clay_palette_get_standard (ClayContext *context)
{
  static ClayData *standard_palette = NULL;

  if (! standard_palette)
    {
      g_set_weak_pointer (&standard_palette,
                          clay_palette_new (context, "Standard"));

      clay_data_clean (standard_palette);
      clay_data_make_internal (standard_palette, "clay-palette-standard");
    }

  return standard_palette;
}

static const gchar *
clay_palette_get_extension (ClayData *data)
{
  return CLAY_PALETTE_FILE_EXTENSION;
}

static void
clay_palette_copy (ClayData *data,
                   ClayData *src_data)
{
  ClayPalette *palette     = CLAY_PALETTE (data);
  ClayPalette *src_palette = CLAY_PALETTE (src_data);
  GList       *list;

  clay_data_freeze (data);

  if (palette->colors)
    {
      g_list_free_full (palette->colors,
                        (GDestroyNotify) clay_palette_entry_free);
      palette->colors = NULL;
    }

  palette->n_colors  = 0;
  palette->n_columns = src_palette->n_columns;
  palette->format    = src_palette->format;
  /* Note: we don't copy the image variable on purpose (same as we don't
   * copy the one from the parent ClayData, nor the file), and we don't
   * want to keep syncing our format with changes in this image.
   */

  for (list = src_palette->colors; list; list = g_list_next (list))
    {
      ClayPaletteEntry *entry = list->data;

      /* Small validity check. */
      g_return_if_fail (! palette->format || palette->format == gegl_color_get_format (entry->color));
      clay_palette_add_entry (palette, -1, entry->name, entry->color);
    }

  clay_data_thaw (data);
}

static void
clay_palette_real_entry_changed (ClayPalette *palette,
                                 gint         index)
{
  ClayImage *image = clay_data_get_image (CLAY_DATA (palette));

  if (image != NULL)
    clay_image_colormap_changed (image, index);
}

static gchar *
clay_palette_get_checksum (ClayTagged *tagged)
{
  const Babl  *srgb            = babl_space ("sRGB");
  ClayPalette *palette         = CLAY_PALETTE (tagged);
  gchar       *checksum_string = NULL;

  if (palette->n_colors > 0)
    {
      GChecksum *checksum       = g_checksum_new (G_CHECKSUM_MD5);
      GList     *color_iterator = palette->colors;

      g_checksum_update (checksum, (const guchar *) &palette->n_colors, sizeof (palette->n_colors));
      g_checksum_update (checksum, (const guchar *) &palette->n_columns, sizeof (palette->n_columns));

      while (color_iterator)
        {
          ClayPaletteEntry *entry    = (ClayPaletteEntry *) color_iterator->data;
          const Babl       *format   = gegl_color_get_format (entry->color);
          const Babl       *space    = babl_format_get_space (format);
          const char       *encoding = babl_format_get_encoding (format);
          gint              bpp      = babl_format_get_bytes_per_pixel (format);
          guint8            pixel[48];

          g_checksum_update (checksum, (const guchar *) encoding, strlen (encoding));
          if (space != srgb)
            {
              const char *icc;
              int         icc_length;

              icc = babl_space_get_icc (space, &icc_length);
              if (icc)
                g_checksum_update (checksum, (const guchar *) icc, icc_length);
            }
          gegl_color_get_pixel (entry->color, format, pixel);
          g_checksum_update (checksum, (const guchar *) pixel, bpp);

          if (entry->name)
            g_checksum_update (checksum, (const guchar *) entry->name, strlen (entry->name));

          color_iterator = g_list_next (color_iterator);
        }

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

static void
clay_palette_savable_save (ClaySavable   *savable,
                           ClaySaveState *state)
{
  ClayPalette *palette = CLAY_PALETTE (savable);
  GList       *iter;
  const Babl  *format;

  clay_savable_print_element_start (state, "palette",
                                    "name", "%s", clay_object_get_name (palette),
                                    NULL);

  format = clay_palette_get_restriction (palette);
  if (format != NULL)
    {
      clay_savable_print_element_start (state, "restriction", NULL);
      clay_savable_format_save (format, state);
      clay_savable_print_element_end (state, "restriction");
    }

  for (iter = palette->colors; iter; iter = iter->next)
    {
      ClayPaletteEntry *entry = iter->data;
      clay_savable_color_save (entry->color, entry->name, format, state);
    }

  clay_savable_print_element_end (state, "palette");
}

static void
clay_palette_image_space_updated (ClayImage   *image,
                                  ClayPalette *palette)
{
  const Babl *space;
  const Babl *format;

  space  = clay_image_get_layer_space (image);
  format = clay_babl_format (CLAY_RGB, clay_image_get_precision (image), FALSE, space);
  clay_palette_restrict_format (palette, format, FALSE);
}

static void
clay_palette_notify_image (ClayPalette      *palette,
                           const GParamSpec *unused,
                           gpointer          unused_user_data)
{
  ClayImage *image = clay_data_get_image (CLAY_DATA (palette));

  if (palette->image == image)
    return;

  if (palette->image)
    g_signal_handlers_disconnect_by_func (palette->image,
                                          G_CALLBACK (clay_palette_image_space_updated),
                                          palette);

  g_set_weak_pointer (&palette->image, image);

  if (image)
    {
      /* Note: I only connect to "precision-changed", and not
       * "profile-changed" changes which is handled in
       * clay_image_convert_profile_colormap() with additional bpc
       * argument.
       * In current implementation of indexed images,
       * "precision-changed" should not happen because it is always
       * 8-bit non-linear.
       */
      g_signal_connect_object (image,
                               "precision-changed",
                               G_CALLBACK (clay_palette_image_space_updated),
                               palette, 0);

      clay_palette_image_space_updated (image, palette);
    }
  else
    {
      clay_palette_restrict_format (palette, NULL, FALSE);
    }
}


/*  public functions  */

void
clay_palette_restrict_format (ClayPalette *palette,
                              const Babl  *format,
                              gboolean     push_undo_if_image)
{
  gint n_colors;

  g_return_if_fail (CLAY_IS_PALETTE (palette));

  if (palette->format == format)
    return;

  if (push_undo_if_image && clay_data_get_image (CLAY_DATA (palette)))
    clay_image_undo_push_image_colormap (clay_data_get_image (CLAY_DATA (palette)),
                                         C_("undo-type", "Change Colormap format restriction"));
  palette->format = format;

  if (palette->format == NULL)
    /* No more restriction. */
    return;

  /* Convert all colors to the new format. */
  n_colors = clay_palette_get_n_colors (palette);
  for (gint i = 0; i < n_colors; i++)
    {
      ClayPaletteEntry *entry;
      guint8            pixel[40];

      entry = clay_palette_get_entry (palette, i);
      gegl_color_get_pixel (entry->color, format, pixel);
      gegl_color_set_pixel (entry->color, format, pixel);
    }
}

const Babl *
clay_palette_get_restriction (ClayPalette *palette)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);

  return palette->format;
}

GList *
clay_palette_get_colors (ClayPalette *palette)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);

  return palette->colors;
}

gint
clay_palette_get_n_colors (ClayPalette *palette)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), 0);

  return palette->n_colors;
}

void
clay_palette_move_entry (ClayPalette      *palette,
                         ClayPaletteEntry *entry,
                         gint              position)
{
  g_return_if_fail (CLAY_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      gint old_position = g_list_index (palette->colors, entry);

      palette->colors = g_list_remove (palette->colors,
                                       entry);
      palette->colors = g_list_insert (palette->colors,
                                       entry, position);

      if (! clay_data_is_frozen (CLAY_DATA (palette)))
        for (gint i = MIN (position, old_position); i <= MAX (position, old_position); i++)
          g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

      clay_data_dirty (CLAY_DATA (palette));
    }
}

ClayPaletteEntry *
clay_palette_add_entry (ClayPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        GeglColor     *color)
{
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  entry = g_slice_new0 (ClayPaletteEntry);

  entry->color = gegl_color_duplicate (color);
  entry->name  = g_strdup (name ? name : _("Untitled"));

  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (position < 0 || position >= palette->n_colors)
    {
      palette->colors = g_list_append (palette->colors, entry);
    }
  else
    {
      palette->colors = g_list_insert (palette->colors, entry, position);
    }

  palette->n_colors += 1;

  if (! clay_data_is_frozen (CLAY_DATA (palette)))
    for (gint i = position; i < palette->n_colors; i++)
      g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

  clay_data_dirty (CLAY_DATA (palette));

  return entry;
}

void
clay_palette_delete_entry (ClayPalette      *palette,
                           ClayPaletteEntry *entry)
{
  g_return_if_fail (CLAY_IS_PALETTE (palette));
  g_return_if_fail (entry != NULL);

  if (g_list_find (palette->colors, entry))
    {
      gint old_position = g_list_index (palette->colors, entry);

      clay_palette_entry_free (entry);

      palette->colors = g_list_remove (palette->colors, entry);

      palette->n_colors--;

      if (! clay_data_is_frozen (CLAY_DATA (palette)))
        for (gint i = old_position; i < palette->n_colors; i++)
          g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);

      clay_data_dirty (CLAY_DATA (palette));
    }
}

gboolean
clay_palette_set_entry (ClayPalette   *palette,
                        gint           position,
                        const gchar   *name,
                        GeglColor     *color)
{
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color), FALSE);

  entry = clay_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  g_clear_object (&entry->color);
  entry->color = gegl_color_duplicate (color);
  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  if (! clay_data_is_frozen (CLAY_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

  clay_data_dirty (CLAY_DATA (palette));

  return TRUE;
}

gboolean
clay_palette_set_entry_color (ClayPalette   *palette,
                              gint           position,
                              GeglColor     *color,
                              gboolean       push_undo_if_image)
{
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR (color), FALSE);

  entry = clay_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  if (push_undo_if_image && clay_data_get_image (CLAY_DATA (palette)))
    clay_image_undo_push_image_colormap (clay_data_get_image (CLAY_DATA (palette)),
                                         C_("undo-type", "Change Colormap entry"));

  g_clear_object (&entry->color);
  entry->color = gegl_color_duplicate (color);
  if (palette->format != NULL)
    {
      guint8 pixel[40];

      gegl_color_get_pixel (entry->color, palette->format, pixel);
      gegl_color_set_pixel (entry->color, palette->format, pixel);
    }

  if (! clay_data_is_frozen (CLAY_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

  clay_data_dirty (CLAY_DATA (palette));

  return TRUE;
}

gboolean
clay_palette_set_entry_name (ClayPalette *palette,
                             gint         position,
                             const gchar *name)
{
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), FALSE);

  entry = clay_palette_get_entry (palette, position);

  if (! entry)
    return FALSE;

  if (entry->name)
    g_free (entry->name);

  entry->name = g_strdup (name);

  if (! clay_data_is_frozen (CLAY_DATA (palette)))
    g_signal_emit (palette, signals[ENTRY_CHANGED], 0, position);

  clay_data_dirty (CLAY_DATA (palette));

  return TRUE;
}

ClayPaletteEntry *
clay_palette_get_entry (ClayPalette *palette,
                        gint         position)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);

  return g_list_nth_data (palette->colors, position);
}

gint
clay_palette_get_entry_position (ClayPalette      *palette,
                                 ClayPaletteEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), -1);
  g_return_val_if_fail (entry != NULL, -1);

  return g_list_index (palette->colors, entry);
}

void
clay_palette_set_columns (ClayPalette *palette,
                          gint         columns)
{
  g_return_if_fail (CLAY_IS_PALETTE (palette));

  columns = CLAMP (columns, 0, 64);

  if (palette->n_columns != columns)
    {
      palette->n_columns = columns;

      clay_data_dirty (CLAY_DATA (palette));
    }
}

gint
clay_palette_get_columns (ClayPalette *palette)
{
  g_return_val_if_fail (CLAY_IS_PALETTE (palette), 0);

  return palette->n_columns;
}

ClayPaletteEntry *
clay_palette_find_entry (ClayPalette      *palette,
                         GeglColor        *color,
                         ClayPaletteEntry *start_from)
{
  ClayPaletteEntry *entry;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  if (! start_from)
    {
      GList *list;

      /* search from the start */

      for (list = palette->colors; list; list = g_list_next (list))
        {
          entry = (ClayPaletteEntry *) list->data;
          if (clay_color_is_perceptually_identical (entry->color, color))
            return entry;
        }
    }
  else if (clay_color_is_perceptually_identical (start_from->color, color))
    {
      return start_from;
    }
  else
    {
      GList *old = g_list_find (palette->colors, start_from);
      GList *next;
      GList *prev;

      g_return_val_if_fail (old != NULL, NULL);

      next = old->next;
      prev = old->prev;

      /* proximity-based search */

      while (next || prev)
        {
          if (next)
            {
              entry = (ClayPaletteEntry *) next->data;
              if (clay_color_is_perceptually_identical (entry->color, color))
                return entry;

              next = next->next;
            }

          if (prev)
            {
              entry = (ClayPaletteEntry *) prev->data;
              if (clay_color_is_perceptually_identical (entry->color, color))
                return entry;

              prev = prev->prev;
            }
        }
    }

  return NULL;
}

guchar *
clay_palette_get_colormap (ClayPalette *palette,
                           const Babl  *format,
                           gint        *n_colors)
{
  guchar *colormap = NULL;
  gint    bpp;

  g_return_val_if_fail (CLAY_IS_PALETTE (palette), NULL);
  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (n_colors != NULL, NULL);

  bpp = babl_format_get_bytes_per_pixel (format);

  *n_colors = clay_palette_get_n_colors (palette);

  if (*n_colors > 0)
    {
      guchar *p;

      colormap = g_new0 (guchar, bpp * *n_colors);
      p        = colormap;

      for (gint i = 0; i < *n_colors; i++)
        {
          ClayPaletteEntry *entry = clay_palette_get_entry (palette, i);

          gegl_color_get_pixel (entry->color, format, p);
          p += bpp;
        }
    }

  return colormap;
}

void
clay_palette_set_colormap (ClayPalette  *palette,
                           const Babl   *format,
                           const guchar *colormap,
                           gint          n_colors,
                           gboolean      push_undo_if_image)
{
  ClayPaletteEntry *entry;
  GeglColor        *color;
  gchar             name[64];
  gint              bpp;
  gint              i;

  g_return_if_fail (CLAY_IS_PALETTE (palette));
  g_return_if_fail (format != NULL);
  g_return_if_fail (n_colors > 0);

  if (push_undo_if_image && clay_data_get_image (CLAY_DATA (palette)))
    clay_image_undo_push_image_colormap (clay_data_get_image (CLAY_DATA (palette)),
                                         C_("undo-type", "Set Colormap"));

  if (clay_data_get_image (CLAY_DATA (palette)))
    n_colors = MIN (n_colors, 256);

  clay_data_freeze (CLAY_DATA (palette));

  while ((entry = clay_palette_get_entry (palette, 0)))
    clay_palette_delete_entry (palette, entry);

  bpp = babl_format_get_bytes_per_pixel (format);

  color = gegl_color_new (NULL);
  for (i = 0; i < n_colors; i++)
    {
      gegl_color_set_pixel (color, format, &colormap[i * bpp]);
      g_snprintf (name, sizeof (name), "#%d", i);
      clay_palette_add_entry (palette, i, name, color);
    }
  g_object_unref (color);

  clay_data_thaw (CLAY_DATA (palette));

  if (! clay_data_is_frozen (CLAY_DATA (palette)))
    for (gint i = 0; i < n_colors; i++)
      g_signal_emit (palette, signals[ENTRY_CHANGED], 0, i);
}


/*  private functions  */

static void
clay_palette_entry_free (ClayPaletteEntry *entry)
{
  g_return_if_fail (entry != NULL);

  g_free (entry->name);
  g_clear_object (&entry->color);

  g_slice_free (ClayPaletteEntry, entry);
}

static gint64
clay_palette_entry_get_memsize (ClayPaletteEntry *entry,
                                gint64           *gui_size)
{
  gint64 memsize = sizeof (ClayPaletteEntry);

  memsize += clay_string_get_memsize (entry->name);
  /* the GeglColor is missing here */

  return memsize;
}
