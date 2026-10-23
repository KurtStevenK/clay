/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#include "tools-types.h"

#include "claytoolcontrol.h"


static void clay_tool_control_finalize (GObject *object);


G_DEFINE_TYPE (ClayToolControl, clay_tool_control, CLAY_TYPE_OBJECT)

#define parent_class clay_tool_control_parent_class


static void
clay_tool_control_class_init (ClayToolControlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_tool_control_finalize;
}

static void
clay_tool_control_init (ClayToolControl *control)
{
  control->active                 = FALSE;
  control->paused_count           = 0;

  control->preserve               = TRUE;
  control->scroll_lock            = FALSE;
  control->handle_empty_image     = FALSE;

  control->dirty_mask             = CLAY_DIRTY_NONE;
  control->dirty_action           = CLAY_TOOL_ACTION_HALT;
  control->motion_mode            = CLAY_MOTION_MODE_COMPRESS;

  control->auto_snap_to           = TRUE;
  control->snap_offset_x          = 0;
  control->snap_offset_y          = 0;
  control->snap_width             = 0;
  control->snap_height            = 0;

  control->precision              = CLAY_CURSOR_PRECISION_PIXEL_CENTER;

  control->toggled                = FALSE;

  control->wants_click            = FALSE;
  control->wants_double_click     = FALSE;
  control->wants_triple_click     = FALSE;
  control->wants_all_key_events   = FALSE;

  control->active_modifiers       = CLAY_TOOL_ACTIVE_MODIFIERS_OFF;

  control->cursor                 = CLAY_CURSOR_MOUSE;
  control->tool_cursor            = CLAY_TOOL_CURSOR_NONE;
  control->cursor_modifier        = CLAY_CURSOR_MODIFIER_NONE;

  control->toggle_cursor          = -1;
  control->toggle_tool_cursor     = -1;
  control->toggle_cursor_modifier = -1;
}

static void
clay_tool_control_finalize (GObject *object)
{
  ClayToolControl *control = CLAY_TOOL_CONTROL (object);

  g_slist_free (control->preserve_stack);

  g_free (control->action_opacity);
  g_free (control->action_size);
  g_free (control->action_aspect);
  g_free (control->action_angle);
  g_free (control->action_spacing);
  g_free (control->action_hardness);
  g_free (control->action_force);
  g_free (control->action_object_1);
  g_free (control->action_object_2);

  g_free (control->action_pixel_size);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

void
clay_tool_control_activate (ClayToolControl *control)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));
  g_return_if_fail (control->active == FALSE);

  control->active = TRUE;
}

void
clay_tool_control_halt (ClayToolControl *control)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));
  g_return_if_fail (control->active == TRUE);

  control->active = FALSE;
}

gboolean
clay_tool_control_is_active (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->active;
}

void
clay_tool_control_pause (ClayToolControl *control)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->paused_count++;
}

void
clay_tool_control_resume (ClayToolControl *control)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));
  g_return_if_fail (control->paused_count > 0);

  control->paused_count--;
}

gboolean
clay_tool_control_is_paused (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->paused_count > 0;
}

void
clay_tool_control_set_preserve (ClayToolControl *control,
                                gboolean         preserve)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->preserve = preserve ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_preserve (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->preserve;
}

void
clay_tool_control_push_preserve (ClayToolControl *control,
                                 gboolean         preserve)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->preserve_stack =
    g_slist_prepend (control->preserve_stack,
                     GINT_TO_POINTER (control->preserve));

  control->preserve = preserve ? TRUE : FALSE;
}

void
clay_tool_control_pop_preserve (ClayToolControl *control)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));
  g_return_if_fail (control->preserve_stack != NULL);

  control->preserve = GPOINTER_TO_INT (control->preserve_stack->data);

  control->preserve_stack = g_slist_delete_link (control->preserve_stack,
                                                 control->preserve_stack);
}

void
clay_tool_control_set_scroll_lock (ClayToolControl *control,
                                   gboolean         scroll_lock)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->scroll_lock = scroll_lock ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_scroll_lock (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->scroll_lock;
}

void
clay_tool_control_set_handle_empty_image (ClayToolControl *control,
                                          gboolean         handle_empty)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->handle_empty_image = handle_empty ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_handle_empty_image (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->handle_empty_image;
}

void
clay_tool_control_set_dirty_mask (ClayToolControl *control,
                                  ClayDirtyMask    dirty_mask)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->dirty_mask = dirty_mask;
}

ClayDirtyMask
clay_tool_control_get_dirty_mask (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), CLAY_DIRTY_NONE);

  return control->dirty_mask;
}

void
clay_tool_control_set_dirty_action (ClayToolControl *control,
                                    ClayToolAction  action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->dirty_action = action;
}

ClayToolAction
clay_tool_control_get_dirty_action (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), CLAY_TOOL_ACTION_HALT);

  return control->dirty_action;
}

void
clay_tool_control_set_motion_mode (ClayToolControl *control,
                                   ClayMotionMode   motion_mode)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->motion_mode = motion_mode;
}

ClayMotionMode
clay_tool_control_get_motion_mode (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), CLAY_MOTION_MODE_EXACT);

  return control->motion_mode;
}

void
clay_tool_control_set_snap_to (ClayToolControl *control,
                               gboolean         snap_to)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->auto_snap_to = snap_to ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_snap_to (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->auto_snap_to;
}

void
clay_tool_control_set_wants_click (ClayToolControl *control,
                                   gboolean         wants_click)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->wants_click = wants_click ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_wants_click (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->wants_click;
}

void
clay_tool_control_set_wants_double_click (ClayToolControl *control,
                                          gboolean         wants_double_click)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->wants_double_click = wants_double_click ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_wants_double_click (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->wants_double_click;
}

void
clay_tool_control_set_wants_triple_click (ClayToolControl *control,
                                          gboolean         wants_triple_click)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->wants_triple_click = wants_triple_click ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_wants_triple_click (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->wants_triple_click;
}

void
clay_tool_control_set_wants_all_key_events (ClayToolControl *control,
                                            gboolean         wants_key_events)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->wants_all_key_events = wants_key_events ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_wants_all_key_events (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->wants_all_key_events;
}

void
clay_tool_control_set_active_modifiers (ClayToolControl         *control,
                                        ClayToolActiveModifiers  active_modifiers)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->active_modifiers = active_modifiers;
}

ClayToolActiveModifiers
clay_tool_control_get_active_modifiers (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control),
                        CLAY_TOOL_ACTIVE_MODIFIERS_OFF);

  return control->active_modifiers;
}

void
clay_tool_control_set_snap_offsets (ClayToolControl *control,
                                    gint             offset_x,
                                    gint             offset_y,
                                    gint             width,
                                    gint             height)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->snap_offset_x = offset_x;
  control->snap_offset_y = offset_y;
  control->snap_width    = width;
  control->snap_height   = height;
}

void
clay_tool_control_get_snap_offsets (ClayToolControl *control,
                                    gint            *offset_x,
                                    gint            *offset_y,
                                    gint            *width,
                                    gint            *height)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (offset_x) *offset_x = control->snap_offset_x;
  if (offset_y) *offset_y = control->snap_offset_y;
  if (width)    *width    = control->snap_width;
  if (height)   *height   = control->snap_height;
}

void
clay_tool_control_set_precision (ClayToolControl     *control,
                                 ClayCursorPrecision  precision)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->precision = precision;
}

ClayCursorPrecision
clay_tool_control_get_precision (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control),
                        CLAY_CURSOR_PRECISION_PIXEL_CENTER);

  return control->precision;
}

void
clay_tool_control_set_toggled (ClayToolControl *control,
                               gboolean         toggled)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->toggled = toggled ? TRUE : FALSE;
}

gboolean
clay_tool_control_get_toggled (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  return control->toggled;
}

void
clay_tool_control_set_cursor (ClayToolControl *control,
                              ClayCursorType   cursor)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->cursor = cursor;
}

void
clay_tool_control_set_tool_cursor (ClayToolControl    *control,
                                   ClayToolCursorType  cursor)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->tool_cursor = cursor;
}

void
clay_tool_control_set_cursor_modifier (ClayToolControl    *control,
                                       ClayCursorModifier  modifier)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->cursor_modifier = modifier;
}

void
clay_tool_control_set_toggle_cursor (ClayToolControl *control,
                                     ClayCursorType   cursor)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->toggle_cursor = cursor;
}

void
clay_tool_control_set_toggle_tool_cursor (ClayToolControl    *control,
                                          ClayToolCursorType  cursor)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->toggle_tool_cursor = cursor;
}

void
clay_tool_control_set_toggle_cursor_modifier (ClayToolControl    *control,
                                              ClayCursorModifier  modifier)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  control->toggle_cursor_modifier = modifier;
}

ClayCursorType
clay_tool_control_get_cursor (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_cursor != -1)
    return control->toggle_cursor;

  return control->cursor;
}

ClayToolCursorType
clay_tool_control_get_tool_cursor (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_tool_cursor != -1)
    return control->toggle_tool_cursor;

  return control->tool_cursor;
}

ClayCursorModifier
clay_tool_control_get_cursor_modifier (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), FALSE);

  if (control->toggled && control->toggle_cursor_modifier != -1)
    return control->toggle_cursor_modifier;

  return control->cursor_modifier;
}

void
clay_tool_control_set_action_opacity (ClayToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_opacity)
    {
      g_free (control->action_opacity);
      control->action_opacity = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_opacity (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_opacity;
}

void
clay_tool_control_set_action_size (ClayToolControl *control,
                                   const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_size)
    {
      g_free (control->action_size);
      control->action_size = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_size (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_size;
}

void
clay_tool_control_set_action_aspect (ClayToolControl *control,
                                     const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_aspect)
    {
      g_free (control->action_aspect);
      control->action_aspect = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_aspect (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_aspect;
}

void
clay_tool_control_set_action_angle (ClayToolControl *control,
                                    const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_angle)
    {
      g_free (control->action_angle);
      control->action_angle = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_angle (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_angle;
}

void
clay_tool_control_set_action_spacing (ClayToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_spacing)
    {
      g_free (control->action_spacing);
      control->action_spacing = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_spacing (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_spacing;
}

void
clay_tool_control_set_action_hardness (ClayToolControl *control,
                                       const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_hardness)
    {
      g_free (control->action_hardness);
      control->action_hardness = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_hardness (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_hardness;
}

void
clay_tool_control_set_action_force (ClayToolControl *control,
                                      const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_force)
    {
      g_free (control->action_force);
      control->action_force = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_force (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_force;
}

void
clay_tool_control_set_action_object_1 (ClayToolControl *control,
                                       const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_object_1)
    {
      g_free (control->action_object_1);
      control->action_object_1 = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_object_1 (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_object_1;
}

void
clay_tool_control_set_action_object_2 (ClayToolControl *control,
                                       const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_object_2)
    {
      g_free (control->action_object_2);
      control->action_object_2 = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_object_2 (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_object_2;
}

void
clay_tool_control_set_action_pixel_size (ClayToolControl *control,
                                         const gchar     *action)
{
  g_return_if_fail (CLAY_IS_TOOL_CONTROL (control));

  if (action != control->action_pixel_size)
    {
      g_free (control->action_pixel_size);
      control->action_pixel_size = g_strdup (action);
    }
}

const gchar *
clay_tool_control_get_action_pixel_size (ClayToolControl *control)
{
  g_return_val_if_fail (CLAY_IS_TOOL_CONTROL (control), NULL);

  return control->action_pixel_size;
}
