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
#include "core/clay-edit.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "widgets/claycontainereditor.h"
#include "widgets/claycontainerview.h"
#include "widgets/claycontainerview-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-transform.h"

#include "buffers-commands.h"

#include "clay-intl.h"


/*  public functions */

void
buffers_paste_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayBuffer          *buffer;
  ClayPasteType        paste_type = (ClayPasteType) g_variant_get_int32 (value);

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  buffer = clay_context_get_buffer (context);

  if (buffer && clay_container_have (container, CLAY_OBJECT (buffer)))
    {
      ClayDisplay *display = clay_context_get_display (context);
      ClayImage   *image   = NULL;
      gint         x       = -1;
      gint         y       = -1;
      gint         width   = -1;
      gint         height  = -1;

      if (display)
        {
          ClayDisplayShell *shell = clay_display_get_shell (display);

          clay_display_shell_untransform_viewport (
            shell,
            ! clay_display_shell_get_infinite_canvas (shell),
            &x, &y, &width, &height);

          image = clay_display_get_image (display);
        }
      else
        {
          image = clay_context_get_image (context);
        }

      if (image)
        {
          GList *drawables = clay_image_get_selected_drawables (image);

          g_list_free (clay_edit_paste (image, drawables,
                                        CLAY_OBJECT (buffer), paste_type,
                                        context, FALSE,
                                        x, y, width, height));

          clay_image_flush (image);
          g_list_free (drawables);
        }
    }
}

void
buffers_paste_as_new_image_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayBuffer          *buffer;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  buffer = clay_context_get_buffer (context);

  if (buffer && clay_container_have (container, CLAY_OBJECT (buffer)))
    {
      GtkWidget *widget = GTK_WIDGET (editor);
      ClayImage *new_image;

      new_image = clay_edit_paste_as_new_image (context->clay,
                                                CLAY_OBJECT (buffer),
                                                context);
      clay_create_display (context->clay, new_image,
                           clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (new_image);
    }
}

void
buffers_delete_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);

  clay_container_view_remove_active (editor->view);
}
