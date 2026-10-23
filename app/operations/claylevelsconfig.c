/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylevelsconfig.c
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

#include <errno.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaycolor/claycolor-private.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "core/clay-utils.h"
#include "core/claycurve.h"
#include "core/clayhistogram.h"

#include "claycurvesconfig.h"
#include "claylevelsconfig.h"
#include "clayoperationlevels.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_TRC,
  PROP_LINEAR,
  PROP_CHANNEL,
  PROP_LOW_INPUT,
  PROP_HIGH_INPUT,
  PROP_CLAMP_INPUT,
  PROP_GAMMA,
  PROP_LOW_OUTPUT,
  PROP_HIGH_OUTPUT,
  PROP_CLAMP_OUTPUT
};


static void     clay_levels_config_iface_init   (ClayConfigInterface *iface);

static void     clay_levels_config_get_property (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     clay_levels_config_set_property (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static gboolean clay_levels_config_serialize    (ClayConfig       *config,
                                                 ClayConfigWriter *writer,
                                                 gpointer          data);
static gboolean clay_levels_config_deserialize  (ClayConfig       *config,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);
static gboolean clay_levels_config_equal        (ClayConfig       *a,
                                                 ClayConfig       *b);
static void     clay_levels_config_reset        (ClayConfig       *config);
static gboolean clay_levels_config_copy         (ClayConfig       *src,
                                                 ClayConfig       *dest,
                                                 GParamFlags       flags);


G_DEFINE_TYPE_WITH_CODE (ClayLevelsConfig, clay_levels_config,
                         CLAY_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_levels_config_iface_init))

#define parent_class clay_levels_config_parent_class


static void
clay_levels_config_class_init (ClayLevelsConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->set_property        = clay_levels_config_set_property;
  object_class->get_property        = clay_levels_config_get_property;

  viewable_class->default_icon_name = "clay-tool-levels";

  /* "trc" should default to CLAY_TRC_PERCEPTUAL (cf. #15962).
   * We cannot change it until we implement GEGL op versioning.
   * In Clay 3.0, calling this op from the public API was always run in
   * linear (#15681).
   */
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_TRC,
                         "trc",
                         _("Tone Reproduction Curve"),
                         _("Work on linear or perceptual RGB, or following the image's TRC"),
                         CLAY_TYPE_TRC_TYPE,
                         CLAY_TRC_LINEAR, 0);

  /* "linear" is a compat property initially kept for compatibility
   * reason, in particular for config parsing. It's not needed anymore
   * since we have config migration rules now and was rendered bogus to
   * avoid property mixups when setting from API.
   * TODO: remove "linear" when we can version GEGL operations.
   */
  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_LINEAR,
                            "linear",
                            _("Linear"),
                            _("Work on linear RGB (this property is ignored; use \"trc\" instead)"),
                            TRUE, 0);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         _("The affected channel"),
                         CLAY_TYPE_HISTOGRAM_CHANNEL,
                         CLAY_HISTOGRAM_VALUE, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LOW_INPUT,
                           "low-input",
                           _("Low Input"),
                           _("Low Input"),
                           0.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_HIGH_INPUT,
                           "high-input",
                           _("High Input"),
                           _("High Input"),
                           0.0, 1.0, 1.0, 0);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CLAMP_INPUT,
                           "clamp-input",
                           _("Clamp Input"),
                           _("Clamp input values before applying output mapping."),
                            FALSE, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_GAMMA,
                           "gamma",
                           _("Gamma"),
                           _("Gamma"),
                           0.1, 10.0, 1.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LOW_OUTPUT,
                           "low-output",
                           _("Low Output"),
                           _("Low Output"),
                           0.0, 1.0, 0.0, 0);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_HIGH_OUTPUT,
                           "high-output",
                           _("High Output"),
                           _("High Output"),
                           0.0, 1.0, 1.0, 0);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_CLAMP_OUTPUT,
                           "clamp-output",
                           _("Clamp Output"),
                           _("Clamp final output values."),
                            FALSE, 0);
}

static void
clay_levels_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_levels_config_serialize;
  iface->deserialize = clay_levels_config_deserialize;
  iface->equal       = clay_levels_config_equal;
  iface->reset       = clay_levels_config_reset;
  iface->copy        = clay_levels_config_copy;
}

static void
clay_levels_config_init (ClayLevelsConfig *self)
{
  clay_config_reset (CLAY_CONFIG (self));
}

static void
clay_levels_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayLevelsConfig *self = CLAY_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      g_value_set_enum (value, self->trc);
      break;

    case PROP_LINEAR:
      g_value_set_boolean (value, self->trc == CLAY_TRC_LINEAR ? TRUE : FALSE);
      break;

    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_LOW_INPUT:
      g_value_set_double (value, self->low_input[self->channel]);
      break;

    case PROP_HIGH_INPUT:
      g_value_set_double (value, self->high_input[self->channel]);
      break;

    case PROP_CLAMP_INPUT:
      g_value_set_boolean (value, self->clamp_input);
      break;

    case PROP_GAMMA:
      g_value_set_double (value, self->gamma[self->channel]);
      break;

    case PROP_LOW_OUTPUT:
      g_value_set_double (value, self->low_output[self->channel]);
      break;

    case PROP_HIGH_OUTPUT:
      g_value_set_double (value, self->high_output[self->channel]);
      break;

    case PROP_CLAMP_OUTPUT:
      g_value_set_boolean (value, self->clamp_output);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_levels_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayLevelsConfig *self = CLAY_LEVELS_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      self->trc = g_value_get_enum (value);
      break;

    case PROP_LINEAR:
      /* Ignored */
      break;

    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      g_object_notify (object, "low-input");
      g_object_notify (object, "high-input");
      g_object_notify (object, "gamma");
      g_object_notify (object, "low-output");
      g_object_notify (object, "high-output");
      break;

    case PROP_LOW_INPUT:
      self->low_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_INPUT:
      self->high_input[self->channel] = g_value_get_double (value);
      break;

    case PROP_CLAMP_INPUT:
      self->clamp_input = g_value_get_boolean (value);
      break;

    case PROP_GAMMA:
      self->gamma[self->channel] = g_value_get_double (value);
      break;

    case PROP_LOW_OUTPUT:
      self->low_output[self->channel] = g_value_get_double (value);
      break;

    case PROP_HIGH_OUTPUT:
      self->high_output[self->channel] = g_value_get_double (value);
      break;

    case PROP_CLAMP_OUTPUT:
      self->clamp_output = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_levels_config_serialize (ClayConfig       *config,
                              ClayConfigWriter *writer,
                              gpointer          data)
{
  ClayLevelsConfig     *l_config = CLAY_LEVELS_CONFIG (config);
  ClayHistogramChannel  channel;
  ClayHistogramChannel  old_channel;
  gboolean              success = TRUE;

  if (! clay_operation_settings_config_serialize_base (config, writer, data)    ||
      ! clay_config_serialize_property_by_name (config, "trc",       writer)    ||
      ! clay_config_serialize_property_by_name (config, "clamp-input",  writer) ||
      ! clay_config_serialize_property_by_name (config, "clamp-output", writer))
    return FALSE;

  old_channel = l_config->channel;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      l_config->channel = channel;

      /*  serialize the channel properties manually (not using
       *  clay_config_serialize_properties()), so the parent class'
       *  properties don't end up in the config file one per channel.
       *  See bug #700653.
       */
      success =
        (clay_config_serialize_property_by_name (config, "channel",     writer) &&
         clay_config_serialize_property_by_name (config, "low-input",   writer) &&
         clay_config_serialize_property_by_name (config, "high-input",  writer) &&
         clay_config_serialize_property_by_name (config, "gamma",       writer) &&
         clay_config_serialize_property_by_name (config, "low-output",  writer) &&
         clay_config_serialize_property_by_name (config, "high-output", writer));

      if (! success)
        break;
    }

  l_config->channel = old_channel;

  return success;
}

static gboolean
clay_levels_config_deserialize (ClayConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  ClayLevelsConfig     *l_config = CLAY_LEVELS_CONFIG (config);
  ClayHistogramChannel  old_channel;
  gboolean              success = TRUE;

  old_channel = l_config->channel;

  success = clay_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "channel", old_channel, NULL);

  return success;
}

static gboolean
clay_levels_config_equal (ClayConfig *a,
                          ClayConfig *b)
{
  ClayLevelsConfig     *config_a = CLAY_LEVELS_CONFIG (a);
  ClayLevelsConfig     *config_b = CLAY_LEVELS_CONFIG (b);
  ClayHistogramChannel  channel;

  if (! clay_operation_settings_config_equal_base (a, b) ||
      config_a->trc          != config_b->trc            ||
      config_a->clamp_input  != config_b->clamp_input    ||
      config_a->clamp_output != config_b->clamp_output)
    return FALSE;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      if (config_a->gamma[channel]       != config_b->gamma[channel]      ||
          config_a->low_input[channel]   != config_b->low_input[channel]  ||
          config_a->high_input[channel]  != config_b->high_input[channel] ||
          config_a->low_output[channel]  != config_b->low_output[channel] ||
          config_a->high_output[channel] != config_b->high_output[channel])
        return FALSE;
    }

  /* don't compare "channel" */

  return TRUE;
}

static void
clay_levels_config_reset (ClayConfig *config)
{
  ClayLevelsConfig     *l_config = CLAY_LEVELS_CONFIG (config);
  ClayHistogramChannel  channel;

  clay_operation_settings_config_reset_base (config);

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      l_config->channel = channel;
      clay_levels_config_reset_channel (l_config);
    }

  clay_config_reset_property (G_OBJECT (config), "trc");
  clay_config_reset_property (G_OBJECT (config), "channel");
  clay_config_reset_property (G_OBJECT (config), "clamp-input");
  clay_config_reset_property (G_OBJECT (config), "clamp_output");
}

static gboolean
clay_levels_config_copy (ClayConfig  *src,
                         ClayConfig  *dest,
                         GParamFlags  flags)
{
  ClayLevelsConfig     *src_config  = CLAY_LEVELS_CONFIG (src);
  ClayLevelsConfig     *dest_config = CLAY_LEVELS_CONFIG (dest);
  ClayHistogramChannel  channel;

  if (! clay_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      dest_config->gamma[channel]       = src_config->gamma[channel];
      dest_config->low_input[channel]   = src_config->low_input[channel];
      dest_config->high_input[channel]  = src_config->high_input[channel];
      dest_config->low_output[channel]  = src_config->low_output[channel];
      dest_config->high_output[channel] = src_config->high_output[channel];
    }

  g_object_notify (G_OBJECT (dest), "gamma");
  g_object_notify (G_OBJECT (dest), "low-input");
  g_object_notify (G_OBJECT (dest), "high-input");
  g_object_notify (G_OBJECT (dest), "low-output");
  g_object_notify (G_OBJECT (dest), "high-output");

  dest_config->trc          = src_config->trc;
  dest_config->channel      = src_config->channel;
  dest_config->clamp_input  = src_config->clamp_input;
  dest_config->clamp_output = src_config->clamp_output;

  g_object_notify (G_OBJECT (dest), "trc");
  g_object_notify (G_OBJECT (dest), "channel");
  g_object_notify (G_OBJECT (dest), "clamp-input");
  g_object_notify (G_OBJECT (dest), "clamp-output");

  return TRUE;
}


/*  public functions  */

void
clay_levels_config_reset_channel (ClayLevelsConfig *config)
{
  g_return_if_fail (CLAY_IS_LEVELS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  clay_config_reset_property (G_OBJECT (config), "gamma");
  clay_config_reset_property (G_OBJECT (config), "low-input");
  clay_config_reset_property (G_OBJECT (config), "high-input");
  clay_config_reset_property (G_OBJECT (config), "low-output");
  clay_config_reset_property (G_OBJECT (config), "high-output");

  g_object_thaw_notify (G_OBJECT (config));
}

void
clay_levels_config_stretch (ClayLevelsConfig *config,
                            ClayHistogram    *histogram,
                            gboolean          is_color)
{
  g_return_if_fail (CLAY_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  g_object_freeze_notify (G_OBJECT (config));

  if (is_color)
    {
      ClayHistogramChannel channel;

      /*  Set the overall value to defaults  */
      channel = config->channel;
      config->channel = CLAY_HISTOGRAM_VALUE;
      clay_levels_config_reset_channel (config);
      config->channel = channel;

      for (channel = CLAY_HISTOGRAM_RED;
           channel <= CLAY_HISTOGRAM_BLUE;
           channel++)
        {
          clay_levels_config_stretch_channel (config, histogram, channel);
        }
    }
  else
    {
      clay_levels_config_stretch_channel (config, histogram,
                                          CLAY_HISTOGRAM_VALUE);
    }

  g_object_thaw_notify (G_OBJECT (config));
}

void
clay_levels_config_stretch_channel (ClayLevelsConfig     *config,
                                    ClayHistogram        *histogram,
                                    ClayHistogramChannel  channel)
{
  gdouble count;
  gdouble bias = 0.006;
  gint    n_bins;
  gint    i;

  g_return_if_fail (CLAY_IS_LEVELS_CONFIG (config));
  g_return_if_fail (histogram != NULL);

  g_object_freeze_notify (G_OBJECT (config));

  config->gamma[channel]       = 1.0;
  config->low_output[channel]  = 0.0;
  config->high_output[channel] = 1.0;

  n_bins = clay_histogram_n_bins (histogram);
  count  = clay_histogram_get_count (histogram, channel, 0, n_bins - 1);

  if (count == 0.0)
    {
      config->low_input[channel]  = 0.0;
      config->high_input[channel] = 0.0;
    }
  else
    {
      gdouble new_count;
      gdouble percentage;
      gdouble next_percentage;

      /*  Set the low input  */
      new_count = 0.0;

      for (i = 0; i < (n_bins - 1); i++)
        {
          new_count += clay_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             clay_histogram_get_value (histogram,
                                                       channel,
                                                       i + 1)) / count;

          if (fabs (percentage - bias) < fabs (next_percentage - bias))
            {
              config->low_input[channel] = (gdouble) (i + 1) / (n_bins - 1);
              break;
            }
        }

      /*  Set the high input  */
      new_count = 0.0;

      for (i = (n_bins - 1); i > 0; i--)
        {
          new_count += clay_histogram_get_value (histogram, channel, i);
          percentage = new_count / count;
          next_percentage = (new_count +
                             clay_histogram_get_value (histogram,
                                                       channel,
                                                       i - 1)) / count;

          if (fabs (percentage - bias) < fabs (next_percentage - bias))
            {
              config->high_input[channel] = (gdouble) (i - 1) / (n_bins - 1);
              break;
            }
        }
    }

  g_object_notify (G_OBJECT (config), "gamma");
  g_object_notify (G_OBJECT (config), "low-input");
  g_object_notify (G_OBJECT (config), "high-input");
  g_object_notify (G_OBJECT (config), "low-output");
  g_object_notify (G_OBJECT (config), "high-output");

  g_object_thaw_notify (G_OBJECT (config));
}

static gdouble
clay_levels_config_input_from_color (ClayLevelsConfig     *config,
                                     ClayHistogramChannel  channel,
                                     const Babl           *target_space,
                                     GeglColor            *color)
{
  gdouble rgba[4];

  if (config->trc == CLAY_TRC_LINEAR)
    gegl_color_get_pixel (color, babl_format_with_space ("RGBA double", target_space), rgba);
  else
    gegl_color_get_pixel (color, babl_format_with_space ("R'G'B'A double", target_space), rgba);

  switch (channel)
    {
    case CLAY_HISTOGRAM_VALUE:
      return MAX (MAX (rgba[0], rgba[1]), rgba[2]);

    case CLAY_HISTOGRAM_RED:
      return rgba[0];

    case CLAY_HISTOGRAM_GREEN:
      return rgba[1];

    case CLAY_HISTOGRAM_BLUE:
      return rgba[2];

    case CLAY_HISTOGRAM_ALPHA:
      return rgba[3];

    case CLAY_HISTOGRAM_RGB:
      return MIN (MIN (rgba[0], rgba[1]), rgba[2]);

    case CLAY_HISTOGRAM_LUMINANCE:
      return CLAY_RGB_LUMINANCE (rgba[0], rgba[1], rgba[2]);
    }

  return 0.0;
}

void
clay_levels_config_adjust_by_colors (ClayLevelsConfig     *config,
                                     ClayHistogramChannel  channel,
                                     const Babl           *target_space,
                                     GeglColor            *black,
                                     GeglColor            *gray,
                                     GeglColor            *white)
{
  g_return_if_fail (CLAY_IS_LEVELS_CONFIG (config));

  g_object_freeze_notify (G_OBJECT (config));

  if (black)
    {
      config->low_input[channel] = clay_levels_config_input_from_color (config, channel, target_space, black);
      g_object_notify (G_OBJECT (config), "low-input");
    }


  if (white)
    {
      config->high_input[channel] = clay_levels_config_input_from_color (config, channel, target_space, white);
      g_object_notify (G_OBJECT (config), "high-input");
    }

  if (gray)
    {
      gdouble input;
      gdouble range;
      gdouble inten;
      gdouble out_light;
      gdouble lightness;
      gdouble rgba[4];

      /* Calculate lightness value */
      gegl_color_get_pixel (gray, babl_format ("RGBA double"), rgba);
      lightness = CLAY_RGB_LUMINANCE (rgba[0], rgba[1], rgba[2]);

      input = clay_levels_config_input_from_color (config, channel, target_space, gray);

      range = config->high_input[channel] - config->low_input[channel];
      if (range <= 0)
        goto out;

      input -= config->low_input[channel];
      if (input < 0)
        goto out;

      /* Normalize input and lightness */
      inten = input / range;
      out_light = lightness / range;

      /* See bug 622054: picking pure black or white as gamma doesn't
       * work. But we cannot compare to 0.0 or 1.0 because cpus and
       * compilers are shit. If you try to check out_light using
       * printf() it will give exact 0.0 or 1.0 anyway, probably
       * because the generated code is different and out_light doesn't
       * live in a register. That must be why the cpu/compiler mafia
       * invented epsilon and defined this shit to be the programmer's
       * responsibility.
       */
      if (out_light <= 0.0001 || out_light >= 0.9999)
        goto out;

      /* Map selected color to corresponding lightness */
      config->gamma[channel] = log (inten) / log (out_light);
      config->gamma[channel] = CLAMP (config->gamma[channel], 0.1, 10.0);
      g_object_notify (G_OBJECT (config), "gamma");
    }

 out:
  g_object_thaw_notify (G_OBJECT (config));
}

ClayCurvesConfig *
clay_levels_config_to_curves_config (ClayLevelsConfig *config)
{
  ClayCurvesConfig     *curves;
  ClayHistogramChannel  channel;

  g_return_val_if_fail (CLAY_IS_LEVELS_CONFIG (config), NULL);

  curves = g_object_new (CLAY_TYPE_CURVES_CONFIG, NULL);

  clay_operation_settings_config_copy_base (CLAY_CONFIG (config),
                                            CLAY_CONFIG (curves),
                                            0);

  curves->trc = config->trc;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      ClayCurve  *curve    = curves->curve[channel];
      static const gint n  = 8;
      gdouble     gamma    = config->gamma[channel];
      gdouble     delta_in;
      gdouble     delta_out;
      gdouble     x, y;

      /* clear the points set by default */
      clay_curve_clear_points (curve);

      delta_in  = config->high_input[channel] - config->low_input[channel];
      delta_out = config->high_output[channel] - config->low_output[channel];

      x = config->low_input[channel];
      y = config->low_output[channel];

      clay_curve_add_point (curve, x, y);

      if (delta_out != 0 && gamma != 1.0)
        {
          /* The Levels tool performs gamma adjustment, which is a
           * power law, while the Curves tool uses cubic Bézier
           * curves. Here we try to approximate this gamma adjustment
           * with a Bézier curve with 5 control points. Two of them
           * must be (low_input, low_output) and (high_input,
           * high_output), so we need to add 3 more control points in
           * the middle.
           */
          gint i;

          if (gamma > 1)
            {
              /* Case no. 1: γ > 1
               *
               * The curve should look like a horizontal
               * parabola. Since its curvature is greatest when x is
               * small, we add more control points there, so the
               * approximation is more accurate. I decided to set the
               * length of the consecutive segments to x₀, γ⋅x₀, γ²⋅x₀
               * and γ³⋅x₀ and I saw that the curves looked
               * good. Still, this is completely arbitrary.
               */
              gdouble dx = 0;
              gdouble x0;

              for (i = 0; i < n; ++i)
                dx = dx * gamma + 1;
              x0 = delta_in / dx;

              dx = 0;
              for (i = 1; i < n; ++i)
                {
                  dx = dx * gamma + x0;
                  x = config->low_input[channel] + dx;
                  y = config->low_output[channel] + delta_out *
                      clay_operation_levels_map_input (config, channel, x);
                  clay_curve_add_point (curve, x, y);
                }
            }
          else
            {
              /* Case no. 2: γ < 1
               *
               * The curve is the same as the one in case no. 1,
               * observed through a reflexion along the y = x axis. So
               * if we invert γ and swap the x and y axes we can use
               * the same method as in case no. 1.
               */
              ClayLevelsConfig *config_inv;
              gdouble           dy = 0;
              gdouble           y0;
              const gdouble     gamma_inv = 1 / gamma;

              config_inv = clay_config_duplicate (CLAY_CONFIG (config));

              config_inv->gamma[channel]       = gamma_inv;
              config_inv->low_input[channel]   = config->low_output[channel];
              config_inv->low_output[channel]  = config->low_input[channel];
              config_inv->high_input[channel]  = config->high_output[channel];
              config_inv->high_output[channel] = config->high_input[channel];

              for (i = 0; i < n; ++i)
                dy = dy * gamma_inv + 1;
              y0 = delta_out / dy;

              dy = 0;
              for (i = 1; i < n; ++i)
                {
                  dy = dy * gamma_inv + y0;
                  y = config->low_output[channel] + dy;
                  x = config->low_input[channel] + delta_in *
                      clay_operation_levels_map_input (config_inv, channel, y);
                  clay_curve_add_point (curve, x, y);
                }

              g_object_unref (config_inv);
            }
        }

      x = config->high_input[channel];
      y = config->high_output[channel];

      clay_curve_add_point (curve, x, y);
    }

  return curves;
}

gboolean
clay_levels_config_load_cruft (ClayLevelsConfig  *config,
                               GInputStream      *input,
                               GError           **error)
{
  GDataInputStream *data_input;
  gint              low_input[5];
  gint              high_input[5];
  gint              low_output[5];
  gint              high_output[5];
  gdouble           gamma[5];
  gchar            *line;
  gsize             line_len;
  gint              i;

  g_return_val_if_fail (CLAY_IS_LEVELS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  line_len = 64;
  line = clay_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    return FALSE;

  if (strcmp (line, "# Clay Levels File") != 0)
    {
      g_set_error_literal (error, CLAY_CONFIG_ERROR, CLAY_CONFIG_ERROR_PARSE,
                           _("not a Clay Levels file"));
      g_object_unref (data_input);
      g_free (line);
      return FALSE;
    }

  g_free (line);

  for (i = 0; i < 5; i++)
    {
      gchar  float_buf[32];
      gchar *endp;
      gint   fields;

      line_len = 64;
      line = clay_data_input_stream_read_line_always (data_input, &line_len,
                                                      NULL, error);
      if (! line)
        {
          g_object_unref (data_input);
          return FALSE;
        }

#ifndef _UCRT
      fields = sscanf (line, "%d %d %d %d %31s",
                       &low_input[i],
                       &high_input[i],
                       &low_output[i],
                       &high_output[i],
                       float_buf);
#else
     fields = sscanf_s (line, "%d %d %d %d %31s",
                        &low_input[i],
                        &high_input[i],
                        &low_output[i],
                        &high_output[i],
                        float_buf, (unsigned) sizeof (float_buf));
#endif

      g_free (line);

      if (fields != 5)
        goto error;

      gamma[i] = g_ascii_strtod (float_buf, &endp);

      if (endp == float_buf || errno == ERANGE)
        goto error;
    }

  g_object_unref (data_input);

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      config->low_input[i]   = low_input[i]   / 255.0;
      config->high_input[i]  = high_input[i]  / 255.0;
      config->gamma[i]       = gamma[i];
      config->low_output[i]  = low_output[i]  / 255.0;
      config->high_output[i] = high_output[i] / 255.0;
    }

  config->trc          = CLAY_TRC_NON_LINEAR;
  config->clamp_input  = TRUE;
  config->clamp_output = TRUE;

  g_object_notify (G_OBJECT (config), "trc");
  g_object_notify (G_OBJECT (config), "low-input");
  g_object_notify (G_OBJECT (config), "high-input");
  g_object_notify (G_OBJECT (config), "clamp-input");
  g_object_notify (G_OBJECT (config), "gamma");
  g_object_notify (G_OBJECT (config), "low-output");
  g_object_notify (G_OBJECT (config), "high-output");
  g_object_notify (G_OBJECT (config), "clamp-output");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;

 error:
  g_object_unref (data_input);

  g_set_error_literal (error, CLAY_CONFIG_ERROR, CLAY_CONFIG_ERROR_PARSE,
                       _("parse error"));
  return FALSE;
}

gboolean
clay_levels_config_save_cruft (ClayLevelsConfig  *config,
                               GOutputStream     *output,
                               GError           **error)
{
  GString *string;
  gint     i;

  g_return_val_if_fail (CLAY_IS_LEVELS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  string = g_string_new ("# Clay Levels File\n");

  for (i = 0; i < 5; i++)
    {
      gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

      g_string_append_printf (string,
                              "%d %d %d %d %s\n",
                              (gint) (config->low_input[i]   * 255.999),
                              (gint) (config->high_input[i]  * 255.999),
                              (gint) (config->low_output[i]  * 255.999),
                              (gint) (config->high_output[i] * 255.999),
                              g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE,
                                              config->gamma[i]));
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_prefix_error (error, _("Writing levels file failed: "));
      g_string_free (string, TRUE);
      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

gboolean
clay_levels_config_load_alv (ClayLevelsConfig  *config,
                             GInputStream      *input,
                             GError           **error)
{
  GDataInputStream *data_input;
  guint16           version = 0;
  gint              low_input[4];
  gint              high_input[4];
  gint              low_output[4];
  gint              high_output[4];
  gdouble           gamma[4];

  g_return_val_if_fail (CLAY_IS_LEVELS_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  version = g_data_input_stream_read_uint16 (data_input, NULL, error);
  if (! version)
    goto error;

  /* TODO: Photoshop Level presets store 29 levels, for channels beyond
   * RGB. When we can support those, we should update the read limit */
  for (gint i = 0; i < 4; i++)
    {
      low_input[i] = g_data_input_stream_read_uint16 (data_input, NULL, error);
      if (error && *error)
        goto error;

      high_input[i] = g_data_input_stream_read_uint16 (data_input, NULL,
                                                       error);
      if (error && *error)
        goto error;

      low_output[i] = g_data_input_stream_read_uint16 (data_input, NULL,
                                                       error);
      if (error && *error)
        goto error;

      high_output[i] = g_data_input_stream_read_uint16 (data_input, NULL,
                                                        error);
      if (error && *error)
        goto error;

      gamma[i] = g_data_input_stream_read_int16 (data_input, NULL, error);
      if (error && *error)
        goto error;
    }

  g_object_unref (data_input);

  g_object_freeze_notify (G_OBJECT (config));

  for (gint i = 0; i < 4; i++)
    {
      config->low_input[i]   = low_input[i]   / 253.0;
      config->high_input[i]  = high_input[i]  / 255.0;
      config->gamma[i]       = gamma[i]       / 100.0;
      config->low_output[i]  = low_output[i]  / 255.0;
      config->high_output[i] = high_output[i] / 255.0;
    }

  config->trc          = CLAY_TRC_NON_LINEAR;
  config->clamp_input  = TRUE;
  config->clamp_output = TRUE;

  g_object_notify (G_OBJECT (config), "trc");
  g_object_notify (G_OBJECT (config), "low-input");
  g_object_notify (G_OBJECT (config), "high-input");
  g_object_notify (G_OBJECT (config), "clamp-input");
  g_object_notify (G_OBJECT (config), "gamma");
  g_object_notify (G_OBJECT (config), "low-output");
  g_object_notify (G_OBJECT (config), "high-output");
  g_object_notify (G_OBJECT (config), "clamp-output");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;

  error:
    g_object_unref (data_input);

    if (error && *error)
      g_prefix_error (error, _("parse error"));
    else
      g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                   _("parse error"));
  return FALSE;
}
