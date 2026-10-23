/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "core-types.h"

#include "operations/claylevelsconfig.h"

#include "claydrawable.h"
#include "claydrawable-histogram.h"
#include "claydrawable-levels.h"
#include "claydrawable-operation.h"
#include "clayhistogram.h"
#include "clayprogress.h"

#include "clay-intl.h"


/*  public functions  */

void
clay_drawable_levels_stretch (ClayDrawable *drawable,
                              ClayProgress *progress)
{
  ClayLevelsConfig *config;
  ClayHistogram    *histogram;
  GeglNode         *levels;
  ClayTRCType       trc = CLAY_TRC_LINEAR;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  if (! clay_item_mask_intersect (CLAY_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  config = g_object_new (CLAY_TYPE_LEVELS_CONFIG, "trc", trc, NULL);

  histogram = clay_histogram_new (trc);
  clay_drawable_calculate_histogram (drawable, histogram, FALSE);

  clay_levels_config_stretch (config, histogram,
                              clay_drawable_is_rgb (drawable));

  g_object_unref (histogram);

  levels = g_object_new (GEGL_TYPE_NODE,
                         "operation", "clay:levels",
                         NULL);

  gegl_node_set (levels,
                 "config", config,
                 NULL);

  clay_drawable_apply_operation (drawable, progress, _("Levels"),
                                 levels);

  g_object_unref (levels);
  g_object_unref (config);
}
