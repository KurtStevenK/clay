/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "paint-types.h"

#include "claycloneoptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLONE_TYPE
};


static void   clay_clone_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   clay_clone_options_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);


G_DEFINE_TYPE (ClayCloneOptions, clay_clone_options, CLAY_TYPE_SOURCE_OPTIONS)

#define parent_class clay_clone_options_parent_class


static void
clay_clone_options_class_init (ClayCloneOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_clone_options_set_property;
  object_class->get_property = clay_clone_options_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CLONE_TYPE,
                         "clone-type",
                         _("Source"),
                         NULL,
                         CLAY_TYPE_CLONE_TYPE,
                         CLAY_CLONE_IMAGE,
                         CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_clone_options_init (ClayCloneOptions *options)
{
}

static void
clay_clone_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayCloneOptions *options = CLAY_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_TYPE:
      options->clone_type = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_clone_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayCloneOptions *options = CLAY_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLONE_TYPE:
      g_value_set_enum (value, options->clone_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
