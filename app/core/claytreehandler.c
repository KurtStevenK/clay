/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTreeHandler
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include "claycontainer.h"
#include "claytreehandler.h"
#include "clayviewable.h"


static void   clay_tree_handler_dispose          (GObject         *object);

static void   clay_tree_handler_freeze           (ClayTreeHandler *handler,
                                                  ClayContainer   *container);
static void   clay_tree_handler_thaw             (ClayTreeHandler *handler,
                                                  ClayContainer   *container);

static void   clay_tree_handler_add_container    (ClayTreeHandler *handler,
                                                  ClayContainer   *container);
static void   clay_tree_handler_add_foreach      (ClayViewable    *viewable,
                                                  ClayTreeHandler *handler);
static void   clay_tree_handler_add              (ClayTreeHandler *handler,
                                                  ClayViewable    *viewable,
                                                  ClayContainer   *container);

static void   clay_tree_handler_remove_container (ClayTreeHandler *handler,
                                                  ClayContainer   *container);
static void   clay_tree_handler_remove_foreach   (ClayViewable    *viewable,
                                                  ClayTreeHandler *handler);
static void   clay_tree_handler_remove           (ClayTreeHandler *handler,
                                                  ClayViewable    *viewable,
                                                  ClayContainer   *container);


G_DEFINE_TYPE (ClayTreeHandler, clay_tree_handler, CLAY_TYPE_OBJECT)

#define parent_class clay_tree_handler_parent_class


static void
clay_tree_handler_class_init (ClayTreeHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_tree_handler_dispose;
}

static void
clay_tree_handler_init (ClayTreeHandler *handler)
{
}

static void
clay_tree_handler_dispose (GObject *object)
{
  ClayTreeHandler *handler = CLAY_TREE_HANDLER (object);

  if (handler->container)
    {
      g_signal_handlers_disconnect_by_func (handler->container,
                                            clay_tree_handler_freeze,
                                            handler);
      g_signal_handlers_disconnect_by_func (handler->container,
                                            clay_tree_handler_thaw,
                                            handler);

      if (! clay_container_frozen (handler->container))
        clay_tree_handler_remove_container (handler, handler->container);

      g_clear_object (&handler->container);
      g_clear_pointer (&handler->signal_name, g_free);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

ClayTreeHandler *
clay_tree_handler_connect (ClayContainer *container,
                           const gchar   *signal_name,
                           GCallback      callback,
                           gpointer       user_data)
{
  ClayTreeHandler *handler;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (signal_name != NULL, NULL);

  handler = g_object_new (CLAY_TYPE_TREE_HANDLER, NULL);

  handler->container   = g_object_ref (container);
  handler->signal_name = g_strdup (signal_name);
  handler->callback    = callback;
  handler->user_data   = user_data;

  if (! clay_container_frozen (container))
    clay_tree_handler_add_container (handler, container);

  g_signal_connect_object (container, "freeze",
                           G_CALLBACK (clay_tree_handler_freeze),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (clay_tree_handler_thaw),
                           handler,
                           G_CONNECT_SWAPPED);

  return handler;
}

void
clay_tree_handler_disconnect (ClayTreeHandler *handler)
{
  g_return_if_fail (CLAY_IS_TREE_HANDLER (handler));

  g_object_run_dispose (G_OBJECT (handler));
  g_object_unref (handler);
}


/*  private functions  */

static void
clay_tree_handler_freeze (ClayTreeHandler *handler,
                          ClayContainer   *container)
{
  clay_tree_handler_remove_container (handler, container);
}

static void
clay_tree_handler_thaw (ClayTreeHandler *handler,
                        ClayContainer   *container)
{
  clay_tree_handler_add_container (handler, container);
}

static void
clay_tree_handler_add_container (ClayTreeHandler *handler,
                                 ClayContainer   *container)
{
  clay_container_foreach (container,
                          (GFunc) clay_tree_handler_add_foreach,
                          handler);

  g_signal_connect_object (container, "add",
                           G_CALLBACK (clay_tree_handler_add),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (clay_tree_handler_remove),
                           handler,
                           G_CONNECT_SWAPPED);
}

static void
clay_tree_handler_add_foreach (ClayViewable    *viewable,
                               ClayTreeHandler *handler)
{
  clay_tree_handler_add (handler, viewable, NULL);
}

static void
clay_tree_handler_add (ClayTreeHandler *handler,
                       ClayViewable    *viewable,
                       ClayContainer   *unused)
{
  ClayContainer *children = clay_viewable_get_children (viewable);

  g_signal_connect (viewable,
                    handler->signal_name,
                    handler->callback,
                    handler->user_data);

  if (children)
    clay_tree_handler_add_container (handler, children);
}

static void
clay_tree_handler_remove_container (ClayTreeHandler *handler,
                                    ClayContainer   *container)
{
  g_signal_handlers_disconnect_by_func (container,
                                        clay_tree_handler_add,
                                        handler);
  g_signal_handlers_disconnect_by_func (container,
                                        clay_tree_handler_remove,
                                        handler);

  clay_container_foreach (container,
                          (GFunc) clay_tree_handler_remove_foreach,
                          handler);
}

static void
clay_tree_handler_remove_foreach (ClayViewable    *viewable,
                                  ClayTreeHandler *handler)
{
  clay_tree_handler_remove (handler, viewable, NULL);
}

static void
clay_tree_handler_remove (ClayTreeHandler *handler,
                          ClayViewable    *viewable,
                          ClayContainer   *unused)
{
  ClayContainer *children = clay_viewable_get_children (viewable);

  if (children)
    clay_tree_handler_remove_container (handler, children);

  g_signal_handlers_disconnect_by_func (viewable,
                                        handler->callback,
                                        handler->user_data);
}
