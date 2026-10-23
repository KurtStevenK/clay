/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others.
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

#include "claytool.h"


#define CLAY_TOOL_HANDLE_SIZE_CIRCLE    13
#define CLAY_TOOL_HANDLE_SIZE_CROSS     15
#define CLAY_TOOL_HANDLE_SIZE_CROSSHAIR 43
#define CLAY_TOOL_HANDLE_SIZE_LARGE     25
#define CLAY_TOOL_HANDLE_SIZE_SMALL     7


#define CLAY_TYPE_DRAW_TOOL            (clay_draw_tool_get_type ())
#define CLAY_DRAW_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAW_TOOL, ClayDrawTool))
#define CLAY_DRAW_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAW_TOOL, ClayDrawToolClass))
#define CLAY_IS_DRAW_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAW_TOOL))
#define CLAY_IS_DRAW_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAW_TOOL))
#define CLAY_DRAW_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAW_TOOL, ClayDrawToolClass))


typedef struct _ClayDrawToolClass ClayDrawToolClass;

struct _ClayDrawTool
{
  ClayTool        parent_instance;

  ClayDisplay    *display;        /*  The display we are drawing to (may be
                                   *  a different one than tool->display)
                                   */

  gint            paused_count;   /*  count to keep track of multiple pauses  */
  guint           draw_timeout;   /*  draw delay timeout ID                   */
  guint64         last_draw_time; /*  time of last draw(), monotonically      */

  ClayToolWidget *widget;
  gchar          *default_status;
  ClayCanvasItem *preview;
  ClayCanvasItem *item;
  GList          *group_stack;
};

struct _ClayDrawToolClass
{
  ClayToolClass   parent_class;

  /*  virtual function  */

  void (* draw) (ClayDrawTool *draw_tool);
};


GType            clay_draw_tool_get_type             (void);

void             clay_draw_tool_start                (ClayDrawTool     *draw_tool,
                                                      ClayDisplay      *display);
void             clay_draw_tool_stop                 (ClayDrawTool     *draw_tool);

gboolean         clay_draw_tool_is_active            (ClayDrawTool     *draw_tool);

void             clay_draw_tool_pause                (ClayDrawTool     *draw_tool);
void             clay_draw_tool_resume               (ClayDrawTool     *draw_tool);

gdouble          clay_draw_tool_calc_distance        (ClayDrawTool     *draw_tool,
                                                      ClayDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
gdouble          clay_draw_tool_calc_distance_square (ClayDrawTool     *draw_tool,
                                                      ClayDisplay      *display,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

void             clay_draw_tool_set_widget           (ClayDrawTool     *draw_tool,
                                                      ClayToolWidget   *widget);
void             clay_draw_tool_set_default_status   (ClayDrawTool     *draw_tool,
                                                      const gchar      *status);

void             clay_draw_tool_add_preview          (ClayDrawTool     *draw_tool,
                                                      ClayCanvasItem   *item);
void             clay_draw_tool_remove_preview       (ClayDrawTool     *draw_tool,
                                                      ClayCanvasItem   *item);

void             clay_draw_tool_add_item             (ClayDrawTool     *draw_tool,
                                                      ClayCanvasItem   *item);
void             clay_draw_tool_remove_item          (ClayDrawTool     *draw_tool,
                                                      ClayCanvasItem   *item);

ClayCanvasGroup* clay_draw_tool_add_stroke_group     (ClayDrawTool     *draw_tool);
ClayCanvasGroup* clay_draw_tool_add_fill_group       (ClayDrawTool     *draw_tool);

void             clay_draw_tool_push_group           (ClayDrawTool     *draw_tool,
                                                      ClayCanvasGroup  *group);
void             clay_draw_tool_pop_group            (ClayDrawTool     *draw_tool);

ClayCanvasItem * clay_draw_tool_add_line             (ClayDrawTool     *draw_tool,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);
ClayCanvasItem * clay_draw_tool_add_guide            (ClayDrawTool     *draw_tool,
                                                      ClayOrientationType  orientation,
                                                      gint              position,
                                                      ClayGuideStyle    style);
ClayCanvasItem * clay_draw_tool_add_crosshair        (ClayDrawTool     *draw_tool,
                                                      gint              position_x,
                                                      gint              position_y);
ClayCanvasItem * clay_draw_tool_add_sample_point     (ClayDrawTool     *draw_tool,
                                                      gint              x,
                                                      gint              y,
                                                      gint              index);
ClayCanvasItem * clay_draw_tool_add_rectangle        (ClayDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height);
ClayCanvasItem * clay_draw_tool_add_arc              (ClayDrawTool     *draw_tool,
                                                      gboolean          filled,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           width,
                                                      gdouble           height,
                                                      gdouble           start_angle,
                                                      gdouble           slice_angle);
ClayCanvasItem * clay_draw_tool_add_transform_preview(ClayDrawTool     *draw_tool,
                                                      ClayPickable     *pickable,
                                                      const ClayMatrix3 *transform,
                                                      gdouble           x1,
                                                      gdouble           y1,
                                                      gdouble           x2,
                                                      gdouble           y2);

ClayCanvasItem * clay_draw_tool_add_handle           (ClayDrawTool     *draw_tool,
                                                      ClayHandleType    type,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gint              width,
                                                      gint              height,
                                                      ClayHandleAnchor  anchor);

ClayCanvasItem * clay_draw_tool_add_lines            (ClayDrawTool     *draw_tool,
                                                      const ClayVector2 *points,
                                                      gint              n_points,
                                                      ClayMatrix3      *transform,
                                                      gboolean          filled);

ClayCanvasItem * clay_draw_tool_add_strokes          (ClayDrawTool     *draw_tool,
                                                      const ClayCoords *points,
                                                      gint              n_points,
                                                      ClayMatrix3      *transform,
                                                      gboolean          filled);

ClayCanvasItem * clay_draw_tool_add_pen              (ClayDrawTool     *draw_tool,
                                                      const ClayVector2 *points,
                                                      gint              n_points,
                                                      ClayContext      *context,
                                                      ClayActiveColor   color,
                                                      gint              width);

ClayCanvasItem * clay_draw_tool_add_boundary         (ClayDrawTool     *draw_tool,
                                                      const ClayBoundSeg *bound_segs,
                                                      gint              n_bound_segs,
                                                      ClayMatrix3      *transform,
                                                      gdouble           offset_x,
                                                      gdouble           offset_y);

ClayCanvasItem * clay_draw_tool_add_text_cursor      (ClayDrawTool     *draw_tool,
                                                      PangoRectangle   *cursor,
                                                      gboolean          overwrite,
                                                      ClayTextDirection direction);

ClayCanvasItem * clay_draw_tool_add_text             (ClayDrawTool     *draw_tool,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      gdouble           font_size,
                                                      gchar            *text);

gboolean         clay_draw_tool_on_handle            (ClayDrawTool     *draw_tool,
                                                      ClayDisplay      *display,
                                                      gdouble           x,
                                                      gdouble           y,
                                                      ClayHandleType    type,
                                                      gdouble           handle_x,
                                                      gdouble           handle_y,
                                                      gint              width,
                                                      gint              height,
                                                      ClayHandleAnchor  anchor);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayDrawTool, g_object_unref);
