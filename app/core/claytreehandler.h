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

#pragma once

#include "core/clayobject.h"


#define CLAY_TYPE_TREE_HANDLER            (clay_tree_handler_get_type ())
#define CLAY_TREE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TREE_HANDLER, ClayTreeHandler))
#define CLAY_TREE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TREE_HANDLER, ClayTreeHandlerClass))
#define CLAY_IS_TREE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TREE_HANDLER))
#define CLAY_IS_TREE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TREE_HANDLER))
#define CLAY_TREE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TREE_HANDLER, ClayTreeHandlerClass))


typedef struct _ClayTreeHandlerClass ClayTreeHandlerClass;

struct _ClayTreeHandler
{
  ClayObject     parent_instance;

  ClayContainer *container;

  gchar         *signal_name;
  GCallback      callback;
  gpointer       user_data;
};

struct _ClayTreeHandlerClass
{
  ClayObjectClass  parent_class;
};


GType             clay_tree_handler_get_type   (void);

ClayTreeHandler * clay_tree_handler_connect    (ClayContainer   *container,
                                                const gchar     *signal_name,
                                                GCallback        callback,
                                                gpointer         user_data);
void              clay_tree_handler_disconnect (ClayTreeHandler *handler);
