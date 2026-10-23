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

#include <gegl.h>
#include <gtk/gtk.h>

#include "actions-types.h"

#include "core/claycontext.h"
#include "core/claypalette.h"

#include "widgets/claypaletteeditor.h"
#include "widgets/claypaletteview.h"

#include "palette-editor-commands.h"


/*  public functions  */

void
palette_editor_edit_color_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayPaletteEditor *editor = CLAY_PALETTE_EDITOR (data);

  clay_palette_editor_edit_color (editor);
}

void
palette_editor_new_color_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayPaletteEditor *editor      = CLAY_PALETTE_EDITOR (data);
  ClayDataEditor    *data_editor = CLAY_DATA_EDITOR (data);
  gboolean           background  = (gboolean) g_variant_get_int32 (value);

  if (data_editor->data_editable)
    {
      ClayPalette      *palette = CLAY_PALETTE (data_editor->data);
      ClayPaletteEntry *entry;
      GeglColor        *color;

      if (background)
        color = clay_context_get_background (data_editor->context);
      else
        color = clay_context_get_foreground (data_editor->context);

      entry = clay_palette_add_entry (palette, -1, NULL, color);
      clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view), entry);
    }
}

void
palette_editor_delete_color_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayPaletteEditor *editor      = CLAY_PALETTE_EDITOR (data);
  ClayDataEditor    *data_editor = CLAY_DATA_EDITOR (data);

  if (data_editor->data_editable && editor->color)
    {
      ClayPalette *palette = CLAY_PALETTE (data_editor->data);
      gint         index;

      index = clay_palette_get_entry_position (palette, editor->color);

      clay_palette_delete_entry (palette, editor->color);

      if (index >= clay_palette_get_n_colors (palette))
        index = clay_palette_get_n_colors (palette) - 1;

      clay_palette_view_select_entry (CLAY_PALETTE_VIEW (editor->view),
                                      clay_palette_get_entry (palette, index));
    }
}

void
palette_editor_zoom_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayPaletteEditor *editor    = CLAY_PALETTE_EDITOR (data);
  ClayZoomType       zoom_type = (ClayZoomType) g_variant_get_int32 (value);

  clay_palette_editor_zoom (editor, zoom_type);
}
