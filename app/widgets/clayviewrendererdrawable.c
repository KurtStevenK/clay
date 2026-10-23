/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererdrawable.c
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

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/clayasync.h"
#include "core/claycancelable.h"
#include "core/claydrawable.h"
#include "core/claydrawable-preview.h"
#include "core/clayimage.h"
#include "core/claytempbuf.h"

#include "clayviewrendererdrawable.h"


struct _ClayViewRendererDrawablePrivate
{
  ClayAsync *render_async;
  GtkWidget *render_widget;
  gint       render_scale_factor;
  gint       render_buf_x;
  gint       render_buf_y;
  gboolean   render_update;

  gint       prev_width;
  gint       prev_height;
};


/*  local function prototypes  */

static void   clay_view_renderer_drawable_dispose       (GObject                  *object);

static void   clay_view_renderer_drawable_invalidate    (ClayViewRenderer         *renderer);
static void   clay_view_renderer_drawable_render        (ClayViewRenderer         *renderer,
                                                         GtkWidget                *widget);

static void   clay_view_renderer_drawable_cancel_render (ClayViewRendererDrawable *renderdrawable);


G_DEFINE_TYPE_WITH_PRIVATE (ClayViewRendererDrawable,
                            clay_view_renderer_drawable,
                            CLAY_TYPE_VIEW_RENDERER)

#define parent_class clay_view_renderer_drawable_parent_class


/*  private functions  */

static void
clay_view_renderer_drawable_class_init (ClayViewRendererDrawableClass *klass)
{
  GObjectClass          *object_class   = G_OBJECT_CLASS (klass);
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  object_class->dispose      = clay_view_renderer_drawable_dispose;

  renderer_class->invalidate = clay_view_renderer_drawable_invalidate;
  renderer_class->render     = clay_view_renderer_drawable_render;
}

static void
clay_view_renderer_drawable_init (ClayViewRendererDrawable *renderdrawable)
{
  renderdrawable->priv =
    clay_view_renderer_drawable_get_instance_private (renderdrawable);
}

static void
clay_view_renderer_drawable_dispose (GObject *object)
{
  ClayViewRendererDrawable *renderdrawable = CLAY_VIEW_RENDERER_DRAWABLE (object);

  clay_view_renderer_drawable_cancel_render (renderdrawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_view_renderer_drawable_invalidate (ClayViewRenderer *renderer)
{
  ClayViewRendererDrawable *renderdrawable = CLAY_VIEW_RENDERER_DRAWABLE (renderer);

  clay_view_renderer_drawable_cancel_render (renderdrawable);

  CLAY_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
clay_view_renderer_drawable_render_async_callback (ClayAsync                *async,
                                                   ClayViewRendererDrawable *renderdrawable)
{
  GtkWidget *widget;

  /* rendering was canceled, and the view renderer is potentially dead (see
   * clay_view_renderer_drawable_cancel_render()).  bail.
   */
  if (clay_async_is_canceled (async))
    return;

  widget = renderdrawable->priv->render_widget;

  renderdrawable->priv->render_async  = NULL;
  renderdrawable->priv->render_widget = NULL;

  if (clay_async_is_finished (async))
    {
      ClayViewRenderer *renderer   = CLAY_VIEW_RENDERER (renderdrawable);
      ClayTempBuf      *render_buf = clay_async_get_result (async);

      clay_view_renderer_render_temp_buf (
        renderer,
        widget,
        render_buf,
        renderdrawable->priv->render_scale_factor,
        renderdrawable->priv->render_buf_x,
        renderdrawable->priv->render_buf_y,
        -1,
        CLAY_VIEW_BG_CHECKS,
        CLAY_VIEW_BG_CHECKS);

      if (renderdrawable->priv->render_update)
        clay_view_renderer_update (renderer);
    }

  g_object_unref (widget);
}

static void
clay_view_renderer_drawable_render (ClayViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  ClayViewRendererDrawable *renderdrawable = CLAY_VIEW_RENDERER_DRAWABLE (renderer);
  ClayDrawable             *drawable;
  ClayItem                 *item;
  ClayImage                *image;
  const gchar              *icon_name;
  ClayAsync                *async;
  gint                      scale_factor;
  gint                      image_width;
  gint                      image_height;
  gint                      view_width;
  gint                      view_height;
  gint                      src_x;
  gint                      src_y;
  gint                      src_width;
  gint                      src_height;
  gint                      dst_x;
  gint                      dst_y;
  gint                      dst_width;
  gint                      dst_height;
  gdouble                   xres  = 1.0;
  gdouble                   yres  = 1.0;
  gboolean                  empty = FALSE;

  /* render is already in progress */
  if (renderdrawable->priv->render_async)
    return;

  drawable  = CLAY_DRAWABLE (renderer->viewable);
  item      = CLAY_ITEM (drawable);
  image     = clay_item_get_image (item);
  icon_name = clay_viewable_get_icon_name (renderer->viewable);

  scale_factor = gtk_widget_get_scale_factor (widget);

  if (image && ! image->clay->config->layer_previews)
    {
      renderdrawable->priv->prev_width  = 0;
      renderdrawable->priv->prev_height = 0;

      clay_view_renderer_render_icon (renderer, widget,
                                      icon_name, scale_factor);

      return;
    }

  if (image)
    clay_image_get_resolution (image, &xres, &yres);

  if (renderer->is_popup)
    image = NULL;

  if (image)
    {
      image_width  = clay_image_get_width  (image);
      image_height = clay_image_get_height (image);
    }
  else
    {
      image_width  = clay_item_get_width  (item);
      image_height = clay_item_get_height (item);
    }

  clay_viewable_calc_preview_size (image_width,
                                   image_height,
                                   renderer->width,
                                   renderer->height,
                                   renderer->dot_for_dot,
                                   xres,
                                   yres,
                                   &view_width,
                                   &view_height,
                                   NULL);

  src_x      = 0;
  src_y      = 0;
  src_width  = clay_item_get_width  (item);
  src_height = clay_item_get_height (item);

  if (image)
    {
      gint offset_x;
      gint offset_y;

      clay_item_get_offset (item, &offset_x, &offset_y);

      if (clay_rectangle_intersect (src_x, src_y,
                                    src_width, src_height,
                                    -offset_x, -offset_y,
                                    image_width, image_height,
                                    &src_x, &src_y,
                                    &src_width, &src_height))
        {
          offset_x += src_x;
          offset_y += src_y;

          dst_x      = ROUND (((gdouble) view_width  / image_width)  *
                              offset_x);
          dst_y      = ROUND (((gdouble) view_height / image_height) *
                              offset_y);
          dst_width  = ROUND (((gdouble) view_width  / image_width)  *
                              src_width);
          dst_height = ROUND (((gdouble) view_height / image_height) *
                              src_height);
        }
      else
        {
          dst_x      = 0;
          dst_y      = 0;
          dst_width  = 1;
          dst_height = 1;

          empty = TRUE;
        }
    }
  else
    {
      dst_x      = (renderer->width  - view_width)  / 2;
      dst_y      = (renderer->height - view_height) / 2;
      dst_width  = view_width;
      dst_height = view_height;
    }

  dst_width  = MAX (dst_width,  1);
  dst_height = MAX (dst_height, 1);

  if (! empty)
    {
      async = clay_drawable_get_sub_preview_async (drawable,
                                                   src_x, src_y,
                                                   src_width, src_height,
                                                   dst_width, dst_height,
                                                   scale_factor);
    }
  else
    {
      const Babl  *format = clay_drawable_get_preview_format (drawable);
      ClayTempBuf *render_buf;

      async = clay_async_new ();

      render_buf = clay_temp_buf_new (dst_width  * scale_factor,
                                      dst_height * scale_factor,
                                      format);
      clay_temp_buf_data_clear (render_buf);

      clay_async_finish_full (async,
                              render_buf,
                              (GDestroyNotify) clay_temp_buf_unref);
    }

  if (async)
    {
      renderdrawable->priv->render_async        = async;
      renderdrawable->priv->render_widget       = g_object_ref (widget);
      renderdrawable->priv->render_scale_factor = scale_factor;
      renderdrawable->priv->render_buf_x        = dst_x * scale_factor;
      renderdrawable->priv->render_buf_y        = dst_y * scale_factor;
      renderdrawable->priv->render_update       = FALSE;

      clay_async_add_callback_for_object (
        async,
        (ClayAsyncCallback) clay_view_renderer_drawable_render_async_callback,
        renderdrawable,
        renderdrawable);

      /* if rendering isn't done yet, update the render-view once it is, and
       * either keep the old drawable preview for now, or, if size changed (or
       * there's no old preview,) render an icon in the meantime.
       */
      if (renderdrawable->priv->render_async)
        {
          renderdrawable->priv->render_update = TRUE;

          if (renderer->width  != renderdrawable->priv->prev_width ||
              renderer->height != renderdrawable->priv->prev_height)
            {
              clay_view_renderer_render_icon (renderer, widget,
                                              icon_name, scale_factor);
            }
        }

      renderdrawable->priv->prev_width  = renderer->width ;
      renderdrawable->priv->prev_height = renderer->height;

      g_object_unref (async);
    }
  else
    {
      renderdrawable->priv->prev_width  = 0;
      renderdrawable->priv->prev_height = 0;

      clay_view_renderer_render_icon (renderer, widget,
                                      icon_name, scale_factor);
    }
}

static void
clay_view_renderer_drawable_cancel_render (ClayViewRendererDrawable *renderdrawable)
{
  /* cancel the async render operation (if one is ongoing) without actually
   * waiting for it.  if the actual rendering hasn't started yet, it will be
   * immediately aborted; otherwise, it can't really be interrupted, so we just
   * let it go on without blocking the main thread.
   * clay_drawable_get_sub_preview_async() can continue rendering safely even
   * after the drawable had died, and our completion callback is prepared to
   * handle cancellation.
   */
  if (renderdrawable->priv->render_async)
    {
      clay_cancelable_cancel (
        CLAY_CANCELABLE (renderdrawable->priv->render_async));

      renderdrawable->priv->render_async = NULL;
    }

  g_clear_object (&renderdrawable->priv->render_widget);
}
