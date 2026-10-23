/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayfilterstack.c
 * Copyright (C) 2008-2013 Michael Natterer <mitch@gimp.org>
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

#include "core-types.h"

#include "clayfilter.h"
#include "clayfilterstack.h"


/*  local function prototypes  */

static void   clay_filter_stack_constructed      (GObject         *object);
static void   clay_filter_stack_finalize         (GObject         *object);

static void   clay_filter_stack_add              (ClayContainer   *container,
                                                  ClayObject      *object);
static void   clay_filter_stack_remove           (ClayContainer   *container,
                                                  ClayObject      *object);
static void   clay_filter_stack_reorder          (ClayContainer   *container,
                                                  ClayObject      *object,
                                                  gint             old_index,
                                                  gint             new_index);

static void   clay_filter_stack_add_node         (ClayFilterStack *stack,
                                                  ClayFilter      *filter);
static void   clay_filter_stack_remove_node      (ClayFilterStack *stack,
                                                  ClayFilter      *filter);
static void   clay_filter_stack_update_last_node (ClayFilterStack *stack);

static void   clay_filter_stack_filter_active    (ClayFilter      *filter,
                                                  ClayFilterStack *stack);


G_DEFINE_TYPE (ClayFilterStack, clay_filter_stack, CLAY_TYPE_LIST);

#define parent_class clay_filter_stack_parent_class


static void
clay_filter_stack_class_init (ClayFilterStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerClass *container_class = CLAY_CONTAINER_CLASS (klass);

  object_class->constructed = clay_filter_stack_constructed;
  object_class->finalize    = clay_filter_stack_finalize;

  container_class->add      = clay_filter_stack_add;
  container_class->remove   = clay_filter_stack_remove;
  container_class->reorder  = clay_filter_stack_reorder;
}

static void
clay_filter_stack_init (ClayFilterStack *stack)
{
}

static void
clay_filter_stack_constructed (GObject *object)
{
  ClayContainer *container = CLAY_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (g_type_is_a (clay_container_get_child_type (container),
                            CLAY_TYPE_FILTER));

  clay_container_add_handler (container, "active-changed",
                              G_CALLBACK (clay_filter_stack_filter_active),
                              container);
}

static void
clay_filter_stack_finalize (GObject *object)
{
  ClayFilterStack *stack = CLAY_FILTER_STACK (object);

  g_clear_object (&stack->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_filter_stack_add (ClayContainer *container,
                       ClayObject    *object)
{
  ClayFilterStack *stack  = CLAY_FILTER_STACK (container);
  ClayFilter      *filter = CLAY_FILTER (object);

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);

  if (clay_filter_get_active (filter))
    {
      if (stack->graph)
        {
          gegl_node_add_child (stack->graph, clay_filter_get_node (filter));
          clay_filter_stack_add_node (stack, filter);
        }

      clay_filter_stack_update_last_node (stack);
    }
}

static void
clay_filter_stack_remove (ClayContainer *container,
                          ClayObject    *object)
{
  ClayFilterStack *stack  = CLAY_FILTER_STACK (container);
  ClayFilter      *filter = CLAY_FILTER (object);

  if (stack->graph && clay_filter_get_active (filter))
    {
      clay_filter_stack_remove_node (stack, filter);
      gegl_node_remove_child (stack->graph, clay_filter_get_node (filter));
    }

  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (clay_filter_get_active (filter))
    {
      clay_filter_set_is_last_node (filter, FALSE);
      clay_filter_stack_update_last_node (stack);
    }
}

static void
clay_filter_stack_reorder (ClayContainer *container,
                           ClayObject    *object,
                           gint           old_index,
                           gint           new_index)
{
  ClayFilterStack *stack  = CLAY_FILTER_STACK (container);
  ClayFilter      *filter = CLAY_FILTER (object);

  if (stack->graph && clay_filter_get_active (filter))
    clay_filter_stack_remove_node (stack, filter);

  CLAY_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);

  if (clay_filter_get_active (filter))
    {
      clay_filter_stack_update_last_node (stack);

      if (stack->graph)
        clay_filter_stack_add_node (stack, filter);
    }
}


/*  public functions  */

ClayContainer *
clay_filter_stack_new (GType filter_type)
{
  g_return_val_if_fail (g_type_is_a (filter_type, CLAY_TYPE_FILTER), NULL);

  return g_object_new (CLAY_TYPE_FILTER_STACK,
                       "name",       g_type_name (filter_type),
                       "child-type", filter_type,
                       "policy",     CLAY_CONTAINER_POLICY_STRONG,
                       NULL);
}

GeglNode *
clay_filter_stack_get_graph (ClayFilterStack *stack)
{
  GList    *list;
  GeglNode *previous;
  GeglNode *output;

  g_return_val_if_fail (CLAY_IS_FILTER_STACK (stack), NULL);

  if (stack->graph)
    return stack->graph;

  stack->graph = gegl_node_new ();

  previous = gegl_node_get_input_proxy (stack->graph, "input");

  for (list = CLAY_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      ClayFilter *filter = list->data;
      GeglNode   *node;

      if (! clay_filter_get_active (filter))
        continue;

      node = clay_filter_get_node (filter);

      gegl_node_add_child (stack->graph, node);

      gegl_node_link (previous, node);

      previous = node;
    }

  output = gegl_node_get_output_proxy (stack->graph, "output");

  gegl_node_link (previous, output);

  return stack->graph;
}


/*  private functions  */

static void
clay_filter_stack_add_node (ClayFilterStack *stack,
                            ClayFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = clay_filter_get_node (filter);


  iter = g_list_find (CLAY_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      ClayFilter *filter_above = iter->data;

      if (clay_filter_get_active (filter_above))
        {
          node_above = clay_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node_above, "input", NULL);

  gegl_node_link (node_below, node);
  gegl_node_link (node, node_above);
}

static void
clay_filter_stack_remove_node (ClayFilterStack *stack,
                               ClayFilter      *filter)
{
  GeglNode *node;
  GeglNode *node_above = NULL;
  GeglNode *node_below = NULL;
  GList    *iter;

  node = clay_filter_get_node (filter);

  iter = g_list_find (CLAY_LIST (stack)->queue->head, filter);

  while ((iter = g_list_previous (iter)))
    {
      ClayFilter *filter_above = iter->data;

      if (clay_filter_get_active (filter_above))
        {
          node_above = clay_filter_get_node (filter_above);

          break;
        }
    }

  if (! node_above)
    node_above = gegl_node_get_output_proxy (stack->graph, "output");

  node_below = gegl_node_get_producer (node, "input", NULL);

  gegl_node_disconnect (node, "input");

  gegl_node_link (node_below, node_above);
}

static void
clay_filter_stack_update_last_node (ClayFilterStack *stack)
{
  GList    *list;
  gboolean  found_last = FALSE;

  for (list = CLAY_LIST (stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      ClayFilter *filter = list->data;

      if (! found_last && clay_filter_get_active (filter))
        {
          clay_filter_set_is_last_node (filter, TRUE);
          found_last = TRUE;
        }
      else
        {
          clay_filter_set_is_last_node (filter, FALSE);
        }
    }
}

static void
clay_filter_stack_filter_active (ClayFilter      *filter,
                                 ClayFilterStack *stack)
{
  if (stack->graph)
    {
      if (clay_filter_get_active (filter))
        {
          gegl_node_add_child (stack->graph, clay_filter_get_node (filter));
          clay_filter_stack_add_node (stack, filter);
        }
      else
        {
          clay_filter_stack_remove_node (stack, filter);
          gegl_node_remove_child (stack->graph, clay_filter_get_node (filter));
        }
    }

  clay_filter_stack_update_last_node (stack);

  if (! clay_filter_get_active (filter))
    clay_filter_set_is_last_node (filter, FALSE);
}
