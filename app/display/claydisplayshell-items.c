/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydisplayshell-items.c
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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

#include <libclaymath/claymath.h>

#include "display-types.h"

#include "claycanvascanvasboundary.h"
#include "claycanvascursor.h"
#include "claycanvasgrid.h"
#include "claycanvaslayerboundary.h"
#include "claycanvaspassepartout.h"
#include "claycanvasproxygroup.h"
#include "claydisplayshell.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-items.h"
#include "claydisplayshell-transform.h"


/*  local function prototypes  */

static void   clay_display_shell_item_update           (ClayCanvasItem   *item,
                                                        cairo_region_t   *region,
                                                        ClayDisplayShell *shell);
static void   clay_display_shell_unrotated_item_update (ClayCanvasItem   *item,
                                                        cairo_region_t   *region,
                                                        ClayDisplayShell *shell);


/*  public functions  */

void
clay_display_shell_items_init (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  shell->canvas_item = clay_canvas_group_new (shell);

  shell->passe_partout = clay_canvas_passe_partout_new (shell, 0, 0, 0, 0);
  clay_canvas_item_set_visible (shell->passe_partout, FALSE);
  clay_display_shell_add_item (shell, shell->passe_partout);
  g_object_unref (shell->passe_partout);

  shell->preview_items = clay_canvas_group_new (shell);
  clay_display_shell_add_item (shell, shell->preview_items);
  g_object_unref (shell->preview_items);

  shell->paths = clay_canvas_proxy_group_new (shell);
  clay_display_shell_add_item (shell, shell->paths);
  g_object_unref (shell->paths);

  shell->grid = clay_canvas_grid_new (shell, NULL);
  clay_canvas_item_set_visible (shell->grid, FALSE);
  g_object_set (shell->grid, "grid-style", TRUE, NULL);
  clay_display_shell_add_item (shell, shell->grid);
  g_object_unref (shell->grid);

  shell->guides = clay_canvas_proxy_group_new (shell);
  clay_display_shell_add_item (shell, shell->guides);
  g_object_unref (shell->guides);

  shell->sample_points = clay_canvas_proxy_group_new (shell);
  clay_display_shell_add_item (shell, shell->sample_points);
  g_object_unref (shell->sample_points);

  shell->canvas_boundary = clay_canvas_canvas_boundary_new (shell);
  clay_canvas_item_set_visible (shell->canvas_boundary, FALSE);
  clay_display_shell_add_item (shell, shell->canvas_boundary);
  g_object_unref (shell->canvas_boundary);

  shell->layer_boundary = clay_canvas_layer_boundary_new (shell);
  clay_canvas_item_set_visible (shell->layer_boundary, FALSE);
  clay_display_shell_add_item (shell, shell->layer_boundary);
  g_object_unref (shell->layer_boundary);

  shell->tool_items = clay_canvas_group_new (shell);
  clay_display_shell_add_item (shell, shell->tool_items);
  g_object_unref (shell->tool_items);

  g_signal_connect (shell->canvas_item, "update",
                    G_CALLBACK (clay_display_shell_item_update),
                    shell);

  shell->unrotated_item = clay_canvas_group_new (shell);

  shell->cursor = clay_canvas_cursor_new (shell);
  clay_canvas_item_set_visible (shell->cursor, FALSE);
  clay_display_shell_add_unrotated_item (shell, shell->cursor);
  g_object_unref (shell->cursor);

  g_signal_connect (shell->unrotated_item, "update",
                    G_CALLBACK (clay_display_shell_unrotated_item_update),
                    shell);
}

void
clay_display_shell_items_free (ClayDisplayShell *shell)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  if (shell->canvas_item)
    {
      g_signal_handlers_disconnect_by_func (shell->canvas_item,
                                            clay_display_shell_item_update,
                                            shell);

      g_clear_object (&shell->canvas_item);

      shell->passe_partout   = NULL;
      shell->preview_items   = NULL;
      shell->paths           = NULL;
      shell->grid            = NULL;
      shell->guides          = NULL;
      shell->sample_points   = NULL;
      shell->canvas_boundary = NULL;
      shell->layer_boundary  = NULL;
      shell->tool_items      = NULL;
    }

  if (shell->unrotated_item)
    {
      g_signal_handlers_disconnect_by_func (shell->unrotated_item,
                                            clay_display_shell_unrotated_item_update,
                                            shell);

      g_clear_object (&shell->unrotated_item);

      shell->cursor = NULL;
    }
}

void
clay_display_shell_add_item (ClayDisplayShell *shell,
                             ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (shell->canvas_item), item);
}

void
clay_display_shell_remove_item (ClayDisplayShell *shell,
                                ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (shell->canvas_item), item);
}

void
clay_display_shell_add_preview_item (ClayDisplayShell *shell,
                                     ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (shell->preview_items), item);
}

void
clay_display_shell_remove_preview_item (ClayDisplayShell *shell,
                                        ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (shell->preview_items), item);
}

void
clay_display_shell_add_unrotated_item (ClayDisplayShell *shell,
                                       ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (shell->unrotated_item), item);
}

void
clay_display_shell_remove_unrotated_item (ClayDisplayShell *shell,
                                          ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (shell->unrotated_item), item);
}

void
clay_display_shell_add_tool_item (ClayDisplayShell *shell,
                                  ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (shell->tool_items), item);
}

void
clay_display_shell_remove_tool_item (ClayDisplayShell *shell,
                                     ClayCanvasItem   *item)
{
  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (shell->tool_items), item);
}


/*  private functions  */

static void
clay_display_shell_item_update (ClayCanvasItem   *item,
                                cairo_region_t   *region,
                                ClayDisplayShell *shell)
{
  if (shell->rotate_transform)
    {
      gint n_rects;
      gint i;

      n_rects = cairo_region_num_rectangles (region);

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;
          gdouble               tx1, ty1;
          gdouble               tx2, ty2;
          gint                  x1, y1, x2, y2;

          cairo_region_get_rectangle (region, i, &rect);

          clay_display_shell_rotate_bounds (shell,
                                            rect.x, rect.y,
                                            rect.x + rect.width,
                                            rect.y + rect.height,
                                            &tx1, &ty1, &tx2, &ty2);

          x1 = floor (tx1 - 0.5);
          y1 = floor (ty1 - 0.5);
          x2 = ceil (tx2 + 0.5);
          y2 = ceil (ty2 + 0.5);

          clay_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);
        }
    }
  else
    {
      clay_display_shell_expose_region (shell, region);
    }
}

static void
clay_display_shell_unrotated_item_update (ClayCanvasItem   *item,
                                          cairo_region_t   *region,
                                          ClayDisplayShell *shell)
{
  clay_display_shell_expose_region (shell, region);
}
