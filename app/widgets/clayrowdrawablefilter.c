/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowdrawablefilter.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/claydrawable.h"
#include "core/claydrawablefilter.h"
#include "core/clayimage.h"
#include "core/clayimage-undo-push.h"

#include "clayrowdrawablefilter.h"


struct _ClayRowDrawableFilter
{
  ClayRowFilter  parent_instance;
};


static void   clay_row_filter_drawable_active_toggled (ClayRowFilter *row,
                                                       gboolean       active);


G_DEFINE_TYPE (ClayRowDrawableFilter,
               clay_row_drawable_filter,
               CLAY_TYPE_ROW_FILTER)

#define parent_class clay_row_drawable_filter_parent_class


static void
clay_row_drawable_filter_class_init (ClayRowDrawableFilterClass *klass)
{
  ClayRowFilterClass *row_filter_class = CLAY_ROW_FILTER_CLASS (klass);

  row_filter_class->active_toggled = clay_row_filter_drawable_active_toggled;
}

static void
clay_row_drawable_filter_init (ClayRowDrawableFilter *row)
{
}

static void
clay_row_filter_drawable_active_toggled (ClayRowFilter *rowfilter,
                                         gboolean       active)
{
  ClayRow            *row      = CLAY_ROW (rowfilter);
  ClayViewable       *viewable = clay_row_get_viewable (row);
  ClayContext        *context;
  ClayDrawableFilter *filter;

  if (viewable)
    {
      filter  = CLAY_DRAWABLE_FILTER (viewable);
      context = clay_row_get_context (row);
      clay_image_undo_push_filter_visibility_compressible (filter, context);
    }

  CLAY_ROW_FILTER_CLASS (parent_class)->active_toggled (rowfilter, active);

  if (viewable)
    {
      ClayDrawable *drawable = clay_drawable_filter_get_drawable (filter);

      clay_drawable_update (drawable, 0, 0, -1, -1);
      clay_image_flush (clay_item_get_image (CLAY_ITEM (drawable)));
    }
}
