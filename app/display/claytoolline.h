/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolline.h
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

#include "claytoolwidget.h"


/* in the context of ClayToolLine, "handle" is a collective term for either an
 * endpoint or a slider.  a handle value may be either a (nonnegative) slider
 * index, or one of the values below:
 */
#define CLAY_TOOL_LINE_HANDLE_NONE  (-3)
#define CLAY_TOOL_LINE_HANDLE_START (-2)
#define CLAY_TOOL_LINE_HANDLE_END   (-1)

#define CLAY_TOOL_LINE_HANDLE_IS_SLIDER(handle) ((handle) >= 0)


#define CLAY_TYPE_TOOL_LINE            (clay_tool_line_get_type ())
#define CLAY_TOOL_LINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_LINE, ClayToolLine))
#define CLAY_TOOL_LINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_LINE, ClayToolLineClass))
#define CLAY_IS_TOOL_LINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_LINE))
#define CLAY_IS_TOOL_LINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_LINE))
#define CLAY_TOOL_LINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_LINE, ClayToolLineClass))


typedef struct _ClayToolLine        ClayToolLine;
typedef struct _ClayToolLinePrivate ClayToolLinePrivate;
typedef struct _ClayToolLineClass   ClayToolLineClass;

struct _ClayToolLine
{
  ClayToolWidget       parent_instance;

  ClayToolLinePrivate *private;
};

struct _ClayToolLineClass
{
  ClayToolWidgetClass  parent_class;

  /*  signals  */
  gboolean (* can_add_slider)           (ClayToolLine        *line,
                                         gdouble              value);
  gint     (* add_slider)               (ClayToolLine        *line,
                                         gdouble              value);
  void     (* prepare_to_remove_slider) (ClayToolLine        *line,
                                         gint                 slider,
                                         gboolean             remove);
  void     (* remove_slider)            (ClayToolLine        *line,
                                         gint                 slider);
  void     (* selection_changed)        (ClayToolLine        *line);
  gboolean (* handle_clicked)           (ClayToolLine        *line,
                                         gint                 handle,
                                         GdkModifierType      state,
                                         ClayButtonPressType  press_type);
};


GType                        clay_tool_line_get_type      (void);

ClayToolWidget             * clay_tool_line_new           (ClayDisplayShell           *shell,
                                                           gdouble                     x1,
                                                           gdouble                     y1,
                                                           gdouble                     x2,
                                                           gdouble                     y2);

void                         clay_tool_line_set_sliders   (ClayToolLine               *line,
                                                           const ClayControllerSlider *sliders,
                                                           gint                        n_sliders);
const ClayControllerSlider * clay_tool_line_get_sliders   (ClayToolLine               *line,
                                                           gint                       *n_sliders);

void                         clay_tool_line_set_selection (ClayToolLine               *line,
                                                           gint                        handle);
gint                         clay_tool_line_get_selection (ClayToolLine               *line);
