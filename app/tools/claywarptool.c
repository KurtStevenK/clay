/* xyz.kainzmayer.ClayDesign
 *
 * claywarptool.c
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"
#include "config/clayguiconfig.h"

#include "gegl/clay-gegl-apply-operation.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"
#include "core/claysubprogress.h"
#include "core/claytoolinfo.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"

#include "claywarptool.h"
#include "claywarpoptions.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


#define STROKE_TIMER_MAX_FPS 20
#define PREVIEW_SAMPLER      GEGL_SAMPLER_NEAREST


static void            clay_warp_tool_constructed               (GObject               *object);

static void            clay_warp_tool_control                   (ClayTool              *tool,
                                                                 ClayToolAction         action,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_button_press              (ClayTool              *tool,
                                                                 const ClayCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 ClayButtonPressType    press_type,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_button_release            (ClayTool              *tool,
                                                                 const ClayCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 ClayButtonReleaseType  release_type,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_motion                    (ClayTool              *tool,
                                                                 const ClayCoords      *coords,
                                                                 guint32                time,
                                                                 GdkModifierType        state,
                                                                 ClayDisplay           *display);
static gboolean        clay_warp_tool_key_press                 (ClayTool              *tool,
                                                                 GdkEventKey           *kevent,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_oper_update               (ClayTool              *tool,
                                                                 const ClayCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 gboolean               proximity,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_cursor_update             (ClayTool              *tool,
                                                                 const ClayCoords      *coords,
                                                                 GdkModifierType        state,
                                                                 ClayDisplay           *display);
const gchar          * clay_warp_tool_can_undo                  (ClayTool              *tool,
                                                                 ClayDisplay           *display);
const gchar          * clay_warp_tool_can_redo                  (ClayTool              *tool,
                                                                 ClayDisplay           *display);
static gboolean        clay_warp_tool_undo                      (ClayTool              *tool,
                                                                 ClayDisplay           *display);
static gboolean        clay_warp_tool_redo                      (ClayTool              *tool,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_options_notify            (ClayTool              *tool,
                                                                 ClayToolOptions       *options,
                                                                 const GParamSpec      *pspec);

static void            clay_warp_tool_draw                      (ClayDrawTool          *draw_tool);

static void            clay_warp_tool_cursor_notify             (ClayDisplayConfig     *config,
                                                                 GParamSpec            *pspec,
                                                                 ClayWarpTool          *wt);

static gboolean        clay_warp_tool_can_stroke                (ClayWarpTool          *wt,
                                                                 ClayDisplay           *display,
                                                                 gboolean               show_message);

static gboolean        clay_warp_tool_start                     (ClayWarpTool          *wt,
                                                                 ClayDisplay           *display);
static void            clay_warp_tool_halt                      (ClayWarpTool          *wt);
static void            clay_warp_tool_commit                    (ClayWarpTool          *wt);

static void            clay_warp_tool_start_stroke_timer        (ClayWarpTool          *wt);
static void            clay_warp_tool_stop_stroke_timer         (ClayWarpTool          *wt);
static gboolean        clay_warp_tool_stroke_timer              (ClayWarpTool          *wt);

static void            clay_warp_tool_create_graph              (ClayWarpTool          *wt);
static void            clay_warp_tool_create_filter             (ClayWarpTool          *wt,
                                                                 ClayDrawable          *drawable);
static void            clay_warp_tool_set_sampler               (ClayWarpTool          *wt,
                                                                 gboolean               commit);
static GeglRectangle
                       clay_warp_tool_get_stroke_bounds         (GeglNode              *node);
static GeglRectangle   clay_warp_tool_get_node_bounds           (GeglNode              *node);
static void            clay_warp_tool_clear_node_bounds         (GeglNode              *node);
static GeglRectangle   clay_warp_tool_get_invalidated_by_change (ClayWarpTool          *wt,
                                                                 const GeglRectangle   *area);
static void            clay_warp_tool_update_bounds             (ClayWarpTool          *wt);
static void            clay_warp_tool_update_area               (ClayWarpTool          *wt,
                                                                 const GeglRectangle   *area,
                                                                 gboolean               synchronous);
static void            clay_warp_tool_update_stroke             (ClayWarpTool          *wt,
                                                                 GeglNode              *node);
static void            clay_warp_tool_stroke_append             (ClayWarpTool          *wt,
                                                                 gchar                  type,
                                                                 gdouble                x,
                                                                 gdouble                y);
static void            clay_warp_tool_filter_flush              (ClayDrawableFilter    *filter,
                                                                 ClayTool              *tool);
static void            clay_warp_tool_add_op                    (ClayWarpTool          *wt,
                                                                 GeglNode              *op);
static void            clay_warp_tool_remove_op                 (ClayWarpTool          *wt,
                                                                 GeglNode              *op);
static void            clay_warp_tool_free_op                   (GeglNode              *op);

static void            clay_warp_tool_animate                   (ClayWarpTool          *wt);


G_DEFINE_TYPE (ClayWarpTool, clay_warp_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_warp_tool_parent_class


void
clay_warp_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_WARP_TOOL,
                CLAY_TYPE_WARP_OPTIONS,
                clay_warp_options_gui,
                0,
                "clay-warp-tool",
                _("Warp Transform"),
                _("Warp Transform: Deform with different tools"),
                N_("_Warp Transform"), "W",
                NULL, CLAY_HELP_TOOL_WARP,
                CLAY_ICON_TOOL_WARP,
                data);
}

static void
clay_warp_tool_class_init (ClayWarpToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->constructed  = clay_warp_tool_constructed;

  tool_class->control        = clay_warp_tool_control;
  tool_class->button_press   = clay_warp_tool_button_press;
  tool_class->button_release = clay_warp_tool_button_release;
  tool_class->motion         = clay_warp_tool_motion;
  tool_class->key_press      = clay_warp_tool_key_press;
  tool_class->oper_update    = clay_warp_tool_oper_update;
  tool_class->cursor_update  = clay_warp_tool_cursor_update;
  tool_class->can_undo       = clay_warp_tool_can_undo;
  tool_class->can_redo       = clay_warp_tool_can_redo;
  tool_class->undo           = clay_warp_tool_undo;
  tool_class->redo           = clay_warp_tool_redo;
  tool_class->options_notify = clay_warp_tool_options_notify;

  draw_tool_class->draw      = clay_warp_tool_draw;
}

static void
clay_warp_tool_init (ClayWarpTool *self)
{
  ClayTool *tool = CLAY_TOOL (self);

  clay_tool_control_set_motion_mode     (tool->control, CLAY_MOTION_MODE_EXACT);
  clay_tool_control_set_scroll_lock     (tool->control, TRUE);
  clay_tool_control_set_preserve        (tool->control, FALSE);
  clay_tool_control_set_dirty_mask      (tool->control,
                                         CLAY_DIRTY_IMAGE           |
                                         CLAY_DIRTY_DRAWABLE        |
                                         CLAY_DIRTY_SELECTION       |
                                         CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_dirty_action    (tool->control,
                                         CLAY_TOOL_ACTION_COMMIT);
  clay_tool_control_set_wants_click     (tool->control, TRUE);
  clay_tool_control_set_precision       (tool->control,
                                         CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor     (tool->control,
                                         CLAY_TOOL_CURSOR_WARP);

  clay_tool_control_set_action_pixel_size (tool->control,
                                           "tools-warp-effect-pixel-size-set");
  clay_tool_control_set_action_size       (tool->control,
                                           "tools-warp-effect-size-set");
  clay_tool_control_set_action_hardness   (tool->control,
                                           "tools-warp-effect-hardness-set");

  self->show_cursor = TRUE;
  self->draw_brush  = TRUE;
  self->snap_brush  = FALSE;
}

static void
clay_warp_tool_constructed (GObject *object)
{
  ClayWarpTool      *wt   = CLAY_WARP_TOOL (object);
  ClayTool          *tool = CLAY_TOOL (object);
  ClayDisplayConfig *display_config;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  display_config = CLAY_DISPLAY_CONFIG (tool->tool_info->clay->config);

  wt->show_cursor = display_config->show_paint_tool_cursor;
  wt->draw_brush  = display_config->show_brush_outline;
  wt->snap_brush  = display_config->snap_brush_outline;

  g_signal_connect_object (display_config, "notify::show-paint-tool-cursor",
                           G_CALLBACK (clay_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::show-brush-outline",
                           G_CALLBACK (clay_warp_tool_cursor_notify),
                           wt, 0);
  g_signal_connect_object (display_config, "notify::snap-brush-outline",
                           G_CALLBACK (clay_warp_tool_cursor_notify),
                           wt, 0);
}

static void
clay_warp_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_warp_tool_halt (wt);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_warp_tool_commit (wt);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_warp_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayWarpTool    *wt      = CLAY_WARP_TOOL (tool);
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode        *new_op;
  gint             off_x, off_y;

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      if (! clay_warp_tool_start (wt, display))
        return;
    }

  if (! clay_warp_tool_can_stroke (wt, display, TRUE))
    return;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  wt->current_stroke = gegl_path_new ();

  wt->last_pos.x = coords->x;
  wt->last_pos.y = coords->y;

  wt->total_dist = 0.0;

  new_op = gegl_node_new_child (NULL,
                                "operation", "gegl:warp",
                                "behavior",  options->behavior,
                                "size",      options->effect_size,
                                "hardness",  options->effect_hardness / 100.0,
                                "strength",  options->effect_strength,
                                /* we implement spacing manually.
                                 * anything > 1 will do.
                                 */
                                "spacing",   10.0,
                                "stroke",    wt->current_stroke,
                                NULL);

  clay_warp_tool_add_op (wt, new_op);
  g_object_unref (new_op);

  clay_item_get_offset (CLAY_ITEM (tool->drawables->data), &off_x, &off_y);

  clay_warp_tool_stroke_append (wt,
                                'M', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  clay_warp_tool_start_stroke_timer (wt);

  clay_tool_control_activate (tool->control);
}

void
clay_warp_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (wt));

  clay_tool_control_halt (tool->control);

  clay_warp_tool_stop_stroke_timer (wt);

#ifdef WARP_DEBUG
  g_printerr ("%s\n", gegl_path_to_string (wt->current_stroke));
#endif

  g_clear_object (&wt->current_stroke);

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      clay_warp_tool_undo (tool, display);

      /*  the just undone stroke has no business on the redo stack  */
      clay_warp_tool_free_op (wt->redo_stack->data);
      wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);
    }
  else
    {
      if (wt->redo_stack)
        {
          /*  the redo stack becomes invalid by actually doing a stroke  */
          g_list_free_full (wt->redo_stack,
                            (GDestroyNotify) clay_warp_tool_free_op);
          wt->redo_stack = NULL;
        }

      clay_tool_push_status (tool, tool->display,
                             _("Press ENTER to commit the transform"));
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));

  /*  update the undo actions / menu items  */
  clay_image_flush (clay_display_get_image (CLAY_TOOL (wt)->display));
}

static void
clay_warp_tool_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
  ClayWarpTool    *wt             = CLAY_WARP_TOOL (tool);
  ClayWarpOptions *options        = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  ClayVector2      old_cursor_pos;
  ClayVector2      delta;
  gdouble          dist;
  gdouble          step;
  gboolean         stroke_changed = FALSE;

  if (! wt->snap_brush)
    clay_draw_tool_pause (CLAY_DRAW_TOOL (wt));

  old_cursor_pos = wt->cursor_pos;

  wt->cursor_pos.x = coords->x;
  wt->cursor_pos.y = coords->y;

  clay_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
  dist = clay_vector2_length (&delta);

  step = options->effect_size * options->stroke_spacing / 100.0;

  while (wt->total_dist + dist >= step)
    {
      gdouble diff = step - wt->total_dist;

      clay_vector2_mul (&delta, diff / dist);
      clay_vector2_add (&old_cursor_pos, &old_cursor_pos, &delta);

      clay_vector2_sub (&delta, &wt->cursor_pos, &old_cursor_pos);
      dist -= diff;

      wt->last_pos   = old_cursor_pos;
      wt->total_dist = 0.0;

      if (options->stroke_during_motion)
        {
          gint off_x, off_y;

          if (! stroke_changed)
            {
              stroke_changed = TRUE;

              clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
            }

          clay_item_get_offset (CLAY_ITEM (tool->drawables->data), &off_x, &off_y);

          clay_warp_tool_stroke_append (wt,
                                        'L', wt->last_pos.x - off_x,
                                             wt->last_pos.y - off_y);
        }
    }

  wt->total_dist += dist;

  if (stroke_changed)
    {
      clay_warp_tool_start_stroke_timer (wt);

      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }

  if (! wt->snap_brush)
    clay_draw_tool_resume (CLAY_DRAW_TOOL (wt));
}

static gboolean
clay_warp_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);
      return TRUE;

    case GDK_KEY_Escape:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      break;
    }

  return FALSE;
}

static void
clay_warp_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayWarpTool *wt        = CLAY_WARP_TOOL (tool);
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (proximity)
    {
      clay_draw_tool_pause (draw_tool);

      wt->cursor_pos.x = coords->x;
      wt->cursor_pos.y = coords->y;
      
      if (! tool->display || display == tool->display)
        wt->last_pos = wt->cursor_pos;

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
clay_warp_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayWarpTool       *wt       = CLAY_WARP_TOOL (tool);
  ClayWarpOptions    *options  = CLAY_WARP_TOOL_GET_OPTIONS (tool);
  ClayCursorModifier  modifier = CLAY_CURSOR_MODIFIER_NONE;

  if (! clay_warp_tool_can_stroke (wt, display, FALSE))
    {
      modifier = CLAY_CURSOR_MODIFIER_BAD;
    }
  else if (display == tool->display)
    {
#if 0
      /* FIXME have better cursors  */

      switch (options->behavior)
        {
        case CLAY_WARP_BEHAVIOR_MOVE:
        case CLAY_WARP_BEHAVIOR_GROW:
        case CLAY_WARP_BEHAVIOR_SHRINK:
        case CLAY_WARP_BEHAVIOR_SWIRL_CW:
        case CLAY_WARP_BEHAVIOR_SWIRL_CCW:
        case CLAY_WARP_BEHAVIOR_ERASE:
        case CLAY_WARP_BEHAVIOR_SMOOTH:
          modifier = CLAY_CURSOR_MODIFIER_MOVE;
          break;
        }
#else
      (void) options;
#endif
    }

  if (! wt->show_cursor && modifier != CLAY_CURSOR_MODIFIER_BAD)
    {
      clay_tool_set_cursor (tool, display,
                            CLAY_CURSOR_NONE,
                            CLAY_TOOL_CURSOR_NONE,
                            CLAY_CURSOR_MODIFIER_NONE);
      return;
    }

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

const gchar *
clay_warp_tool_can_undo (ClayTool    *tool,
                         ClayDisplay *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);
  GeglNode     *to_delete;
  const gchar  *type;

  if (! wt->render_node)
    return NULL;

  to_delete = gegl_node_get_producer (wt->render_node, "aux", NULL);
  type = gegl_node_get_operation (to_delete);

  if (strcmp (type, "gegl:warp"))
    return NULL;

  return _("Warp Tool Stroke");
}

const gchar *
clay_warp_tool_can_redo (ClayTool    *tool,
                         ClayDisplay *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);

  if (! wt->render_node || ! wt->redo_stack)
    return NULL;

  return _("Warp Tool Stroke");
}

static gboolean
clay_warp_tool_undo (ClayTool    *tool,
                     ClayDisplay *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);
  GeglNode     *to_delete;
  GeglNode     *prev_node;

  to_delete = gegl_node_get_producer (wt->render_node, "aux", NULL);

  wt->redo_stack = g_list_prepend (wt->redo_stack, to_delete);

  /* we connect render_node to the previous node, but keep the current node
   * in the graph, connected to the previous node as well, so that it doesn't
   * get invalidated and maintains its cache.  this way, redoing it doesn't
   * require reprocessing.
   */
  prev_node = gegl_node_get_producer (to_delete, "input", NULL);

  gegl_node_connect (prev_node,       "output",
                     wt->render_node, "aux");

  clay_warp_tool_update_bounds (wt);
  clay_warp_tool_update_stroke (wt, to_delete);

  return TRUE;
}

static gboolean
clay_warp_tool_redo (ClayTool    *tool,
                     ClayDisplay *display)
{
  ClayWarpTool *wt = CLAY_WARP_TOOL (tool);
  GeglNode     *to_add;

  to_add = wt->redo_stack->data;

  gegl_node_connect (to_add,          "output",
                     wt->render_node, "aux");

  wt->redo_stack = g_list_remove_link (wt->redo_stack, wt->redo_stack);

  clay_warp_tool_update_bounds (wt);
  clay_warp_tool_update_stroke (wt, to_add);

  return TRUE;
}

static void
clay_warp_tool_options_notify (ClayTool         *tool,
                               ClayToolOptions  *options,
                               const GParamSpec *pspec)
{
  ClayWarpTool    *wt         = CLAY_WARP_TOOL (tool);
  ClayWarpOptions *wt_options = CLAY_WARP_OPTIONS (options);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "effect-size"))
    {
      clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));
      clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
    }
  else if (! strcmp (pspec->name, "interpolation"))
    {
      clay_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
  else if (! strcmp (pspec->name, "abyss-policy"))
    {
      if (wt->render_node)
        {
          gegl_node_set (wt->render_node,
                         "abyss-policy", wt_options->abyss_policy,
                         NULL);

          clay_warp_tool_update_stroke (wt, NULL);
        }
    }
  else if (! strcmp (pspec->name, "high-quality-preview"))
    {
      clay_warp_tool_set_sampler (wt, /* commit = */ FALSE);
    }
}

static void
clay_warp_tool_draw (ClayDrawTool *draw_tool)
{
  ClayWarpTool    *wt      = CLAY_WARP_TOOL (draw_tool);
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  gdouble          x, y;

  if (wt->snap_brush)
    {
      x = wt->last_pos.x;
      y = wt->last_pos.y;
    }
  else
    {
      x = wt->cursor_pos.x;
      y = wt->cursor_pos.y;
    }

  if (wt->draw_brush)
    {
      clay_draw_tool_add_arc (draw_tool,
                              FALSE,
                              x - options->effect_size * 0.5,
                              y - options->effect_size * 0.5,
                              options->effect_size,
                              options->effect_size,
                              0.0, 2.0 * G_PI);
    }
  else if (! wt->show_cursor)
    {
      /*  don't leave the user without any indication and draw
       *  a fallback crosshair
       */
      clay_draw_tool_add_handle (draw_tool,
                                 CLAY_HANDLE_CROSSHAIR,
                                 x, y,
                                 CLAY_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 CLAY_TOOL_HANDLE_SIZE_CROSSHAIR,
                                 CLAY_HANDLE_ANCHOR_CENTER);
    }
}

static void
clay_warp_tool_cursor_notify (ClayDisplayConfig *config,
                              GParamSpec        *pspec,
                              ClayWarpTool      *wt)
{
  clay_draw_tool_pause (CLAY_DRAW_TOOL (wt));

  wt->show_cursor = config->show_paint_tool_cursor;
  wt->draw_brush  = config->show_brush_outline;
  wt->snap_brush  = config->snap_brush_outline;

  clay_draw_tool_resume (CLAY_DRAW_TOOL (wt));
}

static gboolean
clay_warp_tool_can_stroke (ClayWarpTool *wt,
                           ClayDisplay  *display,
                           gboolean      show_message)
{
  ClayTool        *tool        = CLAY_TOOL (wt);
  ClayWarpOptions *options     = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  ClayGuiConfig   *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage       *image       = clay_display_get_image (display);
  ClayItem        *locked_item = NULL;
  GList           *drawables   = clay_image_get_selected_drawables (image);
  ClayDrawable    *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (show_message)
        {
          if (g_list_length (drawables) > 1)
            clay_tool_message_literal (tool, display,
                                       _("Cannot warp multiple layers. Select only one layer."));
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
                                     _("Cannot warp layer groups."));
        }

      return FALSE;
    }

  if (clay_item_is_content_locked (CLAY_ITEM (drawable), &locked_item))
    {
      if (show_message)
        {
          clay_tool_message_literal (tool, display,
                                     _("The selected item's pixels are locked."));

          clay_tools_blink_lock_box (display->clay, locked_item);
        }

      return FALSE;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      if (show_message)
        {
          clay_tool_message_literal (tool, display,
                                     _("The selected item is not visible."));
        }

      return FALSE;
    }

  if (! options->stroke_during_motion &&
      ! options->stroke_periodically)
    {
      if (show_message)
        {
          clay_tool_message_literal (tool, display,
                                     _("No stroke events selected."));

          clay_tools_show_tool_options (display->clay);
          clay_widget_blink (options->stroke_frame);
        }

      return FALSE;
    }

  if (! wt->filter || ! clay_tool_can_undo (tool, display))
    {
      const gchar *message = NULL;

      switch (options->behavior)
        {
        case CLAY_WARP_BEHAVIOR_MOVE:
        case CLAY_WARP_BEHAVIOR_GROW:
        case CLAY_WARP_BEHAVIOR_SHRINK:
        case CLAY_WARP_BEHAVIOR_SWIRL_CW:
        case CLAY_WARP_BEHAVIOR_SWIRL_CCW:
          break;

        case CLAY_WARP_BEHAVIOR_ERASE:
          message = _("No warp to erase.");
          break;

        case CLAY_WARP_BEHAVIOR_SMOOTH:
          message = _("No warp to smooth.");
          break;
        }

      if (message)
        {
          if (show_message)
            {
              clay_tool_message_literal (tool, display, message);

              clay_tools_show_tool_options (display->clay);
              clay_widget_blink (options->behavior_combo);
            }

          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
clay_warp_tool_start (ClayWarpTool *wt,
                      ClayDisplay  *display)
{
  ClayTool        *tool     = CLAY_TOOL (wt);
  ClayWarpOptions *options  = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  ClayImage       *image    = clay_display_get_image (display);
  ClayDrawable    *drawable;
  const Babl      *format;
  GeglRectangle    bbox;

  if (! clay_warp_tool_can_stroke (wt, display, TRUE))
    return FALSE;

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = clay_image_get_selected_drawables (image);

  drawable = tool->drawables->data;

  /* Create the coords buffer, with the size of the selection */
  format = babl_format_n (babl_type ("float"), 2);

  clay_item_mask_intersect (CLAY_ITEM (drawable), &bbox.x, &bbox.y,
                            &bbox.width, &bbox.height);

#ifdef WARP_DEBUG
  g_printerr ("Initialize coordinate buffer (%d,%d) at %d,%d\n",
              bbox.width, bbox.height, bbox.x, bbox.y);
#endif

  wt->coords_buffer = gegl_buffer_new (&bbox, format);

  clay_warp_tool_create_filter (wt, drawable);

  if (! clay_draw_tool_is_active (CLAY_DRAW_TOOL (wt)))
    clay_draw_tool_start (CLAY_DRAW_TOOL (wt), display);

  if (options->animate_button)
    {
      g_signal_connect_swapped (options->animate_button, "clicked",
                                G_CALLBACK (clay_warp_tool_animate),
                                wt);

      gtk_widget_set_sensitive (options->animate_button, TRUE);
    }

  return TRUE;
}

static void
clay_warp_tool_halt (ClayWarpTool *wt)
{
  ClayTool        *tool    = CLAY_TOOL (wt);
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);

  g_clear_object (&wt->coords_buffer);

  g_clear_object (&wt->graph);
  wt->render_node = NULL;

  if (wt->filter)
    {
      clay_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);

      clay_image_flush (clay_display_get_image (tool->display));
    }

  if (wt->redo_stack)
    {
      g_list_free (wt->redo_stack);
      wt->redo_stack = NULL;
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (wt)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (wt));

  if (options->animate_button)
    {
      gtk_widget_set_sensitive (options->animate_button, FALSE);

      g_signal_handlers_disconnect_by_func (options->animate_button,
                                            clay_warp_tool_animate,
                                            wt);
    }
}

static void
clay_warp_tool_commit (ClayWarpTool *wt)
{
  ClayTool *tool = CLAY_TOOL (wt);

  /* don't commit a nop */
  if (tool->display && clay_tool_can_undo (tool, tool->display))
    {
      clay_tool_control_push_preserve (tool->control, TRUE);

      clay_warp_tool_set_sampler (wt, /* commit = */ TRUE);

      clay_drawable_filter_commit (wt->filter, FALSE,
                                   CLAY_PROGRESS (tool), FALSE);
      g_clear_object (&wt->filter);

      clay_tool_control_pop_preserve (tool->control);

      clay_image_flush (clay_display_get_image (tool->display));
    }
}

static void
clay_warp_tool_start_stroke_timer (ClayWarpTool *wt)
{
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);

  clay_warp_tool_stop_stroke_timer (wt);

  if (options->stroke_periodically                    &&
      options->stroke_periodically_rate > 0.0         &&
      ! (options->behavior == CLAY_WARP_BEHAVIOR_MOVE &&
         options->stroke_during_motion))
    {
      gdouble fps;

      fps = STROKE_TIMER_MAX_FPS * options->stroke_periodically_rate / 100.0;

      wt->stroke_timer = g_timeout_add (1000.0 / fps,
                                        (GSourceFunc) clay_warp_tool_stroke_timer,
                                        wt);
    }
}

static void
clay_warp_tool_stop_stroke_timer (ClayWarpTool *wt)
{
  if (wt->stroke_timer)
    g_source_remove (wt->stroke_timer);

  wt->stroke_timer = 0;
}

static gboolean
clay_warp_tool_stroke_timer (ClayWarpTool *wt)
{
  ClayTool *tool = CLAY_TOOL (wt);
  gint      off_x, off_y;

  g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  clay_item_get_offset (CLAY_ITEM (tool->drawables->data), &off_x, &off_y);

  clay_warp_tool_stroke_append (wt,
                                'L', wt->last_pos.x - off_x,
                                     wt->last_pos.y - off_y);

  return TRUE;
}

static void
clay_warp_tool_create_graph (ClayWarpTool *wt)
{
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  GeglNode        *graph;           /* Wrapper to be returned */
  GeglNode        *input, *output;  /* Proxy nodes */
  GeglNode        *coords, *render; /* Render nodes */

  /* render_node is not supposed to be recreated */
  g_return_if_fail (wt->graph == NULL);

  graph = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (graph, "input");
  output = gegl_node_get_output_proxy (graph, "output");

  coords = gegl_node_new_child (graph,
                               "operation", "gegl:buffer-source",
                               "buffer",    wt->coords_buffer,
                               NULL);

  render = gegl_node_new_child (graph,
                                "operation",    "gegl:map-relative",
                                "abyss-policy", options->abyss_policy,
                                NULL);

  gegl_node_link_many (input, render, output, NULL);
  gegl_node_connect (coords, "output",
                     render, "aux");


  wt->graph       = graph;
  wt->render_node = render;
}

static void
clay_warp_tool_create_filter (ClayWarpTool *wt,
                              ClayDrawable *drawable)
{
  if (! wt->graph)
    clay_warp_tool_create_graph (wt);

  clay_warp_tool_set_sampler (wt, /* commit = */ FALSE);

  wt->filter = clay_drawable_filter_new (drawable,
                                         _("Warp transform"),
                                         wt->graph,
                                         CLAY_ICON_TOOL_WARP);
  clay_drawable_filter_set_temporary (wt->filter, TRUE);

  clay_drawable_filter_set_region (wt->filter, CLAY_FILTER_REGION_DRAWABLE);

#if 0
  g_object_set (wt->filter, "gegl-caching", TRUE, NULL);
#endif

  g_signal_connect (wt->filter, "flush",
                    G_CALLBACK (clay_warp_tool_filter_flush),
                    wt);
}

static void
clay_warp_tool_set_sampler (ClayWarpTool *wt,
                            gboolean      commit)
{
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  GeglSamplerType  sampler;
  GeglSamplerType  old_sampler;

  if (! wt->render_node)
    return;

  if (commit || options->high_quality_preview)
    sampler = (GeglSamplerType) options->interpolation;
  else
    sampler = PREVIEW_SAMPLER;

  gegl_node_get (wt->render_node,
                 "sampler-type", &old_sampler,
                 NULL);

  if (sampler != old_sampler)
    {
      gegl_node_set (wt->render_node,
                     "sampler-type", sampler,
                     NULL);

      clay_warp_tool_update_bounds (wt);
      clay_warp_tool_update_stroke (wt, NULL);
    }
}

static GeglRectangle
clay_warp_tool_get_stroke_bounds (GeglNode *node)
{
  GeglRectangle  bbox = {0, 0, 0, 0};
  GeglPath      *stroke;
  gdouble        size;

  gegl_node_get (node,
                 "stroke", &stroke,
                 "size",   &size,
                 NULL);

  if (stroke)
    {
      gdouble        min_x;
      gdouble        max_x;
      gdouble        min_y;
      gdouble        max_y;

      gegl_path_get_bounds (stroke, &min_x, &max_x, &min_y, &max_y);
      g_object_unref (stroke);

      bbox.x      = floor (min_x - size * 0.5);
      bbox.y      = floor (min_y - size * 0.5);
      bbox.width  = ceil (max_x + size * 0.5) - bbox.x;
      bbox.height = ceil (max_y + size * 0.5) - bbox.y;
    }

  return bbox;
}

static GeglRectangle
clay_warp_tool_get_node_bounds (GeglNode *node)
{
  GeglRectangle *bounds;

  if (! node || strcmp (gegl_node_get_operation (node), "gegl:warp"))
    return *GEGL_RECTANGLE (0, 0, 0, 0);

  bounds = g_object_get_data (G_OBJECT (node), "clay-warp-tool-bounds");

  if (! bounds)
    {
      GeglNode      *input_node;
      GeglRectangle  input_bounds;
      GeglRectangle  stroke_bounds;

      input_node   = gegl_node_get_producer (node, "input", NULL);
      input_bounds = clay_warp_tool_get_node_bounds (input_node);

      stroke_bounds = clay_warp_tool_get_stroke_bounds (node);

      gegl_rectangle_bounding_box (&input_bounds,
                                   &input_bounds, &stroke_bounds);

      bounds = gegl_rectangle_dup (&input_bounds);

      g_object_set_data_full (G_OBJECT (node), "clay-warp-tool-bounds",
                              bounds, g_free);
    }

  return *bounds;
}

static void
clay_warp_tool_clear_node_bounds (GeglNode *node)
{
  if (node && ! strcmp (gegl_node_get_operation (node), "gegl:warp"))
    g_object_set_data (G_OBJECT (node), "clay-warp-tool-bounds", NULL);
}

static GeglRectangle
clay_warp_tool_get_invalidated_by_change (ClayWarpTool        *wt,
                                          const GeglRectangle *area)
{
  GeglRectangle result = *area;

  if (! wt->filter)
    return result;

  if (wt->render_node)
    {
      GeglOperation *operation = gegl_node_get_gegl_operation (wt->render_node);

      result = gegl_operation_get_invalidated_by_change (operation,
                                                         "aux", area);
    }

  return result;
}

static void
clay_warp_tool_update_bounds (ClayWarpTool *wt)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (wt->render_node)
    {
      GeglNode *node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = clay_warp_tool_get_node_bounds (node);

      bounds = clay_warp_tool_get_invalidated_by_change (wt, &bounds);
    }

  clay_drawable_filter_set_crop (wt->filter, &bounds, FALSE);
}

static void
clay_warp_tool_update_area (ClayWarpTool        *wt,
                            const GeglRectangle *area,
                            gboolean             synchronous)
{
  GeglRectangle  rect;
  ClayContainer *filters;

  if (! wt->filter)
    return;

  rect = clay_warp_tool_get_invalidated_by_change (wt, area);

  /* Move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  filters =
    clay_drawable_get_filters (clay_drawable_filter_get_drawable (wt->filter));

  if (clay_container_have (filters, CLAY_OBJECT (wt->filter)))
  {
    gint end_index = clay_container_get_n_children (filters) - 1;
    gint index     = clay_container_get_child_index (filters,
                                                     CLAY_OBJECT (wt->filter));

    if (end_index > 0 && index != end_index)
      clay_container_reorder (filters, CLAY_OBJECT (wt->filter),
                              end_index);
  }

  if (synchronous)
    {
      ClayTool  *tool  = CLAY_TOOL (wt);
      ClayImage *image = clay_display_get_image (tool->display);

      g_signal_handlers_block_by_func (wt->filter,
                                       clay_warp_tool_filter_flush,
                                       wt);

      clay_drawable_filter_apply (wt->filter, &rect);

      clay_projection_flush_now (clay_image_get_projection (image), TRUE);
      clay_display_flush_now (tool->display);

      g_signal_handlers_unblock_by_func (wt->filter,
                                         clay_warp_tool_filter_flush,
                                         wt);
    }
  else
    {
      clay_drawable_filter_apply (wt->filter, &rect);
    }
}

static void
clay_warp_tool_update_stroke (ClayWarpTool *wt,
                              GeglNode     *node)
{
  GeglRectangle bounds = {0, 0, 0, 0};

  if (! wt->filter)
    return;

  if (node)
    {
      /* update just this stroke */
      bounds = clay_warp_tool_get_stroke_bounds (node);
    }
  else if (wt->render_node)
    {
      node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      bounds = clay_warp_tool_get_node_bounds (node);
    }

  if (! gegl_rectangle_is_empty (&bounds))
    {
#ifdef WARP_DEBUG
  g_printerr ("update stroke: (%d,%d), %dx%d\n",
              bounds.x, bounds.y,
              bounds.width, bounds.height);
#endif

      clay_warp_tool_update_area (wt, &bounds, FALSE);
    }
}

static void
clay_warp_tool_stroke_append (ClayWarpTool *wt,
                              gchar         type,
                              gdouble       x,
                              gdouble       y)
{
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  GeglRectangle    area;

  if (! wt->filter)
    return;

  gegl_path_append (wt->current_stroke, type, x, y);

  area.x      = floor (x - options->effect_size * 0.5);
  area.y      = floor (y - options->effect_size * 0.5);
  area.width  = ceil  (x + options->effect_size * 0.5) - area.x;
  area.height = ceil  (y + options->effect_size * 0.5) - area.y;

#ifdef WARP_DEBUG
  g_printerr ("update rect: (%d,%d), %dx%d\n",
              area.x, area.y,
              area.width, area.height);
#endif

  if (wt->render_node)
    {
      GeglNode *node = gegl_node_get_producer (wt->render_node, "aux", NULL);

      clay_warp_tool_clear_node_bounds (node);

      clay_warp_tool_update_bounds (wt);
    }

  clay_warp_tool_update_area (wt, &area, options->real_time_preview);
}

static void
clay_warp_tool_filter_flush (ClayDrawableFilter *filter,
                             ClayTool           *tool)
{
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

static void
clay_warp_tool_add_op (ClayWarpTool *wt,
                       GeglNode     *op)
{
  GeglNode *last_op;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  gegl_node_add_child (wt->graph, op);

  last_op = gegl_node_get_producer (wt->render_node, "aux", NULL);

  gegl_node_disconnect (wt->render_node, "aux");
  gegl_node_link (last_op, op);
  gegl_node_connect (op,              "output",
                     wt->render_node, "aux");
}

static void
clay_warp_tool_remove_op (ClayWarpTool *wt,
                          GeglNode     *op)
{
  GeglNode *previous;

  g_return_if_fail (GEGL_IS_NODE (wt->render_node));

  previous = gegl_node_get_producer (op, "input", NULL);

  gegl_node_disconnect (op,              "input");
  gegl_node_connect (previous,        "output",
                     wt->render_node, "aux");

  gegl_node_remove_child (wt->graph, op);
}

static void
clay_warp_tool_free_op (GeglNode *op)
{
  GeglNode *parent;

  parent = gegl_node_get_parent (op);

  clay_assert (parent != NULL);

  gegl_node_remove_child (parent, op);
}

static void
clay_warp_tool_animate (ClayWarpTool *wt)
{
  ClayTool        *tool    = CLAY_TOOL (wt);
  ClayWarpOptions *options = CLAY_WARP_TOOL_GET_OPTIONS (wt);
  ClayImage       *orig_image;
  ClayImage       *image;
  ClayLayer       *layer;
  ClayLayer       *first_layer;
  GeglNode        *scale_node;
  ClayProgress    *progress;
  GtkWidget       *widget;
  gint             i;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! clay_warp_tool_can_undo (tool, tool->display))
    {
      clay_tool_message_literal (tool, tool->display,
                                 _("Please add some warp strokes first."));
      return;
    }

  /*  get rid of the image map so we can use wt->graph  */
  if (wt->filter)
    {
      clay_drawable_filter_abort (wt->filter);
      g_clear_object (&wt->filter);
    }

  clay_warp_tool_set_sampler (wt, /* commit = */ TRUE);

  clay_progress_start (CLAY_PROGRESS (tool), FALSE,
                       _("Rendering Frame %d"), 1);

  orig_image = clay_item_get_image (CLAY_ITEM (tool->drawables->data));

  image = clay_create_image (orig_image->clay,
                             clay_item_get_width  (CLAY_ITEM (tool->drawables->data)),
                             clay_item_get_height (CLAY_ITEM (tool->drawables->data)),
                             clay_drawable_get_base_type (tool->drawables->data),
                             clay_drawable_get_precision (tool->drawables->data),
                             TRUE);

  /*  the first frame is always the unwarped image  */
  layer = CLAY_LAYER (clay_item_convert (CLAY_ITEM (tool->drawables->data), image,
                                         CLAY_TYPE_LAYER));
  clay_object_take_name (CLAY_OBJECT (layer),
                         g_strdup_printf (_("Frame %d"), 1));

  clay_item_set_offset (CLAY_ITEM (layer), 0, 0);
  clay_item_set_visible (CLAY_ITEM (layer), TRUE, FALSE);
  clay_layer_set_mode (layer, clay_image_get_default_new_layer_mode (image),
                       FALSE);
  clay_layer_set_opacity (layer, CLAY_OPACITY_OPAQUE, FALSE);
  clay_image_add_layer (image, layer, NULL, 0, FALSE);

  first_layer = layer;

  scale_node = gegl_node_new_child (NULL,
                                    "operation",    "clay:scalar-multiply",
                                    "n-components", 2,
                                    NULL);
  clay_warp_tool_add_op (wt, scale_node);

  progress = clay_sub_progress_new (CLAY_PROGRESS (tool));

  for (i = 1; i < options->n_animation_frames; i++)
    {
      clay_progress_set_text (CLAY_PROGRESS (tool),
                              _("Rendering Frame %d"), i + 1);

      clay_sub_progress_set_step (CLAY_SUB_PROGRESS (progress),
                                  i, options->n_animation_frames);

      layer = CLAY_LAYER (clay_item_duplicate (CLAY_ITEM (first_layer),
                                               CLAY_TYPE_LAYER));
      clay_object_take_name (CLAY_OBJECT (layer),
                             g_strdup_printf (_("Frame %d"), i + 1));

      gegl_node_set (scale_node,
                     "factor", (gdouble) i /
                               (gdouble) (options->n_animation_frames - 1),
                     NULL);

      clay_gegl_apply_operation (clay_drawable_get_buffer (CLAY_DRAWABLE (first_layer)),
                                 progress,
                                 _("Frame"),
                                 wt->graph,
                                 clay_drawable_get_buffer (CLAY_DRAWABLE (layer)),
                                 NULL, FALSE);

      clay_image_add_layer (image, layer, NULL, 0, FALSE);
    }

  g_object_unref (progress);

  clay_warp_tool_remove_op (wt, scale_node);

  clay_progress_end (CLAY_PROGRESS (tool));

  /*  recreate the image map  */
  clay_warp_tool_create_filter (wt, tool->drawables->data);
  clay_warp_tool_update_stroke (wt, NULL);

  widget = GTK_WIDGET (clay_display_get_shell (tool->display));
  clay_create_display (orig_image->clay, image, clay_unit_pixel (), 1.0,
                       G_OBJECT (clay_widget_get_monitor (widget)));
  g_object_unref (image);
}
