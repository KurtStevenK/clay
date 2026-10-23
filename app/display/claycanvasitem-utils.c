/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycanvasitem-utils.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include "libclaymath/claymath.h"

#include "display-types.h"

#include "core/clayimage.h"

#include "path/clayanchor.h"
#include "path/claybezierstroke.h"
#include "path/claypath.h"

#include "claycanvasitem.h"
#include "claycanvasitem-utils.h"
#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-transform.h"


gboolean
clay_canvas_item_on_handle (ClayCanvasItem  *item,
                            gdouble           x,
                            gdouble           y,
                            ClayHandleType    type,
                            gdouble           handle_x,
                            gdouble           handle_y,
                            gint              width,
                            gint              height,
                            ClayHandleAnchor  anchor)
{
  ClayDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (CLAY_IS_CANVAS_ITEM (item), FALSE);

  shell = clay_canvas_item_get_shell (item);

  clay_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  clay_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case CLAY_HANDLE_SQUARE:
    case CLAY_HANDLE_FILLED_SQUARE:
    case CLAY_HANDLE_CROSS:
    case CLAY_HANDLE_CROSSHAIR:
      clay_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case CLAY_HANDLE_CIRCLE:
    case CLAY_HANDLE_FILLED_CIRCLE:
      clay_canvas_item_shift_to_center (anchor,
                                        handle_tx, handle_ty,
                                        width, height,
                                        &handle_tx, &handle_ty);

      /* FIXME */
      if (width != height)
        width = (width + height) / 2;

      width /= 2;

      return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));

    default:
      g_warning ("%s: invalid handle type %d", G_STRFUNC, type);
      break;
    }

  return FALSE;
}

gboolean
clay_canvas_item_on_path_handle (ClayCanvasItem    *item,
                                 ClayPath          *path,
                                 const ClayCoords  *coord,
                                 gint               width,
                                 gint               height,
                                 ClayAnchorType     preferred,
                                 gboolean           exclusive,
                                 ClayAnchor       **ret_anchor,
                                 ClayStroke       **ret_stroke)
{
  ClayStroke *stroke       = NULL;
  ClayStroke *pref_stroke  = NULL;
  ClayAnchor *anchor       = NULL;
  ClayAnchor *pref_anchor  = NULL;
  gdouble     dx, dy;
  gdouble     pref_mindist = -1;
  gdouble     mindist      = -1;

  g_return_val_if_fail (CLAY_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (CLAY_IS_PATH (path), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_anchor) *ret_anchor = NULL;
  if (ret_stroke) *ret_stroke = NULL;

  while ((stroke = clay_path_stroke_get_next (path, stroke)))
    {
      GList *anchor_list;
      GList *list;

      anchor_list = g_list_concat (clay_stroke_get_draw_anchors (stroke),
                                   clay_stroke_get_draw_controls (stroke));

      for (list = anchor_list; list; list = g_list_next (list))
        {
          dx = coord->x - CLAY_ANCHOR (list->data)->position.x;
          dy = coord->y - CLAY_ANCHOR (list->data)->position.y;

          if (mindist < 0 || mindist > dx * dx + dy * dy)
            {
              mindist = dx * dx + dy * dy;
              anchor = CLAY_ANCHOR (list->data);

              if (ret_stroke)
                *ret_stroke = stroke;
            }

          if ((pref_mindist < 0 || pref_mindist > dx * dx + dy * dy) &&
              CLAY_ANCHOR (list->data)->type == preferred)
            {
              pref_mindist = dx * dx + dy * dy;
              pref_anchor = CLAY_ANCHOR (list->data);
              pref_stroke = stroke;
            }
        }

      g_list_free (anchor_list);
    }

  /* If the data passed into ret_anchor is a preferred anchor, return it. */
  if (ret_anchor && *ret_anchor &&
      clay_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  CLAY_HANDLE_CIRCLE,
                                  (*ret_anchor)->position.x,
                                  (*ret_anchor)->position.y,
                                  width, height,
                                  CLAY_HANDLE_ANCHOR_CENTER) &&
      (*ret_anchor)->type == preferred)
    {
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }

  if (pref_anchor && clay_canvas_item_on_handle (item,
                                                 coord->x,
                                                 coord->y,
                                                 CLAY_HANDLE_CIRCLE,
                                                 pref_anchor->position.x,
                                                 pref_anchor->position.y,
                                                 width, height,
                                                 CLAY_HANDLE_ANCHOR_CENTER))
    {
      if (ret_anchor) *ret_anchor = pref_anchor;
      if (ret_stroke) *ret_stroke = pref_stroke;

      return TRUE;
    }
  else if (!exclusive && anchor &&
           clay_canvas_item_on_handle (item,
                                       coord->x,
                                       coord->y,
                                       CLAY_HANDLE_CIRCLE,
                                       anchor->position.x,
                                       anchor->position.y,
                                       width, height,
                                       CLAY_HANDLE_ANCHOR_CENTER))
    {
      if (ret_anchor)
        *ret_anchor = anchor;

      /* *ret_stroke already set correctly. */
      return TRUE;
    }

  if (ret_anchor)
    *ret_anchor = NULL;
  if (ret_stroke)
    *ret_stroke = NULL;

  return FALSE;
}

gboolean
clay_canvas_item_on_path_curve (ClayCanvasItem    *item,
                                ClayPath          *path,
                                const ClayCoords  *coord,
                                gint               width,
                                gint               height,
                                ClayCoords        *ret_coords,
                                gdouble           *ret_pos,
                                ClayAnchor       **ret_segment_start,
                                ClayAnchor       **ret_segment_end,
                                ClayStroke       **ret_stroke)
{
  ClayStroke *stroke = NULL;
  ClayAnchor *segment_start;
  ClayAnchor *segment_end;
  ClayCoords  min_coords = CLAY_COORDS_DEFAULT_VALUES;
  ClayCoords  cur_coords;
  gdouble     min_dist, cur_dist, cur_pos;

  g_return_val_if_fail (CLAY_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (CLAY_IS_PATH (path), FALSE);
  g_return_val_if_fail (coord != NULL, FALSE);

  if (ret_coords)        *ret_coords        = *coord;
  if (ret_pos)           *ret_pos           = -1.0;
  if (ret_segment_start) *ret_segment_start = NULL;
  if (ret_segment_end)   *ret_segment_end   = NULL;
  if (ret_stroke)        *ret_stroke        = NULL;

  min_dist = -1.0;

  while ((stroke = clay_path_stroke_get_next (path, stroke)))
    {
      cur_dist = clay_stroke_nearest_point_get (stroke, coord, 1.0,
                                                &cur_coords,
                                                &segment_start,
                                                &segment_end,
                                                &cur_pos);

      if (cur_dist >= 0 && (min_dist < 0 || cur_dist < min_dist))
        {
          min_dist   = cur_dist;
          min_coords = cur_coords;

          if (ret_coords)        *ret_coords        = cur_coords;
          if (ret_pos)           *ret_pos           = cur_pos;
          if (ret_segment_start) *ret_segment_start = segment_start;
          if (ret_segment_end)   *ret_segment_end   = segment_end;
          if (ret_stroke)        *ret_stroke        = stroke;
        }
    }

  if (min_dist >= 0 &&
      clay_canvas_item_on_handle (item,
                                  coord->x,
                                  coord->y,
                                  CLAY_HANDLE_CIRCLE,
                                  min_coords.x,
                                  min_coords.y,
                                  width, height,
                                  CLAY_HANDLE_ANCHOR_CENTER))
    {
      return TRUE;
    }

  return FALSE;
}

gboolean
clay_canvas_item_on_path (ClayCanvasItem    *item,
                          const ClayCoords  *coords,
                          gint               width,
                          gint               height,
                          ClayCoords        *ret_coords,
                          gdouble           *ret_pos,
                          ClayAnchor       **ret_segment_start,
                          ClayAnchor       **ret_segment_end,
                          ClayStroke       **ret_stroke,
                          ClayPath         **ret_path)
{
  ClayDisplayShell *shell;
  ClayImage        *image;
  GList            *all_path;
  GList            *list;

  g_return_val_if_fail (CLAY_IS_CANVAS_ITEM (item), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  shell = clay_canvas_item_get_shell (item);
  image = clay_display_get_image (shell->display);

  if (ret_coords)        *ret_coords         = *coords;
  if (ret_pos)           *ret_pos            = -1.0;
  if (ret_segment_start) *ret_segment_start  = NULL;
  if (ret_segment_end)   *ret_segment_end    = NULL;
  if (ret_stroke)        *ret_stroke         = NULL;
  if (ret_path)          *ret_path           = NULL;

  all_path = clay_image_get_path_list (image);

  for (list = all_path; list; list = g_list_next (list))
    {
      ClayPath *path = list->data;

      if (! clay_item_get_visible (CLAY_ITEM (path)))
        continue;

      if (clay_canvas_item_on_path_curve (item,
                                          path, coords,
                                          width, height,
                                          ret_coords,
                                          ret_pos,
                                          ret_segment_start,
                                          ret_segment_end,
                                          ret_stroke))
        {
          if (ret_path)
            *ret_path = path;

          g_list_free (all_path);

          return TRUE;
        }
    }

  g_list_free (all_path);

  return FALSE;
}

void
clay_canvas_item_shift_to_north_west (ClayHandleAnchor  anchor,
                                      gdouble           x,
                                      gdouble           y,
                                      gint              width,
                                      gint              height,
                                      gdouble          *shifted_x,
                                      gdouble          *shifted_y)
{
  switch (anchor)
    {
    case CLAY_HANDLE_ANCHOR_CENTER:
      x -= width  / 2;
      y -= height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH:
      x -= width / 2;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_WEST:
      /*  nothing, this is the default  */
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_EAST:
      x -= width;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH:
      x -= width / 2;
      y -= height;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_WEST:
      y -= height;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width;
      y -= height;
      break;

    case CLAY_HANDLE_ANCHOR_WEST:
      y -= height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_EAST:
      x -= width;
      y -= height / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}

void
clay_canvas_item_shift_to_center (ClayHandleAnchor  anchor,
                                  gdouble           x,
                                  gdouble           y,
                                  gint              width,
                                  gint              height,
                                  gdouble          *shifted_x,
                                  gdouble          *shifted_y)
{
  switch (anchor)
    {
    case CLAY_HANDLE_ANCHOR_CENTER:
      /*  nothing, this is the default  */
      break;

    case CLAY_HANDLE_ANCHOR_NORTH:
      y += height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_WEST:
      x += width  / 2;
      y += height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_NORTH_EAST:
      x -= width  / 2;
      y += height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH:
      y -= height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_WEST:
      x += width  / 2;
      y -= height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_SOUTH_EAST:
      x -= width  / 2;
      y -= height / 2;
      break;

    case CLAY_HANDLE_ANCHOR_WEST:
      x += width / 2;
      break;

    case CLAY_HANDLE_ANCHOR_EAST:
      x -= width / 2;
      break;

    default:
      break;
    }

  if (shifted_x)
    *shifted_x = x;

  if (shifted_y)
    *shifted_y = y;
}
