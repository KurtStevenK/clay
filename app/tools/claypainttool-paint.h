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


typedef void (* ClayPaintToolPaintFunc) (ClayPaintTool *tool,
                                         gpointer       data);



gboolean   clay_paint_tool_paint_start     (ClayPaintTool           *tool,
                                            ClayDisplay             *display,
                                            const ClayCoords        *coords,
                                            guint32                  time,
                                            gboolean                 constrain,
                                            GError                 **error);
void       clay_paint_tool_paint_end       (ClayPaintTool           *tool,
                                            guint32                  time,
                                            gboolean                 cancel);

gboolean   clay_paint_tool_paint_is_active (ClayPaintTool           *tool);

void       clay_paint_tool_paint_push      (ClayPaintTool           *tool,
                                            ClayPaintToolPaintFunc   func,
                                            gpointer                 data);

void       clay_paint_tool_paint_motion    (ClayPaintTool           *tool,
                                            const ClayCoords        *coords,
                                            guint32                  time);
