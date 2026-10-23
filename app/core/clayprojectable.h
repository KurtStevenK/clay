/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayprojectable.h
 * Copyright (C) 2008  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PROJECTABLE (clay_projectable_get_type ())
G_DECLARE_INTERFACE (ClayProjectable,
                     clay_projectable,
                     CLAY, PROJECTABLE,
                     GObject)


struct _ClayProjectableInterface
{
  GTypeInterface base_iface;

  /*  signals  */
  void         (* invalidate)         (ClayProjectable *projectable,
                                       gint             x,
                                       gint             y,
                                       gint             width,
                                       gint             height);
  void         (* flush)              (ClayProjectable *projectable,
                                       gboolean         invalidate_preview);
  void         (* structure_changed)  (ClayProjectable *projectable);
  void         (* bounds_changed)     (ClayProjectable *projectable,
                                       gint             old_x,
                                       gint             old_y);

  /*  virtual functions  */
  ClayImage  * (* get_image)          (ClayProjectable *projectable);
  const Babl * (* get_format)         (ClayProjectable *projectable);
  void         (* get_offset)         (ClayProjectable *projectable,
                                       gint            *x,
                                       gint            *y);
  GeglRectangle (* get_bounding_box)  (ClayProjectable *projectable);
  GeglNode   * (* get_graph)          (ClayProjectable *projectable);
  void         (* begin_render)       (ClayProjectable *projectable);
  void         (* end_render)         (ClayProjectable *projectable);
  void         (* invalidate_preview) (ClayProjectable *projectable);
};


void         clay_projectable_invalidate         (ClayProjectable *projectable,
                                                  gint             x,
                                                  gint             y,
                                                  gint             width,
                                                  gint             height);
void         clay_projectable_flush              (ClayProjectable *projectable,
                                                  gboolean         preview_invalidated);
void         clay_projectable_structure_changed  (ClayProjectable *projectable);
void         clay_projectable_bounds_changed     (ClayProjectable *projectable,
                                                  gint             old_x,
                                                  gint             old_y);

ClayImage  * clay_projectable_get_image          (ClayProjectable *projectable);
const Babl * clay_projectable_get_format         (ClayProjectable *projectable);
void         clay_projectable_get_offset         (ClayProjectable *projectable,
                                                  gint            *x,
                                                  gint            *y);
GeglRectangle clay_projectable_get_bounding_box  (ClayProjectable *projectable);
GeglNode   * clay_projectable_get_graph          (ClayProjectable *projectable);
void         clay_projectable_begin_render       (ClayProjectable *projectable);
void         clay_projectable_end_render         (ClayProjectable *projectable);
void         clay_projectable_invalidate_preview (ClayProjectable *projectable);
