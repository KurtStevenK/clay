/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claychannel.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"


G_DEFINE_TYPE (ClayChannel, clay_channel, CLAY_TYPE_DRAWABLE)

#define parent_class clay_drawable_parent_class


static void
clay_channel_class_init (ClayChannelClass *klass)
{
}

static void
clay_channel_init (ClayChannel *channel)
{
}

/**
 * clay_channel_get_by_id:
 * @channel_id: The channel id.
 *
 * Returns a #ClayChannel representing @channel_id. This function
 * calls clay_item_get_by_id() and returns the item if it is channel
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClayChannel for @channel_id
 *          or %NULL if @channel_id does not represent a valid
 *          channel. The object belongs to libclay and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
ClayChannel *
clay_channel_get_by_id (gint32 channel_id)
{
  ClayItem *item = clay_item_get_by_id (channel_id);

  if (CLAY_IS_CHANNEL (item))
    return (ClayChannel *) item;

  return NULL;
}
