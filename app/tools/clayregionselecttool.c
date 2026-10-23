/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayregionselecttool.c
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

#include "core/clay-utils.h"
#include "core/clayboundary.h"
#include "core/claychannel.h"
#include "core/claychannel-select.h"
#include "core/clayimage.h"
#include "core/claylayer-floating-selection.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-cursor.h"

#include "clayregionselectoptions.h"
#include "clayregionselecttool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


static void   clay_region_select_tool_finalize       (GObject               *object);

static void   clay_region_select_tool_button_press   (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayButtonPressType    press_type,
                                                      ClayDisplay           *display);
static void   clay_region_select_tool_button_release (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayButtonReleaseType  release_type,
                                                      ClayDisplay           *display);
static void   clay_region_select_tool_motion         (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayDisplay           *display);
static void   clay_region_select_tool_cursor_update  (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      GdkModifierType        state,
                                                      ClayDisplay           *display);

static void   clay_region_select_tool_draw           (ClayDrawTool          *draw_tool);

static void   clay_region_select_tool_get_mask       (ClayRegionSelectTool  *region_sel,
                                                      ClayDisplay           *display);


G_DEFINE_TYPE (ClayRegionSelectTool, clay_region_select_tool,
               CLAY_TYPE_SELECTION_TOOL)

#define parent_class clay_region_select_tool_parent_class


static void
clay_region_select_tool_class_init (ClayRegionSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = clay_region_select_tool_finalize;

  tool_class->button_press   = clay_region_select_tool_button_press;
  tool_class->button_release = clay_region_select_tool_button_release;
  tool_class->motion         = clay_region_select_tool_motion;
  tool_class->cursor_update  = clay_region_select_tool_cursor_update;

  draw_tool_class->draw      = clay_region_select_tool_draw;
}

static void
clay_region_select_tool_init (ClayRegionSelectTool *region_select)
{
  ClayTool *tool = CLAY_TOOL (region_select);

  clay_tool_control_set_scroll_lock (tool->control, TRUE);

  region_select->x               = 0;
  region_select->y               = 0;
  region_select->saved_threshold = 0.0;

  region_select->region_mask     = NULL;
  region_select->segs            = NULL;
  region_select->n_segs          = 0;
}

static void
clay_region_select_tool_finalize (GObject *object)
{
  ClayRegionSelectTool *region_sel = CLAY_REGION_SELECT_TOOL (object);

  g_clear_object (&region_sel->region_mask);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_region_select_tool_button_press (ClayTool            *tool,
                                      const ClayCoords    *coords,
                                      guint32              time,
                                      GdkModifierType      state,
                                      ClayButtonPressType  press_type,
                                      ClayDisplay         *display)
{
  ClayRegionSelectTool    *region_sel = CLAY_REGION_SELECT_TOOL (tool);
  ClayRegionSelectOptions *options    = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);

  region_sel->x               = coords->x;
  region_sel->y               = coords->y;
  region_sel->saved_threshold = options->threshold;

  if (clay_selection_tool_start_edit (CLAY_SELECTION_TOOL (region_sel),
                                      display, coords))
    {
      return;
    }

  clay_tool_control_activate (tool->control);
  tool->display = display;

  clay_tool_push_status (tool, display,
                         _("Move the mouse to change threshold"));

  clay_region_select_tool_get_mask (region_sel, display);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

static void
clay_region_select_tool_button_release (ClayTool              *tool,
                                        const ClayCoords      *coords,
                                        guint32                time,
                                        GdkModifierType        state,
                                        ClayButtonReleaseType  release_type,
                                        ClayDisplay           *display)
{
  ClayRegionSelectTool    *region_sel  = CLAY_REGION_SELECT_TOOL (tool);
  ClaySelectionOptions    *sel_options = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayRegionSelectOptions *options     = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  ClayImage               *image       = clay_display_get_image (display);

  clay_tool_pop_status (tool, display);

  clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  clay_tool_control_halt (tool->control);

  if (options->draw_mask)
    clay_display_shell_set_mask (clay_display_get_shell (display),
                                 NULL, 0, 0, NULL, FALSE);

  if (release_type != CLAY_BUTTON_RELEASE_CANCEL)
    {
      if (CLAY_SELECTION_TOOL (tool)->function == SELECTION_ANCHOR)
        {
          if (clay_image_get_floating_selection (image))
            {
              /*  If there is a floating selection, anchor it  */
              floating_sel_anchor (clay_image_get_floating_selection (image));
            }
          else
            {
              /*  Otherwise, clear the selection mask  */
              clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);
            }

          clay_image_flush (image);
        }
      else if (region_sel->region_mask)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              GList *drawables = clay_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                clay_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          clay_channel_select_buffer (clay_image_get_mask (image),
                                      CLAY_REGION_SELECT_TOOL_GET_CLASS (tool)->undo_desc,
                                      region_sel->region_mask,
                                      off_x,
                                      off_y,
                                      sel_options->operation,
                                      sel_options->feather,
                                      sel_options->feather_radius,
                                      sel_options->feather_radius);


          clay_image_flush (image);
        }
    }

  g_clear_object (&region_sel->region_mask);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  /*  Restore the original threshold  */
  g_object_set (options,
                "threshold", region_sel->saved_threshold,
                NULL);
}

static void
clay_region_select_tool_motion (ClayTool         *tool,
                                const ClayCoords *coords,
                                guint32           time,
                                GdkModifierType   state,
                                ClayDisplay      *display)
{
  ClayRegionSelectTool    *region_sel = CLAY_REGION_SELECT_TOOL (tool);
  ClayRegionSelectOptions *options    = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  gint                     diff_x, diff_y;
  gdouble                  diff;

  static guint32 last_time = 0;

  /* don't let the events come in too fast, ignore below a delay of 100 ms */
  if (time - last_time < 100)
    return;

  last_time = time;

  diff_x = coords->x - region_sel->x;
  diff_y = coords->y - region_sel->y;

  diff = ((ABS (diff_x) > ABS (diff_y)) ? diff_x : diff_y) / 2.0;

  g_object_set (options,
                "threshold", CLAMP (region_sel->saved_threshold + diff, 0, 255),
                NULL);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  clay_region_select_tool_get_mask (region_sel, display);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_region_select_tool_cursor_update (ClayTool         *tool,
                                       const ClayCoords *coords,
                                       GdkModifierType   state,
                                       ClayDisplay      *display)
{
  ClayRegionSelectOptions *options  = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (tool);
  ClayCursorModifier       modifier = CLAY_CURSOR_MODIFIER_NONE;
  ClayImage               *image    = clay_display_get_image (display);

  if (! clay_image_coords_in_active_pickable (image, coords,
                                              FALSE, options->sample_merged,
                                              FALSE))
    modifier = CLAY_CURSOR_MODIFIER_BAD;

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_region_select_tool_draw (ClayDrawTool *draw_tool)
{
  ClayRegionSelectTool    *region_sel = CLAY_REGION_SELECT_TOOL (draw_tool);
  ClayRegionSelectOptions *options    = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (draw_tool);

  if (! options->draw_mask && region_sel->region_mask)
    {
      if (! region_sel->segs)
        {
          /*  calculate and allocate a new segment array which represents
           *  the boundary of the contiguous region
           */
          region_sel->segs = clay_boundary_find (region_sel->region_mask, NULL,
                                                 babl_format ("Y float"),
                                                 CLAY_BOUNDARY_WITHIN_BOUNDS,
                                                 0, 0,
                                                 gegl_buffer_get_width  (region_sel->region_mask),
                                                 gegl_buffer_get_height (region_sel->region_mask),
                                                 CLAY_BOUNDARY_HALF_WAY,
                                                 &region_sel->n_segs);

        }

      if (region_sel->segs)
        {
          gint off_x = 0;
          gint off_y = 0;

          if (! options->sample_merged)
            {
              ClayImage *image     = clay_display_get_image (draw_tool->display);
              GList     *drawables = clay_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                clay_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          clay_draw_tool_add_boundary (draw_tool,
                                       region_sel->segs,
                                       region_sel->n_segs,
                                       NULL,
                                       off_x, off_y);
        }
    }
}

static void
clay_region_select_tool_get_mask (ClayRegionSelectTool *region_sel,
                                  ClayDisplay          *display)
{
  ClayRegionSelectOptions *options = CLAY_REGION_SELECT_TOOL_GET_OPTIONS (region_sel);
  ClayDisplayShell        *shell   = clay_display_get_shell (display);

  clay_display_shell_set_override_cursor (shell, (ClayCursorType) GDK_WATCH);

  g_clear_pointer (&region_sel->segs, g_free);
  region_sel->n_segs = 0;

  if (region_sel->region_mask)
    g_object_unref (region_sel->region_mask);

  region_sel->region_mask =
    CLAY_REGION_SELECT_TOOL_GET_CLASS (region_sel)->get_mask (region_sel,
                                                              display);

  if (options->draw_mask)
    {
      if (region_sel->region_mask)
        {
          GeglColor *color = gegl_color_new ("fuchsia");
          gint       off_x = 0;
          gint       off_y = 0;

          if (! options->sample_merged)
            {
              ClayImage *image     = clay_display_get_image (display);
              GList     *drawables = clay_image_get_selected_drawables (image);

              if (g_list_length (drawables) == 1)
                clay_item_get_offset (drawables->data, &off_x, &off_y);

              g_list_free (drawables);
            }

          clay_display_shell_set_mask (shell, region_sel->region_mask,
                                       off_x, off_y, color, FALSE);
          g_object_unref (color);
        }
      else
        {
          clay_display_shell_set_mask (shell, NULL, 0, 0, NULL, FALSE);
        }
    }

  clay_display_shell_unset_override_cursor (shell);
}
