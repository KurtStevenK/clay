/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrenderer.h
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

#pragma once


#define CLAY_VIEW_MAX_BORDER_WIDTH 16


#define CLAY_TYPE_VIEW_RENDERER            (clay_view_renderer_get_type ())
#define CLAY_VIEW_RENDERER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW_RENDERER, ClayViewRenderer))
#define CLAY_VIEW_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW_RENDERER, ClayViewRendererClass))
#define CLAY_IS_VIEW_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW_RENDERER))
#define CLAY_IS_VIEW_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW_RENDERER))
#define CLAY_VIEW_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW_RENDERER, ClayViewRendererClass))


typedef struct _ClayViewRendererPrivate ClayViewRendererPrivate;
typedef struct _ClayViewRendererClass   ClayViewRendererClass;

struct _ClayViewRenderer
{
  GObject             parent_instance;

  ClayContext        *context;

  GType               viewable_type;
  ClayViewable       *viewable;

  gint                width;
  gint                height;
  gint                border_width;
  guint               dot_for_dot : 1;
  guint               is_popup    : 1;

  /*< protected >*/
  cairo_surface_t    *surface;
  ClayViewBG          surface_bg;

  gint                size;

  /*< private >*/
  ClayViewRendererPrivate *priv;
};

struct _ClayViewRendererClass
{
  GObjectClass   parent_class;

  ClayViewBG     default_bg;
  ClayViewBG     follow_theme_bg;

  GdkPixbuf     *frame;
  gint           frame_left;
  gint           frame_right;
  gint           frame_bottom;
  gint           frame_top;

  /*  signals  */
  void (* update)      (ClayViewRenderer *renderer);

  /*  virtual functions  */
  void (* set_context) (ClayViewRenderer *renderer,
                        ClayContext      *context);
  void (* invalidate)  (ClayViewRenderer *renderer);
  void (* draw)        (ClayViewRenderer *renderer,
                        GtkWidget        *widget,
                        cairo_t          *cr,
                        gint              available_width,
                        gint              available_height);
  void (* render)      (ClayViewRenderer *renderer,
                        GtkWidget        *widget);
};


GType              clay_view_renderer_get_type (void);

ClayViewRenderer * clay_view_renderer_new      (ClayContext *context,
                                                GType        viewable_type,
                                                gint         size,
                                                gint         border_width,
                                                gboolean     is_popup);
ClayViewRenderer * clay_view_renderer_new_full (ClayContext *context,
                                                GType        viewable_type,
                                                gint         width,
                                                gint         height,
                                                gint         border_width,
                                                gboolean     is_popup);

void   clay_view_renderer_set_context      (ClayViewRenderer   *renderer,
                                            ClayContext        *context);
void   clay_view_renderer_set_viewable     (ClayViewRenderer   *renderer,
                                            ClayViewable       *viewable);
void   clay_view_renderer_set_size         (ClayViewRenderer   *renderer,
                                            gint                size,
                                            gint                border_width);
void   clay_view_renderer_set_size_full    (ClayViewRenderer   *renderer,
                                            gint                width,
                                            gint                height,
                                            gint                border_width);
void   clay_view_renderer_set_dot_for_dot  (ClayViewRenderer   *renderer,
                                            gboolean            dot_for_dot);

void   clay_view_renderer_set_border_type  (ClayViewRenderer   *renderer,
                                            ClayViewBorderType  border_type);
ClayViewBorderType
       clay_view_renderer_get_border_type  (ClayViewRenderer   *renderer);

void   clay_view_renderer_set_background   (ClayViewRenderer   *renderer,
                                            const gchar        *icon_name);
void   clay_view_renderer_set_color_config (ClayViewRenderer   *renderer,
                                            ClayColorConfig    *color_config);
ClayColorConfig *
       clay_view_renderer_get_color_config (ClayViewRenderer   *renderer);

void   clay_view_renderer_invalidate       (ClayViewRenderer   *renderer);
void   clay_view_renderer_update           (ClayViewRenderer   *renderer);
void   clay_view_renderer_update_idle      (ClayViewRenderer   *renderer);
void   clay_view_renderer_remove_idle      (ClayViewRenderer   *renderer);

void   clay_view_renderer_draw             (ClayViewRenderer   *renderer,
                                            GtkWidget          *widget,
                                            cairo_t            *cr,
                                            gint                available_width,
                                            gint                available_height);

/*  protected  */

void   clay_view_renderer_render_temp_buf_simple (ClayViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  ClayTempBuf      *temp_buf,
                                                  gint              temp_buf_scale,
                                                  ClayViewBG        inside_bg,
                                                  ClayViewBG        outside_bg);
void   clay_view_renderer_render_temp_buf        (ClayViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  ClayTempBuf      *temp_buf,
                                                  gint              temp_buf_scale,
                                                  gint              temp_buf_scaled_x,
                                                  gint              temp_buf_scaled_y,
                                                  gint              channel,
                                                  ClayViewBG        inside_bg,
                                                  ClayViewBG        outside_bg);
void   clay_view_renderer_render_pixbuf          (ClayViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  GdkPixbuf        *pixbuf,
                                                  gint              pixbuf_scale);
void   clay_view_renderer_render_icon            (ClayViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  const gchar      *icon_name,
                                                  gint              icon_scale);
ClayColorTransform *
       clay_view_renderer_get_color_transform    (ClayViewRenderer *renderer,
                                                  GtkWidget        *widget,
                                                  const Babl       *src_format,
                                                  const Babl       *dest_format);
void   clay_view_renderer_free_color_transform   (ClayViewRenderer *renderer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayViewRenderer, g_object_unref);
