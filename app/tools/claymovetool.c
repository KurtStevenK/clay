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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-cairo.h"
#include "core/clay-utils.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-item.h"
#include "core/claylayer.h"
#include "core/clayimage-undo.h"
#include "core/claylayermask.h"
#include "core/claylayer-floating-selection.h"
#include "core/clayundostack.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/claydisplayshell-selection.h"
#include "display/claydisplayshell-transform.h"

#include "clayeditselectiontool.h"
#include "clayguidetool.h"
#include "claymoveoptions.h"
#include "claymovetool.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_move_tool_finalize       (GObject               *object);

static void   clay_move_tool_button_press   (ClayTool              *tool,
                                             const ClayCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             ClayButtonPressType    press_type,
                                             ClayDisplay           *display);
static void   clay_move_tool_button_release (ClayTool              *tool,
                                             const ClayCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             ClayButtonReleaseType  release_type,
                                             ClayDisplay           *display);
static gboolean clay_move_tool_key_press    (ClayTool              *tool,
                                             GdkEventKey           *kevent,
                                             ClayDisplay           *display);
static void   clay_move_tool_modifier_key   (ClayTool              *tool,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state,
                                             ClayDisplay           *display);
static void   clay_move_tool_oper_update    (ClayTool              *tool,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity,
                                             ClayDisplay           *display);
static void   clay_move_tool_cursor_update  (ClayTool              *tool,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             ClayDisplay           *display);

static void   clay_move_tool_draw           (ClayDrawTool          *draw_tool);


G_DEFINE_TYPE (ClayMoveTool, clay_move_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_move_tool_parent_class


void
clay_move_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_MOVE_TOOL,
                CLAY_TYPE_MOVE_OPTIONS,
                clay_move_options_gui,
                0,
                "clay-move-tool",
                C_("tool", "Move"),
                _("Move Tool: Move layers, selections, and other objects"),
                N_("_Move"), "M",
                NULL, CLAY_HELP_TOOL_MOVE,
                CLAY_ICON_TOOL_MOVE,
                data);
}

static void
clay_move_tool_class_init (ClayMoveToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  ClayToolClass     *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);

  object_class->finalize     = clay_move_tool_finalize;

  tool_class->button_press   = clay_move_tool_button_press;
  tool_class->button_release = clay_move_tool_button_release;
  tool_class->key_press      = clay_move_tool_key_press;
  tool_class->modifier_key   = clay_move_tool_modifier_key;
  tool_class->oper_update    = clay_move_tool_oper_update;
  tool_class->cursor_update  = clay_move_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  draw_tool_class->draw      = clay_move_tool_draw;
}

static void
clay_move_tool_init (ClayMoveTool *move_tool)
{
  ClayTool *tool = CLAY_TOOL (move_tool);

  clay_tool_control_set_snap_to            (tool->control, FALSE);
  clay_tool_control_set_handle_empty_image (tool->control, TRUE);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_MOVE);

  move_tool->floating_layer     = NULL;
  move_tool->guides             = NULL;

  move_tool->saved_type         = CLAY_TRANSFORM_TYPE_LAYER;

  move_tool->old_selected_layers = NULL;
  move_tool->old_selected_paths  = NULL;
}

static void
clay_move_tool_finalize (GObject *object)
{
  ClayMoveTool *move = CLAY_MOVE_TOOL (object);

  g_clear_pointer (&move->guides,              g_list_free);
  g_clear_pointer (&move->old_selected_layers, g_list_free);
  g_clear_pointer (&move->old_selected_paths,  g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_move_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayMoveTool      *move           = CLAY_MOVE_TOOL (tool);
  ClayMoveOptions   *options        = CLAY_MOVE_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell  *shell          = clay_display_get_shell (display);
  ClayImage         *image          = clay_display_get_image (display);
  ClayItem          *active_item    = NULL;
  GList             *selected_items = NULL;
  GList             *iter;
  ClayTranslateMode  translate_mode = CLAY_TRANSLATE_MODE_MASK;
  const gchar       *null_message   = NULL;
  const gchar       *locked_message = NULL;
  ClayItem          *locked_item    = NULL;

  tool->display = display;

  move->floating_layer = NULL;

  g_clear_pointer (&move->guides, g_list_free);

  if (! options->move_current)
    {
      const gint snap_distance = display->config->snap_distance;

      if (options->move_type == CLAY_TRANSFORM_TYPE_PATH)
        {
          ClayPath *path;

          path = clay_image_pick_path (image,
                                       coords->x, coords->y,
                                       FUNSCALEX (shell, snap_distance),
                                       FUNSCALEY (shell, snap_distance));
          if (path)
            {
              GList *new_selected_paths = g_list_prepend (NULL, path);

              move->old_selected_paths =
                g_list_copy (clay_image_get_selected_paths (image));

              clay_image_set_selected_paths (image, new_selected_paths);
              g_list_free (new_selected_paths);
            }
          else
            {
              /*  no path picked  */
              return;
            }
        }
      else if (options->move_type == CLAY_TRANSFORM_TYPE_LAYER)
        {
          GList     *guides;
          ClayLayer *layer;

          if (clay_display_shell_get_show_guides (shell) &&
              (guides = clay_image_pick_guides (image,
                                                coords->x, coords->y,
                                                FUNSCALEX (shell, snap_distance),
                                                FUNSCALEY (shell, snap_distance))))
            {
              move->guides = guides;

              clay_guide_tool_start_edit_many (tool, display, guides);

              return;
            }
          else if ((layer = clay_image_pick_layer (image,
                                                   coords->x,
                                                   coords->y,
                                                   NULL)))
            {
              if (clay_image_get_floating_selection (image) &&
                  ! clay_layer_is_floating_sel (layer))
                {
                  /*  If there is a floating selection, and this aint it,
                   *  use the move tool to anchor it.
                   */
                  move->floating_layer =
                    clay_image_get_floating_selection (image);

                  clay_tool_control_activate (tool->control);

                  return;
                }
              else
                {
                  GList *new_selected_layers = g_list_prepend (NULL, layer);

                  move->old_selected_layers = g_list_copy (clay_image_get_selected_layers (image));

                  clay_image_set_selected_layers (image, new_selected_layers);
                  g_list_free (new_selected_layers);
                }
            }
          else
            {
              /*  no guide and no layer picked  */

              return;
            }
        }
    }

  switch (options->move_type)
    {
    case CLAY_TRANSFORM_TYPE_PATH:
      {
        selected_items = clay_image_get_selected_paths (image);
        selected_items = g_list_copy (selected_items);

        translate_mode = CLAY_TRANSLATE_MODE_PATH;

        if (! selected_items)
          {
            null_message = _("There are no paths to move.");
          }
        else
          {
            gint n_items = 0;

            for (iter = selected_items; iter; iter = iter->next)
              {
                if (! clay_item_is_position_locked (iter->data, &locked_item))
                  n_items++;
              }

            if (n_items == 0)
              locked_message = _("All selected path's position are locked.");
          }
      }
      break;

    case CLAY_TRANSFORM_TYPE_SELECTION:
      {
        active_item = CLAY_ITEM (clay_image_get_mask (image));

        if (clay_channel_is_empty (CLAY_CHANNEL (active_item)))
          active_item = NULL;

        translate_mode = CLAY_TRANSLATE_MODE_MASK;

        if (! active_item)
          {
            /* cannot happen, don't translate this message */
            null_message  = "There is no selection to move.";
          }
        else if (clay_item_is_position_locked (active_item, &locked_item))
          {
            locked_message = "The selection's position is locked.";
          }
      }
      break;

    case CLAY_TRANSFORM_TYPE_LAYER:
      {
        selected_items = clay_image_get_selected_drawables (image);

        if (! selected_items)
          {
            null_message = _("There is no layer to move.");
          }
        else if (CLAY_IS_LAYER_MASK (selected_items->data))
          {
            g_return_if_fail (g_list_length (selected_items) == 1);

            translate_mode = CLAY_TRANSLATE_MODE_LAYER_MASK;

            if (clay_item_is_position_locked (selected_items->data, &locked_item))
              locked_message = _("The selected layer's position is locked.");
            else if (clay_item_is_content_locked (selected_items->data, NULL))
              locked_message = _("The selected layer's pixels are locked.");
          }
        else if (CLAY_IS_CHANNEL (selected_items->data))
          {
            translate_mode = CLAY_TRANSLATE_MODE_CHANNEL;

            for (iter = selected_items; iter; iter = iter->next)
              if (clay_item_is_position_locked (iter->data, &locked_item) ||
                  clay_item_is_content_locked (iter->data, &locked_item))
              locked_message = _("A selected channel's position or pixels are locked.");
          }
        else
          {
            translate_mode = CLAY_TRANSLATE_MODE_LAYER;

            for (iter = selected_items; iter; iter = iter->next)
              if (clay_item_is_position_locked (iter->data, &locked_item))
              locked_message = _("A selected layer's position is locked.");
          }
      }
      break;

    case CLAY_TRANSFORM_TYPE_IMAGE:
      g_return_if_reached ();
    }

  if (! active_item && ! selected_items)
    {
      clay_tool_message_literal (tool, display, null_message);
      clay_tools_show_tool_options (display->clay);
      clay_widget_blink (options->type_box);
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
      return;
    }
  else if (locked_message)
    {
      clay_tool_message_literal (tool, display, locked_message);

      if (locked_item == NULL)
        locked_item = active_item ? active_item : selected_items->data;

      clay_tools_blink_lock_box (display->clay, locked_item);
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
      g_list_free (selected_items);
      return;
    }

  clay_tool_control_activate (tool->control);

  clay_edit_selection_tool_start (tool, display, coords,
                                  translate_mode,
                                  TRUE);
  g_list_free (selected_items);
}

static void
clay_move_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayMoveTool  *move   = CLAY_MOVE_TOOL (tool);
  ClayGuiConfig *config = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage     *image  = clay_display_get_image (display);
  gboolean       flush  = FALSE;

  clay_tool_control_halt (tool->control);

  if (! config->move_tool_changes_active ||
      (release_type == CLAY_BUTTON_RELEASE_CANCEL))
    {
      if (move->old_selected_layers)
        {
          clay_image_set_selected_layers (image, move->old_selected_layers);
          g_clear_pointer (&move->old_selected_layers, g_list_free);

          flush = TRUE;
        }

      if (move->old_selected_paths)
        {
          clay_image_set_selected_paths (image, move->old_selected_paths);
          g_clear_pointer (&move->old_selected_paths, g_list_free);

          flush = TRUE;
        }
    }

  if (release_type != CLAY_BUTTON_RELEASE_CANCEL)
    {
      if (move->floating_layer)
        {
          floating_sel_anchor (move->floating_layer);

          flush = TRUE;
        }
    }

  if (flush)
    clay_image_flush (image);
}

static gboolean
clay_move_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  ClayMoveOptions *options = CLAY_MOVE_TOOL_GET_OPTIONS (tool);

  return clay_edit_selection_tool_translate (tool, kevent,
                                             options->move_type,
                                             display,
                                             &options->type_box);
}

static void
clay_move_tool_modifier_key (ClayTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             ClayDisplay     *display)
{
  ClayMoveTool    *move    = CLAY_MOVE_TOOL (tool);
  ClayMoveOptions *options = CLAY_MOVE_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_extend_selection_mask ())
    {
      g_object_set (options, "move-current", ! options->move_current, NULL);
    }
  else if (key == GDK_MOD1_MASK ||
           key == clay_get_toggle_behavior_mask ())
    {
      ClayTransformType button_type;

      button_type = options->move_type;

      if (press)
        {
          if (key == (state & (GDK_MOD1_MASK |
                               clay_get_toggle_behavior_mask ())))
            {
              /*  first modifier pressed  */

              move->saved_type = options->move_type;
            }
        }
      else
        {
          if (! (state & (GDK_MOD1_MASK |
                          clay_get_toggle_behavior_mask ())))
            {
              /*  last modifier released  */

              button_type = move->saved_type;
            }
        }

      if (state & GDK_MOD1_MASK)
        {
          button_type = CLAY_TRANSFORM_TYPE_SELECTION;
        }
      else if (state & clay_get_toggle_behavior_mask ())
        {
          button_type = CLAY_TRANSFORM_TYPE_PATH;
        }

      if (button_type != options->move_type)
        {
          g_object_set (options, "move-type", button_type, NULL);
        }
    }
}

static void
clay_move_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayMoveTool     *move    = CLAY_MOVE_TOOL (tool);
  ClayMoveOptions  *options = CLAY_MOVE_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell *shell   = clay_display_get_shell (display);
  ClayImage        *image   = clay_display_get_image (display);
  GList            *guides  = NULL;

  if (options->move_type == CLAY_TRANSFORM_TYPE_LAYER &&
      ! options->move_current                         &&
      clay_display_shell_get_show_guides (shell)      &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guides = clay_image_pick_guides (image, coords->x, coords->y,
                                       FUNSCALEX (shell, snap_distance),
                                       FUNSCALEY (shell, snap_distance));
    }

  if (clay_g_list_compare (guides, move->guides))
    {
      ClayDrawTool *draw_tool = CLAY_DRAW_TOOL (tool);

      clay_draw_tool_pause (draw_tool);

      if (clay_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        clay_draw_tool_stop (draw_tool);

      g_clear_pointer (&move->guides, g_list_free);

      move->guides = guides;

      if (! clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_start (draw_tool, display);

      clay_draw_tool_resume (draw_tool);
    }
  else
    {
      g_list_free (guides);
    }
}

static void
clay_move_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayMoveOptions    *options       = CLAY_MOVE_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell   *shell         = clay_display_get_shell (display);
  ClayImage          *image         = clay_display_get_image (display);
  ClayCursorType      cursor        = CLAY_CURSOR_MOUSE;
  ClayToolCursorType  tool_cursor   = CLAY_TOOL_CURSOR_MOVE;
  ClayCursorModifier  modifier      = CLAY_CURSOR_MODIFIER_NONE;
  gint                snap_distance = display->config->snap_distance;

  if (options->move_type == CLAY_TRANSFORM_TYPE_PATH)
    {
      tool_cursor = CLAY_TOOL_CURSOR_PATHS;
      modifier    = CLAY_CURSOR_MODIFIER_MOVE;

      if (options->move_current)
        {
          GList *selected = clay_image_get_selected_paths (image);
          GList *iter;
          gint   n_items = 0;

          for (iter = selected; iter; iter = iter->next)
            {
              if (! clay_item_is_position_locked (iter->data, NULL))
                n_items++;
            }

          if (n_items == 0)
            modifier = CLAY_CURSOR_MODIFIER_BAD;
        }
      else
        {
          if (clay_image_pick_path (image,
                                    coords->x, coords->y,
                                    FUNSCALEX (shell, snap_distance),
                                    FUNSCALEY (shell, snap_distance)))
            {
              tool_cursor = CLAY_TOOL_CURSOR_HAND;
            }
          else
            {
              modifier = CLAY_CURSOR_MODIFIER_BAD;
            }
        }
    }
  else if (options->move_type == CLAY_TRANSFORM_TYPE_SELECTION)
    {
      tool_cursor = CLAY_TOOL_CURSOR_RECT_SELECT;
      modifier    = CLAY_CURSOR_MODIFIER_MOVE;

      if (clay_channel_is_empty (clay_image_get_mask (image)))
        modifier = CLAY_CURSOR_MODIFIER_BAD;
    }
  else if (options->move_current)
    {
      GList *items   = clay_image_get_selected_drawables (image);
      GList *iter;
      gint   n_items = 0;

      for (iter = items; iter; iter = iter->next)
        if (! clay_item_is_position_locked (iter->data, NULL))
          n_items++;
      if (n_items == 0)
        modifier = CLAY_CURSOR_MODIFIER_BAD;

      g_list_free (items);
    }
  else
    {
      ClayLayer  *layer;

      if (clay_display_shell_get_show_guides (shell) &&
          clay_image_pick_guide (image, coords->x, coords->y,
                                 FUNSCALEX (shell, snap_distance),
                                 FUNSCALEY (shell, snap_distance)))
        {
          tool_cursor = CLAY_TOOL_CURSOR_HAND;
          modifier    = CLAY_CURSOR_MODIFIER_MOVE;
        }
      else if ((layer = clay_image_pick_layer (image,
                                               coords->x, coords->y,
                                               NULL)))
        {
          /*  if there is a floating selection, and this aint it...  */
          if (clay_image_get_floating_selection (image) &&
              ! clay_layer_is_floating_sel (layer))
            {
              tool_cursor = CLAY_TOOL_CURSOR_MOVE;
              modifier    = CLAY_CURSOR_MODIFIER_ANCHOR;
            }
          else if (clay_item_is_position_locked (CLAY_ITEM (layer), NULL))
            {
              modifier = CLAY_CURSOR_MODIFIER_BAD;
            }
          else if (! g_list_find (clay_image_get_selected_layers (image), layer))
            {
              tool_cursor = CLAY_TOOL_CURSOR_HAND;
              modifier    = CLAY_CURSOR_MODIFIER_MOVE;
            }
        }
      else
        {
          modifier = CLAY_CURSOR_MODIFIER_BAD;
        }
    }

  clay_tool_control_set_cursor          (tool->control, cursor);
  clay_tool_control_set_tool_cursor     (tool->control, tool_cursor);
  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_move_tool_draw (ClayDrawTool *draw_tool)
{
  ClayMoveTool *move = CLAY_MOVE_TOOL (draw_tool);
  GList        *iter;

  for (iter = move->guides; iter; iter = g_list_next (iter))
    {
      ClayGuide      *guide = iter->data;
      ClayCanvasItem *item;
      ClayGuideStyle  style;

      style = clay_guide_get_style (guide);

      item = clay_draw_tool_add_guide (draw_tool,
                                       clay_guide_get_orientation (guide),
                                       clay_guide_get_position (guide),
                                       style);
      clay_canvas_item_set_highlight (item, TRUE);
    }
}
