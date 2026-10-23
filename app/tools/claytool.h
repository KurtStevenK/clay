/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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


#define CLAY_TYPE_TOOL            (clay_tool_get_type ())
#define CLAY_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL, ClayTool))
#define CLAY_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL, ClayToolClass))
#define CLAY_IS_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL))
#define CLAY_IS_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL))
#define CLAY_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL, ClayToolClass))

#define CLAY_TOOL_GET_OPTIONS(t)  (clay_tool_get_options (CLAY_TOOL (t)))


typedef struct _ClayToolClass ClayToolClass;

struct _ClayTool
{
  ClayObject       parent_instance;

  ClayToolInfo    *tool_info;

  gchar           *label;
  gchar           *undo_desc;
  gchar           *icon_name;
  gchar           *help_id;

  gint             ID;          /*  unique tool ID                         */

  ClayToolControl *control;

  ClayDisplay     *display;     /*  pointer to currently active display    */
  GList           *drawables;   /*  list of the tool's current drawables   */

  /*  private state of clay_tool_set_focus_display() and
   *  clay_tool_set_[active_]modifier_state()
   */
  ClayDisplay     *focus_display;
  GdkModifierType  modifier_state;
  GdkModifierType  button_press_state;
  GdkModifierType  active_modifier_state;

  /*  private state for synthesizing button_release() events
   */
  ClayCoords       last_pointer_coords;
  guint32          last_pointer_time;
  GdkModifierType  last_pointer_state;

  /*  private state for click detection
   */
  gboolean         in_click_distance;
  gboolean         got_motion_event;
  ClayCoords       button_press_coords;
  guint32          button_press_time;

  /*  private list of displays which have a status message from this tool
   */
  GList           *status_displays;

  /*  on-canvas progress  */
  ClayCanvasItem  *progress;
  ClayDisplay     *progress_display;
  GtkWidget       *progress_grab_widget;
  gboolean         progress_cancelable;
};

struct _ClayToolClass
{
  ClayObjectClass  parent_class;

  /*  virtual functions  */

  gboolean        (* has_display)         (ClayTool              *tool,
                                           ClayDisplay           *display);
  ClayDisplay   * (* has_image)           (ClayTool              *tool,
                                           ClayImage             *image);

  gboolean        (* initialize)          (ClayTool              *tool,
                                           ClayDisplay           *display,
                                           GError               **error);
  void            (* control)             (ClayTool              *tool,
                                           ClayToolAction         action,
                                           ClayDisplay           *display);

  void            (* button_press)        (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           ClayButtonPressType    press_type,
                                           ClayDisplay           *display);
  void            (* button_release)      (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           ClayButtonReleaseType  release_type,
                                           ClayDisplay           *display);
  void            (* motion)              (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           ClayDisplay           *display);

  gboolean        (* key_press)           (ClayTool              *tool,
                                           GdkEventKey           *kevent,
                                           ClayDisplay           *display);
  gboolean        (* key_release)         (ClayTool              *tool,
                                           GdkEventKey           *kevent,
                                           ClayDisplay           *display);
  void            (* modifier_key)        (ClayTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           ClayDisplay           *display);
  void            (* active_modifier_key) (ClayTool              *tool,
                                           GdkModifierType        key,
                                           gboolean               press,
                                           GdkModifierType        state,
                                           ClayDisplay           *display);

  void            (* oper_update)         (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           GdkModifierType        state,
                                           gboolean               proximity,
                                           ClayDisplay           *display);
  void            (* cursor_update)       (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           GdkModifierType        state,
                                           ClayDisplay           *display);

  const gchar   * (* can_undo)            (ClayTool              *tool,
                                           ClayDisplay           *display);
  const gchar   * (* can_redo)            (ClayTool              *tool,
                                           ClayDisplay           *display);
  gboolean        (* undo)                (ClayTool              *tool,
                                           ClayDisplay           *display);
  gboolean        (* redo)                (ClayTool              *tool,
                                           ClayDisplay           *display);

  ClayUIManager * (* get_popup)           (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           GdkModifierType        state,
                                           ClayDisplay           *display,
                                           const gchar          **ui_path);

  void            (* options_notify)      (ClayTool              *tool,
                                           ClayToolOptions       *options,
                                           const GParamSpec      *pspec);

  gboolean           is_destructive;
};


GType             clay_tool_get_type            (void);

ClayToolOptions * clay_tool_get_options         (ClayTool            *tool);

void              clay_tool_set_label           (ClayTool            *tool,
                                                 const gchar         *label);
const gchar     * clay_tool_get_label           (ClayTool            *tool);

void              clay_tool_set_undo_desc       (ClayTool            *tool,
                                                 const gchar         *undo_desc);
const gchar     * clay_tool_get_undo_desc       (ClayTool            *tool);

void              clay_tool_set_icon_name       (ClayTool            *tool,
                                                 const gchar         *icon_name);
const gchar     * clay_tool_get_icon_name       (ClayTool            *tool);

void              clay_tool_set_help_id         (ClayTool            *tool,
                                                 const gchar         *help_id);
const gchar     * clay_tool_get_help_id         (ClayTool            *tool);

gboolean          clay_tool_has_display         (ClayTool            *tool,
                                                 ClayDisplay         *display);
ClayDisplay     * clay_tool_has_image           (ClayTool            *tool,
                                                 ClayImage           *image);

gboolean          clay_tool_initialize          (ClayTool            *tool,
                                                 ClayDisplay         *display);
void              clay_tool_control             (ClayTool            *tool,
                                                 ClayToolAction       action,
                                                 ClayDisplay         *display);

void              clay_tool_button_press        (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 ClayButtonPressType  press_type,
                                                 ClayDisplay         *display);
void              clay_tool_button_release      (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display);
void              clay_tool_motion              (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 guint32              time,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display);

gboolean          clay_tool_key_press           (ClayTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 ClayDisplay         *display);
gboolean          clay_tool_key_release         (ClayTool            *tool,
                                                 GdkEventKey         *kevent,
                                                 ClayDisplay         *display);

void              clay_tool_set_focus_display   (ClayTool            *tool,
                                                 ClayDisplay         *display);
void              clay_tool_set_modifier_state  (ClayTool            *tool,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display);
void        clay_tool_set_active_modifier_state (ClayTool            *tool,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display);

void              clay_tool_oper_update         (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 GdkModifierType      state,
                                                 gboolean             proximity,
                                                 ClayDisplay         *display);
void              clay_tool_cursor_update       (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display);

const gchar     * clay_tool_can_undo            (ClayTool            *tool,
                                                 ClayDisplay         *display);
const gchar     * clay_tool_can_redo            (ClayTool            *tool,
                                                 ClayDisplay         *display);
gboolean          clay_tool_undo                (ClayTool            *tool,
                                                 ClayDisplay         *display);
gboolean          clay_tool_redo                (ClayTool            *tool,
                                                 ClayDisplay         *display);

ClayUIManager   * clay_tool_get_popup           (ClayTool            *tool,
                                                 const ClayCoords    *coords,
                                                 GdkModifierType      state,
                                                 ClayDisplay         *display,
                                                 const gchar        **ui_path);

void              clay_tool_push_status         (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              clay_tool_push_status_coords  (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 ClayCursorPrecision  precision,
                                                 const gchar         *title,
                                                 gdouble              x,
                                                 const gchar         *separator,
                                                 gdouble              y,
                                                 const gchar         *help);
void              clay_tool_push_status_length  (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 const gchar         *title,
                                                 ClayOrientationType  axis,
                                                 gdouble              value,
                                                 const gchar         *help);
void              clay_tool_replace_status      (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              clay_tool_pop_status          (ClayTool            *tool,
                                                 ClayDisplay         *display);

void              clay_tool_message             (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 const gchar         *format,
                                                 ...) G_GNUC_PRINTF(3,4);
void              clay_tool_message_literal     (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 const gchar         *message);

void              clay_tool_set_cursor          (ClayTool            *tool,
                                                 ClayDisplay         *display,
                                                 ClayCursorType       cursor,
                                                 ClayToolCursorType   tool_cursor,
                                                 ClayCursorModifier   modifier);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayTool, g_object_unref);
