/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolrectangle.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on ClayRectangleTool
 * Copyright (C) 2007 Martin Nordholts
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


typedef enum
{
  CLAY_TOOL_RECTANGLE_DEAD,
  CLAY_TOOL_RECTANGLE_CREATING,
  CLAY_TOOL_RECTANGLE_MOVING,
  CLAY_TOOL_RECTANGLE_RESIZING_UPPER_LEFT,
  CLAY_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT,
  CLAY_TOOL_RECTANGLE_RESIZING_LOWER_LEFT,
  CLAY_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT,
  CLAY_TOOL_RECTANGLE_RESIZING_LEFT,
  CLAY_TOOL_RECTANGLE_RESIZING_RIGHT,
  CLAY_TOOL_RECTANGLE_RESIZING_TOP,
  CLAY_TOOL_RECTANGLE_RESIZING_BOTTOM,
  CLAY_TOOL_RECTANGLE_AUTO_SHRINK,
  CLAY_TOOL_RECTANGLE_EXECUTING,
  CLAY_N_TOOL_RECTANGLE_FUNCTIONS
} ClayRectangleFunction;


#define CLAY_TYPE_TOOL_RECTANGLE            (clay_tool_rectangle_get_type ())
#define CLAY_TOOL_RECTANGLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_RECTANGLE, ClayToolRectangle))
#define CLAY_TOOL_RECTANGLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_RECTANGLE, ClayToolRectangleClass))
#define CLAY_IS_TOOL_RECTANGLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_RECTANGLE))
#define CLAY_IS_TOOL_RECTANGLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_RECTANGLE))
#define CLAY_TOOL_RECTANGLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_RECTANGLE, ClayToolRectangleClass))


typedef struct _ClayToolRectangle        ClayToolRectangle;
typedef struct _ClayToolRectanglePrivate ClayToolRectanglePrivate;
typedef struct _ClayToolRectangleClass   ClayToolRectangleClass;

struct _ClayToolRectangle
{
  ClayToolWidget            parent_instance;

  ClayToolRectanglePrivate *private;
};

struct _ClayToolRectangleClass
{
  ClayToolWidgetClass  parent_class;

  /*  signals  */

  gboolean (* change_complete) (ClayToolRectangle *rectangle);
};


GType    clay_tool_rectangle_get_type         (void);

ClayToolWidget * clay_tool_rectangle_new      (ClayDisplayShell       *shell);

ClayRectangleFunction
         clay_tool_rectangle_get_function     (ClayToolRectangle      *rectangle);
void     clay_tool_rectangle_set_function     (ClayToolRectangle      *rectangle,
                                               ClayRectangleFunction   function);

void     clay_tool_rectangle_set_constraint   (ClayToolRectangle      *rectangle,
                                               ClayRectangleConstraint constraint);
ClayRectangleConstraint
         clay_tool_rectangle_get_constraint   (ClayToolRectangle      *rectangle);

void     clay_tool_rectangle_get_public_rect  (ClayToolRectangle      *rectangle,
                                               gdouble                *pub_x1,
                                               gdouble                *pub_y1,
                                               gdouble                *pub_x2,
                                               gdouble                *pub_y2);

void     clay_tool_rectangle_pending_size_set (ClayToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

void     clay_tool_rectangle_constraint_size_set
                                              (ClayToolRectangle      *rectangle,
                                               GObject                *object,
                                               const gchar            *width_property,
                                               const gchar            *height_property);

gboolean clay_tool_rectangle_rectangle_is_first
                                              (ClayToolRectangle      *rectangle);
gboolean clay_tool_rectangle_rectangle_is_new (ClayToolRectangle      *rectangle);
gboolean clay_tool_rectangle_point_in_rectangle
                                              (ClayToolRectangle      *rectangle,
                                               gdouble                 x,
                                               gdouble                 y);

void     clay_tool_rectangle_frame_item       (ClayToolRectangle      *rectangle,
                                               ClayItem               *item);
void     clay_tool_rectangle_auto_shrink      (ClayToolRectangle      *rectrectangle,
                                               gboolean                shrink_merged);
