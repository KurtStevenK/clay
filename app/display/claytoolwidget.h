/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolwidget.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TOOL_WIDGET_RESPONSE_CONFIRM -1
#define CLAY_TOOL_WIDGET_RESPONSE_CANCEL  -2
#define CLAY_TOOL_WIDGET_RESPONSE_RESET   -3


#define CLAY_TYPE_TOOL_WIDGET            (clay_tool_widget_get_type ())
#define CLAY_TOOL_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_WIDGET, ClayToolWidget))
#define CLAY_TOOL_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_WIDGET, ClayToolWidgetClass))
#define CLAY_IS_TOOL_WIDGET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_WIDGET))
#define CLAY_IS_TOOL_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_WIDGET))
#define CLAY_TOOL_WIDGET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_WIDGET, ClayToolWidgetClass))


typedef struct _ClayToolWidgetPrivate ClayToolWidgetPrivate;
typedef struct _ClayToolWidgetClass   ClayToolWidgetClass;

struct _ClayToolWidget
{
  ClayObject             parent_instance;

  ClayToolWidgetPrivate *private;
};

struct _ClayToolWidgetClass
{
  ClayObjectClass  parent_class;

  /*  signals  */
  void     (* changed)         (ClayToolWidget        *widget);
  void     (* response)        (ClayToolWidget        *widget,
                                gint                   response_id);
  void     (* snap_offsets)    (ClayToolWidget        *widget,
                                gint                   offset_x,
                                gint                   offset_y,
                                gint                   width,
                                gint                   height);
  void     (* status)          (ClayToolWidget        *widget,
                                const gchar           *status);
  void     (* status_coords)   (ClayToolWidget        *widget,
                                const gchar           *title,
                                gdouble                x,
                                const gchar           *separator,
                                gdouble                y,
                                const gchar           *help);
  void     (* message)         (ClayToolWidget        *widget,
                                const gchar           *message);
  void     (* focus_changed)   (ClayToolWidget        *widget);

  /*  virtual functions  */
  gint     (* button_press)    (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonPressType    press_type);
  void     (* button_release)  (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state,
                                ClayButtonReleaseType  release_type);
  void     (* motion)          (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                guint32                time,
                                GdkModifierType        state);

  ClayHit  (* hit)             (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* hover)           (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                GdkModifierType        state,
                                gboolean               proximity);
  void     (* leave_notify)    (ClayToolWidget        *widget);

  gboolean (* key_press)       (ClayToolWidget        *widget,
                                GdkEventKey           *kevent);
  gboolean (* key_release)     (ClayToolWidget        *widget,
                                GdkEventKey           *kevent);

  void     (* motion_modifier) (ClayToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);
  void     (* hover_modifier)  (ClayToolWidget        *widget,
                                GdkModifierType        key,
                                gboolean               press,
                                GdkModifierType        state);

  gboolean (* get_cursor)      (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                GdkModifierType        state,
                                ClayCursorType        *cursor,
                                ClayToolCursorType    *tool_cursor,
                                ClayCursorModifier    *modifier);

  ClayUIManager *
           (* get_popup)       (ClayToolWidget        *widget,
                                const ClayCoords      *coords,
                                GdkModifierType        state,
                                const gchar          **ui_path);

  gboolean update_on_scale;
  gboolean update_on_scroll;
  gboolean update_on_rotate;
};


GType              clay_tool_widget_get_type          (void);

ClayDisplayShell * clay_tool_widget_get_shell         (ClayToolWidget  *widget);
ClayCanvasItem   * clay_tool_widget_get_item          (ClayToolWidget  *widget);

void               clay_tool_widget_set_visible       (ClayToolWidget  *widget,
                                                       gboolean         visible);
gboolean           clay_tool_widget_get_visible       (ClayToolWidget  *widget);

void               clay_tool_widget_set_focus         (ClayToolWidget  *widget,
                                                       gboolean         focus);
gboolean           clay_tool_widget_get_focus         (ClayToolWidget  *widget);

/*  for subclasses, to notify the handling tool
 */
void               clay_tool_widget_changed           (ClayToolWidget  *widget);

void               clay_tool_widget_response          (ClayToolWidget  *widget,
                                                       gint             response_id);

void               clay_tool_widget_set_snap_offsets  (ClayToolWidget  *widget,
                                                       gint             offset_x,
                                                       gint             offset_y,
                                                       gint             width,
                                                       gint             height);
void               clay_tool_widget_get_snap_offsets  (ClayToolWidget  *widget,
                                                       gint            *offset_x,
                                                       gint            *offset_y,
                                                       gint            *width,
                                                       gint            *height);

void               clay_tool_widget_set_status        (ClayToolWidget  *widget,
                                                       const gchar     *status);
void               clay_tool_widget_set_status_coords (ClayToolWidget  *widget,
                                                       const gchar     *title,
                                                       gdouble          x,
                                                       const gchar     *separator,
                                                       gdouble          y,
                                                       const gchar     *help);

void               clay_tool_widget_message           (ClayToolWidget  *widget,
                                                       const gchar     *format,
                                                       ...) G_GNUC_PRINTF (2, 3);
void               clay_tool_widget_message_literal   (ClayToolWidget  *widget,
                                                       const gchar     *message);

/*  for subclasses, to add and manage their items
 */
void               clay_tool_widget_add_item         (ClayToolWidget  *widget,
                                                      ClayCanvasItem  *item);
void               clay_tool_widget_remove_item      (ClayToolWidget  *widget,
                                                      ClayCanvasItem  *item);

ClayCanvasGroup  * clay_tool_widget_add_group        (ClayToolWidget  *widget);
ClayCanvasGroup  * clay_tool_widget_add_stroke_group (ClayToolWidget  *widget);
ClayCanvasGroup  * clay_tool_widget_add_fill_group   (ClayToolWidget  *widget);

void               clay_tool_widget_push_group       (ClayToolWidget  *widget,
                                                      ClayCanvasGroup *group);
void               clay_tool_widget_pop_group        (ClayToolWidget  *widget);

/*  for subclasses, convenience functions to add specific items
 */
ClayCanvasItem * clay_tool_widget_add_line      (ClayToolWidget       *widget,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2);
ClayCanvasItem * clay_tool_widget_add_rectangle (ClayToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 gboolean              filled);
ClayCanvasItem * clay_tool_widget_add_arc       (ClayToolWidget       *widget,
                                                 gdouble               center_x,
                                                 gdouble               center_y,
                                                 gdouble               radius_x,
                                                 gdouble               radius_y,
                                                 gdouble               start_angle,
                                                 gdouble               slice_angle,
                                                 gboolean              filled);
ClayCanvasItem * clay_tool_widget_add_limit     (ClayToolWidget       *widget,
                                                 ClayLimitType         type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               radius,
                                                 gdouble               aspect_ratio,
                                                 gdouble               angle,
                                                 gboolean              dashed);
ClayCanvasItem * clay_tool_widget_add_polygon   (ClayToolWidget       *widget,
                                                 ClayMatrix3          *transform,
                                                 const ClayVector2    *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
ClayCanvasItem * clay_tool_widget_add_polygon_from_coords
                                                (ClayToolWidget       *widget,
                                                 ClayMatrix3          *transform,
                                                 const ClayCoords     *points,
                                                 gint                  n_points,
                                                 gboolean              filled);
ClayCanvasItem * clay_tool_widget_add_path      (ClayToolWidget       *widget,
                                                 const ClayBezierDesc *desc);

ClayCanvasItem * clay_tool_widget_add_handle    (ClayToolWidget       *widget,
                                                 ClayHandleType        type,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gint                  width,
                                                 gint                  height,
                                                 ClayHandleAnchor      anchor);
ClayCanvasItem * clay_tool_widget_add_corner    (ClayToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 ClayHandleAnchor      anchor,
                                                 gint                  corner_width,
                                                 gint                  corner_height,
                                                 gboolean              outside);

ClayCanvasItem * clay_tool_widget_add_rectangle_guides
                                                (ClayToolWidget       *widget,
                                                 gdouble               x,
                                                 gdouble               y,
                                                 gdouble               width,
                                                 gdouble               height,
                                                 ClayGuidesType        type);
ClayCanvasItem * clay_tool_widget_add_transform_guides
                                                (ClayToolWidget       *widget,
                                                 const ClayMatrix3    *transform,
                                                 gdouble               x1,
                                                 gdouble               y1,
                                                 gdouble               x2,
                                                 gdouble               y2,
                                                 ClayGuidesType        type,
                                                 gint                  n_guides,
                                                 gboolean              clip);

/*  for tools, to be called from the respective ClayTool method
 *  implementations
 */
gint       clay_tool_widget_button_press    (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             ClayButtonPressType    press_type);
void       clay_tool_widget_button_release  (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state,
                                             ClayButtonReleaseType  release_type);
void       clay_tool_widget_motion          (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             guint32                time,
                                             GdkModifierType        state);

ClayHit    clay_tool_widget_hit             (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       clay_tool_widget_hover           (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             gboolean               proximity);
void       clay_tool_widget_leave_notify    (ClayToolWidget        *widget);

gboolean   clay_tool_widget_key_press       (ClayToolWidget        *widget,
                                             GdkEventKey           *kevent);
gboolean   clay_tool_widget_key_release     (ClayToolWidget        *widget,
                                             GdkEventKey           *kevent);

void       clay_tool_widget_motion_modifier (ClayToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);
void       clay_tool_widget_hover_modifier  (ClayToolWidget        *widget,
                                             GdkModifierType        key,
                                             gboolean               press,
                                             GdkModifierType        state);

gboolean   clay_tool_widget_get_cursor      (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             ClayCursorType        *cursor,
                                             ClayToolCursorType    *tool_cursor,
                                             ClayCursorModifier    *modifier);

ClayUIManager *
           clay_tool_widget_get_popup       (ClayToolWidget        *widget,
                                             const ClayCoords      *coords,
                                             GdkModifierType        state,
                                             const gchar          **ui_path);
