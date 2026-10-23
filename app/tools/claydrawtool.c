/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "core/clayimage.h"
#include "core/claypickable.h"

#include "display/claycanvas.h"
#include "display/claycanvasarc.h"
#include "display/claycanvasboundary.h"
#include "display/claycanvasgroup.h"
#include "display/claycanvasguide.h"
#include "display/claycanvashandle.h"
#include "display/claycanvasitem-utils.h"
#include "display/claycanvasline.h"
#include "display/claycanvaspen.h"
#include "display/claycanvaspolygon.h"
#include "display/claycanvasrectangle.h"
#include "display/claycanvassamplepoint.h"
#include "display/claycanvastextcursor.h"
#include "display/claycanvastransformpreview.h"
#include "display/claycanvastext.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-items.h"
#include "display/claydisplayshell-transform.h"
#include "display/claytoolwidget.h"

#include "claydrawtool.h"
#include "claytoolcontrol.h"


#define USE_TIMEOUT
#define DRAW_FPS              120
#define DRAW_TIMEOUT          (1000 /* milliseconds */ / (2 * DRAW_FPS))
#define MINIMUM_DRAW_INTERVAL (G_TIME_SPAN_SECOND / DRAW_FPS)


static void            clay_draw_tool_dispose       (GObject           *object);

static gboolean        clay_draw_tool_has_display   (ClayTool          *tool,
                                                     ClayDisplay       *display);
static ClayDisplay   * clay_draw_tool_has_image     (ClayTool          *tool,
                                                     ClayImage         *image);
static void            clay_draw_tool_control       (ClayTool          *tool,
                                                     ClayToolAction     action,
                                                     ClayDisplay       *display);
static gboolean        clay_draw_tool_key_press     (ClayTool          *tool,
                                                     GdkEventKey       *kevent,
                                                     ClayDisplay       *display);
static gboolean        clay_draw_tool_key_release   (ClayTool          *tool,
                                                     GdkEventKey       *kevent,
                                                     ClayDisplay       *display);
static void            clay_draw_tool_modifier_key  (ClayTool          *tool,
                                                     GdkModifierType    key,
                                                     gboolean           press,
                                                     GdkModifierType    state,
                                                     ClayDisplay       *display);
static void            clay_draw_tool_active_modifier_key
                                                    (ClayTool          *tool,
                                                     GdkModifierType    key,
                                                     gboolean           press,
                                                     GdkModifierType    state,
                                                     ClayDisplay       *display);
static void            clay_draw_tool_oper_update   (ClayTool          *tool,
                                                     const ClayCoords  *coords,
                                                     GdkModifierType    state,
                                                     gboolean           proximity,
                                                     ClayDisplay       *display);
static void            clay_draw_tool_cursor_update (ClayTool          *tool,
                                                     const ClayCoords  *coords,
                                                     GdkModifierType    state,
                                                     ClayDisplay       *display);
static ClayUIManager * clay_draw_tool_get_popup     (ClayTool          *tool,
                                                     const ClayCoords  *coords,
                                                     GdkModifierType    state,
                                                     ClayDisplay       *display,
                                                     const gchar      **ui_path);

static void            clay_draw_tool_widget_status (ClayToolWidget    *widget,
                                                     const gchar       *status,
                                                     ClayTool          *tool);
static void            clay_draw_tool_widget_status_coords
                                                    (ClayToolWidget    *widget,
                                                     const gchar       *title,
                                                     gdouble            x,
                                                     const gchar       *separator,
                                                     gdouble            y,
                                                     const gchar       *help,
                                                     ClayTool          *tool);
static void            clay_draw_tool_widget_message
                                                    (ClayToolWidget    *widget,
                                                     const gchar       *message,
                                                     ClayTool          *tool);
static void            clay_draw_tool_widget_snap_offsets
                                                    (ClayToolWidget    *widget,
                                                     gint               offset_x,
                                                     gint               offset_y,
                                                     gint               width,
                                                     gint               height,
                                                     ClayTool          *tool);

static void            clay_draw_tool_draw          (ClayDrawTool      *draw_tool);
static void            clay_draw_tool_undraw        (ClayDrawTool      *draw_tool);
static void            clay_draw_tool_real_draw     (ClayDrawTool      *draw_tool);


G_DEFINE_TYPE (ClayDrawTool, clay_draw_tool, CLAY_TYPE_TOOL)

#define parent_class clay_draw_tool_parent_class


static void
clay_draw_tool_class_init (ClayDrawToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->dispose           = clay_draw_tool_dispose;

  tool_class->has_display         = clay_draw_tool_has_display;
  tool_class->has_image           = clay_draw_tool_has_image;
  tool_class->control             = clay_draw_tool_control;
  tool_class->key_press           = clay_draw_tool_key_press;
  tool_class->key_release         = clay_draw_tool_key_release;
  tool_class->modifier_key        = clay_draw_tool_modifier_key;
  tool_class->active_modifier_key = clay_draw_tool_active_modifier_key;
  tool_class->oper_update         = clay_draw_tool_oper_update;
  tool_class->cursor_update       = clay_draw_tool_cursor_update;
  tool_class->get_popup           = clay_draw_tool_get_popup;

  klass->draw                     = clay_draw_tool_real_draw;
}

static void
clay_draw_tool_init (ClayDrawTool *draw_tool)
{
  draw_tool->display      = NULL;
  draw_tool->paused_count = 0;
  draw_tool->preview      = NULL;
  draw_tool->item         = NULL;
}

static void
clay_draw_tool_dispose (GObject *object)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (object);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  clay_draw_tool_set_widget (draw_tool, NULL);
  clay_draw_tool_set_default_status (draw_tool, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
clay_draw_tool_has_display (ClayTool    *tool,
                            ClayDisplay *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  return (display == draw_tool->display ||
          CLAY_TOOL_CLASS (parent_class)->has_display (tool, display));
}

static ClayDisplay *
clay_draw_tool_has_image (ClayTool  *tool,
                          ClayImage *image)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);
  ClayDisplay  *display;

  display = CLAY_TOOL_CLASS (parent_class)->has_image (tool, image);

  if (! display && draw_tool->display)
    {
      if (image && clay_display_get_image (draw_tool->display) == image)
        display = draw_tool->display;

      /*  NULL image means any display  */
      if (! image)
        display = draw_tool->display;
    }

  return display;
}

static void
clay_draw_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      if (clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_stop (draw_tool);
      clay_draw_tool_set_widget (draw_tool, NULL);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gboolean
clay_draw_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return clay_tool_widget_key_press (draw_tool->widget, kevent);
    }

  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
clay_draw_tool_key_release (ClayTool    *tool,
                            GdkEventKey *kevent,
                            ClayDisplay *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      return clay_tool_widget_key_release (draw_tool->widget, kevent);
    }

  return CLAY_TOOL_CLASS (parent_class)->key_release (tool, kevent, display);
}

static void
clay_draw_tool_modifier_key (ClayTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             ClayDisplay     *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      clay_tool_widget_hover_modifier (draw_tool->widget, key, press, state);
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
clay_draw_tool_active_modifier_key (ClayTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    ClayDisplay     *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      clay_tool_widget_motion_modifier (draw_tool->widget, key, press, state);
    }

  CLAY_TOOL_CLASS (parent_class)->active_modifier_key (tool, key, press, state,
                                                       display);
}

static void
clay_draw_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      clay_tool_widget_hover (draw_tool->widget, coords, state, proximity);
    }
  else if (proximity && draw_tool->default_status)
    {
      clay_tool_replace_status (tool, display, "%s", draw_tool->default_status);
    }
  else if (! proximity)
    {
      clay_tool_pop_status (tool, display);
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
clay_draw_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      ClayCursorType     cursor;
      ClayToolCursorType tool_cursor;
      ClayCursorModifier modifier;

      cursor      = clay_tool_control_get_cursor (tool->control);
      tool_cursor = clay_tool_control_get_tool_cursor (tool->control);
      modifier    = clay_tool_control_get_cursor_modifier (tool->control);

      if (clay_tool_widget_get_cursor (draw_tool->widget, coords, state,
                                       &cursor, &tool_cursor, &modifier))
        {
          clay_tool_set_cursor (tool, display,
                                cursor, tool_cursor, modifier);
          return;
        }
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static ClayUIManager *
clay_draw_tool_get_popup (ClayTool          *tool,
                          const ClayCoords  *coords,
                          GdkModifierType    state,
                          ClayDisplay       *display,
                          const gchar      **ui_path)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (draw_tool->widget && display == draw_tool->display)
    {
      ClayUIManager *ui_manager;

      ui_manager = clay_tool_widget_get_popup (draw_tool->widget,
                                               coords, state,
                                               ui_path);

      if (ui_manager)
        return ui_manager;
    }

  return CLAY_TOOL_CLASS (parent_class)->get_popup (tool,
                                                    coords, state, display,
                                                    ui_path);
}

static void
clay_draw_tool_widget_status (ClayToolWidget *widget,
                              const gchar    *status,
                              ClayTool       *tool)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (clay_draw_tool_is_active (draw_tool))
    {
      if (status)
        clay_tool_replace_status (tool, draw_tool->display, "%s", status);
      else
        clay_tool_pop_status (tool, draw_tool->display);
    }
}

static void
clay_draw_tool_widget_status_coords (ClayToolWidget *widget,
                                     const gchar    *title,
                                     gdouble         x,
                                     const gchar    *separator,
                                     gdouble         y,
                                     const gchar    *help,
                                     ClayTool       *tool)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (clay_draw_tool_is_active (draw_tool))
    {
      clay_tool_pop_status (tool, draw_tool->display);
      clay_tool_push_status_coords (tool, draw_tool->display,
                                    clay_tool_control_get_precision (
                                      tool->control),
                                    title, x, separator, y, help);
    }
}

static void
clay_draw_tool_widget_message (ClayToolWidget *widget,
                               const gchar    *message,
                               ClayTool       *tool)
{
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (clay_draw_tool_is_active (draw_tool))
    clay_tool_message_literal (tool, draw_tool->display, message);
}

static void
clay_draw_tool_widget_snap_offsets (ClayToolWidget   *widget,
                                    gint              offset_x,
                                    gint              offset_y,
                                    gint              width,
                                    gint              height,
                                    ClayTool         *tool)
{
  clay_tool_control_set_snap_offsets (tool->control,
                                      offset_x, offset_y,
                                      width, height);
}

#ifdef USE_TIMEOUT
static gboolean
clay_draw_tool_draw_timeout (ClayDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  /* keep the timeout running if the last drawing just happened */
  if ((now - draw_tool->last_draw_time) <= MINIMUM_DRAW_INTERVAL)
    return TRUE;

  draw_tool->draw_timeout = 0;

  clay_draw_tool_draw (draw_tool);

  return FALSE;
}
#endif

static void
clay_draw_tool_draw (ClayDrawTool *draw_tool)
{
  guint64 now = g_get_monotonic_time ();

  if (draw_tool->display &&
      draw_tool->paused_count == 0 &&
      (! draw_tool->draw_timeout ||
       (now - draw_tool->last_draw_time) > MINIMUM_DRAW_INTERVAL))
    {
      ClayDisplayShell *shell = clay_display_get_shell (draw_tool->display);

      if (draw_tool->draw_timeout)
        {
          g_source_remove (draw_tool->draw_timeout);
          draw_tool->draw_timeout = 0;
        }

      clay_draw_tool_undraw (draw_tool);

      CLAY_DRAW_TOOL_GET_CLASS (draw_tool)->draw (draw_tool);

      if (draw_tool->group_stack)
        {
          g_warning ("%s: draw_tool->group_stack not empty after calling "
                     "ClayDrawTool::draw() of %s",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (draw_tool)));

          while (draw_tool->group_stack)
            clay_draw_tool_pop_group (draw_tool);
        }

      if (draw_tool->preview)
        clay_display_shell_add_preview_item (shell, draw_tool->preview);

      if (draw_tool->item)
        clay_display_shell_add_tool_item (shell, draw_tool->item);

      draw_tool->last_draw_time = g_get_monotonic_time ();

#if 0
      g_printerr ("drawing tool stuff took %f seconds\n",
                  (draw_tool->last_draw_time - now) / 1000000.0);
#endif
    }
}

static void
clay_draw_tool_undraw (ClayDrawTool *draw_tool)
{
  if (draw_tool->display)
    {
      ClayDisplayShell *shell = clay_display_get_shell (draw_tool->display);

      if (draw_tool->preview)
        {
          clay_display_shell_remove_preview_item (shell, draw_tool->preview);
          g_clear_object (&draw_tool->preview);
        }

      if (draw_tool->item)
        {
          clay_display_shell_remove_tool_item (shell, draw_tool->item);
          g_clear_object (&draw_tool->item);
        }
    }
}

static void
clay_draw_tool_real_draw (ClayDrawTool *draw_tool)
{
  if (draw_tool->widget)
    {
      ClayCanvasItem *item = clay_tool_widget_get_item (draw_tool->widget);

      clay_draw_tool_add_item (draw_tool, item);
    }
}

void
clay_draw_tool_start (ClayDrawTool *draw_tool,
                      ClayDisplay  *display)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (clay_draw_tool_is_active (draw_tool) == FALSE);

  draw_tool->display = display;

  clay_draw_tool_draw (draw_tool);
}

void
clay_draw_tool_stop (ClayDrawTool *draw_tool)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (clay_draw_tool_is_active (draw_tool) == TRUE);

  clay_draw_tool_undraw (draw_tool);

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }

  draw_tool->last_draw_time = 0;

  draw_tool->display = NULL;
}

gboolean
clay_draw_tool_is_active (ClayDrawTool *draw_tool)
{
  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), FALSE);

  return draw_tool->display != NULL;
}

void
clay_draw_tool_pause (ClayDrawTool *draw_tool)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));

  draw_tool->paused_count++;

  if (draw_tool->draw_timeout)
    {
      g_source_remove (draw_tool->draw_timeout);
      draw_tool->draw_timeout = 0;
    }
}

void
clay_draw_tool_resume (ClayDrawTool *draw_tool)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (draw_tool->paused_count > 0);

  draw_tool->paused_count--;

  if (draw_tool->paused_count == 0)
    {
#ifdef USE_TIMEOUT
      /* Don't install the timeout if the draw tool isn't active, so
       * suspend()/resume() can always be called, and have no side
       * effect on an inactive tool. See bug #687851.
       */
      if (clay_draw_tool_is_active (draw_tool) && ! draw_tool->draw_timeout)
        {
          draw_tool->draw_timeout =
            gdk_threads_add_timeout_full (G_PRIORITY_HIGH_IDLE,
                                          DRAW_TIMEOUT,
                                          (GSourceFunc) clay_draw_tool_draw_timeout,
                                          draw_tool, NULL);
        }
#endif

      /* call draw() anyway, it will do nothing if the timeout is
       * running, but will additionally check the drawing times to
       * ensure the minimum framerate
       */
      clay_draw_tool_draw (draw_tool);
    }
}

/**
 * clay_draw_tool_calc_distance:
 * @draw_tool: a #ClayDrawTool
 * @display:   a #ClayDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * If you just need to compare distances, consider to use
 * clay_draw_tool_calc_distance_square() instead.
 *
 * Returns: the distance between the given points in display coordinates
 **/
gdouble
clay_draw_tool_calc_distance (ClayDrawTool *draw_tool,
                              ClayDisplay  *display,
                              gdouble       x1,
                              gdouble       y1,
                              gdouble       x2,
                              gdouble       y2)
{
  return sqrt (clay_draw_tool_calc_distance_square (draw_tool, display,
                                                    x1, y1, x2, y2));
}

/**
 * clay_draw_tool_calc_distance_square:
 * @draw_tool: a #ClayDrawTool
 * @display:   a #ClayDisplay
 * @x1:        start point X in image coordinates
 * @y1:        start point Y in image coordinates
 * @x2:        end point X in image coordinates
 * @y2:        end point Y in image coordinates
 *
 * This function is more effective than clay_draw_tool_calc_distance()
 * as it doesn't perform a sqrt(). Use this if you just need to compare
 * distances.
 *
 * Returns: the square of the distance between the given points in
 *          display coordinates
 **/
gdouble
clay_draw_tool_calc_distance_square (ClayDrawTool *draw_tool,
                                     ClayDisplay  *display,
                                     gdouble       x1,
                                     gdouble       y1,
                                     gdouble       x2,
                                     gdouble       y2)
{
  ClayDisplayShell *shell;
  gdouble           tx1, ty1;
  gdouble           tx2, ty2;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), 0.0);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), 0.0);

  shell = clay_display_get_shell (display);

  clay_display_shell_transform_xy_f (shell, x1, y1, &tx1, &ty1);
  clay_display_shell_transform_xy_f (shell, x2, y2, &tx2, &ty2);

  return SQR (tx2 - tx1) + SQR (ty2 - ty1);
}

void
clay_draw_tool_set_widget (ClayDrawTool   *draw_tool,
                           ClayToolWidget *widget)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (widget == NULL || CLAY_IS_TOOL_WIDGET (widget));

  if (widget == draw_tool->widget)
    return;

  if (draw_tool->widget)
    {
      clay_tool_widget_set_focus (draw_tool->widget, FALSE);

      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            clay_draw_tool_widget_status,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            clay_draw_tool_widget_status_coords,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            clay_draw_tool_widget_message,
                                            draw_tool);
      g_signal_handlers_disconnect_by_func (draw_tool->widget,
                                            clay_draw_tool_widget_snap_offsets,
                                            draw_tool);

      if (clay_draw_tool_is_active (draw_tool))
        {
          ClayCanvasItem *item = clay_tool_widget_get_item (draw_tool->widget);

          clay_draw_tool_remove_item (draw_tool, item);
        }

      g_object_unref (draw_tool->widget);
    }

  draw_tool->widget = widget;

  if (draw_tool->widget)
    {
      g_object_ref (draw_tool->widget);

      if (clay_draw_tool_is_active (draw_tool))
        {
          ClayCanvasItem *item = clay_tool_widget_get_item (draw_tool->widget);

          clay_draw_tool_add_item (draw_tool, item);
        }

      g_signal_connect (draw_tool->widget, "status",
                        G_CALLBACK (clay_draw_tool_widget_status),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "status-coords",
                        G_CALLBACK (clay_draw_tool_widget_status_coords),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "message",
                        G_CALLBACK (clay_draw_tool_widget_message),
                        draw_tool);
      g_signal_connect (draw_tool->widget, "snap-offsets",
                        G_CALLBACK (clay_draw_tool_widget_snap_offsets),
                        draw_tool);

      clay_tool_widget_set_focus (draw_tool->widget, TRUE);
    }
}

void
clay_draw_tool_set_default_status (ClayDrawTool *draw_tool,
                                   const gchar  *status)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));

  if (draw_tool->default_status)
    g_free (draw_tool->default_status);

  draw_tool->default_status = g_strdup (status);
}

void
clay_draw_tool_add_preview (ClayDrawTool   *draw_tool,
                            ClayCanvasItem *item)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  if (! draw_tool->preview)
    draw_tool->preview =
      clay_canvas_group_new (clay_display_get_shell (draw_tool->display));

  clay_canvas_group_add_item (CLAY_CANVAS_GROUP (draw_tool->preview), item);
}

void
clay_draw_tool_remove_preview (ClayDrawTool   *draw_tool,
                               ClayCanvasItem *item)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->preview != NULL);

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (draw_tool->preview), item);
}

void
clay_draw_tool_add_item (ClayDrawTool   *draw_tool,
                         ClayCanvasItem *item)
{
  ClayCanvasGroup *group;

  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));

  if (! draw_tool->item)
    draw_tool->item =
      clay_canvas_group_new (clay_display_get_shell (draw_tool->display));

  group = CLAY_CANVAS_GROUP (draw_tool->item);

  if (draw_tool->group_stack)
    group = draw_tool->group_stack->data;

  clay_canvas_group_add_item (group, item);
}

void
clay_draw_tool_remove_item (ClayDrawTool   *draw_tool,
                            ClayCanvasItem *item)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_CANVAS_ITEM (item));
  g_return_if_fail (draw_tool->item != NULL);

  clay_canvas_group_remove_item (CLAY_CANVAS_GROUP (draw_tool->item), item);
}

ClayCanvasGroup *
clay_draw_tool_add_stroke_group (ClayDrawTool *draw_tool)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_group_new (clay_display_get_shell (draw_tool->display));
  clay_canvas_group_set_group_stroking (CLAY_CANVAS_GROUP (item), TRUE);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return CLAY_CANVAS_GROUP (item);
}

ClayCanvasGroup *
clay_draw_tool_add_fill_group (ClayDrawTool *draw_tool)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_group_new (clay_display_get_shell (draw_tool->display));
  clay_canvas_group_set_group_filling (CLAY_CANVAS_GROUP (item), TRUE);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return CLAY_CANVAS_GROUP (item);
}

void
clay_draw_tool_push_group (ClayDrawTool    *draw_tool,
                           ClayCanvasGroup *group)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (CLAY_IS_CANVAS_GROUP (group));

  draw_tool->group_stack = g_list_prepend (draw_tool->group_stack, group);
}

void
clay_draw_tool_pop_group (ClayDrawTool *draw_tool)
{
  g_return_if_fail (CLAY_IS_DRAW_TOOL (draw_tool));
  g_return_if_fail (draw_tool->group_stack != NULL);

  draw_tool->group_stack = g_list_remove (draw_tool->group_stack,
                                          draw_tool->group_stack->data);
}

/**
 * clay_draw_tool_add_line:
 * @draw_tool:   the #ClayDrawTool
 * @x1:          start point X in image coordinates
 * @y1:          start point Y in image coordinates
 * @x2:          end point X in image coordinates
 * @y2:          end point Y in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws a line between the resulting
 * coordinates.
 **/
ClayCanvasItem *
clay_draw_tool_add_line (ClayDrawTool *draw_tool,
                         gdouble       x1,
                         gdouble       y1,
                         gdouble       x2,
                         gdouble       y2)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_line_new (clay_display_get_shell (draw_tool->display),
                               x1, y1, x2, y2);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * clay_draw_tool_add_guide:
 * @draw_tool:   the #ClayDrawTool
 * @orientation: the orientation of the guide line
 * @position:    the position of the guide line in image coordinates
 *
 * This function draws a guide line across the canvas.
 **/
ClayCanvasItem *
clay_draw_tool_add_guide (ClayDrawTool        *draw_tool,
                          ClayOrientationType  orientation,
                          gint                 position,
                          ClayGuideStyle       style)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_guide_new (clay_display_get_shell (draw_tool->display),
                                orientation, position,
                                style);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * clay_draw_tool_add_crosshair:
 * @draw_tool:  the #ClayDrawTool
 * @position_x: the position of the vertical guide line in image coordinates
 * @position_y: the position of the horizontal guide line in image coordinates
 *
 * This function draws two crossing guide lines across the canvas.
 **/
ClayCanvasItem *
clay_draw_tool_add_crosshair (ClayDrawTool *draw_tool,
                              gint          position_x,
                              gint          position_y)
{
  ClayCanvasGroup *group;

  group = clay_draw_tool_add_stroke_group (draw_tool);

  clay_draw_tool_push_group (draw_tool, group);
  clay_draw_tool_add_guide (draw_tool,
                            CLAY_ORIENTATION_VERTICAL, position_x,
                            CLAY_GUIDE_STYLE_NONE);
  clay_draw_tool_add_guide (draw_tool,
                            CLAY_ORIENTATION_HORIZONTAL, position_y,
                            CLAY_GUIDE_STYLE_NONE);
  clay_draw_tool_pop_group (draw_tool);

  return CLAY_CANVAS_ITEM (group);
}

/**
 * clay_draw_tool_add_sample_point:
 * @draw_tool: the #ClayDrawTool
 * @x:         X position of the sample point
 * @y:         Y position of the sample point
 * @index:     Index of the sample point
 *
 * This function draws a sample point
 **/
ClayCanvasItem *
clay_draw_tool_add_sample_point (ClayDrawTool *draw_tool,
                                 gint          x,
                                 gint          y,
                                 gint          index)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_sample_point_new (clay_display_get_shell (draw_tool->display),
                                       x, y, index, TRUE);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * clay_draw_tool_add_rectangle:
 * @draw_tool:   the #ClayDrawTool
 * @filled:      whether to fill the rectangle
 * @x:           horizontal image coordinate
 * @y:           vertical image coordinate
 * @width:       width in image coordinates
 * @height:      height in image coordinates
 *
 * This function takes image space coordinates and transforms them to
 * screen window coordinates, then draws the resulting rectangle.
 **/
ClayCanvasItem *
clay_draw_tool_add_rectangle (ClayDrawTool *draw_tool,
                              gboolean      filled,
                              gdouble       x,
                              gdouble       y,
                              gdouble       width,
                              gdouble       height)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_rectangle_new (clay_display_get_shell (draw_tool->display),
                                    x, y, width, height, filled);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_arc (ClayDrawTool *draw_tool,
                        gboolean      filled,
                        gdouble       x,
                        gdouble       y,
                        gdouble       width,
                        gdouble       height,
                        gdouble       start_angle,
                        gdouble       slice_angle)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_arc_new (clay_display_get_shell (draw_tool->display),
                              x + width  / 2.0,
                              y + height / 2.0,
                              width  / 2.0,
                              height / 2.0,
                              start_angle,
                              slice_angle,
                              filled);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_handle (ClayDrawTool     *draw_tool,
                           ClayHandleType    type,
                           gdouble           x,
                           gdouble           y,
                           gint              width,
                           gint              height,
                           ClayHandleAnchor  anchor)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_handle_new (clay_display_get_shell (draw_tool->display),
                                 type, anchor, x, y, width, height);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_lines (ClayDrawTool      *draw_tool,
                          const ClayVector2 *points,
                          gint               n_points,
                          ClayMatrix3       *transform,
                          gboolean           filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = clay_canvas_polygon_new (clay_display_get_shell (draw_tool->display),
                                  points, n_points, transform, filled);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_strokes (ClayDrawTool     *draw_tool,
                            const ClayCoords *points,
                            gint              n_points,
                            ClayMatrix3      *transform,
                            gboolean          filled)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = clay_canvas_polygon_new_from_coords (clay_display_get_shell (draw_tool->display),
                                              points, n_points, transform, filled);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_pen (ClayDrawTool      *draw_tool,
                        const ClayVector2 *points,
                        gint               n_points,
                        ClayContext       *context,
                        ClayActiveColor    color,
                        gint               width)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  if (points == NULL || n_points < 2)
    return NULL;

  item = clay_canvas_pen_new (clay_display_get_shell (draw_tool->display),
                              points, n_points, context, color, width);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

/**
 * clay_draw_tool_add_boundary:
 * @draw_tool:    a #ClayDrawTool
 * @bound_segs:   the sorted brush outline
 * @n_bound_segs: the number of segments in @bound_segs
 * @matrix:       transform matrix for the boundary
 * @offset_x:     x offset
 * @offset_y:     y offset
 *
 * Draw the boundary of the brush that @draw_tool uses. The boundary
 * should be sorted with sort_boundary(), and @n_bound_segs should
 * include the sentinel segments inserted by sort_boundary() that
 * indicate the end of connected segment sequences (groups) .
 */
ClayCanvasItem *
clay_draw_tool_add_boundary (ClayDrawTool       *draw_tool,
                             const ClayBoundSeg *bound_segs,
                             gint                n_bound_segs,
                             ClayMatrix3        *transform,
                             gdouble             offset_x,
                             gdouble             offset_y)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (n_bound_segs > 0, NULL);
  g_return_val_if_fail (bound_segs != NULL, NULL);

  item = clay_canvas_boundary_new (clay_display_get_shell (draw_tool->display),
                                   bound_segs, n_bound_segs,
                                   transform,
                                   offset_x, offset_y);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_text_cursor (ClayDrawTool     *draw_tool,
                                PangoRectangle   *cursor,
                                gboolean          overwrite,
                                ClayTextDirection direction)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_text_cursor_new (clay_display_get_shell (draw_tool->display),
                                      cursor, overwrite, direction);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_transform_preview (ClayDrawTool      *draw_tool,
                                      ClayPickable      *pickable,
                                      const ClayMatrix3 *transform,
                                      gdouble            x1,
                                      gdouble            y1,
                                      gdouble            x2,
                                      gdouble            y2)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);
  g_return_val_if_fail (CLAY_IS_PICKABLE (pickable), NULL);
  g_return_val_if_fail (transform != NULL, NULL);

  item = clay_canvas_transform_preview_new (clay_display_get_shell (draw_tool->display),
                                            pickable, transform,
                                            x1, y1, x2, y2);

  clay_draw_tool_add_preview (draw_tool, item);
  g_object_unref (item);

  return item;
}

ClayCanvasItem *
clay_draw_tool_add_text (ClayDrawTool *draw_tool,
                         gdouble       x,
                         gdouble       y,
                         gdouble       font_size,
                         gchar        *text)
{
  ClayCanvasItem *item;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), NULL);

  item = clay_canvas_text_new (clay_display_get_shell (draw_tool->display),
                               x, y, font_size, text);

  clay_draw_tool_add_item (draw_tool, item);
  g_object_unref (item);

  return item;
}

gboolean
clay_draw_tool_on_handle (ClayDrawTool     *draw_tool,
                          ClayDisplay      *display,
                          gdouble           x,
                          gdouble           y,
                          ClayHandleType    type,
                          gdouble           handle_x,
                          gdouble           handle_y,
                          gint              width,
                          gint              height,
                          ClayHandleAnchor  anchor)
{
  ClayDisplayShell *shell;
  gdouble           tx, ty;
  gdouble           handle_tx, handle_ty;

  g_return_val_if_fail (CLAY_IS_DRAW_TOOL (draw_tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  shell = clay_display_get_shell (display);

  clay_display_shell_zoom_xy_f (shell,
                                x, y,
                                &tx, &ty);
  clay_display_shell_zoom_xy_f (shell,
                                handle_x, handle_y,
                                &handle_tx, &handle_ty);

  switch (type)
    {
    case CLAY_HANDLE_SQUARE:
    case CLAY_HANDLE_FILLED_SQUARE:
    case CLAY_HANDLE_CROSS:
    case CLAY_HANDLE_CROSSHAIR:
      clay_canvas_item_shift_to_north_west (anchor,
                                            handle_tx, handle_ty,
                                            width, height,
                                            &handle_tx, &handle_ty);

      return (tx == CLAMP (tx, handle_tx, handle_tx + width) &&
              ty == CLAMP (ty, handle_ty, handle_ty + height));

    case CLAY_HANDLE_CIRCLE:
    case CLAY_HANDLE_FILLED_CIRCLE:
      clay_canvas_item_shift_to_center (anchor,
                                        handle_tx, handle_ty,
                                        width, height,
                                        &handle_tx, &handle_ty);

      /* FIXME */
      if (width != height)
        width = (width + height) / 2;

      width /= 2;

      return ((SQR (handle_tx - tx) + SQR (handle_ty - ty)) < SQR (width));

    default:
      g_warning ("%s: invalid handle type %d", G_STRFUNC, type);
      break;
    }

  return FALSE;
}
