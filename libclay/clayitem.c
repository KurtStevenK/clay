/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * clayitem.c
 * Copyright (C) Jehan
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

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "clayplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


typedef struct _ClayItemPrivate
{
  gint id;
} ClayItemPrivate;


static void   clay_item_set_property  (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec);
static void   clay_item_get_property  (GObject      *object,
                                       guint         property_id,
                                       GValue       *value,
                                       GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (ClayItem, clay_item, G_TYPE_OBJECT)

#define parent_class clay_item_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_item_class_init (ClayItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_item_set_property;
  object_class->get_property = clay_item_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The item id",
                      "The item id for internal use",
                      0, G_MAXINT32, 0,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_item_init (ClayItem *item)
{
}

static void
clay_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayItem        *item = CLAY_ITEM (object);
  ClayItemPrivate *priv = clay_item_get_instance_private (item);

  switch (property_id)
    {
    case PROP_ID:
      priv->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_item_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayItem        *item = CLAY_ITEM (object);
  ClayItemPrivate *priv = clay_item_get_instance_private (item);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, priv->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API. */


/**
 * clay_item_get_id:
 * @item: The item.
 *
 * Note: in most use cases, you should not need an item's ID which is
 * mostly internal data and not reusable across sessions.
 *
 * Returns: the item ID.
 *
 * Since: 3.0
 **/
gint32
clay_item_get_id (ClayItem *item)
{
  if (item)
    {
      ClayItemPrivate *priv = clay_item_get_instance_private (item);

      return priv->id;
    }
  else
    {
      return -1;
    }
}

/**
 * clay_item_get_by_id:
 * @item_id: The item id.
 *
 * Returns a #ClayItem representing @item_id. Since #ClayItem is an
 * abstract class, the real object type will actually be the proper
 * subclass.
 *
 * Note: in most use cases, you should not need to retrieve a #ClayItem
 * by its ID, which is mostly internal data and not reusable across
 * sessions. Use the appropriate functions for your use case instead.
 *
 * Returns: (nullable) (transfer none): a #ClayItem for @item_id or
 *          %NULL if @item_id does not represent a valid item.
 *          The object belongs to libclay and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
ClayItem *
clay_item_get_by_id (gint32 item_id)
{
  if (item_id > 0)
    {
      ClayPlugIn *plug_in = clay_get_plug_in ();

      return _clay_plug_in_get_item (plug_in, item_id);
    }

  return NULL;
}

/**
 * clay_item_is_valid:
 * @item: The item to check.
 *
 * Returns TRUE if the item is valid.
 *
 * This procedure checks if the given item is valid and refers to an
 * existing item.
 *
 * Returns: Whether the item is valid.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_valid (ClayItem *item)
{
  return clay_item_id_is_valid (clay_item_get_id (item));
}

/**
 * clay_item_is_drawable:
 * @item: The item.
 *
 * Returns whether the item is a drawable.
 *
 * This procedure returns TRUE if the specified item is a drawable.
 *
 * Returns: TRUE if the item is a drawable, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_drawable (ClayItem *item)
{
  return clay_item_id_is_drawable (clay_item_get_id (item));
}

/**
 * clay_item_is_layer:
 * @item: The item.
 *
 * Returns whether the item is a layer.
 *
 * This procedure returns TRUE if the specified item is a layer.
 *
 * Returns: TRUE if the item is a layer, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_layer (ClayItem *item)
{
  return clay_item_id_is_layer (clay_item_get_id (item));
}

/**
 * clay_item_is_text_layer:
 * @item: The item.
 *
 * Returns whether the item is a text layer.
 *
 * This procedure returns TRUE if the specified item is a text
 * layer.
 *
 * Returns: TRUE if the item is a text layer, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_text_layer (ClayItem *item)
{
  return clay_item_id_is_text_layer (clay_item_get_id (item));
}

/**
 * clay_item_is_vector_layer:
 * @item: The item.
 *
 * Returns whether the item is a vector layer.
 *
 * This procedure returns TRUE if the specified item is a vector
 * layer.
 *
 * Returns: TRUE if the item is a vector layer, FALSE otherwise.
 *
 * Since: 3.2
 **/
gboolean
clay_item_is_vector_layer (ClayItem *item)
{
  return clay_item_id_is_vector_layer (clay_item_get_id (item));
}

/**
 * clay_item_is_link_layer:
 * @item: The item.
 *
 * Returns whether the item is a link layer.
 *
 * This procedure returns TRUE if the specified item is a link
 * layer.
 *
 * Returns: TRUE if the item is a link layer, FALSE otherwise.
 *
 * Since: 3.2
 **/
gboolean
clay_item_is_link_layer (ClayItem *item)
{
  return clay_item_id_is_link_layer (clay_item_get_id (item));
}

/**
 * clay_item_is_group_layer:
 * @item: The item.
 *
 * Returns whether the item is a group layer.
 *
 * This procedure returns TRUE if the specified item is a group
 * layer.
 *
 * Returns: TRUE if the item is a group layer, FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_item_is_group_layer (ClayItem *item)
{
  return clay_item_id_is_group_layer (clay_item_get_id (item));
}

/**
 * clay_item_is_channel:
 * @item: The item.
 *
 * Returns whether the item is a channel.
 *
 * This procedure returns TRUE if the specified item is a channel.
 *
 * Returns: TRUE if the item is a channel, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_channel (ClayItem *item)
{
  return clay_item_id_is_channel (clay_item_get_id (item));
}

/**
 * clay_item_is_layer_mask:
 * @item: The item.
 *
 * Returns whether the item is a layer mask.
 *
 * This procedure returns TRUE if the specified item is a layer
 * mask.
 *
 * Returns: TRUE if the item is a layer mask, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_layer_mask (ClayItem *item)
{
  return clay_item_id_is_layer_mask (clay_item_get_id (item));
}

/**
 * clay_item_is_selection:
 * @item: The item.
 *
 * Returns whether the item is a selection.
 *
 * This procedure returns TRUE if the specified item is a selection.
 *
 * Returns: TRUE if the item is a selection, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_selection (ClayItem *item)
{
  return clay_item_id_is_selection (clay_item_get_id (item));
}

/**
 * clay_item_is_path:
 * @item: The item.
 *
 * Returns whether the item is a path.
 *
 * This procedure returns TRUE if the specified item is a path.
 *
 * Returns: TRUE if the item is a path, FALSE otherwise.
 *
 * Since: 2.8
 **/
gboolean
clay_item_is_path (ClayItem *item)
{
  return clay_item_id_is_path (clay_item_get_id (item));
}

/**
 * clay_item_list_children: (skip) (attributes skip-reason=provided_by_clay_item_get_children_since_703305d2)
 * @item: The item.
 *
 * Returns the item's list of children.
 *
 * This procedure returns the list of items which are children of the
 * specified item. The order is topmost to bottommost.
 *
 * Returns: (element-type ClayItem) (transfer container):
 *          The item's list of children.
 *          The returned list must be freed with g_list_free(). Item
 *          elements belong to libclay and must not be unrefed.
 *
 * Since: 3.0
 **/
GList *
clay_item_list_children (ClayItem *item)
{
  ClayItem **children;
  GList     *list = NULL;
  gint       i;

  children = clay_item_get_children (item);

  if (children)
    for (i = 0; children[i] != NULL; i++)
      list = g_list_prepend (list, children[i]);

  g_free (children);

  return g_list_reverse (list);
}
