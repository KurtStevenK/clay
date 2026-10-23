/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvement to support polygonal segments
 * Copyright (C) 2008 Martin Nordholts
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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/claychannel.h"
#include "core/claychannel-select.h"
#include "core/clayimage.h"
#include "core/claylayer-floating-selection.h"

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"
#include "display/claytoolpolygon.h"

#include "clayfreeselecttool.h"
#include "clayselectionoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


struct _ClayFreeSelectToolPrivate
{
  gboolean        started;
  gboolean        changed;

  /* The selection operation active when the tool was started */
  ClayChannelOps  operation_at_start;
};


static void       clay_free_select_tool_control         (ClayTool              *tool,
                                                         ClayToolAction         action,
                                                         ClayDisplay           *display);
static void       clay_free_select_tool_button_press    (ClayTool              *tool,
                                                         const ClayCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         ClayButtonPressType    press_type,
                                                         ClayDisplay           *display);
static void       clay_free_select_tool_button_release  (ClayTool              *tool,
                                                         const ClayCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         ClayButtonReleaseType  release_type,
                                                         ClayDisplay           *display);
static void       clay_free_select_tool_options_notify  (ClayTool              *tool,
                                                         ClayToolOptions       *options,
                                                         const GParamSpec      *pspec);

static gboolean   clay_free_select_tool_have_selection  (ClaySelectionTool     *sel_tool,
                                                         ClayDisplay           *display);

static void       clay_free_select_tool_change_complete (ClayPolygonSelectTool *poly_sel,
                                                         ClayDisplay           *display);

static void       clay_free_select_tool_commit          (ClayFreeSelectTool    *free_sel,
                                                         ClayDisplay           *display);
static void       clay_free_select_tool_halt            (ClayFreeSelectTool    *free_sel);

static gboolean   clay_free_select_tool_select          (ClayFreeSelectTool    *free_sel,
                                                         ClayDisplay           *display);


G_DEFINE_TYPE_WITH_PRIVATE (ClayFreeSelectTool, clay_free_select_tool,
                            CLAY_TYPE_POLYGON_SELECT_TOOL)

#define parent_class clay_free_select_tool_parent_class


void
clay_free_select_tool_register (ClayToolRegisterCallback  callback,
                                gpointer                  data)
{
  (* callback) (CLAY_TYPE_FREE_SELECT_TOOL,
                CLAY_TYPE_SELECTION_OPTIONS,
                clay_selection_options_gui,
                0,
                "clay-free-select-tool",
                _("Free Select"),
                _("Free Select Tool: Select a hand-drawn region with free "
                  "and polygonal segments"),
                N_("_Free Select"), "F",
                NULL, CLAY_HELP_TOOL_FREE_SELECT,
                CLAY_ICON_TOOL_FREE_SELECT,
                data);
}

static void
clay_free_select_tool_class_init (ClayFreeSelectToolClass *klass)
{
  ClayToolClass              *tool_class     = CLAY_TOOL_CLASS (klass);
  ClaySelectionToolClass     *sel_class      = CLAY_SELECTION_TOOL_CLASS (klass);
  ClayPolygonSelectToolClass *poly_sel_class = CLAY_POLYGON_SELECT_TOOL_CLASS (klass);

  tool_class->control             = clay_free_select_tool_control;
  tool_class->button_press        = clay_free_select_tool_button_press;
  tool_class->button_release      = clay_free_select_tool_button_release;
  tool_class->options_notify      = clay_free_select_tool_options_notify;

  sel_class->have_selection       = clay_free_select_tool_have_selection;

  poly_sel_class->change_complete = clay_free_select_tool_change_complete;
}

static void
clay_free_select_tool_init (ClayFreeSelectTool *free_sel)
{
  ClayTool          *tool     = CLAY_TOOL (free_sel);
  ClaySelectionTool *sel_tool = CLAY_SELECTION_TOOL (tool);

  free_sel->priv = clay_free_select_tool_get_instance_private (free_sel);

  clay_tool_control_set_preserve     (tool->control, FALSE);
  clay_tool_control_set_dirty_mask   (tool->control,
                                      CLAY_DIRTY_SELECTION);
  clay_tool_control_set_dirty_action (tool->control,
                                      CLAY_TOOL_ACTION_COMMIT);
  clay_tool_control_set_tool_cursor  (tool->control,
                                      CLAY_TOOL_CURSOR_FREE_SELECT);

  sel_tool->allow_move = TRUE;
}

static void
clay_free_select_tool_control (ClayTool       *tool,
                               ClayToolAction  action,
                               ClayDisplay    *display)
{
  ClayFreeSelectTool *free_sel = CLAY_FREE_SELECT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_free_select_tool_halt (free_sel);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_free_select_tool_commit (free_sel, display);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_free_select_tool_button_press (ClayTool            *tool,
                                    const ClayCoords    *coords,
                                    guint32              time,
                                    GdkModifierType      state,
                                    ClayButtonPressType  press_type,
                                    ClayDisplay         *display)
{
  ClaySelectionOptions      *options  = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayFreeSelectTool        *free_sel = CLAY_FREE_SELECT_TOOL (tool);
  ClayPolygonSelectTool     *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayFreeSelectToolPrivate *priv     = free_sel->priv;

  if (press_type == CLAY_BUTTON_PRESS_NORMAL &&
      clay_selection_tool_start_edit (CLAY_SELECTION_TOOL (poly_sel),
                                      display, coords))
    return;

  CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  if (press_type == CLAY_BUTTON_PRESS_NORMAL &&
      clay_polygon_select_tool_is_grabbed (poly_sel))
    {
      if (! priv->started)
        {
          priv->started            = TRUE;
          priv->operation_at_start = options->operation;
        }

      clay_selection_tool_start_change (
        CLAY_SELECTION_TOOL (tool),
        ! clay_polygon_select_tool_is_closed (poly_sel),
        priv->operation_at_start);

      priv->changed = FALSE;
    }
}

static void
clay_free_select_tool_button_release (ClayTool              *tool,
                                      const ClayCoords      *coords,
                                      guint32                time,
                                      GdkModifierType        state,
                                      ClayButtonReleaseType  release_type,
                                      ClayDisplay           *display)
{
  ClayFreeSelectTool        *free_sel = CLAY_FREE_SELECT_TOOL (tool);
  ClayPolygonSelectTool     *poly_sel = CLAY_POLYGON_SELECT_TOOL (tool);
  ClayFreeSelectToolPrivate *priv     = free_sel->priv;

  if (clay_polygon_select_tool_is_grabbed (poly_sel))
    {
      clay_selection_tool_end_change (CLAY_SELECTION_TOOL (tool),
                                      ! priv->changed);

      priv->changed = FALSE;
    }

  CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                  release_type, display);
}

static void
clay_free_select_tool_options_notify (ClayTool         *tool,
                                      ClayToolOptions  *options,
                                      const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "antialias") ||
      ! strcmp (pspec->name, "feather")   ||
      ! strcmp (pspec->name, "feather-radius"))
    {
      if (tool->display)
        {
          clay_free_select_tool_change_complete (
            CLAY_POLYGON_SELECT_TOOL (tool), tool->display);
        }
    }

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);
}

static gboolean
clay_free_select_tool_have_selection (ClaySelectionTool *sel_tool,
                                      ClayDisplay       *display)
{
  ClayPolygonSelectTool *poly_sel = CLAY_POLYGON_SELECT_TOOL (sel_tool);
  ClayTool              *tool     = CLAY_TOOL (sel_tool);

  if (display == tool->display)
    {
      gint n_points;

      clay_polygon_select_tool_get_points (poly_sel, NULL, &n_points);

      if (n_points > 2)
        return TRUE;
    }

  return CLAY_SELECTION_TOOL_CLASS (parent_class)->have_selection (sel_tool,
                                                                   display);
}

static void
clay_free_select_tool_change_complete (ClayPolygonSelectTool *poly_sel,
                                       ClayDisplay           *display)
{
  ClayFreeSelectTool        *free_sel = CLAY_FREE_SELECT_TOOL (poly_sel);
  ClayFreeSelectToolPrivate *priv     = free_sel->priv;

  priv->changed = TRUE;

  clay_selection_tool_start_change (CLAY_SELECTION_TOOL (free_sel),
                                    FALSE,
                                    priv->operation_at_start);

  if (clay_polygon_select_tool_is_closed (poly_sel))
    clay_free_select_tool_select (free_sel, display);

  clay_selection_tool_end_change (CLAY_SELECTION_TOOL (free_sel),
                                  FALSE);
}

static void
clay_free_select_tool_halt (ClayFreeSelectTool *free_sel)
{
  ClayFreeSelectToolPrivate *priv = free_sel->priv;

  priv->started = FALSE;
}

static void
clay_free_select_tool_commit (ClayFreeSelectTool *free_sel,
                              ClayDisplay        *display)
{
  ClayPolygonSelectTool *poly_sel = CLAY_POLYGON_SELECT_TOOL (free_sel);

  if (! clay_polygon_select_tool_is_closed (poly_sel))
    {
      if (clay_free_select_tool_select (free_sel, display))
        clay_image_flush (clay_display_get_image (display));
    }
}

static gboolean
clay_free_select_tool_select (ClayFreeSelectTool *free_sel,
                              ClayDisplay        *display)
{
  ClaySelectionOptions      *options = CLAY_SELECTION_TOOL_GET_OPTIONS (free_sel);
  ClayTool                  *tool    = CLAY_TOOL (free_sel);
  ClayFreeSelectToolPrivate *priv    = free_sel->priv;
  ClayImage                 *image   = clay_display_get_image (display);
  const ClayVector2         *points;
  gint                       n_points;

  clay_polygon_select_tool_get_points (CLAY_POLYGON_SELECT_TOOL (free_sel),
                                       &points, &n_points);

  if (n_points > 2)
    {
      /* prevent this change from halting the tool */
      clay_tool_control_push_preserve (tool->control, TRUE);

      clay_channel_select_polygon (clay_image_get_mask (image),
                                   C_("command", "Free Select"),
                                   n_points,
                                   points,
                                   priv->operation_at_start,
                                   options->antialias,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius,
                                   TRUE);

      clay_tool_control_pop_preserve (tool->control);

      return TRUE;
    }

  return FALSE;
}
