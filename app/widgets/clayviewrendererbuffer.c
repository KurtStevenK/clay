/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererbuffer.c
 * Copyright (C) 2004-2006 Michael Natterer <mitch@gimp.org>
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

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claytempbuf.h"
#include "core/clayviewable.h"

#include "clayviewrendererbuffer.h"


static void   clay_view_renderer_buffer_render (ClayViewRenderer *renderer,
                                                GtkWidget        *widget);


G_DEFINE_TYPE (ClayViewRendererBuffer, clay_view_renderer_buffer,
               CLAY_TYPE_VIEW_RENDERER)

#define parent_class clay_view_renderer_buffer_class_init


static void
clay_view_renderer_buffer_class_init (ClayViewRendererBufferClass *klass)
{
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = clay_view_renderer_buffer_render;
}

static void
clay_view_renderer_buffer_init (ClayViewRendererBuffer *renderer)
{
}

static void
clay_view_renderer_buffer_render (ClayViewRenderer *renderer,
                                  GtkWidget        *widget)
{
  gint         buffer_width;
  gint         buffer_height;
  gint         view_width;
  gint         view_height;
  gint         scale_factor = 1;
  gboolean     scaling_up;
  ClayTempBuf *render_buf = NULL;

  clay_viewable_get_size (renderer->viewable, &buffer_width, &buffer_height);

  clay_viewable_calc_preview_size (buffer_width,
                                   buffer_height,
                                   renderer->width,
                                   renderer->height,
                                   TRUE, 1.0, 1.0,
                                   &view_width,
                                   &view_height,
                                   &scaling_up);

  if (scaling_up)
    {
      ClayTempBuf *temp_buf;

      temp_buf = clay_viewable_get_new_preview (renderer->viewable,
                                                renderer->context,
                                                buffer_width, buffer_height,
                                                scale_factor,
                                                NULL);

      if (temp_buf)
        {
          render_buf = clay_temp_buf_scale (temp_buf, view_width, view_height);

          clay_temp_buf_unref (temp_buf);
        }
    }
  else
    {
      scale_factor = gtk_widget_get_scale_factor (widget);

      render_buf = clay_viewable_get_new_preview (renderer->viewable,
                                                  renderer->context,
                                                  view_width, view_height,
                                                  scale_factor,
                                                  NULL);
    }

  if (render_buf)
    {
      clay_view_renderer_render_temp_buf_simple (renderer, widget,
                                                 render_buf,
                                                 scale_factor,
                                                 CLAY_VIEW_BG_CHECKS,
                                                 CLAY_VIEW_BG_WHITE);

      clay_temp_buf_unref (render_buf);
    }
  else /* no preview available */
    {
      const gchar *icon_name;

      icon_name = clay_viewable_get_icon_name (renderer->viewable);

      clay_view_renderer_render_icon (renderer, widget,
                                      icon_name,
                                      gtk_widget_get_scale_factor (widget));
    }
}
