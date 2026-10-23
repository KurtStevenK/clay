/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytreeproxy.c
 * Copyright (C) 2020 Ell
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clayviewable.h"
#include "claytreeproxy.h"


enum
{
  PROP_0,
  PROP_CONTAINER,
  PROP_FLAT
};


struct _ClayTreeProxyPrivate
{
  ClayContainer *container;
  gboolean       flat;
};


/*  local function prototypes  */

static void   clay_tree_proxy_dispose             (GObject      *object);
static void   clay_tree_proxy_set_property        (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   clay_tree_proxy_get_property        (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void   clay_tree_proxy_container_add       (ClayContainer *container,
                                                   ClayObject    *object,
                                                   ClayTreeProxy *tree_proxy);
static void   clay_tree_proxy_container_remove    (ClayContainer *container,
                                                   ClayObject    *object,
                                                   ClayTreeProxy *tree_proxy);
static void   clay_tree_proxy_container_reorder   (ClayContainer *container,
                                                   ClayObject    *object,
                                                   gint           old_index,
                                                   gint           new_index,
                                                   ClayTreeProxy *tree_proxy);
static void   clay_tree_proxy_container_freeze    (ClayContainer *container,
                                                   ClayTreeProxy *tree_proxy);
static void   clay_tree_proxy_container_thaw      (ClayContainer *container,
                                                   ClayTreeProxy *tree_proxy);

static gint   clay_tree_proxy_add_container       (ClayTreeProxy *tree_proxy,
                                                   ClayContainer *container,
                                                   gint           index);
static void   clay_tree_proxy_remove_container    (ClayTreeProxy *tree_proxy,
                                                   ClayContainer *container);

static gint   clay_tree_proxy_add_object          (ClayTreeProxy *tree_proxy,
                                                   ClayObject    *object,
                                                   gint           index);
static void   clay_tree_proxy_remove_object       (ClayTreeProxy *tree_proxy,
                                                   ClayObject    *object);

static gint   clay_tree_proxy_find_container      (ClayTreeProxy *tree_proxy,
                                                   ClayContainer *container);
static gint   clay_tree_proxy_find_object         (ClayContainer *container,
                                                   ClayObject    *object);


G_DEFINE_TYPE_WITH_PRIVATE (ClayTreeProxy, clay_tree_proxy, CLAY_TYPE_LIST)

#define parent_class clay_tree_proxy_parent_class


/*  private functions  */

static void
clay_tree_proxy_class_init (ClayTreeProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_tree_proxy_dispose;
  object_class->set_property = clay_tree_proxy_set_property;
  object_class->get_property = clay_tree_proxy_get_property;

  g_object_class_install_property (object_class, PROP_CONTAINER,
                                   g_param_spec_object ("container", NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FLAT,
                                   g_param_spec_boolean ("flat", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_tree_proxy_init (ClayTreeProxy *tree_proxy)
{
  tree_proxy->priv = clay_tree_proxy_get_instance_private (tree_proxy);
}

static void
clay_tree_proxy_dispose (GObject *object)
{
  ClayTreeProxy *tree_proxy = CLAY_TREE_PROXY (object);

  clay_tree_proxy_set_container (tree_proxy, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);;
}

static void
clay_tree_proxy_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayTreeProxy *tree_proxy = CLAY_TREE_PROXY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      clay_tree_proxy_set_container (tree_proxy, g_value_get_object (value));
      break;

    case PROP_FLAT:
      clay_tree_proxy_set_flat (tree_proxy, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tree_proxy_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayTreeProxy *tree_proxy = CLAY_TREE_PROXY (object);

  switch (property_id)
    {
    case PROP_CONTAINER:
      g_value_set_object (value, tree_proxy->priv->container);
      break;

    case PROP_FLAT:
      g_value_set_boolean (value, tree_proxy->priv->flat);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_tree_proxy_container_add (ClayContainer *container,
                               ClayObject    *object,
                               ClayTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = clay_tree_proxy_find_container (tree_proxy, container) +
              clay_tree_proxy_find_object    (container,  object);
    }
  else
    {
      index = clay_container_get_child_index (container, object);
    }

  clay_tree_proxy_add_object (tree_proxy, object, index);
}

static void
clay_tree_proxy_container_remove (ClayContainer *container,
                                  ClayObject    *object,
                                  ClayTreeProxy *tree_proxy)
{
  clay_tree_proxy_remove_object (tree_proxy, object);
}

static void
clay_tree_proxy_container_reorder (ClayContainer *container,
                                   ClayObject    *object,
                                   gint           old_index,
                                   gint           new_index,
                                   ClayTreeProxy *tree_proxy)
{
  gint index;

  if (tree_proxy->priv->flat)
    {
      index = clay_tree_proxy_find_container (tree_proxy, container) +
              clay_tree_proxy_find_object    (container,  object);

      if (clay_viewable_get_children (CLAY_VIEWABLE (object)))
        {
          clay_container_freeze (CLAY_CONTAINER (tree_proxy));

          clay_tree_proxy_remove_object (tree_proxy, object);
          clay_tree_proxy_add_object    (tree_proxy, object, index);

          clay_container_thaw (CLAY_CONTAINER (tree_proxy));

          return;
        }
    }
  else
    {
      index = new_index;
    }

  clay_container_reorder (CLAY_CONTAINER (tree_proxy), object, index);
}

static void
clay_tree_proxy_container_freeze (ClayContainer *container,
                                  ClayTreeProxy *tree_proxy)
{
  clay_container_freeze (CLAY_CONTAINER (tree_proxy));
}

static void
clay_tree_proxy_container_thaw (ClayContainer *container,
                                ClayTreeProxy *tree_proxy)
{
  clay_container_thaw (CLAY_CONTAINER (tree_proxy));
}

typedef struct
{
  ClayTreeProxy *tree_proxy;
  gint           index;
} AddContainerData;

static void
clay_tree_proxy_add_container_func (ClayObject       *object,
                                    AddContainerData *data)
{
  data->index = clay_tree_proxy_add_object (data->tree_proxy,
                                            object, data->index);
}

static gint
clay_tree_proxy_add_container (ClayTreeProxy *tree_proxy,
                               ClayContainer *container,
                               gint           index)
{
  AddContainerData data;

  g_signal_connect (container, "add",
                    G_CALLBACK (clay_tree_proxy_container_add),
                    tree_proxy);
  g_signal_connect (container, "remove",
                    G_CALLBACK (clay_tree_proxy_container_remove),
                    tree_proxy);
  g_signal_connect (container, "reorder",
                    G_CALLBACK (clay_tree_proxy_container_reorder),
                    tree_proxy);
  g_signal_connect (container, "freeze",
                    G_CALLBACK (clay_tree_proxy_container_freeze),
                    tree_proxy);
  g_signal_connect (container, "thaw",
                    G_CALLBACK (clay_tree_proxy_container_thaw),
                    tree_proxy);

  data.tree_proxy = tree_proxy;
  data.index      = index;

  clay_container_freeze (CLAY_CONTAINER (tree_proxy));

  clay_container_foreach (container,
                          (GFunc) clay_tree_proxy_add_container_func,
                          &data);

  clay_container_thaw (CLAY_CONTAINER (tree_proxy));

  return data.index;
}

static void
clay_tree_proxy_remove_container_func (ClayObject    *object,
                                       ClayTreeProxy *tree_proxy)
{
  clay_tree_proxy_remove_object (tree_proxy, object);
}

static void
clay_tree_proxy_remove_container (ClayTreeProxy *tree_proxy,
                                  ClayContainer *container)
{
  clay_container_freeze (CLAY_CONTAINER (tree_proxy));

  clay_container_foreach (container,
                          (GFunc) clay_tree_proxy_remove_container_func,
                          tree_proxy);

  clay_container_thaw (CLAY_CONTAINER (tree_proxy));

  g_signal_handlers_disconnect_by_func (
    container,
    clay_tree_proxy_container_add,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    clay_tree_proxy_container_remove,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    clay_tree_proxy_container_reorder,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    clay_tree_proxy_container_freeze,
    tree_proxy);
  g_signal_handlers_disconnect_by_func (
    container,
    clay_tree_proxy_container_thaw,
    tree_proxy);
}

static gint
clay_tree_proxy_add_object (ClayTreeProxy *tree_proxy,
                            ClayObject    *object,
                            gint           index)
{
  if (index == clay_container_get_n_children (CLAY_CONTAINER (tree_proxy)))
    index = -1;

  if (tree_proxy->priv->flat)
    {
      ClayContainer *children;

      children = clay_viewable_get_children (CLAY_VIEWABLE (object));

      if (children)
        return clay_tree_proxy_add_container (tree_proxy, children, index);
    }

  if (index >= 0)
    {
      clay_container_insert (CLAY_CONTAINER (tree_proxy), object, index);

      return index + 1;
    }
  else
    {
      clay_container_add (CLAY_CONTAINER (tree_proxy), object);

      return index;
    }
}

static void
clay_tree_proxy_remove_object (ClayTreeProxy *tree_proxy,
                               ClayObject    *object)
{
  if (tree_proxy->priv->flat)
    {
      ClayContainer *children;

      children = clay_viewable_get_children (CLAY_VIEWABLE (object));

      if (children)
        return clay_tree_proxy_remove_container (tree_proxy, children);
    }

  clay_container_remove (CLAY_CONTAINER (tree_proxy), object);
}

typedef struct
{
  ClayContainer *container;
  gint           index;
} FindContainerData;

static gboolean
clay_tree_proxy_find_container_search_func (ClayObject        *object,
                                            FindContainerData *data)
{
  ClayContainer *children;

  children = clay_viewable_get_children (CLAY_VIEWABLE (object));

  if (children)
    {
      if (children == data->container)
        return TRUE;

      return clay_container_search (
        children,
        (ClayContainerSearchFunc) clay_tree_proxy_find_container_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
clay_tree_proxy_find_container (ClayTreeProxy *tree_proxy,
                                ClayContainer *container)
{
  FindContainerData data;

  if (container == tree_proxy->priv->container)
    return 0;

  data.container = container;
  data.index     = 0;

  if (clay_container_search (
        tree_proxy->priv->container,
        (ClayContainerSearchFunc) clay_tree_proxy_find_container_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}

typedef struct
{
  ClayObject *object;
  gint        index;
} FindObjectData;

static gboolean
clay_tree_proxy_find_object_search_func (ClayObject     *object,
                                         FindObjectData *data)
{
  ClayContainer *children;

  if (object == data->object)
    return TRUE;

  children = clay_viewable_get_children (CLAY_VIEWABLE (object));

  if (children)
    {
      return clay_container_search (
        children,
        (ClayContainerSearchFunc) clay_tree_proxy_find_object_search_func,
        data) != NULL;
    }

  data->index++;

  return FALSE;
}

static gint
clay_tree_proxy_find_object (ClayContainer *container,
                             ClayObject    *object)
{
  FindObjectData data;

  data.object = object;
  data.index  = 0;

  if (clay_container_search (
        container,
        (ClayContainerSearchFunc) clay_tree_proxy_find_object_search_func,
        &data))
    {
      return data.index;
    }

  g_return_val_if_reached (0);
}


/*  public functions  */

ClayContainer *
clay_tree_proxy_new (GType child_type)
{
  GTypeClass *children_class;

  children_class = g_type_class_ref (child_type);

  g_return_val_if_fail (G_TYPE_CHECK_CLASS_TYPE (children_class,
                                                 CLAY_TYPE_VIEWABLE),
                        NULL);

  g_type_class_unref (children_class);

  return g_object_new (CLAY_TYPE_TREE_PROXY,
                       "child-type", child_type,
                       "policy",     CLAY_CONTAINER_POLICY_WEAK,
                       "append",     TRUE,
                       NULL);
}

ClayContainer *
clay_tree_proxy_new_for_container (ClayContainer *container)
{
  ClayTreeProxy *tree_proxy;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);

  tree_proxy = CLAY_TREE_PROXY (
    clay_tree_proxy_new (clay_container_get_child_type (container)));

  clay_tree_proxy_set_container (tree_proxy, container);

  return CLAY_CONTAINER (tree_proxy);
}

void
clay_tree_proxy_set_container (ClayTreeProxy *tree_proxy,
                               ClayContainer *container)
{
  g_return_if_fail (CLAY_IS_TREE_PROXY (tree_proxy));
  g_return_if_fail (container == NULL || CLAY_IS_CONTAINER (container));

  if (container)
    {
      GTypeClass *children_class;

      children_class = g_type_class_ref (
        clay_container_get_child_type (container));

      g_return_if_fail (
        G_TYPE_CHECK_CLASS_TYPE (
          children_class,
          clay_container_get_child_type (CLAY_CONTAINER (tree_proxy))));

      g_type_class_unref (children_class);
    }

  if (container != tree_proxy->priv->container)
    {
      clay_container_freeze (CLAY_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          clay_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      g_set_object (&tree_proxy->priv->container, container);

      if (tree_proxy->priv->container)
        {
          clay_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      clay_container_thaw (CLAY_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "container");
    }
}

ClayContainer *
clay_tree_proxy_get_container (ClayTreeProxy *tree_proxy)
{
  g_return_val_if_fail (CLAY_IS_TREE_PROXY (tree_proxy), NULL);

  return tree_proxy->priv->container;
}

void
clay_tree_proxy_set_flat (ClayTreeProxy *tree_proxy,
                          gboolean       flat)
{
  g_return_if_fail (CLAY_IS_TREE_PROXY (tree_proxy));

  if (flat != tree_proxy->priv->flat)
    {
      clay_container_freeze (CLAY_CONTAINER (tree_proxy));

      if (tree_proxy->priv->container)
        {
          clay_tree_proxy_remove_container (tree_proxy,
                                            tree_proxy->priv->container);
        }

      tree_proxy->priv->flat = flat;

      if (tree_proxy->priv->container)
        {
          clay_tree_proxy_add_container (tree_proxy,
                                         tree_proxy->priv->container,
                                         -1);
        }

      clay_container_thaw (CLAY_CONTAINER (tree_proxy));

      g_object_notify (G_OBJECT (tree_proxy), "flat");
    }
}

gboolean
clay_tree_proxy_get_flat (ClayTreeProxy *tree_proxy)
{
  g_return_val_if_fail (CLAY_IS_TREE_PROXY (tree_proxy), FALSE);

  return tree_proxy->priv->flat;
}
