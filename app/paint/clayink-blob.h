/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayink-blob.h: routines for manipulating scan converted convex polygons.
 * Copyright 1998, Owen Taylor <otaylor@gtk.org>
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
 *
*/

#pragma once


typedef struct _ClayBlobPoint ClayBlobPoint;
typedef struct _ClayBlobSpan  ClayBlobSpan;
typedef struct _ClayBlob      ClayBlob;

typedef ClayBlob * (* ClayBlobFunc) (gdouble xc,
                                     gdouble yc,
                                     gdouble xp,
                                     gdouble yp,
                                     gdouble xq,
                                     gdouble yq);

struct _ClayBlobPoint
{
  gint x;
  gint y;
};

struct _ClayBlobSpan
{
  gint left;
  gint right;
};

struct _ClayBlob
{
  gint         y;
  gint         height;
  ClayBlobSpan data[1];
};


ClayBlob * clay_blob_polygon      (ClayBlobPoint *points,
                                   gint           n_points);
ClayBlob * clay_blob_square       (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
ClayBlob * clay_blob_diamond      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
ClayBlob * clay_blob_ellipse      (gdouble        xc,
                                   gdouble        yc,
                                   gdouble        xp,
                                   gdouble        yp,
                                   gdouble        xq,
                                   gdouble        yq);
void       clay_blob_bounds       (ClayBlob      *b,
                                   gint          *x,
                                   gint          *y,
                                   gint          *width,
                                   gint          *height);
ClayBlob * clay_blob_convex_union (ClayBlob      *b1,
                                   ClayBlob      *b2);
ClayBlob * clay_blob_duplicate    (ClayBlob      *b);
void       clay_blob_move         (ClayBlob      *b,
                                   gint           x,
                                   gint           y);
