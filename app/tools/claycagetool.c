/* xyz.kainzmayer.ClayDesign
 *
 * claycagetool.c
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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
#include <gdk/gdkkeysyms.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "gegl/clay-gegl-utils.h"

#include "operations/claycageconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/clayitem.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"

#include "claycagetool.h"
#include "claycageoptions.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


/* XXX: if this state list is updated, in particular if for some reason,
   a new CAGE_STATE_* was to be inserted after CAGE_STATE_CLOSING, check
   if the function clay_cage_tool_is_complete() has to be updated.
   Current algorithm is that all DEFORM_* states are complete states,
   and all CAGE_* states are incomplete states. */
enum
{
  CAGE_STATE_INIT,
  CAGE_STATE_WAIT,
  CAGE_STATE_MOVE_HANDLE,
  CAGE_STATE_SELECTING,
  CAGE_STATE_CLOSING,
  DEFORM_STATE_WAIT,
  DEFORM_STATE_MOVE_HANDLE,
  DEFORM_STATE_SELECTING
};


static gboolean   clay_cage_tool_initialize         (ClayTool              *tool,
                                                     ClayDisplay           *display,
                                                     GError               **error);
static void       clay_cage_tool_control            (ClayTool              *tool,
                                                     ClayToolAction         action,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_button_press       (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     ClayButtonPressType    press_type,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_button_release     (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     ClayButtonReleaseType  release_type,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_motion             (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     ClayDisplay           *display);
static gboolean   clay_cage_tool_key_press          (ClayTool              *tool,
                                                     GdkEventKey           *kevent,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_cursor_update      (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     GdkModifierType        state,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_oper_update        (ClayTool              *tool,
                                                     const ClayCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_options_notify     (ClayTool              *tool,
                                                     ClayToolOptions       *options,
                                                     const GParamSpec      *pspec);

static void       clay_cage_tool_draw               (ClayDrawTool          *draw_tool);

static void       clay_cage_tool_start              (ClayCageTool          *ct,
                                                     ClayDisplay           *display);
static void       clay_cage_tool_halt               (ClayCageTool          *ct);
static void       clay_cage_tool_commit             (ClayCageTool          *ct);

static gint       clay_cage_tool_is_on_handle       (ClayCageTool          *ct,
                                                     ClayDrawTool          *draw_tool,
                                                     ClayDisplay           *display,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);
static gint       clay_cage_tool_is_on_edge         (ClayCageTool          *ct,
                                                     gdouble                x,
                                                     gdouble                y,
                                                     gint                   handle_size);

static gboolean   clay_cage_tool_is_complete        (ClayCageTool          *ct);
static void       clay_cage_tool_remove_last_handle (ClayCageTool          *ct);
static void       clay_cage_tool_compute_coef       (ClayCageTool          *ct);
static void       clay_cage_tool_create_filter      (ClayCageTool          *ct);
static void       clay_cage_tool_filter_flush       (ClayDrawableFilter    *filter,
                                                     ClayTool              *tool);
static void       clay_cage_tool_filter_update      (ClayCageTool          *ct);

static void       clay_cage_tool_create_render_node (ClayCageTool          *ct);
static void       clay_cage_tool_render_node_update (ClayCageTool          *ct);


G_DEFINE_TYPE (ClayCageTool, clay_cage_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_cage_tool_parent_class


void
clay_cage_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_CAGE_TOOL,
                CLAY_TYPE_CAGE_OPTIONS,
                clay_cage_options_gui,
                0,
                "clay-cage-tool",
                _("Cage Transform"),
                _("Cage Transform: Deform a selection with a cage"),
                N_("_Cage Transform"), "<shift>G",
                NULL, CLAY_HELP_TOOL_CAGE,
                CLAY_ICON_TOOL_CAGE,
                data);
}

static void
clay_cage_tool_class_init (ClayCageToolClass *klass)
{
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  tool_class->initialize     = clay_cage_tool_initialize;
  tool_class->control        = clay_cage_tool_control;
  tool_class->button_press   = clay_cage_tool_button_press;
  tool_class->button_release = clay_cage_tool_button_release;
  tool_class->key_press      = clay_cage_tool_key_press;
  tool_class->motion         = clay_cage_tool_motion;
  tool_class->cursor_update  = clay_cage_tool_cursor_update;
  tool_class->oper_update    = clay_cage_tool_oper_update;
  tool_class->options_notify = clay_cage_tool_options_notify;

  draw_tool_class->draw      = clay_cage_tool_draw;
}

static void
clay_cage_tool_init (ClayCageTool *self)
{
  ClayTool *tool = CLAY_TOOL (self);

  clay_tool_control_set_preserve    (tool->control, FALSE);
  clay_tool_control_set_dirty_mask  (tool->control,
                                     CLAY_DIRTY_IMAGE           |
                                     CLAY_DIRTY_IMAGE_STRUCTURE |
                                     CLAY_DIRTY_DRAWABLE        |
                                     CLAY_DIRTY_SELECTION       |
                                     CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_wants_click (tool->control, TRUE);
  clay_tool_control_set_precision   (tool->control,
                                     CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor (tool->control,
                                     CLAY_TOOL_CURSOR_PERSPECTIVE);

  self->config          = g_object_new (CLAY_TYPE_CAGE_CONFIG, NULL);
  self->hovering_handle = -1;
  self->tool_state      = CAGE_STATE_INIT;
}

static gboolean
clay_cage_tool_initialize (ClayTool     *tool,
                           ClayDisplay  *display,
                           GError      **error)
{
  ClayGuiConfig *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage     *image       = clay_display_get_image (display);
  ClayItem      *locked_item = NULL;
  GList         *drawables   = clay_image_get_selected_drawables (image);
  ClayDrawable  *drawable;

  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                   _("Cannot modify multiple layers. Select only one layer."));
      else
        g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED, _("No selected drawables."));

      g_list_free (drawables);
      return FALSE;
    }

  drawable = drawables->data;
  g_list_free (drawables);

  if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (clay_item_is_content_locked (CLAY_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("The selected item's pixels are locked."));
      if (error)
        clay_tools_blink_lock_box (display->clay, locked_item);
      return FALSE;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("The active item is not visible."));
      return FALSE;
    }

  clay_cage_tool_start (CLAY_CAGE_TOOL (tool), display);

  return TRUE;
}

static void
clay_cage_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayCageTool *ct = CLAY_CAGE_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_cage_tool_halt (ct);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_cage_tool_commit (ct);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_cage_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayCageTool *ct        = CLAY_CAGE_TOOL (tool);
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);
  gint          handle    = -1;
  gint          edge      = -1;

  clay_tool_control_activate (tool->control);

  if (ct->config)
    {
      handle = clay_cage_tool_is_on_handle (ct,
                                            draw_tool,
                                            display,
                                            coords->x,
                                            coords->y,
                                            CLAY_TOOL_HANDLE_SIZE_CIRCLE);
      edge = clay_cage_tool_is_on_edge (ct,
                                        coords->x,
                                        coords->y,
                                        CLAY_TOOL_HANDLE_SIZE_CIRCLE);
    }

  ct->movement_start_x = coords->x;
  ct->movement_start_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_INIT:
      /* No handle yet, we add the first one and switch the tool to
       * moving handle state.
       */
      clay_cage_config_add_cage_point (ct->config,
                                       coords->x - ct->offset_x,
                                       coords->y - ct->offset_y);
      clay_cage_config_select_point (ct->config, 0);
      ct->tool_state = CAGE_STATE_MOVE_HANDLE;
      break;

    case CAGE_STATE_WAIT:
      if (handle == -1 && edge <= 0)
        {
          /* User clicked on the background, we add a new handle
           * and move it
           */
          clay_cage_config_add_cage_point (ct->config,
                                           coords->x - ct->offset_x,
                                           coords->y - ct->offset_y);
          clay_cage_config_select_point (ct->config,
                                         clay_cage_config_get_n_points (ct->config) - 1);
          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (handle == 0 && clay_cage_config_get_n_points (ct->config) > 2)
        {
          /* User clicked on the first handle, we wait for
           * release for closing the cage and switching to
           * deform if possible
           */
          clay_cage_config_select_point (ct->config, 0);
          ct->tool_state = CAGE_STATE_CLOSING;
        }
      else if (handle >= 0)
        {
          /* User clicked on a handle, so we move it */

          if (state & clay_get_extend_selection_mask ())
            {
              /* Multiple selection */

              clay_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! clay_cage_config_point_is_selected (ct->config, handle))
                {
                  clay_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      else if (edge > 0)
        {
          /* User clicked on an edge, we add a new handle here and select it */

          clay_cage_config_insert_cage_point (ct->config, edge,
                                              coords->x, coords->y);
          clay_cage_config_select_point (ct->config, edge);
          ct->tool_state = CAGE_STATE_MOVE_HANDLE;
        }
      break;

    case DEFORM_STATE_WAIT:
      if (handle == -1)
        {
          /* User clicked on the background, we start a rubber band
           * selection
           */
          ct->selection_start_x = coords->x;
          ct->selection_start_y = coords->y;
          ct->tool_state = DEFORM_STATE_SELECTING;
        }

      if (handle >= 0)
        {
          /* User clicked on a handle, so we move it */

          if (state & clay_get_extend_selection_mask ())
            {
              /* Multiple selection */

              clay_cage_config_toggle_point_selection (ct->config, handle);
            }
          else
            {
              /* New selection */

              if (! clay_cage_config_point_is_selected (ct->config, handle))
                {
                  clay_cage_config_select_point (ct->config, handle);
                }
            }

          ct->tool_state = DEFORM_STATE_MOVE_HANDLE;
        }
      break;
    }
}

void
clay_cage_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayCageTool    *ct      = CLAY_CAGE_TOOL (tool);
  ClayCageOptions *options = CLAY_CAGE_TOOL_GET_OPTIONS (ct);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (ct));

  clay_tool_control_halt (tool->control);

  if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
    {
      /* Cancelling */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_MOVE_HANDLE:
          clay_cage_config_remove_last_cage_point (ct->config);
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case CAGE_STATE_SELECTING:
          ct->tool_state = CAGE_STATE_WAIT;
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          clay_cage_tool_filter_update (ct);
          ct->tool_state = DEFORM_STATE_WAIT;
          break;

        case DEFORM_STATE_SELECTING:
          ct->tool_state = DEFORM_STATE_WAIT;
          break;
        }

      clay_cage_config_reset_displacement (ct->config);
    }
  else
    {
      /* Normal release */

      switch (ct->tool_state)
        {
        case CAGE_STATE_CLOSING:
          ct->dirty_coef = TRUE;
          clay_cage_config_commit_displacement (ct->config);

          if (release_type == CLAY_BUTTON_RELEASE_CLICK)
            g_object_set (options, "cage-mode", CLAY_CAGE_MODE_DEFORM, NULL);
          break;

        case CAGE_STATE_MOVE_HANDLE:
          ct->dirty_coef = TRUE;
          ct->tool_state = CAGE_STATE_WAIT;
          clay_cage_config_commit_displacement (ct->config);
          break;

        case CAGE_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & clay_get_extend_selection_mask ())
              {
                clay_cage_config_select_add_area (ct->config,
                                                  CLAY_CAGE_MODE_CAGE_CHANGE,
                                                  area);
              }
            else
              {
                clay_cage_config_select_area (ct->config,
                                              CLAY_CAGE_MODE_CAGE_CHANGE,
                                              area);
              }

            ct->tool_state = CAGE_STATE_WAIT;
          }
          break;

        case DEFORM_STATE_MOVE_HANDLE:
          ct->tool_state = DEFORM_STATE_WAIT;
          clay_cage_config_commit_displacement (ct->config);
          gegl_node_set (ct->cage_node,
                         "config", ct->config,
                         NULL);
          clay_cage_tool_filter_update (ct);
          break;

        case DEFORM_STATE_SELECTING:
          {
            GeglRectangle area =
              { MIN (ct->selection_start_x, coords->x) - ct->offset_x,
                MIN (ct->selection_start_y, coords->y) - ct->offset_y,
                ABS (ct->selection_start_x - coords->x),
                ABS (ct->selection_start_y - coords->y) };

            if (state & clay_get_extend_selection_mask ())
              {
                clay_cage_config_select_add_area (ct->config,
                                                  CLAY_CAGE_MODE_DEFORM, area);
              }
            else
              {
                clay_cage_config_select_area (ct->config,
                                              CLAY_CAGE_MODE_DEFORM, area);
              }

            ct->tool_state = DEFORM_STATE_WAIT;
          }
          break;
        }
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_cage_tool_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
  ClayCageTool    *ct       = CLAY_CAGE_TOOL (tool);
  ClayCageOptions *options  = CLAY_CAGE_TOOL_GET_OPTIONS (ct);

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  switch (ct->tool_state)
    {
    case CAGE_STATE_MOVE_HANDLE:
    case CAGE_STATE_CLOSING:
    case DEFORM_STATE_MOVE_HANDLE:
      clay_cage_config_add_displacement (ct->config,
                                         options->cage_mode,
                                         ct->cursor_x - ct->movement_start_x,
                                         ct->cursor_y - ct->movement_start_y);
      break;
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static gboolean
clay_cage_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  ClayCageTool *ct = CLAY_CAGE_TOOL (tool);

  if (! ct->config)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      if (ct->tool_state == CAGE_STATE_WAIT)
        {
          if (clay_cage_config_get_n_points (ct->config) != 0)
            clay_cage_tool_remove_last_handle (ct);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          clay_cage_config_remove_selected_points (ct->config);

          /* if the cage have less than 3 handles, we reopen it */
          if (clay_cage_config_get_n_points (ct->config) <= 2)
            {
              ct->tool_state = CAGE_STATE_WAIT;
            }

          clay_cage_tool_compute_coef (ct);
          clay_cage_tool_render_node_update (ct);
        }
      return TRUE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (! clay_cage_tool_is_complete (ct) &&
          clay_cage_config_get_n_points (ct->config) > 2)
        {
          g_object_set (clay_tool_get_options (tool),
                        "cage-mode", CLAY_CAGE_MODE_DEFORM,
                        NULL);
        }
      else if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);
        }
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
clay_cage_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayCageTool *ct        = CLAY_CAGE_TOOL (tool);
  ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

  if (ct->config)
    {
      ct->hovering_handle = clay_cage_tool_is_on_handle (ct,
                                                         draw_tool,
                                                         display,
                                                         coords->x,
                                                         coords->y,
                                                         CLAY_TOOL_HANDLE_SIZE_CIRCLE);

      ct->hovering_edge = clay_cage_tool_is_on_edge (ct,
                                                     coords->x,
                                                     coords->y,
                                                     CLAY_TOOL_HANDLE_SIZE_CIRCLE);
    }

  clay_draw_tool_pause (draw_tool);

  ct->cursor_x = coords->x;
  ct->cursor_y = coords->y;

  clay_draw_tool_resume (draw_tool);
}

static void
clay_cage_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayCageTool       *ct       = CLAY_CAGE_TOOL (tool);
  ClayCageOptions    *options  = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  ClayGuiConfig      *config   = CLAY_GUI_CONFIG (display->clay->config);
  ClayCursorModifier  modifier = CLAY_CURSOR_MODIFIER_PLUS;

  if (tool->display)
    {
      if (ct->hovering_handle != -1)
        {
          modifier = CLAY_CURSOR_MODIFIER_MOVE;
        }
      else if (ct->hovering_edge  != -1 &&
               options->cage_mode == CLAY_CAGE_MODE_CAGE_CHANGE)
        {
          modifier = CLAY_CURSOR_MODIFIER_PLUS;
        }
      else
        {
          if (clay_cage_tool_is_complete (ct))
            modifier = CLAY_CURSOR_MODIFIER_BAD;
        }
    }
  else
    {
      ClayImage    *image     = clay_display_get_image (display);
      GList        *drawables = clay_image_get_selected_drawables (image);
      ClayDrawable *drawable  = NULL;

      if (g_list_length (drawables) == 1)
        drawable = drawables->data;
      g_list_free (drawables);

      if (! drawable                                               ||
          clay_viewable_get_children (CLAY_VIEWABLE (drawable))    ||
          clay_item_is_content_locked (CLAY_ITEM (drawable), NULL) ||
          ! (clay_item_is_visible (CLAY_ITEM (drawable)) ||
             config->edit_non_visible))
        {
          modifier = CLAY_CURSOR_MODIFIER_BAD;
        }
    }

  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_cage_tool_options_notify (ClayTool         *tool,
                               ClayToolOptions  *options,
                               const GParamSpec *pspec)
{
  ClayCageTool *ct = CLAY_CAGE_TOOL (tool);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! tool->display)
    return;

  clay_draw_tool_pause (CLAY_DRAW_TOOL (tool));

  if (strcmp (pspec->name, "cage-mode") == 0)
    {
      ClayCageMode mode;

      g_object_get (options,
                    "cage-mode", &mode,
                    NULL);

      if (mode == CLAY_CAGE_MODE_DEFORM)
        {
          /* switch to deform mode */

          if (clay_cage_config_get_n_points (ct->config) > 2)
            {
              clay_cage_config_reset_displacement (ct->config);
              clay_cage_config_reverse_cage_if_needed (ct->config);
              clay_tool_push_status (tool, tool->display,
                                     _("Press ENTER to commit the transform"));
              ct->tool_state = DEFORM_STATE_WAIT;

              if (! ct->render_node)
                {
                  clay_cage_tool_create_render_node (ct);
                }

              if (ct->dirty_coef)
                {
                  clay_cage_tool_compute_coef (ct);
                  clay_cage_tool_render_node_update (ct);
                }

              if (! ct->filter)
                clay_cage_tool_create_filter (ct);

              clay_cage_tool_filter_update (ct);
            }
          else
            {
              g_object_set (options,
                            "cage-mode", CLAY_CAGE_MODE_CAGE_CHANGE,
                            NULL);
            }
        }
      else
        {
          /* switch to edit mode */
          if (ct->filter)
            {
              clay_drawable_filter_abort (ct->filter);

              clay_tool_pop_status (tool, tool->display);
              ct->tool_state = CAGE_STATE_WAIT;
            }
        }
    }
  else if (strcmp  (pspec->name, "fill-plain-color") == 0)
    {
      if (ct->tool_state == DEFORM_STATE_WAIT)
        {
          clay_cage_tool_render_node_update (ct);
          clay_cage_tool_filter_update (ct);
        }
    }

  clay_draw_tool_resume (CLAY_DRAW_TOOL (tool));
}

static void
clay_cage_tool_draw (ClayDrawTool *draw_tool)
{
  ClayCageTool    *ct        = CLAY_CAGE_TOOL (draw_tool);
  ClayCageOptions *options   = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  ClayCageConfig  *config    = ct->config;
  ClayCanvasGroup *stroke_group;
  gint             n_vertices;
  gint             i;
  ClayHandleType   handle;

  n_vertices = clay_cage_config_get_n_points (config);

  if (n_vertices == 0)
    return;

  if (ct->tool_state == CAGE_STATE_INIT)
    return;

  stroke_group = clay_draw_tool_add_stroke_group (draw_tool);

  clay_draw_tool_push_group (draw_tool, stroke_group);

  /* If needed, draw line to the cursor. */
  if (! clay_cage_tool_is_complete (ct))
    {
      ClayVector2 last_point;

      last_point = clay_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          n_vertices - 1);

      clay_draw_tool_add_line (draw_tool,
                               last_point.x + ct->offset_x,
                               last_point.y + ct->offset_y,
                               ct->cursor_x,
                               ct->cursor_y);
    }

  clay_draw_tool_pop_group (draw_tool);

  /* Draw the cage with handles. */
  for (i = 0; i < n_vertices; i++)
    {
      ClayCanvasItem *item;
      ClayVector2     point1, point2;

      point1 = clay_cage_config_get_point_coordinate (ct->config,
                                                      options->cage_mode,
                                                      i);
      point1.x += ct->offset_x;
      point1.y += ct->offset_y;

      if (i > 0 || clay_cage_tool_is_complete (ct))
        {
          gint index_point2;

          if (i == 0)
            index_point2 = n_vertices - 1;
          else
            index_point2 = i - 1;

          point2 = clay_cage_config_get_point_coordinate (ct->config,
                                                          options->cage_mode,
                                                          index_point2);
          point2.x += ct->offset_x;
          point2.y += ct->offset_y;

          if (i != ct->hovering_edge ||
              clay_cage_tool_is_complete (ct))
            {
              clay_draw_tool_push_group (draw_tool, stroke_group);
            }

          item = clay_draw_tool_add_line (draw_tool,
                                          point1.x,
                                          point1.y,
                                          point2.x,
                                          point2.y);

          if (i == ct->hovering_edge &&
              ! clay_cage_tool_is_complete (ct))
            {
              clay_canvas_item_set_highlight (item, TRUE);
            }
          else
            {
              clay_draw_tool_pop_group (draw_tool);
            }
        }

      if (clay_cage_config_point_is_selected (ct->config, i))
        {
          if (i == ct->hovering_handle)
            handle = CLAY_HANDLE_FILLED_SQUARE;
          else
            handle = CLAY_HANDLE_SQUARE;
        }
      else
        {
          if (i == ct->hovering_handle)
            handle = CLAY_HANDLE_FILLED_CIRCLE;
          else
            handle = CLAY_HANDLE_CIRCLE;
        }

      item = clay_draw_tool_add_handle (draw_tool,
                                        handle,
                                        point1.x,
                                        point1.y,
                                        CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                        CLAY_TOOL_HANDLE_SIZE_CIRCLE,
                                        CLAY_HANDLE_ANCHOR_CENTER);

      if (i == ct->hovering_handle)
        clay_canvas_item_set_highlight (item, TRUE);
    }

  if (ct->tool_state == DEFORM_STATE_SELECTING ||
      ct->tool_state == CAGE_STATE_SELECTING)
    {
      clay_draw_tool_add_rectangle (draw_tool,
                                    FALSE,
                                    MIN (ct->selection_start_x, ct->cursor_x),
                                    MIN (ct->selection_start_y, ct->cursor_y),
                                    ABS (ct->selection_start_x - ct->cursor_x),
                                    ABS (ct->selection_start_y - ct->cursor_y));
    }
}

static void
clay_cage_tool_start (ClayCageTool *ct,
                      ClayDisplay  *display)
{
  ClayTool     *tool     = CLAY_TOOL (ct);
  ClayImage    *image    = clay_display_get_image (display);
  GList        *drawables = clay_image_get_selected_drawables (image);

  g_return_if_fail (g_list_length (drawables) == 1);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  g_clear_object (&ct->config);

  g_clear_object (&ct->coef);
  ct->dirty_coef = TRUE;

  if (ct->filter)
    {
      clay_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);
    }

  if (ct->render_node)
    {
      g_clear_object (&ct->render_node);
      ct->coef_node = NULL;
      ct->cage_node = NULL;
    }

  ct->config          = g_object_new (CLAY_TYPE_CAGE_CONFIG, NULL);
  ct->hovering_handle = -1;
  ct->hovering_edge   = -1;
  ct->tool_state      = CAGE_STATE_INIT;

  /* Setting up cage offset to convert the cage point coords to
   * drawable coords
   */
  clay_item_get_offset (CLAY_ITEM (tool->drawables->data),
                        &ct->offset_x, &ct->offset_y);

  clay_draw_tool_start (CLAY_DRAW_TOOL (ct), display);
}

static void
clay_cage_tool_halt (ClayCageTool *ct)
{
  ClayTool *tool = CLAY_TOOL (ct);

  g_clear_object (&ct->config);
  g_clear_object (&ct->coef);
  g_clear_object (&ct->render_node);
  ct->coef_node = NULL;
  ct->cage_node = NULL;

  if (ct->filter)
    {
      clay_tool_control_push_preserve (tool->control, TRUE);

      clay_drawable_filter_abort (ct->filter);
      g_clear_object (&ct->filter);

      clay_tool_control_pop_preserve (tool->control);

      clay_image_flush (clay_display_get_image (tool->display));
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;
  ct->tool_state  = CAGE_STATE_INIT;

  g_object_set (clay_tool_get_options (tool),
                "cage-mode", CLAY_CAGE_MODE_CAGE_CHANGE,
                NULL);
}

static void
clay_cage_tool_commit (ClayCageTool *ct)
{
  if (ct->filter)
    {
      ClayTool *tool = CLAY_TOOL (ct);

      clay_tool_control_push_preserve (tool->control, TRUE);

      clay_drawable_filter_commit (ct->filter, FALSE,
                                   CLAY_PROGRESS (tool), FALSE);
      g_clear_object (&ct->filter);

      clay_tool_control_pop_preserve (tool->control);

      clay_image_flush (clay_display_get_image (tool->display));
    }
}

static gint
clay_cage_tool_is_on_handle (ClayCageTool *ct,
                             ClayDrawTool *draw_tool,
                             ClayDisplay  *display,
                             gdouble       x,
                             gdouble       y,
                             gint          handle_size)
{
  ClayCageOptions *options = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  ClayCageConfig  *config  = ct->config;
  gdouble          dist    = G_MAXDOUBLE;
  gint             i;
  ClayVector2      cage_point;
  guint            n_cage_vertices;

  g_return_val_if_fail (CLAY_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = clay_cage_config_get_n_points (config);

  if (n_cage_vertices == 0)
    return -1;

  for (i = 0; i < n_cage_vertices; i++)
    {
      cage_point = clay_cage_config_get_point_coordinate (config,
                                                          options->cage_mode,
                                                          i);
      cage_point.x += ct->offset_x;
      cage_point.y += ct->offset_y;

      dist = clay_draw_tool_calc_distance_square (CLAY_DRAW_TOOL (draw_tool),
                                                  display,
                                                  x, y,
                                                  cage_point.x,
                                                  cage_point.y);

      if (dist <= SQR (handle_size / 2))
        return i;
    }

  return -1;
}

static gint
clay_cage_tool_is_on_edge (ClayCageTool *ct,
                           gdouble       x,
                           gdouble       y,
                           gint          handle_size)
{
  ClayCageOptions *options = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  ClayCageConfig  *config  = ct->config;
  gint             i;
  guint            n_cage_vertices;
  ClayVector2      A, B, C, AB, BC, AC;
  gdouble          lAB, lBC, lAC, lEB, lEC;

  g_return_val_if_fail (CLAY_IS_CAGE_TOOL (ct), -1);

  n_cage_vertices = clay_cage_config_get_n_points (config);

  if (n_cage_vertices < 2)
    return -1;

  A = clay_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             n_cage_vertices-1);
  B = clay_cage_config_get_point_coordinate (config,
                                             options->cage_mode,
                                             0);
  C.x = x;
  C.y = y;

  for (i = 0; i < n_cage_vertices; i++)
    {
      clay_vector2_sub (&AB, &A, &B);
      clay_vector2_sub (&BC, &B, &C);
      clay_vector2_sub (&AC, &A, &C);

      lAB = clay_vector2_length (&AB);
      lBC = clay_vector2_length (&BC);
      lAC = clay_vector2_length (&AC);
      lEB = lAB / 2 + (SQR (lBC) - SQR (lAC)) / (2 * lAB);
      lEC = sqrt (SQR (lBC) - SQR (lEB));

      if ((lEC < handle_size / 2) && (ABS (SQR (lBC) - SQR (lAC)) <= SQR (lAB)))
        return i;

      A = B;
      B = clay_cage_config_get_point_coordinate (config,
                                                 options->cage_mode,
                                                 (i+1) % n_cage_vertices);
    }

  return -1;
}

static gboolean
clay_cage_tool_is_complete (ClayCageTool *ct)
{
  return (ct->tool_state > CAGE_STATE_CLOSING);
}

static void
clay_cage_tool_remove_last_handle (ClayCageTool *ct)
{
  ClayCageConfig *config = ct->config;

  clay_draw_tool_pause (CLAY_DRAW_TOOL (ct));

  clay_cage_config_remove_last_cage_point (config);

  clay_draw_tool_resume (CLAY_DRAW_TOOL (ct));
}

static void
clay_cage_tool_compute_coef (ClayCageTool *ct)
{
  ClayCageConfig *config = ct->config;
  ClayProgress   *progress;
  const Babl     *format;
  GeglNode       *gegl;
  GeglNode       *input;
  GeglNode       *output;
  GeglProcessor  *processor;
  GeglBuffer     *buffer;
  gdouble         value;

  progress = clay_progress_start (CLAY_PROGRESS (ct), FALSE,
                                  _("Computing Cage Coefficients"));

  g_clear_object (&ct->coef);

  format = babl_format_n (babl_type ("float"),
                          clay_cage_config_get_n_points (config) * 2);


  gegl = gegl_node_new ();

  input = gegl_node_new_child (gegl,
                               "operation", "clay:cage-coef-calc",
                               "config",    ct->config,
                               NULL);

  output = gegl_node_new_child (gegl,
                                "operation", "gegl:buffer-sink",
                                "buffer",    &buffer,
                                "format",    format,
                                NULL);

  gegl_node_link (input, output);

  processor = gegl_node_new_processor (output, NULL);

  while (gegl_processor_work (processor, &value))
    {
      if (progress)
        clay_progress_set_value (progress, value);
    }

  if (progress)
    clay_progress_end (progress);

  g_object_unref (processor);

  ct->coef = buffer;
  g_object_unref (gegl);

  ct->dirty_coef = FALSE;
}

static void
clay_cage_tool_create_render_node (ClayCageTool *ct)
{
  ClayCageOptions *options = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  GeglNode        *render;
  GeglNode        *input;
  GeglNode        *output;

  g_return_if_fail (ct->render_node == NULL);
  /* render_node is not supposed to be recreated */

  ct->render_node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (ct->render_node, "input");
  output = gegl_node_get_output_proxy (ct->render_node, "output");

  ct->coef_node = gegl_node_new_child (ct->render_node,
                                       "operation", "gegl:buffer-source",
                                       "buffer",    ct->coef,
                                       NULL);

  ct->cage_node = gegl_node_new_child (ct->render_node,
                                       "operation",        "clay:cage-transform",
                                       "config",           ct->config,
                                       "fill-plain-color", options->fill_plain_color,
                                       NULL);

  render = gegl_node_new_child (ct->render_node,
                                "operation", "gegl:map-absolute",
                                NULL);

  gegl_node_link (input, ct->cage_node);

  gegl_node_connect (ct->coef_node, "output",
                     ct->cage_node, "aux");

  gegl_node_link (input, render);

  gegl_node_connect (ct->cage_node, "output",
                     render,        "aux");

  gegl_node_link (render, output);

  clay_gegl_progress_connect (ct->cage_node, CLAY_PROGRESS (ct),
                              _("Cage Transform"));
}

static void
clay_cage_tool_render_node_update (ClayCageTool *ct)
{
  ClayCageOptions *options  = CLAY_CAGE_TOOL_GET_OPTIONS (ct);
  gboolean         fill;
  GeglBuffer      *buffer;

  gegl_node_get (ct->cage_node,
                 "fill-plain-color", &fill,
                 NULL);

  if (fill != options->fill_plain_color)
    {
      gegl_node_set (ct->cage_node,
                     "fill-plain-color", options->fill_plain_color,
                     NULL);
    }

  gegl_node_get (ct->coef_node,
                 "buffer", &buffer,
                 NULL);

  if (buffer != ct->coef)
    {
      gegl_node_set (ct->coef_node,
                     "buffer", ct->coef,
                     NULL);
    }

  if (buffer)
    g_object_unref (buffer);
}

static void
clay_cage_tool_create_filter (ClayCageTool *ct)
{
  if (! ct->render_node)
    clay_cage_tool_create_render_node (ct);

  ct->filter = clay_drawable_filter_new (CLAY_TOOL (ct)->drawables->data,
                                         _("Cage transform"),
                                         ct->render_node,
                                         CLAY_ICON_TOOL_CAGE);
  clay_drawable_filter_set_temporary (ct->filter, TRUE);
  clay_drawable_filter_set_region (ct->filter, CLAY_FILTER_REGION_DRAWABLE);

  g_signal_connect (ct->filter, "flush",
                    G_CALLBACK (clay_cage_tool_filter_flush),
                    ct);
}

static void
clay_cage_tool_filter_flush (ClayDrawableFilter *filter,
                             ClayTool           *tool)
{
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

static void
clay_cage_tool_filter_update (ClayCageTool *ct)
{
  ClayContainer *filters;

  /* Move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  filters =
    clay_drawable_get_filters (clay_drawable_filter_get_drawable (ct->filter));

  if (clay_container_have (filters, CLAY_OBJECT (ct->filter)))
  {
    gint end_index = clay_container_get_n_children (filters) - 1;
    gint index     = clay_container_get_child_index (filters,
                                                     CLAY_OBJECT (ct->filter));

    if (end_index > 0 && index != end_index)
      clay_container_reorder (filters, CLAY_OBJECT (ct->filter),
                              end_index);
  }

  clay_drawable_filter_apply (ct->filter, NULL);
}
