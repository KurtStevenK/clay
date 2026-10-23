/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "claycurve.h"
#include "claydynamics.h"
#include "claydynamics-load.h"
#include "claydynamics-save.h"
#include "claydynamicsoutput.h"

#include "clay-intl.h"


#define DEFAULT_NAME "Nameless dynamics"

enum
{
  PROP_0,

  PROP_NAME,

  PROP_OPACITY_OUTPUT,
  PROP_SIZE_OUTPUT,
  PROP_ANGLE_OUTPUT,
  PROP_COLOR_OUTPUT,
  PROP_FORCE_OUTPUT,
  PROP_HARDNESS_OUTPUT,
  PROP_ASPECT_RATIO_OUTPUT,
  PROP_SPACING_OUTPUT,
  PROP_RATE_OUTPUT,
  PROP_FLOW_OUTPUT,
  PROP_JITTER_OUTPUT
};


typedef struct _ClayDynamicsPrivate ClayDynamicsPrivate;

struct _ClayDynamicsPrivate
{
  ClayDynamicsOutput *opacity_output;
  ClayDynamicsOutput *hardness_output;
  ClayDynamicsOutput *force_output;
  ClayDynamicsOutput *rate_output;
  ClayDynamicsOutput *flow_output;
  ClayDynamicsOutput *size_output;
  ClayDynamicsOutput *aspect_ratio_output;
  ClayDynamicsOutput *color_output;
  ClayDynamicsOutput *angle_output;
  ClayDynamicsOutput *jitter_output;
  ClayDynamicsOutput *spacing_output;
};

#define GET_PRIVATE(output) \
        ((ClayDynamicsPrivate *) clay_dynamics_get_instance_private ((ClayDynamics *) (output)))


static void          clay_dynamics_finalize      (GObject      *object);
static void          clay_dynamics_set_property  (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void          clay_dynamics_get_property  (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);
static void
       clay_dynamics_dispatch_properties_changed (GObject      *object,
                                                  guint         n_pspecs,
                                                  GParamSpec  **pspecs);

static const gchar * clay_dynamics_get_extension (ClayData     *data);
static void          clay_dynamics_copy          (ClayData     *data,
                                                  ClayData     *src_data);

static ClayDynamicsOutput *
                     clay_dynamics_create_output (ClayDynamics           *dynamics,
                                                  const gchar            *name,
                                                  ClayDynamicsOutputType  type);
static void          clay_dynamics_output_notify (GObject          *output,
                                                  const GParamSpec *pspec,
                                                  ClayDynamics     *dynamics);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDynamics, clay_dynamics, CLAY_TYPE_DATA)

#define parent_class clay_dynamics_parent_class


static void
clay_dynamics_class_init (ClayDynamicsClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayDataClass     *data_class     = CLAY_DATA_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->finalize                    = clay_dynamics_finalize;
  object_class->set_property                = clay_dynamics_set_property;
  object_class->get_property                = clay_dynamics_get_property;
  object_class->dispatch_properties_changed = clay_dynamics_dispatch_properties_changed;

  viewable_class->default_icon_name         = "clay-dynamics";

  data_class->save                          = clay_dynamics_save;
  data_class->get_extension                 = clay_dynamics_get_extension;
  data_class->copy                          = clay_dynamics_copy;

  CLAY_CONFIG_PROP_STRING (object_class, PROP_NAME,
                           "name",
                           NULL, NULL,
                           DEFAULT_NAME,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_OPACITY_OUTPUT,
                           "opacity-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_FORCE_OUTPUT,
                           "force-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_HARDNESS_OUTPUT,
                           "hardness-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_RATE_OUTPUT,
                           "rate-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_FLOW_OUTPUT,
                           "flow-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_SIZE_OUTPUT,
                           "size-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_ASPECT_RATIO_OUTPUT,
                           "aspect-ratio-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_COLOR_OUTPUT,
                           "color-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_ANGLE_OUTPUT,
                           "angle-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_JITTER_OUTPUT,
                           "jitter-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_SPACING_OUTPUT,
                           "spacing-output",
                           NULL, NULL,
                           CLAY_TYPE_DYNAMICS_OUTPUT,
                           CLAY_CONFIG_PARAM_AGGREGATE);
}

static void
clay_dynamics_init (ClayDynamics *dynamics)
{
  ClayDynamicsPrivate *private = GET_PRIVATE (dynamics);

  private->opacity_output =
    clay_dynamics_create_output (dynamics,
                                 "opacity-output",
                                 CLAY_DYNAMICS_OUTPUT_OPACITY);

  private->force_output =
    clay_dynamics_create_output (dynamics,
                                 "force-output",
                                 CLAY_DYNAMICS_OUTPUT_FORCE);

  private->hardness_output =
    clay_dynamics_create_output (dynamics,
                                 "hardness-output",
                                 CLAY_DYNAMICS_OUTPUT_HARDNESS);

  private->rate_output =
    clay_dynamics_create_output (dynamics,
                                 "rate-output",
                                 CLAY_DYNAMICS_OUTPUT_RATE);

  private->flow_output =
    clay_dynamics_create_output (dynamics,
                                 "flow-output",
                                 CLAY_DYNAMICS_OUTPUT_FLOW);

  private->size_output =
    clay_dynamics_create_output (dynamics,
                                 "size-output",
                                 CLAY_DYNAMICS_OUTPUT_SIZE);

  private->aspect_ratio_output =
    clay_dynamics_create_output (dynamics,
                                 "aspect-ratio-output",
                                 CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO);

  private->color_output =
    clay_dynamics_create_output (dynamics,
                                 "color-output",
                                 CLAY_DYNAMICS_OUTPUT_COLOR);

  private->angle_output =
    clay_dynamics_create_output (dynamics,
                                 "angle-output",
                                 CLAY_DYNAMICS_OUTPUT_ANGLE);

  private->jitter_output =
    clay_dynamics_create_output (dynamics,
                                 "jitter-output",
                                 CLAY_DYNAMICS_OUTPUT_JITTER);

  private->spacing_output =
    clay_dynamics_create_output (dynamics,
                                 "spacing-output",
                                 CLAY_DYNAMICS_OUTPUT_SPACING);
}

static void
clay_dynamics_finalize (GObject *object)
{
  ClayDynamicsPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->opacity_output);
  g_clear_object (&private->force_output);
  g_clear_object (&private->hardness_output);
  g_clear_object (&private->rate_output);
  g_clear_object (&private->flow_output);
  g_clear_object (&private->size_output);
  g_clear_object (&private->aspect_ratio_output);
  g_clear_object (&private->color_output);
  g_clear_object (&private->angle_output);
  g_clear_object (&private->jitter_output);
  g_clear_object (&private->spacing_output);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_dynamics_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClayDynamicsPrivate *private     = GET_PRIVATE (object);
  ClayDynamicsOutput  *src_output  = NULL;
  ClayDynamicsOutput  *dest_output = NULL;

  switch (property_id)
    {
    case PROP_NAME:
      clay_object_set_name (CLAY_OBJECT (object), g_value_get_string (value));
      break;

    case PROP_OPACITY_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->opacity_output;
      break;

    case PROP_FORCE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->force_output;
      break;

    case PROP_HARDNESS_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->hardness_output;
      break;

    case PROP_RATE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->rate_output;
      break;

    case PROP_FLOW_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->flow_output;
      break;

    case PROP_SIZE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->size_output;
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->aspect_ratio_output;
      break;

    case PROP_COLOR_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->color_output;
      break;

    case PROP_ANGLE_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->angle_output;
      break;

    case PROP_JITTER_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->jitter_output;
      break;

    case PROP_SPACING_OUTPUT:
      src_output  = g_value_get_object (value);
      dest_output = private->spacing_output;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (src_output && dest_output)
    {
      clay_config_copy (CLAY_CONFIG (src_output),
                        CLAY_CONFIG (dest_output),
                        CLAY_CONFIG_PARAM_SERIALIZE);
    }
}

static void
clay_dynamics_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClayDynamicsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, clay_object_get_name (object));
      break;

    case PROP_OPACITY_OUTPUT:
      g_value_set_object (value, private->opacity_output);
      break;

    case PROP_FORCE_OUTPUT:
      g_value_set_object (value, private->force_output);
      break;

    case PROP_HARDNESS_OUTPUT:
      g_value_set_object (value, private->hardness_output);
      break;

    case PROP_RATE_OUTPUT:
      g_value_set_object (value, private->rate_output);
      break;

    case PROP_FLOW_OUTPUT:
      g_value_set_object (value, private->flow_output);
      break;

    case PROP_SIZE_OUTPUT:
      g_value_set_object (value, private->size_output);
      break;

    case PROP_ASPECT_RATIO_OUTPUT:
      g_value_set_object (value, private->aspect_ratio_output);
      break;

    case PROP_COLOR_OUTPUT:
      g_value_set_object (value, private->color_output);
      break;

    case PROP_ANGLE_OUTPUT:
      g_value_set_object (value, private->angle_output);
      break;

    case PROP_JITTER_OUTPUT:
      g_value_set_object (value, private->jitter_output);
      break;

    case PROP_SPACING_OUTPUT:
      g_value_set_object (value, private->spacing_output);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dynamics_dispatch_properties_changed (GObject     *object,
                                           guint        n_pspecs,
                                           GParamSpec **pspecs)
{
  gint i;

  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      if (pspecs[i]->flags & CLAY_CONFIG_PARAM_SERIALIZE)
        {
          clay_data_dirty (CLAY_DATA (object));
          break;
        }
    }
}

static const gchar *
clay_dynamics_get_extension (ClayData *data)
{
  return CLAY_DYNAMICS_FILE_EXTENSION;
}

static void
clay_dynamics_copy (ClayData *data,
                    ClayData *src_data)
{
  clay_data_freeze (data);

  clay_config_copy (CLAY_CONFIG (src_data),
                    CLAY_CONFIG (data), 0);

  clay_data_thaw (data);
}


/*  public functions  */

ClayData *
clay_dynamics_new (ClayContext *context,
                   const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\0', NULL);

  return g_object_new (CLAY_TYPE_DYNAMICS,
                       "name", name,
                       NULL);
}

ClayData *
clay_dynamics_get_standard (ClayContext *context)
{
  static ClayData *standard_dynamics = NULL;

  if (! standard_dynamics)
    {
      g_set_weak_pointer (&standard_dynamics,
                          clay_dynamics_new (context, "Standard dynamics"));

      clay_data_clean (standard_dynamics);
      clay_data_make_internal (standard_dynamics, "clay-dynamics-standard");
    }

  return standard_dynamics;
}

ClayDynamicsOutput *
clay_dynamics_get_output (ClayDynamics           *dynamics,
                          ClayDynamicsOutputType  type_id)
{
  ClayDynamicsPrivate *private;

  g_return_val_if_fail (CLAY_IS_DYNAMICS (dynamics), NULL);

  private = GET_PRIVATE (dynamics);

  switch (type_id)
    {
    case CLAY_DYNAMICS_OUTPUT_OPACITY:
      return private->opacity_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_FORCE:
      return private->force_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_HARDNESS:
      return private->hardness_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_RATE:
      return private->rate_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_FLOW:
      return private->flow_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_SIZE:
      return private->size_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO:
      return private->aspect_ratio_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_COLOR:
      return private->color_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_ANGLE:
      return private->angle_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_JITTER:
      return private->jitter_output;
      break;

    case CLAY_DYNAMICS_OUTPUT_SPACING:
      return private->spacing_output;
      break;

    default:
      g_return_val_if_reached (NULL);
      break;
    }
}

gboolean
clay_dynamics_is_output_enabled (ClayDynamics           *dynamics,
                                 ClayDynamicsOutputType  type)
{
  ClayDynamicsOutput *output;

  g_return_val_if_fail (CLAY_IS_DYNAMICS (dynamics), FALSE);

  output = clay_dynamics_get_output (dynamics, type);

  return clay_dynamics_output_is_enabled (output);
}

gdouble
clay_dynamics_get_linear_value (ClayDynamics           *dynamics,
                                ClayDynamicsOutputType  type,
                                const ClayCoords       *coords,
                                ClayPaintOptions       *options,
                                gdouble                 fade_point)
{
  ClayDynamicsOutput *output;

  g_return_val_if_fail (CLAY_IS_DYNAMICS (dynamics), 0.0);

  output = clay_dynamics_get_output (dynamics, type);

  return clay_dynamics_output_get_linear_value (output, coords,
                                                options, fade_point);
}

gdouble
clay_dynamics_get_angular_value (ClayDynamics           *dynamics,
                                 ClayDynamicsOutputType  type,
                                 const ClayCoords       *coords,
                                 ClayPaintOptions       *options,
                                 gdouble                 fade_point)
{
  ClayDynamicsOutput *output;

  g_return_val_if_fail (CLAY_IS_DYNAMICS (dynamics), 0.0);

  output = clay_dynamics_get_output (dynamics, type);

  return clay_dynamics_output_get_angular_value (output, coords,
                                                 options, fade_point);
}

gdouble
clay_dynamics_get_aspect_value (ClayDynamics           *dynamics,
                                ClayDynamicsOutputType  type,
                                const ClayCoords       *coords,
                                ClayPaintOptions       *options,
                                gdouble                 fade_point)
{
  ClayDynamicsOutput *output;

  g_return_val_if_fail (CLAY_IS_DYNAMICS (dynamics), 0.0);

  output = clay_dynamics_get_output (dynamics, type);

  return clay_dynamics_output_get_aspect_value (output, coords,
                                                options, fade_point);
}


/*  private functions  */

static ClayDynamicsOutput *
clay_dynamics_create_output (ClayDynamics           *dynamics,
                             const gchar            *name,
                             ClayDynamicsOutputType  type)
{
  ClayDynamicsOutput *output = clay_dynamics_output_new (name, type);

  g_signal_connect (output, "notify",
                    G_CALLBACK (clay_dynamics_output_notify),
                    dynamics);

  return output;
}

static void
clay_dynamics_output_notify (GObject          *output,
                             const GParamSpec *pspec,
                             ClayDynamics     *dynamics)
{
  g_object_notify (G_OBJECT (dynamics), clay_object_get_name (output));
}
