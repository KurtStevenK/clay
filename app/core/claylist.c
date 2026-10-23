/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claylist.c
 * Copyright (C) 2001-2016 Michael Natterer <mitch@gimp.org>
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
#include <string.h> /* strcmp */

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay-memsize.h"
#include "claylist.h"


enum
{
  PROP_0,
  PROP_UNIQUE_NAMES,
  PROP_SORT_FUNC,
  PROP_APPEND
};


static void         clay_list_finalize           (GObject                 *object);
static void         clay_list_set_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  const GValue            *value,
                                                  GParamSpec              *pspec);
static void         clay_list_get_property       (GObject                 *object,
                                                  guint                    property_id,
                                                  GValue                  *value,
                                                  GParamSpec              *pspec);

static gint64       clay_list_get_memsize        (ClayObject              *object,
                                                  gint64                  *gui_size);

static void         clay_list_add                (ClayContainer           *container,
                                                  ClayObject              *object);
static void         clay_list_remove             (ClayContainer           *container,
                                                  ClayObject              *object);
static void         clay_list_reorder            (ClayContainer           *container,
                                                  ClayObject              *object,
                                                  gint                     old_index,
                                                  gint                     new_index);
static void         clay_list_clear              (ClayContainer           *container);
static gboolean     clay_list_have               (ClayContainer           *container,
                                                  ClayObject              *object);
static void         clay_list_foreach            (ClayContainer           *container,
                                                  GFunc                    func,
                                                  gpointer                 user_data);
static ClayObject * clay_list_search             (ClayContainer           *container,
                                                  ClayContainerSearchFunc  func,
                                                  gpointer                 user_data);
static gboolean     clay_list_get_unique_names   (ClayContainer           *container);
static GList    * clay_list_get_children_by_name (ClayContainer           *container,
                                                  const gchar             *name);
static ClayObject * clay_list_get_child_by_name  (ClayContainer           *container,
                                                  const gchar             *name);
static ClayObject * clay_list_get_child_by_index (ClayContainer           *container,
                                                  gint                     index);
static gint         clay_list_get_child_index    (ClayContainer           *container,
                                                  ClayObject              *object);

static void         clay_list_uniquefy_name      (ClayList                *clay_list,
                                                  ClayObject              *object);
static void         clay_list_object_renamed     (ClayObject              *object,
                                                  ClayList                *list);


G_DEFINE_TYPE (ClayList, clay_list, CLAY_TYPE_CONTAINER)

#define parent_class clay_list_parent_class


static void
clay_list_class_init (ClayListClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass    *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayContainerClass *container_class   = CLAY_CONTAINER_CLASS (klass);

  object_class->finalize                = clay_list_finalize;
  object_class->set_property            = clay_list_set_property;
  object_class->get_property            = clay_list_get_property;

  clay_object_class->get_memsize        = clay_list_get_memsize;

  container_class->add                  = clay_list_add;
  container_class->remove               = clay_list_remove;
  container_class->reorder              = clay_list_reorder;
  container_class->clear                = clay_list_clear;
  container_class->have                 = clay_list_have;
  container_class->foreach              = clay_list_foreach;
  container_class->search               = clay_list_search;
  container_class->get_unique_names     = clay_list_get_unique_names;
  container_class->get_children_by_name = clay_list_get_children_by_name;
  container_class->get_child_by_name    = clay_list_get_child_by_name;
  container_class->get_child_by_index   = clay_list_get_child_by_index;
  container_class->get_child_index      = clay_list_get_child_index;

  g_object_class_install_property (object_class, PROP_UNIQUE_NAMES,
                                   g_param_spec_boolean ("unique-names",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SORT_FUNC,
                                   g_param_spec_pointer ("sort-func",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_APPEND,
                                   g_param_spec_boolean ("append",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
clay_list_init (ClayList *list)
{
  list->queue        = g_queue_new ();
  list->unique_names = FALSE;
  list->sort_func    = NULL;
  list->append       = FALSE;
}

static void
clay_list_finalize (GObject *object)
{
  ClayList *list = CLAY_LIST (object);

  if (list->queue)
    {
      g_queue_free (list->queue);
      list->queue = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_list_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayList *list = CLAY_LIST (object);

  switch (property_id)
    {
    case PROP_UNIQUE_NAMES:
      list->unique_names = g_value_get_boolean (value);
      break;
    case PROP_SORT_FUNC:
      clay_list_set_sort_func (list, g_value_get_pointer (value));
      break;
    case PROP_APPEND:
      list->append = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_list_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  ClayList *list = CLAY_LIST (object);

  switch (property_id)
    {
    case PROP_UNIQUE_NAMES:
      g_value_set_boolean (value, list->unique_names);
      break;
    case PROP_SORT_FUNC:
      g_value_set_pointer (value, list->sort_func);
      break;
    case PROP_APPEND:
      g_value_set_boolean (value, list->append);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_list_get_memsize (ClayObject *object,
                       gint64     *gui_size)
{
  ClayList *list    = CLAY_LIST (object);
  gint64    memsize = 0;

  if (clay_container_get_policy (CLAY_CONTAINER (list)) ==
      CLAY_CONTAINER_POLICY_STRONG)
    {
      memsize += clay_g_queue_get_memsize_foreach (list->queue,
                                                   (ClayMemsizeFunc) clay_object_get_memsize,
                                                   gui_size);
    }
  else
    {
      memsize += clay_g_queue_get_memsize (list->queue, 0);
    }

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gint
clay_list_sort_func (gconstpointer a,
                     gconstpointer b,
                     gpointer      user_data)
{
  GCompareFunc func = user_data;

  return func (a, b);
}

static void
clay_list_add (ClayContainer *container,
               ClayObject    *object)
{
  ClayList *list = CLAY_LIST (container);

  if (list->unique_names)
    clay_list_uniquefy_name (list, object);

  if (list->unique_names || list->sort_func)
    g_signal_connect (object, "name-changed",
                      G_CALLBACK (clay_list_object_renamed),
                      list);

  if (list->sort_func)
    {
      g_queue_insert_sorted (list->queue, object, clay_list_sort_func,
                             list->sort_func);
    }
  else if (list->append)
    {
      g_queue_push_tail (list->queue, object);
    }
  else
    {
      g_queue_push_head (list->queue, object);
    }

  CLAY_CONTAINER_CLASS (parent_class)->add (container, object);
}

static void
clay_list_remove (ClayContainer *container,
                  ClayObject    *object)
{
  ClayList *list = CLAY_LIST (container);

  if (list->unique_names || list->sort_func)
    g_signal_handlers_disconnect_by_func (object,
                                          clay_list_object_renamed,
                                          list);

  g_queue_remove (list->queue, object);

  CLAY_CONTAINER_CLASS (parent_class)->remove (container, object);
}

static void
clay_list_reorder (ClayContainer *container,
                   ClayObject    *object,
                   gint           old_index,
                   gint           new_index)
{
  ClayList *list = CLAY_LIST (container);

  g_queue_remove (list->queue, object);

  if (new_index == clay_container_get_n_children (container) - 1)
    g_queue_push_tail (list->queue, object);
  else
    g_queue_push_nth (list->queue, object, new_index);

  CLAY_CONTAINER_CLASS (parent_class)->reorder (container, object,
                                                old_index, new_index);
}

static void
clay_list_clear (ClayContainer *container)
{
  ClayList *list = CLAY_LIST (container);

  while (g_queue_peek_head (list->queue))
    clay_container_remove (container, g_queue_peek_head (list->queue));
}

static gboolean
clay_list_have (ClayContainer *container,
                ClayObject    *object)
{
  ClayList *list = CLAY_LIST (container);

  return g_queue_find (list->queue, object) ? TRUE : FALSE;
}

static void
clay_list_foreach (ClayContainer *container,
                   GFunc          func,
                   gpointer       user_data)
{
  ClayList *list = CLAY_LIST (container);

  g_queue_foreach (list->queue, func, user_data);
}

static ClayObject *
clay_list_search (ClayContainer           *container,
                  ClayContainerSearchFunc  func,
                  gpointer                 user_data)
{
  ClayList *list = CLAY_LIST (container);
  GList    *iter;

  iter = list->queue->head;

  while (iter)
    {
      ClayObject *object = iter->data;

      iter = g_list_next (iter);

      if (func (object, user_data))
        return object;
    }

  return NULL;
}

static gboolean
clay_list_get_unique_names (ClayContainer *container)
{
  ClayList *list = CLAY_LIST (container);

  return list->unique_names;
}

static GList *
clay_list_get_children_by_name (ClayContainer *container,
                                const gchar   *name)
{
  ClayList *list     = CLAY_LIST (container);
  GList    *children = NULL;
  GList    *iter;

  for (iter = list->queue->head; iter; iter = g_list_next (iter))
    {
      ClayObject *object = iter->data;

      if (! strcmp (clay_object_get_name (object), name))
        {
          children = g_list_prepend (children, object);
          if (list->unique_names)
            return children;
        }
    }

  return children;
}

static ClayObject *
clay_list_get_child_by_name (ClayContainer *container,
                             const gchar   *name)
{
  ClayList *list = CLAY_LIST (container);
  GList    *glist;

  for (glist = list->queue->head; glist; glist = g_list_next (glist))
    {
      ClayObject *object = glist->data;

      if (! strcmp (clay_object_get_name (object), name))
        return object;
    }

  return NULL;
}

static ClayObject *
clay_list_get_child_by_index (ClayContainer *container,
                              gint           index)
{
  ClayList *list = CLAY_LIST (container);

  return g_queue_peek_nth (list->queue, index);
}

static gint
clay_list_get_child_index (ClayContainer *container,
                           ClayObject    *object)
{
  ClayList *list = CLAY_LIST (container);

  return g_queue_index (list->queue, (gpointer) object);
}

/**
 * clay_list_new:
 * @child_type:   the #GType of objects the list is going to hold
 * @unique_names: if the list should ensure that all its children
 *                have unique names.
 *
 * Creates a new #ClayList object. Since #ClayList is a #ClayContainer
 * implementation, it holds ClayObjects. Thus @child_type must be
 * CLAY_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #CLAY_CONTAINER_POLICY_STRONG.
 *
 * Returns: a new #ClayList object
 **/
ClayContainer *
clay_list_new (GType    child_type,
               gboolean unique_names)
{
  ClayList *list;

  g_return_val_if_fail (g_type_is_a (child_type, CLAY_TYPE_OBJECT), NULL);

  list = g_object_new (CLAY_TYPE_LIST,
                       "child-type",   child_type,
                       "policy",       CLAY_CONTAINER_POLICY_STRONG,
                       "unique-names", unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  clay_object_set_static_name (CLAY_OBJECT (list), g_type_name (child_type));

  return CLAY_CONTAINER (list);
}

/**
 * clay_list_new_weak:
 * @child_type:   the #GType of objects the list is going to hold
 * @unique_names: if the list should ensure that all its children
 *                have unique names.
 *
 * Creates a new #ClayList object. Since #ClayList is a #ClayContainer
 * implementation, it holds ClayObjects. Thus @child_type must be
 * CLAY_TYPE_OBJECT or a type derived from it.
 *
 * The returned list has the #CLAY_CONTAINER_POLICY_WEAK.
 *
 * Returns: a new #ClayList object
 **/
ClayContainer *
clay_list_new_weak (GType    child_type,
                    gboolean unique_names)
{
  ClayList *list;

  g_return_val_if_fail (g_type_is_a (child_type, CLAY_TYPE_OBJECT), NULL);

  list = g_object_new (CLAY_TYPE_LIST,
                       "child-type",   child_type,
                       "policy",       CLAY_CONTAINER_POLICY_WEAK,
                       "unique-names", unique_names ? TRUE : FALSE,
                       NULL);

  /* for debugging purposes only */
  clay_object_set_static_name (CLAY_OBJECT (list), g_type_name (child_type));

  return CLAY_CONTAINER (list);
}

/**
 * clay_list_reverse:
 * @list: a #ClayList
 *
 * Reverses the order of elements in a #ClayList.
 **/
void
clay_list_reverse (ClayList *list)
{
  g_return_if_fail (CLAY_IS_LIST (list));

  if (clay_container_get_n_children (CLAY_CONTAINER (list)) > 1)
    {
      clay_container_freeze (CLAY_CONTAINER (list));
      g_queue_reverse (list->queue);
      clay_container_thaw (CLAY_CONTAINER (list));
    }
}

/**
 * clay_list_set_sort_func:
 * @list:      a #ClayList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of @list using clay_list_sort() and remembers the
 * passed @sort_func in order to keep the list ordered across inserting
 * or renaming children.
 **/
void
clay_list_set_sort_func (ClayList     *list,
                         GCompareFunc  sort_func)
{
  g_return_if_fail (CLAY_IS_LIST (list));

  if (sort_func != list->sort_func)
    {
      if (sort_func)
        clay_list_sort (list, sort_func);

      list->sort_func = sort_func;
      g_object_notify (G_OBJECT (list), "sort-func");
    }
}

/**
 * clay_list_get_sort_func:
 * @list: a #ClayList
 *
 * Returns the @list's sort function, see clay_list_set_sort_func().
 *
 * Returns: The @list's sort function.
 **/
GCompareFunc
clay_list_get_sort_func (ClayList *list)
{
  g_return_val_if_fail (CLAY_IS_LIST (list), NULL);

  return list->sort_func;
}

/**
 * clay_list_sort:
 * @list: a #ClayList
 * @sort_func: a #GCompareFunc
 *
 * Sorts the elements of a #ClayList according to the given @sort_func.
 * See g_list_sort() for a detailed description of this function.
 **/
void
clay_list_sort (ClayList     *list,
                GCompareFunc  sort_func)
{
  g_return_if_fail (CLAY_IS_LIST (list));
  g_return_if_fail (sort_func != NULL);

  if (clay_container_get_n_children (CLAY_CONTAINER (list)) > 1)
    {
      clay_container_freeze (CLAY_CONTAINER (list));
      g_queue_sort (list->queue, clay_list_sort_func, sort_func);
      clay_container_thaw (CLAY_CONTAINER (list));
    }
}

/**
 * clay_list_sort_by_name:
 * @list: a #ClayList
 *
 * Sorts the #ClayObject elements of a #ClayList by their names.
 **/
void
clay_list_sort_by_name (ClayList *list)
{
  g_return_if_fail (CLAY_IS_LIST (list));

  clay_list_sort (list, (GCompareFunc) clay_object_name_collate);
}


/*  private functions  */

static void
clay_list_uniquefy_name (ClayList   *clay_list,
                         ClayObject *object)
{
  gchar *name = (gchar *) clay_object_get_name (object);
  GList *list;

  if (! name)
    return;

  for (list = clay_list->queue->head; list; list = g_list_next (list))
    {
      ClayObject  *object2 = list->data;
      const gchar *name2   = clay_object_get_name (object2);

      if (object != object2 &&
          name2             &&
          ! strcmp (name, name2))
        break;
    }

  if (list)
    {
      gchar *ext;
      gchar *new_name   = NULL;
      gint   unique_ext = 0;

      name = g_strdup (name);

      ext = strrchr (name, '#');

      if (ext)
        {
          gchar ext_str[8];

          unique_ext = atoi (ext + 1);

          g_snprintf (ext_str, sizeof (ext_str), "%d", unique_ext);

          /*  check if the extension really is of the form "#<n>"  */
          if (! strcmp (ext_str, ext + 1))
            {
              if (ext > name && *(ext - 1) == ' ')
                ext--;

              *ext = '\0';
            }
          else
            {
              unique_ext = 0;
            }
        }

      do
        {
          unique_ext++;

          g_free (new_name);

          new_name = g_strdup_printf ("%s #%d", name, unique_ext);

          for (list = clay_list->queue->head; list; list = g_list_next (list))
            {
              ClayObject  *object2 = list->data;
              const gchar *name2   = clay_object_get_name (object2);

              if (object != object2 &&
                  name2             &&
                  ! strcmp (new_name, name2))
                break;
            }
        }
      while (list);

      g_free (name);

      clay_object_take_name (object, new_name);
    }
}

static void
clay_list_object_renamed (ClayObject *object,
                          ClayList   *list)
{
  if (list->unique_names)
    {
      g_signal_handlers_block_by_func (object,
                                       clay_list_object_renamed,
                                       list);

      clay_list_uniquefy_name (list, object);

      g_signal_handlers_unblock_by_func (object,
                                         clay_list_object_renamed,
                                         list);
    }

  if (list->sort_func)
    {
      GList *glist;
      gint   old_index;
      gint   new_index = 0;

      old_index = g_queue_index (list->queue, object);

      for (glist = list->queue->head; glist; glist = g_list_next (glist))
        {
          ClayObject *object2 = CLAY_OBJECT (glist->data);

          if (object == object2)
            continue;

          if (list->sort_func (object, object2) > 0)
            new_index++;
          else
            break;
        }

      if (new_index != old_index)
        clay_container_reorder (CLAY_CONTAINER (list), object, new_index);
    }
}
