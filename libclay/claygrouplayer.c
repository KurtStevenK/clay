/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claygrouplayer.c
 * Copyright (C) 2022 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"


struct _ClayGroupLayer
{
  ClayLayer parent_instance;
};


G_DEFINE_TYPE (ClayGroupLayer, clay_group_layer, CLAY_TYPE_LAYER)

#define parent_class clay_group_layer_parent_class


static void
clay_group_layer_class_init (ClayGroupLayerClass *klass)
{
}

static void
clay_group_layer_init (ClayGroupLayer *layer)
{
}


/* Public API. */

/**
 * clay_group_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #ClayGroupLayer representing @layer_id. This function calls
 * clay_item_get_by_id() and returns the item if it is a group layer or
 * %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClayGroupLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid group layer.
 *          The object belongs to libclay and you must not modify or
 *          unref it.
 *
 * Since: 3.0
 **/
ClayGroupLayer *
clay_group_layer_get_by_id (gint32 layer_id)
{
  ClayItem *item = clay_item_get_by_id (layer_id);

  if (CLAY_IS_GROUP_LAYER (item))
    return (ClayGroupLayer *) item;

  return NULL;
}
