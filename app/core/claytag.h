/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytag.h
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


#define CLAY_TYPE_TAG            (clay_tag_get_type ())
#define CLAY_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TAG, ClayTag))
#define CLAY_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TAG, ClayTagClass))
#define CLAY_IS_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TAG))
#define CLAY_IS_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TAG))
#define CLAY_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TAG, ClayTagClass))


typedef struct _ClayTagClass    ClayTagClass;

struct _ClayTag
{
  GObject parent_instance;

  GQuark  tag;
  GQuark  collate_key;

  gboolean internal; /* Tags that are not serialized to disk */
};

struct _ClayTagClass
{
  GObjectClass parent_class;
};

GType         clay_tag_get_type            (void);

ClayTag     * clay_tag_new                 (const gchar *tag_string);
ClayTag     * clay_tag_try_new             (const gchar *tag_string);

const gchar * clay_tag_get_name            (ClayTag     *tag);
guint         clay_tag_get_hash            (ClayTag     *tag);

gboolean      clay_tag_get_internal        (ClayTag     *tag);
void          clay_tag_set_internal        (ClayTag     *tag,
                                            gboolean     internal);

gboolean      clay_tag_equals              (ClayTag     *tag,
                                            ClayTag     *other);
gint          clay_tag_compare_func        (const void  *p1,
                                            const void  *p2);
gint          clay_tag_compare_with_string (ClayTag     *tag,
                                            const gchar *tag_string);
gboolean      clay_tag_has_prefix          (ClayTag     *tag,
                                            const gchar *prefix_string);
gchar       * clay_tag_string_make_valid   (const gchar *tag_string);
gboolean      clay_tag_is_tag_separator    (gunichar     c);

void          clay_tag_or_null_ref         (ClayTag     *tag_or_null);
void          clay_tag_or_null_unref       (ClayTag     *tag_or_null);
