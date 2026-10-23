/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycurvesconfig.c
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
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "core/clay-utils.h"
#include "core/claycurve.h"
#include "core/clayhistogram.h"

#include "claycurvesconfig.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_TRC,
  PROP_CHANNEL,
  PROP_CURVE
};


static void     clay_curves_config_iface_init   (ClayConfigInterface *iface);

static void     clay_curves_config_finalize     (GObject          *object);
static void     clay_curves_config_get_property (GObject          *object,
                                                 guint             property_id,
                                                 GValue           *value,
                                                 GParamSpec       *pspec);
static void     clay_curves_config_set_property (GObject          *object,
                                                 guint             property_id,
                                                 const GValue     *value,
                                                 GParamSpec       *pspec);

static gboolean clay_curves_config_serialize    (ClayConfig       *config,
                                                 ClayConfigWriter *writer,
                                                 gpointer          data);
static gboolean clay_curves_config_deserialize  (ClayConfig       *config,
                                                 GScanner         *scanner,
                                                 gint              nest_level,
                                                 gpointer          data);
static gboolean clay_curves_config_equal        (ClayConfig       *a,
                                                 ClayConfig       *b);
static void     clay_curves_config_reset        (ClayConfig       *config);
static gboolean clay_curves_config_copy         (ClayConfig       *src,
                                                 ClayConfig       *dest,
                                                 GParamFlags       flags);

static void     clay_curves_config_curve_dirty  (ClayCurve        *curve,
                                                 ClayCurvesConfig *config);


G_DEFINE_TYPE_WITH_CODE (ClayCurvesConfig, clay_curves_config,
                         CLAY_TYPE_OPERATION_SETTINGS,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_curves_config_iface_init))

#define parent_class clay_curves_config_parent_class


static void
clay_curves_config_class_init (ClayCurvesConfigClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class = CLAY_VIEWABLE_CLASS (klass);

  object_class->finalize            = clay_curves_config_finalize;
  object_class->set_property        = clay_curves_config_set_property;
  object_class->get_property        = clay_curves_config_get_property;

  viewable_class->default_icon_name = "clay-tool-curves";

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_TRC,
                         "trc",
                         _("Tone Reproduction Curve"),
                         _("Work on linear or perceptual RGB, or following the image's TRC"),
                         CLAY_TYPE_TRC_TYPE,
                         CLAY_TRC_PERCEPTUAL, 0);

  /* Only channels CLAY_HISTOGRAM_VALUE to CLAY_HISTOGRAM_ALPHA are
   * supported right now in this op. Unfortunately this is not visible
   * through the param specification. The ClayParamSpecEnum would allow
   * to make it visible, but it does not exist on libclay side right
   * now, so plug-in writers would see it listed as allowed.
   *
   * TODO: ClayParamSpecEnum should be moved to libclaybase
   * (libclaybase/clayparamspecs.h) and passed through PDB.
   */
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         _("The affected channel"),
                         CLAY_TYPE_HISTOGRAM_CHANNEL,
                         CLAY_HISTOGRAM_VALUE, 0);

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_CURVE,
                           "curve",
                           _("Curve"),
                           _("Curve"),
                           CLAY_TYPE_CURVE,
                           CLAY_CONFIG_PARAM_AGGREGATE);
}

static void
clay_curves_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_curves_config_serialize;
  iface->deserialize = clay_curves_config_deserialize;
  iface->equal       = clay_curves_config_equal;
  iface->reset       = clay_curves_config_reset;
  iface->copy        = clay_curves_config_copy;
}

static void
clay_curves_config_init (ClayCurvesConfig *self)
{
  ClayHistogramChannel channel;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      self->curve[channel] = CLAY_CURVE (clay_curve_new ("curves config"));

      g_signal_connect_object (self->curve[channel], "dirty",
                               G_CALLBACK (clay_curves_config_curve_dirty),
                               self, 0);
    }

  clay_config_reset (CLAY_CONFIG (self));
}

static void
clay_curves_config_finalize (GObject *object)
{
  ClayCurvesConfig     *self = CLAY_CURVES_CONFIG (object);
  ClayHistogramChannel  channel;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      g_object_unref (self->curve[channel]);
      self->curve[channel] = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_curves_config_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayCurvesConfig *self = CLAY_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      g_value_set_enum (value, self->trc);
      break;

    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_CURVE:
      g_value_set_object (value, self->curve[self->channel]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_curves_config_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayCurvesConfig *self = CLAY_CURVES_CONFIG (object);

  switch (property_id)
    {
    case PROP_TRC:
      self->trc = g_value_get_enum (value);
      break;

    case PROP_CHANNEL:
      if (g_value_get_enum (value) >= CLAY_HISTOGRAM_VALUE &&
          g_value_get_enum (value) <= CLAY_HISTOGRAM_ALPHA)
        {
          self->channel = g_value_get_enum (value);
          g_object_notify (object, "curve");
        }
      break;

    case PROP_CURVE:
      {
        ClayCurve *src_curve  = g_value_get_object (value);
        ClayCurve *dest_curve = self->curve[self->channel];

        if (src_curve && dest_curve)
          {
            clay_config_copy (CLAY_CONFIG (src_curve),
                              CLAY_CONFIG (dest_curve), 0);
          }
      }
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_curves_config_serialize (ClayConfig       *config,
                              ClayConfigWriter *writer,
                              gpointer          data)
{
  ClayCurvesConfig     *c_config = CLAY_CURVES_CONFIG (config);
  ClayHistogramChannel  channel;
  ClayHistogramChannel  old_channel;
  gboolean              success = TRUE;

  if (! clay_operation_settings_config_serialize_base (config, writer, data) ||
      ! clay_config_serialize_property_by_name (config, "trc", writer))
    return FALSE;

  old_channel = c_config->channel;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;

      /*  serialize the channel properties manually (not using
       *  clay_config_serialize_properties()), so the parent class'
       *  properties don't end up in the config file one per channel.
       *  See bug #700653.
       */
      success =
        (clay_config_serialize_property_by_name (config, "channel", writer) &&
         clay_config_serialize_property_by_name (config, "curve",   writer));

      if (! success)
        break;
    }

  c_config->channel = old_channel;

  return success;
}

static gboolean
clay_curves_config_deserialize (ClayConfig *config,
                                GScanner   *scanner,
                                gint        nest_level,
                                gpointer    data)
{
  ClayCurvesConfig     *c_config = CLAY_CURVES_CONFIG (config);
  ClayHistogramChannel  old_channel;
  gboolean              success = TRUE;

  old_channel = c_config->channel;

  success = clay_config_deserialize_properties (config, scanner, nest_level);

  g_object_set (config, "channel", old_channel, NULL);

  return success;
}

static gboolean
clay_curves_config_equal (ClayConfig *a,
                          ClayConfig *b)
{
  ClayCurvesConfig     *config_a = CLAY_CURVES_CONFIG (a);
  ClayCurvesConfig     *config_b = CLAY_CURVES_CONFIG (b);
  ClayHistogramChannel  channel;

  if (! clay_operation_settings_config_equal_base (a, b) ||
      config_a->trc != config_b->trc)
    return FALSE;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      ClayCurve *curve_a = config_a->curve[channel];
      ClayCurve *curve_b = config_b->curve[channel];

      if (curve_a && curve_b)
        {
          if (! clay_config_is_equal_to (CLAY_CONFIG (curve_a),
                                         CLAY_CONFIG (curve_b)))
            return FALSE;
        }
      else if (curve_a || curve_b)
        {
          return FALSE;
        }
    }

  /* don't compare "channel" */

  return TRUE;
}

static void
clay_curves_config_reset (ClayConfig *config)
{
  ClayCurvesConfig     *c_config = CLAY_CURVES_CONFIG (config);
  ClayHistogramChannel  channel;

  clay_operation_settings_config_reset_base (config);

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      c_config->channel = channel;
      clay_curves_config_reset_channel (c_config);
    }

  clay_config_reset_property (G_OBJECT (config), "trc");
  clay_config_reset_property (G_OBJECT (config), "channel");
}

static gboolean
clay_curves_config_copy (ClayConfig  *src,
                         ClayConfig  *dest,
                         GParamFlags  flags)
{
  ClayCurvesConfig     *src_config  = CLAY_CURVES_CONFIG (src);
  ClayCurvesConfig     *dest_config = CLAY_CURVES_CONFIG (dest);
  ClayHistogramChannel  channel;

  if (! clay_operation_settings_config_copy_base (src, dest, flags))
    return FALSE;

  for (channel = CLAY_HISTOGRAM_VALUE;
       channel <= CLAY_HISTOGRAM_ALPHA;
       channel++)
    {
      clay_config_copy (CLAY_CONFIG (src_config->curve[channel]),
                        CLAY_CONFIG (dest_config->curve[channel]),
                        flags);
    }

  dest_config->trc     = src_config->trc;
  dest_config->channel = src_config->channel;

  g_object_notify (G_OBJECT (dest), "trc");
  g_object_notify (G_OBJECT (dest), "channel");

  return TRUE;
}

static void
clay_curves_config_curve_dirty (ClayCurve        *curve,
                                ClayCurvesConfig *config)
{
  g_object_notify (G_OBJECT (config), "curve");
}


/*  public functions  */

GObject *
clay_curves_config_new_spline (gint32         channel,
                               const gdouble *points,
                               gint           n_points)
{
  ClayCurvesConfig *config;
  ClayCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= CLAY_HISTOGRAM_VALUE &&
                        channel <= CLAY_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  config = g_object_new (CLAY_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  clay_data_freeze (CLAY_DATA (curve));

  clay_curve_set_curve_type (curve, CLAY_CURVE_SMOOTH);
  clay_curve_clear_points (curve);

  for (i = 0; i < n_points; i++)
    clay_curve_add_point (curve,
                          (gdouble) points[i * 2],
                          (gdouble) points[i * 2 + 1]);

  clay_data_thaw (CLAY_DATA (curve));

  return G_OBJECT (config);
}

GObject *
clay_curves_config_new_explicit (gint32         channel,
                                 const gdouble *samples,
                                 gint           n_samples)
{
  ClayCurvesConfig *config;
  ClayCurve        *curve;
  gint              i;

  g_return_val_if_fail (channel >= CLAY_HISTOGRAM_VALUE &&
                        channel <= CLAY_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  config = g_object_new (CLAY_TYPE_CURVES_CONFIG, NULL);

  curve = config->curve[channel];

  clay_data_freeze (CLAY_DATA (curve));

  clay_curve_set_curve_type (curve, CLAY_CURVE_FREE);
  clay_curve_set_n_samples (curve, n_samples);

  for (i = 0; i < n_samples; i++)
    clay_curve_set_sample (curve,
                           (gdouble) i / (gdouble) (n_samples - 1),
                           (gdouble) samples[i]);

  clay_data_thaw (CLAY_DATA (curve));

  return G_OBJECT (config);
}

GObject *
clay_curves_config_new_spline_cruft (gint32        channel,
                                     const guint8 *points,
                                     gint          n_points)
{
  GObject *config;
  gdouble *d_points;
  gint     i;

  g_return_val_if_fail (channel >= CLAY_HISTOGRAM_VALUE &&
                        channel <= CLAY_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (points != NULL, NULL);
  g_return_val_if_fail (n_points >= 2 && n_points <= 1024, NULL);

  d_points = g_new (gdouble, 2 * n_points);

  for (i = 0; i < n_points; i++)
    {
      d_points[i * 2]     = (gdouble) points[i * 2]     / 255.0;
      d_points[i * 2 + 1] = (gdouble) points[i * 2 + 1] / 255.0;
    }

  config = clay_curves_config_new_spline (channel, d_points, n_points);

  g_free (d_points);

  return config;
}

GObject *
clay_curves_config_new_explicit_cruft (gint32        channel,
                                       const guint8 *samples,
                                       gint          n_samples)
{
  GObject *config;
  gdouble *d_samples;
  gint     i;

  g_return_val_if_fail (channel >= CLAY_HISTOGRAM_VALUE &&
                        channel <= CLAY_HISTOGRAM_ALPHA, NULL);
  g_return_val_if_fail (samples != NULL, NULL);
  g_return_val_if_fail (n_samples >= 2 && n_samples <= 4096, NULL);

  d_samples = g_new (gdouble, n_samples);

  for (i = 0; i < n_samples; i++)
    {
      d_samples[i] = (gdouble) samples[i] / 255.0;
    }

  config = clay_curves_config_new_explicit (channel, d_samples, n_samples);

  g_free (d_samples);

  return config;
}

void
clay_curves_config_reset_channel (ClayCurvesConfig *config)
{
  g_return_if_fail (CLAY_IS_CURVES_CONFIG (config));

  clay_config_reset (CLAY_CONFIG (config->curve[config->channel]));
}

#define CLAY_CURVE_N_CRUFT_POINTS 17

gboolean
clay_curves_config_load_cruft (ClayCurvesConfig  *config,
                               GInputStream      *input,
                               GError           **error)
{
  GDataInputStream *data_input;
  gint              index[5][CLAY_CURVE_N_CRUFT_POINTS];
  gint              value[5][CLAY_CURVE_N_CRUFT_POINTS];
  gchar            *line;
  gsize             line_len;
  gint              i, j;

  g_return_val_if_fail (CLAY_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  line_len = 64;
  line = clay_data_input_stream_read_line_always (data_input, &line_len,
                                                  NULL, error);
  if (! line)
    return FALSE;

  if (strcmp (line, "# Clay Curves File") != 0)
    {
      g_set_error_literal (error, CLAY_CONFIG_ERROR, CLAY_CONFIG_ERROR_PARSE,
                           _("not a Clay Curves file"));
      g_object_unref (data_input);
      g_free (line);
      return FALSE;
    }

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < CLAY_CURVE_N_CRUFT_POINTS; j++)
        {
          gchar *x_str = NULL;
          gchar *y_str = NULL;

          if (! (x_str = g_data_input_stream_read_upto (data_input, " ", -1,
                                                        NULL, NULL, error)) ||
              ! g_data_input_stream_read_byte (data_input,  NULL, error) ||
              ! (y_str = g_data_input_stream_read_upto (data_input, " ", -1,
                                                        NULL, NULL, error)) ||
              ! g_data_input_stream_read_byte (data_input,  NULL, error))
            {
              g_free (x_str);
              g_free (y_str);
              g_object_unref (data_input);
              return FALSE;
            }

#ifndef _UCRT
          if (sscanf (x_str, "%d", &index[i][j]) != 1 ||
              sscanf (y_str, "%d", &value[i][j]) != 1)
#else
          if (sscanf_s (x_str, "%d", &index[i][j]) != 1 ||
              sscanf_s (y_str, "%d", &value[i][j]) != 1)
#endif
            {
              g_set_error_literal (error,
                                   CLAY_CONFIG_ERROR, CLAY_CONFIG_ERROR_PARSE,
                                   _("Parse error, didn't find 2 integers"));
              g_free (x_str);
              g_free (y_str);
              g_object_unref (data_input);
              return FALSE;
            }

          g_free (x_str);
          g_free (y_str);
        }
    }

  g_object_unref (data_input);

  g_object_freeze_notify (G_OBJECT (config));

  for (i = 0; i < 5; i++)
    {
      ClayCurve *curve = config->curve[i];

      clay_data_freeze (CLAY_DATA (curve));

      clay_curve_set_curve_type (curve, CLAY_CURVE_SMOOTH);
      clay_curve_clear_points (curve);

      for (j = 0; j < CLAY_CURVE_N_CRUFT_POINTS; j++)
        {
          gdouble x;
          gdouble y;

          x = (gdouble) index[i][j] / 255.0;
          y = (gdouble) value[i][j] / 255.0;

          if (x >= 0.0)
            clay_curve_add_point (curve, x, y);
        }

      clay_data_thaw (CLAY_DATA (curve));
    }

  config->trc = CLAY_TRC_NON_LINEAR;

  g_object_notify (G_OBJECT (config), "trc");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;
}

gboolean
clay_curves_config_save_cruft (ClayCurvesConfig  *config,
                               GOutputStream     *output,
                               GError           **error)
{
  GString *string;
  gint     i;

  g_return_val_if_fail (CLAY_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_OUTPUT_STREAM (output), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  string = g_string_new ("# Clay Curves File\n");

  for (i = 0; i < 5; i++)
    {
      ClayCurve *curve = config->curve[i];
      gint       j;

      if (curve->curve_type == CLAY_CURVE_SMOOTH)
        {
          g_object_ref (curve);
        }
      else
        {
          curve = CLAY_CURVE (clay_data_duplicate (CLAY_DATA (curve)));

          clay_curve_set_curve_type (curve, CLAY_CURVE_SMOOTH);
        }

      for (j = 0; j < CLAY_CURVE_N_CRUFT_POINTS; j++)
        {
          gint x = -1;
          gint y = -1;

          if (j < clay_curve_get_n_points (curve))
            {
              gdouble point_x;
              gdouble point_y;

              clay_curve_get_point (curve, j, &point_x, &point_y);

              x = floor (point_x * 255.999);
              y = floor (point_y * 255.999);
            }

          g_string_append_printf (string, "%d %d ", x, y);
        }

      g_string_append_printf (string, "\n");

      g_object_unref (curve);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_prefix_error (error, _("Writing curves file failed: "));
      g_string_free (string, TRUE);
      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}

#define PS_CURVE_N_MAX_POINTS 19

/* Loads Photoshop .acv Curves preset */
gboolean
clay_curves_config_load_acv (ClayCurvesConfig  *config,
                             GInputStream      *input,
                             GError           **error)
{
  GDataInputStream *data_input;
  guint16           version = 0;
  guint16           count   = 0;
  gint              in[5][PS_CURVE_N_MAX_POINTS];
  gint              out[5][PS_CURVE_N_MAX_POINTS];

  g_return_val_if_fail (CLAY_IS_CURVES_CONFIG (config), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (input), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  data_input = g_data_input_stream_new (input);

  g_data_input_stream_set_byte_order (data_input,
                                      G_DATA_STREAM_BYTE_ORDER_BIG_ENDIAN);

  version = g_data_input_stream_read_uint16 (data_input, NULL, error);
  if (! version)
    goto error;

  count = g_data_input_stream_read_uint16 (data_input, NULL, error);
  if (! count)
    goto error;

  count = CLAMP (count, 1, 5);
  for (gint i = 0; i < count; i++)
    {
      guint16 points = 0;

      points = g_data_input_stream_read_uint16 (data_input, NULL, error);
      if (! points || points > PS_CURVE_N_MAX_POINTS)
        goto error;

      for (gint j = 0; j < points; j++)
        {
          /* Curves can start at 0, 0, so we'll check for an error instead */
          out[i][j] = g_data_input_stream_read_uint16 (data_input, NULL, error);
          if (error && *error)
            goto error;

          in[i][j] = g_data_input_stream_read_uint16 (data_input, NULL, error);
          if (error && *error)
            goto error;
        }
      if (points < PS_CURVE_N_MAX_POINTS)
        {
          in[i][points]  = -1;
          out[i][points] = -1;
        }
    }

  g_object_unref (data_input);
  g_object_freeze_notify (G_OBJECT (config));

  for (gint i = 0; i < count; i++)
    {
      ClayCurve *curve = config->curve[i];

      clay_data_freeze (CLAY_DATA (curve));

      clay_curve_set_curve_type (curve, CLAY_CURVE_SMOOTH);
      clay_curve_clear_points (curve);

      for (gint j = 0; j < PS_CURVE_N_MAX_POINTS; j++)
        {
          if (in[i][j] > -1 && out[i][j] > -1)
            clay_curve_add_point (curve, in[i][j] / 255.0, out[i][j] / 255.0);
          else
            break;
        }
      clay_data_thaw (CLAY_DATA (curve));
    }

  config->trc = CLAY_TRC_NON_LINEAR;
  g_object_notify (G_OBJECT (config), "trc");

  g_object_thaw_notify (G_OBJECT (config));

  return TRUE;

  error:
      if (error && *error)
        g_prefix_error (error, _("Could not read header: "));
      else
        g_set_error (error, CLAY_DATA_ERROR, CLAY_DATA_ERROR_READ,
                     _("Could not read header: "));
    g_object_unref (data_input);

    return FALSE;
}
