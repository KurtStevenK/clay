/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationlightenonlylegacy.c
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

#include "clayoperationlightenonlylegacy.h"


static gboolean   clay_operation_lighten_only_legacy_process (GeglOperation       *op,
                                                              void                *in,
                                                              void                *layer,
                                                              void                *mask,
                                                              void                *out,
                                                              glong                samples,
                                                              const GeglRectangle *roi,
                                                              gint                 level);


G_DEFINE_TYPE (ClayOperationLightenOnlyLegacy, clay_operation_lighten_only_legacy,
               CLAY_TYPE_OPERATION_LAYER_MODE)


static void
clay_operation_lighten_only_legacy_class_init (ClayOperationLightenOnlyLegacyClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  ClayOperationLayerModeClass *layer_mode_class = CLAY_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "clay:lighten-only-legacy",
                                 "description", "Clay lighten only legacy operation",
                                 NULL);

  layer_mode_class->process = clay_operation_lighten_only_legacy_process;
}

static void
clay_operation_lighten_only_legacy_init (ClayOperationLightenOnlyLegacy *self)
{
}

static gboolean
clay_operation_lighten_only_legacy_process (GeglOperation       *op,
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
  const gint              n_components = babl_format_get_n_components (format);
  const gint              alpha        = n_components - 1;

  while (samples--)
    {
      gfloat comp_alpha, new_alpha;

      comp_alpha = MIN (in[alpha], layer[alpha]) * opacity;
      if (mask)
        comp_alpha *= *mask;

      new_alpha = in[alpha] + (1.0f - in[alpha]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gint   b;
          gfloat ratio = comp_alpha / new_alpha;

          for (b = 0; b < alpha; b++)
            {
              gfloat comp = MAX (layer[b], in[b]);

              out[b] = comp * ratio + in[b] * (1.0f - ratio);
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

      out[alpha] = in[alpha];

      in    += n_components;
      layer += n_components;
      out   += n_components;

      if (mask)
        mask++;
    }

  return TRUE;
}
