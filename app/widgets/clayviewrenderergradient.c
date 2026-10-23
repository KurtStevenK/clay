/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrenderergradient.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claygradient.h"

#include "clayviewrenderergradient.h"


static void   clay_view_renderer_gradient_set_context (ClayViewRenderer *renderer,
                                                       ClayContext      *context);
static void   clay_view_renderer_gradient_invalidate  (ClayViewRenderer *renderer);
static void   clay_view_renderer_gradient_render      (ClayViewRenderer *renderer,
                                                       GtkWidget        *widget);


G_DEFINE_TYPE (ClayViewRendererGradient, clay_view_renderer_gradient,
               CLAY_TYPE_VIEW_RENDERER);

#define parent_class clay_view_renderer_gradient_parent_class


static void
clay_view_renderer_gradient_class_init (ClayViewRendererGradientClass *klass)
{
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  renderer_class->default_bg      = CLAY_VIEW_BG_CHECKS;
  renderer_class->follow_theme_bg = CLAY_VIEW_BG_CHECKS;

  renderer_class->set_context     = clay_view_renderer_gradient_set_context;
  renderer_class->invalidate      = clay_view_renderer_gradient_invalidate;
  renderer_class->render          = clay_view_renderer_gradient_render;
}

static void
clay_view_renderer_gradient_init (ClayViewRendererGradient *renderer)
{
  renderer->left  = 0.0;
  renderer->right = 1.0;
}

static void
clay_view_renderer_gradient_fg_bg_changed (ClayContext      *context,
                                           GeglColor        *color,
                                           ClayViewRenderer *renderer)
{
#if 0
  g_printerr ("%s: invalidating %s\n", G_STRFUNC,
              clay_object_get_name (renderer->viewable));
#endif

  clay_view_renderer_invalidate (renderer);
}

static void
clay_view_renderer_gradient_set_context (ClayViewRenderer *renderer,
                                         ClayContext      *context)
{
  ClayViewRendererGradient *rendergrad;

  rendergrad = CLAY_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_handlers_disconnect_by_func (renderer->context,
                                            clay_view_renderer_gradient_fg_bg_changed,
                                            renderer);
    }

  CLAY_VIEW_RENDERER_CLASS (parent_class)->set_context (renderer, context);

  if (renderer->context && rendergrad->has_fg_bg_segments)
    {
      g_signal_connect (renderer->context, "foreground-changed",
                        G_CALLBACK (clay_view_renderer_gradient_fg_bg_changed),
                        renderer);
      g_signal_connect (renderer->context, "background-changed",
                        G_CALLBACK (clay_view_renderer_gradient_fg_bg_changed),
                        renderer);

      clay_view_renderer_gradient_fg_bg_changed (renderer->context,
                                                 NULL,
                                                 renderer);
    }
}

static void
clay_view_renderer_gradient_invalidate (ClayViewRenderer *renderer)
{
  ClayViewRendererGradient *rendergrad;
  gboolean                  has_fg_bg_segments = FALSE;

  rendergrad = CLAY_VIEW_RENDERER_GRADIENT (renderer);

  if (renderer->viewable)
    has_fg_bg_segments =
      clay_gradient_has_fg_bg_segments (CLAY_GRADIENT (renderer->viewable));

  if (rendergrad->has_fg_bg_segments != has_fg_bg_segments)
    {
      if (renderer->context)
        {
          if (rendergrad->has_fg_bg_segments)
            {
              g_signal_handlers_disconnect_by_func (renderer->context,
                                                    clay_view_renderer_gradient_fg_bg_changed,
                                                    renderer);
            }
          else
            {
              g_signal_connect (renderer->context, "foreground-changed",
                                G_CALLBACK (clay_view_renderer_gradient_fg_bg_changed),
                                renderer);
              g_signal_connect (renderer->context, "background-changed",
                                G_CALLBACK (clay_view_renderer_gradient_fg_bg_changed),
                                renderer);
            }
        }

      rendergrad->has_fg_bg_segments = has_fg_bg_segments;
    }

  CLAY_VIEW_RENDERER_CLASS (parent_class)->invalidate (renderer);
}

static void
clay_view_renderer_gradient_render (ClayViewRenderer *renderer,
                                    GtkWidget        *widget)
{
  ClayViewRendererGradient *rendergrad = CLAY_VIEW_RENDERER_GRADIENT (renderer);
  ClayGradient             *gradient   = CLAY_GRADIENT (renderer->viewable);
  ClayGradientSegment      *seg        = NULL;
  ClayColorConfig          *color_config;
  const Babl               *dest_space;
  guchar                   *buf;
  guchar                   *dest;
  gint                      dest_stride;
  gint                      x;
  gint                      y;
  gdouble                   dx, cur_x;

  buf   = g_alloca (4 * renderer->width);
  dx    = (rendergrad->right - rendergrad->left) / (renderer->width - 1);
  cur_x = rendergrad->left;

  color_config = clay_view_renderer_get_color_config (renderer);

  g_return_if_fail (color_config != NULL);

  dest_space = clay_widget_get_render_space (widget, color_config);
  for (x = 0, dest = buf; x < renderer->width; x++, dest += 4)
    {
      GeglColor *color = NULL;

      seg = clay_gradient_get_color_at (gradient, renderer->context, seg,
                                        cur_x,
                                        rendergrad->reverse,
                                        rendergrad->blend_color_space,
                                        &color);
      cur_x += dx;

      /* This is only for rendering the gradient on thumbnails or small
       * previews, so cairo-ARGB32 is probably enough.
       */
      gegl_color_get_pixel (color,
                            babl_format_with_space ("cairo-ARGB32", dest_space),
                            dest);

      g_object_unref (color);
    }

  if (! renderer->surface)
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                    renderer->width,
                                                    renderer->height);

  cairo_surface_flush (renderer->surface);

  dest        = cairo_image_surface_get_data (renderer->surface);
  dest_stride = cairo_image_surface_get_stride (renderer->surface);

  for (y = 0; y < renderer->height; y++, dest += dest_stride)
    memcpy (dest, buf, renderer->width * 4);

  cairo_surface_mark_dirty (renderer->surface);
}

void
clay_view_renderer_gradient_set_offsets (ClayViewRendererGradient *renderer,
                                         gdouble                   left,
                                         gdouble                   right)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER_GRADIENT (renderer));

  left  = CLAMP (left, 0.0, 1.0);
  right = CLAMP (right, left, 1.0);

  if (left != renderer->left || right != renderer->right)
    {
      renderer->left  = left;
      renderer->right = right;

      clay_view_renderer_invalidate (CLAY_VIEW_RENDERER (renderer));
    }
}

void
clay_view_renderer_gradient_set_reverse (ClayViewRendererGradient *renderer,
                                         gboolean                  reverse)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (reverse != renderer->reverse)
    {
      renderer->reverse = reverse ? TRUE : FALSE;

      clay_view_renderer_invalidate (CLAY_VIEW_RENDERER (renderer));
      clay_view_renderer_update (CLAY_VIEW_RENDERER (renderer));
    }
}

void
clay_view_renderer_gradient_set_blend_color_space (ClayViewRendererGradient    *renderer,
                                                   ClayGradientBlendColorSpace  blend_color_space)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER_GRADIENT (renderer));

  if (blend_color_space != renderer->blend_color_space)
    {
      renderer->blend_color_space = blend_color_space;

      clay_view_renderer_invalidate (CLAY_VIEW_RENDERER (renderer));
      clay_view_renderer_update (CLAY_VIEW_RENDERER (renderer));
    }
}
