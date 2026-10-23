/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimagesaveview.c
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "clayimagesaveview.h"
#include "clayrow-utils.h"
#include "clayviewrenderer.h"


G_DEFINE_TYPE (ClayImageSaveView,
               clay_image_save_view,
               CLAY_TYPE_CONTAINER_LIST_VIEW)

#define parent_class clay_image_save_view_parent_class


static void
clay_image_save_view_class_init (ClayImageSaveViewClass *klass)
{
  ClayContainerListViewClass *list_class =
    CLAY_CONTAINER_LIST_VIEW_CLASS (klass);

  list_class->create_row_func = clay_row_create_for_save_view;
}

static void
clay_image_save_view_init (ClayImageSaveView *view)
{
}

GtkWidget *
clay_image_save_view_new (ClayContainer *container,
                          ClayContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);

  return g_object_new (CLAY_TYPE_IMAGE_SAVE_VIEW,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "context",           context,
                       "container",         container,
                       NULL);
}
