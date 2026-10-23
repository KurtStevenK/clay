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

#include "claybrushcore.h"


#define CLAY_TYPE_SOURCE_CORE            (clay_source_core_get_type ())
#define CLAY_SOURCE_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SOURCE_CORE, ClaySourceCore))
#define CLAY_SOURCE_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SOURCE_CORE, ClaySourceCoreClass))
#define CLAY_IS_SOURCE_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SOURCE_CORE))
#define CLAY_IS_SOURCE_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SOURCE_CORE))
#define CLAY_SOURCE_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SOURCE_CORE, ClaySourceCoreClass))


typedef struct _ClaySourceCoreClass ClaySourceCoreClass;

struct _ClaySourceCore
{
  ClayBrushCore  parent_instance;

  gboolean       set_source;

  gint           orig_src_x;
  gint           orig_src_y;

  gint           offset_x;
  gint           offset_y;
  gboolean       first_stroke;
};

struct _ClaySourceCoreClass
{
  ClayBrushCoreClass  parent_class;

  gboolean     (* use_source) (ClaySourceCore    *source_core,
                               ClaySourceOptions *options);

  GeglBuffer * (* get_source) (ClaySourceCore    *source_core,
                               ClayDrawable      *drawable,
                               ClayPaintOptions  *paint_options,
                               gboolean           self_drawable,
                               ClayPickable      *src_pickable,
                               gint               src_offset_x,
                               gint               src_offset_y,
                               GeglBuffer        *paint_buffer,
                               gint               paint_buffer_x,
                               gint               paint_buffer_y,
                               /* offsets *into* the paint_buffer: */
                               gint              *paint_area_offset_x,
                               gint              *paint_area_offset_y,
                               gint              *paint_area_width,
                               gint              *paint_area_height,
                               GeglRectangle     *src_rect);

  void         (*  motion)    (ClaySourceCore    *source_core,
                               ClayDrawable      *drawable,
                               ClayPaintOptions  *paint_options,
                               const ClayCoords  *coords,
                               GeglNode          *op,
                               gdouble            opacity,
                               ClayPickable      *src_pickable,
                               GeglBuffer        *src_buffer,
                               GeglRectangle     *src_rect,
                               gint               src_offset_x,
                               gint               src_offset_y,
                               GeglBuffer        *paint_buffer,
                               gint               paint_buffer_x,
                               gint               paint_buffer_y,
                               /* offsets *into* the paint_buffer: */
                               gint               paint_area_offset_x,
                               gint               paint_area_offset_y,
                               gint               paint_area_width,
                               gint               paint_area_height);
};


GType    clay_source_core_get_type   (void);

gboolean clay_source_core_use_source (ClaySourceCore    *source_core,
                                      ClaySourceOptions *options);

/* TEMP HACK */
void     clay_source_core_motion     (ClaySourceCore    *source_core,
                                      ClayDrawable      *drawable,
                                      ClayPaintOptions  *paint_options,
                                      gboolean           self_drawable,
                                      ClaySymmetry      *sym);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClaySourceCore, g_object_unref);
