/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claycontainer.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "clayobject.h"


#define CLAY_TYPE_CONTAINER            (clay_container_get_type ())
#define CLAY_CONTAINER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER, ClayContainer))
#define CLAY_CONTAINER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER, ClayContainerClass))
#define CLAY_IS_CONTAINER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER))
#define CLAY_IS_CONTAINER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER))
#define CLAY_CONTAINER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER, ClayContainerClass))


typedef gboolean (* ClayContainerSearchFunc) (ClayObject *object,
                                              gpointer    user_data);


typedef struct _ClayContainerClass   ClayContainerClass;
typedef struct _ClayContainerPrivate ClayContainerPrivate;

struct _ClayContainer
{
  ClayObject            parent_instance;

  ClayContainerPrivate *priv;
};

struct _ClayContainerClass
{
  ClayObjectClass  parent_class;

  /*  signals  */
  void         (* add)                (ClayContainer           *container,
                                       ClayObject              *object);
  void         (* remove)             (ClayContainer           *container,
                                       ClayObject              *object);
  void         (* reorder)            (ClayContainer           *container,
                                       ClayObject              *object,
                                       gint                     old_index,
                                       gint                     new_index);
  void         (* freeze)             (ClayContainer           *container);
  void         (* thaw)               (ClayContainer           *container);

  /*  virtual functions  */
  void         (* clear)              (ClayContainer           *container);
  gboolean     (* have)               (ClayContainer           *container,
                                       ClayObject              *object);
  void         (* foreach)            (ClayContainer           *container,
                                       GFunc                    func,
                                       gpointer                 user_data);
  ClayObject * (* search)             (ClayContainer           *container,
                                       ClayContainerSearchFunc  func,
                                       gpointer                 user_data);
  gboolean     (* get_unique_names)   (ClayContainer           *container);
  ClayObject * (* get_child_by_name)  (ClayContainer           *container,
                                       const gchar             *name);
  GList    * (* get_children_by_name) (ClayContainer           *container,
                                       const gchar             *name);
  ClayObject * (* get_child_by_index) (ClayContainer           *container,
                                       gint                     index);
  gint         (* get_child_index)    (ClayContainer           *container,
                                       ClayObject              *object);
};


GType        clay_container_get_type           (void);

GType        clay_container_get_child_type     (ClayContainer           *container);
ClayContainerPolicy clay_container_get_policy  (ClayContainer           *container);
gint         clay_container_get_n_children     (ClayContainer           *container);

gboolean     clay_container_add                (ClayContainer           *container,
                                                ClayObject              *object);
gboolean     clay_container_remove             (ClayContainer           *container,
                                                ClayObject              *object);
gboolean     clay_container_insert             (ClayContainer           *container,
                                                ClayObject              *object,
                                                gint                     new_index);
gboolean     clay_container_reorder            (ClayContainer           *container,
                                                ClayObject              *object,
                                                gint                     new_index);

void         clay_container_freeze             (ClayContainer           *container);
void         clay_container_thaw               (ClayContainer           *container);
gboolean     clay_container_frozen             (ClayContainer           *container);
gint         clay_container_freeze_count       (ClayContainer           *container);

void         clay_container_clear              (ClayContainer           *container);
gboolean     clay_container_is_empty           (ClayContainer           *container);
gboolean     clay_container_have               (ClayContainer           *container,
                                                ClayObject              *object);
void         clay_container_foreach            (ClayContainer           *container,
                                                GFunc                    func,
                                                gpointer                 user_data);
ClayObject * clay_container_search             (ClayContainer           *container,
                                                ClayContainerSearchFunc  func,
                                                gpointer                 user_data);

gboolean     clay_container_get_unique_names   (ClayContainer           *container);

GList    * clay_container_get_children_by_name (ClayContainer           *container,
                                                const gchar             *name);
ClayObject * clay_container_get_child_by_name  (ClayContainer           *container,
                                                const gchar             *name);
ClayObject * clay_container_get_child_by_index (ClayContainer           *container,
                                                gint                     index);
ClayObject * clay_container_get_first_child    (ClayContainer           *container);
ClayObject * clay_container_get_last_child     (ClayContainer           *container);
gint         clay_container_get_child_index    (ClayContainer           *container,
                                                ClayObject              *object);

ClayObject * clay_container_get_neighbor_of    (ClayContainer           *container,
                                                ClayObject              *object);

gchar     ** clay_container_get_name_array     (ClayContainer           *container);

GQuark       clay_container_add_handler        (ClayContainer           *container,
                                                const gchar             *signame,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         clay_container_remove_handler     (ClayContainer           *container,
                                                GQuark                   id);
void         clay_container_remove_handlers_by_func
                                               (ClayContainer           *container,
                                                GCallback                callback,
                                                gpointer                 callback_data);
void         clay_container_remove_handlers_by_data
                                               (ClayContainer           *container,
                                                gpointer                 callback_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayContainer, g_object_unref);
