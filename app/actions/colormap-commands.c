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

#include "core/claychannel-select.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-colormap.h"
#include "core/claypalette.h"

#include "widgets/claycolormapeditor.h"
#include "widgets/claycolormapselection.h"

#include "actions.h"
#include "colormap-commands.h"


/*  public functions  */

void
colormap_edit_color_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (data);

  clay_colormap_editor_edit_color (editor);
}

void
colormap_delete_color_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (data);

  clay_colormap_editor_delete_color (editor);
}

void
colormap_add_color_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayContext *context;
  ClayImage   *image;
  gboolean     background;
  return_if_no_context (context, data);
  return_if_no_image (image, data);

  background = (gboolean) g_variant_get_int32 (value);

  if (clay_palette_get_n_colors (clay_image_get_colormap_palette ((image))) < 256)
    {
      GeglColor *color;

      if (background)
        color = clay_context_get_background (context);
      else
        color = clay_context_get_foreground (context);

      clay_image_add_colormap_entry (image, color);
      clay_image_flush (image);
    }
}

void
colormap_to_selection_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayColormapSelection *selection;
  ClayColormapEditor    *editor;
  ClayImage             *image;
  GList                 *drawables;
  ClayChannelOps         op;
  gint                   col_index;

  return_if_no_image (image, data);

  editor    = CLAY_COLORMAP_EDITOR (data);
  selection = CLAY_COLORMAP_SELECTION (editor->selection);
  col_index = clay_colormap_selection_get_index (selection, NULL);

  op = (ClayChannelOps) g_variant_get_int32 (value);

  drawables = clay_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      /* We should not reach this anyway as colormap-actions.c normally takes
       * care at making the action insensitive when the item selection is wrong.
       */
      g_warning ("This action requires exactly one selected drawable.");
      g_list_free (drawables);
      return;
    }

  clay_channel_select_by_index (clay_image_get_mask (image),
                                drawables->data,
                                col_index, op,
                                FALSE, 0.0, 0.0);

  g_list_free (drawables);
  clay_image_flush (image);
}
