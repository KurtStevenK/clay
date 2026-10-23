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
#include "libclaymath/claymath.h"

#include "display-types.h"
#include "tools/tools-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayprogress.h"

#include "widgets/claydialogfactory.h"

#include "tools/claytool.h"
#include "tools/tool_manager.h"

#include "claydisplay.h"
#include "claydisplay-handlers.h"
#include "claydisplayshell.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-handlers.h"
#include "claydisplayshell-render.h"
#include "claydisplayshell-scroll.h"
#include "claydisplayshell-scrollbars.h"
#include "claydisplayshell-title.h"
#include "claydisplayshell-transform.h"
#include "clayimagewindow.h"

#include "clay-intl.h"


#define PAINT_AREA_CHUNK_WIDTH  32
#define PAINT_AREA_CHUNK_HEIGHT 32


enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_SHELL
};


struct _ClayDisplayImplPrivate
{
  ClayImage      *image;        /*  pointer to the associated image     */
  gint            instance;     /*  the instance # of this display as
                                 *  taken from the image at creation    */

  GeglRectangle   bounding_box;

  GtkWidget      *shell;
  cairo_region_t *update_region;
};


/*  local function prototypes  */

static void     clay_display_progress_iface_init        (ClayProgressInterface *iface);

static void     clay_display_set_property               (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);
static void     clay_display_get_property               (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);

static gboolean clay_display_impl_present               (ClayDisplay         *display);
static gboolean clay_display_impl_grab_focus            (ClayDisplay         *display);

static ClayProgress * clay_display_progress_start       (ClayProgress        *progress,
                                                         gboolean             cancellable,
                                                         const gchar         *message);
static void     clay_display_progress_end               (ClayProgress        *progress);
static gboolean clay_display_progress_is_active         (ClayProgress        *progress);
static void     clay_display_progress_set_text          (ClayProgress        *progress,
                                                         const gchar         *message);
static void     clay_display_progress_set_value         (ClayProgress        *progress,
                                                         gdouble              percentage);
static gdouble  clay_display_progress_get_value         (ClayProgress        *progress);
static void     clay_display_progress_pulse             (ClayProgress        *progress);
static GBytes * clay_display_progress_get_window_handle (ClayProgress        *progress);
static gboolean clay_display_progress_message           (ClayProgress        *progress,
                                                         Clay                *clay,
                                                         ClayMessageSeverity  severity,
                                                         const gchar         *domain,
                                                         const gchar         *message);
static void     clay_display_progress_canceled          (ClayProgress        *progress,
                                                         ClayDisplay         *display);

static void     clay_display_flush_update_region        (ClayDisplay         *display);
static void     clay_display_paint_area                 (ClayDisplay         *display,
                                                         gint                 x,
                                                         gint                 y,
                                                         gint                 w,
                                                         gint                 h);


G_DEFINE_TYPE_WITH_CODE (ClayDisplayImpl, clay_display_impl, CLAY_TYPE_DISPLAY,
                         G_ADD_PRIVATE (ClayDisplayImpl)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_display_progress_iface_init))

#define parent_class clay_display_impl_parent_class


static void
clay_display_impl_class_init (ClayDisplayImplClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  ClayDisplayClass *display_class = CLAY_DISPLAY_CLASS (klass);

  object_class->set_property = clay_display_set_property;
  object_class->get_property = clay_display_get_property;

  display_class->present     = clay_display_impl_present;
  display_class->grab_focus  = clay_display_impl_grab_focus;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_SHELL,
                                   g_param_spec_object ("shell",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DISPLAY_SHELL,
                                                        CLAY_PARAM_READABLE));
}

static void
clay_display_impl_init (ClayDisplayImpl *display)
{
  display->priv = clay_display_impl_get_instance_private (display);
}

static void
clay_display_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start             = clay_display_progress_start;
  iface->end               = clay_display_progress_end;
  iface->is_active         = clay_display_progress_is_active;
  iface->set_text          = clay_display_progress_set_text;
  iface->set_value         = clay_display_progress_set_value;
  iface->get_value         = clay_display_progress_get_value;
  iface->pulse             = clay_display_progress_pulse;
  iface->get_window_handle = clay_display_progress_get_window_handle;
  iface->message           = clay_display_progress_message;
}

static void
clay_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, display->priv->image);
      break;

    case PROP_SHELL:
      g_value_set_object (value, display->priv->shell);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_display_impl_present (ClayDisplay *display)
{
  clay_display_shell_present (clay_display_get_shell (display));

  return TRUE;
}

static gboolean
clay_display_impl_grab_focus (ClayDisplay *display)
{
  ClayDisplayImpl *display_impl = CLAY_DISPLAY_IMPL (display);

  if (display_impl->priv->shell && clay_display_get_image (display))
    {
      ClayImageWindow *image_window;

      image_window = clay_display_shell_get_window (clay_display_get_shell (display));

      clay_display_present (display);
      gtk_widget_grab_focus (clay_window_get_primary_focus_widget (CLAY_WINDOW (image_window)));

      return TRUE;
    }

  return FALSE;
}

static ClayProgress *
clay_display_progress_start (ClayProgress *progress,
                             gboolean      cancellable,
                             const gchar  *message)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return clay_progress_start (CLAY_PROGRESS (display->priv->shell),
                                cancellable,
                                "%s", message);

  return NULL;
}

static void
clay_display_progress_end (ClayProgress *progress)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    clay_progress_end (CLAY_PROGRESS (display->priv->shell));
}

static gboolean
clay_display_progress_is_active (ClayProgress *progress)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return clay_progress_is_active (CLAY_PROGRESS (display->priv->shell));

  return FALSE;
}

static void
clay_display_progress_set_text (ClayProgress *progress,
                                const gchar  *message)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    clay_progress_set_text_literal (CLAY_PROGRESS (display->priv->shell),
                                    message);
}

static void
clay_display_progress_set_value (ClayProgress *progress,
                                 gdouble       percentage)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    clay_progress_set_value (CLAY_PROGRESS (display->priv->shell),
                             percentage);
}

static gdouble
clay_display_progress_get_value (ClayProgress *progress)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return clay_progress_get_value (CLAY_PROGRESS (display->priv->shell));

  return 0.0;
}

static void
clay_display_progress_pulse (ClayProgress *progress)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    clay_progress_pulse (CLAY_PROGRESS (display->priv->shell));
}

static GBytes *
clay_display_progress_get_window_handle (ClayProgress *progress)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return clay_progress_get_window_handle (CLAY_PROGRESS (display->priv->shell));

  return NULL;
}

static gboolean
clay_display_progress_message (ClayProgress        *progress,
                               Clay                *clay,
                               ClayMessageSeverity  severity,
                               const gchar         *domain,
                               const gchar         *message)
{
  ClayDisplayImpl *display = CLAY_DISPLAY_IMPL (progress);

  if (display->priv->shell)
    return clay_progress_message (CLAY_PROGRESS (display->priv->shell), clay,
                                  severity, domain, message);

  return FALSE;
}

static void
clay_display_progress_canceled (ClayProgress *progress,
                                ClayDisplay  *display)
{
  clay_progress_cancel (CLAY_PROGRESS (display));
}


/*  public functions  */

ClayDisplay *
clay_display_new (Clay              *clay,
                  ClayImage         *image,
                  ClayUnit          *unit,
                  gdouble            scale,
                  ClayUIManager     *popup_manager,
                  ClayDialogFactory *dialog_factory,
                  GdkMonitor        *monitor)
{
  ClayDisplay            *display;
  ClayDisplayImplPrivate *private;
  ClayImageWindow        *window = NULL;
  ClayDisplayShell       *shell;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (image == NULL || CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  /*  If there isn't an interface, never create a display  */
  if (clay->no_interface)
    return NULL;

  display = g_object_new (CLAY_TYPE_DISPLAY_IMPL,
                          "clay", clay,
                          NULL);

  private = CLAY_DISPLAY_IMPL (display)->priv;

  /*  refs the image  */
  if (image)
    clay_display_set_image (display, image);

  /*  get an image window  */
  if (CLAY_GUI_CONFIG (display->config)->single_window_mode)
    {
      ClayDisplay *active_display;

      active_display = clay_context_get_display (clay_get_user_context (clay));

      if (! active_display)
        {
          active_display =
            CLAY_DISPLAY (clay_container_get_first_child (clay->displays));
        }

      if (active_display)
        {
          ClayDisplayShell *shell = clay_display_get_shell (active_display);

          window = clay_display_shell_get_window (shell);
        }
    }

  if (! window)
    {
      window = clay_image_window_new (clay,
                                      private->image,
                                      dialog_factory,
                                      monitor);
    }

  /*  create the shell for the image  */
  private->shell = clay_display_shell_new (display, unit, scale,
                                           popup_manager,
                                           monitor);

  shell = clay_display_get_shell (display);

  clay_display_update_bounding_box (display);

  clay_image_window_add_shell (window, shell);
  clay_display_shell_present (shell);

  /* make sure the docks are visible, in case all other image windows
   * are iconified, see bug #686544.
   */
  clay_dialog_factory_show_with_display (dialog_factory);

  g_signal_connect (clay_display_shell_get_statusbar (shell), "cancel",
                    G_CALLBACK (clay_display_progress_canceled),
                    display);

  /* add the display to the list */
  clay_container_add (clay->displays, CLAY_OBJECT (display));

  return display;
}

/**
 * clay_display_delete:
 * @display:
 *
 * Closes the display and removes it from the display list. You should
 * not call this function directly, use clay_display_close() instead.
 */
void
clay_display_delete (ClayDisplay *display)
{
  ClayDisplayImplPrivate *private;
  ClayTool               *active_tool;

  g_return_if_fail (CLAY_IS_DISPLAY (display));

  private = CLAY_DISPLAY_IMPL (display)->priv;

  /* remove the display from the list */
  clay_container_remove (display->clay->displays, CLAY_OBJECT (display));

  /*  unrefs the image  */
  clay_display_set_image (display, NULL);

  active_tool = tool_manager_get_active (display->clay);

  if (active_tool && active_tool->focus_display == display)
    tool_manager_focus_display_active (display->clay, NULL);

  if (private->shell)
    {
      ClayDisplayShell *shell  = clay_display_get_shell (display);
      ClayImageWindow  *window = clay_display_shell_get_window (shell);

      /*  set private->shell to NULL *before* destroying the shell.
       *  all callbacks in claydisplayshell-callbacks.c will check
       *  this pointer and do nothing if the shell is in destruction.
       */
      private->shell = NULL;

      if (window)
        {
          if (clay_image_window_get_n_shells (window) > 1)
            {
              g_object_ref (shell);

              clay_image_window_remove_shell (window, shell);
              gtk_widget_destroy (GTK_WIDGET (shell));

              g_object_unref (shell);
            }
          else
            {
              clay_image_window_destroy (window);
            }
        }
      else
        {
          g_object_unref (shell);
        }
    }

  g_object_unref (display);
}

/**
 * clay_display_close:
 * @display:
 *
 * Closes the display. If this is the last display, it will remain
 * open, but without an image.
 */
void
clay_display_close (ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  if (clay_container_get_n_children (display->clay->displays) > 1)
    {
      clay_display_delete (display);
    }
  else
    {
      clay_display_empty (display);
    }
}

/**
 * clay_display_get_action_name:
 * @display:
 *
 * Returns: The action name for the given display. The action name
 * depends on the display ID. The result must be freed with g_free().
 **/
gchar *
clay_display_get_action_name (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  return g_strdup_printf ("windows-display-%04d",
                          clay_display_get_id (display));
}

ClayImage *
clay_display_get_image (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  return CLAY_DISPLAY_IMPL (display)->priv->image;
}

void
clay_display_set_image (ClayDisplay *display,
                        ClayImage   *image)
{
  ClayDisplayImplPrivate *private;
  ClayImage              *old_image = NULL;
  ClayDisplayShell       *shell;

  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  private = CLAY_DISPLAY_IMPL (display)->priv;

  shell = clay_display_get_shell (display);

  if (private->image)
    {
      /*  stop any active tool  */
      tool_manager_control_active (display->clay, CLAY_TOOL_ACTION_HALT,
                                   display);

      clay_display_shell_disconnect (shell);

      clay_display_disconnect (display);

      g_clear_pointer (&private->update_region, cairo_region_destroy);

      clay_image_dec_display_count (private->image);

      /*  set private->image before unrefing because there may be code
       *  that listens for image removals and then iterates the
       *  display list to find a valid display.
       */
      old_image = private->image;

#if 0
      g_print ("%s: image->ref_count before unrefing: %d\n",
               G_STRFUNC, G_OBJECT (old_image)->ref_count);
#endif
    }

  private->image = image;

  if (image)
    {
#if 0
      g_print ("%s: image->ref_count before refing: %d\n",
               G_STRFUNC, G_OBJECT (image)->ref_count);
#endif

      g_object_ref (image);

      private->instance = clay_image_get_instance_count (image);
      clay_image_inc_instance_count (image);

      clay_image_inc_display_count (image);

      clay_display_connect (display);

      if (shell)
        clay_display_shell_connect (shell);
    }

  if (old_image)
    g_object_unref (old_image);

  clay_display_update_bounding_box (display);

  if (shell)
    {
      if (image)
        {
          clay_display_shell_reconnect (shell);
        }
      else
        {
          clay_display_shell_title_update (shell);
        }
    }

  if (old_image != image)
    g_object_notify (G_OBJECT (display), "image");
}

gint
clay_display_get_instance (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), 0);

  return CLAY_DISPLAY_IMPL (display)->priv->instance;
}

ClayDisplayShell *
clay_display_get_shell (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  return CLAY_DISPLAY_SHELL (CLAY_DISPLAY_IMPL (display)->priv->shell);
}

void
clay_display_empty (ClayDisplay *display)
{
  ClayDisplayImplPrivate *private;
  GList                 *iter;

  g_return_if_fail (CLAY_IS_DISPLAY (display));

  private = CLAY_DISPLAY_IMPL (display)->priv;

  g_return_if_fail (CLAY_IS_IMAGE (private->image));

  for (iter = display->clay->context_list; iter; iter = g_list_next (iter))
    {
      ClayContext *context = iter->data;

      if (clay_context_get_display (context) == display)
        clay_context_set_image (context, NULL);
    }

  clay_display_set_image (display, NULL);

  clay_display_shell_empty (clay_display_get_shell (display));
}

void
clay_display_fill (ClayDisplay *display,
                   ClayImage   *image,
                   ClayUnit    *unit,
                   gdouble      scale)
{
  ClayDisplayImplPrivate *private;

  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = CLAY_DISPLAY_IMPL (display)->priv;

  g_return_if_fail (private->image == NULL);

  clay_display_set_image (display, image);

  clay_display_shell_fill (clay_display_get_shell (display),
                           image, unit, scale);
}

void
clay_display_update_bounding_box (ClayDisplay *display)
{
  ClayDisplayImplPrivate *private;
  ClayDisplayShell       *shell;
  GeglRectangle           bounding_box = {};

  g_return_if_fail (CLAY_IS_DISPLAY (display));

  private = CLAY_DISPLAY_IMPL (display)->priv;
  shell   = clay_display_get_shell (display);

  if (shell)
    {
      bounding_box = clay_display_shell_get_bounding_box (shell);

      if (! gegl_rectangle_equal (&bounding_box, &private->bounding_box))
        {
          GeglRectangle diff_rects[4];
          gint          n_diff_rects;
          gint          i;

          n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                                  &private->bounding_box,
                                                  &bounding_box);

          for (i = 0; i < n_diff_rects; i++)
            {
              clay_display_paint_area (display,
                                       diff_rects[i].x,     diff_rects[i].y,
                                       diff_rects[i].width, diff_rects[i].height);
            }

          private->bounding_box = bounding_box;

          clay_display_shell_scroll_clamp_and_update (shell);
          clay_display_shell_scrollbars_update (shell);
        }
    }
  else
    {
      private->bounding_box = bounding_box;
    }
}

void
clay_display_update_area (ClayDisplay *display,
                          gboolean     now,
                          gint         x,
                          gint         y,
                          gint         w,
                          gint         h)
{
  ClayDisplayImplPrivate *private;

  g_return_if_fail (CLAY_IS_DISPLAY (display));

  private = CLAY_DISPLAY_IMPL (display)->priv;

  if (now)
    {
      clay_display_paint_area (display, x, y, w, h);
    }
  else
    {
      cairo_rectangle_int_t rect;
      gint                  image_width;
      gint                  image_height;

      image_width  = clay_image_get_width  (private->image);
      image_height = clay_image_get_height (private->image);

      rect.x      = CLAMP (x,     0, image_width);
      rect.y      = CLAMP (y,     0, image_height);
      rect.width  = CLAMP (x + w, 0, image_width)  - rect.x;
      rect.height = CLAMP (y + h, 0, image_height) - rect.y;

      if (private->update_region)
        cairo_region_union_rectangle (private->update_region, &rect);
      else
        private->update_region = cairo_region_create_rectangle (&rect);
    }
}

void
clay_display_flush (ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  /* FIXME: we can end up being called during shell construction if "show all"
   * is enabled by default, in which case the shell's display pointer is still
   * NULL
   */
  if (clay_display_get_shell (display))
    {
      clay_display_flush_update_region (display);

      clay_display_shell_flush (clay_display_get_shell (display));
    }
}

void
clay_display_flush_now (ClayDisplay *display)
{
  g_return_if_fail (CLAY_IS_DISPLAY (display));

  clay_display_flush_update_region (display);
}


/*  private functions  */

static void
clay_display_flush_update_region (ClayDisplay *display)
{
  ClayDisplayImplPrivate *private = CLAY_DISPLAY_IMPL (display)->priv;

  if (private->update_region)
    {
      gint n_rects = cairo_region_num_rectangles (private->update_region);
      gint i;

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;

          cairo_region_get_rectangle (private->update_region,
                                      i, &rect);

          clay_display_paint_area (display,
                                   rect.x,
                                   rect.y,
                                   rect.width,
                                   rect.height);
        }

      g_clear_pointer (&private->update_region, cairo_region_destroy);
    }
}

static void
clay_display_paint_area (ClayDisplay *display,
                         gint         x,
                         gint         y,
                         gint         w,
                         gint         h)
{
  ClayDisplayImplPrivate *private = CLAY_DISPLAY_IMPL (display)->priv;
  ClayDisplayShell       *shell   = clay_display_get_shell (display);
  GeglRectangle           rect;
  gint                    x1, y1, x2, y2;
  gdouble                 x1_f, y1_f, x2_f, y2_f;

  if (! gegl_rectangle_intersect (&rect,
                                  &private->bounding_box,
                                  GEGL_RECTANGLE (x, y, w, h)))
    {
      return;
    }

  /*  display the area  */
  clay_display_shell_transform_bounds (shell,
                                       rect.x,
                                       rect.y,
                                       rect.x + rect.width,
                                       rect.y + rect.height,
                                       &x1_f, &y1_f, &x2_f, &y2_f);

  /*  make sure to expose a superset of the transformed sub-pixel expose
   *  area, not a subset. bug #126942. --mitch
   *
   *  also accommodate for spill introduced by potential box filtering.
   *  (bug #474509). --simon
   */
  x1 = floor (x1_f - 0.5);
  y1 = floor (y1_f - 0.5);
  x2 = ceil (x2_f + 0.5);
  y2 = ceil (y2_f + 0.5);

  /*  align transformed area to a coarse grid, to simplify the
   *  invalidated area
   */
  x1 = floor ((gdouble) x1 / PAINT_AREA_CHUNK_WIDTH)  * PAINT_AREA_CHUNK_WIDTH;
  y1 = floor ((gdouble) y1 / PAINT_AREA_CHUNK_HEIGHT) * PAINT_AREA_CHUNK_HEIGHT;
  x2 = ceil  ((gdouble) x2 / PAINT_AREA_CHUNK_WIDTH)  * PAINT_AREA_CHUNK_WIDTH;
  y2 = ceil  ((gdouble) y2 / PAINT_AREA_CHUNK_HEIGHT) * PAINT_AREA_CHUNK_HEIGHT;

  clay_display_shell_expose_area (shell, x1, y1, x2 - x1, y2 - y1);

  clay_display_shell_render_invalidate_area (shell, x1, y1, x2 - x1, y2 - y1);
}
