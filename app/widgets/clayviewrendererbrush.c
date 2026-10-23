/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererbrush.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claybrushpipe.h"
#include "core/claybrushgenerated.h"
#include "core/claycontext.h"
#include "core/claytempbuf.h"

#include "clayviewrendererbrush.h"
#include "claywidgets-utils.h"


static void   clay_view_renderer_brush_finalize (GObject          *object);

static void   clay_view_renderer_brush_render   (ClayViewRenderer *renderer,
                                                 GtkWidget        *widget);
static void   clay_view_renderer_brush_draw     (ClayViewRenderer *renderer,
                                                 GtkWidget        *widget,
                                                 cairo_t          *cr,
                                                 gint              available_width,
                                                 gint              available_height);

static gboolean clay_view_renderer_brush_render_timeout (gpointer    data);


G_DEFINE_TYPE (ClayViewRendererBrush, clay_view_renderer_brush,
               CLAY_TYPE_VIEW_RENDERER)

#define parent_class clay_view_renderer_brush_parent_class


static void
clay_view_renderer_brush_class_init (ClayViewRendererBrushClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  object_class->finalize = clay_view_renderer_brush_finalize;

  renderer_class->default_bg      = CLAY_VIEW_BG_WHITE;
  renderer_class->follow_theme_bg = CLAY_VIEW_BG_STYLE;

  renderer_class->render          = clay_view_renderer_brush_render;
  renderer_class->draw            = clay_view_renderer_brush_draw;
}

static void
clay_view_renderer_brush_init (ClayViewRendererBrush *renderer)
{
  renderer->pipe_timeout_id      = 0;
  renderer->pipe_animation_index = 0;
}

static void
clay_view_renderer_brush_finalize (GObject *object)
{
  ClayViewRendererBrush *renderer = CLAY_VIEW_RENDERER_BRUSH (object);

  if (renderer->pipe_timeout_id)
    {
      g_source_remove (renderer->pipe_timeout_id);
      renderer->pipe_timeout_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_view_renderer_brush_render (ClayViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  ClayViewRendererBrush *renderbrush = CLAY_VIEW_RENDERER_BRUSH (renderer);
  ClayTempBuf           *temp_buf;
  GeglColor             *fg_color   = NULL;
  ClayViewBG             bg_style   = CLAY_VIEW_BG_WHITE;
  gint                   temp_buf_x = 0;
  gint                   temp_buf_y = 0;
  gint                   temp_buf_width;
  gint                   temp_buf_height;
  gint                   scale_factor;

  if (renderbrush->pipe_timeout_id)
    {
      g_source_remove (renderbrush->pipe_timeout_id);
      renderbrush->pipe_timeout_id = 0;
    }

  if (renderer->context &&
      CLAY_GUI_CONFIG (renderer->context->clay->config)->viewables_follow_theme)
    {
      fg_color = clay_get_style_color (widget, GTK_STYLE_PROPERTY_COLOR);
      bg_style = CLAY_VIEW_BG_STYLE;
    }

  scale_factor = gtk_widget_get_scale_factor (widget);

  temp_buf = clay_viewable_get_new_preview (renderer->viewable,
                                            renderer->context,
                                            renderer->width,
                                            renderer->height,
                                            scale_factor,
                                            fg_color);
  g_clear_object (&fg_color);

  temp_buf_width  = clay_temp_buf_get_width  (temp_buf);
  temp_buf_height = clay_temp_buf_get_height (temp_buf);

  if (temp_buf_width < renderer->width * scale_factor)
    temp_buf_x = (renderer->width * scale_factor - temp_buf_width) / 2;

  if (temp_buf_height < renderer->height * scale_factor)
    temp_buf_y = (renderer->height * scale_factor - temp_buf_height) / 2;

  clay_view_renderer_render_temp_buf (renderer, widget,
                                      temp_buf,
                                      scale_factor,
                                      temp_buf_x,
                                      temp_buf_y,
                                      -1,
                                      bg_style,
                                      bg_style);

  if (renderer->is_popup &&
      CLAY_IS_BRUSH_PIPE (renderer->viewable))
    {
      renderbrush->widget = widget;
      renderbrush->pipe_animation_index = 0;
      renderbrush->pipe_timeout_id =
        g_timeout_add (300, clay_view_renderer_brush_render_timeout,
                       renderbrush);
    }

  clay_temp_buf_unref (temp_buf);
}

static gboolean
clay_view_renderer_brush_render_timeout (gpointer data)
{
  ClayViewRendererBrush *renderbrush = CLAY_VIEW_RENDERER_BRUSH (data);
  ClayViewRenderer      *renderer    = CLAY_VIEW_RENDERER (data);
  ClayBrushPipe         *brush_pipe;
  ClayBrush             *brush;
  ClayTempBuf           *temp_buf;
  GeglColor             *fg_color   = NULL;
  ClayViewBG             bg_style   = CLAY_VIEW_BG_WHITE;
  gint                   temp_buf_x = 0;
  gint                   temp_buf_y = 0;
  gint                   temp_buf_width;
  gint                   temp_buf_height;
  gint                   scale_factor;

  if (! renderer->viewable)
    {
      renderbrush->pipe_timeout_id      = 0;
      renderbrush->pipe_animation_index = 0;

      return FALSE;
    }

  brush_pipe = CLAY_BRUSH_PIPE (renderer->viewable);

  if (renderer->context &&
      CLAY_GUI_CONFIG (renderer->context->clay->config)->viewables_follow_theme)
    {
      bg_style = CLAY_VIEW_BG_STYLE;

      fg_color = clay_get_style_color (renderbrush->widget,
                                       GTK_STYLE_PROPERTY_COLOR);
    }

  renderbrush->pipe_animation_index++;

  if (renderbrush->pipe_animation_index >= brush_pipe->n_brushes)
    renderbrush->pipe_animation_index = 0;

  brush = CLAY_BRUSH (brush_pipe->brushes[renderbrush->pipe_animation_index]);

  scale_factor = gtk_widget_get_scale_factor (renderbrush->widget);

  temp_buf = clay_viewable_get_new_preview (CLAY_VIEWABLE (brush),
                                            renderer->context,
                                            renderer->width,
                                            renderer->height,
                                            scale_factor,
                                            fg_color);
  g_clear_object (&fg_color);

  temp_buf_width  = clay_temp_buf_get_width  (temp_buf);
  temp_buf_height = clay_temp_buf_get_height (temp_buf);

  if (temp_buf_width < renderer->width * scale_factor)
    temp_buf_x = (renderer->width * scale_factor - temp_buf_width) / 2;

  if (temp_buf_height < renderer->height * scale_factor)
    temp_buf_y = (renderer->height * scale_factor - temp_buf_height) / 2;

  clay_view_renderer_render_temp_buf (renderer, renderbrush->widget,
                                      temp_buf,
                                      scale_factor,
                                      temp_buf_x,
                                      temp_buf_y,
                                      -1,
                                      bg_style,
                                      bg_style);

  clay_temp_buf_unref (temp_buf);

  clay_view_renderer_update (renderer);

  return TRUE;
}

static void
clay_view_renderer_brush_draw (ClayViewRenderer *renderer,
                               GtkWidget        *widget,
                               cairo_t          *cr,
                               gint              available_width,
                               gint              available_height)
{
  CLAY_VIEW_RENDERER_CLASS (parent_class)->draw (renderer, widget, cr,
                                                 available_width,
                                                 available_height);

#define INDICATOR_WIDTH  7
#define INDICATOR_HEIGHT 7

  if (renderer->width  > 2 * INDICATOR_WIDTH &&
      renderer->height > 2 * INDICATOR_HEIGHT)
    {
      gboolean  pipe      = CLAY_IS_BRUSH_PIPE (renderer->viewable);
      gboolean  generated = CLAY_IS_BRUSH_GENERATED (renderer->viewable);
      gint      brush_width;
      gint      brush_height;

      if (generated || pipe)
        {
          cairo_move_to (cr, available_width, available_height);
          cairo_rel_line_to (cr, - INDICATOR_WIDTH, 0);
          cairo_rel_line_to (cr, INDICATOR_WIDTH, - INDICATOR_HEIGHT);
          cairo_rel_line_to (cr, 0, INDICATOR_HEIGHT);

          if (pipe)
            cairo_set_source_rgb (cr, 1.0, 0.5, 0.5);
          else
            cairo_set_source_rgb (cr, 0.5, 0.6, 1.0);

          cairo_fill (cr);
        }

      clay_viewable_get_size (renderer->viewable, &brush_width, &brush_height);

      if (renderer->width < brush_width || renderer->height < brush_height)
        {
          cairo_move_to (cr,
                         available_width  - INDICATOR_WIDTH + 1,
                         available_height - INDICATOR_HEIGHT / 2.0);
          cairo_rel_line_to (cr, INDICATOR_WIDTH - 2, 0);

          cairo_move_to (cr,
                         available_width  - INDICATOR_WIDTH / 2.0,
                         available_height - INDICATOR_HEIGHT + 1);
          cairo_rel_line_to (cr, 0, INDICATOR_WIDTH - 2);

          cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
          cairo_set_line_width (cr, 1);
          cairo_stroke (cr);
        }
    }

#undef INDICATOR_WIDTH
#undef INDICATOR_HEIGHT
}
