/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
 *
 * This program is polygon software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Polygon Software Foundation; either version 3 of the License, or
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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/claychannel.h"
#include "core/claychannel-select.h"
#include "core/clayimage.h"
#include "core/claylayer-floating-selection.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolpolygon.h"

#include "claypolygonselecttool.h"
#include "clayselectionoptions.h"
#include "claytoolcontrol.h"


struct _ClayPolygonSelectToolPrivate
{
  ClayToolWidget *widget;
  ClayToolWidget *grab_widget;

  gboolean        pending_response;
  gint            pending_response_id;
};


/*  local function prototypes  */

static void       clay_polygon_select_tool_finalize                (GObject               *object);

static void       clay_polygon_select_tool_control                 (ClayTool              *tool,
                                                                    ClayToolAction         action,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_button_press            (ClayTool              *tool,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    ClayButtonPressType    press_type,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_button_release          (ClayTool              *tool,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    ClayButtonReleaseType  release_type,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_motion                  (ClayTool              *tool,
                                                                    const ClayCoords      *coords,
                                                                    guint32                time,
                                                                    GdkModifierType        state,
                                                                    ClayDisplay           *display);
static gboolean   clay_polygon_select_tool_key_press               (ClayTool              *tool,
                                                                    GdkEventKey           *kevent,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_modifier_key            (ClayTool              *tool,
                                                                    GdkModifierType        key,
                                                                    gboolean               press,
                                                                    GdkModifierType        state,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_oper_update             (ClayTool              *tool,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    gboolean               proximity,
                                                                    ClayDisplay           *display);
static void       clay_polygon_select_tool_cursor_update           (ClayTool              *tool,
                                                                    const ClayCoords      *coords,
                                                                    GdkModifierType        state,
                                                                    ClayDisplay           *display);

static void       clay_polygon_select_tool_real_confirm            (ClayPolygonSelectTool *poly_sel,
                                                                    ClayDisplay           *display);

static void       clay_polygon_select_tool_polygon_change_complete (ClayToolWidget        *polygon,
                                                                    ClayPolygonSelectTool *poly_sel);
static void       clay_polygon_select_tool_polygon_response        (ClayToolWidget        *polygon,
                                                                    gint                   response_id,
                                                                    ClayPolygonSelectTool *poly_sel);

static void       clay_polygon_select_tool_start                   (ClayPolygonSelectTool *poly_sel,
                                                                    ClayDisplay           *display);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPolygonSelectTool, clay_polygon_select_tool,
                            CLAY_TYPE_SELECTION_TOOL)

#define parent_class clay_polygon_select_tool_parent_class


/*  private functions  */

static void
clay_polygon_select_tool_class_init (ClayPolygonSelectToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->finalize     = clay_polygon_select_tool_finalize;

  tool_class->control        = clay_polygon_select_tool_control;
  tool_class->button_press   = clay_polygon_select_tool_button_press;
  tool_class->button_release = clay_polygon_select_tool_button_release;
  tool_class->motion         = clay_polygon_select_tool_motion;
  tool_class->key_press      = clay_polygon_select_tool_key_press;
  tool_class->modifier_key   = clay_polygon_select_tool_modifier_key;
  tool_class->oper_update    = clay_polygon_select_tool_oper_update;
  tool_class->cursor_update  = clay_polygon_select_tool_cursor_update;

  klass->change_complete     = NULL;
  klass->confirm             = clay_polygon_select_tool_real_confirm;
}

static void
clay_polygon_select_tool_init (ClayPolygonSelectTool *poly_sel)
{
  ClayTool          *tool     = CLAY_TOOL (poly_sel);
  ClaySelectionTool *sel_tool = CLAY_SELECTION_TOOL (tool);

  poly_sel->priv = clay_polygon_select_tool_get_instance_private (poly_sel);

  clay_tool_control_set_motion_mode        (tool->control,
                                            CLAY_MOTION_MODE_EXACT);
  clay_tool_control_set_wants_click        (tool->control, TRUE);
  clay_tool_control_set_wants_double_click (tool->control, TRUE);
  clay_tool_control_set_active_modifiers   (tool->control,
                                            CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_SUBPIXEL);

  sel_tool->allow_move = FALSE;
}

static void
clay_polygon_select_tool_finalize (GObject *object)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (object);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  g_clear_object (&priv->widget);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_polygon_select_tool_control (ClayTool       *tool,
                                  ClayToolAction  action,
                                  ClayDisplay    *display)
{
  ClayPolygonSelectTool *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_polygon_select_tool_halt (poly_sel);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_polygon_select_tool_button_press (ClayTool            *tool,
                                       const ClayCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       ClayButtonPressType  press_type,
                                       ClayDisplay         *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (tool->display && tool->display != display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);

  if (! priv->widget) /* not tool->display, we have a subclass */
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (clay_selection_tool_start_edit (CLAY_SELECTION_TOOL (poly_sel),
                                          display, coords))
        {
          return;
        }

      clay_polygon_select_tool_start (poly_sel, display);

      clay_tool_widget_hover (priv->widget, coords, state, TRUE);
    }

  if (clay_tool_widget_button_press (priv->widget, coords, time, state,
                                     press_type))
    {
      priv->grab_widget = priv->widget;
    }

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    clay_tool_control_activate (tool->control);
}

static void
clay_polygon_select_tool_button_release (ClayTool              *tool,
                                         const ClayCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         ClayButtonReleaseType  release_type,
                                         ClayDisplay           *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;
  ClayImage                    *image    = clay_display_get_image (display);

  clay_tool_control_halt (tool->control);

  switch (release_type)
    {
    case CLAY_BUTTON_RELEASE_CLICK:
    case CLAY_BUTTON_RELEASE_NO_MOTION:
      /*  If there is a floating selection, anchor it  */
      if (clay_image_get_floating_selection (image))
        {
          floating_sel_anchor (clay_image_get_floating_selection (image));

          clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

          return;
        }

      /* fallthru */

    default:
      if (priv->grab_widget)
        {
          clay_tool_widget_button_release (priv->grab_widget,
                                           coords, time, state, release_type);
          priv->grab_widget = NULL;
        }
    }

  if (priv->pending_response)
    {
      clay_polygon_select_tool_polygon_response (priv->widget,
                                                 priv->pending_response_id,
                                                 poly_sel);

      priv->pending_response = FALSE;
    }
}

static void
clay_polygon_select_tool_motion (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 ClayDisplay      *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->grab_widget)
    {
      clay_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
clay_polygon_select_tool_key_press (ClayTool    *tool,
                                    GdkEventKey *kevent,
                                    ClayDisplay *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      return clay_tool_widget_key_press (priv->widget, kevent);
    }

  return FALSE;
}

static void
clay_polygon_select_tool_modifier_key (ClayTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       ClayDisplay     *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      clay_tool_widget_hover_modifier (priv->widget, key, press, state);

      /* let ClaySelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(clay_get_extend_selection_mask () |
                     clay_get_modify_selection_mask ());
        }
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
clay_polygon_select_tool_oper_update (ClayTool         *tool,
                                      const ClayCoords *coords,
                                      GdkModifierType   state,
                                      gboolean          proximity,
                                      ClayDisplay      *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;

  if (priv->widget && display == tool->display)
    {
      clay_tool_widget_hover (priv->widget, coords, state, proximity);

      /* let ClaySelectTool handle alt+<mod> */
      if (! (state & GDK_MOD1_MASK))
        {
          /* otherwise, shift/ctrl are handled by the widget */
          state &= ~(clay_get_extend_selection_mask () |
                     clay_get_modify_selection_mask ());
        }
    }

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
clay_polygon_select_tool_cursor_update (ClayTool         *tool,
                                        const ClayCoords *coords,
                                        GdkModifierType   state,
                                        ClayDisplay      *display)
{
  ClayPolygonSelectTool        *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayPolygonSelectToolPrivate *priv     = poly_sel->priv;
  ClayCursorModifier            modifier = CLAY_CURSOR_MODIFIER_NONE;

  if (tool->display)
    {
      if (priv->widget && display == tool->display)
        {
          clay_tool_widget_get_cursor (priv->widget, coords, state,
                                       NULL, NULL, &modifier);

          /* let ClaySelectTool handle alt+<mod> */
          if (! (state & GDK_MOD1_MASK))
            {
              /* otherwise, shift/ctrl are handled by the widget */
              state &= ~(clay_get_extend_selection_mask () |
                         clay_get_modify_selection_mask ());
            }
        }

      clay_tool_set_cursor (tool, display,
                            clay_tool_control_get_cursor (tool->control),
                            clay_tool_control_get_tool_cursor (tool->control),
                            modifier);
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                 display);
}

static void
clay_polygon_select_tool_real_confirm (ClayPolygonSelectTool *poly_sel,
                                       ClayDisplay           *display)
{
  clay_tool_control (CLAY_TOOL (poly_sel), CLAY_TOOL_ACTION_COMMIT, display);
}

static void
clay_polygon_select_tool_polygon_change_complete (ClayToolWidget        *polygon,
                                                  ClayPolygonSelectTool *poly_sel)
{
  if (CLAY_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete)
    {
      CLAY_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->change_complete (
        poly_sel, CLAY_TOOL (poly_sel)->display);
    }
}

static void
clay_polygon_select_tool_polygon_response (ClayToolWidget        *polygon,
                                           gint                   response_id,
                                           ClayPolygonSelectTool *poly_sel)
{
  ClayTool                     *tool = CLAY_TOOL (poly_sel);
  ClayPolygonSelectToolPrivate *priv = poly_sel->priv;

  /* if we're in the middle of a click, defer the response to the
   * button_release() event
   */
  if (clay_tool_control_is_active (tool->control))
    {
      priv->pending_response    = TRUE;
      priv->pending_response_id = response_id;

      return;
    }

  switch (response_id)
    {
    case CLAY_TOOL_WIDGET_RESPONSE_CONFIRM:
      /*  don't clay_tool_control(COMMIT) here because we don't always
       *  want to HALT the tool after committing because we have a
       *  subclass, we do that in the default implementation of
       *  confirm().
       */
      if (CLAY_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm)
        {
          CLAY_POLYGON_SELECT_TOOL_GET_CLASS (poly_sel)->confirm (
            poly_sel, tool->display);
        }
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_CANCEL:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_polygon_select_tool_start (ClayPolygonSelectTool *poly_sel,
                                ClayDisplay           *display)
{
  ClayTool                     *tool  = CLAY_TOOL (poly_sel);
  ClayPolygonSelectToolPrivate *priv  = poly_sel->priv;
  ClayDisplayShell             *shell = clay_display_get_shell (display);

  tool->display = display;

  priv->widget = clay_tool_polygon_new (shell);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), priv->widget);

  g_signal_connect (priv->widget, "change-complete",
                    G_CALLBACK (clay_polygon_select_tool_polygon_change_complete),
                    poly_sel);
  g_signal_connect (priv->widget, "response",
                    G_CALLBACK (clay_polygon_select_tool_polygon_response),
                    poly_sel);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}


/*  public functions  */

gboolean
clay_polygon_select_tool_is_closed (ClayPolygonSelectTool *poly_sel)
{
  ClayPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (CLAY_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  if (priv->widget)
    return clay_tool_polygon_is_closed (CLAY_TOOL_POLYGON (priv->widget));

  return FALSE;
}

void
clay_polygon_select_tool_get_points (ClayPolygonSelectTool  *poly_sel,
                                     const ClayVector2     **points,
                                     gint                   *n_points)
{
  ClayPolygonSelectToolPrivate *priv;

  g_return_if_fail (CLAY_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  if (priv->widget)
    {
      clay_tool_polygon_get_points (CLAY_TOOL_POLYGON (priv->widget),
                                    points, n_points);
    }
  else
    {
      if (points)   *points   = NULL;
      if (n_points) *n_points = 0;
    }
}


/*  protected functions  */

gboolean
clay_polygon_select_tool_is_grabbed (ClayPolygonSelectTool *poly_sel)
{
  ClayPolygonSelectToolPrivate *priv;

  g_return_val_if_fail (CLAY_IS_POLYGON_SELECT_TOOL (poly_sel), FALSE);

  priv = poly_sel->priv;

  return priv->grab_widget != NULL;
}

void
clay_polygon_select_tool_halt (ClayPolygonSelectTool *poly_sel)
{
  ClayPolygonSelectToolPrivate *priv;

  g_return_if_fail (CLAY_IS_POLYGON_SELECT_TOOL (poly_sel));

  priv = poly_sel->priv;

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (poly_sel), NULL);
  g_clear_object (&priv->widget);
}
