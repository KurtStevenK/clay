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

#include "core/clayobject.h"


#define CLAY_TYPE_PAINT_CORE            (clay_paint_core_get_type ())
#define CLAY_PAINT_CORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINT_CORE, ClayPaintCore))
#define CLAY_PAINT_CORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINT_CORE, ClayPaintCoreClass))
#define CLAY_IS_PAINT_CORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINT_CORE))
#define CLAY_IS_PAINT_CORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINT_CORE))
#define CLAY_PAINT_CORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINT_CORE, ClayPaintCoreClass))


typedef struct _ClayPaintCoreClass ClayPaintCoreClass;

struct _ClayPaintCore
{
  ClayObject      parent_instance;

  gint            ID;                /*  unique instance ID                  */

  gchar          *undo_desc;         /*  undo description                    */

  gboolean        show_all;          /*  whether working in show-all mode    */

  ClayCoords      start_coords;      /*  the last stroke's endpoint for undo */

  ClayCoords      cur_coords;        /*  current coords                      */
  ClayCoords      last_coords;       /*  last coords                         */

  ClayVector2     last_paint;        /*  last point that was painted         */

  gdouble         distance;          /*  distance traveled by brush          */
  gdouble         pixel_dist;        /*  distance in pixels                  */

  gint            x1, y1;            /*  undo extents in image coords        */
  gint            x2, y2;            /*  undo extents in image coords        */

  gboolean        use_saved_proj;    /*  keep the unmodified proj around     */

  ClayPickable   *image_pickable;    /*  the image pickable                  */

  GHashTable     *undo_buffers;      /*  pixels which have been modified     */
  GeglBuffer     *saved_proj_buffer; /*  proj tiles which have been modified */
  GeglBuffer     *canvas_buffer;     /*  the buffer to paint the mask to     */
  GeglBuffer     *paint_buffer;      /*  the buffer to paint pixels to       */
  gint            paint_buffer_x;
  gint            paint_buffer_y;
  GHashTable     *original_bounds;   /* the original bounds of drawables     */

  GeglBuffer     *mask_buffer;       /*  the target drawable's mask          */

  GHashTable     *applicators;

  GArray         *stroke_buffer;

  ClaySymmetry   *sym;
  ClayPaintLockBlinkState
                  lock_blink_state;
};

struct _ClayPaintCoreClass
{
  ClayObjectClass  parent_class;

  /*  virtual functions  */
  gboolean     (* start)            (ClayPaintCore    *core,
                                     GList            *drawables,
                                     ClayPaintOptions *paint_options,
                                     const ClayCoords *coords,
                                     GError          **error);

  gboolean     (* pre_paint)        (ClayPaintCore    *core,
                                     GList            *drawables,
                                     ClayPaintOptions *paint_options,
                                     ClayPaintState    paint_state,
                                     guint32           time);
  void         (* paint)            (ClayPaintCore    *core,
                                     GList            *drawables,
                                     ClayPaintOptions *paint_options,
                                     ClaySymmetry     *sym,
                                     ClayPaintState    paint_state,
                                     guint32           time);
  void         (* post_paint)       (ClayPaintCore    *core,
                                     GList            *drawables,
                                     ClayPaintOptions *paint_options,
                                     ClayPaintState    paint_state,
                                     guint32           time);

  void         (* interpolate)      (ClayPaintCore    *core,
                                     GList            *drawables,
                                     ClayPaintOptions *paint_options,
                                     guint32           time);

  GeglBuffer * (* get_paint_buffer) (ClayPaintCore    *core,
                                     ClayDrawable     *drawable,
                                     ClayPaintOptions *paint_options,
                                     ClayLayerMode     paint_mode,
                                     const ClayCoords *coords,
                                     gint             *paint_buffer_x,
                                     gint             *paint_buffer_y,
                                     gint             *paint_width,
                                     gint             *paint_height);

  ClayUndo   * (* push_undo)        (ClayPaintCore    *core,
                                     ClayImage        *image,
                                     const gchar      *undo_desc);
};


GType     clay_paint_core_get_type                  (void);

void      clay_paint_core_paint                     (ClayPaintCore    *core,
                                                     GList            *drawables,
                                                     ClayPaintOptions *paint_options,
                                                     ClayPaintState    state,
                                                     guint32           time);

gboolean  clay_paint_core_start                     (ClayPaintCore    *core,
                                                     GList            *drawables,
                                                     ClayPaintOptions *paint_options,
                                                     const ClayCoords *coords,
                                                     GError          **error);
void      clay_paint_core_finish                    (ClayPaintCore    *core,
                                                     GList            *drawables,
                                                     gboolean          push_undo);
void      clay_paint_core_cancel                    (ClayPaintCore    *core,
                                                     GList            *drawables);
void      clay_paint_core_cleanup                   (ClayPaintCore    *core);

void      clay_paint_core_interpolate               (ClayPaintCore    *core,
                                                     GList            *drawables,
                                                     ClayPaintOptions *paint_options,
                                                     const ClayCoords *coords,
                                                     guint32           time);

void      clay_paint_core_set_show_all              (ClayPaintCore    *core,
                                                     gboolean          show_all);
gboolean  clay_paint_core_get_show_all              (ClayPaintCore    *core);

gboolean  clay_paint_core_expand_drawable           (ClayPaintCore    *paint_core,
                                                     ClayDrawable     *drawable,
                                                     ClayPaintOptions *paint_options,
                                                     gint              x1,
                                                     gint              x2,
                                                     gint              y1,
                                                     gint              y2,
                                                     gint             *x,
                                                     gint             *y);

void      clay_paint_core_set_current_coords        (ClayPaintCore    *core,
                                                     const ClayCoords *coords);
void      clay_paint_core_get_current_coords        (ClayPaintCore    *core,
                                                     ClayCoords       *coords);

void      clay_paint_core_set_last_coords           (ClayPaintCore    *core,
                                                     const ClayCoords *coords);
void      clay_paint_core_get_last_coords           (ClayPaintCore    *core,
                                                     ClayCoords       *coords);

void      clay_paint_core_round_line                (ClayPaintCore    *core,
                                                     ClayPaintOptions *options,
                                                     gboolean          constrain_15_degrees,
                                                     gdouble           constrain_offset_angle,
                                                     gdouble           constrain_xres,
                                                     gdouble           constrain_yres);


/*  protected functions  */

GeglBuffer * clay_paint_core_get_paint_buffer       (ClayPaintCore    *core,
                                                     ClayDrawable     *drawable,
                                                     ClayPaintOptions *options,
                                                     ClayLayerMode     paint_mode,
                                                     const ClayCoords *coords,
                                                     gint             *paint_buffer_x,
                                                     gint             *paint_buffer_y,
                                                     gint             *paint_width,
                                                     gint             *paint_height);

ClayPickable * clay_paint_core_get_image_pickable   (ClayPaintCore    *core);

GeglBuffer * clay_paint_core_get_orig_image         (ClayPaintCore    *core,
                                                     ClayDrawable     *drawable);
GeglBuffer * clay_paint_core_get_orig_proj          (ClayPaintCore    *core);

void      clay_paint_core_paste             (ClayPaintCore            *core,
                                             const ClayTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             ClayDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             ClayLayerMode             paint_mode,
                                             ClayPaintApplicationMode  mode);

void      clay_paint_core_replace           (ClayPaintCore            *core,
                                             const ClayTempBuf        *paint_mask,
                                             gint                      paint_mask_offset_x,
                                             gint                      paint_mask_offset_y,
                                             ClayDrawable             *drawable,
                                             gdouble                   paint_opacity,
                                             gdouble                   image_opacity,
                                             ClayPaintApplicationMode  mode);

void      clay_paint_core_smooth_coords             (ClayPaintCore    *core,
                                                     ClayPaintOptions *paint_options,
                                                     ClayCoords       *coords);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPaintCore, g_object_unref);
