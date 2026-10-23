/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayitemstack.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "clayitem.h"
#include "clayitemstack.h"


/*  local function prototypes  */

static void   clay_item_stack_constructed (GObject       *object);

static void   clay_item_stack_add         (ClayContainer *container,
                                           ClayObject    *object);
static void   clay_item_stack_remove      (ClayContainer *container,
                                           ClayObject    *object);


G_DEFINE_TYPE (ClayItemStack, clay_item_stack, CLAY_TYPE_FILTER_STACK)

#define parent_class clay_item_stack_parent_class


static void
clay_item_stack_class_init (ClayItemStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerClass *container_class = CLAY_CONTAINER_CLASS (klass);

  object_class->constructed = clay_item_stack_constructed;

  container_class->add      = clay_item_stack_add;
  container_class->remove   = clay_item_stack_remove;
}

static void
clay_item_stack_init (ClayItemStack *stack)
{
}

static void
clay_item_stack_constructed (GObject *object)
{
  ClayContainer *container = CLAY_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (g_type_is_a (clay_container_get_child_type (container),
                         CLAY_TYPE_ITEM));
}

static void
clay_item_stack_add (ClayContainer *container,
                     ClayObject    *object)
{
  g_object_ref_sink (object);

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);

  g_object_unref (object);
}

static void
clay_item_stack_remove (ClayContainer *container,
                        ClayObject    *object)
{
  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);
}


/*  public functions  */

ClayContainer *
clay_item_stack_new (GType item_type)
{
  g_return_val_if_fail (g_type_is_a (item_type, CLAY_TYPE_ITEM), NULL);

  return g_object_new (CLAY_TYPE_ITEM_STACK,
                       "name",       g_type_name (item_type),
                       "child-type", item_type,
                       "policy",     CLAY_CONTAINER_POLICY_STRONG,
                       NULL);
}

gint
clay_item_stack_get_n_items (ClayItemStack *stack)
{
  GList *list;
  gint   n_items = 0;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), 0);

  for (list = CLAY_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      ClayItem      *item = list->data;
      ClayContainer *children;

      n_items++;

      children = clay_viewable_get_children (CLAY_VIEWABLE (item));

      if (children)
        n_items += clay_item_stack_get_n_items (CLAY_ITEM_STACK (children));
    }

  return n_items;
}

gboolean
clay_item_stack_is_flat (ClayItemStack *stack)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), TRUE);

  for (list = CLAY_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      ClayViewable *viewable = list->data;

      if (clay_viewable_get_children (viewable))
        return FALSE;
    }

  return TRUE;
}

GList *
clay_item_stack_get_item_iter (ClayItemStack *stack)
{
  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), NULL);

  return CLAY_LIST (stack)->queue->head;
}

GList *
clay_item_stack_get_item_list (ClayItemStack *stack)
{
  GList *list;
  GList *result = NULL;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), NULL);

  for (list = CLAY_LIST (stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayViewable  *viewable = list->data;
      ClayContainer *children;

      result = g_list_prepend (result, viewable);

      children = clay_viewable_get_children (viewable);

      if (children)
        {
          GList *child_list;

          child_list = clay_item_stack_get_item_list (CLAY_ITEM_STACK (children));

          while (child_list)
            {
              result = g_list_prepend (result, child_list->data);

              child_list = g_list_remove (child_list, child_list->data);
            }
        }
    }

  return g_list_reverse (result);
}

ClayItem *
clay_item_stack_get_item_by_tattoo (ClayItemStack *stack,
                                    ClayTattoo     tattoo)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), NULL);

  for (list = CLAY_LIST (stack)->queue->head; list; list = g_list_next (list))
    {
      ClayItem      *item = list->data;
      ClayContainer *children;

      if (clay_item_get_tattoo (item) == tattoo)
        return item;

      children = clay_viewable_get_children (CLAY_VIEWABLE (item));

      if (children)
        {
          item = clay_item_stack_get_item_by_tattoo (CLAY_ITEM_STACK (children),
                                                     tattoo);

          if (item)
            return item;
        }
    }

  return NULL;
}

ClayItem *
clay_item_stack_get_item_by_path (ClayItemStack *stack,
                                  GList         *path)
{
  ClayContainer *container;
  ClayItem      *item = NULL;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  container = CLAY_CONTAINER (stack);

  while (path)
    {
      guint32 i = GPOINTER_TO_UINT (path->data);

      item = CLAY_ITEM (clay_container_get_child_by_index (container, i));

      g_return_val_if_fail (CLAY_IS_ITEM (item), item);

      if (path->next)
        {
          container = clay_viewable_get_children (CLAY_VIEWABLE (item));

          g_return_val_if_fail (CLAY_IS_ITEM_STACK (container), item);
        }

      path = path->next;
    }

  return item;
}

ClayItem *
clay_item_stack_get_parent_by_path (ClayItemStack *stack,
                                    GList         *path,
                                    gint          *index)
{
  ClayItem *parent = NULL;
  guint32   i;

  g_return_val_if_fail (CLAY_IS_ITEM_STACK (stack), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  i = GPOINTER_TO_UINT (path->data);

  if (index)
    *index = i;

  while (path->next)
    {
      ClayObject    *child;
      ClayContainer *children;

      child = clay_container_get_child_by_index (CLAY_CONTAINER (stack), i);

      g_return_val_if_fail (CLAY_IS_ITEM (child), parent);

      children = clay_viewable_get_children (CLAY_VIEWABLE (child));

      g_return_val_if_fail (CLAY_IS_ITEM_STACK (children), parent);

      parent = CLAY_ITEM (child);
      stack  = CLAY_ITEM_STACK (children);

      path = path->next;

      i = GPOINTER_TO_UINT (path->data);

      if (index)
        *index = i;
    }

  return parent;
}

static void
clay_item_stack_viewable_invalidate_previews (ClayViewable *viewable)
{
  ClayContainer *children = clay_viewable_get_children (viewable);

  if (children)
    clay_item_stack_invalidate_previews (CLAY_ITEM_STACK (children));

  clay_viewable_invalidate_preview (viewable);
}

void
clay_item_stack_invalidate_previews (ClayItemStack *stack)
{
  g_return_if_fail (CLAY_IS_ITEM_STACK (stack));

  clay_container_foreach (CLAY_CONTAINER (stack),
                          (GFunc) clay_item_stack_viewable_invalidate_previews,
                          NULL);
}

static void
clay_item_stack_viewable_profile_changed (ClayViewable *viewable)
{
  ClayContainer *children = clay_viewable_get_children (viewable);

  if (children)
    clay_item_stack_profile_changed (CLAY_ITEM_STACK (children));

  if (CLAY_IS_COLOR_MANAGED (viewable))
    clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (viewable));
}

void
clay_item_stack_profile_changed (ClayItemStack *stack)
{
  g_return_if_fail (CLAY_IS_ITEM_STACK (stack));

  clay_container_foreach (CLAY_CONTAINER (stack),
                          (GFunc) clay_item_stack_viewable_profile_changed,
                          NULL);
}
