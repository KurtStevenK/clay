/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayanchor.h
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

#pragma once


#define CLAY_ANCHOR(anchor)  ((ClayAnchor *) (anchor))

#define CLAY_TYPE_ANCHOR               (clay_anchor_get_type ())
#define CLAY_VALUE_HOLDS_ANCHOR(value) (G_TYPE_CHECK_VALUE_TYPE ((value), CLAY_TYPE_ANCHOR))

GType   clay_anchor_get_type           (void);


struct _ClayAnchor
{
  ClayCoords        position;

  ClayAnchorType    type;   /* Interpretation dependent on ClayStroke type */
  gboolean          selected;
};


ClayAnchor  * clay_anchor_new  (ClayAnchorType    type,
                                const ClayCoords *position);

ClayAnchor  * clay_anchor_copy (const ClayAnchor *anchor);
void          clay_anchor_free (ClayAnchor       *anchor);
