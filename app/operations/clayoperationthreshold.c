/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationthreshold.c
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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaycolor/claycolor-private.h"
#include "libclayconfig/clayconfig.h"

#include "operations-types.h"

#include "clayoperationthreshold.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_CHANNEL,
  PROP_LOW,
  PROP_HIGH
};


static void     clay_operation_threshold_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void     clay_operation_threshold_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);

static gboolean clay_operation_threshold_process (GeglOperation       *operation,
                                                  void                *in_buf,
                                                  void                *out_buf,
                                                  glong                samples,
                                                  const GeglRectangle *roi,
                                                  gint                 level);

static void clay_operation_threshold_prepare (GeglOperation *operation);

G_DEFINE_TYPE (ClayOperationThreshold, clay_operation_threshold,
               CLAY_TYPE_OPERATION_POINT_FILTER)

#define parent_class clay_operation_threshold_parent_class


static void
clay_operation_threshold_class_init (ClayOperationThresholdClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property = clay_operation_threshold_set_property;
  object_class->get_property = clay_operation_threshold_get_property;

  operation_class->prepare   = clay_operation_threshold_prepare;

  point_class->process       = clay_operation_threshold_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "clay:threshold",
                                 "title",       _("Threshold"),
                                 "categories",  "color",
                                 "description", _("Reduce image to two colors using a threshold"),
                                 NULL);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_CHANNEL,
                         "channel",
                         _("Channel"),
                         NULL,
                         CLAY_TYPE_HISTOGRAM_CHANNEL,
                         CLAY_HISTOGRAM_VALUE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_LOW,
                           "low",
                           _("Low threshold"),
                           NULL,
                           0.0, 1.0, 0.5,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_HIGH,
                           "high",
                           _("High threshold"),
                           NULL,
                           0.0, 1.0, 1.0,
                           CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_operation_threshold_init (ClayOperationThreshold *self)
{
}

static void
clay_operation_threshold_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayOperationThreshold *self = CLAY_OPERATION_THRESHOLD (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      g_value_set_enum (value, self->channel);
      break;

    case PROP_LOW:
      g_value_set_double (value, self->low);
      break;

    case PROP_HIGH:
      g_value_set_double (value, self->high);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_threshold_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayOperationThreshold *self = CLAY_OPERATION_THRESHOLD (object);

  switch (property_id)
    {
    case PROP_CHANNEL:
      self->channel = g_value_get_enum (value);
      break;

    case PROP_LOW:
      self->low = g_value_get_double (value);
      break;

    case PROP_HIGH:
      self->high = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_threshold_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format_with_space (
      "R'G'B'A float", gegl_operation_get_format (operation, "input"));

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
clay_operation_threshold_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  ClayOperationThreshold *threshold = CLAY_OPERATION_THRESHOLD (operation);
  gfloat                 *src       = in_buf;
  gfloat                 *dest      = out_buf;

  while (samples--)
    {
      gfloat value = 0.0;

      switch (threshold->channel)
        {
        case CLAY_HISTOGRAM_VALUE:
          value = MAX (src[RED], src[GREEN]);
          value = MAX (value, src[BLUE]);
          break;

        case CLAY_HISTOGRAM_RED:
          value = src[RED];
          break;

        case CLAY_HISTOGRAM_GREEN:
          value = src[GREEN];
          break;

        case CLAY_HISTOGRAM_BLUE:
          value = src[BLUE];
          break;

        case CLAY_HISTOGRAM_ALPHA:
          value = src[ALPHA];
          break;

        case CLAY_HISTOGRAM_RGB:
          value = MIN (src[RED], src[GREEN]);
          value = MIN (value, src[BLUE]);
          break;

        case CLAY_HISTOGRAM_LUMINANCE:
          value = CLAY_RGB_LUMINANCE (src[RED], src[GREEN], src[BLUE]);
          break;
        }

      value = (value >= threshold->low && value <= threshold->high) ? 1.0 : 0.0;

      dest[RED]   = value;
      dest[GREEN] = value;
      dest[BLUE]  = value;
      dest[ALPHA] = src[ALPHA];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}
