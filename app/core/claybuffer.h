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

#include "clayviewable.h"


#define CLAY_TYPE_BUFFER            (clay_buffer_get_type ())
#define CLAY_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BUFFER, ClayBuffer))
#define CLAY_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BUFFER, ClayBufferClass))
#define CLAY_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BUFFER))
#define CLAY_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BUFFER))
#define CLAY_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BUFFER, ClayBufferClass))


typedef struct _ClayBufferClass ClayBufferClass;

struct _ClayBuffer
{
  ClayViewable      parent_instance;

  GeglBuffer       *buffer;
  gint              offset_x;
  gint              offset_y;

  gdouble           resolution_x;
  gdouble           resolution_y;
  ClayUnit         *unit;

  ClayColorProfile *color_profile;
  ClayColorProfile *format_profile;
};

struct _ClayBufferClass
{
  ClayViewableClass  parent_class;
};


GType              clay_buffer_get_type          (void);

ClayBuffer       * clay_buffer_new               (GeglBuffer       *buffer,
                                                  const gchar      *name,
                                                  gint              offset_x,
                                                  gint              offset_y,
                                                  gboolean          copy_pixels);
ClayBuffer       * clay_buffer_new_from_pixbuf   (GdkPixbuf        *pixbuf,
                                                  const gchar      *name,
                                                  gint              offset_x,
                                                  gint              offset_y);

gint               clay_buffer_get_width         (ClayBuffer       *buffer);
gint               clay_buffer_get_height        (ClayBuffer       *buffer);
const Babl       * clay_buffer_get_format        (ClayBuffer       *buffer);

GeglBuffer       * clay_buffer_get_buffer        (ClayBuffer       *buffer);

void               clay_buffer_set_resolution    (ClayBuffer       *buffer,
                                                  gdouble           resolution_x,
                                                  gdouble           resolution_y);
gboolean           clay_buffer_get_resolution    (ClayBuffer       *buffer,
                                                  gdouble          *resolution_x,
                                                  gdouble          *resolution_y);

void               clay_buffer_set_unit          (ClayBuffer       *buffer,
                                                  ClayUnit         *unit);
ClayUnit         * clay_buffer_get_unit          (ClayBuffer       *buffer);

void               clay_buffer_set_color_profile (ClayBuffer       *buffer,
                                                  ClayColorProfile *profile);
ClayColorProfile * clay_buffer_get_color_profile (ClayBuffer       *buffer);
