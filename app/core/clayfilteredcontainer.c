/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayfilteredcontainer.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
 *               2011 Michael Natterer <mitch@gimp.org>
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

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clayfilteredcontainer.h"


enum
{
  PROP_0,
  PROP_SRC_CONTAINER,
  PROP_FILTER_FUNC,
  PROP_FILTER_DATA
};


static void     clay_filtered_container_constructed     (GObject               *object);
static void     clay_filtered_container_dispose         (GObject               *object);
static void     clay_filtered_container_finalize        (GObject               *object);
static void     clay_filtered_container_set_property    (GObject               *object,
                                                         guint                  property_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static void     clay_filtered_container_get_property    (GObject               *object,
                                                         guint                  property_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);

static void     clay_filtered_container_real_src_add    (ClayFilteredContainer *filtered_container,
                                                         ClayObject            *object);
static void     clay_filtered_container_real_src_remove (ClayFilteredContainer *filtered_container,
                                                         ClayObject            *object);
static void     clay_filtered_container_real_src_freeze (ClayFilteredContainer *filtered_container);
static void     clay_filtered_container_real_src_thaw   (ClayFilteredContainer *filtered_container);

static gboolean clay_filtered_container_object_matches  (ClayFilteredContainer *filtered_container,
                                                         ClayObject            *object);
static void     clay_filtered_container_src_sort_func   (ClayContainer         *src,
                                                         GParamSpec            *pspec,
                                                         ClayFilteredContainer *filtered_container);
static void     clay_filtered_container_src_add         (ClayContainer         *src_container,
                                                         ClayObject            *obj,
                                                         ClayFilteredContainer *filtered_container);
static void     clay_filtered_container_src_remove      (ClayContainer         *src_container,
                                                         ClayObject            *obj,
                                                         ClayFilteredContainer *filtered_container);
static void     clay_filtered_container_src_freeze      (ClayContainer         *src_container,
                                                         ClayFilteredContainer *filtered_container);
static void     clay_filtered_container_src_thaw        (ClayContainer         *src_container,
                                                         ClayFilteredContainer *filtered_container);


G_DEFINE_TYPE (ClayFilteredContainer, clay_filtered_container, CLAY_TYPE_LIST)

#define parent_class clay_filtered_container_parent_class


static void
clay_filtered_container_class_init (ClayFilteredContainerClass *klass)
{
  GObjectClass               *g_object_class = G_OBJECT_CLASS (klass);
  ClayFilteredContainerClass *filtered_class = CLAY_FILTERED_CONTAINER_CLASS (klass);

  g_object_class->constructed  = clay_filtered_container_constructed;
  g_object_class->dispose      = clay_filtered_container_dispose;
  g_object_class->finalize     = clay_filtered_container_finalize;
  g_object_class->set_property = clay_filtered_container_set_property;
  g_object_class->get_property = clay_filtered_container_get_property;

  filtered_class->src_add      = clay_filtered_container_real_src_add;
  filtered_class->src_remove   = clay_filtered_container_real_src_remove;
  filtered_class->src_freeze   = clay_filtered_container_real_src_freeze;
  filtered_class->src_thaw     = clay_filtered_container_real_src_thaw;

  g_object_class_install_property (g_object_class, PROP_SRC_CONTAINER,
                                   g_param_spec_object ("src-container",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_FUNC,
                                   g_param_spec_pointer ("filter-func",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (g_object_class, PROP_FILTER_DATA,
                                   g_param_spec_pointer ("filter-data",
                                                         NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_filtered_container_init (ClayFilteredContainer *filtered_container)
{
}

static void
clay_filtered_container_constructed (GObject *object)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CONTAINER (filtered_container->src_container));

  if (! clay_container_frozen (filtered_container->src_container))
    {
      /*  a freeze/thaw can't hurt on a newly created container because
       *  we can't have any views yet. This way we get away without
       *  having a virtual function for initializing the container.
       */
      clay_filtered_container_src_freeze (filtered_container->src_container,
                                          filtered_container);
      clay_filtered_container_src_thaw (filtered_container->src_container,
                                        filtered_container);
    }
}

static void
clay_filtered_container_dispose (GObject *object)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (object);

  if (filtered_container->src_container)
    {
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            clay_filtered_container_src_sort_func,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            clay_filtered_container_src_add,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            clay_filtered_container_src_remove,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            clay_filtered_container_src_freeze,
                                            filtered_container);
      g_signal_handlers_disconnect_by_func (filtered_container->src_container,
                                            clay_filtered_container_src_thaw,
                                            filtered_container);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_filtered_container_finalize (GObject *object)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (object);

  g_clear_object (&filtered_container->src_container);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_filtered_container_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (object);
  gint                   freeze_count;

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      filtered_container->src_container = g_value_dup_object (value);

      freeze_count = clay_container_freeze_count (filtered_container->src_container);
      while (freeze_count-- > 0)
        clay_filtered_container_src_freeze (filtered_container->src_container,
                                            filtered_container);

      g_signal_connect (filtered_container->src_container, "notify::sort-func",
                        G_CALLBACK (clay_filtered_container_src_sort_func),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "add",
                        G_CALLBACK (clay_filtered_container_src_add),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "remove",
                        G_CALLBACK (clay_filtered_container_src_remove),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "freeze",
                        G_CALLBACK (clay_filtered_container_src_freeze),
                        filtered_container);
      g_signal_connect (filtered_container->src_container, "thaw",
                        G_CALLBACK (clay_filtered_container_src_thaw),
                        filtered_container);
      break;

    case PROP_FILTER_FUNC:
      filtered_container->filter_func = g_value_get_pointer (value);
      break;

    case PROP_FILTER_DATA:
      filtered_container->filter_data = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_filtered_container_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (object);

  switch (property_id)
    {
    case PROP_SRC_CONTAINER:
      g_value_set_object (value, filtered_container->src_container);
      break;

    case PROP_FILTER_FUNC:
      g_value_set_pointer (value, filtered_container->filter_func);
      break;

    case PROP_FILTER_DATA:
      g_value_set_pointer (value, filtered_container->filter_data);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_filtered_container_real_src_add (ClayFilteredContainer *filtered_container,
                                      ClayObject            *object)
{
  if (clay_filtered_container_object_matches (filtered_container, object))
    {
      clay_container_add (CLAY_CONTAINER (filtered_container), object);
    }
}

static void
clay_filtered_container_real_src_remove (ClayFilteredContainer *filtered_container,
                                         ClayObject            *object)
{
  if (clay_filtered_container_object_matches (filtered_container, object))
    {
      clay_container_remove (CLAY_CONTAINER (filtered_container), object);
    }
}

static void
clay_filtered_container_real_src_freeze (ClayFilteredContainer *filtered_container)
{
  clay_container_clear (CLAY_CONTAINER (filtered_container));
}

static void
clay_filtered_container_real_src_thaw (ClayFilteredContainer *filtered_container)
{
  GList *list;

  for (list = CLAY_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayObject *object = list->data;

      if (clay_filtered_container_object_matches (filtered_container, object))
        {
          clay_container_add (CLAY_CONTAINER (filtered_container), object);
        }
    }
}

/**
 * clay_filtered_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #ClayFilteredContainer object which creates filtered
 * data view of #ClayTagged objects. It filters @src_container for objects
 * containing all of the filtering tags. Synchronization with @src_container
 * data is performed automatically.
 *
 * Returns: a new #ClayFilteredContainer object.
 **/
ClayContainer *
clay_filtered_container_new (ClayContainer        *src_container,
                             ClayObjectFilterFunc  filter_func,
                             gpointer              filter_data)
{
  GType        child_type;
  GCompareFunc sort_func;

  g_return_val_if_fail (CLAY_IS_LIST (src_container), NULL);

  child_type = clay_container_get_child_type (src_container);
  sort_func  = clay_list_get_sort_func (CLAY_LIST (src_container));

  return g_object_new (CLAY_TYPE_FILTERED_CONTAINER,
                       "sort-func",     sort_func,
                       "child-type",    child_type,
                       "policy",        CLAY_CONTAINER_POLICY_WEAK,
                       "unique-names",  FALSE,
                       "src-container", src_container,
                       "filter-func",   filter_func,
                       "filter-data",   filter_data,
                       NULL);
}

static gboolean
clay_filtered_container_object_matches (ClayFilteredContainer *filtered_container,
                                        ClayObject            *object)
{
  return (! filtered_container->filter_func ||
          filtered_container->filter_func (object,
                                           filtered_container->filter_data));
}

static void
clay_filtered_container_src_sort_func (ClayContainer         *src,
                                       GParamSpec            *pspec,
                                       ClayFilteredContainer *filtered_container)
{
  GCompareFunc sort_func = clay_list_get_sort_func (CLAY_LIST (src));

  clay_list_set_sort_func (CLAY_LIST (filtered_container), sort_func);
}

static void
clay_filtered_container_src_add (ClayContainer         *src_container,
                                 ClayObject            *object,
                                 ClayFilteredContainer *filtered_container)
{
  if (! clay_container_frozen (filtered_container->src_container))
    {
      CLAY_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_add (filtered_container,
                                                                       object);
    }
}

static void
clay_filtered_container_src_remove (ClayContainer         *src_container,
                                    ClayObject            *object,
                                    ClayFilteredContainer *filtered_container)
{
  if (! clay_container_frozen (filtered_container->src_container))
    {
      CLAY_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_remove (filtered_container,
                                                                          object);
    }
}

static void
clay_filtered_container_src_freeze (ClayContainer         *src_container,
                                    ClayFilteredContainer *filtered_container)
{
  clay_container_freeze (CLAY_CONTAINER (filtered_container));

  CLAY_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_freeze (filtered_container);
}

static void
clay_filtered_container_src_thaw (ClayContainer         *src_container,
                                  ClayFilteredContainer *filtered_container)
{
  CLAY_FILTERED_CONTAINER_GET_CLASS (filtered_container)->src_thaw (filtered_container);

  clay_container_thaw (CLAY_CONTAINER (filtered_container));
}
