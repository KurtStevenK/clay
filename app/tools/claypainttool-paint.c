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

#include "libclaymath/claymath.h"

#include "tools-types.h"

#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/clayprojection.h"

#include "paint/claypaintcore.h"
#include "paint/claypaintoptions.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-utils.h"

#include "claypainttool.h"
#include "claypainttool-paint.h"
#include "claytools-utils.h"


#define DISPLAY_UPDATE_INTERVAL 10000 /* microseconds */


#define PAINT_FINISH            NULL


typedef struct
{
  ClayPaintTool          *paint_tool;
  ClayPaintToolPaintFunc  func;
  union
    {
      gpointer            data;
      gboolean           *finished;
    };
} PaintItem;

typedef struct
{
  GList        *drawables;
  ClayCoords    coords;
  guint32       time;
} InterpolateData;


/*  local function prototypes  */

static gboolean   clay_paint_tool_paint_use_thread  (ClayPaintTool   *paint_tool);
static gpointer   clay_paint_tool_paint_thread      (gpointer         data);

static gboolean   clay_paint_tool_paint_timeout     (ClayPaintTool   *paint_tool);

static void       clay_paint_tool_paint_interpolate (ClayPaintTool   *paint_tool,
                                                     InterpolateData *data);


/*  static variables  */

static GThread           *paint_thread;

static GMutex             paint_mutex;
static GCond              paint_cond;

static GQueue             paint_queue = G_QUEUE_INIT;
static GMutex             paint_queue_mutex;
static GCond              paint_queue_cond;

static guint              paint_timeout_id;
static volatile gboolean  paint_timeout_pending;


/*  private functions  */


static gboolean
clay_paint_tool_paint_use_thread (ClayPaintTool *paint_tool)
{
  if (! paint_tool->draw_line)
    {
      if (! paint_thread)
        {
          static gint use_paint_thread = -1;

          if (use_paint_thread < 0)
            use_paint_thread = g_getenv ("CLAY_NO_PAINT_THREAD") == NULL;

          if (use_paint_thread)
            {
              paint_thread = g_thread_new ("paint",
                                           clay_paint_tool_paint_thread, NULL);
            }
        }

      return paint_thread != NULL;
    }

  return FALSE;
}

static gpointer
clay_paint_tool_paint_thread (gpointer data)
{
  g_mutex_lock (&paint_queue_mutex);

  while (TRUE)
    {
      PaintItem *item;

      while (! (item = g_queue_pop_head (&paint_queue)))
        g_cond_wait (&paint_queue_cond, &paint_queue_mutex);

      if (item->func == PAINT_FINISH)
        {
          *item->finished = TRUE;
          g_cond_signal (&paint_queue_cond);
        }
      else
        {
          g_mutex_unlock (&paint_queue_mutex);
          g_mutex_lock (&paint_mutex);

          while (paint_timeout_pending)
            g_cond_wait (&paint_cond, &paint_mutex);

          item->func (item->paint_tool, item->data);

          g_mutex_unlock (&paint_mutex);
          g_mutex_lock (&paint_queue_mutex);
        }

      g_slice_free (PaintItem, item);
    }

  g_mutex_unlock (&paint_queue_mutex);

  return NULL;
}

static gboolean
clay_paint_tool_paint_timeout (ClayPaintTool *paint_tool)
{
  ClayPaintCore *core   = paint_tool->core;
  gboolean       update = FALSE;

  paint_timeout_pending = TRUE;

  g_mutex_lock (&paint_mutex);

  paint_tool->paint_x = core->last_paint.x;
  paint_tool->paint_y = core->last_paint.y;

  for (GList *iter = paint_tool->drawables; iter; iter = iter->next)
    {
      update |= clay_drawable_flush_paint (iter->data);
      if (update)
        break;
    }

  if (update && CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_flush)
    CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_flush (paint_tool);

  paint_timeout_pending = FALSE;
  g_cond_signal (&paint_cond);

  g_mutex_unlock (&paint_mutex);

  if (update)
    {
      ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (paint_tool);
      ClayDisplay  *display   = paint_tool->display;
      ClayImage    *image     = clay_display_get_image (display);

      if (paint_tool->snap_brush)
        clay_draw_tool_pause (draw_tool);

      clay_projection_flush_now (clay_image_get_projection (image), TRUE);
      clay_display_flush_now (display);

      if (paint_tool->snap_brush)
        clay_draw_tool_resume (draw_tool);
    }

  return G_SOURCE_CONTINUE;
}

static void
clay_paint_tool_paint_interpolate (ClayPaintTool   *paint_tool,
                                   InterpolateData *data)
{
  ClayPaintOptions *paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  ClayPaintCore    *core          = paint_tool->core;

  clay_paint_core_interpolate (core, data->drawables, paint_options,
                               &data->coords, data->time);

  /* Blink the lock box if required */
  if (core->lock_blink_state == CLAY_PAINT_LOCK_BLINK_PENDING)
    {
      GList            *iter;
      ClayLayer        *layer;

      /* Blink the lock only once per stroke */
      core->lock_blink_state = CLAY_PAINT_LOCK_BLINKED;

      for (iter = data->drawables; iter; iter = g_list_next (iter))
        {
          layer = CLAY_IS_LAYER_MASK (iter->data) ?
                    CLAY_LAYER_MASK (iter->data)->layer :
                    CLAY_LAYER (iter->data);

          if (clay_item_get_lock_position (CLAY_ITEM (layer)))
            clay_tools_blink_lock_box (CLAY_CONTEXT (paint_options)->clay,
                                       CLAY_ITEM (layer));
        }
    }

  g_list_free (data->drawables);
  g_slice_free (InterpolateData, data);
}


/*  public functions  */


gboolean
clay_paint_tool_paint_start (ClayPaintTool     *paint_tool,
                             ClayDisplay       *display,
                             const ClayCoords  *coords,
                             guint32            time,
                             gboolean           constrain,
                             GError           **error)
{
  ClayTool         *tool;
  ClayPaintOptions *paint_options;
  ClayPaintCore    *core;
  ClayDisplayShell *shell;
  ClayImage        *image;
  GList            *drawables;
  GList            *iter;
  ClayCoords        curr_coords;

  g_return_val_if_fail (CLAY_IS_PAINT_TOOL (paint_tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  g_return_val_if_fail (paint_tool->display == NULL, FALSE);

  tool          = CLAY_TOOL (paint_tool);
  paint_tool    = CLAY_PAINT_TOOL (paint_tool);
  paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  core          = paint_tool->core;
  shell         = clay_display_get_shell (display);
  image         = clay_display_get_image (display);
  drawables     = clay_image_get_selected_drawables (image);

  g_return_val_if_fail (g_list_length (drawables) == 1 ||
                        (g_list_length (drawables) > 1 && paint_tool->can_multi_paint),
                        FALSE);

  curr_coords = *coords;

  paint_tool->paint_x = curr_coords.x;
  paint_tool->paint_y = curr_coords.y;

  /*  If we use a separate paint thread, enter paint mode before starting the
   *  paint core
   */
  if (clay_paint_tool_paint_use_thread (paint_tool))
    for (iter = drawables; iter; iter = iter->next)
      clay_drawable_start_paint (iter->data);

  /*  Prepare to start the paint core  */
  if (CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_prepare)
    CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_prepare (paint_tool, display);

  /*  Start the paint core  */
  if (! clay_paint_core_start (core,
                               drawables, paint_options, &curr_coords,
                               error))
    {
      for (iter = drawables; iter; iter = iter->next)
        clay_drawable_end_paint (iter->data);
      g_list_free (drawables);

      return FALSE;
    }

  paint_tool->display  = display;
  g_list_free (paint_tool->drawables);
  paint_tool->drawables = drawables;

  if ((display != tool->display) || ! paint_tool->draw_line)
    {
      /*  If this is a new display, reset the "last stroke's endpoint"
       *  because there is none
       */
      if (display != tool->display)
        core->start_coords = core->cur_coords;

      core->last_coords = core->cur_coords;

      core->distance   = 0.0;
      core->pixel_dist = 0.0;
    }
  else if (paint_tool->draw_line)
    {
      gdouble offset_angle;
      gdouble xres, yres;

      clay_display_shell_get_constrained_line_params (shell,
                                                      &offset_angle,
                                                      &xres, &yres);

      /*  If shift is down and this is not the first paint
       *  stroke, then draw a line from the last coords to the pointer
       */
      clay_paint_core_round_line (core, paint_options,
                                  constrain, offset_angle, xres, yres);
    }

  /*  Notify subclasses  */
  if (clay_paint_tool_paint_use_thread (paint_tool) &&
      CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_start)
    {
      CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_start (paint_tool);
    }

  /*  Let the specific painting function initialize itself  */
  clay_paint_core_paint (core, drawables, paint_options,
                         CLAY_PAINT_STATE_INIT, time);

  /*  Paint to the image  */
  if (paint_tool->draw_line)
    {
      clay_paint_core_interpolate (core, drawables, paint_options,
                                   &core->cur_coords, time);
    }
  else
    {
      clay_paint_core_paint (core, drawables, paint_options,
                             CLAY_PAINT_STATE_MOTION, time);
    }

  clay_projection_flush_now (clay_image_get_projection (image), TRUE);
  clay_display_flush_now (display);

  /*  Start the display update timeout  */
  if (clay_paint_tool_paint_use_thread (paint_tool))
    {
      paint_timeout_id = g_timeout_add_full (
        G_PRIORITY_HIGH_IDLE,
        DISPLAY_UPDATE_INTERVAL / 1000,
        (GSourceFunc) clay_paint_tool_paint_timeout,
        paint_tool, NULL);
    }

  return TRUE;
}

void
clay_paint_tool_paint_end (ClayPaintTool *paint_tool,
                           guint32        time,
                           gboolean       cancel)
{
  ClayPaintOptions *paint_options;
  ClayPaintCore    *core;
  GList            *drawables;

  g_return_if_fail (CLAY_IS_PAINT_TOOL (paint_tool));
  g_return_if_fail (paint_tool->display != NULL);

  paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  core          = paint_tool->core;
  drawables     = paint_tool->drawables;

  /*  Process remaining paint items  */
  if (clay_paint_tool_paint_use_thread (paint_tool))
    {
      PaintItem *item;
      gboolean   finished = FALSE;
      guint64    end_time;

      g_return_if_fail (clay_paint_tool_paint_is_active (paint_tool));

      g_source_remove (paint_timeout_id);
      paint_timeout_id = 0;

      item = g_slice_new (PaintItem);

      item->paint_tool = paint_tool;
      item->func       = PAINT_FINISH;
      item->finished   = &finished;

      g_mutex_lock (&paint_queue_mutex);

      g_queue_push_tail (&paint_queue, item);
      g_cond_signal (&paint_queue_cond);

      end_time = g_get_monotonic_time () + DISPLAY_UPDATE_INTERVAL;

      while (! finished)
        {
          if (! g_cond_wait_until (&paint_queue_cond, &paint_queue_mutex,
                                   end_time))
            {
              g_mutex_unlock (&paint_queue_mutex);

              clay_paint_tool_paint_timeout (paint_tool);

              g_mutex_lock (&paint_queue_mutex);

              end_time = g_get_monotonic_time () + DISPLAY_UPDATE_INTERVAL;
            }
        }

      g_mutex_unlock (&paint_queue_mutex);
    }

  /*  Let the specific painting function finish up  */
  clay_paint_core_paint (core, drawables, paint_options,
                         CLAY_PAINT_STATE_FINISH, time);

  if (cancel)
    clay_paint_core_cancel (core, drawables);
  else
    clay_paint_core_finish (core, drawables, TRUE);

  /*  Notify subclasses  */
  if (clay_paint_tool_paint_use_thread (paint_tool) &&
      CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_end)
    {
      CLAY_PAINT_TOOL_GET_CLASS (paint_tool)->paint_end (paint_tool);
    }

  /*  Exit paint mode  */
  if (clay_paint_tool_paint_use_thread (paint_tool))
    for (GList *iter = drawables; iter; iter = iter->next)
      clay_drawable_end_paint (iter->data);

  paint_tool->display = NULL;
  g_clear_pointer (&paint_tool->drawables, g_list_free);
}

gboolean
clay_paint_tool_paint_is_active (ClayPaintTool *paint_tool)
{
  g_return_val_if_fail (CLAY_IS_PAINT_TOOL (paint_tool), FALSE);

  for (GList *iter = paint_tool->drawables; iter; iter = iter->next)
    {
      if (clay_drawable_is_painting (iter->data))
        return TRUE;
    }

  return FALSE;
}

void
clay_paint_tool_paint_push (ClayPaintTool          *paint_tool,
                            ClayPaintToolPaintFunc  func,
                            gpointer                data)
{
  g_return_if_fail (CLAY_IS_PAINT_TOOL (paint_tool));
  g_return_if_fail (func != NULL);

  if (clay_paint_tool_paint_use_thread (paint_tool))
    {
      PaintItem *item;

      g_return_if_fail (clay_paint_tool_paint_is_active (paint_tool));

      /*  Push an item to the queue, to be processed by the paint thread  */

      item = g_slice_new (PaintItem);

      item->paint_tool = paint_tool;
      item->func       = func;
      item->data       = data;

      g_mutex_lock (&paint_queue_mutex);

      g_queue_push_tail (&paint_queue, item);
      g_cond_signal (&paint_queue_cond);

      g_mutex_unlock (&paint_queue_mutex);
    }
  else
    {
      ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (paint_tool);
      ClayDisplay  *display   = paint_tool->display;
      ClayImage    *image     = clay_display_get_image (display);

      /*  Paint directly  */

      clay_draw_tool_pause (draw_tool);

      func (paint_tool, data);

      clay_projection_flush_now (clay_image_get_projection (image), TRUE);
      clay_display_flush_now (display);

      clay_draw_tool_resume (draw_tool);
    }
}

void
clay_paint_tool_paint_motion (ClayPaintTool    *paint_tool,
                              const ClayCoords *coords,
                              guint32           time)
{
  ClayPaintOptions *paint_options;
  ClayPaintCore    *core;
  GList            *drawables;
  InterpolateData  *data;

  g_return_if_fail (CLAY_IS_PAINT_TOOL (paint_tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (paint_tool->display != NULL);

  paint_options = CLAY_PAINT_TOOL_GET_OPTIONS (paint_tool);
  core          = paint_tool->core;
  drawables     = paint_tool->drawables;

  data = g_slice_new (InterpolateData);

  data->drawables = g_list_copy (drawables);
  data->coords    = *coords;
  data->time      = time;

  paint_tool->cursor_x = data->coords.x;
  paint_tool->cursor_y = data->coords.y;

  clay_paint_core_smooth_coords (core, paint_options, &data->coords);

  /*  Don't paint while the Shift key is pressed for line drawing  */
  if (paint_tool->draw_line)
    {
      clay_paint_core_set_current_coords (core, &data->coords);
      g_list_free (data->drawables);

      g_slice_free (InterpolateData, data);

      return;
    }

  clay_paint_tool_paint_push (paint_tool,
                              (ClayPaintToolPaintFunc) clay_paint_tool_paint_interpolate,
                              data);
}
