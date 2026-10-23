/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claydrawablestack.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "claydrawable.h"
#include "claydrawablestack.h"
#include "claymarshal.h"


enum
{
  UPDATE,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void   clay_drawable_stack_constructed      (GObject           *object);

static void   clay_drawable_stack_add              (ClayContainer     *container,
                                                    ClayObject        *object);
static void   clay_drawable_stack_remove           (ClayContainer     *container,
                                                    ClayObject        *object);
static void   clay_drawable_stack_reorder          (ClayContainer     *container,
                                                    ClayObject        *object,
                                                    gint               old_index,
                                                    gint               new_index);

static void   clay_drawable_stack_drawable_update  (ClayItem          *item,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height,
                                                    ClayDrawableStack *stack);
static void   clay_drawable_stack_drawable_active  (ClayItem          *item,
                                                    ClayDrawableStack *stack);


G_DEFINE_TYPE (ClayDrawableStack, clay_drawable_stack, CLAY_TYPE_ITEM_STACK)

#define parent_class clay_drawable_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
clay_drawable_stack_class_init (ClayDrawableStackClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayContainerClass *container_class = CLAY_CONTAINER_CLASS (klass);

  stack_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableStackClass, update),
                  NULL, NULL,
                  clay_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->constructed = clay_drawable_stack_constructed;

  container_class->add      = clay_drawable_stack_add;
  container_class->remove   = clay_drawable_stack_remove;
  container_class->reorder  = clay_drawable_stack_reorder;
}

static void
clay_drawable_stack_init (ClayDrawableStack *stack)
{
}

static void
clay_drawable_stack_constructed (GObject *object)
{
  ClayContainer *container = CLAY_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (g_type_is_a (clay_container_get_child_type (container),
                            CLAY_TYPE_DRAWABLE));

  clay_container_add_handler (container, "update",
                              G_CALLBACK (clay_drawable_stack_drawable_update),
                              container);
  clay_container_add_handler (container, "active-changed",
                              G_CALLBACK (clay_drawable_stack_drawable_active),
                              container);
}

static void
clay_drawable_stack_add (ClayContainer *container,
                         ClayObject    *object)
{
  ClayDrawableStack *stack = CLAY_DRAWABLE_STACK (container);

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);

  if (clay_filter_get_active (CLAY_FILTER (object)))
    clay_drawable_stack_drawable_active (CLAY_ITEM (object), stack);
}

static void
clay_drawable_stack_remove (ClayContainer *container,
                            ClayObject    *object)
{
  ClayDrawableStack *stack = CLAY_DRAWABLE_STACK (container);

  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);

  if (clay_filter_get_active (CLAY_FILTER (object)))
    clay_drawable_stack_drawable_active (CLAY_ITEM (object), stack);
}

static void
clay_drawable_stack_reorder (ClayContainer *container,
                             ClayObject    *object,
                             gint           old_index,
                             gint           new_index)
{
  ClayDrawableStack *stack  = CLAY_DRAWABLE_STACK (container);

  CLAY_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);

  if (clay_filter_get_active (CLAY_FILTER (object)))
    clay_drawable_stack_drawable_active (CLAY_ITEM (object), stack);
}


/*  public functions  */

ClayContainer *
clay_drawable_stack_new (GType drawable_type)
{
  g_return_val_if_fail (g_type_is_a (drawable_type, CLAY_TYPE_DRAWABLE), NULL);

  return g_object_new (CLAY_TYPE_DRAWABLE_STACK,
                       "name",       g_type_name (drawable_type),
                       "child-type", drawable_type,
                       "policy",     CLAY_CONTAINER_POLICY_STRONG,
                       NULL);
}


/*  protected functions  */

void
clay_drawable_stack_update (ClayDrawableStack *stack,
                            gint               x,
                            gint               y,
                            gint               width,
                            gint               height)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_STACK (stack));

  g_signal_emit (stack, stack_signals[UPDATE], 0,
                 x, y, width, height);
}


/*  private functions  */

static void
clay_drawable_stack_drawable_update (ClayItem          *item,
                                     gint               x,
                                     gint               y,
                                     gint               width,
                                     gint               height,
                                     ClayDrawableStack *stack)
{
  if (clay_filter_get_active (CLAY_FILTER (item)))
    {
      gint offset_x;
      gint offset_y;

      clay_item_get_offset (item, &offset_x, &offset_y);

      clay_drawable_stack_update (stack,
                                  x + offset_x, y + offset_y,
                                  width, height);
    }
}

static void
clay_drawable_stack_drawable_active (ClayItem          *item,
                                     ClayDrawableStack *stack)
{
  GeglRectangle bounding_box;

  bounding_box = clay_drawable_get_bounding_box (CLAY_DRAWABLE (item));

  bounding_box.x += clay_item_get_offset_x (item);
  bounding_box.y += clay_item_get_offset_y (item);

  clay_drawable_stack_update (stack,
                              bounding_box.x,     bounding_box.y,
                              bounding_box.width, bounding_box.height);
}
