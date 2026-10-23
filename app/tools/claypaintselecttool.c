/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayPaintSelectTool
 * Copyright (C) 2020  Thomas Manni <thomas.manni@free.fr>
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
#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>


#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-mask.h"
#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/claychannel-select.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/clayprogress.h"
#include "core/clayscanconvert.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-scroll.h"
#include "display/claytoolgui.h"

#include "core/claytooloptions.h"
#include "claypaintselecttool.h"
#include "claypaintselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"

#include "config/clayguiconfig.h" /* playground */


static void      clay_paint_select_tool_control                     (ClayTool             *tool,
                                                                     ClayToolAction        action,
                                                                     ClayDisplay          *display);
static void      clay_paint_select_tool_button_press                (ClayTool             *tool,
                                                                     const ClayCoords     *coords,
                                                                     guint32               time,
                                                                     GdkModifierType       state,
                                                                     ClayButtonPressType   press_type,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_button_release              (ClayTool              *tool,
                                                                     const ClayCoords      *coords,
                                                                     guint32                time,
                                                                     GdkModifierType        state,
                                                                     ClayButtonReleaseType  release_type,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_motion                      (ClayTool              *tool,
                                                                     const ClayCoords      *coords,
                                                                     guint32                time,
                                                                     GdkModifierType        state,
                                                                     ClayDisplay           *display);
static gboolean  clay_paint_select_tool_key_press                   (ClayTool              *tool,
                                                                     GdkEventKey           *kevent,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_modifier_key                (ClayTool              *tool,
                                                                     GdkModifierType        key,
                                                                     gboolean               press,
                                                                     GdkModifierType        state,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_oper_update                 (ClayTool              *tool,
                                                                     const ClayCoords      *coords,
                                                                     GdkModifierType        state,
                                                                     gboolean               proximity,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_options_notify              (ClayTool              *tool,
                                                                     ClayToolOptions       *options,
                                                                     const GParamSpec      *pspec);
static void      clay_paint_select_tool_cursor_update               (ClayTool              *tool,
                                                                     const ClayCoords      *coords,
                                                                     GdkModifierType        state,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_draw                        (ClayDrawTool          *draw_tool);

static void      clay_paint_select_tool_progress                    (GeglNode              *ps_node,
                                                                     gdouble                value,
                                                                     ClayProgress          *progress);

static void      clay_paint_select_tool_halt                        (ClayPaintSelectTool   *ps_tool);

static void      clay_paint_select_tool_update_image_mask           (ClayPaintSelectTool   *ps_tool,
                                                                     GeglBuffer            *buffer,
                                                                     gint                   offset_x,
                                                                     gint                   offset_y);
static void      clay_paint_select_tool_init_buffers                (ClayPaintSelectTool   *ps_tool,
                                                                     ClayImage             *image,
                                                                     ClayDrawable          *drawable);
static gboolean  clay_paint_select_tool_can_paint                   (ClayPaintSelectTool   *ps_tool,
                                                                     ClayDisplay           *display,
                                                                     gboolean               show_message);
static gboolean  clay_paint_select_tool_start                       (ClayPaintSelectTool   *ps_tool,
                                                                     ClayDisplay           *display);
static void      clay_paint_select_tool_init_scribble               (ClayPaintSelectTool   *ps_tool);

static void      clay_paint_select_tool_create_graph                (ClayPaintSelectTool   *ps_tool);

static gboolean  clay_paint_select_tool_paint_scribble              (ClayPaintSelectTool   *ps_tool);

static void      clay_paint_select_tool_toggle_scribbles_visibility (ClayPaintSelectTool   *ps_tool);

static GeglRectangle clay_paint_select_tool_get_local_region        (ClayPaintSelectTool   *ps_tool,
                                                                     ClayDisplay           *display);

static gfloat    euclidean_distance                                 (gint                   x1,
                                                                     gint                   y1,
                                                                     gint                   x2,
                                                                     gint                   y2);


G_DEFINE_TYPE (ClayPaintSelectTool, clay_paint_select_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_paint_select_tool_parent_class


void
clay_paint_select_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data)
{
  if (gegl_has_operation ("gegl:paint-select") &&
      CLAY_GUI_CONFIG (Clay (data)->config)->playground_paint_select_tool)
    (* callback) (CLAY_TYPE_PAINT_SELECT_TOOL,
                  CLAY_TYPE_PAINT_SELECT_OPTIONS,
                  clay_paint_select_options_gui,
                  0,
                  "clay-paint-select-tool",
                  _("Paint Select"),
                  _("Paint Select Tool: Select objects by painting roughly"),
                  N_("P_aint Select"), NULL,
                  NULL, CLAY_HELP_TOOL_FOREGROUND_SELECT,
                  CLAY_ICON_TOOL_PAINT_SELECT,
                  data);
}

static void
clay_paint_select_tool_class_init (ClayPaintSelectToolClass *klass)
{
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  tool_class->button_press           = clay_paint_select_tool_button_press;
  tool_class->button_release         = clay_paint_select_tool_button_release;
  tool_class->control                = clay_paint_select_tool_control;
  tool_class->cursor_update          = clay_paint_select_tool_cursor_update;
  tool_class->key_press              = clay_paint_select_tool_key_press;
  tool_class->modifier_key           = clay_paint_select_tool_modifier_key;
  tool_class->motion                 = clay_paint_select_tool_motion;
  tool_class->oper_update            = clay_paint_select_tool_oper_update;
  tool_class->options_notify         = clay_paint_select_tool_options_notify;

  draw_tool_class->draw              = clay_paint_select_tool_draw;
}

static void
clay_paint_select_tool_init (ClayPaintSelectTool *ps_tool)
{
  ClayTool *tool = CLAY_TOOL (ps_tool);

  clay_tool_control_set_motion_mode       (tool->control, CLAY_MOTION_MODE_EXACT);
  clay_tool_control_set_scroll_lock       (tool->control, FALSE);
  clay_tool_control_set_preserve          (tool->control, FALSE);
  clay_tool_control_set_dirty_mask        (tool->control,
                                           CLAY_DIRTY_IMAGE           |
                                           CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_dirty_action      (tool->control,
                                           CLAY_TOOL_ACTION_HALT);
  clay_tool_control_set_precision         (tool->control,
                                           CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor       (tool->control,
                                           CLAY_TOOL_CURSOR_PAINTBRUSH);
  clay_tool_control_set_cursor_modifier   (tool->control,
                                           CLAY_CURSOR_MODIFIER_PLUS);
  clay_tool_control_set_action_pixel_size (tool->control,
                                           "tools-paint-select-pixel-size-set");
  /* TODO: the size-set action is not implemented. */
  clay_tool_control_set_action_size       (tool->control,
                                           "tools-paint-select-size-set");
  ps_tool->image_mask  = NULL;
  ps_tool->trimap      = NULL;
  ps_tool->drawable    = NULL;
  ps_tool->scribble    = NULL;
  ps_tool->graph       = NULL;
  ps_tool->ps_node     = NULL;
  ps_tool->render_node = NULL;
}

static void
clay_paint_select_tool_button_press (ClayTool            *tool,
                                     const ClayCoords    *coords,
                                     guint32              time,
                                     GdkModifierType      state,
                                     ClayButtonPressType  press_type,
                                     ClayDisplay         *display)
{
  ClayPaintSelectTool    *ps_tool = CLAY_PAINT_SELECT_TOOL (tool);
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  GeglColor              *grey    = gegl_color_new ("#888");

  if (tool->display && display != tool->display)
     clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! clay_paint_select_tool_start (ps_tool, display))
        return;
    }

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  ps_tool->last_pos.x = coords->x;
  ps_tool->last_pos.y = coords->y;

  /* Always reset the "scribbles" to start with a blank slate. */
  gegl_buffer_set_color (ps_tool->trimap, NULL, grey);

  if (options->operation == CLAY_CHANNEL_OP_ADD)
    {
      gegl_node_set (ps_tool->ps_node, "mode", 0, NULL);
      gegl_node_set (ps_tool->threshold_node, "value", 0.99, NULL);
    }
  else
    {
      gegl_node_set (ps_tool->ps_node, "mode", 1, NULL);
      gegl_node_set (ps_tool->threshold_node, "value", 0.01, NULL);
    }
  ps_tool->painting_op = options->operation;

  ps_tool->process = clay_paint_select_tool_paint_scribble (ps_tool);

  clay_tool_control_activate (tool->control);

  g_object_unref (grey);
}

static void
clay_paint_select_tool_button_release (ClayTool              *tool,
                                       const ClayCoords      *coords,
                                       guint32                time,
                                       GdkModifierType        state,
                                       ClayButtonReleaseType  release_type,
                                       ClayDisplay           *display)
{
  ClayPaintSelectTool *ps_tool   = CLAY_PAINT_SELECT_TOOL (tool);
  ClayDrawTool        *draw_tool = CLAY_DRAW_TOOL (tool);

  clay_draw_tool_pause (draw_tool);
  clay_tool_control_halt (tool->control);

  if (ps_tool->process)
    {
      GTimer        *timer = g_timer_new ();
      ClayProgress  *progress;
      GeglBuffer    *result;
      GeglRectangle  local_region;

      progress = clay_progress_start (CLAY_PROGRESS (tool), FALSE,
                                      "%s", "Selecting");

      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, FALSE);

      local_region = clay_paint_select_tool_get_local_region (ps_tool, display);

      if (local_region.width < ps_tool->drawable_width ||
          local_region.height < ps_tool->drawable_height)
        {
          gegl_node_set (ps_tool->ps_node,
                         "use_local_region", TRUE,
                         "region_x",         local_region.x,
                         "region_y",         local_region.y,
                         "region_width",     local_region.width,
                         "region_height",    local_region.height,
                         NULL);
        }
      else
        {
          gegl_node_set (ps_tool->ps_node,
                         "use_local_region", FALSE, NULL);
        }

      gegl_node_set (ps_tool->render_node, "buffer", &result, NULL);
      g_signal_connect (ps_tool->ps_node, "progress",
                        G_CALLBACK (clay_paint_select_tool_progress),
                        ps_tool);
      gegl_node_process (ps_tool->render_node);
      g_signal_handlers_disconnect_by_func (ps_tool->ps_node,
                                            G_CALLBACK (clay_paint_select_tool_progress),
                                            ps_tool);

      g_timer_stop (timer);
      g_printerr ("processing graph takes %.3f s\n\n", g_timer_elapsed (timer, NULL));
      g_timer_destroy (timer);

      clay_paint_select_tool_update_image_mask (ps_tool,
                                                result,
                                                ps_tool->drawable_off_x,
                                                ps_tool->drawable_off_y);
      if (progress)
        clay_progress_end (progress);
      g_object_unref (result);
    }

  clay_draw_tool_resume (draw_tool);
}

static void
clay_paint_select_tool_control (ClayTool       *tool,
                                ClayToolAction  action,
                                ClayDisplay    *display)
{
  ClayPaintSelectTool *paint_select = CLAY_PAINT_SELECT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
      break;

    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_paint_select_tool_halt (paint_select);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_paint_select_tool_cursor_update (ClayTool         *tool,
                                      const ClayCoords *coords,
                                      GdkModifierType   state,
                                      ClayDisplay      *display)
{
  ClayPaintSelectOptions *options  = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (tool);
  ClayCursorModifier      modifier = CLAY_CURSOR_MODIFIER_NONE;

  if (options->operation == CLAY_CHANNEL_OP_ADD)
    {
      modifier = CLAY_CURSOR_MODIFIER_PLUS;
    }
  else
    {
      modifier = CLAY_CURSOR_MODIFIER_MINUS;
    }

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static gboolean
clay_paint_select_tool_can_paint (ClayPaintSelectTool *ps_tool,
                                  ClayDisplay         *display,
                                  gboolean             show_message)
{
  ClayTool      *tool      = CLAY_TOOL (ps_tool);
  ClayGuiConfig *config    = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage     *image     = clay_display_get_image (display);
  GList         *drawables = clay_image_get_selected_drawables (image);
  ClayDrawable  *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            clay_tool_message_literal (tool, display,
                                       _("Cannot paint select on multiple layers. Select only one layer."));
          else
            clay_tool_message_literal (tool, display,
                                       _("No active drawables."));
        }

      g_list_free (drawables);

      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
    {
      if (show_message)
        {
          clay_tool_message_literal (tool, display,
                                     _("Cannot paint select on layer groups."));
        }

      return FALSE;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          clay_tool_message_literal (tool, display,
                                     _("The active layer is not visible."));
        }

      return FALSE;
    }

  return TRUE;
}

static gboolean
clay_paint_select_tool_start (ClayPaintSelectTool *ps_tool,
                              ClayDisplay         *display)
{
  ClayTool     *tool  = CLAY_TOOL (ps_tool);
  ClayImage    *image = clay_display_get_image (display);
  ClayDrawable *drawable;

  if (! clay_paint_select_tool_can_paint (ps_tool, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = clay_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  clay_paint_select_tool_init_buffers (ps_tool, image, drawable);
  clay_paint_select_tool_create_graph (ps_tool);

  if (! clay_draw_tool_is_active (CLAY_DRAW_TOOL (ps_tool)))
    clay_draw_tool_start (CLAY_DRAW_TOOL (ps_tool), display);

  return TRUE;
}

static gboolean
clay_paint_select_tool_key_press (ClayTool    *tool,
                                  GdkEventKey *kevent,
                                  ClayDisplay *display)
{
  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

/* Based on clay_selection_tool_modifier_key () */
static void
clay_paint_select_tool_modifier_key (ClayTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     ClayDisplay     *display)
{
  ClayPaintSelectTool    *ps_tool = CLAY_PAINT_SELECT_TOOL (tool);
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  GdkModifierType         extend_mask;
  GdkModifierType         modify_mask;

  extend_mask = clay_get_extend_selection_mask ();
  modify_mask = clay_get_modify_selection_mask ();

  if (key == extend_mask ||
      key == modify_mask ||
      key == GDK_MOD1_MASK)
    {
      ClayChannelOps button_op = options->operation;

      state &= extend_mask | modify_mask | GDK_MOD1_MASK;

      if (press)
        {
          if (key == state || ! state)
            ps_tool->saved_op = options->operation;
        }
      else
        {
          if (! state)
            button_op = ps_tool->saved_op;
        }

      if (state & GDK_MOD1_MASK)
        {
          button_op = ps_tool->saved_op;
        }
      else if (state & (extend_mask | modify_mask))
        {
          ClayChannelOps op = clay_modifiers_to_channel_op (state);

          switch (op)
            {
            case CLAY_CHANNEL_OP_ADD:
              button_op = CLAY_CHANNEL_OP_ADD;
              break;

            case CLAY_CHANNEL_OP_SUBTRACT:
              button_op = CLAY_CHANNEL_OP_SUBTRACT;
              break;

            default:
              break;
            }
        }

      if (button_op != options->operation)
        g_object_set (options, "operation", button_op, NULL);
    }
}

static void
clay_paint_select_tool_motion (ClayTool         *tool,
                               const ClayCoords *coords,
                               guint32           time,
                               GdkModifierType   state,
                               ClayDisplay      *display)
{
  ClayPaintSelectTool *ps_tool = CLAY_PAINT_SELECT_TOOL (tool);

  CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state, display);

  if (state & GDK_BUTTON1_MASK)
    {
      gfloat distance = euclidean_distance (coords->x,
                                            coords->y,
                                            ps_tool->last_pos.x,
                                            ps_tool->last_pos.y);

      if (distance >= 2.f)
        {
          clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
          ps_tool->last_pos.x = coords->x;
          ps_tool->last_pos.y = coords->y;

          ps_tool->process |= clay_paint_select_tool_paint_scribble (ps_tool);
          clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
        }
    }
}

static void
clay_paint_select_tool_oper_update (ClayTool         *tool,
                                    const ClayCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    ClayDisplay      *display)
{
  ClayPaintSelectTool *ps        = CLAY_PAINT_SELECT_TOOL (tool);
  ClayDrawTool        *draw_tool = CLAY_DRAW_TOOL (tool);

  if (proximity)
    {
      clay_draw_tool_pause (draw_tool);

      if (! tool->display || display == tool->display)
        {
          ps->last_pos.x = coords->x;
          ps->last_pos.y = coords->y;
        }

      if (! clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_start (draw_tool, display);

      clay_draw_tool_resume (draw_tool);
    }
  else if (clay_draw_tool_is_active (draw_tool))
    {
      clay_draw_tool_stop (draw_tool);
    }
}

static void
clay_paint_select_tool_draw (ClayDrawTool *draw_tool)
{
  ClayPaintSelectTool    *paint_select = CLAY_PAINT_SELECT_TOOL (draw_tool);
  ClayPaintSelectOptions *options      = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (paint_select);
  gint                    size         = options->stroke_width;

  clay_draw_tool_add_arc (draw_tool,
                          FALSE,
                          paint_select->last_pos.x - (size / 2.0),
                          paint_select->last_pos.y - (size / 2.0),
                          size, size,
                          0.0, (2.0 * G_PI));
}

static void
clay_paint_select_tool_options_notify (ClayTool         *tool,
                                       ClayToolOptions  *options,
                                       const GParamSpec *pspec)
{
  ClayPaintSelectTool *ps_tool = CLAY_PAINT_SELECT_TOOL (tool);

  if (g_strcmp0 (pspec->name, "stroke-width") == 0)
    {
      /* This triggers a redraw of the tool pointer, especially useful
       * here when we change the pen size with on-canvas interaction.
       */
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "stroke-width") && ps_tool->scribble)
    {
      g_object_unref (ps_tool->scribble);
      ps_tool->scribble = NULL;
    }
  else if (! strcmp (pspec->name, "show-scribbles"))
    {
      clay_paint_select_tool_toggle_scribbles_visibility (ps_tool);
    }
}

static void
clay_paint_select_tool_progress (GeglNode     *ps_node,
                                 gdouble       value,
                                 ClayProgress *progress)
{
  clay_progress_set_value (progress, value);
  while (g_main_context_pending (NULL))
    g_main_context_iteration (NULL, FALSE);
}

static void
clay_paint_select_tool_halt (ClayPaintSelectTool *ps_tool)
{
  ClayTool *tool = CLAY_TOOL (ps_tool);

  g_clear_object (&ps_tool->trimap);
  g_clear_object (&ps_tool->graph);
  g_clear_object (&ps_tool->scribble);

  ps_tool->drawable    = NULL;
  ps_tool->render_node = NULL;
  ps_tool->ps_node     = NULL;

  ps_tool->image_mask  = NULL;

  if (tool->display)
    {
      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
      clay_image_flush (clay_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
}

static void
clay_paint_select_tool_update_image_mask (ClayPaintSelectTool *ps_tool,
                                          GeglBuffer          *buffer,
                                          gint                 offset_x,
                                          gint                 offset_y)
{
  ClayTool *tool = CLAY_TOOL (ps_tool);

  if (tool->display)
    {
      ClayImage *image = clay_display_get_image (tool->display);

      clay_channel_select_buffer (clay_image_get_mask (image),
                                  C_("command", "Paint Select"),
                                  buffer,
                                  offset_x,
                                  offset_y,
                                  ps_tool->painting_op,
                                  FALSE,
                                  0,
                                  0);
      clay_image_flush (image);
    }
}

static void
clay_paint_select_tool_init_buffers (ClayPaintSelectTool *ps_tool,
                                     ClayImage           *image,
                                     ClayDrawable        *drawable)
{
  ClayChannel *channel;
  GeglColor   *grey = gegl_color_new ("#888");

  g_return_if_fail (ps_tool->trimap == NULL);
  g_return_if_fail (ps_tool->drawable == NULL);

  clay_item_get_offset (CLAY_ITEM (drawable),
                        &ps_tool->drawable_off_x,
                        &ps_tool->drawable_off_y);
  ps_tool->drawable_width  = clay_item_get_width (CLAY_ITEM (drawable));
  ps_tool->drawable_height = clay_item_get_height (CLAY_ITEM (drawable));

  ps_tool->drawable = clay_drawable_get_buffer (drawable);

  channel = clay_image_get_mask (image);
  ps_tool->image_mask = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));
  ps_tool->trimap = gegl_buffer_new (gegl_buffer_get_extent (ps_tool->drawable),
                                     babl_format ("Y float"));
  gegl_buffer_set_color (ps_tool->trimap, NULL, grey);

  g_object_unref (grey);
}

static void
clay_paint_select_tool_init_scribble (ClayPaintSelectTool *ps_tool)
{
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  ClayScanConvert        *scan_convert;
  ClayVector2             points[2];
  gint                    size   = options->stroke_width;
  gint                    radius = size / 2;
  GeglRectangle           square = {0, 0, size, size};

  if (ps_tool->scribble)
    g_object_unref (ps_tool->scribble);

  ps_tool->scribble = gegl_buffer_linear_new (&square, babl_format ("Y float"));

  points[0].x = points[1].x = radius;
  points[0].y = points[1].y = radius;
  points[1].x += 0.01;
  points[1].y += 0.01;

  scan_convert = clay_scan_convert_new ();
  clay_scan_convert_add_polyline (scan_convert, 2, points, FALSE);
  clay_scan_convert_stroke (scan_convert, size,
                            CLAY_JOIN_ROUND, CLAY_CAP_ROUND, 10.0,
                            0.0, NULL);
  clay_scan_convert_compose (scan_convert, ps_tool->scribble, 0, 0);
  clay_scan_convert_free (scan_convert);
}

static gboolean
clay_paint_select_tool_paint_scribble (ClayPaintSelectTool *ps_tool)
{
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  gint                    size    = options->stroke_width;
  gint                    radius  = size / 2;
  GeglRectangle           trimap_area;
  GeglRectangle           mask_area;
  GeglBufferIterator     *iter;
  gfloat                  scribble_value;
  gboolean                overlap = FALSE;

  if (! ps_tool->scribble)
    {
      clay_paint_select_tool_init_scribble (ps_tool);
    }

  /* add the scribble to the trimap buffer and check the image mask to see if
     an optimization should be triggered.
   */

  if (ps_tool->painting_op == CLAY_CHANNEL_OP_ADD)
    {
      scribble_value = 1.f;
    }
  else
    {
      scribble_value = 0.f;
    }

  iter = gegl_buffer_iterator_new (ps_tool->scribble, NULL, 0,
                                   babl_format ("Y float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 3);

  mask_area   = *gegl_buffer_get_extent (ps_tool->scribble);
  mask_area.x = ps_tool->last_pos.x - radius;
  mask_area.y = ps_tool->last_pos.y - radius;

  gegl_rectangle_copy (&trimap_area, &mask_area);

  trimap_area.x = mask_area.x - ps_tool->drawable_off_x;
  trimap_area.y = mask_area.y - ps_tool->drawable_off_y;

  gegl_buffer_iterator_add (iter, ps_tool->trimap, &trimap_area, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READWRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, ps_tool->image_mask, &mask_area, 0,
                            babl_format ("Y float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat  *scribble_pix = iter->items[0].data;
      gfloat  *trimap_pix   = iter->items[1].data;
      gfloat  *mask_pix     = iter->items[2].data;
      gint     n_pixels     = iter->length;

      while (n_pixels--)
        {
          if (*scribble_pix)
            {
              *trimap_pix = scribble_value;

              if (*mask_pix != scribble_value)
                overlap = TRUE;
            }

          scribble_pix++;
          trimap_pix++;
          mask_pix++;
        }
    }

  clay_paint_select_tool_toggle_scribbles_visibility (ps_tool);

  return overlap;
}

static void
clay_paint_select_tool_create_graph (ClayPaintSelectTool  *ps_tool)
{
  GeglNode *t;         /* trimap */
  GeglNode *m;         /* mask   */
  GeglNode *d;         /* drawable */
  GeglNode *crop;
  GeglNode *translate = NULL;

  ps_tool->graph = gegl_node_new ();

  m = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->image_mask,
                           NULL);

  crop = gegl_node_new_child (ps_tool->graph,
                              "operation", "gegl:crop",
                              "x", (gdouble) ps_tool->drawable_off_x,
                              "y", (gdouble) ps_tool->drawable_off_y,
                              "width", (gdouble) gegl_buffer_get_width (ps_tool->drawable),
                              "height", (gdouble) gegl_buffer_get_height (ps_tool->drawable),
                              NULL);

  ps_tool->threshold_node = gegl_node_new_child (ps_tool->graph,
                                                 "operation", "gegl:threshold",
                                                 NULL);

  if (ps_tool->drawable_off_x || ps_tool->drawable_off_y)
    {
      translate = gegl_node_new_child (ps_tool->graph,
                                       "operation", "gegl:translate",
                                       "x", -1.0 * ps_tool->drawable_off_x,
                                       "y", -1.0 * ps_tool->drawable_off_y,
                                       NULL);
    }

  d = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->drawable,
                           NULL);


  t = gegl_node_new_child (ps_tool->graph,
                           "operation", "gegl:buffer-source",
                           "buffer", ps_tool->trimap,
                           NULL);

  ps_tool->ps_node = gegl_node_new_child (ps_tool->graph,
                            "operation", "gegl:paint-select",
                            NULL);

  ps_tool->render_node = gegl_node_new_child (ps_tool->graph,
                                            "operation", "gegl:buffer-sink",
                                             NULL);

  gegl_node_link_many (m, ps_tool->threshold_node, crop, NULL);

  if (translate)
    gegl_node_link_many (crop, translate, ps_tool->ps_node, ps_tool->render_node, NULL);
  else
    gegl_node_link_many (crop, ps_tool->ps_node, ps_tool->render_node, NULL);

  gegl_node_connect (d, "output", ps_tool->ps_node, "aux");
  gegl_node_connect (t, "output", ps_tool->ps_node, "aux2");
}

static void
clay_paint_select_tool_toggle_scribbles_visibility (ClayPaintSelectTool *ps_tool)
{
  ClayTool               *tool    = CLAY_TOOL (ps_tool);
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (tool);

  if (options->show_scribbles)
    {
      GeglColor *black = gegl_color_new ("black");

      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   ps_tool->trimap,
                                   ps_tool->drawable_off_x,
                                   ps_tool->drawable_off_y,
                                   black,
                                   TRUE);
      g_object_unref (black);
    }
  else
    {
      clay_display_shell_set_mask (clay_display_get_shell (tool->display),
                                   NULL, 0, 0, NULL, FALSE);
    }
}

static GeglRectangle
clay_paint_select_tool_get_local_region (ClayPaintSelectTool *ps_tool,
                                         ClayDisplay         *display)
{
  ClayPaintSelectOptions *options = CLAY_PAINT_SELECT_TOOL_GET_OPTIONS (ps_tool);
  ClayDisplayShell       *shell;
  GeglRectangle           drawable_region;
  GeglRectangle           viewport;
  GeglRectangle           local_region;
  gdouble                 x, y, w, h;
  gint                    radius = ceil (options->stroke_width / 2.0);

  shell = clay_display_get_shell (display);
  clay_display_shell_scroll_get_viewport (shell, &x, &y, &w, &h);

  viewport.x             = (gint) x - radius;
  viewport.y             = (gint) y - radius;
  viewport.width         = (gint) w + options->stroke_width;
  viewport.height        = (gint) h + options->stroke_width;

  drawable_region.x      = ps_tool->drawable_off_x;
  drawable_region.y      = ps_tool->drawable_off_y;
  drawable_region.width  = ps_tool->drawable_width;
  drawable_region.height = ps_tool->drawable_height;

  gegl_rectangle_intersect (&local_region, &viewport, &drawable_region);

  local_region.x -= ps_tool->drawable_off_x;
  local_region.y -= ps_tool->drawable_off_y;

  g_printerr ("local region: (%d,%d) %d x %d\n",
              local_region.x, local_region.y,
              local_region.width, local_region.height);

  return local_region;
}

static gfloat
euclidean_distance (gint x1,
                    gint y1,
                    gint x2,
                    gint y2)
{
  return sqrtf ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}
