/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#include <string.h>
#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"
#include "libclaycolor/claycolor-private.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "clay-atomic.h"
#include "clay-parallel.h"
#include "clayasync.h"
#include "claychannel.h"
#include "claydrawable.h"
#include "clayhistogram.h"
#include "clayimage.h"
#include "claywaitable.h"


#define N_DERIVED_CHANNELS 2

#define PIXELS_PER_THREAD \
  (/* each thread costs as much as */ 64.0 * 64.0 /* pixels */)


enum
{
  PROP_0,
  PROP_N_COMPONENTS,
  PROP_N_BINS,
  PROP_VALUES
};

struct _ClayHistogramPrivate
{
  ClayTRCType  trc;
  gint         n_channels;
  gint         n_bins;
  gdouble     *values;
  ClayAsync   *calculate_async;
};

typedef struct
{
  /*  input  */
  ClayHistogram *histogram;
  GeglBuffer    *buffer;
  GeglRectangle  buffer_rect;
  GeglBuffer    *mask;
  GeglRectangle  mask_rect;

  /*  output  */
  gint           n_components;
  gint           n_bins;
  gdouble       *values;
} CalculateContext;

typedef struct
{
  ClayAsync        *async;
  CalculateContext *context;

  const Babl       *format;
  GSList           *values_list;
} CalculateData;


/*  local function prototypes  */

static void       clay_histogram_finalize                 (GObject              *object);
static void       clay_histogram_set_property             (GObject              *object,
                                                           guint                 property_id,
                                                           const GValue         *value,
                                                           GParamSpec           *pspec);
static void       clay_histogram_get_property             (GObject              *object,
                                                           guint                 property_id,
                                                           GValue               *value,
                                                           GParamSpec           *pspec);

static gint64     clay_histogram_get_memsize              (ClayObject           *object,
                                                           gint64               *gui_size);

static gboolean   clay_histogram_map_channel              (ClayHistogram        *histogram,
                                                           ClayHistogramChannel *channel);

static void       clay_histogram_set_values               (ClayHistogram        *histogram,
                                                           gint                  n_components,
                                                           gint                  n_bins,
                                                           gdouble              *values);

static void       clay_histogram_calculate_internal       (ClayAsync            *async,
                                                           CalculateContext     *context);
static void       clay_histogram_calculate_area           (const GeglRectangle  *area,
                                                           CalculateData        *data);
static void       clay_histogram_calculate_async_callback (ClayAsync            *async,
                                                           CalculateContext     *context);
static guint      hash_color_bytes                        (gpointer             *key);
static gboolean   color_bytes_equal                       (gpointer             *key1,
                                                           gpointer             *key2);


G_DEFINE_TYPE_WITH_PRIVATE (ClayHistogram, clay_histogram, CLAY_TYPE_OBJECT)

#define parent_class clay_histogram_parent_class


static void
clay_histogram_class_init (ClayHistogramClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_histogram_finalize;
  object_class->set_property     = clay_histogram_set_property;
  object_class->get_property     = clay_histogram_get_property;

  clay_object_class->get_memsize = clay_histogram_get_memsize;

  g_object_class_install_property (object_class, PROP_N_COMPONENTS,
                                   g_param_spec_int ("n-components", NULL, NULL,
                                                     0, MAX_CHANNELS, 0,
                                                     CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_N_BINS,
                                   g_param_spec_int ("n-bins", NULL, NULL,
                                                     256, 1024, 1024,
                                                     CLAY_PARAM_READABLE));

  /* this is just for notifications */
  g_object_class_install_property (object_class, PROP_VALUES,
                                   g_param_spec_boolean ("values", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READABLE));
}

static void
clay_histogram_init (ClayHistogram *histogram)
{
  histogram->priv = clay_histogram_get_instance_private (histogram);
}

static void
clay_histogram_finalize (GObject *object)
{
  ClayHistogram *histogram = CLAY_HISTOGRAM (object);

  clay_histogram_clear_values (histogram, 0);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_histogram_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_histogram_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayHistogram *histogram = CLAY_HISTOGRAM (object);

  switch (property_id)
    {
    case PROP_N_COMPONENTS:
      g_value_set_int (value, clay_histogram_n_components (histogram));
      break;

    case PROP_N_BINS:
      g_value_set_int (value, histogram->priv->n_bins);
      break;

    case PROP_VALUES:
      /* return a silly boolean */
      g_value_set_boolean (value, histogram->priv->values != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_histogram_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayHistogram *histogram = CLAY_HISTOGRAM (object);
  gint64         memsize   = 0;

  if (histogram->priv->values)
    memsize += (histogram->priv->n_channels *
                histogram->priv->n_bins * sizeof (gdouble));

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static guint
hash_color_bytes (gpointer *key)
{
  GBytes       *bytes     = (GBytes *) key;
  gsize         size;
  const guint8 *data;
  gdouble       max_value = 0.0;
  gdouble       value     = 0.0;
  guint         hash;

  data = g_bytes_get_data (bytes, &size);

  for (gsize i = 0; i < size; ++i)
    {
      value     = value * 256.0 + data[i];
      max_value = max_value * 256.0 + 255.0;
    }

  if (max_value == 0)
    return 0;

  hash = (guint) (value * G_MAXUINT / max_value);

  return hash;
}

static gboolean
color_bytes_equal (gpointer *key1,
                   gpointer *key2)
{
  GBytes       *bytes1 = (GBytes *) key1;
  GBytes       *bytes2 = (GBytes *) key2;
  gsize         size1;
  gsize         size2;
  const guint8 *data1  = g_bytes_get_data (bytes1, &size1);
  const guint8 *data2  = g_bytes_get_data (bytes2, &size2);

  if (size1 != size2)
    return FALSE;

  return memcmp (data1, data2, size1) == 0;
}

/*  public functions  */

ClayHistogram *
clay_histogram_new (ClayTRCType trc)
{
  ClayHistogram *histogram = g_object_new (CLAY_TYPE_HISTOGRAM, NULL);

  histogram->priv->trc = trc;

  return histogram;
}

/**
 * clay_histogram_duplicate:
 * @histogram: a %ClayHistogram
 *
 * Creates a duplicate of @histogram. The duplicate has a reference
 * count of 1 and contains the values from @histogram.
 *
 * Returns: a newly allocated %ClayHistogram
 **/
ClayHistogram *
clay_histogram_duplicate (ClayHistogram *histogram)
{
  ClayHistogram *dup;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), NULL);

  if (histogram->priv->calculate_async)
    clay_waitable_wait (CLAY_WAITABLE (histogram->priv->calculate_async));

  dup = clay_histogram_new (histogram->priv->trc);

  dup->priv->n_channels = histogram->priv->n_channels;
  dup->priv->n_bins     = histogram->priv->n_bins;
  dup->priv->values     = g_memdup2 (histogram->priv->values,
                                     sizeof (gdouble) *
                                     dup->priv->n_channels *
                                     dup->priv->n_bins);

  return dup;
}

void
clay_histogram_calculate (ClayHistogram       *histogram,
                          GeglBuffer          *buffer,
                          const GeglRectangle *buffer_rect,
                          GeglBuffer          *mask,
                          const GeglRectangle *mask_rect)
{
  CalculateContext context = {};

  g_return_if_fail (CLAY_IS_HISTOGRAM (histogram));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_rect != NULL);

  if (histogram->priv->calculate_async)
    clay_async_cancel_and_wait (histogram->priv->calculate_async);

  context.histogram   = histogram;
  context.buffer      = buffer;
  context.buffer_rect = *buffer_rect;

  if (mask)
    {
      context.mask = mask;

      if (mask_rect)
        context.mask_rect = *mask_rect;
      else
        context.mask_rect = *gegl_buffer_get_extent (mask);
    }

  clay_histogram_calculate_internal (NULL, &context);

  clay_histogram_set_values (histogram,
                             context.n_components, context.n_bins,
                             context.values);
}

ClayAsync *
clay_histogram_calculate_async (ClayHistogram       *histogram,
                                GeglBuffer          *buffer,
                                const GeglRectangle *buffer_rect,
                                GeglBuffer          *mask,
                                const GeglRectangle *mask_rect)
{
  CalculateContext *context;
  GeglRectangle     rect;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (buffer_rect != NULL, NULL);

  if (histogram->priv->calculate_async)
    clay_async_cancel_and_wait (histogram->priv->calculate_async);

  gegl_rectangle_align_to_buffer (&rect, buffer_rect, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  context = g_slice_new0 (CalculateContext);

  context->histogram   = histogram;
  context->buffer      = gegl_buffer_new (&rect,
                                          gegl_buffer_get_format (buffer));
  context->buffer_rect = *buffer_rect;

  clay_gegl_buffer_copy (buffer, &rect, GEGL_ABYSS_NONE,
                         context->buffer, NULL);

  if (mask)
    {
      if (mask_rect)
        context->mask_rect = *mask_rect;
      else
        context->mask_rect = *gegl_buffer_get_extent (mask);

      gegl_rectangle_align_to_buffer (&rect, &context->mask_rect, mask,
                                      GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      context->mask = gegl_buffer_new (&rect, gegl_buffer_get_format (mask));

      clay_gegl_buffer_copy (mask, &rect, GEGL_ABYSS_NONE,
                             context->mask, NULL);
    }

  histogram->priv->calculate_async = clay_parallel_run_async (
    (ClayRunAsyncFunc) clay_histogram_calculate_internal,
    context);

  clay_async_add_callback (
    histogram->priv->calculate_async,
    (ClayAsyncCallback) clay_histogram_calculate_async_callback,
    context);

  return histogram->priv->calculate_async;
}

void
clay_histogram_clear_values (ClayHistogram *histogram,
                             gint           n_components)
{
  g_return_if_fail (CLAY_IS_HISTOGRAM (histogram));

  if (histogram->priv->calculate_async)
    clay_async_cancel_and_wait (histogram->priv->calculate_async);

  clay_histogram_set_values (histogram, n_components, 0, NULL);
}


#define HISTOGRAM_VALUE(c,i) (priv->values[(c) * priv->n_bins + (i)])


gdouble
clay_histogram_get_maximum (ClayHistogram        *histogram,
                            ClayHistogramChannel  channel)
{
  ClayHistogramPrivate *priv;
  gdouble               max = 0.0;
  gint                  x;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  if (channel == CLAY_HISTOGRAM_RGB)
    {
      for (x = 0; x < priv->n_bins; x++)
        {
          max = MAX (max, HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED,   x));
          max = MAX (max, HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, x));
          max = MAX (max, HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE,  x));
        }
    }
  else
    {
      for (x = 0; x < priv->n_bins; x++)
        {
          max = MAX (max, HISTOGRAM_VALUE (channel, x));
        }
    }

  return max;
}

gdouble
clay_histogram_get_value (ClayHistogram        *histogram,
                          ClayHistogramChannel  channel,
                          gint                  bin)
{
  ClayHistogramPrivate *priv;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values                   ||
      (bin < 0 || bin >= priv->n_bins) ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  if (channel == CLAY_HISTOGRAM_RGB)
    {
      gdouble min = HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED, bin);

      min = MIN (min, HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, bin));

      return MIN (min, HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE, bin));
    }
  else
    {
      return HISTOGRAM_VALUE (channel, bin);
    }
}

gdouble
clay_histogram_get_component (ClayHistogram *histogram,
                              gint           component,
                              gint           bin)
{
  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  if (clay_histogram_n_components (histogram) > 2)
    component++;

  return clay_histogram_get_value (histogram, component, bin);
}

gint
clay_histogram_n_components (ClayHistogram *histogram)
{
  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0);

  if (histogram->priv->n_channels > 0)
    return histogram->priv->n_channels - N_DERIVED_CHANNELS;
  else
    return 0;
}

gint
clay_histogram_n_bins (ClayHistogram *histogram)
{
  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0);

  return histogram->priv->n_bins;
}

gboolean
clay_histogram_has_channel (ClayHistogram        *histogram,
                            ClayHistogramChannel  channel)
{
  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), FALSE);

  switch (channel)
    {
    case CLAY_HISTOGRAM_VALUE:
      return TRUE;

    case CLAY_HISTOGRAM_RED:
    case CLAY_HISTOGRAM_GREEN:
    case CLAY_HISTOGRAM_BLUE:
    case CLAY_HISTOGRAM_LUMINANCE:
    case CLAY_HISTOGRAM_RGB:
      return clay_histogram_n_components (histogram) >= 3;

    case CLAY_HISTOGRAM_ALPHA:
      return clay_histogram_n_components (histogram) == 2 ||
             clay_histogram_n_components (histogram) == 4;
    }

  g_return_val_if_reached (FALSE);
}

gdouble
clay_histogram_get_count (ClayHistogram        *histogram,
                          ClayHistogramChannel  channel,
                          gint                  start,
                          gint                  end)
{
  ClayHistogramPrivate *priv;
  gint                  i;
  gdouble               count = 0.0;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  if (channel == CLAY_HISTOGRAM_RGB)
    return (clay_histogram_get_count (histogram,
                                      CLAY_HISTOGRAM_RED, start, end)   +
            clay_histogram_get_count (histogram,
                                      CLAY_HISTOGRAM_GREEN, start, end) +
            clay_histogram_get_count (histogram,
                                      CLAY_HISTOGRAM_BLUE, start, end));

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  for (i = start; i <= end; i++)
    count += HISTOGRAM_VALUE (channel, i);

  return count;
}

gdouble
clay_histogram_get_mean (ClayHistogram        *histogram,
                         ClayHistogramChannel  channel,
                         gint                  start,
                         gint                  end)
{
  ClayHistogramPrivate *priv;
  gint                  i;
  gdouble               mean = 0.0;
  gdouble               count;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  if (channel == CLAY_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        {
          gdouble factor = (gdouble) i / (gdouble)  (priv->n_bins - 1);

          mean += (factor * HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED,   i) +
                   factor * HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, i) +
                   factor * HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE,  i));
        }
    }
  else
    {
      for (i = start; i <= end; i++)
        {
          gdouble factor = (gdouble) i / (gdouble)  (priv->n_bins - 1);

          mean += factor * HISTOGRAM_VALUE (channel, i);
        }
    }

  count = clay_histogram_get_count (histogram, channel, start, end);

  if (count > 0.0)
    return mean / count;

  return mean;
}

gdouble
clay_histogram_get_median (ClayHistogram         *histogram,
                           ClayHistogramChannel   channel,
                           gint                   start,
                           gint                   end)
{
  ClayHistogramPrivate *priv;
  gint                  i;
  gdouble               sum = 0.0;
  gdouble               count;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), -1.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  count = clay_histogram_get_count (histogram, channel, start, end);

  if (channel == CLAY_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        {
          sum += (HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED,   i) +
                  HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, i) +
                  HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE,  i));

          if (sum * 2 > count)
            return ((gdouble) i / (gdouble)  (priv->n_bins - 1));
        }
    }
  else
    {
      for (i = start; i <= end; i++)
        {
          sum += HISTOGRAM_VALUE (channel, i);

          if (sum * 2 > count)
            return ((gdouble) i / (gdouble)  (priv->n_bins - 1));
        }
    }

  return -1.0;
}

/*
 * adapted from GNU ocrad 0.14 : page_image_io.cc : otsu_th
 *
 *  N. Otsu, "A threshold selection method from gray-level histograms,"
 *  IEEE Trans. Systems, Man, and Cybernetics, vol. 9, no. 1, pp. 62-66, 1979.
 */
gdouble
clay_histogram_get_threshold (ClayHistogram        *histogram,
                              ClayHistogramChannel  channel,
                              gint                  start,
                              gint                  end)
{
  ClayHistogramPrivate *priv;
  gint                 i;
  gint                 maxval;
  gdouble             *hist      = NULL;
  gdouble             *chist     = NULL;
  gdouble             *cmom      = NULL;
  gdouble              hist_max  = 0.0;
  gdouble              chist_max = 0.0;
  gdouble              cmom_max  = 0.0;
  gdouble              bvar_max  = 0.0;
  gint                 threshold = 127;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), -1);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0;
    }

  start = CLAMP (start, 0, priv->n_bins - 1);
  end   = CLAMP (end,   0, priv->n_bins - 1);

  maxval = end - start;

  hist  = g_newa (gdouble, maxval + 1);
  chist = g_newa (gdouble, maxval + 1);
  cmom  = g_newa (gdouble, maxval + 1);

  if (channel == CLAY_HISTOGRAM_RGB)
    {
      for (i = start; i <= end; i++)
        hist[i - start] = (HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED,   i) +
                           HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, i) +
                           HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE,  i));
    }
  else
    {
      for (i = start; i <= end; i++)
        hist[i - start] = HISTOGRAM_VALUE (channel, i);
    }

  hist_max = hist[0];
  chist[0] = hist[0];
  cmom[0] = 0;

  for (i = 1; i <= maxval; i++)
    {
      if (hist[i] > hist_max)
        hist_max = hist[i];

      chist[i] = chist[i-1] + hist[i];
      cmom[i] = cmom[i-1] + i * hist[i];
    }

  chist_max = chist[maxval];
  cmom_max = cmom[maxval];
  bvar_max = 0;

  for (i = 0; i < maxval; ++i)
    {
      if (chist[i] > 0 && chist[i] < chist_max)
        {
          gdouble bvar;

          bvar = (gdouble) cmom[i] / chist[i];
          bvar -= (cmom_max - cmom[i]) / (chist_max - chist[i]);
          bvar *= bvar;
          bvar *= chist[i];
          bvar *= chist_max - chist[i];

          if (bvar > bvar_max)
            {
              bvar_max = bvar;
              threshold = start + i;
            }
        }
    }

  return threshold;
}

gdouble
clay_histogram_get_std_dev (ClayHistogram        *histogram,
                            ClayHistogramChannel  channel,
                            gint                  start,
                            gint                  end)
{
  ClayHistogramPrivate *priv;
  gint                  i;
  gdouble               dev = 0.0;
  gdouble               count;
  gdouble               mean;

  g_return_val_if_fail (CLAY_IS_HISTOGRAM (histogram), 0.0);

  priv = histogram->priv;

  if (! priv->values ||
      start > end    ||
      ! clay_histogram_map_channel (histogram, &channel))
    {
      return 0.0;
    }

  mean  = clay_histogram_get_mean  (histogram, channel, start, end);
  count = clay_histogram_get_count (histogram, channel, start, end);

  if (count == 0.0)
    count = 1.0;

  for (i = start; i <= end; i++)
    {
      gdouble value;

      if (channel == CLAY_HISTOGRAM_RGB)
        {
          value = (HISTOGRAM_VALUE (CLAY_HISTOGRAM_RED,   i) +
                   HISTOGRAM_VALUE (CLAY_HISTOGRAM_GREEN, i) +
                   HISTOGRAM_VALUE (CLAY_HISTOGRAM_BLUE,  i));
        }
      else
        {
          value = clay_histogram_get_value (histogram, channel, i);
        }

      dev += value * SQR (((gdouble) i / (gdouble)  (priv->n_bins - 1)) - mean);
    }

  return sqrt (dev / count);
}


/*  private functions  */

static gboolean
clay_histogram_map_channel (ClayHistogram        *histogram,
                            ClayHistogramChannel *channel)
{
  ClayHistogramPrivate *priv = histogram->priv;

  if (*channel == CLAY_HISTOGRAM_RGB)
    return clay_histogram_n_components (histogram) >= 3;

  switch (*channel)
    {
    case CLAY_HISTOGRAM_ALPHA:
      if (clay_histogram_n_components (histogram) == 2)
        *channel = 1;
      break;

    case CLAY_HISTOGRAM_LUMINANCE:
      *channel = clay_histogram_n_components (histogram) + 1;
      break;

    default:
      break;
    }

  return *channel < priv->n_channels;
}

static void
clay_histogram_set_values (ClayHistogram *histogram,
                           gint           n_components,
                           gint           n_bins,
                           gdouble       *values)
{
  ClayHistogramPrivate *priv                = histogram->priv;
  gint                  n_channels          = n_components;
  gboolean              notify_n_components = FALSE;
  gboolean              notify_n_bins       = FALSE;

  if (n_channels > 0)
    n_channels += N_DERIVED_CHANNELS;

  if (n_channels != priv->n_channels)
    {
      priv->n_channels = n_channels;

      notify_n_components = TRUE;
    }

  if (n_bins != priv->n_bins)
    {
      priv->n_bins = n_bins;

      notify_n_bins = TRUE;
    }

  if (values != priv->values)
    {
      if (priv->values)
        g_free (priv->values);

      priv->values = values;
    }

  if (notify_n_components)
    g_object_notify (G_OBJECT (histogram), "n-components");

  if (notify_n_bins)
    g_object_notify (G_OBJECT (histogram), "n-bins");

  g_object_notify (G_OBJECT (histogram), "values");
}

static void
clay_histogram_calculate_internal (ClayAsync        *async,
                                   CalculateContext *context)
{
  CalculateData         data;
  ClayHistogramPrivate *priv;
  const Babl           *format;
  const Babl           *space;

  priv = context->histogram->priv;

  format = gegl_buffer_get_format (context->buffer);
  space  = babl_format_get_space (format);

  if (babl_format_get_type (format, 0) == babl_type ("u8"))
    context->n_bins = 256;
  else
    context->n_bins = 1024;

  switch (clay_babl_format_get_base_type (format))
    {
    case CLAY_RGB:
    case CLAY_INDEXED:
      format = clay_babl_format (CLAY_RGB,
                                 clay_babl_precision (CLAY_COMPONENT_TYPE_FLOAT,
                                                      priv->trc),
                                 babl_format_has_alpha (format),
                                 space);
      break;

    case CLAY_GRAY:
      format = clay_babl_format (CLAY_GRAY,
                                 clay_babl_precision (CLAY_COMPONENT_TYPE_FLOAT,
                                                      priv->trc),
                                 babl_format_has_alpha (format),
                                 space);
      break;

    default:
      if (async)
        clay_async_abort (async);

      g_return_if_reached ();
    }

  context->n_components = babl_format_get_n_components (format);

  data.async       = async;
  data.context     = context;
  data.format      = format;
  data.values_list = NULL;

  gegl_parallel_distribute_area (
    &context->buffer_rect, PIXELS_PER_THREAD, GEGL_SPLIT_STRATEGY_AUTO,
    (GeglParallelDistributeAreaFunc) clay_histogram_calculate_area,
    &data);

  if (! async || ! clay_async_is_canceled (async))
    {
      gdouble *total_values = NULL;
      gint     n_values     = (context->n_components + N_DERIVED_CHANNELS) *
                              context->n_bins;
      GSList  *iter;

      for (iter = data.values_list; iter; iter = g_slist_next (iter))
        {
          gdouble *values = iter->data;

          if (! total_values)
            {
              total_values = values;
            }
          else
            {
              gint i;

              for (i = 0; i < n_values; i++)
                total_values[i] += values[i];

              g_free (values);
            }
        }

      g_slist_free (data.values_list);

      context->values = total_values;

      if (async)
        clay_async_finish (async, NULL);
    }
  else
    {
      g_slist_free_full (data.values_list, g_free);

      if (async)
        clay_async_abort (async);
    }
}

static void
clay_histogram_calculate_area (const GeglRectangle *area,
                               CalculateData       *data)
{
  ClayAsync            *async;
  CalculateContext     *context;
  GeglBufferIterator   *iter;
  const Babl           *fish;
  gdouble              *values;
  gint                  n_components;
  gint                  n_bins;
  gfloat                n_bins_1f;
  gfloat                temp;

  async   = data->async;
  context = data->context;

  n_bins       = context->n_bins;
  n_components = context->n_components;

  fish = babl_fish (data->format, babl_format ("Y float"));

  values = g_new0 (gdouble, (n_components + N_DERIVED_CHANNELS) * n_bins);
  clay_atomic_slist_push_head (&data->values_list, values);

  iter = gegl_buffer_iterator_new (context->buffer, area, 0,
                                   data->format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  if (context->mask)
    {
      GeglRectangle mask_area = *area;

      mask_area.x += context->mask_rect.x - context->buffer_rect.x;
      mask_area.y += context->mask_rect.y - context->buffer_rect.y;

      gegl_buffer_iterator_add (iter, context->mask, &mask_area, 0,
                                babl_format ("Y float"),
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    }

  n_bins_1f = n_bins - 1;

#define VALUE(c,i) (*(temp = (i) * n_bins_1f,                                  \
                      &values[(c) * n_bins +                                   \
                              SIGNED_ROUND (SAFE_CLAMP (temp,                  \
                                                        0.0f,                  \
                                                        n_bins_1f))]))

#define CHECK_CANCELED(length)                                                 \
  G_STMT_START                                                                 \
    {                                                                          \
      if ((length) % 128 == 0 && async && clay_async_is_canceled (async))      \
        {                                                                      \
          gegl_buffer_iterator_stop (iter);                                    \
                                                                               \
          return;                                                              \
        }                                                                      \
    }                                                                          \
  G_STMT_END

  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *data   = iter->items[0].data;
      gint          length = iter->length;
      gfloat        max;
      gfloat        luminance;

      CHECK_CANCELED (0);

      if (context->mask)
        {
          const gfloat *mask_data = iter->items[1].data;

          switch (n_components)
            {
            case 1:
              while (length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (0, data[0]) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 2:
              while (length--)
                {
                  const gdouble masked = *mask_data;
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight * masked;
                  VALUE (1, data[1]) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 3: /* calculate separate value values */
              while (length--)
                {
                  const gdouble masked = *mask_data;

                  VALUE (1, data[0]) += masked;
                  VALUE (2, data[1]) += masked;
                  VALUE (3, data[2]) += masked;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += masked;

                  babl_process (fish, data, &luminance, 1);
                  VALUE (4, luminance) += masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;

            case 4: /* calculate separate value values */
              while (length--)
                {
                  const gdouble masked = *mask_data;
                  const gdouble weight = data[3];

                  VALUE (1, data[0]) += weight * masked;
                  VALUE (2, data[1]) += weight * masked;
                  VALUE (3, data[2]) += weight * masked;
                  VALUE (4, data[3]) += masked;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += weight * masked;

                  babl_process (fish, data, &luminance, 1);
                  VALUE (5, luminance) += weight * masked;

                  data += n_components;
                  mask_data += 1;

                  CHECK_CANCELED (length);
                }
              break;
            }
        }
      else /* no mask */
        {
          switch (n_components)
            {
            case 1:
              while (length--)
                {
                  VALUE (0, data[0]) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 2:
              while (length--)
                {
                  const gdouble weight = data[1];

                  VALUE (0, data[0]) += weight;
                  VALUE (1, data[1]) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 3: /* calculate separate value values */
              while (length--)
                {
                  VALUE (1, data[0]) += 1.0;
                  VALUE (2, data[1]) += 1.0;
                  VALUE (3, data[2]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += 1.0;

                  babl_process (fish, data, &luminance, 1);
                  VALUE (4, luminance) += 1.0;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;

            case 4: /* calculate separate value values */
              while (length--)
                {
                  const gdouble weight = data[3];

                  VALUE (1, data[0]) += weight;
                  VALUE (2, data[1]) += weight;
                  VALUE (3, data[2]) += weight;
                  VALUE (4, data[3]) += 1.0;

                  max = MAX (data[0], data[1]);
                  max = MAX (data[2], max);
                  VALUE (0, max) += weight;

                  babl_process (fish, data, &luminance, 1);
                  VALUE (5, luminance) += weight;

                  data += n_components;

                  CHECK_CANCELED (length);
                }
              break;
            }
        }
    }

#undef VALUE
#undef CHECK_CANCELED
}

static void
clay_histogram_calculate_async_callback (ClayAsync        *async,
                                         CalculateContext *context)
{
  context->histogram->priv->calculate_async = NULL;

  if (clay_async_is_finished (async))
    {
      clay_histogram_set_values (context->histogram,
                                 context->n_components, context->n_bins,
                                 context->values);
    }

  g_object_unref (context->buffer);
  if (context->mask)
    g_object_unref (context->mask);

  g_slice_free (CalculateContext, context);
}

guint
clay_histogram_unique_colors (ClayDrawable *drawable)
{
  ClayImage          *image;
  const Babl         *format;
  guint               bpp;
  GeglBufferIterator *iter;
  GHashTable         *hash_table;
  guint               uniques = 0;
  gboolean            selection_empty;
  GeglRectangle       area;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), 0);

  image  = clay_item_get_image (CLAY_ITEM (drawable));
  format = clay_drawable_get_format (drawable);
  bpp    = babl_format_get_bytes_per_pixel (format);

  selection_empty = clay_channel_is_empty (clay_image_get_mask (image));
  if (! selection_empty)
    clay_item_mask_intersect (CLAY_ITEM (drawable), &area.x, &area.y,
                              &area.width, &area.height);

  iter = gegl_buffer_iterator_new (clay_drawable_get_buffer (drawable),
                                   selection_empty ? NULL : &area,
                                   0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);


  hash_table = g_hash_table_new_full ((GHashFunc)      hash_color_bytes,
                                      (GEqualFunc)     color_bytes_equal,
                                      (GDestroyNotify) g_bytes_unref,
                                      NULL);

  while (gegl_buffer_iterator_next (iter))
    {
      guint8 *data   = iter->items[0].data;
      guint   length = iter->length;

      while (length--)
        {
          GBytes *key = g_bytes_new (data, bpp);

          if (! g_hash_table_lookup_extended (hash_table, key, NULL, NULL))
            {
              g_hash_table_insert (hash_table, key, NULL);
              key = NULL;
              uniques++;
            }

          g_bytes_unref (key);

          data += bpp;
        }
    }

  g_hash_table_destroy (hash_table);

  return uniques;
}
