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

#include "claydrawable.h"


#define CLAY_TYPE_CHANNEL            (clay_channel_get_type ())
#define CLAY_CHANNEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CHANNEL, ClayChannel))
#define CLAY_CHANNEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CHANNEL, ClayChannelClass))
#define CLAY_IS_CHANNEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CHANNEL))
#define CLAY_IS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CHANNEL))
#define CLAY_CHANNEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CHANNEL, ClayChannelClass))


typedef struct _ClayChannelClass ClayChannelClass;

struct _ClayChannel
{
  ClayDrawable  parent_instance;

  GeglColor    *color;             /*  Also stores the opacity        */
  gboolean      show_masked;       /*  Show masked areas--as          */
                                   /*  opposed to selected areas      */

  GeglNode     *color_node;
  GeglNode     *invert_node;
  GeglNode     *mask_node;

  /*  Selection mask variables  */
  gboolean      boundary_known;    /*  is the current boundary valid  */
  ClayBoundSeg *segs_in;           /*  outline of selected region     */
  ClayBoundSeg *segs_out;          /*  outline of selected region     */
  gint          num_segs_in;       /*  number of lines in boundary    */
  gint          num_segs_out;      /*  number of lines in boundary    */
  gboolean      empty;             /*  is the region empty?           */
  gboolean      full;              /*  is the region completely full? */
  gboolean      bounds_known;      /*  recalculate the bounds?        */
  gint          x1, y1;            /*  coordinates for bounding box   */
  gint          x2, y2;            /*  lower right hand coordinate    */
};

struct _ClayChannelClass
{
  ClayDrawableClass  parent_class;

  /*  signals  */
  void     (* color_changed) (ClayChannel             *channel);

  /*  virtual functions  */
  gboolean (* boundary)      (ClayChannel             *channel,
                              const ClayBoundSeg     **segs_in,
                              const ClayBoundSeg     **segs_out,
                              gint                    *num_segs_in,
                              gint                    *num_segs_out,
                              gint                     x1,
                              gint                     y1,
                              gint                     x2,
                              gint                     y2);
  gboolean (* is_empty)      (ClayChannel             *channel);
  gboolean (* is_full)       (ClayChannel             *channel);

  void     (* feather)       (ClayChannel             *channel,
                              gdouble                  radius_x,
                              gdouble                  radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* sharpen)       (ClayChannel             *channel,
                              gboolean                 push_undo);
  void     (* clear)         (ClayChannel             *channel,
                              const gchar             *undo_desc,
                              gboolean                 push_undo);
  void     (* all)           (ClayChannel             *channel,
                              gboolean                 push_undo);
  void     (* invert)        (ClayChannel             *channel,
                              gboolean                 push_undo);
  void     (* border)        (ClayChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              ClayChannelBorderStyle   style,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* grow)          (ClayChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 push_undo);
  void     (* shrink)        (ClayChannel             *channel,
                              gint                     radius_x,
                              gint                     radius_y,
                              gboolean                 edge_lock,
                              gboolean                 push_undo);
  void     (* flood)         (ClayChannel             *channel,
                              gboolean                 push_undo);

  const gchar *feather_desc;
  const gchar *sharpen_desc;
  const gchar *clear_desc;
  const gchar *all_desc;
  const gchar *invert_desc;
  const gchar *border_desc;
  const gchar *grow_desc;
  const gchar *shrink_desc;
  const gchar *flood_desc;
};


/*  function declarations  */

GType         clay_channel_get_type           (void);

ClayChannel * clay_channel_new                (ClayImage         *image,
                                               gint               width,
                                               gint               height,
                                               const gchar       *name,
                                               GeglColor         *color);
ClayChannel * clay_channel_new_from_buffer    (ClayImage         *image,
                                               GeglBuffer        *buffer,
                                               const gchar       *name,
                                               GeglColor         *color);
ClayChannel * clay_channel_new_from_alpha     (ClayImage         *image,
                                               ClayDrawable      *drawable,
                                               const gchar       *name,
                                               GeglColor         *color);
ClayChannel * clay_channel_new_from_component (ClayImage         *image,
                                               ClayChannelType    type,
                                               const gchar       *name,
                                               GeglColor         *color);

ClayChannel * clay_channel_get_parent         (ClayChannel       *channel);

gdouble       clay_channel_get_opacity        (ClayChannel       *channel);
void          clay_channel_set_opacity        (ClayChannel       *channel,
                                               gdouble            opacity,
                                               gboolean           push_undo);

GeglColor   * clay_channel_get_color          (ClayChannel       *channel);
void          clay_channel_set_color          (ClayChannel       *channel,
                                               GeglColor         *color,
                                               gboolean           push_undo);

gboolean      clay_channel_get_show_masked    (ClayChannel       *channel);
void          clay_channel_set_show_masked    (ClayChannel       *channel,
                                               gboolean           show_masked);

void          clay_channel_push_undo          (ClayChannel       *mask,
                                               const gchar       *undo_desc);


/*  selection mask functions  */

ClayChannel * clay_channel_new_mask           (ClayImage              *image,
                                               gint                    width,
                                               gint                    height);

gboolean      clay_channel_boundary           (ClayChannel            *mask,
                                               const ClayBoundSeg    **segs_in,
                                               const ClayBoundSeg    **segs_out,
                                               gint                   *num_segs_in,
                                               gint                   *num_segs_out,
                                               gint                    x1,
                                               gint                    y1,
                                               gint                    x2,
                                               gint                    y2);
gboolean      clay_channel_is_empty           (ClayChannel            *mask);

gboolean      clay_channel_is_full            (ClayChannel            *mask);

void          clay_channel_feather            (ClayChannel            *mask,
                                               gdouble                 radius_x,
                                               gdouble                 radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          clay_channel_sharpen            (ClayChannel            *mask,
                                               gboolean                push_undo);

void          clay_channel_clear              (ClayChannel            *mask,
                                               const gchar            *undo_desc,
                                               gboolean                push_undo);
void          clay_channel_all                (ClayChannel            *mask,
                                               gboolean                push_undo);
void          clay_channel_invert             (ClayChannel            *mask,
                                               gboolean                push_undo);

void          clay_channel_border             (ClayChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               ClayChannelBorderStyle  style,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          clay_channel_grow               (ClayChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                push_undo);
void          clay_channel_shrink             (ClayChannel            *mask,
                                               gint                    radius_x,
                                               gint                    radius_y,
                                               gboolean                edge_lock,
                                               gboolean                push_undo);
void          clay_channel_flood              (ClayChannel            *mask,
                                               gboolean                push_undo);
