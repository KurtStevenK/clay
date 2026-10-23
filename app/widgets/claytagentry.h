/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytagentry.h
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

#pragma once


#define CLAY_TYPE_TAG_ENTRY            (clay_tag_entry_get_type ())
#define CLAY_TAG_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TAG_ENTRY, ClayTagEntry))
#define CLAY_TAG_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TAG_ENTRY, ClayTagEntryClass))
#define CLAY_IS_TAG_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TAG_ENTRY))
#define CLAY_IS_TAG_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TAG_ENTRY))
#define CLAY_TAG_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TAG_ENTRY, ClayTagEntryClass))


typedef struct _ClayTagEntryClass  ClayTagEntryClass;

struct _ClayTagEntry
{
  GtkEntry             parent_instance;

  ClayTaggedContainer *container;

  /* mask describes the meaning of each char in ClayTagEntry.
   * It is maintained automatically on insert-text and delete-text
   * events. If manual mask modification is desired, then
   * suppress_mask_update must be increased before calling any
   * function changing entry contents.
   * Meaning of mask chars:
   * u - undefined / unknown (just typed unparsed text)
   * t - tag
   * s - separator
   * w - whitespace.
   */
  GString             *mask;
  GList               *selected_items;
  GList               *common_tags;
  GList               *recent_list;
  gint                 tab_completion_index;
  gint                 internal_operation;
  gint                 suppress_mask_update;
  gint                 suppress_tag_query;
  ClayTagEntryMode     mode;
  gboolean             description_shown;
  gboolean             has_invalid_tags;
  guint                tag_query_idle_id;
};

struct _ClayTagEntryClass
{
  GtkEntryClass   parent_class;
};


GType          clay_tag_entry_get_type           (void);

GtkWidget    * clay_tag_entry_new                (ClayTaggedContainer *container,
                                                  ClayTagEntryMode     mode);

void           clay_tag_entry_set_selected_items (ClayTagEntry        *entry,
                                                  GList               *items);
gchar       ** clay_tag_entry_parse_tags         (ClayTagEntry        *entry);
void           clay_tag_entry_set_tag_string     (ClayTagEntry        *entry,
                                                  const gchar         *tag_string);

const gchar  * clay_tag_entry_get_separator      (void);
