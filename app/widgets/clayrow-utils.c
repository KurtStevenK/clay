/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrow-utils.c
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

#include "core/claycontext.h"
#include "core/claydrawablefilter.h"
#include "core/clayimage.h"
#include "core/claysettings.h"

#include "claycontainerview.h"
#include "claydeviceinfo.h"
#include "clayrow-utils.h"
#include "clayrowdeviceinfo.h"
#include "clayrowdrawablefilter.h"
#include "clayrowimagesave.h"
#include "clayrowseparator.h"
#include "clayrowsettings.h"


GType
clay_row_type_from_viewable (ClayViewable *viewable)
{
  GType row_type = CLAY_TYPE_ROW;

  g_return_val_if_fail (CLAY_IS_VIEWABLE (viewable), G_TYPE_NONE);

  if (CLAY_IS_DRAWABLE_FILTER (viewable))
    {
      row_type = CLAY_TYPE_ROW_DRAWABLE_FILTER;
    }
  else if (CLAY_IS_FILTER (viewable))
    {
      row_type = CLAY_TYPE_ROW_FILTER;
    }
  else if (CLAY_IS_SETTINGS (viewable))
    {
      if (clay_object_get_name (viewable))
        row_type = CLAY_TYPE_ROW_SETTINGS;
      else
        row_type = CLAY_TYPE_ROW_SEPARATOR;
    }
  else if (CLAY_IS_DEVICE_INFO (viewable))
    {
      row_type = CLAY_TYPE_ROW_DEVICE_INFO;
    }
  else if (CLAY_IS_IMAGE (viewable))
    {
      row_type = CLAY_TYPE_ROW_IMAGE;
    }

#if 0
  g_printerr ("viewable/row type: %s/%s\n",
              g_type_name (G_OBJECT_TYPE (viewable)),
              g_type_name (row_type));
#endif

  return row_type;
}

GtkWidget *
clay_row_create_for_context (gpointer item,
                             gpointer context)
{
  g_return_val_if_fail (CLAY_IS_VIEWABLE (item), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return clay_row_new (clay_row_type_from_viewable (item),
                       context, item, CLAY_VIEW_SIZE_MEDIUM, 0);
}

GtkWidget *
clay_row_create_for_container_view (gpointer item,
                                    gpointer container_view)
{
  ClayContext *context;
  gint         view_size;
  gint         view_border_width;

  g_return_val_if_fail (CLAY_IS_VIEWABLE (item), NULL);
  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (container_view), NULL);

  context   = clay_container_view_get_context (container_view);
  view_size = clay_container_view_get_view_size (container_view,
                                                 &view_border_width);

  return clay_row_new (clay_row_type_from_viewable (item),
                       context, item, view_size, view_border_width);
}

GtkWidget *
clay_row_create_for_save_view (gpointer item,
                               gpointer container_view)
{
  ClayContext *context;
  gint         view_size;
  gint         view_border_width;

  g_return_val_if_fail (CLAY_IS_VIEWABLE (item), NULL);
  g_return_val_if_fail (CLAY_IS_CONTAINER_VIEW (container_view), NULL);

  context   = clay_container_view_get_context (container_view);
  view_size = clay_container_view_get_view_size (container_view,
                                                 &view_border_width);

  return clay_row_new (CLAY_TYPE_ROW_IMAGE_SAVE,
                       context, item, view_size, view_border_width);
}
