/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claytaggedcontainer.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "core-types.h"

#include "clay.h"
#include "claytag.h"
#include "claytagged.h"
#include "claytaggedcontainer.h"


enum
{
  TAG_COUNT_CHANGED,
  LAST_SIGNAL
};


static void      clay_tagged_container_dispose            (GObject               *object);
static gint64    clay_tagged_container_get_memsize        (ClayObject            *object,
                                                           gint64                *gui_size);

static void      clay_tagged_container_clear              (ClayContainer         *container);

static void      clay_tagged_container_src_add            (ClayFilteredContainer *filtered_container,
                                                           ClayObject            *object);
static void      clay_tagged_container_src_remove         (ClayFilteredContainer *filtered_container,
                                                           ClayObject            *object);
static void      clay_tagged_container_src_freeze         (ClayFilteredContainer *filtered_container);
static void      clay_tagged_container_src_thaw           (ClayFilteredContainer *filtered_container);

static void      clay_tagged_container_disconnect_handlers (ClayTaggedContainer  *tagged_container);

static gboolean  clay_tagged_container_object_matches     (ClayTaggedContainer   *tagged_container,
                                                           ClayObject            *object);

static void      clay_tagged_container_tag_added          (ClayTagged            *tagged,
                                                           ClayTag               *tag,
                                                           ClayTaggedContainer   *tagged_container);
static void      clay_tagged_container_tag_removed        (ClayTagged            *tagged,
                                                           ClayTag               *tag,
                                                           ClayTaggedContainer   *tagged_container);
static void      clay_tagged_container_ref_tag            (ClayTaggedContainer   *tagged_container,
                                                           ClayTag               *tag);
static void      clay_tagged_container_unref_tag          (ClayTaggedContainer   *tagged_container,
                                                           ClayTag               *tag);
static void      clay_tagged_container_tag_count_changed  (ClayTaggedContainer   *tagged_container,
                                                           gint                   tag_count);


G_DEFINE_TYPE (ClayTaggedContainer, clay_tagged_container,
               CLAY_TYPE_FILTERED_CONTAINER)

#define parent_class clay_tagged_container_parent_class

static guint clay_tagged_container_signals[LAST_SIGNAL] = { 0, };


static void
clay_tagged_container_class_init (ClayTaggedContainerClass *klass)
{
  GObjectClass               *g_object_class    = G_OBJECT_CLASS (klass);
  ClayObjectClass            *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayContainerClass         *container_class   = CLAY_CONTAINER_CLASS (klass);
  ClayFilteredContainerClass *filtered_class    = CLAY_FILTERED_CONTAINER_CLASS (klass);

  g_object_class->dispose        = clay_tagged_container_dispose;

  clay_object_class->get_memsize = clay_tagged_container_get_memsize;

  container_class->clear         = clay_tagged_container_clear;

  filtered_class->src_add        = clay_tagged_container_src_add;
  filtered_class->src_remove     = clay_tagged_container_src_remove;
  filtered_class->src_freeze     = clay_tagged_container_src_freeze;
  filtered_class->src_thaw       = clay_tagged_container_src_thaw;

  klass->tag_count_changed       = clay_tagged_container_tag_count_changed;

  clay_tagged_container_signals[TAG_COUNT_CHANGED] =
    g_signal_new ("tag-count-changed",
                  CLAY_TYPE_TAGGED_CONTAINER,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayTaggedContainerClass, tag_count_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}

static void
clay_tagged_container_init (ClayTaggedContainer *tagged_container)
{
  tagged_container->tag_ref_counts =
    g_hash_table_new_full ((GHashFunc) clay_tag_get_hash,
                           (GEqualFunc) clay_tag_equals,
                           (GDestroyNotify) g_object_unref,
                           (GDestroyNotify) NULL);
}

static void
clay_tagged_container_disconnect_handlers (ClayTaggedContainer *tagged_container)
{
  ClayFilteredContainer *filtered_container = CLAY_FILTERED_CONTAINER (tagged_container);
  GList                 *list;

  for (list = CLAY_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      g_signal_handlers_disconnect_by_func (list->data,
                                            clay_tagged_container_tag_added,
                                            tagged_container);
      g_signal_handlers_disconnect_by_func (list->data,
                                            clay_tagged_container_tag_removed,
                                            tagged_container);
    }
}

static void
clay_tagged_container_dispose (GObject *object)
{
  ClayTaggedContainer *tagged_container = CLAY_TAGGED_CONTAINER (object);

  if (tagged_container->filter)
    {
      g_list_free_full (tagged_container->filter,
                        (GDestroyNotify) clay_tag_or_null_unref);
      tagged_container->filter = NULL;
    }

  clay_tagged_container_disconnect_handlers (tagged_container);

  g_clear_pointer (&tagged_container->tag_ref_counts, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gint64
clay_tagged_container_get_memsize (ClayObject *object,
                                   gint64     *gui_size)
{
  gint64 memsize = 0;

  /* FIXME take members into account */

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_tagged_container_clear (ClayContainer *container)
{
  ClayTaggedContainer *tagged_container = CLAY_TAGGED_CONTAINER (container);

  clay_tagged_container_disconnect_handlers (tagged_container);

  if (tagged_container->tag_ref_counts)
    {
      g_hash_table_remove_all (tagged_container->tag_ref_counts);
      tagged_container->tag_count = 0;
    }

  CLAY_CONTAINER_CLASS (parent_class)->clear (container);
}

static void
clay_tagged_container_src_add (ClayFilteredContainer *filtered_container,
                               ClayObject            *object)
{
  ClayTaggedContainer *tagged_container = CLAY_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  for (list = clay_tagged_get_tags (CLAY_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      clay_tagged_container_ref_tag (tagged_container, list->data);
    }

  g_signal_connect (object, "tag-added",
                    G_CALLBACK (clay_tagged_container_tag_added),
                    tagged_container);
  g_signal_connect (object, "tag-removed",
                    G_CALLBACK (clay_tagged_container_tag_removed),
                    tagged_container);

  if (clay_tagged_container_object_matches (tagged_container, object))
    {
      clay_container_add (CLAY_CONTAINER (tagged_container), object);
    }
}

static void
clay_tagged_container_src_remove (ClayFilteredContainer *filtered_container,
                                  ClayObject            *object)
{
  ClayTaggedContainer *tagged_container = CLAY_TAGGED_CONTAINER (filtered_container);
  GList               *list;

  g_signal_handlers_disconnect_by_func (object,
                                        clay_tagged_container_tag_added,
                                        tagged_container);
  g_signal_handlers_disconnect_by_func (object,
                                        clay_tagged_container_tag_removed,
                                        tagged_container);

  for (list = clay_tagged_get_tags (CLAY_TAGGED (object));
       list;
       list = g_list_next (list))
    {
      clay_tagged_container_unref_tag (tagged_container, list->data);
    }

  if (clay_tagged_container_object_matches (tagged_container, object))
    {
      clay_container_remove (CLAY_CONTAINER (tagged_container), object);
    }
}

static void
clay_tagged_container_src_freeze (ClayFilteredContainer *filtered_container)
{
  ClayTaggedContainer *tagged_container = CLAY_TAGGED_CONTAINER (filtered_container);

  clay_tagged_container_disconnect_handlers (tagged_container);

  clay_container_clear (CLAY_CONTAINER (filtered_container));
}

static void
clay_tagged_container_src_thaw (ClayFilteredContainer *filtered_container)
{
  GList *list;

  for (list = CLAY_LIST (filtered_container->src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      clay_tagged_container_src_add (filtered_container, list->data);
    }
}

/**
 * clay_tagged_container_new:
 * @src_container: container to be filtered.
 *
 * Creates a new #ClayTaggedContainer object which creates filtered
 * data view of #ClayTagged objects. It filters @src_container for
 * objects containing all of the filtering tags. Synchronization with
 * @src_container data is performed automatically.
 *
 * Returns: a new #ClayTaggedContainer object.
 **/
ClayContainer *
clay_tagged_container_new (ClayContainer *src_container)
{
  ClayTaggedContainer *tagged_container;
  GType                child_type;
  GCompareFunc         sort_func;

  g_return_val_if_fail (CLAY_IS_LIST (src_container), NULL);

  child_type = clay_container_get_child_type (src_container);
  sort_func  = clay_list_get_sort_func (CLAY_LIST (src_container));

  tagged_container = g_object_new (CLAY_TYPE_TAGGED_CONTAINER,
                                   "sort-func",     sort_func,
                                   "child-type",    child_type,
                                   "policy",        CLAY_CONTAINER_POLICY_WEAK,
                                   "unique-names",  FALSE,
                                   "src-container", src_container,
                                   NULL);

  return CLAY_CONTAINER (tagged_container);
}

/**
 * clay_tagged_container_set_filter:
 * @tagged_container: a #ClayTaggedContainer object.
 * @tags:             list of #ClayTag objects.
 *
 * Sets list of tags to be used for filtering. Only objects which have
 * all of the tags assigned match filtering criteria.
 **/
void
clay_tagged_container_set_filter (ClayTaggedContainer *tagged_container,
                                  GList               *tags)
{
  GList *new_filter;

  g_return_if_fail (CLAY_IS_TAGGED_CONTAINER (tagged_container));

  if (tags)
    {
      GList *list;

      for (list = tags; list; list = g_list_next (list))
        g_return_if_fail (list->data == NULL || CLAY_IS_TAG (list->data));
    }

  if (! clay_container_frozen (CLAY_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      clay_tagged_container_src_freeze (CLAY_FILTERED_CONTAINER (tagged_container));
    }

  /*  ref new tags first, they could be the same as the old ones  */
  new_filter = g_list_copy (tags);
  g_list_foreach (new_filter, (GFunc) clay_tag_or_null_ref, NULL);

  g_list_free_full (tagged_container->filter,
                    (GDestroyNotify) clay_tag_or_null_unref);
  tagged_container->filter = new_filter;

  if (! clay_container_frozen (CLAY_FILTERED_CONTAINER (tagged_container)->src_container))
    {
      clay_tagged_container_src_thaw (CLAY_FILTERED_CONTAINER (tagged_container));
    }
}

/**
 * clay_tagged_container_get_filter:
 * @tagged_container: a #ClayTaggedContainer object.
 *
 * Returns current tag filter. Tag filter is a list of ClayTag objects, which
 * must be contained by each object matching filter criteria.
 *
 * Returns: a list of ClayTag objects used as filter. This value should
 *               not be modified or freed.
 **/
const GList *
clay_tagged_container_get_filter (ClayTaggedContainer *tagged_container)
{
  g_return_val_if_fail (CLAY_IS_TAGGED_CONTAINER (tagged_container), NULL);

  return tagged_container->filter;
}

static gboolean
clay_tagged_container_object_matches (ClayTaggedContainer *tagged_container,
                                      ClayObject          *object)
{
  GList *filter_tags;

  for (filter_tags = tagged_container->filter;
       filter_tags;
       filter_tags = g_list_next (filter_tags))
    {
      if (! filter_tags->data)
        {
          /* invalid tag - does not match */
          return FALSE;
        }

      if (! clay_tagged_has_tag (CLAY_TAGGED (object),
                                 filter_tags->data))
        {
          /* match for the tag was not found.
           * since query is of type AND, it whole fails.
           */
          return FALSE;
        }
    }

  return TRUE;
}

static void
clay_tagged_container_tag_added (ClayTagged          *tagged,
                                 ClayTag             *tag,
                                 ClayTaggedContainer *tagged_container)
{
  clay_tagged_container_ref_tag (tagged_container, tag);

  if (clay_tagged_container_object_matches (tagged_container,
                                            CLAY_OBJECT (tagged)) &&
      ! clay_container_have (CLAY_CONTAINER (tagged_container),
                             CLAY_OBJECT (tagged)))
    {
      clay_container_add (CLAY_CONTAINER (tagged_container),
                          CLAY_OBJECT (tagged));
    }
}

static void
clay_tagged_container_tag_removed (ClayTagged          *tagged,
                                   ClayTag             *tag,
                                   ClayTaggedContainer *tagged_container)
{
  clay_tagged_container_unref_tag (tagged_container, tag);

  if (! clay_tagged_container_object_matches (tagged_container,
                                              CLAY_OBJECT (tagged)) &&
      clay_container_have (CLAY_CONTAINER (tagged_container),
                           CLAY_OBJECT (tagged)))
    {
      clay_container_remove (CLAY_CONTAINER (tagged_container),
                             CLAY_OBJECT (tagged));
    }
}

static void
clay_tagged_container_ref_tag (ClayTaggedContainer *tagged_container,
                               ClayTag             *tag)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count++;
  g_hash_table_insert (tagged_container->tag_ref_counts,
                       g_object_ref (tag),
                       GINT_TO_POINTER (ref_count));

  if (ref_count == 1)
    {
      tagged_container->tag_count++;
      g_signal_emit (tagged_container,
                     clay_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                     tagged_container->tag_count);
    }
}

static void
clay_tagged_container_unref_tag (ClayTaggedContainer *tagged_container,
                                 ClayTag             *tag)
{
  gint ref_count;

  ref_count = GPOINTER_TO_INT (g_hash_table_lookup (tagged_container->tag_ref_counts,
                                                    tag));
  ref_count--;

  if (ref_count > 0)
    {
      g_hash_table_insert (tagged_container->tag_ref_counts,
                           g_object_ref (tag),
                           GINT_TO_POINTER (ref_count));
    }
  else
    {
      if (g_hash_table_remove (tagged_container->tag_ref_counts, tag))
        {
          tagged_container->tag_count--;
          g_signal_emit (tagged_container,
                         clay_tagged_container_signals[TAG_COUNT_CHANGED], 0,
                         tagged_container->tag_count);
        }
    }
}

static void
clay_tagged_container_tag_count_changed (ClayTaggedContainer *container,
                                         gint                 tag_count)
{
}

/**
 * clay_tagged_container_get_tag_count:
 * @container:  a #ClayTaggedContainer object.
 *
 * Get number of distinct tags that are currently assigned to all
 * objects in the container. The count is independent of currently
 * used filter, it is provided for all available objects (ie. empty
 * filter).
 *
 * Returns: number of distinct tags assigned to all objects in the
 *               container.
 **/
gint
clay_tagged_container_get_tag_count (ClayTaggedContainer *container)
{
  g_return_val_if_fail (CLAY_IS_TAGGED_CONTAINER (container), 0);

  return container->tag_count;
}
