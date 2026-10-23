/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claylinklayer.c
 * Copyright (C) 2025 Jehan
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


struct _ClayLinkLayer
{
  ClayLayer parent_instance;
};


G_DEFINE_TYPE_WITH_CODE (ClayLinkLayer, clay_link_layer, CLAY_TYPE_LAYER,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_RASTERIZABLE, NULL))

#define parent_class clay_link_layer_parent_class


static void
clay_link_layer_class_init (ClayLinkLayerClass *klass)
{
}

static void
clay_link_layer_init (ClayLinkLayer *layer)
{
}


/* Public API. */

/**
 * clay_link_layer_get_by_id:
 * @layer_id: The layer id.
 *
 * Returns a #ClayLinkLayer representing @layer_id. This function calls
 * [func@Clay.Item.get_by_id] and returns the item if it is a link
 * layer or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClayLinkLayer for @layer_id or
 *          %NULL if @layer_id does not represent a valid link layer. The
 *          object belongs to libclay and you must not modify or unref
 *          it.
 *
 * Since: 3.2
 **/
ClayLinkLayer *
clay_link_layer_get_by_id (gint32 layer_id)
{
  ClayItem *item = clay_item_get_by_id (layer_id);

  if (CLAY_IS_LINK_LAYER (item))
    return (ClayLinkLayer *) item;

  return NULL;
}
