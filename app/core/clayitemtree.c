/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayitemtree.c
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clayimage.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "clayitemstack.h"
#include "clayitemtree.h"


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_CONTAINER_TYPE,
  PROP_ITEM_TYPE,
  PROP_ACTIVE_ITEM,
  PROP_SELECTED_ITEMS
};


typedef struct _ClayItemTreePrivate ClayItemTreePrivate;

struct _ClayItemTreePrivate
{
  ClayImage  *image;

  GType       container_type;
  GType       item_type;

  GList      *selected_items;

  GHashTable *name_hash;
};

#define CLAY_ITEM_TREE_GET_PRIVATE(object) \
        ((ClayItemTreePrivate *) clay_item_tree_get_instance_private ((ClayItemTree *) (object)))


/*  local function prototypes  */

static void     clay_item_tree_constructed   (GObject      *object);
static void     clay_item_tree_dispose       (GObject      *object);
static void     clay_item_tree_finalize      (GObject      *object);
static void     clay_item_tree_set_property  (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void     clay_item_tree_get_property  (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

static gint64   clay_item_tree_get_memsize   (ClayObject   *object,
                                              gint64       *gui_size);

static void     clay_item_tree_uniquefy_name (ClayItemTree *tree,
                                              ClayItem     *item,
                                              const gchar  *new_name);


G_DEFINE_TYPE_WITH_PRIVATE (ClayItemTree, clay_item_tree, CLAY_TYPE_OBJECT)

#define parent_class clay_item_tree_parent_class


static void
clay_item_tree_class_init (ClayItemTreeClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->constructed      = clay_item_tree_constructed;
  object_class->dispose          = clay_item_tree_dispose;
  object_class->finalize         = clay_item_tree_finalize;
  object_class->set_property     = clay_item_tree_set_property;
  object_class->get_property     = clay_item_tree_get_property;

  clay_object_class->get_memsize = clay_item_tree_get_memsize;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTAINER_TYPE,
                                   g_param_spec_gtype ("container-type",
                                                       NULL, NULL,
                                                       CLAY_TYPE_ITEM_STACK,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ITEM_TYPE,
                                   g_param_spec_gtype ("item-type",
                                                       NULL, NULL,
                                                       CLAY_TYPE_ITEM,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ACTIVE_ITEM,
                                   g_param_spec_object ("active-item",
                                                        NULL, NULL,
                                                        CLAY_TYPE_ITEM,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SELECTED_ITEMS,
                                   g_param_spec_pointer ("selected-items",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_item_tree_init (ClayItemTree *tree)
{
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  private->name_hash      = g_hash_table_new (g_str_hash, g_str_equal);
  private->selected_items = NULL;
}

static void
clay_item_tree_constructed (GObject *object)
{
  ClayItemTree        *tree    = CLAY_ITEM_TREE (object);
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_IMAGE (private->image));
  clay_assert (g_type_is_a (private->container_type, CLAY_TYPE_ITEM_STACK));
  clay_assert (g_type_is_a (private->item_type,      CLAY_TYPE_ITEM));
  clay_assert (private->item_type != CLAY_TYPE_ITEM);

  tree->container = g_object_new (private->container_type,
                                  "name",       g_type_name (private->item_type),
                                  "child-type", private->item_type,
                                  "policy",     CLAY_CONTAINER_POLICY_STRONG,
                                  NULL);
}

static void
clay_item_tree_dispose (GObject *object)
{
  ClayItemTree        *tree    = CLAY_ITEM_TREE (object);
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  clay_item_tree_set_selected_items (tree, NULL);

  clay_container_foreach (tree->container,
                          (GFunc) clay_item_removed, NULL);

  clay_container_clear (tree->container);
  g_hash_table_remove_all (private->name_hash);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_item_tree_finalize (GObject *object)
{
  ClayItemTree        *tree    = CLAY_ITEM_TREE (object);
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_clear_pointer (&private->name_hash, g_hash_table_unref);
  g_clear_object (&tree->container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_item_tree_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      private->image = g_value_get_object (value); /* don't ref */
      break;
    case PROP_CONTAINER_TYPE:
      private->container_type = g_value_get_gtype (value);
      break;
    case PROP_ITEM_TYPE:
      private->item_type = g_value_get_gtype (value);
      break;
    case PROP_ACTIVE_ITEM:
      /* Don't ref the item. */
      private->selected_items = g_list_prepend (NULL, g_value_get_object (value));
      break;
    case PROP_SELECTED_ITEMS:
      /* Don't ref the item. */
      private->selected_items = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_item_tree_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;
    case PROP_CONTAINER_TYPE:
      g_value_set_gtype (value, private->container_type);
      break;
    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, private->item_type);
      break;
    case PROP_ACTIVE_ITEM:
      g_value_set_object (value, clay_item_tree_get_active_item (CLAY_ITEM_TREE (object)));
      break;
    case PROP_SELECTED_ITEMS:
      g_value_set_pointer (value, private->selected_items);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_item_tree_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayItemTree *tree    = CLAY_ITEM_TREE (object);
  gint64        memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (tree->container), gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/*  public functions  */

ClayItemTree *
clay_item_tree_new (ClayImage *image,
                    GType      container_type,
                    GType      item_type)
{
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (container_type, CLAY_TYPE_ITEM_STACK), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, CLAY_TYPE_ITEM), NULL);

  return g_object_new (CLAY_TYPE_ITEM_TREE,
                       "image",          image,
                       "container-type", container_type,
                       "item-type",      item_type,
                       NULL);
}

ClayItem *
clay_item_tree_get_active_item (ClayItemTree *tree)
{
  GList *items;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), NULL);

  items = CLAY_ITEM_TREE_GET_PRIVATE (tree)->selected_items;
  if (g_list_length (items) == 1)
    return items->data;
  else
    return NULL;
}

void
clay_item_tree_set_active_item (ClayItemTree *tree,
                                ClayItem     *item)
{
  clay_item_tree_set_selected_items (tree,
                                     g_list_prepend (NULL, item));
}

GList *
clay_item_tree_get_selected_items (ClayItemTree *tree)
{
  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), NULL);

  return CLAY_ITEM_TREE_GET_PRIVATE (tree)->selected_items;
}

/**
 * clay_item_tree_set_selected_items:
 * @tree:
 * @items: (transfer container):
 *
 * Sets the list of selected items. @tree takes ownership of @items
 * container (not the #ClayItem themselves).
 */
void
clay_item_tree_set_selected_items (ClayItemTree *tree,
                                   GList        *items)
{
  ClayItemTreePrivate *private;
  GList               *iter;
  gboolean             selection_changed = TRUE;
  gint                 prev_selected_count;
  gint                 selected_count;

  g_return_if_fail (CLAY_IS_ITEM_TREE (tree));

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  for (iter = items; iter; iter = iter->next)
    {
      g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (iter->data, private->item_type));
      g_return_if_fail (clay_item_get_tree (iter->data) == tree);
    }

  prev_selected_count = g_list_length (private->selected_items);
  selected_count      = g_list_length (items);

  if (selected_count == prev_selected_count)
    {
      selection_changed = FALSE;
      for (iter = items; iter; iter = iter->next)
        {
          if (g_list_find (private->selected_items, iter->data) == NULL)
            {
              selection_changed = TRUE;
              break;
            }
        }
    }

  if (selection_changed)
    {
      if (private->selected_items)
        g_list_free (private->selected_items);

      private->selected_items = items;
      g_object_notify (G_OBJECT (tree), "selected-items");

      /* XXX: if we add back a proper concept of active item (not just
       * meaning selection of 1), we may also notify of active item
       * update.
       */
      /*g_object_notify (G_OBJECT (tree), "active-item");*/
    }
  else if (items != private->selected_items)
    {
      g_list_free (items);
    }
}

ClayItem *
clay_item_tree_get_item_by_name (ClayItemTree *tree,
                                 const gchar  *name)
{
  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  return g_hash_table_lookup (CLAY_ITEM_TREE_GET_PRIVATE (tree)->name_hash,
                              name);
}

gboolean
clay_item_tree_get_insert_pos (ClayItemTree  *tree,
                               ClayItem      *item,
                               ClayItem     **parent,
                               gint          *position)
{
  ClayItemTreePrivate *private;
  ClayContainer       *container;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), FALSE);
  g_return_val_if_fail (parent != NULL, FALSE);

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        FALSE);
  g_return_val_if_fail (! clay_item_is_attached (item), FALSE);
  g_return_val_if_fail (clay_item_get_image (item) == private->image, FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == CLAY_IMAGE_ACTIVE_PARENT ||
                        G_TYPE_CHECK_INSTANCE_TYPE (*parent, private->item_type),
                        FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == CLAY_IMAGE_ACTIVE_PARENT ||
                        clay_item_get_tree (*parent) == tree, FALSE);
  g_return_val_if_fail (*parent == NULL ||
                        *parent == CLAY_IMAGE_ACTIVE_PARENT ||
                        clay_viewable_get_children (CLAY_VIEWABLE (*parent)),
                        FALSE);
  g_return_val_if_fail (position != NULL, FALSE);

  /*  if we want to insert in the active item's parent container  */
  if (*parent == CLAY_IMAGE_ACTIVE_PARENT)
    {
      GList    *iter;
      ClayItem *selected_parent;

      *parent = NULL;
      for (iter = private->selected_items; iter; iter = iter->next)
        {
          /*  if the selected item is a branch, add to the top of that
           *  branch; add to the selected item's parent container
           *  otherwise
           */
          if (clay_viewable_get_children (CLAY_VIEWABLE (iter->data)))
            {
              selected_parent = iter->data;
              *position       = 0;
            }
          else
            {
              selected_parent = clay_item_get_parent (iter->data);
            }

          /* Only allow if all selected items have the same parent.
           * If hierarchy is different, use the toplevel container
           * (same if there are no selected items).
           */
          if (iter != private->selected_items && *parent != selected_parent)
            {
              *parent = NULL;
              break;
            }
          else
            {
              *parent = selected_parent;
            }
        }
    }

  if (*parent)
    container = clay_viewable_get_children (CLAY_VIEWABLE (*parent));
  else
    container = tree->container;

  /* We want to add on top of the selected items. */
  if (*position == -1)
    {
      GList    *iter;
      gint      selected_position;

      for (iter = private->selected_items; iter; iter = iter->next)
        {
          selected_position =
            clay_container_get_child_index (container, iter->data);

          /* If one the selected items is not in the specified parent
           * container, fall back to index 0.
           */
          if (selected_position == -1)
            {
              *position = 0;
              break;
            }

          if (*position == -1)
            *position = selected_position;
          else
            /* Higher selected items has the lower index. */
            *position = MIN (*position, selected_position);
        }
    }

  /*  don't add at a non-existing index  */
  *position = CLAMP (*position, 0, clay_container_get_n_children (container));

  return TRUE;
}

void
clay_item_tree_add_item (ClayItemTree *tree,
                         ClayItem     *item,
                         ClayItem     *parent,
                         gint          position)
{
  ClayItemTreePrivate *private;
  ClayContainer       *container;
  ClayContainer       *children;

  g_return_if_fail (CLAY_IS_ITEM_TREE (tree));

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type));
  g_return_if_fail (! clay_item_is_attached (item));
  g_return_if_fail (clay_item_get_image (item) == private->image);
  g_return_if_fail (parent == NULL ||
                    G_TYPE_CHECK_INSTANCE_TYPE (parent, private->item_type));
  g_return_if_fail (parent == NULL || clay_item_get_tree (parent) == tree);
  g_return_if_fail (parent == NULL ||
                    clay_viewable_get_children (CLAY_VIEWABLE (parent)));

  clay_item_tree_uniquefy_name (tree, item, NULL);

  children = clay_viewable_get_children (CLAY_VIEWABLE (item));

  if (children)
    {
      GList *list = clay_item_stack_get_item_list (CLAY_ITEM_STACK (children));

      while (list)
        {
          clay_item_tree_uniquefy_name (tree, list->data, NULL);

          list = g_list_remove (list, list->data);
        }
    }

  if (parent)
    container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
  else
    container = tree->container;

  if (parent)
    clay_viewable_set_parent (CLAY_VIEWABLE (item),
                              CLAY_VIEWABLE (parent));

  clay_container_insert (container, CLAY_OBJECT (item), position);

  /*  if the item came from the undo stack, reset its "removed" state  */
  if (clay_item_is_removed (item))
    clay_item_unset_removed (item);
}

GList *
clay_item_tree_remove_item (ClayItemTree *tree,
                            ClayItem     *item,
                            GList        *new_selected)
{
  ClayItemTreePrivate *private;
  ClayItem            *parent;
  ClayContainer       *container;
  ClayContainer       *children;
  gint                 index;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), NULL);

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        NULL);
  g_return_val_if_fail (clay_item_get_tree (item) == tree, NULL);

  parent    = clay_item_get_parent (item);
  container = clay_item_get_container (item);
  index     = clay_item_get_index (item);

  g_object_ref (item);

  g_hash_table_remove (private->name_hash,
                       clay_object_get_name (item));

  children = clay_viewable_get_children (CLAY_VIEWABLE (item));

  if (children)
    {
      GList *list = clay_item_stack_get_item_list (CLAY_ITEM_STACK (children));

      while (list)
        {
          g_hash_table_remove (private->name_hash,
                               clay_object_get_name (list->data));

          list = g_list_remove (list, list->data);
        }
    }

  clay_container_remove (container, CLAY_OBJECT (item));

  if (parent)
    clay_viewable_set_parent (CLAY_VIEWABLE (item), NULL);

  clay_item_removed (item);

  if (! new_selected)
    {
      ClayItem *selected   = NULL;
      gint      n_children = clay_container_get_n_children (container);

      if (n_children > 0)
        {
          index = CLAMP (index, 0, n_children - 1);

          selected =
            CLAY_ITEM (clay_container_get_child_by_index (container, index));
        }
      else if (parent)
        {
          selected = parent;
        }

      if (selected)
        new_selected = g_list_prepend (NULL, selected);
    }
  else
    {
      new_selected = g_list_copy (new_selected);
    }

  g_object_unref (item);

  return new_selected;
}

gboolean
clay_item_tree_reorder_item (ClayItemTree *tree,
                             ClayItem     *item,
                             ClayItem     *new_parent,
                             gint          new_index,
                             gboolean      push_undo,
                             const gchar  *undo_desc)
{
  ClayItemTreePrivate *private;
  ClayContainer       *container;
  ClayContainer       *new_container;
  gint                 n_items;

  g_return_val_if_fail (CLAY_IS_ITEM_TREE (tree), FALSE);

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_return_val_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type),
                        FALSE);
  g_return_val_if_fail (clay_item_get_tree (item) == tree, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        G_TYPE_CHECK_INSTANCE_TYPE (new_parent,
                                                    private->item_type),
                        FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        clay_item_get_tree (new_parent) == tree, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        clay_viewable_get_children (CLAY_VIEWABLE (new_parent)),
                        FALSE);
  g_return_val_if_fail (item != new_parent, FALSE);
  g_return_val_if_fail (new_parent == NULL ||
                        ! clay_viewable_is_ancestor (CLAY_VIEWABLE (item),
                                                     CLAY_VIEWABLE (new_parent)),
                        FALSE);

  container = clay_item_get_container (item);

  if (new_parent)
    new_container = clay_viewable_get_children (CLAY_VIEWABLE (new_parent));
  else
    new_container = tree->container;

  n_items = clay_container_get_n_children (new_container);

  if (new_container == container)
    n_items--;

  new_index = CLAMP (new_index, 0, n_items);

  if (new_container != container ||
      new_index     != clay_item_get_index (item))
    {
      GList *selected_items = g_list_copy (private->selected_items);
      if (push_undo)
        clay_image_undo_push_item_reorder (private->image, undo_desc, item);

      if (new_container != container)
        {
          g_object_ref (item);

          clay_container_remove (container, CLAY_OBJECT (item));

          clay_viewable_set_parent (CLAY_VIEWABLE (item),
                                    CLAY_VIEWABLE (new_parent));

          clay_container_insert (new_container, CLAY_OBJECT (item), new_index);

          g_object_unref (item);
        }
      else
        {
          clay_container_reorder (container, CLAY_OBJECT (item), new_index);
        }
      /* After reorder, selection is likely lost. We must recreate it as
       * it was.
       */
      clay_item_tree_set_selected_items (tree, selected_items);
    }

  return TRUE;
}

void
clay_item_tree_rename_item (ClayItemTree *tree,
                            ClayItem     *item,
                            const gchar  *new_name,
                            gboolean      push_undo,
                            const gchar  *undo_desc)
{
  ClayItemTreePrivate *private;

  g_return_if_fail (CLAY_IS_ITEM_TREE (tree));

  private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  g_return_if_fail (G_TYPE_CHECK_INSTANCE_TYPE (item, private->item_type));
  g_return_if_fail (clay_item_get_tree (item) == tree);
  g_return_if_fail (new_name != NULL);

  if (strcmp (new_name, clay_object_get_name (item)))
    {
      if (push_undo)
        clay_image_undo_push_item_rename (clay_item_get_image (item),
                                          undo_desc, item);

      clay_item_tree_uniquefy_name (tree, item, new_name);
    }
}


/*  private functions  */

static void
clay_item_tree_uniquefy_name (ClayItemTree *tree,
                              ClayItem     *item,
                              const gchar  *new_name)
{
  ClayItemTreePrivate *private = CLAY_ITEM_TREE_GET_PRIVATE (tree);

  if (new_name)
    {
      g_hash_table_remove (private->name_hash,
                           clay_object_get_name (item));

      clay_object_set_name (CLAY_OBJECT (item), new_name);
    }

  /* Remove any trailing whitespace. */
  if (clay_object_get_name (item))
    {
      gchar *name = g_strchomp (g_strdup (clay_object_get_name (item)));

      clay_object_take_name (CLAY_OBJECT (item), name);
    }

  if (g_hash_table_lookup (private->name_hash,
                           clay_object_get_name (item)))
    {
      gchar      *name        = g_strdup (clay_object_get_name (item));
      gchar      *new_name    = NULL;
      gint        number      = 0;
      gint        precision   = 1;
      GRegex     *end_numbers = g_regex_new (" ?#([0-9]+)\\s*$", 0, 0, NULL);
      GMatchInfo *match_info  = NULL;

      if (g_regex_match (end_numbers, name, 0, &match_info))
        {
          gchar *match;
          gint   start_pos;

          match  = g_match_info_fetch (match_info, 1);
          if (match && match[0] == '0')
            {
              precision = strlen (match);
            }
          number = atoi (match);
          g_free (match);

          g_match_info_fetch_pos (match_info, 0,
                                  &start_pos, NULL);
          name[start_pos] = '\0';
        }
      g_match_info_free (match_info);
      g_regex_unref (end_numbers);

      do
        {
          number++;

          g_free (new_name);

          new_name = g_strdup_printf ("%s #%.*d",
                                      name,
                                      precision,
                                      number);
        }
      while (g_hash_table_lookup (private->name_hash, new_name));

      g_free (name);

      clay_object_take_name (CLAY_OBJECT (item), new_name);
    }

  g_hash_table_insert (private->name_hash,
                       (gpointer) clay_object_get_name (item),
                       item);
}
