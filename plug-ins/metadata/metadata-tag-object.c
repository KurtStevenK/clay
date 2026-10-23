/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * metadata-tag.c
 * Copyright (C) 2024 Niels De Graef
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

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "metadata-tag-object.h"

struct _ClayMetadataTagObject
{
  GObject parent_instance;

  gchar *tag;
  gchar *value;
};

G_DEFINE_TYPE (ClayMetadataTagObject, clay_metadata_tag_object, G_TYPE_OBJECT);

static void          clay_metadata_tag_object_finalize          (GObject          *object);

static void
clay_metadata_tag_object_finalize (GObject *object)
{
  ClayMetadataTagObject *self = CLAY_METADATA_TAG_OBJECT (object);

  g_free (self->tag);
  g_free (self->value);

  G_OBJECT_CLASS (clay_metadata_tag_object_parent_class)->finalize (object);
}

static void
clay_metadata_tag_object_init (ClayMetadataTagObject *self)
{
}

static void
clay_metadata_tag_object_class_init (ClayMetadataTagObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_metadata_tag_object_finalize;
}

ClayMetadataTagObject *
clay_metadata_tag_object_new (const gchar *tag,
                              const gchar *value)
{
    ClayMetadataTagObject *self;

    g_return_val_if_fail (tag != NULL, NULL);
    g_return_val_if_fail (value != NULL, NULL);

    self = g_object_new (CLAY_TYPE_METADATA_TAG_OBJECT, NULL);
    self->tag = g_strdup (tag);
    self->value = g_strdup (value);

    return self;
}

const gchar *
clay_metadata_tag_object_get_tag (ClayMetadataTagObject *self)
{
    g_return_val_if_fail (CLAY_IS_METADATA_TAG_OBJECT (self), NULL);

    return self->tag;
}

const gchar *
clay_metadata_tag_object_get_value (ClayMetadataTagObject *self)
{
    g_return_val_if_fail (CLAY_IS_METADATA_TAG_OBJECT (self), NULL);

    return self->value;
}
