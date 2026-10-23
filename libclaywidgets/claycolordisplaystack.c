/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolordisplaystack.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claycolordisplay.h"
#include "claycolordisplaystack.h"
#include "claywidgetsmarshal.h"


/**
 * SECTION: claycolordisplaystack
 * @title: ClayColorDisplayStack
 * @short_description: A stack of color correction modules.
 * @see_also: #ClayColorDisplay
 *
 * A stack of color correction modules.
 **/


enum
{
  CHANGED,
  ADDED,
  REMOVED,
  REORDERED,
  LAST_SIGNAL
};


struct _ClayColorDisplayStack
{
  GObject  parent;

  GList   *filters;
};


static void   clay_color_display_stack_dispose         (GObject               *object);

static void   clay_color_display_stack_list_model_iface_init (GListModelInterface *iface);

static void   clay_color_display_stack_display_changed (ClayColorDisplay      *display,
                                                        ClayColorDisplayStack *stack);
static void   clay_color_display_stack_display_enabled (ClayColorDisplay      *display,
                                                        GParamSpec            *pspec,
                                                        ClayColorDisplayStack *stack);
static void   clay_color_display_stack_disconnect      (ClayColorDisplayStack *stack,
                                                        ClayColorDisplay      *display);


G_DEFINE_TYPE_WITH_CODE (ClayColorDisplayStack, clay_color_display_stack, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL,
                                                clay_color_display_stack_list_model_iface_init))

#define parent_class clay_color_display_stack_parent_class

static guint stack_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_display_stack_class_init (ClayColorDisplayStackClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  stack_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  stack_signals[ADDED] =
    g_signal_new ("added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  _clay_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  stack_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_COLOR_DISPLAY);

  stack_signals[REORDERED] =
    g_signal_new ("reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  _clay_widgets_marshal_VOID__OBJECT_INT,
                  G_TYPE_NONE, 2,
                  CLAY_TYPE_COLOR_DISPLAY,
                  G_TYPE_INT);

  object_class->dispose = clay_color_display_stack_dispose;
}

static void
clay_color_display_stack_init (ClayColorDisplayStack *stack)
{
}

static void
clay_color_display_stack_dispose (GObject *object)
{
  ClayColorDisplayStack *stack = CLAY_COLOR_DISPLAY_STACK (object);

  if (stack->filters)
    {
      GList *list;

      for (list = stack->filters; list; list = g_list_next (list))
        {
          ClayColorDisplay *display = list->data;

          clay_color_display_stack_disconnect (stack, display);
          g_object_unref (display);
        }

      g_list_free (stack->filters);
      stack->filters = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static GType
clay_color_display_stack_get_item_type (GListModel *list)
{
  return CLAY_TYPE_COLOR_DISPLAY;
}

static guint
clay_color_display_stack_get_n_items (GListModel *list)
{
  ClayColorDisplayStack *stack = CLAY_COLOR_DISPLAY_STACK (list);

  return g_list_length (stack->filters);
}

static void *
clay_color_display_stack_get_item (GListModel *list,
                                   guint       index)
{
  ClayColorDisplayStack *stack = CLAY_COLOR_DISPLAY_STACK (list);
  ClayColorDisplay      *filter;

  filter = g_list_nth_data (stack->filters, index);

  if (filter)
    g_object_ref (filter);

  return filter;
}

static void
clay_color_display_stack_list_model_iface_init (GListModelInterface *iface)
{
  iface->get_item_type = clay_color_display_stack_get_item_type;
  iface->get_n_items   = clay_color_display_stack_get_n_items;
  iface->get_item      = clay_color_display_stack_get_item;
}


/*  public functions  */

/**
 * clay_color_display_stack_new:
 *
 * Creates a new stack of color correction modules.
 *
 * Returns: (transfer full): a newly allocated #ClayColorDisplayStack.
 *
 * Since: 2.0
 **/
ClayColorDisplayStack *
clay_color_display_stack_new (void)
{
  return g_object_new (CLAY_TYPE_COLOR_DISPLAY_STACK, NULL);
}

/**
 * clay_color_display_stack_clone:
 * @stack: a #ClayColorDisplayStack
 *
 * Creates a copy of @stack with all its display color modules also
 * duplicated.
 *
 * Returns: (transfer full): a duplicate of @stack.
 *
 * Since: 2.0
 **/
ClayColorDisplayStack *
clay_color_display_stack_clone (ClayColorDisplayStack *stack)
{
  ClayColorDisplayStack *clone;
  GList                 *list;

  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack), NULL);

  clone = g_object_new (CLAY_TYPE_COLOR_DISPLAY_STACK, NULL);

  for (list = stack->filters; list; list = g_list_next (list))
    {
      ClayColorDisplay *display;

      display = clay_color_display_clone (list->data);

      clay_color_display_stack_add (clone, display);
      g_object_unref (display);
    }

  return clone;
}

/**
 * clay_color_display_stack_changed:
 * @stack: a #ClayColorDisplayStack
 *
 * Emit the "changed" signal of @stack.
 *
 * Since: 2.0
 **/
void
clay_color_display_stack_changed (ClayColorDisplayStack *stack)
{
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));

  g_signal_emit (stack, stack_signals[CHANGED], 0);
}

/**
 * clay_color_display_stack_get_filters:
 * @stack: a #ClayColorDisplayStack
 *
 * Gets the list of added color modules.
 *
 * Returns: (transfer none) (element-type ClayColorDisplay):
            the list of @stack's display color modules.
 *
 * Since: 3.0
 **/
GList *
clay_color_display_stack_get_filters (ClayColorDisplayStack *stack)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack), NULL);

  return stack->filters;
}

/**
 * clay_color_display_stack_add:
 * @stack:   a #ClayColorDisplayStack
 * @display: (transfer none): a #ClayColorDisplay
 *
 * Add the color module @display to @stack.
 *
 * Since: 2.0
 **/
void
clay_color_display_stack_add (ClayColorDisplayStack *stack,
                              ClayColorDisplay      *display)
{
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  g_return_if_fail (g_list_find (stack->filters, display) == NULL);

  stack->filters = g_list_append (stack->filters, g_object_ref (display));

  g_signal_connect (display, "changed",
                    G_CALLBACK (clay_color_display_stack_display_changed),
                    G_OBJECT (stack));
  g_signal_connect (display, "notify::enabled",
                    G_CALLBACK (clay_color_display_stack_display_enabled),
                    G_OBJECT (stack));

  g_list_model_items_changed (G_LIST_MODEL (stack),
                              g_list_length (stack->filters) - 1, 0, 1);

  g_signal_emit (stack, stack_signals[ADDED], 0,
                 display, g_list_length (stack->filters) - 1);

  clay_color_display_stack_changed (stack);
}

/**
 * clay_color_display_stack_remove:
 * @stack:   a #ClayColorDisplayStack
 * @display: (transfer none): a #ClayColorDisplay
 *
 * Remove the color module @display from @stack.
 *
 * Since: 2.0
 **/
void
clay_color_display_stack_remove (ClayColorDisplayStack *stack,
                                 ClayColorDisplay      *display)
{
  gint index;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  g_return_if_fail (g_list_find (stack->filters, display) != NULL);

  clay_color_display_stack_disconnect (stack, display);

  index = g_list_index (stack->filters, display);

  stack->filters = g_list_remove (stack->filters, display);

  g_list_model_items_changed (G_LIST_MODEL (stack), index, 1, 0);

  g_signal_emit (stack, stack_signals[REMOVED], 0, display);

  clay_color_display_stack_changed (stack);

  g_object_unref (display);
}

/**
 * clay_color_display_stack_reorder_up:
 * @stack:   a #ClayColorDisplayStack
 * @display: a #ClayColorDisplay
 *
 * Move the color module @display up in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
clay_color_display_stack_reorder_up (ClayColorDisplayStack *stack,
                                     ClayColorDisplay      *display)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  list = g_list_find (stack->filters, display);

  g_return_if_fail (list != NULL);

  if (list->prev)
    {
      gint index;

      list->data       = list->prev->data;
      list->prev->data = display;

      index = g_list_index (stack->filters, display);

      g_list_model_items_changed (G_LIST_MODEL (stack), index, 2, 2);

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (stack->filters, list->prev));

      clay_color_display_stack_changed (stack);
    }
}

/**
 * clay_color_display_stack_reorder_down:
 * @stack:   a #ClayColorDisplayStack
 * @display: a #ClayColorDisplay
 *
 * Move the color module @display down in the filter list of @stack.
 *
 * Since: 2.0
 **/
void
clay_color_display_stack_reorder_down (ClayColorDisplayStack *stack,
                                       ClayColorDisplay      *display)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  list = g_list_find (stack->filters, display);

  g_return_if_fail (list != NULL);

  if (list->next)
    {
      gint index;

      index = g_list_index (stack->filters, display);

      list->data       = list->next->data;
      list->next->data = display;

      g_list_model_items_changed (G_LIST_MODEL (stack), index, 2, 2);

      g_signal_emit (stack, stack_signals[REORDERED], 0,
                     display, g_list_position (stack->filters, list->next));

      clay_color_display_stack_changed (stack);
    }
}

/**
 * clay_color_display_stack_convert_buffer:
 * @stack:  a #ClayColorDisplayStack
 * @buffer: a #GeglBuffer
 * @area:   area of @buffer to convert
 *
 * Runs all the stack's filters on all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
clay_color_display_stack_convert_buffer (ClayColorDisplayStack *stack,
                                         GeglBuffer            *buffer,
                                         GeglRectangle         *area)
{
  GList *list;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY_STACK (stack));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  for (list = stack->filters; list; list = g_list_next (list))
    {
      ClayColorDisplay *display = list->data;

      clay_color_display_convert_buffer (display, buffer, area);
    }
}


/*  private functions  */

static void
clay_color_display_stack_display_changed (ClayColorDisplay      *display,
                                          ClayColorDisplayStack *stack)
{
  if (clay_color_display_get_enabled (display))
    clay_color_display_stack_changed (stack);
}

static void
clay_color_display_stack_display_enabled (ClayColorDisplay      *display,
                                          GParamSpec            *pspec,
                                          ClayColorDisplayStack *stack)
{
  clay_color_display_stack_changed (stack);
}

static void
clay_color_display_stack_disconnect (ClayColorDisplayStack *stack,
                                     ClayColorDisplay      *display)
{
  g_signal_handlers_disconnect_by_func (display,
                                        clay_color_display_stack_display_changed,
                                        stack);
  g_signal_handlers_disconnect_by_func (display,
                                        clay_color_display_stack_display_enabled,
                                        stack);
}
