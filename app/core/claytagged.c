/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytagged.c
 * Copyright (C) 2008  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#include "core-types.h"

#include "claytag.h"
#include "claytagged.h"


enum
{
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (ClayTagged, clay_tagged, G_TYPE_OBJECT)


static guint clay_tagged_signals[LAST_SIGNAL] = { 0, };


/*  private functions  */


static void
clay_tagged_default_init (ClayTaggedInterface *iface)
{
  clay_tagged_signals[TAG_ADDED] =
    g_signal_new ("tag-added",
                  CLAY_TYPE_TAGGED,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayTaggedInterface, tag_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_TAG);

  clay_tagged_signals[TAG_REMOVED] =
    g_signal_new ("tag-removed",
                  CLAY_TYPE_TAGGED,
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayTaggedInterface, tag_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_TAG);
}


/*  public functions  */


/**
 * clay_tagged_add_tag:
 * @tagged: an object that implements the %ClayTagged interface
 * @tag:    a %ClayTag
 *
 * Adds @tag to the @tagged object. The ClayTagged::tag-added signal
 * is emitted if and only if the @tag was not already assigned to this
 * object.
 **/
void
clay_tagged_add_tag (ClayTagged *tagged,
                     ClayTag    *tag)
{
  g_return_if_fail (CLAY_IS_TAGGED (tagged));
  g_return_if_fail (CLAY_IS_TAG (tag));

  if (CLAY_TAGGED_GET_IFACE (tagged)->add_tag (tagged, tag))
    {
      g_signal_emit (tagged, clay_tagged_signals[TAG_ADDED], 0, tag);
    }
}

/**
 * clay_tagged_remove_tag:
 * @tagged: an object that implements the %ClayTagged interface
 * @tag:    a %ClayTag
 *
 * Removes @tag from the @tagged object. The ClayTagged::tag-removed
 * signal is emitted if and only if the @tag was actually assigned to
 * this object.
 **/
void
clay_tagged_remove_tag (ClayTagged *tagged,
                        ClayTag    *tag)
{
  GList *tag_iter;

  g_return_if_fail (CLAY_IS_TAGGED (tagged));
  g_return_if_fail (CLAY_IS_TAG (tag));

  for (tag_iter = clay_tagged_get_tags (tagged);
       tag_iter;
       tag_iter = g_list_next (tag_iter))
    {
      ClayTag *tag_ref = tag_iter->data;

      if (clay_tag_equals (tag_ref, tag))
        {
          g_object_ref (tag_ref);

          if (CLAY_TAGGED_GET_IFACE (tagged)->remove_tag (tagged, tag_ref))
            {
              g_signal_emit (tagged, clay_tagged_signals[TAG_REMOVED], 0,
                             tag_ref);
            }

          g_object_unref (tag_ref);

          return;
        }
    }
}

/**
 * clay_tagged_set_tags:
 * @tagged: an object that implements the %ClayTagged interface
 * @tags:   a list of tags
 *
 * Sets the list of tags assigned to this object. The passed list of
 * tags is copied and should be freed by the caller.
 **/
void
clay_tagged_set_tags (ClayTagged *tagged,
                      GList      *tags)
{
  GList *old_tags;
  GList *list;

  g_return_if_fail (CLAY_IS_TAGGED (tagged));

  old_tags = g_list_copy (clay_tagged_get_tags (tagged));

  for (list = old_tags; list; list = g_list_next (list))
    {
      clay_tagged_remove_tag (tagged, list->data);
    }

  g_list_free (old_tags);

  for (list = tags; list; list = g_list_next (list))
    {
      g_return_if_fail (CLAY_IS_TAG (list->data));

      clay_tagged_add_tag (tagged, list->data);
    }
}

/**
 * clay_tagged_get_tags:
 * @tagged: an object that implements the %ClayTagged interface
 *
 * Returns the list of tags assigned to this object. The returned %GList
 * is owned by the @tagged object and must not be modified or destroyed.
 *
 * Returns: a list of tags
 **/
GList *
clay_tagged_get_tags (ClayTagged *tagged)
{
  g_return_val_if_fail (CLAY_IS_TAGGED (tagged), NULL);

  return CLAY_TAGGED_GET_IFACE (tagged)->get_tags (tagged);
}

/**
 * clay_tagged_get_identifier:
 * @tagged: an object that implements the %ClayTagged interface
 *
 * Returns an identifier string which uniquely identifies the tagged
 * object. Two different objects must have unique identifiers but may
 * have the same checksum (which will be the case if one object is a
 * copy of the other). The identifier must be the same across
 * sessions, so for example an instance pointer cannot be used as an
 * identifier.
 *
 * Returns: a newly allocated string containing unique identifier
 * of the object. It must be freed using #g_free.
 **/
gchar *
clay_tagged_get_identifier (ClayTagged *tagged)
{
  g_return_val_if_fail (CLAY_IS_TAGGED (tagged), NULL);

  return CLAY_TAGGED_GET_IFACE (tagged)->get_identifier (tagged);
}

/**
 * clay_tagged_get_checksum:
 * @tagged: an object that implements the %ClayTagged interface
 *
 * Returns the checksum of the @tagged object. It is used to remap the
 * tags for an object for which the identifier has changed, for
 * example if the user has renamed a data file since the last session.
 *
 * If the object does not want to support such remapping (objects not
 * stored in file for example) it can return %NULL.
 *
 * Returns: (nullable): checksum string if object needs identifier remapping,
 * %NULL otherwise. Returned string must be freed with #g_free().
 **/
gchar *
clay_tagged_get_checksum (ClayTagged *tagged)
{
  g_return_val_if_fail (CLAY_IS_TAGGED (tagged), FALSE);

  return CLAY_TAGGED_GET_IFACE (tagged)->get_checksum (tagged);
}

/**
 * clay_tagged_has_tag:
 * @tagged: an object that implements the %ClayTagged interface
 * @tag:    a %ClayTag
 *
 * Returns: %TRUE if the object has @tag, %FALSE otherwise.
 **/
gboolean
clay_tagged_has_tag (ClayTagged *tagged,
                     ClayTag    *tag)
{
  GList *tag_iter;

  g_return_val_if_fail (CLAY_IS_TAGGED (tagged), FALSE);
  g_return_val_if_fail (CLAY_IS_TAG (tag), FALSE);

  for (tag_iter = clay_tagged_get_tags (tagged);
       tag_iter;
       tag_iter = g_list_next (tag_iter))
    {
      if (clay_tag_equals (tag_iter->data, tag))
        return TRUE;
    }

  return FALSE;
}
