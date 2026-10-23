/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationsettings.c
 * Copyright (C) 2020 Ell
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "gegl/clay-gegl-utils.h"

#include "core/claydrawable.h"
#include "core/claydrawablefilter.h"

#include "clayoperationsettings.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLIP,
  PROP_REGION,
  PROP_MODE,
  PROP_OPACITY,
};


static void   clay_operation_settings_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void   clay_operation_settings_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);


G_DEFINE_TYPE (ClayOperationSettings, clay_operation_settings,
               CLAY_TYPE_SETTINGS)

#define parent_class clay_operation_settings_parent_class


static void
clay_operation_settings_class_init (ClayOperationSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_operation_settings_set_property;
  object_class->get_property = clay_operation_settings_get_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CLIP,
                         "clay-clip",
                         _("Clipping"),
                         _("How to clip"),
                         CLAY_TYPE_TRANSFORM_RESIZE,
                         CLAY_TRANSFORM_RESIZE_ADJUST,
                         CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_REGION,
                         "clay-region",
                         NULL, NULL,
                         CLAY_TYPE_FILTER_REGION,
                         CLAY_FILTER_REGION_SELECTION,
                         CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "clay-mode",
                         _("Mode"),
                         NULL,
                         CLAY_TYPE_LAYER_MODE,
                         CLAY_LAYER_MODE_REPLACE,
                         CLAY_CONFIG_PARAM_DEFAULTS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_OPACITY,
                           "clay-opacity",
                           _("Opacity"),
                           NULL,
                           0.0, 1.0, 1.0,
                           CLAY_CONFIG_PARAM_DEFAULTS);
}

static void
clay_operation_settings_init (ClayOperationSettings *settings)
{
}

static void
clay_operation_settings_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  ClayOperationSettings *settings = CLAY_OPERATION_SETTINGS (object);

  switch (property_id)
    {
    case PROP_CLIP:
      g_value_set_enum (value, settings->clip);
      break;

    case PROP_REGION:
      g_value_set_enum (value, settings->region);
      break;

    case PROP_MODE:
      g_value_set_enum (value, settings->mode);
      break;

    case PROP_OPACITY:
      g_value_set_double (value, settings->opacity);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_settings_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  ClayOperationSettings *settings = CLAY_OPERATION_SETTINGS (object);

  switch (property_id)
    {
    case PROP_CLIP:
      settings->clip = g_value_get_enum (value);
      break;

    case PROP_REGION:
      settings->region = g_value_get_enum (value);
      break;

    case PROP_MODE:
      settings->mode = g_value_get_enum (value);
      break;

    case PROP_OPACITY:
      settings->opacity = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

void
clay_operation_settings_sync_drawable_filter (ClayOperationSettings *settings,
                                              ClayDrawableFilter    *filter)
{
  gboolean clip;

  g_return_if_fail (CLAY_IS_OPERATION_SETTINGS (settings));
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  clip = settings->clip == CLAY_TRANSFORM_RESIZE_CLIP ||
         ! babl_format_has_alpha (clay_drawable_filter_get_format (filter));

  clay_drawable_filter_set_region     (filter, settings->region);
  clay_drawable_filter_set_clip       (filter, clip);
  clay_drawable_filter_set_mode       (filter,
                                       settings->mode,
                                       CLAY_LAYER_COLOR_SPACE_AUTO,
                                       CLAY_LAYER_COLOR_SPACE_AUTO,
                                       CLAY_LAYER_COMPOSITE_AUTO);
  clay_drawable_filter_set_opacity    (filter, settings->opacity);
}


/*  protected functions  */

static const gchar * const base_properties[] =
{
  "time",
  "clay-clip",
  "clay-region",
  "clay-mode",
  "clay-opacity",
};

gboolean
clay_operation_settings_config_serialize_base (ClayConfig       *config,
                                               ClayConfigWriter *writer,
                                               gpointer          data)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    {
      if (! clay_config_serialize_property_by_name (config,
                                                    base_properties[i],
                                                    writer))
        {
          return FALSE;
        }
    }

  return TRUE;
}

gboolean
clay_operation_settings_config_equal_base (ClayConfig *a,
                                           ClayConfig *b)
{
  ClayOperationSettings *settings_a = CLAY_OPERATION_SETTINGS (a);
  ClayOperationSettings *settings_b = CLAY_OPERATION_SETTINGS (b);

  return settings_a->clip       == settings_b->clip    &&
         settings_a->region     == settings_b->region  &&
         settings_a->mode       == settings_b->mode    &&
         settings_a->opacity    == settings_b->opacity;
}

void
clay_operation_settings_config_reset_base (ClayConfig *config)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    clay_config_reset_property (G_OBJECT (config), base_properties[i]);

  g_object_thaw_notify (G_OBJECT (config));
}

gboolean
clay_operation_settings_config_copy_base (ClayConfig  *src,
                                          ClayConfig  *dest,
                                          GParamFlags  flags)
{
  gint i;

  g_object_freeze_notify (G_OBJECT (dest));

  for (i = 0; i < G_N_ELEMENTS (base_properties); i++)
    {
      g_object_unref (g_object_bind_property (src,  base_properties[i],
                                              dest, base_properties[i],
                                              G_BINDING_SYNC_CREATE));
    }

  g_object_thaw_notify (G_OBJECT (dest));

  return TRUE;
}
