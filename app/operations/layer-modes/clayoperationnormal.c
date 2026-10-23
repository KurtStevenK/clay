/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationnormalmode.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <gio/gio.h>
#include <gegl-plugin.h>

#include "libclaybase/claybase.h"

#include "../operations-types.h"

#include "clayoperationnormal.h"


G_DEFINE_TYPE (ClayOperationNormal, clay_operation_normal,
               CLAY_TYPE_OPERATION_LAYER_MODE)


static const gchar* reference_xml = "<?xml version='1.0' encoding='UTF-8'?>"
"<gegl>"
"<node operation='clay:normal'>"
"  <node operation='gegl:load'>"
"    <params>"
"      <param name='path'>blending-test-B.png</param>"
"    </params>"
"  </node>"
"</node>"
"<node operation='gegl:load'>"
"  <params>"
"    <param name='path'>blending-test-A.png</param>"
"  </params>"
"</node>"
"</gegl>";


static void
clay_operation_normal_class_init (ClayOperationNormalClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  ClayOperationLayerModeClass *layer_mode_class = CLAY_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",                  "clay:normal",
                                 "description",           "Clay normal mode operation",
                                 "reference-image",       "normal-mode.png",
                                 "reference-composition", reference_xml,
                                 NULL);

  layer_mode_class->process = clay_operation_normal_process;

#if COMPILE_SSE2_INTRINISICS
  if (clay_cpu_accel_get_support() & CLAY_CPU_ACCEL_X86_SSE2)
    layer_mode_class->process = clay_operation_normal_process_sse2;
#endif /* COMPILE_SSE2_INTRINISICS */

#if COMPILE_SSE4_1_INTRINISICS
  if (clay_cpu_accel_get_support() & CLAY_CPU_ACCEL_X86_SSE4_1)
    layer_mode_class->process = clay_operation_normal_process_sse4;
#endif /* COMPILE_SSE4_1_INTRINISICS */
}

static void
clay_operation_normal_init (ClayOperationNormal *self)
{
}

gboolean
clay_operation_normal_process (GeglOperation       *op,
                               void                *in_p,
                               void                *layer_p,
                               void                *mask_p,
                               void                *out_p,
                               glong                samples,
                               const GeglRectangle *roi,
                               gint                 level)
{
  ClayOperationLayerMode *layer_mode   = (gpointer) op;
  const Babl             *format       = gegl_operation_get_format (op, "input");
  gfloat                 *in           = in_p;
  gfloat                 *out          = out_p;
  gfloat                 *layer        = layer_p;
  gfloat                 *mask         = mask_p;
  gfloat                  opacity      = layer_mode->opacity;
  const gboolean          has_mask     = mask != NULL;
  const gint              n_components = babl_format_get_n_components (format);
  const gint              alpha        = n_components - 1;

  switch (layer_mode->composite_mode)
    {
    case CLAY_LAYER_COMPOSITE_UNION:
    case CLAY_LAYER_COMPOSITE_AUTO:
      while (samples--)
        {
          gfloat layer_alpha;

          layer_alpha = layer[alpha] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[alpha] = layer_alpha + in[alpha] - layer_alpha * in[alpha];

          if (out[alpha])
            {
              gfloat layer_weight = layer_alpha / out[alpha];
              gfloat in_weight    = 1.0f - layer_weight;
              gint   b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = layer[b] * layer_weight + in[b] * in_weight;
                }
            }
          else
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = in[b];
                }
            }

          in    += n_components;
          layer += n_components;
          out   += n_components;

          if (has_mask)
            mask++;
        }
      break;

    case CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP:
      while (samples--)
        {
          gfloat layer_alpha;

          layer_alpha = layer[alpha] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[alpha] = in[alpha];

          if (out[alpha])
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = in[b] + (layer[b] - in[b]) * layer_alpha;
                }
            }
          else
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = in[b];
                }
            }

          in    += n_components;
          layer += n_components;
          out   += n_components;

          if (has_mask)
            mask++;
        }
      break;

    case CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat layer_alpha;

          layer_alpha = layer[alpha] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[alpha] = layer_alpha;

          if (out[alpha])
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = layer[b];
                }
            }
          else
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = in[b];
                }
            }

          in    += n_components;
          layer += n_components;
          out   += n_components;

          if (has_mask)
            mask++;
        }
      break;

    case CLAY_LAYER_COMPOSITE_INTERSECTION:
      while (samples--)
        {
          gfloat layer_alpha;

          layer_alpha = layer[alpha] * opacity;
          if (has_mask)
            layer_alpha *= *mask;

          out[alpha] = in[alpha] * layer_alpha;

          if (out[alpha])
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = layer[b];
                }
            }
          else
            {
              gint b;

              for (b = 0; b < alpha; b++)
                {
                  out[b] = in[b];
                }
            }

          in    += n_components;
          layer += n_components;
          out   += n_components;

          if (has_mask)
            mask++;
        }
      break;
    }

  return TRUE;
}
