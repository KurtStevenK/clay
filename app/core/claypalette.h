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

#pragma once

#include "claydata.h"


#define CLAY_TYPE_PALETTE            (clay_palette_get_type ())
#define CLAY_PALETTE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PALETTE, ClayPalette))
#define CLAY_PALETTE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PALETTE, ClayPaletteClass))
#define CLAY_IS_PALETTE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PALETTE))
#define CLAY_IS_PALETTE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PALETTE))
#define CLAY_PALETTE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PALETTE, ClayPaletteClass))


struct _ClayPaletteEntry
{
  GeglColor *color;
  gchar     *name;
};


typedef struct _ClayPaletteClass ClayPaletteClass;

struct _ClayPalette
{
  ClayData  parent_instance;

  /* When set to an image, we store the image we connect to, so that we
   * can correctly disconnect if parent ClayData's image changes (so the
   * ClayPalette and ClayData's images may be unsynced for a tiny span of
   * time).
   */
  ClayImage  *image;

  /* Palette colors can be restricted to a given format. If NULL, then the
   * palette can be a mix of color models and color spaces.
   */
  const Babl *format;

  GList      *colors;
  gint        n_colors;

  gint        n_columns;
};

struct _ClayPaletteClass
{
  ClayDataClass  parent_class;

  void (* entry_changed) (ClayPalette *palette,
                          gint         index);
};


GType              clay_palette_get_type        (void);

ClayData         * clay_palette_new             (ClayContext      *context,
                                                 const gchar      *name);
ClayData         * clay_palette_get_standard    (ClayContext      *context);

void               clay_palette_restrict_format (ClayPalette      *palette,
                                                 const Babl       *format,
                                                 gboolean          push_undo_if_image);
const Babl       * clay_palette_get_restriction (ClayPalette      *palette);

GList            * clay_palette_get_colors      (ClayPalette      *palette);
gint               clay_palette_get_n_colors    (ClayPalette      *palette);

void               clay_palette_move_entry      (ClayPalette      *palette,
                                                 ClayPaletteEntry *entry,
                                                 gint              position);

ClayPaletteEntry * clay_palette_add_entry       (ClayPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 GeglColor        *color);
void               clay_palette_delete_entry    (ClayPalette      *palette,
                                                 ClayPaletteEntry *entry);

gboolean           clay_palette_set_entry       (ClayPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name,
                                                 GeglColor        *color);
gboolean           clay_palette_set_entry_color (ClayPalette      *palette,
                                                 gint              position,
                                                 GeglColor        *color,
                                                 gboolean          push_undo_if_image);
gboolean           clay_palette_set_entry_name  (ClayPalette      *palette,
                                                 gint              position,
                                                 const gchar      *name);
ClayPaletteEntry * clay_palette_get_entry       (ClayPalette      *palette,
                                                 gint              position);
gint               clay_palette_get_entry_position (ClayPalette   *palette,
                                                 ClayPaletteEntry *entry);

void               clay_palette_set_columns     (ClayPalette      *palette,
                                                 gint              columns);
gint               clay_palette_get_columns     (ClayPalette      *palette);

ClayPaletteEntry * clay_palette_find_entry      (ClayPalette      *palette,
                                                 GeglColor        *color,
                                                 ClayPaletteEntry *start_from);

guchar           * clay_palette_get_colormap    (ClayPalette      *palette,
                                                 const Babl       *format,
                                                 gint             *n_colors);
void               clay_palette_set_colormap    (ClayPalette      *palette,
                                                 const Babl       *format,
                                                 const guchar     *colormap,
                                                 gint              n_colors,
                                                 gboolean          push_undo_if_image);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPalette, g_object_unref);
