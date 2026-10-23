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

#include "core/clay.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-undo.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-selection.h"
#include "display/claydisplayshell-transform.h"

#include "clayguidetool.h"
#include "claytoolcontrol.h"
#include "tool_manager.h"

#include "clay-intl.h"


#define SWAP_ORIENT(orient) ((orient) == CLAY_ORIENTATION_HORIZONTAL ? \
                             CLAY_ORIENTATION_VERTICAL : \
                             CLAY_ORIENTATION_HORIZONTAL)


/*  local function prototypes  */

static void   clay_guide_tool_finalize       (GObject               *object);

static void   clay_guide_tool_button_release (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayButtonReleaseType  release_type,
                                              ClayDisplay           *display);
static void   clay_guide_tool_motion         (ClayTool              *tool,
                                              const ClayCoords      *coords,
                                              guint32                time,
                                              GdkModifierType        state,
                                              ClayDisplay           *display);

static void   clay_guide_tool_draw           (ClayDrawTool          *draw_tool);

static void   clay_guide_tool_start          (ClayTool              *parent_tool,
                                              ClayDisplay           *display,
                                              GList                 *guides,
                                              ClayOrientationType    orientation);

static void   clay_guide_tool_push_status    (ClayGuideTool         *guide_tool,
                                              ClayDisplay           *display,
                                              gboolean               remove_guides);


G_DEFINE_TYPE (ClayGuideTool, clay_guide_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_guide_tool_parent_class


static void
clay_guide_tool_class_init (ClayGuideToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = clay_guide_tool_finalize;

  tool_class->button_release = clay_guide_tool_button_release;
  tool_class->motion         = clay_guide_tool_motion;

  draw_tool_class->draw      = clay_guide_tool_draw;
}

static void
clay_guide_tool_init (ClayGuideTool *guide_tool)
{
  ClayTool *tool = CLAY_TOOL (guide_tool);

  clay_tool_control_set_snap_to            (tool->control, FALSE);
  clay_tool_control_set_handle_empty_image (tool->control, TRUE);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_MOVE);
  clay_tool_control_set_scroll_lock        (tool->control, TRUE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_PIXEL_BORDER);

  guide_tool->guides   = NULL;
  guide_tool->n_guides = 0;
}

static void
clay_guide_tool_finalize (GObject *object)
{
  ClayGuideTool *guide_tool = CLAY_GUIDE_TOOL (object);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    g_clear_object (&guide_tool->guides[i].guide);

  g_free (guide_tool->guides);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_guide_tool_button_release (ClayTool              *tool,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type,
                                ClayDisplay           *display)
{
  ClayGuideTool    *guide_tool = CLAY_GUIDE_TOOL (tool);
  ClayDisplayShell *shell      = clay_display_get_shell (display);
  ClayImage        *image      = clay_display_get_image (display);
  gint              i;

  clay_tool_pop_status (tool, display);

  clay_tool_control_halt (tool->control);

  clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      for (i = 0; i < guide_tool->n_guides; i++)
        {
          ClayGuideToolGuide *guide = &guide_tool->guides[i];

          /* custom guides are moved live */
          if (guide->custom)
            {
              clay_image_move_guide (image, guide->guide, guide->old_position,
                                     TRUE);
            }
        }
    }
  else
    {
      gint     n_non_custom_guides = 0;
      gboolean remove_guides       = FALSE;

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          ClayGuideToolGuide *guide = &guide_tool->guides[i];

          n_non_custom_guides += ! guide->custom;

          if (guide->position == CLAY_GUIDE_POSITION_UNDEFINED)
            {
              remove_guides = TRUE;
            }
        }

      if (n_non_custom_guides > 1)
        {
          clay_image_undo_group_start (image, CLAY_UNDO_GROUP_GUIDE,
                                       remove_guides ?
                                         C_("undo-type", "Remove Guides") :
                                         C_("undo-type", "Move Guides"));
        }

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          ClayGuideToolGuide *guide = &guide_tool->guides[i];

          if (remove_guides)
            {
              /* removing a guide can cause other guides to be removed as well
               * (in particular, in case of symmetry guides).  these guides
               * will be kept alive, since we hold a reference on them, but we
               * need to make sure that they're still part of the image.
               */
              if (g_list_find (clay_image_get_guides (image), guide->guide))
                clay_image_remove_guide (image, guide->guide, TRUE);
            }
          else
            {
              if (guide->guide)
                {
                  /* custom guides are moved live */
                  if (! guide->custom)
                    {
                      clay_image_move_guide (image, guide->guide,
                                             guide->position, TRUE);
                    }
                }
              else
                {
                  switch (guide->orientation)
                    {
                    case CLAY_ORIENTATION_HORIZONTAL:
                      clay_image_add_hguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    case CLAY_ORIENTATION_VERTICAL:
                      clay_image_add_vguide (image,
                                             guide->position,
                                             TRUE);
                      break;

                    default:
                      clay_assert_not_reached ();
                    }
                }
            }
        }

      if (n_non_custom_guides > 1)
        clay_image_undo_group_end (image);

      clay_image_flush (image);
    }

  clay_display_shell_selection_resume (shell);

  tool_manager_pop_tool (display->clay);
  g_object_unref (guide_tool);

  {
    ClayTool *active_tool = tool_manager_get_active (display->clay);

    if (CLAY_IS_DRAW_TOOL (active_tool))
      clay_draw_tool_pause (CLAY_DRAW_TOOL (active_tool));

    tool_manager_oper_update_active (display->clay, coords, state,
                                     TRUE, display);
    tool_manager_cursor_update_active (display->clay, coords, state,
                                       display);

    if (CLAY_IS_DRAW_TOOL (active_tool))
      clay_draw_tool_resume (CLAY_DRAW_TOOL (active_tool));
  }
}

static void
clay_guide_tool_motion (ClayTool         *tool,
                        const ClayCoords *coords,
                        guint32           time,
                        GdkModifierType   state,
                        ClayDisplay      *display)

{
  ClayGuideTool    *guide_tool    = CLAY_GUIDE_TOOL (tool);
  ClayDisplayShell *shell         = clay_display_get_shell (display);
  ClayImage        *image         = clay_display_get_image (display);
  gboolean          remove_guides = FALSE;
  gint              tx, ty;
  gint              i;

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  clay_display_shell_transform_xy (shell,
                                   coords->x, coords->y,
                                   &tx, &ty);

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      ClayGuideToolGuide *guide = &guide_tool->guides[i];
      gint                max_position;
      gint                position;

      if (guide->orientation == CLAY_ORIENTATION_HORIZONTAL)
        max_position = clay_image_get_height (image);
      else
        max_position = clay_image_get_width (image);

      if (guide->orientation == CLAY_ORIENTATION_HORIZONTAL)
        guide->position = RINT (coords->y);
      else
        guide->position = RINT (coords->x);

      position = CLAMP (guide->position, 0, max_position);

      if (tx < 0 || tx >= shell->disp_width ||
          ty < 0 || ty >= shell->disp_height)
        {
          guide->position = CLAY_GUIDE_POSITION_UNDEFINED;

          remove_guides = TRUE;
        }
      else
        {
          if (guide->position < 0 || guide->position > max_position)
            remove_guides = TRUE;
        }

      /* custom guides are moved live */
      if (guide->custom)
        clay_image_move_guide (image, guide->guide, position, TRUE);
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));

  clay_tool_pop_status (tool, display);

  clay_guide_tool_push_status (guide_tool, display, remove_guides);
}

static void
clay_guide_tool_draw (ClayDrawTool *draw_tool)
{
  ClayGuideTool *guide_tool = CLAY_GUIDE_TOOL (draw_tool);
  gint           i;

  for (i = 0; i < guide_tool->n_guides; i++)
    {
      ClayGuideToolGuide *guide = &guide_tool->guides[i];

      if (guide->position != CLAY_GUIDE_POSITION_UNDEFINED)
        {
          /* custom guides are moved live */
          if (! guide->custom)
            {
              clay_draw_tool_add_guide (draw_tool,
                                        guide->orientation,
                                        guide->position,
                                        CLAY_GUIDE_STYLE_NONE);
            }
        }
    }
}

static void
clay_guide_tool_start (ClayTool            *parent_tool,
                       ClayDisplay         *display,
                       GList               *guides,
                       ClayOrientationType  orientation)
{
  ClayGuideTool *guide_tool;
  ClayTool      *tool;

  guide_tool = g_object_new (CLAY_TYPE_GUIDE_TOOL,
                             "tool-info", parent_tool->tool_info,
                             NULL);

  tool = CLAY_TOOL (guide_tool);

  clay_display_shell_selection_pause (clay_display_get_shell (display));

  if (guides)
    {
      gint i;

      guide_tool->n_guides = g_list_length (guides);
      guide_tool->guides   = g_new (ClayGuideToolGuide, guide_tool->n_guides);

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          ClayGuide *guide = guides->data;

          guide_tool->guides[i].guide        = g_object_ref (guide);
          guide_tool->guides[i].old_position = clay_guide_get_position (guide);
          guide_tool->guides[i].position     = clay_guide_get_position (guide);
          guide_tool->guides[i].orientation  = clay_guide_get_orientation (guide);
          guide_tool->guides[i].custom       = clay_guide_is_custom (guide);

          guides = g_list_next (guides);
        }
    }
  else
    {
      guide_tool->n_guides = 1;
      guide_tool->guides   = g_new (ClayGuideToolGuide, 1);

      guide_tool->guides[0].guide        = NULL;
      guide_tool->guides[0].old_position = 0;
      guide_tool->guides[0].position     = CLAY_GUIDE_POSITION_UNDEFINED;
      guide_tool->guides[0].orientation  = orientation;
      guide_tool->guides[0].custom       = FALSE;
    }

  clay_tool_set_cursor (tool, display,
                        CLAY_CURSOR_MOUSE,
                        CLAY_TOOL_CURSOR_HAND,
                        CLAY_CURSOR_MODIFIER_MOVE);

  tool_manager_push_tool (display->clay, tool);

  tool->display = display;
  clay_tool_control_activate (tool->control);

  clay_draw_tool_start (CLAY_DRAW_TOOL (guide_tool), display);

  clay_guide_tool_push_status (guide_tool, display, FALSE);
}

static void
clay_guide_tool_push_status (ClayGuideTool *guide_tool,
                             ClayDisplay   *display,
                             gboolean       remove_guides)
{
  ClayTool *tool = CLAY_TOOL (guide_tool);

  if (remove_guides)
    {
      clay_tool_push_status (tool, display,
                             guide_tool->n_guides > 1    ? _("Remove Guides") :
                             guide_tool->guides[0].guide ? _("Remove Guide")  :
                                                           _("Cancel Guide"));
    }
  else
    {
      ClayGuideToolGuide *guides[2] = { 0, };
      gint                n_guides = 0;
      gint                i;

      for (i = 0; i < guide_tool->n_guides; i++)
        {
          ClayGuideToolGuide *guide = &guide_tool->guides[i];

          if (guide_tool->guides[i].guide)
            {
              if (n_guides == 0 || guide->orientation != guides[0]->orientation)
                {
                  guides[n_guides++] = guide;

                  if (n_guides == 2)
                    break;
                }
            }
        }

      if (n_guides == 2 &&
          guides[0]->orientation == CLAY_ORIENTATION_HORIZONTAL)
        {
          ClayGuideToolGuide *temp;

          temp      = guides[0];
          guides[0] = guides[1];
          guides[1] = temp;
        }

       if (n_guides == 1)
        {
          clay_tool_push_status_length (tool, display,
                                        _("Move Guide: "),
                                        SWAP_ORIENT (guides[0]->orientation),
                                        guides[0]->position -
                                        guides[0]->old_position,
                                        NULL);
        }
      else if (n_guides == 2)
        {
          clay_tool_push_status_coords (tool, display,
                                        CLAY_CURSOR_PRECISION_PIXEL_BORDER,
                                        _("Move Guides: "),
                                        guides[0]->position -
                                        guides[0]->old_position,
                                        ", ",
                                        guides[1]->position -
                                        guides[1]->old_position,
                                        NULL);
        }
      else
        {
          clay_tool_push_status_length (tool, display,
                                        _("Add Guide: "),
                                        SWAP_ORIENT (guide_tool->guides[0].orientation),
                                        guide_tool->guides[0].position,
                                        NULL);
        }
    }
}


/*  public functions  */

void
clay_guide_tool_start_new (ClayTool            *parent_tool,
                           ClayDisplay         *display,
                           ClayOrientationType  orientation)
{
  g_return_if_fail (CLAY_IS_TOOL (parent_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (orientation != CLAY_ORIENTATION_UNKNOWN);

  clay_guide_tool_start (parent_tool, display,
                         NULL, orientation);
}

void
clay_guide_tool_start_edit (ClayTool    *parent_tool,
                            ClayDisplay *display,
                            ClayGuide   *guide)
{
  GList *guides = NULL;

  g_return_if_fail (CLAY_IS_TOOL (parent_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  guides = g_list_append (guides, guide);

  clay_guide_tool_start (parent_tool, display,
                         guides, CLAY_ORIENTATION_UNKNOWN);

  g_list_free (guides);
}

void
clay_guide_tool_start_edit_many (ClayTool    *parent_tool,
                                 ClayDisplay *display,
                                 GList       *guides)
{
  g_return_if_fail (CLAY_IS_TOOL (parent_tool));
  g_return_if_fail (CLAY_IS_DISPLAY (display));
  g_return_if_fail (guides != NULL);

  clay_guide_tool_start (parent_tool, display,
                         guides, CLAY_ORIENTATION_UNKNOWN);
}
