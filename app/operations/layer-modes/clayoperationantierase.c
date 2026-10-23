/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationantierase.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "clayoperationantierase.h"



static gboolean                   clay_operation_anti_erase_process             (GeglOperation          *op,
                                                                                 void                   *in,
                                                                                 void                   *layer,
                                                                                 void                   *mask,
                                                                                 void                   *out,
                                                                                 glong                   samples,
                                                                                 const GeglRectangle    *roi,
                                                                                 gint                    level);
static ClayLayerCompositeRegion   clay_operation_anti_erase_get_affected_region (ClayOperationLayerMode *layer_mode);


G_DEFINE_TYPE (ClayOperationAntiErase, clay_operation_anti_erase,
               CLAY_TYPE_OPERATION_LAYER_MODE)


static void
clay_operation_anti_erase_class_init (ClayOperationAntiEraseClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  ClayOperationLayerModeClass *layer_mode_class = CLAY_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "clay:anti-erase",
                                 "description", "Clay anti erase mode operation",
                                 NULL);

  layer_mode_class->process             = clay_operation_anti_erase_process;
  layer_mode_class->get_affected_region = clay_operation_anti_erase_get_affected_region;
}

static void
clay_operation_anti_erase_init (ClayOperationAntiErase *self)
{
}

static gboolean
clay_operation_anti_erase_process (GeglOperation       *op,
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
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[alpha] = in[alpha] + (1.0 - in[alpha]) * layer[alpha] * value;

          for (b = 0; b < alpha; b++)
            {
              out[b] = in[b];
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
          gint b;

          out[alpha] = in[alpha];

          for (b = 0; b < alpha; b++)
            {
              out[b] = in[b];
            }

          in  += n_components;
          out += n_components;
        }
      break;

    case CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER:
      while (samples--)
        {
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[alpha] = layer[alpha] * value;

          for (b = 0; b < alpha; b++)
            {
              out[b] = in[b];
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
          gfloat value = opacity;
          gint   b;

          if (has_mask)
            value *= *mask;

          out[alpha] = in[alpha] * layer[alpha] * value;

          for (b = 0; b < alpha; b++)
            {
              out[b] = in[b];
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

static ClayLayerCompositeRegion
clay_operation_anti_erase_get_affected_region (ClayOperationLayerMode *layer_mode)
{
  return CLAY_LAYER_COMPOSITE_REGION_SOURCE;
}
