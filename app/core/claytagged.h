/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * claytagged.h
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

#pragma once


#define CLAY_TYPE_TAGGED (clay_tagged_get_type ())
G_DECLARE_INTERFACE (ClayTagged,
                     clay_tagged,
                     CLAY, TAGGED,
                     GObject)


struct _ClayTaggedInterface
{
  GTypeInterface base_iface;

  /*  signals            */
  void       (* tag_added)      (ClayTagged *tagged,
                                 ClayTag    *tag);
  void       (* tag_removed)    (ClayTagged *tagged,
                                 ClayTag    *tag);

  /*  virtual functions  */
  gboolean   (* add_tag)        (ClayTagged *tagged,
                                 ClayTag    *tag);
  gboolean   (* remove_tag)     (ClayTagged *tagged,
                                 ClayTag    *tag);
  GList    * (* get_tags)       (ClayTagged *tagged);
  gchar    * (* get_identifier) (ClayTagged *tagged);
  gchar    * (* get_checksum)   (ClayTagged *tagged);
};


void       clay_tagged_add_tag        (ClayTagged *tagged,
                                       ClayTag    *tag);
void       clay_tagged_remove_tag     (ClayTagged *tagged,
                                       ClayTag    *tag);

void       clay_tagged_set_tags       (ClayTagged *tagged,
                                       GList      *tags);
GList    * clay_tagged_get_tags       (ClayTagged *tagged);

gchar    * clay_tagged_get_identifier (ClayTagged *tagged);
gchar    * clay_tagged_get_checksum   (ClayTagged *tagged);

gboolean   clay_tagged_has_tag        (ClayTagged *tagged,
                                       ClayTag    *tag);
