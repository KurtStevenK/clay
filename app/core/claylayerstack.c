/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claylayerstack.c
 * Copyright (C) 2017 Ell
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

#include "claylayer.h"
#include "claylayerstack.h"


/*  local function prototypes  */

static void   clay_layer_stack_constructed             (GObject       *object);

static void   clay_layer_stack_add                     (ClayContainer *container,
                                                        ClayObject    *object);
static void   clay_layer_stack_remove                  (ClayContainer *container,
                                                        ClayObject    *object);
static void   clay_layer_stack_reorder                 (ClayContainer *container,
                                                        ClayObject    *object,
                                                        gint           old_index,
                                                        gint           new_index);

static void   clay_layer_stack_layer_active            (ClayLayer      *layer,
                                                        ClayLayerStack *stack);
static void   clay_layer_stack_layer_excludes_backdrop (ClayLayer      *layer,
                                                        ClayLayerStack *stack);

static void   clay_layer_stack_update_backdrop         (ClayLayerStack *stack,
                                                        ClayLayer      *layer,
                                                        gboolean        ignore_active,
                                                        gboolean        ignore_excludes_backdrop);
static void   clay_layer_stack_update_range            (ClayLayerStack *stack,
                                                        gint            first,
                                                        gint            last);


G_DEFINE_TYPE (ClayLayerStack, clay_layer_stack, CLAY_TYPE_DRAWABLE_STACK)

#define parent_class clay_layer_stack_parent_class


static void
clay_layer_stack_class_init (ClayLayerStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerClass *container_class = CLAY_CONTAINER_CLASS (klass);

  object_class->constructed = clay_layer_stack_constructed;

  container_class->add      = clay_layer_stack_add;
  container_class->remove   = clay_layer_stack_remove;
  container_class->reorder  = clay_layer_stack_reorder;
}

static void
clay_layer_stack_init (ClayLayerStack *stack)
{
}

static void
clay_layer_stack_constructed (GObject *object)
{
  ClayContainer *container = CLAY_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (g_type_is_a (clay_container_get_child_type (container),
                            CLAY_TYPE_LAYER));

  clay_container_add_handler (container, "active-changed",
                              G_CALLBACK (clay_layer_stack_layer_active),
                              container);
  clay_container_add_handler (container, "excludes-backdrop-changed",
                              G_CALLBACK (clay_layer_stack_layer_excludes_backdrop),
                              container);
}

static void
clay_layer_stack_add (ClayContainer *container,
                      ClayObject    *object)
{
  ClayLayerStack *stack = CLAY_LAYER_STACK (container);

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);

  clay_layer_stack_update_backdrop (stack, CLAY_LAYER (object), FALSE, FALSE);
}

static void
clay_layer_stack_remove (ClayContainer *container,
                         ClayObject    *object)
{
  ClayLayerStack *stack = CLAY_LAYER_STACK (container);
  gboolean        update_backdrop;
  gint            index;

  update_backdrop = clay_filter_get_active (CLAY_FILTER (object)) &&
                    clay_layer_get_excludes_backdrop (CLAY_LAYER (object));

  if (update_backdrop)
    index = clay_container_get_child_index (container, object);

  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (update_backdrop)
    clay_layer_stack_update_range (stack, index, -1);
}

static void
clay_layer_stack_reorder (ClayContainer *container,
                          ClayObject    *object,
                          gint           old_index,
                          gint           new_index)
{
  ClayLayerStack *stack = CLAY_LAYER_STACK (container);
  gboolean        update_backdrop;

  update_backdrop = clay_filter_get_active (CLAY_FILTER (object)) &&
                    clay_layer_get_excludes_backdrop (CLAY_LAYER (object));

  CLAY_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);

  if (update_backdrop)
    clay_layer_stack_update_range (stack, old_index, new_index);
}


/*  public functions  */

ClayContainer *
clay_layer_stack_new (GType layer_type)
{
  g_return_val_if_fail (g_type_is_a (layer_type, CLAY_TYPE_LAYER), NULL);

  return g_object_new (CLAY_TYPE_LAYER_STACK,
                       "name",       g_type_name (layer_type),
                       "child-type", layer_type,
                       "policy",     CLAY_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  private functions  */

static void
clay_layer_stack_layer_active (ClayLayer      *layer,
                               ClayLayerStack *stack)
{
  clay_layer_stack_update_backdrop (stack, layer, TRUE, FALSE);
}

static void
clay_layer_stack_layer_excludes_backdrop (ClayLayer      *layer,
                                          ClayLayerStack *stack)
{
  clay_layer_stack_update_backdrop (stack, layer, FALSE, TRUE);
}

static void
clay_layer_stack_update_backdrop (ClayLayerStack *stack,
                                  ClayLayer      *layer,
                                  gboolean        ignore_active,
                                  gboolean        ignore_excludes_backdrop)
{
  if ((ignore_active            || clay_filter_get_active (CLAY_FILTER (layer))) &&
      (ignore_excludes_backdrop || clay_layer_get_excludes_backdrop (layer)))
    {
      gint index;

      index = clay_container_get_child_index (CLAY_CONTAINER (stack),
                                              CLAY_OBJECT (layer));

      clay_layer_stack_update_range (stack, index + 1, -1);
    }
}

static void
clay_layer_stack_update_range (ClayLayerStack *stack,
                               gint            first,
                               gint            last)
{
  GList *iter;

  g_return_if_fail (first >= 0 && last >= -1);

  /* if the range is reversed, flip first and last; note that last == -1 is
   * used to update all layers from first onward.
   */
  if (last >= 0 && last < first)
    {
      gint temp = first;

      first = last + 1;
      last  = temp + 1;
    }

  iter = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (stack));

  for (iter = g_list_nth (iter, first);
       iter && first != last;
       iter = g_list_next (iter), first++)
    {
      ClayItem *item = iter->data;

      if (clay_filter_get_active (CLAY_FILTER (item)))
        {
          GeglRectangle bounding_box;

          bounding_box = clay_drawable_get_bounding_box (CLAY_DRAWABLE (item));

          bounding_box.x += clay_item_get_offset_x (item);
          bounding_box.y += clay_item_get_offset_y (item);

          clay_drawable_stack_update (CLAY_DRAWABLE_STACK (stack),
                                      bounding_box.x,     bounding_box.y,
                                      bounding_box.width, bounding_box.height);
        }
    }
}
