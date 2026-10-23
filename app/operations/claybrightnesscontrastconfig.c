/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrightnesscontrastconfig.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "claybrightnesscontrastconfig.h"
#include "claylevelsconfig.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_BRIGHTNESS,
  PROP_CONTRAST
};


static void     clay_brightness_contrast_config_iface_init   (ClayConfigInterface *iface);

static void     clay_brightness_contrast_config_get_property (GObject      *object,
                                                              guint         property_id,
                                                              GValue       *value,
                                                              GParamSpec   *pspec);
static void     clay_brightness_contrast_config_set_property (GObject      *object,
                                                              guint         property_id,
                                                              const GValue *value,
                                                              GParamSpec   *pspec);

static gboolean clay_brightness_contrast_config_equal        (ClayConfig   *a,
                                                              ClayConfig   *b);


G_DEFINE_TYPE_WITH_CODE (ClayBrightnessContrastConfig,
                         clay_brightness_contrast_config,
                         CLAY_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_brightness_contrast_config_iface_init))

#define parent_class clay_brightness_contrast_config_parent_class


static void
clay_brightness_contrast_config_class_init (ClayBrightnessContrastConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->set_property        = clay_brightness_contrast_config_set_property;
  object_class->get_property        = clay_brightness_contrast_config_get_property;

  viewable_class->default_icon_name = "clay-tool-brightness-contrast";

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_BRIGHTNESS,
                           "brightness",
                           _("Brightness"),
                           _("Brightness"),
                           -1.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_CONTRAST,
                           "contrast",
                           _("Contrast"),
                           _("Contrast"),
                           -1.0, 1.0, 0.0, 0);
}

static void
clay_brightness_contrast_config_iface_init (ClayConfigInterface *iface)
{
  iface->equal = clay_brightness_contrast_config_equal;
}

static void
clay_brightness_contrast_config_init (ClayBrightnessContrastConfig *self)
{
}

static void
clay_brightness_contrast_config_get_property (GObject    *object,
                                              guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  ClayBrightnessContrastConfig *self = CLAY_BRIGHTNESS_CONTRAST_CONFIG (object);

  switch (property_id)
    {
    case PROP_BRIGHTNESS:
      g_value_set_double (value, self->brightness);
      break;

    case PROP_CONTRAST:
      g_value_set_double (value, self->contrast);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_brightness_contrast_config_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  ClayBrightnessContrastConfig *self = CLAY_BRIGHTNESS_CONTRAST_CONFIG (object);

  switch (property_id)
    {
    case PROP_BRIGHTNESS:
      self->brightness = g_value_get_double (value);
      break;

    case PROP_CONTRAST:
      self->contrast = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_brightness_contrast_config_equal (ClayConfig *a,
                                       ClayConfig *b)
{
  ClayBrightnessContrastConfig *config_a = CLAY_BRIGHTNESS_CONTRAST_CONFIG (a);
  ClayBrightnessContrastConfig *config_b = CLAY_BRIGHTNESS_CONTRAST_CONFIG (b);

  if (! clay_operation_settings_config_equal_base (a, b) ||
      config_a->brightness != config_b->brightness       ||
      config_a->contrast   != config_b->contrast)
    {
      return FALSE;
    }

  return TRUE;
}


/*  public functions  */

ClayLevelsConfig *
clay_brightness_contrast_config_to_levels_config (ClayBrightnessContrastConfig *config)
{
  ClayLevelsConfig *levels;
  gdouble           brightness;
  gdouble           slant;
  gdouble           value;

  g_return_val_if_fail (CLAY_IS_BRIGHTNESS_CONTRAST_CONFIG (config), NULL);

  levels = g_object_new (CLAY_TYPE_LEVELS_CONFIG, NULL);

  clay_operation_settings_config_copy_base (CLAY_CONFIG (config),
                                            CLAY_CONFIG (levels),
                                            0);

  brightness = config->brightness / 2.0;
  slant = tan ((config->contrast + 1) * G_PI_4);

  if (config->brightness >= 0)
    {
      value = -0.5 * slant + brightness * slant + 0.5;

      if (value < 0.0)
        {
          value = 0.0;

          /* this slightly convoluted math follows by inverting the
           * calculation of the brightness/contrast LUT in base/lut-funcs.h */

          levels->low_input[CLAY_HISTOGRAM_VALUE] =
            (- brightness * slant + 0.5 * slant - 0.5) / (slant - brightness * slant);
        }

      levels->low_output[CLAY_HISTOGRAM_VALUE] = value;

      value = 0.5 * slant + 0.5;

      if (value > 1.0)
        {
          value = 1.0;

          levels->high_input[CLAY_HISTOGRAM_VALUE] =
            (- brightness * slant + 0.5 * slant + 0.5) / (slant - brightness * slant);
        }

      levels->high_output[CLAY_HISTOGRAM_VALUE] = value;
    }
  else
    {
      value = 0.5 - 0.5 * slant;

      if (value < 0.0)
        {
          value = 0.0;

          levels->low_input[CLAY_HISTOGRAM_VALUE] =
            (0.5 * slant - 0.5) / (slant + brightness * slant);
        }

      levels->low_output[CLAY_HISTOGRAM_VALUE] = value;

      value = slant * brightness + slant * 0.5 + 0.5;

      if (value > 1.0)
        {
          value = 1.0;

          levels->high_input[CLAY_HISTOGRAM_VALUE] =
            (0.5 * slant + 0.5) / (slant + brightness * slant);
        }

      levels->high_output[CLAY_HISTOGRAM_VALUE] = value;
    }

  return levels;
}
