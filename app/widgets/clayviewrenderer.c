/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrenderer.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2007 Sven Neumann <sven@gimp.org>
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

#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"
#include "config/clayguiconfig.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claytempbuf.h"
#include "core/clayviewable.h"

#include "clayrender.h"
#include "clayviewrenderer.h"
#include "clayviewrenderer-utils.h"
#include "claywidgets-utils.h"

#include "clay-priorities.h"


#define RGB_EPSILON 1e-6

enum
{
  UPDATE,
  LAST_SIGNAL
};


struct _ClayViewRendererPrivate
{
  ClayViewBorderType  border_type;
  GeglColor          *border_color;

  cairo_pattern_t    *pattern;
  gchar              *bg_icon_name;

  ClayColorConfig    *color_config;
  ClayColorTransform *profile_transform;

  gboolean            needs_render;
  guint               idle_id;

  /* Store signal handler IDs so we can disconnect via g_signal_handler_disconnect
   * instead of g_signal_handlers_disconnect_by_func which can be expensive
   * as it scans through all handlers on the object.
   * This only is relevant for the font list widget where we can have thousands
   * of renderers/fonts. Each renderer connects to the shared ClayCoreConfig and
   * ClayColorConfig objects with itself as the data argument. With N renderers
   * those shared objects accumulate N handlers each.
   * Disconnecting all of them on teardown via g_signal_handlers_disconnect_by_func
   * would be O(N^2) while disconnecting via g_signal_handler_disconnect
   * is just O(N).
   */
  gulong              config_follow_theme_handler_id;
  gulong              config_color_scheme_handler_id;
  gulong              color_config_handler_id;
};


static void      clay_view_renderer_dispose           (GObject            *object);
static void      clay_view_renderer_finalize          (GObject            *object);

static gboolean  clay_view_renderer_idle_update       (ClayViewRenderer   *renderer);
static void      clay_view_renderer_real_set_context  (ClayViewRenderer   *renderer,
                                                       ClayContext        *context);
static void      clay_view_renderer_real_invalidate   (ClayViewRenderer   *renderer);
static void      clay_view_renderer_real_draw         (ClayViewRenderer   *renderer,
                                                       GtkWidget          *widget,
                                                       cairo_t            *cr,
                                                       gint                available_width,
                                                       gint                available_height);
static void      clay_view_renderer_real_render       (ClayViewRenderer   *renderer,
                                                       GtkWidget          *widget);

static void      clay_view_renderer_size_changed      (ClayViewRenderer   *renderer,
                                                       ClayViewable       *viewable);
static void      clay_view_renderer_profile_changed   (ClayViewRenderer   *renderer,
                                                       ClayViewable       *viewable);
static void      clay_view_renderer_config_notify     (GObject            *config,
                                                       const GParamSpec   *pspec,
                                                       ClayViewRenderer   *renderer);

static void      clay_view_render_temp_buf_to_surface (ClayViewRenderer   *renderer,
                                                       GtkWidget          *widget,
                                                       ClayTempBuf        *temp_buf,
                                                       gint                temp_buf_scale,
                                                       gint                temp_buf_x,
                                                       gint                temp_buf_y,
                                                       gint                channel,
                                                       ClayViewBG          inside_bg,
                                                       ClayViewBG          outside_bg,
                                                       cairo_surface_t    *surface,
                                                       gint                dest_width,
                                                       gint                dest_height);

static cairo_pattern_t *
                 clay_view_renderer_create_background (ClayViewRenderer   *renderer,
                                                       GtkWidget          *widget);

static void      clay_view_renderer_redraw            (ClayGuiConfig      *config,
                                                       const GParamSpec   *pspec,
                                                       ClayViewRenderer   *renderer);


G_DEFINE_TYPE_WITH_PRIVATE (ClayViewRenderer, clay_view_renderer, G_TYPE_OBJECT)

#define parent_class clay_view_renderer_parent_class

static guint renderer_signals[LAST_SIGNAL] = { 0 };


static void
clay_view_renderer_class_init (ClayViewRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  renderer_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayViewRendererClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose  = clay_view_renderer_dispose;
  object_class->finalize = clay_view_renderer_finalize;

  klass->default_bg      = CLAY_VIEW_BG_STYLE;
  klass->follow_theme_bg = CLAY_VIEW_BG_STYLE;

  klass->frame           = NULL;
  klass->frame_left      = 0;
  klass->frame_right     = 0;
  klass->frame_top       = 0;
  klass->frame_bottom    = 0;

  klass->update          = NULL;
  klass->set_context     = clay_view_renderer_real_set_context;
  klass->invalidate      = clay_view_renderer_real_invalidate;
  klass->draw            = clay_view_renderer_real_draw;
  klass->render          = clay_view_renderer_real_render;
}

static void
clay_view_renderer_init (ClayViewRenderer *renderer)
{
  renderer->priv = clay_view_renderer_get_instance_private (renderer);

  renderer->dot_for_dot = TRUE;
  renderer->surface_bg  = CLAY_VIEW_BG_STYLE;
  renderer->size        = -1;

  renderer->priv->border_type  = CLAY_VIEW_BORDER_STYLE_FG;
  renderer->priv->needs_render = TRUE;
}

static void
clay_view_renderer_dispose (GObject *object)
{
  ClayViewRenderer *renderer = CLAY_VIEW_RENDERER (object);

  if (renderer->viewable)
    clay_view_renderer_set_viewable (renderer, NULL);

  if (renderer->context)
    clay_view_renderer_set_context (renderer, NULL);

  g_clear_object (&renderer->priv->border_color);

  if (renderer->priv->color_config)
    clay_view_renderer_set_color_config (renderer, NULL);

  clay_view_renderer_remove_idle (renderer);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_view_renderer_finalize (GObject *object)
{
  ClayViewRenderer *renderer = CLAY_VIEW_RENDERER (object);

  g_clear_pointer (&renderer->surface, cairo_surface_destroy);
  g_clear_pointer (&renderer->priv->pattern, cairo_pattern_destroy);
  g_clear_pointer (&renderer->priv->bg_icon_name, g_free);
  g_clear_object  (&renderer->priv->border_color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static ClayViewRenderer *
clay_view_renderer_new_internal (ClayContext *context,
                                 GType        viewable_type,
                                 gboolean     is_popup)
{
  ClayViewRenderer *renderer;

  renderer = g_object_new (clay_view_renderer_type_from_viewable_type (viewable_type),
                           NULL);

  renderer->viewable_type = viewable_type;
  renderer->is_popup      = is_popup ? TRUE : FALSE;

  if (context)
    clay_view_renderer_set_context (renderer, context);

  return renderer;
}


/*  public functions  */

ClayViewRenderer *
clay_view_renderer_new (ClayContext *context,
                        GType        viewable_type,
                        gint         size,
                        gint         border_width,
                        gboolean     is_popup)
{
  ClayViewRenderer *renderer;

  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, CLAY_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (size >  0 &&
                        size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= CLAY_VIEW_MAX_BORDER_WIDTH, NULL);

  renderer = clay_view_renderer_new_internal (context, viewable_type,
                                              is_popup);

  clay_view_renderer_set_size (renderer, size, border_width);
  clay_view_renderer_remove_idle (renderer);

  return renderer;
}

ClayViewRenderer *
clay_view_renderer_new_full (ClayContext *context,
                             GType        viewable_type,
                             gint         width,
                             gint         height,
                             gint         border_width,
                             gboolean     is_popup)
{
  ClayViewRenderer *renderer;

  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (g_type_is_a (viewable_type, CLAY_TYPE_VIEWABLE), NULL);
  g_return_val_if_fail (width >  0 &&
                        width <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (height > 0 &&
                        height <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (border_width >= 0 &&
                        border_width <= CLAY_VIEW_MAX_BORDER_WIDTH, NULL);

  renderer = clay_view_renderer_new_internal (context, viewable_type,
                                              is_popup);

  clay_view_renderer_set_size_full (renderer, width, height, border_width);
  clay_view_renderer_remove_idle (renderer);

  return renderer;
}

void
clay_view_renderer_set_context (ClayViewRenderer *renderer,
                                ClayContext      *context)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (context != renderer->context)
    {
      if (renderer->context)
        {
          ClayCoreConfig *config = renderer->context->clay->config;

          g_signal_handler_disconnect (config,
                                       renderer->priv->config_follow_theme_handler_id);
          g_signal_handler_disconnect (config,
                                       renderer->priv->config_color_scheme_handler_id);
        }

      CLAY_VIEW_RENDERER_GET_CLASS (renderer)->set_context (renderer, context);

      if (renderer->context)
        {
          ClayViewRendererClass *klass  = CLAY_VIEW_RENDERER_GET_CLASS (renderer);
          ClayCoreConfig        *config = renderer->context->clay->config;

          renderer->priv->config_follow_theme_handler_id =
            g_signal_connect_object (config,
                                     "notify::viewables-follow-theme",
                                     G_CALLBACK (clay_view_renderer_redraw),
                                     renderer, 0);
          renderer->priv->config_color_scheme_handler_id =
            g_signal_connect_object (config,
                                     "notify::theme-color-scheme",
                                     G_CALLBACK (clay_view_renderer_redraw),
                                     renderer, 0);

          if (CLAY_GUI_CONFIG (config)->viewables_follow_theme)
            renderer->surface_bg = klass->follow_theme_bg;
          else
            renderer->surface_bg = klass->default_bg;
        }

      if (renderer->viewable)
        clay_view_renderer_invalidate (renderer);
    }
}

static void
clay_view_renderer_weak_notify (ClayViewRenderer *renderer,
                                ClayViewable     *viewable)
{
  renderer->viewable = NULL;

  clay_view_renderer_update_idle (renderer);
}

void
clay_view_renderer_set_viewable (ClayViewRenderer *renderer,
                                 ClayViewable     *viewable)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (viewable == NULL || CLAY_IS_VIEWABLE (viewable));

  if (viewable)
    g_return_if_fail (g_type_is_a (G_TYPE_FROM_INSTANCE (viewable),
                                   renderer->viewable_type));

  if (viewable == renderer->viewable)
    return;

  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  clay_view_renderer_free_color_transform (renderer);

  if (renderer->viewable)
    {
      g_object_weak_unref (G_OBJECT (renderer->viewable),
                           (GWeakNotify) clay_view_renderer_weak_notify,
                           renderer);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (clay_view_renderer_invalidate),
                                            renderer);

      g_signal_handlers_disconnect_by_func (renderer->viewable,
                                            G_CALLBACK (clay_view_renderer_size_changed),
                                            renderer);

      if (CLAY_IS_COLOR_MANAGED (renderer->viewable))
        g_signal_handlers_disconnect_by_func (renderer->viewable,
                                              G_CALLBACK (clay_view_renderer_profile_changed),
                                              renderer);
    }

  renderer->viewable = viewable;

  if (renderer->viewable)
    {
      g_object_weak_ref (G_OBJECT (renderer->viewable),
                         (GWeakNotify) clay_view_renderer_weak_notify,
                         renderer);

      g_signal_connect_swapped (renderer->viewable,
                                "invalidate-preview",
                                G_CALLBACK (clay_view_renderer_invalidate),
                                renderer);

      g_signal_connect_swapped (renderer->viewable,
                                "size-changed",
                                G_CALLBACK (clay_view_renderer_size_changed),
                                renderer);

      if (CLAY_IS_COLOR_MANAGED (renderer->viewable))
        g_signal_connect_swapped (renderer->viewable,
                                  "profile-changed",
                                  G_CALLBACK (clay_view_renderer_profile_changed),
                                  renderer);

      if (renderer->size != -1)
        clay_view_renderer_set_size (renderer, renderer->size,
                                     renderer->border_width);

      clay_view_renderer_invalidate (renderer);
    }
  else
    {
      clay_view_renderer_update_idle (renderer);
    }
}

void
clay_view_renderer_set_size (ClayViewRenderer *renderer,
                             gint              view_size,
                             gint              border_width)
{
  gint width;
  gint height;

  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (view_size >  0 &&
                    view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= CLAY_VIEW_MAX_BORDER_WIDTH);

  renderer->size = view_size;

  if (renderer->viewable)
    {
      clay_viewable_get_preview_size (renderer->viewable,
                                      view_size,
                                      renderer->is_popup,
                                      renderer->dot_for_dot,
                                      &width, &height);
    }
  else
    {
      width  = view_size;
      height = view_size;
    }

  clay_view_renderer_set_size_full (renderer, width, height, border_width);
}

void
clay_view_renderer_set_size_full (ClayViewRenderer *renderer,
                                  gint              width,
                                  gint              height,
                                  gint              border_width)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (width >  0 &&
                    width <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (height > 0 &&
                    height <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);
  g_return_if_fail (border_width >= 0 &&
                    border_width <= CLAY_VIEW_MAX_BORDER_WIDTH);

  if (width        != renderer->width  ||
      height       != renderer->height ||
      border_width != renderer->border_width)
    {
      renderer->width        = width;
      renderer->height       = height;
      renderer->border_width = border_width;

      g_clear_pointer (&renderer->surface, cairo_surface_destroy);

      if (renderer->viewable)
        clay_view_renderer_invalidate (renderer);
    }
}

void
clay_view_renderer_set_dot_for_dot (ClayViewRenderer *renderer,
                                    gboolean          dot_for_dot)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (dot_for_dot != renderer->dot_for_dot)
    {
      renderer->dot_for_dot = dot_for_dot ? TRUE: FALSE;

      if (renderer->size != -1)
        clay_view_renderer_set_size (renderer, renderer->size,
                                     renderer->border_width);

      clay_view_renderer_invalidate (renderer);
    }
}

void
clay_view_renderer_set_border_type (ClayViewRenderer   *renderer,
                                    ClayViewBorderType  border_type)
{
  GeglColor *border_color = NULL;

  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  renderer->priv->border_type = border_type;

  switch (border_type)
    {
    case CLAY_VIEW_BORDER_BLACK:
      border_color = gegl_color_new ("black");
      break;
    case CLAY_VIEW_BORDER_WHITE:
      border_color = gegl_color_new ("white");
      break;
    case CLAY_VIEW_BORDER_GREEN:
      border_color = gegl_color_new ("green");
      gegl_color_set_rgba (border_color, 0.0, 0.94, 0.0, 1.0);
      break;
    case CLAY_VIEW_BORDER_RED:
      border_color = gegl_color_new ("red");
      break;
    case CLAY_VIEW_BORDER_STYLE_FG:
      /* get later when we have a widget */
      break;
    case CLAY_VIEW_BORDER_STYLE_BG:
      border_color = gegl_color_new ("transparent");
      break;
    }

  g_set_object (&renderer->priv->border_color, border_color);

  clay_view_renderer_update_idle (renderer);
}

ClayViewBorderType
clay_view_renderer_get_border_type (ClayViewRenderer *renderer)
{
  g_return_val_if_fail (CLAY_IS_VIEW_RENDERER (renderer),
                        CLAY_VIEW_BORDER_STYLE_FG);

  return renderer->priv->border_type;
}

void
clay_view_renderer_set_background (ClayViewRenderer *renderer,
                                   const gchar      *icon_name)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (renderer->priv->bg_icon_name)
    g_free (renderer->priv->bg_icon_name);

  renderer->priv->bg_icon_name = g_strdup (icon_name);

  g_clear_object (&renderer->priv->pattern);
}

void
clay_view_renderer_set_color_config (ClayViewRenderer *renderer,
                                     ClayColorConfig  *color_config)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (color_config == NULL || CLAY_IS_COLOR_CONFIG (color_config));

  if (color_config != renderer->priv->color_config)
    {
      if (renderer->priv->color_config)
        g_signal_handler_disconnect (renderer->priv->color_config,
                                     renderer->priv->color_config_handler_id);

      g_set_object (&renderer->priv->color_config, color_config);

      if (renderer->priv->color_config)
        renderer->priv->color_config_handler_id =
          g_signal_connect (renderer->priv->color_config, "notify",
                            G_CALLBACK (clay_view_renderer_config_notify),
                            renderer);

      clay_view_renderer_config_notify (G_OBJECT (renderer->priv->color_config),
                                        NULL, renderer);
    }
}

ClayColorConfig *
clay_view_renderer_get_color_config (ClayViewRenderer *renderer)
{
  g_return_val_if_fail (CLAY_IS_VIEW_RENDERER (renderer), NULL);

  return renderer->priv->color_config;
}

void
clay_view_renderer_invalidate (ClayViewRenderer *renderer)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (renderer->priv->idle_id)
    {
      g_source_remove (renderer->priv->idle_id);
      renderer->priv->idle_id = 0;
    }

  CLAY_VIEW_RENDERER_GET_CLASS (renderer)->invalidate (renderer);

  renderer->priv->idle_id =
    g_idle_add_full (CLAY_PRIORITY_VIEWABLE_IDLE,
                     (GSourceFunc) clay_view_renderer_idle_update,
                     renderer, NULL);
}

void
clay_view_renderer_update (ClayViewRenderer *renderer)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (renderer->priv->idle_id)
    {
      g_source_remove (renderer->priv->idle_id);
      renderer->priv->idle_id = 0;
    }

  g_signal_emit (renderer, renderer_signals[UPDATE], 0);
}

void
clay_view_renderer_update_idle (ClayViewRenderer *renderer)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (renderer->priv->idle_id)
    g_source_remove (renderer->priv->idle_id);

  renderer->priv->idle_id =
    g_idle_add_full (CLAY_PRIORITY_VIEWABLE_IDLE,
                     (GSourceFunc) clay_view_renderer_idle_update,
                     renderer, NULL);
}

void
clay_view_renderer_remove_idle (ClayViewRenderer *renderer)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  if (renderer->priv->idle_id)
    {
      g_source_remove (renderer->priv->idle_id);
      renderer->priv->idle_id = 0;
    }
}

void
clay_view_renderer_draw (ClayViewRenderer *renderer,
                         GtkWidget        *widget,
                         cairo_t          *cr,
                         gint              available_width,
                         gint              available_height)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (cr != NULL);

  if (G_UNLIKELY (renderer->context == NULL))
    g_warning ("%s: renderer->context is NULL", G_STRFUNC);

  if (! gtk_widget_is_drawable (widget))
    return;

  if (renderer->viewable)
    {
      cairo_save (cr);

      CLAY_VIEW_RENDERER_GET_CLASS (renderer)->draw (renderer, widget, cr,
                                                     available_width,
                                                     available_height);

      cairo_restore (cr);
    }
  else
    {
      ClayViewableClass *viewable_class;

      viewable_class = g_type_class_ref (renderer->viewable_type);

      clay_view_renderer_render_icon (renderer,
                                      widget,
                                      viewable_class->default_icon_name,
                                      gtk_widget_get_scale_factor (widget));
      renderer->priv->needs_render = FALSE;

      g_type_class_unref (viewable_class);

      clay_view_renderer_real_draw (renderer, widget, cr,
                                    available_width,
                                    available_height);
    }

  if (renderer->border_width > 0)
    {
      GeglColor *color;
      gint       width  = renderer->width  + renderer->border_width;
      gint       height = renderer->height + renderer->border_width;
      gdouble    x, y;

      if (renderer->priv->border_color)
        color = g_object_ref (renderer->priv->border_color);
      else
        color = clay_get_style_color (widget, GTK_STYLE_PROPERTY_COLOR);

      cairo_set_line_width (cr, renderer->border_width);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      clay_cairo_set_source_color (cr, color, NULL, FALSE, widget);

      g_object_unref (color);

      x = (available_width  - width)  / 2.0;
      y = (available_height - height) / 2.0;

      cairo_rectangle (cr, x, y, width, height);
      cairo_stroke (cr);
    }
}


/*  private functions  */

static gboolean
clay_view_renderer_idle_update (ClayViewRenderer *renderer)
{
  renderer->priv->idle_id = 0;

  clay_view_renderer_update (renderer);

  return FALSE;
}

static void
clay_view_renderer_real_set_context (ClayViewRenderer *renderer,
                                     ClayContext      *context)
{
  if (renderer->context &&
      renderer->priv->color_config ==
      renderer->context->clay->config->color_management)
    {
      clay_view_renderer_set_color_config (renderer, NULL);
    }

  g_set_object (&renderer->context, context);

  if (renderer->context &&
      renderer->priv->color_config == NULL)
    {
      clay_view_renderer_set_color_config (renderer,
                                           renderer->context->clay->config->color_management);
    }
}

static void
clay_view_renderer_real_invalidate (ClayViewRenderer *renderer)
{
  renderer->priv->needs_render = TRUE;
}

static void
clay_view_renderer_real_draw (ClayViewRenderer *renderer,
                              GtkWidget        *widget,
                              cairo_t          *cr,
                              gint              available_width,
                              gint              available_height)
{
  if (renderer->priv->needs_render)
    {
      CLAY_VIEW_RENDERER_GET_CLASS (renderer)->render (renderer, widget);

      renderer->priv->needs_render = FALSE;
    }

  if (renderer->priv->bg_icon_name)
    {
      if (! renderer->priv->pattern)
        {
          renderer->priv->pattern =
            clay_view_renderer_create_background (renderer, widget);
        }

      cairo_set_source (cr, renderer->priv->pattern);
      cairo_paint (cr);
    }
  else
    {
      gint width    = renderer->width;
      gint height   = renderer->height;
      gint offset_x = (available_width  - width)  / 2;
      gint offset_y = (available_height - height) / 2;

      cairo_translate (cr, offset_x, offset_y);
      cairo_rectangle (cr, 0, 0, width, height);

      switch (renderer->surface_bg)
        {
        case CLAY_VIEW_BG_CHECKS:
          if (! renderer->priv->pattern)
            {
              const GeglColor *rgb1 = clay_render_check_color1 ();
              const GeglColor *rgb2 = clay_render_check_color2 ();

              renderer->priv->pattern =
                clay_cairo_checkerboard_create (cr, CLAY_CHECK_SIZE_SM,
                                                rgb1, rgb2);
            }

          cairo_set_source (cr, renderer->priv->pattern);
          cairo_fill (cr);
          break;

        case CLAY_VIEW_BG_WHITE:
          cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
          cairo_fill (cr);
          break;

        case CLAY_VIEW_BG_STYLE:
          break;
        }

      cairo_translate (cr, -offset_x, -offset_y);
    }

  if (renderer->surface)
    {
      gdouble scale_x;
      gdouble scale_y;
      gint    width;
      gint    height;
      gint    x, y;

      width  = cairo_image_surface_get_width  (renderer->surface);
      height = cairo_image_surface_get_height (renderer->surface);

      cairo_surface_get_device_scale (renderer->surface, &scale_x, &scale_y);

      width  /= scale_x;
      height /= scale_y;

      x = (available_width  - width)  / 2;
      y = (available_height - height) / 2;

      cairo_rectangle (cr, x, y, width, height);

      cairo_set_source_surface (cr, renderer->surface, x, y);
      cairo_fill (cr);
    }
}

static void
clay_view_renderer_real_render (ClayViewRenderer *renderer,
                                GtkWidget        *widget)
{
  GdkPixbuf   *pixbuf;
  ClayTempBuf *temp_buf;
  const gchar *icon_name;
  GeglColor   *fg_color     = NULL;
  ClayViewBG   inside_bg    = CLAY_VIEW_BG_CHECKS;
  ClayViewBG   outside_bg   = CLAY_VIEW_BG_WHITE;
  gint         scale_factor = gtk_widget_get_scale_factor (widget);

  if (renderer->context &&
      CLAY_GUI_CONFIG (renderer->context->clay->config)->viewables_follow_theme)
    {
      fg_color = clay_get_style_color (widget, GTK_STYLE_PROPERTY_COLOR);

      inside_bg  = CLAY_VIEW_BG_STYLE;
      outside_bg = CLAY_VIEW_BG_STYLE;
    }

  pixbuf = clay_viewable_get_pixbuf (renderer->viewable,
                                     renderer->context,
                                     renderer->width,
                                     renderer->height,
                                     scale_factor,
                                     fg_color);
  if (pixbuf)
    {
      clay_view_renderer_render_pixbuf (renderer, widget,
                                        pixbuf, scale_factor);
      g_clear_object (&fg_color);
      return;
    }

  temp_buf = clay_viewable_get_preview (renderer->viewable,
                                        renderer->context,
                                        renderer->width,
                                        renderer->height,
                                        scale_factor,
                                        fg_color);
  if (temp_buf)
    {
      clay_view_renderer_render_temp_buf_simple (renderer, widget,
                                                 temp_buf, scale_factor,
                                                 inside_bg,
                                                 outside_bg);
      g_clear_object (&fg_color);
      return;
    }

  icon_name = clay_viewable_get_icon_name (renderer->viewable);
  clay_view_renderer_render_icon (renderer, widget,
                                  icon_name, scale_factor);

  g_clear_object (&fg_color);
}

static void
clay_view_renderer_size_changed (ClayViewRenderer *renderer,
                                 ClayViewable     *viewable)
{
  if (renderer->size != -1)
    clay_view_renderer_set_size (renderer, renderer->size,
                                 renderer->border_width);

  clay_view_renderer_invalidate (renderer);
}

static void
clay_view_renderer_profile_changed (ClayViewRenderer *renderer,
                                    ClayViewable     *viewable)
{
  clay_view_renderer_free_color_transform (renderer);
}

static void
clay_view_renderer_config_notify (GObject          *config,
                                  const GParamSpec *pspec,
                                  ClayViewRenderer *renderer)
{
  clay_view_renderer_free_color_transform (renderer);
}


/*  protected functions  */

void
clay_view_renderer_render_temp_buf_simple (ClayViewRenderer *renderer,
                                           GtkWidget        *widget,
                                           ClayTempBuf      *temp_buf,
                                           gint              temp_buf_scale,
                                           ClayViewBG        inside_bg,
                                           ClayViewBG        outside_bg)
{
  gint temp_buf_scaled_x = 0;
  gint temp_buf_scaled_y = 0;
  gint temp_buf_width;
  gint temp_buf_height;

  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (temp_buf != NULL);

  temp_buf_width  = clay_temp_buf_get_width  (temp_buf);
  temp_buf_height = clay_temp_buf_get_height (temp_buf);

  if (temp_buf_width < renderer->width * temp_buf_scale)
    temp_buf_scaled_x =
      (renderer->width * temp_buf_scale - temp_buf_width)  / 2;

  if (temp_buf_height < renderer->height * temp_buf_scale)
    temp_buf_scaled_y =
      (renderer->height * temp_buf_scale - temp_buf_height) / 2;

  clay_view_renderer_render_temp_buf (renderer, widget,
                                      temp_buf,
                                      temp_buf_scale,
                                      temp_buf_scaled_x,
                                      temp_buf_scaled_y,
                                      -1,
                                      inside_bg,
                                      outside_bg);
}

void
clay_view_renderer_render_temp_buf (ClayViewRenderer *renderer,
                                    GtkWidget        *widget,
                                    ClayTempBuf      *temp_buf,
                                    gint              temp_buf_scale,
                                    gint              temp_buf_scaled_x,
                                    gint              temp_buf_scaled_y,
                                    gint              channel,
                                    ClayViewBG        inside_bg,
                                    ClayViewBG        outside_bg)
{
  gint surface_width  = renderer->width  * temp_buf_scale;
  gint surface_height = renderer->height * temp_buf_scale;

  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                  surface_width,
                                                  surface_height);
  cairo_surface_set_device_scale (renderer->surface,
                                  temp_buf_scale, temp_buf_scale);

  clay_view_render_temp_buf_to_surface (renderer,
                                        widget,
                                        temp_buf,
                                        temp_buf_scale,
                                        temp_buf_scaled_x,
                                        temp_buf_scaled_y,
                                        channel,
                                        inside_bg,
                                        outside_bg,
                                        renderer->surface,
                                        surface_width,
                                        surface_height);
}


void
clay_view_renderer_render_pixbuf (ClayViewRenderer *renderer,
                                  GtkWidget        *widget,
                                  GdkPixbuf        *pixbuf,
                                  gint              pixbuf_scale)
{
  ClayColorTransform *transform;
  const Babl         *format;

  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  format = clay_pixbuf_get_format (pixbuf);

  transform = clay_view_renderer_get_color_transform (renderer, widget,
                                                      format, format);

  if (transform)
    {
      GdkPixbuf *new;
      gint       width       = gdk_pixbuf_get_width  (pixbuf);
      gint       height      = gdk_pixbuf_get_height (pixbuf);
      gsize      src_stride  = gdk_pixbuf_get_rowstride (pixbuf);
      guchar    *src         = gdk_pixbuf_get_pixels (pixbuf);
      gsize      dest_stride;
      guchar    *dest;
      gint       i;

      new = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                            gdk_pixbuf_get_has_alpha (pixbuf),
                            8, width, height);

      dest_stride = gdk_pixbuf_get_rowstride (new);
      dest        = gdk_pixbuf_get_pixels (new);

      for (i = 0; i < height; i++)
        {
          clay_color_transform_process_pixels (transform,
                                               format, src,
                                               format, dest,
                                               width);

          src  += src_stride;
          dest += dest_stride;
        }

      pixbuf = new;
    }
  else
    {
      g_object_ref (pixbuf);
    }

  renderer->surface = gdk_cairo_surface_create_from_pixbuf (pixbuf,
                                                            pixbuf_scale,
                                                            NULL);
  g_object_unref (pixbuf);
}

void
clay_view_renderer_render_icon (ClayViewRenderer *renderer,
                                GtkWidget        *widget,
                                const gchar      *icon_name,
                                gint              icon_scale)
{
  GdkPixbuf *pixbuf;
  gint       width;
  gint       height;

  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (icon_name != NULL);

  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  pixbuf = clay_widget_load_icon (widget, icon_name,
                                  MIN (renderer->width, renderer->height));
  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  if (width  > renderer->width  * icon_scale ||
      height > renderer->height * icon_scale)
    {
      GdkPixbuf *scaled_pixbuf;

      clay_viewable_calc_preview_size (width, height,
                                       renderer->width  * icon_scale,
                                       renderer->height * icon_scale,
                                       TRUE, 1.0, 1.0,
                                       &width, &height,
                                       NULL);

      scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
                                               width  * icon_scale,
                                               height * icon_scale,
                                               GDK_INTERP_BILINEAR);

      g_object_unref (pixbuf);
      pixbuf = scaled_pixbuf;
    }

  renderer->surface = gdk_cairo_surface_create_from_pixbuf (pixbuf,
                                                            icon_scale,
                                                            NULL);
  g_object_unref (pixbuf);
}

ClayColorTransform *
clay_view_renderer_get_color_transform (ClayViewRenderer *renderer,
                                        GtkWidget        *widget,
                                        const Babl       *src_format,
                                        const Babl       *dest_format)
{
  ClayColorProfile         *profile;
  ClayColorProfile         *proof_profile     = NULL;
  ClayColorRenderingIntent  simulation_intent =
    CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
  gboolean                  simulation_bpc    = FALSE;
  ClayImage                *image;

  g_return_val_if_fail (CLAY_IS_VIEW_RENDERER (renderer), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);
  g_return_val_if_fail (src_format != NULL, NULL);
  g_return_val_if_fail (dest_format != NULL, NULL);

  if (renderer->priv->profile_transform)
    return renderer->priv->profile_transform;

  if (! renderer->priv->color_config)
    {
      g_printerr ("EEK\n");
      return NULL;
    }

  if (CLAY_IS_COLOR_MANAGED (renderer->viewable))
    {
      ClayColorManaged *managed = CLAY_COLOR_MANAGED (renderer->viewable);

      profile = clay_color_managed_get_color_profile (managed);
    }
  else
    {
      static ClayColorProfile *srgb_profile = NULL;

      if (G_UNLIKELY (! srgb_profile))
        srgb_profile = clay_color_profile_new_rgb_srgb ();

      profile = srgb_profile;
    }

  if (renderer->context)
    {
      image = clay_context_get_image (CLAY_CONTEXT (renderer->context));
      if (image)
        {
          proof_profile =
            clay_color_managed_get_simulation_profile (CLAY_COLOR_MANAGED (image));
          simulation_intent =
            clay_color_managed_get_simulation_intent (CLAY_COLOR_MANAGED (image));
          simulation_bpc =
            clay_color_managed_get_simulation_bpc (CLAY_COLOR_MANAGED (image));
        }
    }

  renderer->priv->profile_transform =
    clay_widget_get_color_transform (widget,
                                     renderer->priv->color_config,
                                     profile,
                                     src_format,
                                     dest_format,
                                     proof_profile,
                                     simulation_intent,
                                     simulation_bpc);

  return renderer->priv->profile_transform;
}

void
clay_view_renderer_free_color_transform (ClayViewRenderer *renderer)
{
  g_return_if_fail (CLAY_IS_VIEW_RENDERER (renderer));

  g_clear_object (&renderer->priv->profile_transform);

  clay_view_renderer_invalidate (renderer);
}


/*  private functions  */

static void
clay_view_render_temp_buf_to_surface (ClayViewRenderer *renderer,
                                      GtkWidget        *widget,
                                      ClayTempBuf      *temp_buf,
                                      gint              temp_buf_scale,
                                      gint              temp_buf_scaled_x,
                                      gint              temp_buf_scaled_y,
                                      gint              channel,
                                      ClayViewBG        inside_bg,
                                      ClayViewBG        outside_bg,
                                      cairo_surface_t  *surface,
                                      gint              surface_width,
                                      gint              surface_height)
{
  cairo_t    *cr;
  gint        x, y;
  gint        width, height;
  const Babl *temp_buf_format;
  gint        temp_buf_width;
  gint        temp_buf_height;

  g_return_if_fail (temp_buf != NULL);
  g_return_if_fail (surface != NULL);

  temp_buf_format = clay_temp_buf_get_format (temp_buf);
  temp_buf_width  = clay_temp_buf_get_width  (temp_buf);
  temp_buf_height = clay_temp_buf_get_height (temp_buf);

  /*  Here are the different cases this functions handles correctly:
   *  1)  Offset temp_buf which does not necessarily cover full image area
   *  2)  Color conversion of temp_buf if it is gray and image is color
   *  3)  Background check buffer for transparent temp_bufs
   *  4)  Using the optional "channel" argument, one channel can be extracted
   *      from a multi-channel temp_buf and composited as a grayscale
   *  Prereqs:
   *  1)  Grayscale temp_bufs have bytes == {1, 2}
   *  2)  Color temp_bufs have bytes == {3, 4}
   *  3)  If image is gray, then temp_buf should have bytes == {1, 2}
   */

  cr = cairo_create (surface);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  if (outside_bg == CLAY_VIEW_BG_CHECKS ||
      inside_bg  == CLAY_VIEW_BG_CHECKS)
    {
      if (! renderer->priv->pattern)
        {
          const GeglColor *rgb1 = clay_render_check_color1 ();
          const GeglColor *rgb2 = clay_render_check_color2 ();

          renderer->priv->pattern =
            clay_cairo_checkerboard_create (cr, CLAY_CHECK_SIZE_SM,
                                            rgb1, rgb2);
        }
    }

  switch (outside_bg)
    {
    case CLAY_VIEW_BG_CHECKS:
      cairo_set_source (cr, renderer->priv->pattern);
      cairo_paint (cr);
      break;

    case CLAY_VIEW_BG_WHITE:
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_paint (cr);
      break;

    case CLAY_VIEW_BG_STYLE:
      break;
    }

  if (! clay_rectangle_intersect (0, 0,
                                  surface_width, surface_height,
                                  temp_buf_scaled_x, temp_buf_scaled_y,
                                  temp_buf_width, temp_buf_height,
                                  &x, &y,
                                  &width, &height))
    {
      cairo_destroy (cr);
      return;
    }

  if (inside_bg != outside_bg                 &&
      babl_format_has_alpha (temp_buf_format) &&
      channel == -1)
    {
      cairo_scale (cr, 1.0 / temp_buf_scale, 1.0 / temp_buf_scale);
      cairo_rectangle (cr, x, y, width, height);
      cairo_scale (cr, temp_buf_scale, temp_buf_scale);

      switch (inside_bg)
        {
        case CLAY_VIEW_BG_CHECKS:
          cairo_set_source (cr, renderer->priv->pattern);
          cairo_fill (cr);
          break;

        case CLAY_VIEW_BG_WHITE:
          cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
          cairo_fill (cr);
          break;

        case CLAY_VIEW_BG_STYLE:
          break;
        }
    }

  if (channel == -1)
    {
      ClayColorTransform *transform;
      GeglBuffer         *src_buffer;
      GeglBuffer         *dest_buffer;
      cairo_surface_t    *tmp_surface;

      tmp_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                width, height);
      cairo_surface_set_device_scale (tmp_surface,
                                      temp_buf_scale, temp_buf_scale);

      src_buffer  = clay_temp_buf_create_buffer (temp_buf);
      dest_buffer = clay_cairo_surface_get_buffer (tmp_surface, NULL, TRUE);

      transform =
        clay_view_renderer_get_color_transform (renderer, widget,
                                                gegl_buffer_get_format (src_buffer),
                                                gegl_buffer_get_format (dest_buffer));

      gegl_buffer_freeze_changed (dest_buffer);
      if (transform)
        {
          clay_color_transform_process_buffer (transform,
                                               src_buffer,
                                               GEGL_RECTANGLE (x - temp_buf_scaled_x,
                                                               y - temp_buf_scaled_y,
                                                               width, height),
                                               dest_buffer,
                                               GEGL_RECTANGLE (0, 0, 0, 0));
        }
      else
        {
          clay_gegl_buffer_copy (src_buffer,
                                 GEGL_RECTANGLE (x - temp_buf_scaled_x,
                                                 y - temp_buf_scaled_y,
                                                 width, height),
                                 GEGL_ABYSS_NONE,
                                 dest_buffer,
                                 GEGL_RECTANGLE (0, 0, 0, 0));
        }
      gegl_buffer_thaw_changed (dest_buffer);

      g_object_unref (src_buffer);
      g_object_unref (dest_buffer);

      cairo_surface_mark_dirty (tmp_surface);

      cairo_scale (cr, 1.0 / temp_buf_scale, 1.0 / temp_buf_scale);

      cairo_translate (cr, x, y);
      cairo_rectangle (cr, 0, 0, width, height);

      cairo_scale (cr, temp_buf_scale, temp_buf_scale);

      cairo_set_source_surface (cr, tmp_surface, 0, 0);
      cairo_fill (cr);

      cairo_surface_destroy (tmp_surface);
    }
  else
    {
      GeglColor    *fg_color;
      guint8        fg_rgb[3];
      const Babl   *fish;
      const guchar *src;
      guchar       *dest;
      gint          dest_stride;
      gint          bytes;
      gint          rowstride;
      gint          i;

      fg_color = clay_get_style_color (widget, GTK_STYLE_PROPERTY_COLOR);
      gegl_color_get_pixel (fg_color, babl_format ("R'G'B' u8"), fg_rgb);
      g_object_unref (fg_color);

      cairo_surface_flush (surface);

      bytes     = babl_format_get_bytes_per_pixel (temp_buf_format);
      rowstride = temp_buf_width * bytes;

      src = clay_temp_buf_get_data (temp_buf)    +
            ((y - temp_buf_scaled_y) * rowstride +
             (x - temp_buf_scaled_x) * bytes);

      dest        = cairo_image_surface_get_data (surface);
      dest_stride = cairo_image_surface_get_stride (surface);

      dest += y * dest_stride + x * 4;

      fish = babl_fish (temp_buf_format,
                        babl_format ("R~G~B~A u8"));

      /*  adjust ALPHA_x to point to the right component of "R~G~B~A u8"  */
      switch (clay_image_get_base_type (CLAY_IMAGE (renderer->viewable)))
        {
        case CLAY_RGB:
          break;

        case CLAY_GRAY:
          if (channel == ALPHA_G)
            channel = ALPHA;
          break;

        case CLAY_INDEXED:
          if (channel == ALPHA_I)
            channel = ALPHA;
          break;
        }

      for (i = 0; i < height; i++)
        {
          guchar        line[width * 4];
          const guchar *s = line;
          guint32      *d = (guint32*) dest;
          gint          j;

          babl_process (fish, src, line, width);

          for (j = 0; j < width; j++)
            {
              /*  invert so that higher values render lighter in light
               *  themes and darker in dark themes, matching the
               *  channel's foreground/background contrast
               */
              guint a = 255 - s[channel];

              /*  CAIRO_FORMAT_ARGB32 is premultiplied 0xAARRGGBB  */
              *d = a << 24 |
                   ((fg_rgb[0] * a + 127) / 255) << 16 |
                   ((fg_rgb[1] * a + 127) / 255) << 8 |
                   ((fg_rgb[2] * a + 127) / 255);

              s += 4;
              d += 1;
            }

          src  += rowstride;
          dest += dest_stride;
        }

      cairo_surface_mark_dirty (surface);
    }

  cairo_destroy (cr);
}

/* This function creates a background pattern from a named icon
 * if renderer->priv->bg_icon_name is set.
 */
static cairo_pattern_t *
clay_view_renderer_create_background (ClayViewRenderer *renderer,
                                      GtkWidget        *widget)
{
  cairo_pattern_t *pattern = NULL;

  if (renderer->priv->bg_icon_name)
    {
      cairo_surface_t *surface;
      GdkPixbuf       *pixbuf;

      pixbuf = clay_widget_load_icon (widget,
                                      renderer->priv->bg_icon_name,
                                      64);
      surface = clay_cairo_surface_create_from_pixbuf (pixbuf);
      g_object_unref (pixbuf);

      pattern = cairo_pattern_create_for_surface (surface);
      cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

      cairo_surface_destroy (surface);
    }

  return pattern;
}

static void
clay_view_renderer_redraw (ClayGuiConfig    *config,
                           const GParamSpec *pspec,
                           ClayViewRenderer *renderer)
{
  ClayViewRendererClass *klass = CLAY_VIEW_RENDERER_GET_CLASS (renderer);

  if (config->viewables_follow_theme)
    renderer->surface_bg = klass->follow_theme_bg;
  else
    renderer->surface_bg = klass->default_bg;

  clay_view_renderer_invalidate (renderer);
  clay_view_renderer_update (renderer);
}
