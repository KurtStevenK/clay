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

#include "claysubprogress.h"


#define CLAY_TYPE_OBJECT_QUEUE            (clay_object_queue_get_type ())
#define CLAY_OBJECT_QUEUE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OBJECT_QUEUE, ClayObjectQueue))
#define CLAY_OBJECT_QUEUE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_OBJECT_QUEUE, ClayObjectQueueClass))
#define CLAY_IS_OBJECT_QUEUE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OBJECT_QUEUE))
#define CLAY_IS_OBJECT_QUEUE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_OBJECT_QUEUE))
#define CLAY_OBJECT_QUEUE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_OBJECT_QUEUE, ClayObjectQueueClass))


typedef struct _ClayObjectQueueClass ClayObjectQueueClass;

struct _ClayObjectQueue
{
  ClaySubProgress  parent_instance;

  GQueue           items;
  gint64           processed_memsize;
  gint64           total_memsize;
};

struct _ClayObjectQueueClass
{
  ClaySubProgressClass  parent_class;
};


GType             clay_object_queue_get_type       (void);

ClayObjectQueue * clay_object_queue_new            (ClayProgress    *progress);

void              clay_object_queue_clear          (ClayObjectQueue *queue);

void              clay_object_queue_push           (ClayObjectQueue *queue,
                                                    gpointer         object);
void              clay_object_queue_push_container (ClayObjectQueue *queue,
                                                    ClayContainer   *container);
void              clay_object_queue_push_list      (ClayObjectQueue *queue,
                                                    GList           *list);

gpointer          clay_object_queue_pop            (ClayObjectQueue *queue);
