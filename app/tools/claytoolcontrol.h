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

#pragma once

#include "core/clayobject.h"


#define CLAY_TYPE_TOOL_CONTROL            (clay_tool_control_get_type ())
#define CLAY_TOOL_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_CONTROL, ClayToolControl))
#define CLAY_TOOL_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_CONTROL, ClayToolControlClass))
#define CLAY_IS_TOOL_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_CONTROL))
#define CLAY_IS_TOOL_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_CONTROL))
#define CLAY_TOOL_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_CONTROL, ClayToolControlClass))


typedef struct _ClayToolControlClass ClayToolControlClass;


struct _ClayToolControl
{
  ClayObject           parent_instance;

  gboolean             active;             /*  state of tool activity          */
  gint                 paused_count;       /*  paused control count            */

  gboolean             preserve;           /*  Preserve this tool across       *
                                            *  drawable changes                */
  GSList              *preserve_stack;     /*  for push/pop preserve           */

  gboolean             scroll_lock;        /*  allow scrolling or not          */
  gboolean             handle_empty_image; /*  invoke the tool on images       *
                                            *  without active drawable         */
  ClayDirtyMask        dirty_mask;         /*  if preserve is FALSE, stop      *
                                            *  the tool on these events        */
  ClayToolAction       dirty_action;       /*  use this action to stop the     *
                                            *  tool when one of the dirty      *
                                            *  events occurs                   */
  ClayMotionMode       motion_mode;        /*  how to process motion events    *
                                            *  before they go to the tool      */
  gboolean             auto_snap_to;       /*  snap to guides automatically    */
  gint                 snap_offset_x;
  gint                 snap_offset_y;
  gint                 snap_width;
  gint                 snap_height;

  ClayCursorPrecision  precision;

  gboolean             wants_click;        /*  wants click detection           */
  gboolean             wants_double_click;
  gboolean             wants_triple_click;
  gboolean             wants_all_key_events;

  ClayToolActiveModifiers  active_modifiers;

  gboolean             toggled;

  ClayCursorType       cursor;
  ClayToolCursorType   tool_cursor;
  ClayCursorModifier   cursor_modifier;

  ClayCursorType       toggle_cursor;
  ClayToolCursorType   toggle_tool_cursor;
  ClayCursorModifier   toggle_cursor_modifier;

  gchar               *action_size;
  gchar               *action_aspect;
  gchar               *action_angle;

  gchar               *action_spacing;

  gchar               *action_opacity;
  gchar               *action_hardness;
  gchar               *action_force;

  gchar               *action_object_1;
  gchar               *action_object_2;

  /* Enum actions have a +-/min/max values which are not necessarily so
   * useful. They also have a variable value which works as a per mille
   * between the min and max, so it's hard to use, especially for
   * actions which have a huge max or which can have negative values.
   * For instance, aspect and angle can be negative. As for size and
   * spacing, their max value can be huge. Finally "size" can be
   * negative for the MyPaint brush tool, which uses a logarithmic
   * radius as base value.
   * For this reason, we also register specialized actions with clear
   * unit if needed.
   */
  gchar               *action_pixel_size;
};

struct _ClayToolControlClass
{
  ClayObjectClass parent_class;
};


GType          clay_tool_control_get_type           (void);

void           clay_tool_control_activate           (ClayToolControl *control);
void           clay_tool_control_halt               (ClayToolControl *control);
gboolean       clay_tool_control_is_active          (ClayToolControl *control);

void           clay_tool_control_pause              (ClayToolControl *control);
void           clay_tool_control_resume             (ClayToolControl *control);
gboolean       clay_tool_control_is_paused          (ClayToolControl *control);

void           clay_tool_control_set_preserve       (ClayToolControl *control,
                                                     gboolean         preserve);
gboolean       clay_tool_control_get_preserve       (ClayToolControl *control);

void           clay_tool_control_push_preserve      (ClayToolControl *control,
                                                     gboolean         preserve);
void           clay_tool_control_pop_preserve       (ClayToolControl *control);

void           clay_tool_control_set_scroll_lock    (ClayToolControl *control,
                                                     gboolean         scroll_lock);
gboolean       clay_tool_control_get_scroll_lock    (ClayToolControl *control);

void           clay_tool_control_set_handle_empty_image
                                                    (ClayToolControl *control,
                                                     gboolean         handle_empty);
gboolean       clay_tool_control_get_handle_empty_image
                                                    (ClayToolControl *control);

void           clay_tool_control_set_dirty_mask     (ClayToolControl *control,
                                                     ClayDirtyMask    dirty_mask);
ClayDirtyMask  clay_tool_control_get_dirty_mask     (ClayToolControl *control);

void           clay_tool_control_set_dirty_action   (ClayToolControl *control,
                                                     ClayToolAction   action);
ClayToolAction clay_tool_control_get_dirty_action   (ClayToolControl *control);

void           clay_tool_control_set_motion_mode    (ClayToolControl *control,
                                                     ClayMotionMode   motion_mode);
ClayMotionMode clay_tool_control_get_motion_mode    (ClayToolControl *control);

void           clay_tool_control_set_snap_to        (ClayToolControl *control,
                                                     gboolean         snap_to);
gboolean       clay_tool_control_get_snap_to        (ClayToolControl *control);

void           clay_tool_control_set_wants_click    (ClayToolControl *control,
                                                     gboolean         wants_click);
gboolean       clay_tool_control_get_wants_click    (ClayToolControl *control);

void           clay_tool_control_set_wants_double_click
                                                    (ClayToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       clay_tool_control_get_wants_double_click
                                                    (ClayToolControl *control);

void           clay_tool_control_set_wants_triple_click
                                                    (ClayToolControl *control,
                                                     gboolean         wants_double_click);
gboolean       clay_tool_control_get_wants_triple_click
                                                    (ClayToolControl *control);

void           clay_tool_control_set_wants_all_key_events
                                                    (ClayToolControl *control,
                                                     gboolean         wants_key_events);
gboolean       clay_tool_control_get_wants_all_key_events
                                                    (ClayToolControl *control);

void           clay_tool_control_set_active_modifiers
                                                    (ClayToolControl *control,
                                                     ClayToolActiveModifiers  active_modifiers);
ClayToolActiveModifiers
               clay_tool_control_get_active_modifiers
                                                    (ClayToolControl *control);

void           clay_tool_control_set_snap_offsets   (ClayToolControl *control,
                                                     gint             offset_x,
                                                     gint             offset_y,
                                                     gint             width,
                                                     gint             height);
void           clay_tool_control_get_snap_offsets   (ClayToolControl *control,
                                                     gint            *offset_x,
                                                     gint            *offset_y,
                                                     gint            *width,
                                                     gint            *height);

void           clay_tool_control_set_precision      (ClayToolControl *control,
                                                     ClayCursorPrecision  precision);
ClayCursorPrecision
               clay_tool_control_get_precision      (ClayToolControl *control);

void           clay_tool_control_set_toggled        (ClayToolControl *control,
                                                     gboolean         toggled);
gboolean       clay_tool_control_get_toggled        (ClayToolControl *control);

void           clay_tool_control_set_cursor         (ClayToolControl *control,
                                                     ClayCursorType   cursor);
void           clay_tool_control_set_tool_cursor    (ClayToolControl *control,
                                                     ClayToolCursorType  cursor);
void           clay_tool_control_set_cursor_modifier
                                                    (ClayToolControl *control,
                                                     ClayCursorModifier  modifier);
void           clay_tool_control_set_toggle_cursor  (ClayToolControl *control,
                                                     ClayCursorType   cursor);
void           clay_tool_control_set_toggle_tool_cursor
                                                    (ClayToolControl *control,
                                                     ClayToolCursorType  cursor);
void           clay_tool_control_set_toggle_cursor_modifier
                                                    (ClayToolControl *control,
                                                     ClayCursorModifier  modifier);

ClayCursorType
              clay_tool_control_get_cursor          (ClayToolControl *control);

ClayToolCursorType
              clay_tool_control_get_tool_cursor     (ClayToolControl *control);

ClayCursorModifier
              clay_tool_control_get_cursor_modifier (ClayToolControl *control);

void          clay_tool_control_set_action_opacity  (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_opacity  (ClayToolControl *control);

void          clay_tool_control_set_action_size     (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_size     (ClayToolControl *control);

void          clay_tool_control_set_action_aspect   (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_aspect   (ClayToolControl *control);

void          clay_tool_control_set_action_angle    (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_angle    (ClayToolControl *control);

void          clay_tool_control_set_action_spacing  (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_spacing  (ClayToolControl *control);

void          clay_tool_control_set_action_hardness (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_hardness (ClayToolControl *control);

void          clay_tool_control_set_action_force    (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_force    (ClayToolControl *control);

void          clay_tool_control_set_action_object_1 (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_object_1 (ClayToolControl *control);

void          clay_tool_control_set_action_object_2 (ClayToolControl *control,
                                                     const gchar     *action);
const gchar * clay_tool_control_get_action_object_2 (ClayToolControl *control);


void          clay_tool_control_set_action_pixel_size (ClayToolControl *control,
                                                       const gchar     *action);
const gchar * clay_tool_control_get_action_pixel_size (ClayToolControl *control);
