/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypalettemru.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claypalettemru.h"

#include "clay-intl.h"


#define MAX_N_COLORS 256
#define RGBA_EPSILON 1e-4

enum
{
  COLOR_HISTORY = 1,
  COLOR         = 2
};


G_DEFINE_TYPE (ClayPaletteMru, clay_palette_mru, CLAY_TYPE_PALETTE)

#define parent_class clay_palette_mru_parent_class


static void
clay_palette_mru_class_init (ClayPaletteMruClass *klass)
{
}

static void
clay_palette_mru_init (ClayPaletteMru *palette)
{
}


/*  public functions  */

ClayData *
clay_palette_mru_new (const gchar *name)
{
  ClayPaletteMru *palette;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (*name != '\0', NULL);

  palette = g_object_new (CLAY_TYPE_PALETTE_MRU,
                          "name",      name,
                          "mime-type", "application/x-clay-palette",
                          NULL);

  return CLAY_DATA (palette);
}

void
clay_palette_mru_load (ClayPaletteMru *mru,
                       GFile          *file)
{
  ClayPalette *palette;
  GScanner    *scanner;
  GTokenType   token;

  g_return_if_fail (CLAY_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  palette = CLAY_PALETTE (mru);

  scanner = clay_scanner_new_file (file, NULL);
  if (! scanner)
    return;

  g_scanner_scope_add_symbol (scanner, 0, "color-history",
                              GINT_TO_POINTER (COLOR_HISTORY));
  g_scanner_scope_add_symbol (scanner, 0, "color",
                              GINT_TO_POINTER (COLOR_HISTORY));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GINT_TO_POINTER (COLOR_HISTORY))
            {
              while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
                {
                  GeglColor *color = NULL;

                  if (! clay_scanner_parse_color (scanner, &color))
                    goto end;

                  clay_palette_add_entry (palette, -1, _("History Color"), color);
                  g_object_unref (color);

                  if (clay_palette_get_n_colors (palette) == MAX_N_COLORS)
                    goto end;
                }
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

 end:
  clay_scanner_unref (scanner);
}

void
clay_palette_mru_save (ClayPaletteMru *mru,
                       GFile          *file)
{
  ClayPalette      *palette;
  ClayConfigWriter *writer;
  GList            *list;

  g_return_if_fail (CLAY_IS_PALETTE_MRU (mru));
  g_return_if_fail (G_IS_FILE (file));

  writer = clay_config_writer_new_from_file (file,
                                             TRUE,
                                             "Clay colorrc\n\n"
                                             "This file holds a list of "
                                             "recently used colors.",
                                             NULL);
  if (! writer)
    return;

  palette = CLAY_PALETTE (mru);

  clay_config_writer_open (writer, "color-history");

  for (list = palette->colors; list; list = g_list_next (list))
    {
      ClayPaletteEntry *entry = list->data;
      GeglColor        *color = gegl_color_duplicate (entry->color);
      const gchar      *encoding;
      const Babl       *format = gegl_color_get_format (color);
      const Babl       *space;
      GBytes           *bytes;
      gconstpointer     data;
      gsize             data_length;
      guint8           *profile_data;
      int               profile_length = 0;

      clay_config_writer_open (writer, "color");

      if (babl_format_is_palette (format))
        {
          guint8 pixel[40];

          /* As a special case, we don't want to serialize
           * palette colors, because they are just too much
           * dependent on external data and cannot be
           * deserialized back safely. So we convert them first.
           */
           format = babl_format_with_space ("R'G'B'A u8", format);
           gegl_color_get_pixel (color, format, pixel);
           gegl_color_set_pixel (color, format, pixel);
        }

      encoding = babl_format_get_encoding (format);
      clay_config_writer_string (writer, encoding);

      bytes = gegl_color_get_bytes (color, format);
      data  = g_bytes_get_data (bytes, &data_length);

      clay_config_writer_printf (writer, "%" G_GSIZE_FORMAT, data_length);
      clay_config_writer_data (writer, data_length, data);

      space = babl_format_get_space (format);
      if (space != babl_space ("sRGB"))
        {
          profile_data = (guint8 *) babl_space_get_icc (space, &profile_length);
          clay_config_writer_printf (writer, "%u", profile_length);
          if (profile_data)
            clay_config_writer_data (writer, profile_length, profile_data);
        }
      else
        {
          clay_config_writer_printf (writer, "%u", profile_length);
        }

      clay_config_writer_close (writer);
      g_bytes_unref (bytes);
      g_object_unref (color);
    }

  clay_config_writer_close (writer);

  clay_config_writer_finish (writer, "end of colorrc", NULL);
}

void
clay_palette_mru_add (ClayPaletteMru *mru,
                      GeglColor      *color)
{
  ClayPalette *palette;
  GList       *list;

  g_return_if_fail (CLAY_IS_PALETTE_MRU (mru));
  g_return_if_fail (GEGL_IS_COLOR (color));

  palette = CLAY_PALETTE (mru);

  /*  is the added color already there?  */
  for (list = clay_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      ClayPaletteEntry *entry = list->data;

      if (clay_color_is_perceptually_identical (entry->color, color))
        {
          clay_palette_move_entry (palette, entry, 0);

          /*  Even though they are nearly the same color, let's make them
           *  exactly equal.
           */
          clay_palette_set_entry_color (palette, 0, color, FALSE);
          return;
        }
    }

  if (clay_palette_get_n_colors (palette) == MAX_N_COLORS)
    {
      clay_palette_delete_entry (palette,
                                 clay_palette_get_entry (palette,
                                                         MAX_N_COLORS - 1));
    }

  clay_palette_add_entry (palette, 0, _("History Color"), color);
}
