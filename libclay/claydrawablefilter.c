/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claydrawablefilter.c
 * Copyright (C) 2024 Jehan
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "clayplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _ClayDrawableFilter
{
  GObject                   parent_instance;
  gint                      id;

  gdouble                   opacity;
  ClayLayerMode             blend_mode;
  ClayLayerColorSpace       blend_space;
  ClayLayerCompositeMode    composite_mode;
  ClayLayerColorSpace       composite_space;

  GHashTable               *pad_inputs;

  ClayDrawableFilterConfig *config;
};


static void   clay_drawable_filter_finalize      (GObject      *object);
static void   clay_drawable_filter_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   clay_drawable_filter_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (ClayDrawableFilter, clay_drawable_filter, G_TYPE_OBJECT)

#define parent_class clay_drawable_filter_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_drawable_filter_class_init (ClayDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_drawable_filter_finalize;
  object_class->set_property = clay_drawable_filter_set_property;
  object_class->get_property = clay_drawable_filter_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The drawable_filter id",
                      "The drawable_filter id for internal use",
                      0, G_MAXINT32, 0,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_drawable_filter_init (ClayDrawableFilter *drawable_filter)
{
  drawable_filter->config = NULL;

  drawable_filter->opacity         = 1.0;
  drawable_filter->blend_mode      = CLAY_LAYER_MODE_REPLACE;
  drawable_filter->blend_space     = CLAY_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode  = CLAY_LAYER_COMPOSITE_AUTO;
  drawable_filter->composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;

  drawable_filter->pad_inputs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
clay_drawable_filter_finalize (GObject *object)
{
  ClayDrawableFilter *filter = CLAY_DRAWABLE_FILTER (object);

  g_clear_object (&filter->config);
  g_hash_table_unref (filter->pad_inputs);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_drawable_filter_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  ClayDrawableFilter *drawable_filter = CLAY_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      drawable_filter->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_filter_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  ClayDrawableFilter *drawable_filter = CLAY_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, drawable_filter->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * clay_drawable_filter_get_id:
 * @filter: The [class@Clay.Drawable]'s filter.
 *
 * Returns: the drawable's filter ID.
 *
 * Since: 3.0
 **/
gint32
clay_drawable_filter_get_id (ClayDrawableFilter *filter)
{
  return filter ? filter->id : -1;
}

/**
 * clay_drawable_filter_get_by_id:
 * @filter_id: The %ClayDrawableFilter id.
 *
 * Returns: (nullable) (transfer none): a #ClayDrawableFilter for @filter_id or
 *          %NULL if @filter_id does not represent a valid drawable's filter.
 *          The object belongs to libclay and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
ClayDrawableFilter *
clay_drawable_filter_get_by_id (gint32 filter_id)
{
  if (filter_id > 0)
    {
      ClayPlugIn *plug_in = clay_get_plug_in ();

      return _clay_plug_in_get_filter (plug_in, filter_id);
    }

  return NULL;
}

/**
 * clay_drawable_filter_is_valid:
 * @filter: The drawable's filter to check.
 *
 * Returns TRUE if the @drawable_filter is valid.
 *
 * This procedure checks if the given filter is valid and refers to an
 * existing %ClayDrawableFilter.
 *
 * Returns: Whether @drawable_filter is valid.
 *
 * Since: 3.0
 **/
gboolean
clay_drawable_filter_is_valid (ClayDrawableFilter *filter)
{
  return clay_drawable_filter_id_is_valid (clay_drawable_filter_get_id (filter));
}

/**
 * clay_drawable_filter_set_opacity:
 * @filter: The drawable's filter.
 * @opacity: the opacity.
 *
 * This procedure sets the opacity of @filter on a range from 0.0
 * (transparent) to 1.0 (opaque).
 *
 * The change is not synced immediately with the core application.
 * Use [method@Clay.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
clay_drawable_filter_set_opacity (ClayDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (opacity >= 0.0 && opacity <= 1.0);

  filter->opacity = opacity;
}

/**
 * clay_drawable_filter_set_blend_mode:
 * @filter: The drawable's filter.
 * @mode: blend mode.
 *
 * This procedure sets the blend mode of @filter.
 *
 * The change is not synced immediately with the core application.
 * Use [method@Clay.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
clay_drawable_filter_set_blend_mode (ClayDrawableFilter *filter,
                                     ClayLayerMode       mode)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));

  filter->blend_mode = mode;
}

/**
 * clay_drawable_filter_set_aux_input:
 * @filter: The drawable's filter.
 * @input_pad_name: name of the filter's input pad.
 * @input: the drawable to use as auxiliary input.
 *
 * When a filter has one or several auxiliary inputs, you can use this
 * function to set them.
 *
 * The change is not synced immediately with the core application.
 * Use [method@Clay.Drawable.update] to trigger an actual update.
 *
 * Since: 3.0
 **/
void
clay_drawable_filter_set_aux_input (ClayDrawableFilter *filter,
                                    const gchar        *input_pad_name,
                                    ClayDrawable       *input)
{
  g_return_if_fail (CLAY_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (CLAY_IS_DRAWABLE (input));

  g_hash_table_insert (filter->pad_inputs, (gpointer) g_strdup (input_pad_name), input);
}

/**
 * clay_drawable_filter_get_config:
 * @filter: A drawable filter.
 *
 * Get the #ClayConfig with properties that match @filter's arguments.
 *
 * The config object will be created at the first call of this method
 * and its properties will be synced with the settings of this filter as
 * set in the core application.
 *
 * Further changes to the config's properties are not synced back
 * immediately with the core application. Use
 * [method@Clay.Drawable.update] to trigger an actual update.
 *
 * Returns: (transfer none): The #ClayDrawableFilterConfig. Further
 *                           calls will return the same object.
 *
 * Since: 3.0
 **/
ClayDrawableFilterConfig *
clay_drawable_filter_get_config (ClayDrawableFilter *filter)
{
  gchar          **argnames;
  ClayValueArray  *values;
  gchar           *config_type_name;
  gchar           *op_name;
  gchar           *canonical_name;
  GType            config_type;
  gint             n_args;

  g_return_val_if_fail (CLAY_IS_DRAWABLE_FILTER (filter), NULL);

  if (filter->config)
    return filter->config;

  op_name          = clay_drawable_filter_get_operation_name (filter);
  canonical_name   = clay_canonicalize_identifier (op_name);
  config_type_name = g_strdup_printf ("ClayDrawableFilterConfig-%s", canonical_name);
  config_type      = g_type_from_name (config_type_name);
  n_args           = _clay_drawable_filter_get_number_arguments (filter);

  if (! config_type)
    {
      GParamSpec **config_args;

      config_args = g_new0 (GParamSpec *, n_args);

      for (gint i = 0; i < n_args; i++)
        {
          GParamSpec *pspec;

          pspec = _clay_drawable_filter_get_pspec (filter, i);
          config_args[i] = pspec;
        }

      config_type = clay_config_type_register (CLAY_TYPE_DRAWABLE_FILTER_CONFIG,
                                               config_type_name, config_args, n_args);

      g_free (config_args);
    }

  g_free (op_name);
  g_free (canonical_name);

  filter->config = g_object_new (config_type, NULL);

  argnames = _clay_drawable_filter_get_arguments (filter, &values);
  for (gint i = 0; argnames[i] != NULL; i++)
    g_object_set_property (G_OBJECT (filter->config), argnames[i],
                           clay_value_array_index (values, i));

  g_strfreev (argnames);
  clay_value_array_unref (values);

  return filter->config;
}

/**
 * clay_drawable_filter_update:
 * @filter: A drawable filter.
 *
 * Syncs the #ClayConfig with properties that match @filter's arguments.
 * This procedure updates the settings of the specified filter all at
 * once, including the arguments of the [class@Clay.DrawableFilterConfig]
 * obtained with [method@Clay.DrawableFilter.get_config] as well as the
 * blend mode and opacity.
 *
 * In particular, if the image is displayed, rendering will be frozen
 * and will happen only once for all changed settings.
 *
 * Since: 3.0
 **/
void
clay_drawable_filter_update (ClayDrawableFilter *filter)
{
  GStrvBuilder        *builder = g_strv_builder_new ();
  ClayValueArray      *values  = NULL;
  GStrv                propnames;
  GStrv                auxnames;
  const ClayDrawable **auxinputs;
  guint                n_aux;
  GObjectClass        *klass;
  GParamSpec         **pspecs;
  guint                n_pspecs;

  if (! filter->config)
    clay_drawable_filter_get_config (filter);

  g_return_if_fail (filter->config != NULL);

  klass  = G_OBJECT_GET_CLASS (filter->config);
  pspecs = g_object_class_list_properties (klass, &n_pspecs);
  values = clay_value_array_new (n_pspecs);

  for (guint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (filter->config), pspec->name, &value);

      if (g_param_value_defaults (pspec, &value))
        {
          g_value_unset (&value);
          continue;
        }

      g_strv_builder_add (builder, pspec->name);
      clay_value_array_append (values, &value);

      g_value_unset (&value);
    }

  propnames = g_strv_builder_end (builder);
  auxnames  = (GStrv) g_hash_table_get_keys_as_array (filter->pad_inputs, &n_aux);
  auxinputs = g_new0 (const ClayDrawable *, n_aux + 1);
  for (guint i = 0; auxnames[i]; i++)
    auxinputs[i] = g_hash_table_lookup (filter->pad_inputs, auxnames[i]);

  _clay_drawable_filter_update (filter, (const gchar **) propnames, values,
                                filter->opacity,
                                filter->blend_mode, filter->blend_space,
                                filter->composite_mode, filter->composite_space,
                                (const gchar **) auxnames, auxinputs);

  g_free (pspecs);
  g_strfreev (propnames);
  g_strv_builder_unref (builder);
  clay_value_array_unref (values);
  g_free (auxnames);
  g_free (auxinputs);
}
