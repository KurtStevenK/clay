/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymotionbuffer.h
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


#define CLAY_TYPE_MOTION_BUFFER            (clay_motion_buffer_get_type ())
#define CLAY_MOTION_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MOTION_BUFFER, ClayMotionBuffer))
#define CLAY_MOTION_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MOTION_BUFFER, ClayMotionBufferClass))
#define CLAY_IS_MOTION_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MOTION_BUFFER))
#define CLAY_IS_MOTION_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MOTION_BUFFER))
#define CLAY_MOTION_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MOTION_BUFFER, ClayMotionBufferClass))


typedef struct _ClayMotionBufferClass ClayMotionBufferClass;

struct _ClayMotionBuffer
{
  ClayObject  parent_instance;

  guint32     last_read_motion_time;

  guint32     last_motion_time; /*  previous time of a forwarded motion event  */
  gdouble     last_motion_delta_time;
  gdouble     last_motion_delta_x;
  gdouble     last_motion_delta_y;
  gdouble     last_motion_distance;

  ClayCoords  last_coords;      /* last motion event                   */

  GArray     *event_history;
  GArray     *event_queue;
  gboolean    event_delay;      /* TRUE if there's an unsent event in
                                 *  the history buffer
                                 */

  gint               event_delay_timeout;
  GdkModifierType    last_active_state;
};

struct _ClayMotionBufferClass
{
  ClayObjectClass  parent_class;

  void (* stroke) (ClayMotionBuffer *buffer,
                   const ClayCoords *coords,
                   guint32           time,
                   GdkModifierType   state);
  void (* hover)  (ClayMotionBuffer *buffer,
                   const ClayCoords *coords,
                   GdkModifierType   state,
                   gboolean          proximity);
};


GType              clay_motion_buffer_get_type     (void);

ClayMotionBuffer * clay_motion_buffer_new          (void);

void       clay_motion_buffer_begin_stroke         (ClayMotionBuffer *buffer,
                                                    guint32           time,
                                                    ClayCoords       *last_motion);
void       clay_motion_buffer_end_stroke           (ClayMotionBuffer *buffer);

gboolean   clay_motion_buffer_motion_event         (ClayMotionBuffer *buffer,
                                                    ClayCoords       *coords,
                                                    guint32           time,
                                                    gboolean          event_fill);
guint32    clay_motion_buffer_get_last_motion_time (ClayMotionBuffer *buffer);

void       clay_motion_buffer_request_stroke       (ClayMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    guint32           time);
void       clay_motion_buffer_request_hover        (ClayMotionBuffer *buffer,
                                                    GdkModifierType   state,
                                                    gboolean          proximity);
