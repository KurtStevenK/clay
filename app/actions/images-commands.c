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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "widgets/claycontainerview.h"
#include "widgets/clayimageview.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "images-commands.h"


/*  public functions */

void
images_raise_views_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayImage           *image;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  image = clay_context_get_image (context);

  if (image && clay_container_have (container, CLAY_OBJECT (image)))
    {
      GList *list;

      for (list = clay_get_display_iter (image->clay);
           list;
           list = g_list_next (list))
        {
          ClayDisplay *display = list->data;

          if (clay_display_get_image (display) == image)
            clay_display_shell_present (clay_display_get_shell (display));
        }
    }
}

void
images_new_view_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayImage           *image;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  image = clay_context_get_image (context);

  if (image && clay_container_have (container, CLAY_OBJECT (image)))
    {
      clay_create_display (image->clay, image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (GTK_WIDGET (editor))));
    }
}

void
images_delete_image_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayImage           *image;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  image = clay_context_get_image (context);

  if (image && clay_container_have (container, CLAY_OBJECT (image)))
    {
      if (clay_image_get_display_count (image) == 0)
        {
          clay_container_remove (container, CLAY_OBJECT (image));
          g_object_unref (image);
        }
    }
}
