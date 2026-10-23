/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayanchor.c
 * Copyright (C) 2002 Simon Budig  <simon@gimp.org>
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

#include <glib-object.h>

#include "path-types.h"

#include "clayanchor.h"


G_DEFINE_BOXED_TYPE (ClayAnchor, clay_anchor, clay_anchor_copy, clay_anchor_free)

ClayAnchor *
clay_anchor_new (ClayAnchorType    type,
                 const ClayCoords *position)
{
  ClayAnchor *anchor = g_slice_new0 (ClayAnchor);

  anchor->type = type;

  if (position)
    anchor->position = *position;

  return anchor;
}

ClayAnchor *
clay_anchor_copy (const ClayAnchor *anchor)
{
  g_return_val_if_fail (anchor != NULL, NULL);

  return g_slice_dup (ClayAnchor, anchor);
}

void
clay_anchor_free (ClayAnchor *anchor)
{
  g_return_if_fail (anchor != NULL);

  g_slice_free (ClayAnchor, anchor);
}
