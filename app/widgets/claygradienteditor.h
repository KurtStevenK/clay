/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient editor module copyight (C) 1996-1997 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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

#include "claydataeditor.h"


#define GRAD_NUM_COLORS 10


typedef enum
{
  GRAD_DRAG_NONE = 0,
  GRAD_DRAG_LEFT,
  GRAD_DRAG_MIDDLE,
  GRAD_DRAG_ALL
} GradientEditorDragMode;


#define CLAY_TYPE_GRADIENT_EDITOR            (clay_gradient_editor_get_type ())
#define CLAY_GRADIENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRADIENT_EDITOR, ClayGradientEditor))
#define CLAY_GRADIENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRADIENT_EDITOR, ClayGradientEditorClass))
#define CLAY_IS_GRADIENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRADIENT_EDITOR))
#define CLAY_IS_GRADIENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRADIENT_EDITOR))
#define CLAY_GRADIENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRADIENT_EDITOR, ClayGradientEditorClass))


typedef struct _ClayGradientEditorClass ClayGradientEditorClass;

struct _ClayGradientEditor
{
  ClayDataEditor          parent_instance;

  GtkWidget              *current_color;
  GtkWidget              *hint_label1;
  GtkWidget              *hint_label2;
  GtkWidget              *hint_label3;
  GtkWidget              *hint_label4;
  GtkWidget              *scrollbar;
  GtkWidget              *control;

  /*  Zoom and scrollbar  */
  gdouble                 zoom_factor;
  GtkAdjustment          *scroll_data;
  GtkGesture             *zoom_gesture;
  gdouble                 last_zoom_scale;

  /*  Gradient view  */
  gint                    view_last_x;          /* -1 if mouse has left focus */
  gboolean                view_button_down;

  /*  Gradient control  */
  ClayGradientSegment    *control_drag_segment; /* Segment which is being dragged */
  ClayGradientSegment    *control_sel_l;        /* Left segment of selection */
  ClayGradientSegment    *control_sel_r;        /* Right segment of selection */
  GradientEditorDragMode  control_drag_mode;    /* What is being dragged? */
  guint32                 control_click_time;   /* Time when mouse was pressed */
  gboolean                control_compress;     /* Compressing/expanding handles */
  gint                    control_last_x;       /* Last mouse position, -1 if out of focus */
  gdouble                 control_last_gx;      /* Last position (wrt gradient) when dragging */
  gdouble                 control_orig_pos;     /* Original click position when dragging */

  ClayGradientBlendColorSpace  blend_color_space;

  /*  Saved colors  */
  GeglColor              *saved_colors[GRAD_NUM_COLORS];

  /*  Color dialog  */
  GtkWidget              *color_dialog;
  ClayGradientSegment    *saved_segments;
  gboolean                saved_dirty;
};

struct _ClayGradientEditorClass
{
  ClayDataEditorClass  parent_class;
};


GType       clay_gradient_editor_get_type         (void);

GtkWidget * clay_gradient_editor_new              (ClayContext          *context,
                                                   ClayMenuFactory      *menu_factory);

void        clay_gradient_editor_get_selection    (ClayGradientEditor   *editor,
                                                   ClayGradient        **gradient,
                                                   ClayGradientSegment **left,
                                                   ClayGradientSegment **right);
void        clay_gradient_editor_set_selection    (ClayGradientEditor   *editor,
                                                   ClayGradientSegment  *left,
                                                   ClayGradientSegment  *right);

void        clay_gradient_editor_edit_left_color  (ClayGradientEditor   *editor);
void        clay_gradient_editor_edit_right_color (ClayGradientEditor   *editor);

void        clay_gradient_editor_zoom             (ClayGradientEditor   *editor,
                                                   ClayZoomType          zoom_type,
                                                   gdouble               delta,
                                                   gdouble               zoom_focus_x);
