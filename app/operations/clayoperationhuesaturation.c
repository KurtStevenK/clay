/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationhuesaturation.c
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

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"

#include "operations-types.h"

#include "clayhuesaturationconfig.h"
#include "clayoperationhuesaturation.h"

#include "clay-intl.h"

static void clay_operation_hue_saturation_prepare (GeglOperation *operation);

static gboolean clay_operation_hue_saturation_process (GeglOperation       *operation,
                                                       void                *in_buf,
                                                       void                *out_buf,
                                                       glong                samples,
                                                       const GeglRectangle *roi,
                                                       gint                 level);


G_DEFINE_TYPE (ClayOperationHueSaturation, clay_operation_hue_saturation,
               CLAY_TYPE_OPERATION_POINT_FILTER)

#define parent_class clay_operation_hue_saturation_parent_class


static void
clay_operation_hue_saturation_class_init (ClayOperationHueSaturationClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = clay_operation_point_filter_set_property;
  object_class->get_property   = clay_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "clay:hue-saturation",
                                 "title",       _("Hue-Saturation"),
                                 "categories",  "color",
                                 "description", _("Adjust hue, saturation, and lightness"),
                                 NULL);

  point_class->process = clay_operation_hue_saturation_process;
  operation_class->prepare = clay_operation_hue_saturation_prepare;

  g_object_class_install_property (object_class,
                                   CLAY_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        CLAY_TYPE_HUE_SATURATION_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_operation_hue_saturation_init (ClayOperationHueSaturation *self)
{
}

static inline gfloat
map_hue (ClayHueSaturationConfig *config,
         ClayHueRange             range,
         gfloat                   value)
{
  value += (config->hue[CLAY_HUE_RANGE_ALL] + config->hue[range]) / 2.0f;

  if (value < 0)
    return value + 1.0f;
  else if (value > 1.0f)
    return value - 1.0f;
  else
    return value;
}

static inline gfloat
map_hue_overlap (ClayHueSaturationConfig *config,
                 ClayHueRange             primary_range,
                 ClayHueRange             secondary_range,
                 gfloat                   value,
                 gfloat                   primary_intensity,
                 gfloat                   secondary_intensity)
{
  /*  When calculating an overlap between two ranges, interpolate the
   *  hue adjustment from config->hue[primary_range] and
   *  config->hue[secondary_range] BEFORE mapping it to the input
   *  value.  This fixes odd edge cases where only one of the ranges
   *  crosses the red/magenta wraparound (bug #527085), or if
   *  adjustments to different channels yield more than 180 degree
   *  difference from each other. (Why anyone would do that is beyond
   *  me, but still.)
   *
   *  See bugs #527085 and #644032 for examples of such cases.
   */
  gfloat v = config->hue[primary_range]   * primary_intensity +
             config->hue[secondary_range] * secondary_intensity;

  value += (config->hue[CLAY_HUE_RANGE_ALL] + v) / 2.0f;

  if (value < 0)
    return value + 1.0f;
  else if (value > 1.0f)
    return value - 1.0f;
  else
    return value;
}

static inline gfloat
map_saturation (ClayHueSaturationConfig *config,
                ClayHueRange             range,
                gfloat                   value)
{
  gfloat v = config->saturation[CLAY_HUE_RANGE_ALL] + config->saturation[range];

  /* This change affects the way saturation is computed. With the old
   * code (different code for value < 0), increasing the saturation
   * affected muted colors very much, and bright colors less. With the
   * new code, it affects muted colors and bright colors more or less
   * evenly. For enhancing the color in photos, the new behavior is
   * exactly what you want. It's hard for me to imagine a case in
   * which the old behavior is better.
  */
  value *= (v + 1.0);

  return CLAMP (value, 0.0f, 1.0f);
}

static inline gfloat
map_lightness (ClayHueSaturationConfig *config,
               ClayHueRange             range,
               gfloat                   value)
{
  gfloat v = (config->lightness[CLAY_HUE_RANGE_ALL] + config->lightness[range]);

  if (v < 0)
    return value * (v + 1.0f);
  else
    return value + (v * (1.0f - value));
}

static inline gfloat
map_lightness_achromatic (ClayHueSaturationConfig *config, gfloat value)
{
  gfloat v = config->lightness[CLAY_HUE_RANGE_ALL];

  if (v < 0)
    return value * (v + 1.0f);
  else
    return value + (v * (1.0f - value));
}

static void
clay_operation_hue_saturation_prepare (GeglOperation *operation)
{
  /* We work in HSLA within sRGB space which is much faster to convert
   * to than with specific spaces. Eventually though, I'm not sure if we
   * may not want to work in the source space. So it would be wise to
   * study this possible enhancement once babl got faster conversion
   * from "R'G'B'" with space to HSL. TODO.
   * See: https://gitlab.gnome.org/GNOME/babl/-/issues/103
   */
  const Babl *format = babl_format_with_space ("HSLA float", NULL);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
clay_operation_hue_saturation_process (GeglOperation       *operation,
                                       void                *in_buf,
                                       void                *out_buf,
                                       glong                samples,
                                       const GeglRectangle *roi,
                                       gint                 level)
{
  ClayOperationPointFilter *point  = CLAY_OPERATION_POINT_FILTER (operation);
  ClayHueSaturationConfig  *config = CLAY_HUE_SATURATION_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;
  gfloat                    overlap;

  if (! config)
    return FALSE;

  overlap = config->overlap / 2.0;

  while (samples--)
    {
      gfloat   hsl[4];
      gfloat   h;
      gint     hue_counter;
      gint     hue                 = 0;
      gint     secondary_hue       = 0;
      gboolean use_secondary_hue   = FALSE;
      gfloat   primary_intensity   = 0.0f;
      gfloat   secondary_intensity = 0.0f;

      for (gint i = 0; i < 4; i++)
        hsl[i] = src[i];
      h = hsl[0] * 6.0f;

      for (hue_counter = 0; hue_counter < 7; hue_counter++)
        {
          gfloat hue_threshold = (gfloat) hue_counter + 0.5f;

          if (h < ((gfloat) hue_threshold + overlap))
            {
              hue = hue_counter;

              if (overlap > 0.0f && h > ((gfloat) hue_threshold - overlap))
                {
                  use_secondary_hue = TRUE;

                  secondary_hue = hue_counter + 1;

                  secondary_intensity =
                    (h - (gfloat) hue_threshold + overlap) / (2.0f * overlap);

                  primary_intensity = 1.0f - secondary_intensity;
                }
              else
                {
                  use_secondary_hue = FALSE;
                }

              break;
            }
        }

      if (hue >= 6)
        {
          hue = 0;
          use_secondary_hue = FALSE;
        }

      if (secondary_hue >= 6)
        {
          secondary_hue = 0;
        }

      /*  transform into ClayHueRange values  */
      hue++;
      secondary_hue++;

      if (use_secondary_hue)
        {
          hsl[0] = map_hue_overlap (config, hue, secondary_hue, hsl[0],
                                    primary_intensity, secondary_intensity);

          hsl[1] = (map_saturation (config, hue,           hsl[1]) * primary_intensity +
                    map_saturation (config, secondary_hue, hsl[1]) * secondary_intensity);

          hsl[2] = (map_lightness (config, hue,           hsl[2]) * primary_intensity +
                    map_lightness (config, secondary_hue, hsl[2]) * secondary_intensity);
        }
      else
        {
          if (hsl[1] <= 0.0)
            {
              hsl[2] = map_lightness_achromatic (config, hsl[2]);
            }
          else
            {
              hsl[0] = map_hue (config, hue, hsl[0]);
              hsl[2] = map_lightness (config, hue, hsl[2]);
              hsl[1] = map_saturation (config, hue, hsl[1]);
            }
        }

      for (gint i = 0; i < 3; i++)
        dest[i] = hsl[i];
      dest[3] = src[3];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}


/*  public functions  */

void
clay_operation_hue_saturation_map (ClayHueSaturationConfig *config,
                                   GeglColor               *color,
                                   ClayHueRange             range)
{
  const Babl *format;
  gfloat     hsl[4];

  g_return_if_fail (CLAY_IS_HUE_SATURATION_CONFIG (config));
  g_return_if_fail (GEGL_IS_COLOR (color));

  format = gegl_color_get_format (color);
  format = babl_format_with_space ("HSLA float", format);
  gegl_color_get_pixel (color, format, hsl);

  hsl[0] = map_hue        (config, range, hsl[0]);
  hsl[1] = map_saturation (config, range, hsl[1]);
  hsl[2] = map_lightness  (config, range, hsl[2]);

  gegl_color_set_pixel (color, format, hsl);
}
