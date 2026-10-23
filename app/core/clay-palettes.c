/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * clay-gradients.c
 * Copyright (C) 2014 Michael Natterer  <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay.h"
#include "clay-palettes.h"
#include "claycontext.h"
#include "claycontainer.h"
#include "claydatafactory.h"
#include "claypalettemru.h"

#include "clay-intl.h"


#define COLOR_HISTORY_KEY "clay-palette-color-history"


/*  local function prototypes  */

static ClayPalette * clay_palettes_add_palette (Clay        *clay,
                                                const gchar *name,
                                                const gchar *id);


/*  public functions  */

void
clay_palettes_init (Clay *clay)
{
  ClayPalette *palette;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  palette = clay_palettes_add_palette (clay,
                                       _("Color History"),
                                       COLOR_HISTORY_KEY);
  clay_context_set_palette (clay->user_context, palette);
}

void
clay_palettes_load (Clay *clay)
{
  ClayPalette *palette;
  GFile       *file;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  palette = clay_palettes_get_color_history (clay);

  file = clay_directory_file ("colorrc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  clay_palette_mru_load (CLAY_PALETTE_MRU (palette), file);

  g_object_unref (file);
}

void
clay_palettes_save (Clay *clay)
{
  ClayPalette *palette;
  GFile       *file;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  palette = clay_palettes_get_color_history (clay);

  file = clay_directory_file ("colorrc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  clay_palette_mru_save (CLAY_PALETTE_MRU (palette), file);

  g_object_unref (file);
}

ClayPalette *
clay_palettes_get_color_history (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_get_data (G_OBJECT (clay), COLOR_HISTORY_KEY);
}

void
clay_palettes_add_color_history (Clay      *clay,
                                 GeglColor *color)
{
  ClayPalette *history;

  history = clay_palettes_get_color_history (clay);
  clay_palette_mru_add (CLAY_PALETTE_MRU (history), color);
}

/*  private functions  */

static ClayPalette *
clay_palettes_add_palette (Clay        *clay,
                           const gchar *name,
                           const gchar *id)
{
  ClayData *palette;

  palette = clay_palette_mru_new (name);

  clay_data_make_internal (palette, id);

  clay_container_add (clay_data_factory_get_container (clay->palette_factory),
                      CLAY_OBJECT (palette));
  g_object_unref (palette);

  g_object_set_data (G_OBJECT (clay), id, palette);

  return CLAY_PALETTE (palette);
}
