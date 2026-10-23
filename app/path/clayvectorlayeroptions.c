/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   clayvectorlayers.c
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaycolor/claycolor.h"

#include "path-types.h"

#include "core/clay.h"
#include "core/clayfilloptions.h"
#include "core/clayimage.h"
#include "core/claypattern.h"
#include "core/claystrokeoptions.h"

#include "claypath.h"
#include "clayvectorlayer.h"
#include "clayvectorlayeroptions.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CLAY,
  PROP_PATH,
  PROP_PATH_TATTOO,
  PROP_ENABLE_FILL,
  PROP_FILL_OPTIONS,
  PROP_ENABLE_STROKE,
  PROP_STROKE_OPTIONS,
  /* Individual Fill Options */
  PROP_FILL_STYLE,
  PROP_FILL_COLOR,
  PROP_FILL_PATTERN
};


/* local function declarations */

static GObject *clay_vector_layer_options_constructor  (GType                   type,
                                                        guint                   n_params,
                                                        GObjectConstructParam  *params);
static void     clay_vector_layer_options_finalize     (GObject                *object);

static void     clay_vector_layer_options_get_property (GObject                *object,
                                                        guint                   property_id,
                                                        GValue                 *value,
                                                        GParamSpec             *pspec);
static void     clay_vector_layer_options_set_property (GObject                *object,
                                                        guint                   property_id,
                                                        const GValue           *value,
                                                        GParamSpec             *pspec);
static void     clay_vector_layer_options_path_changed (ClayVectorLayerOptions *options);


G_DEFINE_TYPE_WITH_CODE (ClayVectorLayerOptions,
                         clay_vector_layer_options,
                         CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL))

#define parent_class clay_vector_layer_options_parent_class


static void
clay_vector_layer_options_class_init (ClayVectorLayerOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglColor    *black        = gegl_color_new ("black");

  object_class->constructor  = clay_vector_layer_options_constructor;
  object_class->finalize     = clay_vector_layer_options_finalize;
  object_class->set_property = clay_vector_layer_options_set_property;
  object_class->get_property = clay_vector_layer_options_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_PATH,
                           "path", NULL, NULL,
                           CLAY_TYPE_PATH,
                           CLAY_PARAM_READWRITE |
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_UINT   (object_class, PROP_PATH_TATTOO,
                           "path-tattoo", NULL, NULL,
                           0, G_MAXUINT32, 0,
                           G_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_FILL,
                            "enable-fill",
                            _("Fill Style"), NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_FILL_OPTIONS,
                           "fill-options", NULL, NULL,
                           CLAY_TYPE_FILL_OPTIONS,
                           G_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ENABLE_STROKE,
                            "enable-stroke",
                            _("Stroke Style"), NULL,
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_STROKE_OPTIONS,
                           "stroke-options", NULL, NULL,
                           CLAY_TYPE_STROKE_OPTIONS,
                           G_PARAM_STATIC_STRINGS |
                           CLAY_CONFIG_PARAM_AGGREGATE);

  /* Alias for fill properties */
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_FILL_STYLE,
                         "fill-style",
                         NULL, NULL,
                         CLAY_TYPE_CUSTOM_STYLE,
                         CLAY_CUSTOM_STYLE_SOLID_COLOR,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_COLOR (object_class, PROP_FILL_COLOR,
                          "fill-color",
                          NULL, NULL,
                          FALSE, black,
                          CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_FILL_PATTERN,
                           "fill-pattern",
                           NULL, NULL,
                           CLAY_TYPE_PATTERN,
                           CLAY_PARAM_STATIC_STRINGS);

  g_object_unref (black);
}

static void
clay_vector_layer_options_init (ClayVectorLayerOptions *options)
{
  options->path        = NULL;
  options->path_tattoo = 0;
}

static GObject *
clay_vector_layer_options_constructor (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject                *object;
  ClayVectorLayerOptions *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  options = CLAY_VECTOR_LAYER_OPTIONS (object);
  g_assert (CLAY_IS_CLAY (options->clay));

  options->fill_options   = clay_fill_options_new   (options->clay, NULL, FALSE);
  options->stroke_options = clay_stroke_options_new (options->clay, NULL, FALSE);

  return object;
}

static void
clay_vector_layer_options_finalize (GObject *object)
{
  ClayVectorLayerOptions *options = CLAY_VECTOR_LAYER_OPTIONS (object);

  if (options->path)
    {
      g_object_unref (options->path);
      options->path = NULL;
    }

  if (options->fill_options)
    {
      g_object_unref (options->fill_options);
      options->fill_options = NULL;
    }

  if (options->stroke_options)
    {
      g_object_unref (options->stroke_options);
      options->stroke_options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_vector_layer_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClayVectorLayerOptions *options = CLAY_VECTOR_LAYER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, options->clay);
      break;
    case PROP_PATH:
      g_value_set_object (value, options->path);
      break;
    case PROP_PATH_TATTOO:
      g_value_set_uint (value, options->path_tattoo);
      break;
    case PROP_ENABLE_FILL:
      g_value_set_boolean (value, options->enable_fill);
      break;
    case PROP_FILL_OPTIONS:
      g_value_set_object (value, options->fill_options);
      break;
    case PROP_ENABLE_STROKE:
      g_value_set_boolean (value, options->enable_stroke);
      break;
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, options->stroke_options);
      break;

    /* Alias for fill properties */
    case PROP_FILL_STYLE:
    case PROP_FILL_COLOR:
    case PROP_FILL_PATTERN:
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_vector_layer_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClayVectorLayerOptions *options = CLAY_VECTOR_LAYER_OPTIONS (object);

  switch (property_id)
    {
    case PROP_CLAY:
      options->clay = g_value_get_object (value);
      break;

    case PROP_PATH:
      if (options->path)
        {
          g_signal_handlers_disconnect_by_func (options->path,
                                                G_CALLBACK (clay_vector_layer_options_path_changed),
                                                options);
          g_object_unref (options->path);
        }

      options->path = g_value_dup_object (value);

      if (options->path)
        {
          g_signal_connect_object (options->path, "invalidate-preview",
                                   G_CALLBACK (clay_vector_layer_options_path_changed),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (options->path, "name-changed",
                                   G_CALLBACK (clay_vector_layer_options_path_changed),
                                   options, G_CONNECT_SWAPPED);

          /* update the tattoo */
          options->path_tattoo = clay_item_get_tattoo (CLAY_ITEM (options->path));
        }
      break;

    case PROP_PATH_TATTOO:
      options->path_tattoo = g_value_get_uint (value);

      if (options->path &&
          clay_item_get_tattoo (CLAY_ITEM (options->path)) != options->path_tattoo)
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (options->path));

          g_object_set (options,
                        "path", clay_image_get_path_by_tattoo
                        (image, options->path_tattoo),
                        NULL);
        }
      break;

    case PROP_ENABLE_FILL:
      options->enable_fill = g_value_get_boolean (value);
      break;

    case PROP_FILL_OPTIONS:
      if (g_value_get_object (value))
          clay_config_sync (g_value_get_object (value),
                            G_OBJECT (options->fill_options), 0);
      break;

    case PROP_ENABLE_STROKE:
      options->enable_stroke = g_value_get_boolean (value);
      break;

    case PROP_STROKE_OPTIONS:
      if (g_value_get_object (value))
        {
          if (options->stroke_options)
            g_object_unref (options->stroke_options);
          options->stroke_options = clay_config_duplicate (g_value_get_object (value));
        }
      break;

    /* Alias for fill properties */
    case PROP_FILL_STYLE:
      if (options->fill_options)
        g_object_set (CLAY_FILL_OPTIONS (options->fill_options),
                      "custom-style", g_value_get_enum (value),
                      NULL);
      break;

    case PROP_FILL_COLOR:
      clay_context_set_foreground (CLAY_CONTEXT (options->fill_options),
                                   g_value_get_object (value));
      break;

    case PROP_FILL_PATTERN:
      clay_context_set_pattern (CLAY_CONTEXT (options->fill_options),
                                g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_vector_layer_options_path_changed (ClayVectorLayerOptions *options)
{
  g_object_notify (G_OBJECT (options), "path");
}


/* public functions */

/**
 * clay_vector_layer_options_new:
 * @image:   the #ClayImage the layer belongs to
 * @path:    the #ClayPath object for the layer to render
 * @context: the #ClayContext from which to pull context properties
 *
 * Creates a new vector layer options.
 *
 * Return value: a new #ClayVectorLayerOptions or %NULL in case of a problem
 **/
ClayVectorLayerOptions *
clay_vector_layer_options_new (ClayImage     *image,
                               ClayPath      *path,
                               ClayContext   *context)
{
  ClayVectorLayerOptions *options;
  ClayPattern            *pattern;
  GeglColor              *stroke_color;
  GeglColor              *fill_color;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_PATH (path), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  options = g_object_new (CLAY_TYPE_VECTOR_LAYER_OPTIONS,
                          "clay", image->clay,
                          NULL);

  stroke_color = clay_context_get_foreground (context);
  fill_color   = clay_context_get_background (context);
  pattern      = clay_context_get_pattern (context);

  clay_context_set_foreground (CLAY_CONTEXT (options->fill_options),
                               fill_color);
  clay_context_set_pattern (CLAY_CONTEXT (options->fill_options), pattern);

  clay_context_set_foreground (CLAY_CONTEXT (options->stroke_options),
                               stroke_color);
  clay_context_set_pattern (CLAY_CONTEXT (options->stroke_options), pattern);

  g_object_set (options,
                "path", path,
                NULL);

  return options;
}
