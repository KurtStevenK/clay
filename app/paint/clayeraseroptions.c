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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "paint-types.h"

#include "clayeraseroptions.h"

#include "clay-intl.h"


#define ERASER_DEFAULT_ANTI_ERASE FALSE


enum
{
  PROP_0,
  PROP_ANTI_ERASE
};


static void   clay_eraser_options_set_property (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);
static void   clay_eraser_options_get_property (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);


G_DEFINE_TYPE (ClayEraserOptions, clay_eraser_options,
               CLAY_TYPE_PAINT_OPTIONS)


static void
clay_eraser_options_class_init (ClayEraserOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_eraser_options_set_property;
  object_class->get_property = clay_eraser_options_get_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTI_ERASE,
                            "anti-erase",
                            _("Anti erase"),
                            NULL,
                            ERASER_DEFAULT_ANTI_ERASE,
                            CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_eraser_options_init (ClayEraserOptions *options)
{
}

static void
clay_eraser_options_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayEraserOptions *options = CLAY_ERASER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ANTI_ERASE:
      options->anti_erase = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_eraser_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayEraserOptions *options = CLAY_ERASER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ANTI_ERASE:
      g_value_set_boolean (value, options->anti_erase);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
