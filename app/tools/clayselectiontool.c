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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/claychannel.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-item.h"
#include "core/clayimage-undo.h"
#include "core/claypickable.h"
#include "core/clayundostack.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell-appearance.h"

#include "widgets/claywidgets-utils.h"

#include "clayeditselectiontool.h"
#include "clayselectiontool.h"
#include "clayselectionoptions.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


static void       clay_selection_tool_control             (ClayTool           *tool,
                                                           ClayToolAction      action,
                                                           ClayDisplay        *display);
static void       clay_selection_tool_modifier_key        (ClayTool           *tool,
                                                           GdkModifierType     key,
                                                           gboolean            press,
                                                           GdkModifierType     state,
                                                           ClayDisplay        *display);
static void       clay_selection_tool_oper_update         (ClayTool           *tool,
                                                           const ClayCoords   *coords,
                                                           GdkModifierType     state,
                                                           gboolean            proximity,
                                                           ClayDisplay        *display);
static void       clay_selection_tool_cursor_update       (ClayTool           *tool,
                                                           const ClayCoords   *coords,
                                                           GdkModifierType     state,
                                                           ClayDisplay        *display);

static gboolean   clay_selection_tool_real_have_selection (ClaySelectionTool  *sel_tool,
                                                           ClayDisplay        *display);

static void       clay_selection_tool_commit              (ClaySelectionTool  *sel_tool);
static void       clay_selection_tool_halt                (ClaySelectionTool  *sel_tool,
                                                           ClayDisplay        *display);

static gboolean   clay_selection_tool_check               (ClaySelectionTool  *sel_tool,
                                                           ClayDisplay        *display,
                                                           GError            **error);

static gboolean   clay_selection_tool_have_selection      (ClaySelectionTool  *sel_tool,
                                                           ClayDisplay        *display);


G_DEFINE_TYPE (ClaySelectionTool, clay_selection_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_selection_tool_parent_class


static void
clay_selection_tool_class_init (ClaySelectionToolClass *klass)
{
  ClayToolClass *tool_class = CLAY_TOOL_CLASS (klass);

  tool_class->control        = clay_selection_tool_control;
  tool_class->modifier_key   = clay_selection_tool_modifier_key;
  tool_class->key_press      = clay_edit_selection_tool_key_press;
  tool_class->oper_update    = clay_selection_tool_oper_update;
  tool_class->cursor_update  = clay_selection_tool_cursor_update;
  tool_class->is_destructive = FALSE;

  klass->have_selection      = clay_selection_tool_real_have_selection;
}

static void
clay_selection_tool_init (ClaySelectionTool *selection_tool)
{
  selection_tool->function             = SELECTION_SELECT;
  selection_tool->saved_operation      = CLAY_CHANNEL_OP_REPLACE;
  selection_tool->held_keys            = 0;

  selection_tool->saved_show_selection = FALSE;
  selection_tool->undo                 = NULL;
  selection_tool->redo                 = NULL;
  selection_tool->idle_id              = 0;

  selection_tool->allow_move           = TRUE;
}

static void
clay_selection_tool_control (ClayTool       *tool,
                             ClayToolAction  action,
                             ClayDisplay    *display)
{
  ClaySelectionTool *selection_tool = CLAY_SELECTION_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_selection_tool_halt (selection_tool, display);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_selection_tool_commit (selection_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_selection_tool_modifier_key (ClayTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  ClayDisplay     *display)
{
  ClaySelectionTool    *selection_tool = CLAY_SELECTION_TOOL (tool);
  ClaySelectionOptions *options        = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;

  extend_mask = clay_get_extend_selection_mask ();
  modify_mask = clay_get_modify_selection_mask ();

  if (key == extend_mask ||
      key == modify_mask ||
      key == GDK_MOD1_MASK)
    {
      ClayChannelOps button_op = options->operation;

      state &= extend_mask |
               modify_mask |
               GDK_MOD1_MASK;

      selection_tool->held_keys &= state; /* clear the flags for released keys */

      if (press)
        {
          selection_tool->held_keys |= key;

          if (key == state ||
              /* ClayPolygonSelectTool may mask-out part of the state, which
               * can cause the wrong mode to be restored on release if we don't
               * init saved_operation here.
               *
               * see issue #4992.
               */
              ! state)
            {
              /*  first modifier pressed  */

              selection_tool->saved_operation = options->operation;
            }
        }
      else
        {
          if (! state)
            {
              /*  last modifier released  */
              button_op = selection_tool->saved_operation;
            }
        }

      if ((state & GDK_MOD1_MASK) ||
         ((state & selection_tool->held_keys) == 0))
        {
          /*  if alt is down, pretend that neither
           *  shift nor control are down
           *
           *  The test for at least one flag being set in both state and
           *  held keys is to prevent the problem described in issue #1589
           *  where the modifier key is pressed to restrict the movement of a
           *  floating layer. When the left mouse button is released this
           *  function is called with a release event for the extend key and the
           *  state parameter set to 4 to indicate that the modifier key is
           *  pressed. Without this extra test the operation is set based on the
           *  the setting of state (to CLAY_CHANNEL_OP_SUBTRACT). The next call
           *  of this function is then for a press of the modifier key and,
           *  because state = key = 4 selection_tool->saved_operation is set,
           *  wrongly, to CLAY_CHANNEL_OP_SUBTRACT, overwriting the intended
           *  setting
           */
          button_op = selection_tool->saved_operation;
        }
      else if (state & (extend_mask |
                        modify_mask))
        {
          /*  else get the operation from the modifier state, but only
           *  if there is actually a modifier pressed, so we don't
           *  override the "last modifier released" assignment above
           */
          button_op = clay_modifiers_to_channel_op (state &
                                                    selection_tool->held_keys);
        }

      if (button_op != options->operation)
        {
          g_object_set (options, "operation", button_op, NULL);
        }
    }
}

static void
clay_selection_tool_oper_update (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 ClayDisplay      *display)
{
  ClaySelectionTool    *selection_tool = CLAY_SELECTION_TOOL (tool);
  ClaySelectionOptions *options        = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayImage            *image;
  GList                *drawables;
  ClayLayer            *layer;
  ClayLayer            *floating_sel;
  GdkModifierType       extend_mask;
  GdkModifierType       modify_mask;
  gboolean              have_selection;
  gboolean              move_layer        = FALSE;
  gboolean              move_floating_sel = FALSE;

  image        = clay_display_get_image (display);
  drawables    = clay_image_get_selected_drawables (image);
  layer        = clay_image_pick_layer (image, coords->x, coords->y, NULL);
  floating_sel = clay_image_get_floating_selection (image);

  extend_mask = clay_get_extend_selection_mask ();
  modify_mask = clay_get_modify_selection_mask ();

  have_selection = clay_selection_tool_have_selection (selection_tool, display);

  if (drawables)
    {
      if (floating_sel)
        {
          if (layer == floating_sel)
            move_floating_sel = TRUE;
        }
      else if (have_selection)
        {
          GList *iter;

          for (iter = drawables; iter; iter = iter->next)
            {
              if (clay_item_mask_intersect (CLAY_ITEM (iter->data),
                                            NULL, NULL, NULL, NULL))
                {
                  move_layer = TRUE;
                  break;
                }
            }
        }

      g_list_free (drawables);
    }

  selection_tool->function = SELECTION_SELECT;

  if (selection_tool->allow_move &&
      (state & GDK_MOD1_MASK) && (state & modify_mask) && move_layer)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if (selection_tool->allow_move &&
           (state & GDK_MOD1_MASK) && (state & extend_mask) && move_layer)
    {
      /* move a copy of the selection */
      selection_tool->function = SELECTION_MOVE_COPY;
    }
  else if (selection_tool->allow_move &&
           (state & GDK_MOD1_MASK) && have_selection)
    {
      /* move the selection mask */
      selection_tool->function = SELECTION_MOVE_MASK;
    }
  else if (selection_tool->allow_move &&
           ! (state & (extend_mask | modify_mask)) &&
           move_floating_sel)
    {
      /* move the selection */
      selection_tool->function = SELECTION_MOVE;
    }
  else if ((state & modify_mask) || (state & extend_mask))
    {
      /* select */
      selection_tool->function = SELECTION_SELECT;
    }
  else if (floating_sel)
    {
      /* anchor the selection */
      selection_tool->function = SELECTION_ANCHOR;
    }

  clay_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar     *status      = NULL;
      gboolean         free_status = FALSE;
      GdkModifierType  modifiers   = (extend_mask | modify_mask);

      if (have_selection)
        modifiers |= GDK_MOD1_MASK;

      switch (selection_tool->function)
        {
        case SELECTION_SELECT:
          switch (options->operation)
            {
            case CLAY_CHANNEL_OP_REPLACE:
              if (have_selection)
                {
                  status = clay_suggest_modifiers (_("Click-Drag to replace the "
                                                     "current selection"),
                                                   modifiers & ~state,
                                                   NULL, NULL, NULL);
                  free_status = TRUE;
                }
              else
                {
                  status = _("Click-Drag to create a new selection");
                }
              break;

            case CLAY_CHANNEL_OP_ADD:
              status = clay_suggest_modifiers (_("Click-Drag to add to the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | extend_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case CLAY_CHANNEL_OP_SUBTRACT:
              status = clay_suggest_modifiers (_("Click-Drag to subtract from the "
                                                 "current selection"),
                                               modifiers
                                               & ~(state | modify_mask),
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;

            case CLAY_CHANNEL_OP_INTERSECT:
              status = clay_suggest_modifiers (_("Click-Drag to intersect with "
                                                 "the current selection"),
                                               modifiers & ~state,
                                               NULL, NULL, NULL);
              free_status = TRUE;
              break;
            }
          break;

        case SELECTION_MOVE_MASK:
          status = clay_suggest_modifiers (_("Click-Drag to move the "
                                             "selection mask"),
                                           modifiers & ~state,
                                           NULL, NULL, NULL);
          free_status = TRUE;
          break;

        case SELECTION_MOVE:
          status = _("Click-Drag to move the selected pixels");
          break;

        case SELECTION_MOVE_COPY:
          status = _("Click-Drag to move a copy of the selected pixels");
          break;

        case SELECTION_ANCHOR:
          status = _("Click to anchor the floating selection");
          break;

        default:
          g_return_if_reached ();
        }

      if (status)
        clay_tool_push_status (tool, display, "%s", status);

      if (free_status)
        g_free ((gchar *) status);
    }
}

static void
clay_selection_tool_cursor_update (ClayTool         *tool,
                                   const ClayCoords *coords,
                                   GdkModifierType   state,
                                   ClayDisplay      *display)
{
  ClaySelectionTool    *selection_tool = CLAY_SELECTION_TOOL (tool);
  ClaySelectionOptions *options;
  ClayToolCursorType    tool_cursor;
  ClayCursorModifier    modifier;

  options = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);

  tool_cursor = clay_tool_control_get_tool_cursor (tool->control);
  modifier    = CLAY_CURSOR_MODIFIER_NONE;

  switch (selection_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case CLAY_CHANNEL_OP_REPLACE:
          break;
        case CLAY_CHANNEL_OP_ADD:
          modifier = CLAY_CURSOR_MODIFIER_PLUS;
          break;
        case CLAY_CHANNEL_OP_SUBTRACT:
          modifier = CLAY_CURSOR_MODIFIER_MINUS;
          break;
        case CLAY_CHANNEL_OP_INTERSECT:
          modifier = CLAY_CURSOR_MODIFIER_INTERSECT;
          break;
        }
      break;

    case SELECTION_MOVE_MASK:
      modifier = CLAY_CURSOR_MODIFIER_MOVE;
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      tool_cursor = CLAY_TOOL_CURSOR_MOVE;
      break;

    case SELECTION_ANCHOR:
      modifier = CLAY_CURSOR_MODIFIER_ANCHOR;
      break;
    }

  /*  our subclass might have set a BAD modifier, in which case we leave it
   *  there, since it's more important than what we have to say.
   */
  if (clay_tool_control_get_cursor_modifier (tool->control) ==
      CLAY_CURSOR_MODIFIER_BAD                              ||
      ! clay_selection_tool_check (selection_tool, display, NULL))
    {
      modifier = CLAY_CURSOR_MODIFIER_BAD;
    }

  clay_tool_set_cursor (tool, display,
                        clay_tool_control_get_cursor (tool->control),
                        tool_cursor,
                        modifier);
}

static gboolean
clay_selection_tool_real_have_selection (ClaySelectionTool *sel_tool,
                                         ClayDisplay       *display)
{
  ClayImage   *image     = clay_display_get_image (display);
  ClayChannel *selection = clay_image_get_mask (image);

  return ! clay_channel_is_empty (selection);
}

static void
clay_selection_tool_commit (ClaySelectionTool *sel_tool)
{
  /* make sure clay_selection_tool_halt() doesn't undo the change, if any */
  g_clear_weak_pointer (&sel_tool->undo);
}

static void
clay_selection_tool_halt (ClaySelectionTool *sel_tool,
                          ClayDisplay       *display)
{
  g_warn_if_fail (sel_tool->change_count == 0);

  if (display)
    {
      ClayTool      *tool       = CLAY_TOOL (sel_tool);
      ClayImage     *image      = clay_display_get_image (display);
      ClayUndoStack *undo_stack = clay_image_get_undo_stack (image);
      ClayUndo      *undo       = clay_undo_stack_peek (undo_stack);

      /* if we have an existing selection in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      if (undo && sel_tool->undo == undo)
        {
          /* prevent this change from halting the tool */
          clay_tool_control_push_preserve (tool->control, TRUE);

          clay_image_undo (image);
          clay_image_flush (image);

          clay_tool_control_pop_preserve (tool->control);
        }

      /* reset the automatic undo/redo mechanism */
      g_clear_weak_pointer (&sel_tool->undo);
      g_clear_weak_pointer (&sel_tool->redo);
    }
}

static gboolean
clay_selection_tool_check (ClaySelectionTool  *sel_tool,
                           ClayDisplay        *display,
                           GError            **error)
{
  ClaySelectionOptions *options   = CLAY_SELECTION_TOOL_GET_OPTIONS (sel_tool);
  ClayImage            *image     = clay_display_get_image (display);

  switch (sel_tool->function)
    {
    case SELECTION_SELECT:
      switch (options->operation)
        {
        case CLAY_CHANNEL_OP_ADD:
        case CLAY_CHANNEL_OP_REPLACE:
          break;

        case CLAY_CHANNEL_OP_SUBTRACT:
          if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot subtract from an empty selection."));

              return FALSE;
            }
          break;

        case CLAY_CHANNEL_OP_INTERSECT:
          if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                                  NULL, NULL, NULL, NULL))
            {
              g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot intersect with an empty selection."));

              return FALSE;
            }
          break;
        }
      break;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
        {
          GList    *drawables   = clay_image_get_selected_drawables (image);
          ClayItem *locked_item = NULL;
          GList    *iter;

          for (iter = drawables; iter; iter = iter->next)
            {
              if (clay_viewable_get_children (iter->data))
                {
                  g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                               _("Cannot modify the pixels of layer groups."));

                  g_list_free (drawables);
                  return FALSE;
                }
              else if (clay_item_is_content_locked (iter->data, &locked_item))
                {
                  g_set_error (error, CLAY_ERROR, CLAY_FAILED,
                               _("A selected item's pixels are locked."));

                  if (error)
                    clay_tools_blink_lock_box (display->clay, locked_item);

                  g_list_free (drawables);
                  return FALSE;
                }
            }

          g_list_free (drawables);
        }
      break;

    default:
      break;
    }

  return TRUE;
}

static gboolean
clay_selection_tool_have_selection (ClaySelectionTool *sel_tool,
                                    ClayDisplay       *display)
{
  return CLAY_SELECTION_TOOL_GET_CLASS (sel_tool)->have_selection (sel_tool,
                                                                   display);
}


/*  public functions  */

gboolean
clay_selection_tool_start_edit (ClaySelectionTool *sel_tool,
                                ClayDisplay       *display,
                                const ClayCoords  *coords)
{
  ClayTool             *tool;
  ClaySelectionOptions *options;
  ClayDisplayShell     *shell;
  GError               *error = NULL;

  g_return_val_if_fail (CLAY_IS_SELECTION_TOOL (sel_tool), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);

  tool    = CLAY_TOOL (sel_tool);
  options = CLAY_SELECTION_TOOL_GET_OPTIONS (sel_tool);
  shell   = clay_display_get_shell (display);

  g_return_val_if_fail (clay_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  if (! clay_selection_tool_check (sel_tool, display, &error))
    {
      clay_tool_message_literal (tool, display, error->message);

      clay_tools_show_tool_options (display->clay);
      clay_widget_blink (options->mode_box);

      g_clear_error (&error);

      return TRUE;
    }

  switch (sel_tool->function)
    {
    case SELECTION_MOVE_MASK:
      clay_edit_selection_tool_start (tool, display, coords,
                                      CLAY_TRANSLATE_MODE_MASK, FALSE);
      return TRUE;

    case SELECTION_MOVE:
    case SELECTION_MOVE_COPY:
      {
        ClayTranslateMode edit_mode;

        clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);

        if (sel_tool->function == SELECTION_MOVE)
          edit_mode = CLAY_TRANSLATE_MODE_MASK_TO_LAYER;
        else
          edit_mode = CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER;

        clay_edit_selection_tool_start (tool, display, coords,
                                        edit_mode, FALSE);

        /* Turn off selection for performance if moved as floating selection */
        clay_display_shell_set_show_selection (shell, FALSE);

        return TRUE;
      }

    default:
      break;
    }

  return FALSE;
}

static gboolean
clay_selection_tool_idle (ClaySelectionTool *sel_tool)
{
  ClayTool         *tool  = CLAY_TOOL (sel_tool);
  ClayDisplayShell *shell = clay_display_get_shell (tool->display);

  clay_display_shell_set_show_selection (shell, FALSE);

  sel_tool->idle_id = 0;

  return G_SOURCE_REMOVE;
}

void
clay_selection_tool_start_change (ClaySelectionTool *sel_tool,
                                  gboolean           create,
                                  ClayChannelOps     operation)
{
  ClayTool         *tool;
  ClayDisplayShell *shell;
  ClayImage        *image;
  ClayUndoStack    *undo_stack;

  g_return_if_fail (CLAY_IS_SELECTION_TOOL (sel_tool));

  tool = CLAY_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (sel_tool->change_count++ > 0)
    return;

  shell      = clay_display_get_shell (tool->display);
  image      = clay_display_get_image (tool->display);
  undo_stack = clay_image_get_undo_stack (image);

  sel_tool->saved_show_selection =
    clay_display_shell_get_show_selection (shell);

  if (create)
    {
      g_clear_weak_pointer (&sel_tool->undo);
    }
  else
    {
      ClayUndoStack *redo_stack = clay_image_get_redo_stack (image);
      ClayUndo      *undo;

      undo = clay_undo_stack_peek (undo_stack);

      if (undo && undo == sel_tool->undo)
        {
          /* prevent this change from halting the tool */
          clay_tool_control_push_preserve (tool->control, TRUE);

          clay_image_undo (image);

          clay_tool_control_pop_preserve (tool->control);

          g_clear_weak_pointer (&sel_tool->undo);

          /* we will need to redo if the user cancels or executes */
          g_set_weak_pointer (&sel_tool->redo,
                              clay_undo_stack_peek (redo_stack));
        }

      /* if the operation is "Replace", turn off the marching ants,
       * because they are confusing ...
       */
      if (operation == CLAY_CHANNEL_OP_REPLACE)
        {
          /* ... however, do this in an idle function, to avoid unnecessarily
           * restarting the selection if we don't visit the main loop between
           * the start_change() and end_change() calls.
           */
          sel_tool->idle_id = g_idle_add_full (
            G_PRIORITY_HIGH_IDLE,
            (GSourceFunc) clay_selection_tool_idle,
            sel_tool, NULL);
        }
    }

  g_set_weak_pointer (&sel_tool->undo,
                      clay_undo_stack_peek (undo_stack));
}

void
clay_selection_tool_end_change (ClaySelectionTool *sel_tool,
                                gboolean           cancel)
{
  ClayTool         *tool;
  ClayDisplayShell *shell;
  ClayImage        *image;
  ClayUndoStack    *undo_stack;

  g_return_if_fail (CLAY_IS_SELECTION_TOOL (sel_tool));
  g_return_if_fail (sel_tool->change_count > 0);

  tool = CLAY_TOOL (sel_tool);

  g_return_if_fail (tool->display != NULL);

  if (--sel_tool->change_count > 0)
    return;

  shell      = clay_display_get_shell (tool->display);
  image      = clay_display_get_image (tool->display);
  undo_stack = clay_image_get_undo_stack (image);

  if (cancel)
    {
      ClayUndoStack *redo_stack = clay_image_get_redo_stack (image);
      ClayUndo      *redo       = clay_undo_stack_peek (redo_stack);

      if (redo && redo == sel_tool->redo)
        {
          /* prevent this from halting the tool */
          clay_tool_control_push_preserve (tool->control, TRUE);

          clay_image_redo (image);

          clay_tool_control_pop_preserve (tool->control);

          g_set_weak_pointer (&sel_tool->undo,
                              clay_undo_stack_peek (undo_stack));
        }
      else
        {
          g_clear_weak_pointer (&sel_tool->undo);
        }
    }
  else
    {
      ClayUndo *undo = clay_undo_stack_peek (undo_stack);

      /* save the undo that we got when executing, but only if
       * we actually selected something
       */
      if (undo && undo != sel_tool->undo)
        g_set_weak_pointer (&sel_tool->undo, undo);
      else
        g_clear_weak_pointer (&sel_tool->undo);
    }

  g_clear_weak_pointer (&sel_tool->redo);

  if (sel_tool->idle_id)
    {
      g_source_remove (sel_tool->idle_id);
      sel_tool->idle_id = 0;
    }
  else
    {
      clay_display_shell_set_show_selection (shell,
                                             sel_tool->saved_show_selection);
    }

  clay_image_flush (image);
}
