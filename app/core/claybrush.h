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


#define CLAY_TYPE_BRUSH            (clay_brush_get_type ())
#define CLAY_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH, ClayBrush))
#define CLAY_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH, ClayBrushClass))
#define CLAY_IS_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH))
#define CLAY_IS_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH))
#define CLAY_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH, ClayBrushClass))


typedef struct _ClayBrushPrivate ClayBrushPrivate;
typedef struct _ClayBrushClass   ClayBrushClass;

struct _ClayBrush
{
  ClayData          parent_instance;

  ClayBrushPrivate *priv;
};

struct _ClayBrushClass
{
  ClayDataClass  parent_class;

  /*  virtual functions  */
  void             (* begin_use)          (ClayBrush        *brush);
  void             (* end_use)            (ClayBrush        *brush);
  ClayBrush      * (* select_brush)       (ClayBrush        *brush,
                                           const ClayCoords *last_coords,
                                           const ClayCoords *current_coords);
  gboolean         (* want_null_motion)   (ClayBrush        *brush,
                                           const ClayCoords *last_coords,
                                           const ClayCoords *current_coords);
  void             (* transform_size)     (ClayBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gint             *width,
                                           gint             *height);
  ClayTempBuf    * (* transform_mask)     (ClayBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  ClayTempBuf    * (* transform_pixmap)   (ClayBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness);
  ClayBezierDesc * (* transform_boundary) (ClayBrush        *brush,
                                           gdouble           scale,
                                           gdouble           aspect_ratio,
                                           gdouble           angle,
                                           gboolean          reflect,
                                           gdouble           hardness,
                                           gint             *width,
                                           gint             *height);

  /*  signals  */
  void             (* spacing_changed)    (ClayBrush        *brush);
};


GType                  clay_brush_get_type           (void);

ClayData             * clay_brush_new                (ClayContext      *context,
                                                      const gchar      *name);
ClayData             * clay_brush_get_standard       (ClayContext      *context);

void                   clay_brush_begin_use          (ClayBrush        *brush);
void                   clay_brush_end_use            (ClayBrush        *brush);

ClayBrush            * clay_brush_select_brush       (ClayBrush        *brush,
                                                      const ClayCoords *last_coords,
                                                      const ClayCoords *current_coords);
gboolean               clay_brush_want_null_motion   (ClayBrush        *brush,
                                                      const ClayCoords *last_coords,
                                                      const ClayCoords *current_coords);

/* Gets width and height of a transformed mask of the brush, for
 * provided parameters.
 */
void                   clay_brush_transform_size     (ClayBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gint             *width,
                                                      gint             *height);
const ClayTempBuf    * clay_brush_transform_mask     (ClayBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const ClayTempBuf    * clay_brush_transform_pixmap   (ClayBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness);
const ClayBezierDesc * clay_brush_transform_boundary (ClayBrush        *brush,
                                                      gdouble           scale,
                                                      gdouble           aspect_ratio,
                                                      gdouble           angle,
                                                      gboolean          reflect,
                                                      gdouble           hardness,
                                                      gint             *width,
                                                      gint             *height);

ClayTempBuf          * clay_brush_get_mask           (ClayBrush        *brush);
ClayTempBuf          * clay_brush_get_pixmap         (ClayBrush        *brush);

gint                   clay_brush_get_width          (ClayBrush        *brush);
gint                   clay_brush_get_height         (ClayBrush        *brush);

gint                   clay_brush_get_spacing        (ClayBrush        *brush);
void                   clay_brush_set_spacing        (ClayBrush        *brush,
                                                      gint              spacing);

ClayVector2            clay_brush_get_x_axis         (ClayBrush        *brush);
ClayVector2            clay_brush_get_y_axis         (ClayBrush        *brush);

void                   clay_brush_flush_blur_caches  (ClayBrush        *brush);
gdouble                clay_brush_get_blur_hardness  (ClayBrush        *brush);
