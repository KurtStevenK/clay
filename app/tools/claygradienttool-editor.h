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

#ifndef  __CLAY_GRADIENT_TOOL_EDITOR_H__
#define  __CLAY_GRADIENT_TOOL_EDITOR_H__


void          clay_gradient_tool_editor_options_notify   (ClayGradientTool *gradient_tool,
                                                          ClayToolOptions  *options,
                                                          const GParamSpec *pspec);

void          clay_gradient_tool_editor_start            (ClayGradientTool *gradient_tool);
void          clay_gradient_tool_editor_halt             (ClayGradientTool *gradient_tool);

gboolean      clay_gradient_tool_editor_line_changed     (ClayGradientTool *gradient_tool);

void          clay_gradient_tool_editor_fg_bg_changed    (ClayGradientTool *gradient_tool);

void          clay_gradient_tool_editor_gradient_dirty   (ClayGradientTool *gradient_tool);

void          clay_gradient_tool_editor_gradient_changed (ClayGradientTool *gradient_tool);

const gchar * clay_gradient_tool_editor_can_undo         (ClayGradientTool *gradient_tool);
const gchar * clay_gradient_tool_editor_can_redo         (ClayGradientTool *gradient_tool);

gboolean      clay_gradient_tool_editor_undo             (ClayGradientTool *gradient_tool);
gboolean      clay_gradient_tool_editor_redo             (ClayGradientTool *gradient_tool);

void          clay_gradient_tool_editor_start_edit       (ClayGradientTool *gradient_tool);
void          clay_gradient_tool_editor_end_edit         (ClayGradientTool *gradient_tool,
                                                          gboolean          cancel);


#endif  /*  __CLAY_GRADIENT_TOOL_EDITOR_H__  */
