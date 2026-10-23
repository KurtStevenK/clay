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

#pragma once

#include "claycolortool.h"


#define CLAY_PAINT_TOOL_LINE_MASK (clay_get_extend_selection_mask ())


#define CLAY_TYPE_PAINT_TOOL            (clay_paint_tool_get_type ())
#define CLAY_PAINT_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINT_TOOL, ClayPaintTool))
#define CLAY_PAINT_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINT_TOOL, ClayPaintToolClass))
#define CLAY_IS_PAINT_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINT_TOOL))
#define CLAY_IS_PAINT_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINT_TOOL))
#define CLAY_PAINT_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINT_TOOL, ClayPaintToolClass))

#define CLAY_PAINT_TOOL_GET_OPTIONS(t)  (CLAY_PAINT_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayPaintToolClass ClayPaintToolClass;

struct _ClayPaintTool
{
  ClayColorTool  parent_instance;

  gboolean       active;
  gboolean       pick_colors;      /*  pick color if ctrl is pressed           */
  gboolean       can_multi_paint;  /*  if paint works with multiple drawables  */
  gboolean       draw_line;

  gboolean       show_cursor;
  gboolean       draw_brush;
  gboolean       snap_brush;
  gboolean       draw_fallback;
  gint           fallback_size;
  gboolean       draw_circle;
  gint           circle_size;

  const gchar   *status;       /* status message */
  const gchar   *status_line;  /* status message when drawing a line */
  const gchar   *status_ctrl;  /* additional message for the ctrl modifier */

  ClayPaintCore *core;

  ClayDisplay   *display;
  GList         *drawables;

  gdouble        cursor_x;
  gdouble        cursor_y;

  gdouble        paint_x;
  gdouble        paint_y;
};

struct _ClayPaintToolClass
{
  ClayColorToolClass  parent_class;

  void             (* paint_prepare) (ClayPaintTool *paint_tool,
                                      ClayDisplay   *display);
  void             (* paint_start)   (ClayPaintTool *paint_tool);
  void             (* paint_end)     (ClayPaintTool *paint_tool);
  void             (* paint_flush)   (ClayPaintTool *paint_tool);

  ClayCanvasItem * (* get_outline)   (ClayPaintTool *paint_tool,
                                      ClayDisplay   *display,
                                      gdouble        x,
                                      gdouble        y);

  gboolean         (* is_alpha_only) (ClayPaintTool *paint_tool,
                                      ClayDrawable  *drawable);
};


GType   clay_paint_tool_get_type            (void);

void    clay_paint_tool_set_active          (ClayPaintTool       *tool,
                                             gboolean             active);

void    clay_paint_tool_enable_color_picker (ClayPaintTool       *tool,
                                             ClayColorPickTarget  target);

void    clay_paint_tool_enable_multi_paint  (ClayPaintTool       *tool);

void    clay_paint_tool_set_draw_fallback   (ClayPaintTool       *tool,
                                             gboolean             draw_fallback,
                                             gint                 fallback_size);

void    clay_paint_tool_set_draw_circle     (ClayPaintTool       *tool,
                                             gboolean             draw_circle,
                                             gint                 circle_size);

void    clay_paint_tool_force_draw          (ClayPaintTool       *tool,
                                             gboolean             force);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPaintTool, g_object_unref);
