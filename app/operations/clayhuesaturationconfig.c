/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayhuesaturationconfig.c
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "clayhuesaturationconfig.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_RANGE,
  PROP_HUE,
  PROP_SATURATION,
  PROP_LIGHTNESS,
  PROP_OVERLAP
};


static void     clay_hue_saturation_config_iface_init   (ClayConfigInterface *iface);

static void     clay_hue_saturation_config_get_property (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);
static void     clay_hue_saturation_config_set_property (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);

static gboolean clay_hue_saturation_config_serialize    (ClayConfig       *config,
                                                         ClayConfigWriter *writer,
                                                         gpointer          data);
static gboolean clay_hue_saturation_config_deserialize  (ClayConfig       *config,
                                                         GScanner         *scanner,
                                                         gint              nest_level,
                                                         gpointer          data);
static gboolean clay_hue_saturation_config_equal        (ClayConfig       *a,
                                                         ClayConfig       *b);
static void     clay_hue_saturation_config_reset        (ClayConfig       *config);
static gboolean clay_hue_saturation_config_copy         (ClayConfig       *src,
                                                         ClayConfig       *dest,
                                                         GParamFlags       flags);


G_DEFINE_TYPE_WITH_CODE (ClayHueSaturationConfig, clay_hue_saturation_config,
                         CLAY_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_hue_saturation_config_iface_init))

#define parent_class clay_hue_saturation_config_parent_class


static void
clay_hue_saturation_config_class_init (ClayHueSaturationConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->set_property        = clay_hue_saturation_config_set_property;
  object_class->get_property        = clay_hue_saturation_config_get_property;

  viewable_class->default_icon_name = "clay-tool-hue-saturation";

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_RANGE,
                         "range",
                         _("Range"),
                         _("The affected range"),
                         CLAY_TYPE_HUE_RANGE,
                         CLAY_HUE_RANGE_ALL, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_HUE,
                           "hue",
                           _("Hue"),
                           _("Hue"),
                           -1.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_SATURATION,
                           "saturation",
                           _("Saturation"),
                           _("Saturation"),
                           -1.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LIGHTNESS,
                           "lightness",
                           _("Lightness"),
                           _("Lightness"),
                           -1.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OVERLAP,
                           "overlap",
                           _("Overlap"),
                           _("Overlap"),
                           0.0, 1.0, 0.0, 0);
}

static void
clay_hue_saturation_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_hue_saturation_config_serialize;
  iface->deserialize = clay_hue_saturation_config_deserialize;
  iface->equal       = clay_hue_saturation_config_equal;
  iface->reset       = clay_hue_saturation_config_reset;
  iface->copy        = clay_hue_saturation_config_copy;
}

static void
clay_hue_saturation_config_init (ClayHueSaturationConfig *self)
{
  clay_config_reset (CLAY_CONFIG (self));
}

static void
clay_hue_saturation_config_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  ClayHueSaturationConfig *self = CLAY_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      g_value_set_enum (value, self->range);
      break;

    case PROP_HUE:
      g_value_set_double (value, self->hue[self->range]);
      break;

    case PROP_SATURATION:
      g_value_set_double (value, self->saturation[self->range]);
      break;

    case PROP_LIGHTNESS:
      g_value_set_double (value, self->lightness[self->range]);
      break;

    case PROP_OVERLAP:
      g_value_set_double (value, self->overlap);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_hue_saturation_config_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  ClayHueSaturationConfig *self = CLAY_HUE_SATURATION_CONFIG (object);

  switch (property_id)
    {
    case PROP_RANGE:
      self->range = g_value_get_enum (value);
      g_object_notify (object, "hue");
      g_object_notify (object, "saturation");
      g_object_notify (object, "lightness");
      break;

    case PROP_HUE:
      self->hue[self->range] = g_value_get_double (value);
      break;

    case PROP_SATURATION:
      self->saturation[self->range] = g_value_get_double (value);
      break;

    case PROP_LIGHTNESS:
      self->lightness[self->range] = g_value_get_double (value);
      break;

    case PROP_OVERLAP:
      self->overlap = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_hue_saturation_config_serialize (ClayConfig       *config,
                                      ClayConfigWriter *writer,
                                      gpointer          data)
{
  ClayHueSaturationConfig *hs_config = CLAY_HUE_SATURATION_CONFIG (config);
  ClayHueRange             range;
  ClayHueRange             old_range;
  gboolean                 success = TRUE;

  if (! clay_operation_settings_config_serialize_base (config, writer, data))
    return FALSE;

  old_range = hs_config->range;

  for (range = CLAY_HUE_RANGE_ALL; range <= CLAY_HUE_RANGE_MAGENTA; range++)
    {
      hs_config->range = range;

      success = (clay_config_serialize_property_by_name (config, "range",
                                                         writer) &&
                 clay_config_serialize_property_by_name (config, "hue",
                                                         writer) &&
                 clay_config_serialize_property_by_name (config, "saturation",
                                                         writer) &&
                 clay_config_serialize_property_by_name (config, "lightness",
                                                         writer));

      if (! success)
        break;
    }

  if (success)
    success = clay_config_serialize_property_by_name (config, "overlap",
                                                      writer);

  hs_config->range = old_range;

  return success;
}

static gboolean
clay_hue_saturation_config_deserialize (ClayConfig *config,
                                        GScanner   *scanner,
                                        gint        nest_level,
                                        gpointer    data)
{
  ClayHueSaturationConfig *hs_config = CLAY_HUE_SATURATION_CONFIG (config);
  ClayHueRange             old_range;
  gboolean                 success = TRUE;

  old_range = hs_config->range;

  success = clay_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "range", old_range, NULL);

  return success;
}

static gboolean
clay_hue_saturation_config_equal (ClayConfig *a,
                                  ClayConfig *b)
{
  ClayHueSaturationConfig *config_a = CLAY_HUE_SATURATION_CONFIG (a);
  ClayHueSaturationConfig *config_b = CLAY_HUE_SATURATION_CONFIG (b);
  ClayHueRange             range;

  if (! clay_operation_settings_config_equal_base (a, b))
    return FALSE;

  for (range = CLAY_HUE_RANGE_ALL; range <= CLAY_HUE_RANGE_MAGENTA; range++)
    {
      if (config_a->hue[range]        != config_b->hue[range]        ||
          config_a->saturation[range] != config_b->saturation[range] ||
          config_a->lightness[range]  != config_b->lightness[range])
        return FALSE;
    }

  /* don't compare "range" */

  if (config_a->overlap != config_b->overlap)
    return FALSE;

  return TRUE;
}

static void
clay_hue_saturation_config_reset (ClayConfig *config)
{
  ClayHueSaturationConfig *hs_config = CLAY_HUE_SATURATION_CONFIG (config);
  ClayHueRange             range;

  clay_operation_settings_config_reset_base (config);

  for (range = CLAY_HUE_RANGE_ALL; range <= CLAY_HUE_RANGE_MAGENTA; range++)
    {
      hs_config->range = range;
      clay_hue_saturation_config_reset_range (hs_config);
    }

  clay_config_reset_property (G_OBJECT (config), "range");
  clay_config_reset_property (G_OBJECT (config), "overlap");
}

static gboolean
clay_hue_saturation_config_copy (ClayConfig   *src,
                                 ClayConfig   *dest,
                                 GParamFlags   flags)
{
  ClayHueSaturationConfig *src_config  = CLAY_HUE_SATURATION_CONFIG (src);
  ClayHueSaturationConfig *dest_config = CLAY_HUE_SATURATION_CONFIG (dest);
  ClayHueRange             range;

  if (! clay_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (range = CLAY_HUE_RANGE_ALL; range <= CLAY_HUE_RANGE_MAGENTA; range++)
    {
      dest_config->hue[range]        = src_config->hue[range];
      dest_config->saturation[range] = src_config->saturation[range];
      dest_config->lightness[range]  = src_config->lightness[range];
    }

  g_object_notify (G_OBJECT (dest), "hue");
  g_object_notify (G_OBJECT (dest), "saturation");
  g_object_notify (G_OBJECT (dest), "lightness");

  dest_config->range   = src_config->range;
  dest_config->overlap = src_config->overlap;

  g_object_notify (G_OBJECT (dest), "range");
  g_object_notify (G_OBJECT (dest), "overlap");

  return TRUE;
}


/*  public functions  */

void
clay_hue_saturation_config_reset_range (ClayHueSaturationConfig *config)
{
  g_return_if_fail (CLAY_IS_HUE_SATURATION_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  clay_config_reset_property (G_OBJECT (config), "hue");
  clay_config_reset_property (G_OBJECT (config), "saturation");
  clay_config_reset_property (G_OBJECT (config), "lightness");

  g_object_thaw_notify (G_OBJECT (config));
}
