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


#define CLAY_TYPE_BUTTON_PRESS_TYPE (clay_button_press_type_get_type ())

GType clay_button_press_type_get_type (void);

typedef enum
{
  CLAY_BUTTON_PRESS_NORMAL,
  CLAY_BUTTON_PRESS_DOUBLE,
  CLAY_BUTTON_PRESS_TRIPLE
} ClayButtonPressType;


#define CLAY_TYPE_BUTTON_RELEASE_TYPE (clay_button_release_type_get_type ())

GType clay_button_release_type_get_type (void);

typedef enum
{
  CLAY_BUTTON_RELEASE_NORMAL,
  CLAY_BUTTON_RELEASE_CANCEL,
  CLAY_BUTTON_RELEASE_CLICK,
  CLAY_BUTTON_RELEASE_NO_MOTION
} ClayButtonReleaseType;


#define CLAY_TYPE_COMPASS_ORIENTATION (clay_compass_orientation_get_type ())

GType clay_compass_orientation_get_type (void);

typedef enum
{
  CLAY_COMPASS_ORIENTATION_AUTO,       /*< desc="Auto"       >*/
  CLAY_COMPASS_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  CLAY_COMPASS_ORIENTATION_VERTICAL    /*< desc="Vertical"   >*/
} ClayCompassOrientation;


#define CLAY_TYPE_CURSOR_PRECISION (clay_cursor_precision_get_type ())

GType clay_cursor_precision_get_type (void);

typedef enum
{
  CLAY_CURSOR_PRECISION_PIXEL_CENTER,
  CLAY_CURSOR_PRECISION_PIXEL_BORDER,
  CLAY_CURSOR_PRECISION_SUBPIXEL
} ClayCursorPrecision;


#define CLAY_TYPE_GUIDES_TYPE (clay_guides_type_get_type ())

GType clay_guides_type_get_type (void);

typedef enum
{
  CLAY_GUIDES_NONE,          /*< desc="No guides"       >*/
  CLAY_GUIDES_CENTER_LINES,  /*< desc="Center lines"    >*/
  CLAY_GUIDES_THIRDS,        /*< desc="Rule of thirds"  >*/
  CLAY_GUIDES_FIFTHS,        /*< desc="Rule of fifths"  >*/
  CLAY_GUIDES_GOLDEN,        /*< desc="Golden sections" >*/
  CLAY_GUIDES_DIAGONALS,     /*< desc="Diagonal lines"  >*/
  CLAY_GUIDES_N_LINES,       /*< desc="Number of lines" >*/
  CLAY_GUIDES_SPACING        /*< desc="Line spacing"    >*/
} ClayGuidesType;


#define CLAY_TYPE_HANDLE_TYPE (clay_handle_type_get_type ())

GType clay_handle_type_get_type (void);

typedef enum
{
  CLAY_HANDLE_SQUARE,
  CLAY_HANDLE_DASHED_SQUARE,
  CLAY_HANDLE_FILLED_SQUARE,
  CLAY_HANDLE_CIRCLE,
  CLAY_HANDLE_DASHED_CIRCLE,
  CLAY_HANDLE_FILLED_CIRCLE,
  CLAY_HANDLE_DIAMOND,
  CLAY_HANDLE_DASHED_DIAMOND,
  CLAY_HANDLE_FILLED_DIAMOND,
  CLAY_HANDLE_CROSS,
  CLAY_HANDLE_CROSSHAIR,
  CLAY_HANDLE_DROP,
  CLAY_HANDLE_FILLED_DROP
} ClayHandleType;


#define CLAY_TYPE_HANDLE_ANCHOR (clay_handle_anchor_get_type ())

GType clay_handle_anchor_get_type (void);

typedef enum
{
  CLAY_HANDLE_ANCHOR_CENTER,
  CLAY_HANDLE_ANCHOR_NORTH,
  CLAY_HANDLE_ANCHOR_NORTH_WEST,
  CLAY_HANDLE_ANCHOR_NORTH_EAST,
  CLAY_HANDLE_ANCHOR_SOUTH,
  CLAY_HANDLE_ANCHOR_SOUTH_WEST,
  CLAY_HANDLE_ANCHOR_SOUTH_EAST,
  CLAY_HANDLE_ANCHOR_WEST,
  CLAY_HANDLE_ANCHOR_EAST
} ClayHandleAnchor;


#define CLAY_TYPE_LIMIT_TYPE (clay_limit_type_get_type ())

GType clay_limit_type_get_type (void);

typedef enum
{
  CLAY_LIMIT_CIRCLE,
  CLAY_LIMIT_SQUARE,
  CLAY_LIMIT_DIAMOND,
  CLAY_LIMIT_HORIZONTAL,
  CLAY_LIMIT_VERTICAL
} ClayLimitType;


#define CLAY_TYPE_PATH_STYLE (clay_path_style_get_type ())

GType clay_path_style_get_type (void);

typedef enum
{
  CLAY_PATH_STYLE_DEFAULT,
  CLAY_PATH_STYLE_PATH,
  CLAY_PATH_STYLE_OUTLINE
} ClayPathStyle;


#define CLAY_TYPE_RECTANGLE_CONSTRAINT (clay_rectangle_constraint_get_type ())

GType clay_rectangle_constraint_get_type (void);

typedef enum
{
  CLAY_RECTANGLE_CONSTRAIN_NONE,
  CLAY_RECTANGLE_CONSTRAIN_IMAGE,
  CLAY_RECTANGLE_CONSTRAIN_DRAWABLE
} ClayRectangleConstraint;


#define CLAY_TYPE_RECTANGLE_FIXED_RULE (clay_rectangle_fixed_rule_get_type ())

GType clay_rectangle_fixed_rule_get_type (void);

typedef enum
{
  CLAY_RECTANGLE_FIXED_ASPECT, /*< desc="Aspect ratio" >*/
  CLAY_RECTANGLE_FIXED_WIDTH,  /*< desc="Width"        >*/
  CLAY_RECTANGLE_FIXED_HEIGHT, /*< desc="Height"       >*/
  CLAY_RECTANGLE_FIXED_SIZE,   /*< desc="Size"         >*/
} ClayRectangleFixedRule;


#define CLAY_TYPE_RECTANGLE_PRECISION (clay_rectangle_precision_get_type ())

GType clay_rectangle_precision_get_type (void);

typedef enum
{
  CLAY_RECTANGLE_PRECISION_INT,
  CLAY_RECTANGLE_PRECISION_DOUBLE,
} ClayRectanglePrecision;


#define CLAY_TYPE_TRANSFORM_3D_MODE (clay_transform_3d_mode_get_type ())

GType clay_transform_3d_mode_get_type (void);

typedef enum /*< lowercase_name=clay_transform_3d_mode >*/
{
  CLAY_TRANSFORM_3D_MODE_CAMERA,
  CLAY_TRANSFORM_3D_MODE_MOVE,
  CLAY_TRANSFORM_3D_MODE_ROTATE
} ClayTransform3DMode;


#define CLAY_TYPE_TRANSFORM_FUNCTION (clay_transform_function_get_type ())

GType clay_transform_function_get_type (void);

typedef enum
{
  CLAY_TRANSFORM_FUNCTION_NONE,
  CLAY_TRANSFORM_FUNCTION_MOVE,
  CLAY_TRANSFORM_FUNCTION_SCALE,
  CLAY_TRANSFORM_FUNCTION_ROTATE,
  CLAY_TRANSFORM_FUNCTION_SHEAR,
  CLAY_TRANSFORM_FUNCTION_PERSPECTIVE
} ClayTransformFunction;


#define CLAY_TYPE_TRANSFORM_HANDLE_MODE (clay_transform_handle_mode_get_type ())

GType clay_transform_handle_mode_get_type (void);

typedef enum
{
  CLAY_HANDLE_MODE_ADD_TRANSFORM, /*< desc="Add / Transform" >*/
  CLAY_HANDLE_MODE_MOVE,          /*< desc="Move"            >*/
  CLAY_HANDLE_MODE_REMOVE         /*< desc="Remove"          >*/
} ClayTransformHandleMode;


#define CLAY_TYPE_PATH_MODE (clay_path_mode_get_type ())

GType clay_path_mode_get_type (void);

typedef enum
{
  CLAY_PATH_MODE_DESIGN,      /*< desc="Design" >*/
  CLAY_PATH_MODE_EDIT,        /*< desc="Edit"   >*/
  CLAY_PATH_MODE_MOVE         /*< desc="Move"   >*/
} ClayPathMode;


#define CLAY_TYPE_ZOOM_FOCUS (clay_zoom_focus_get_type ())

GType clay_zoom_focus_get_type (void);

typedef enum
{
  /* Make a best guess */
  CLAY_ZOOM_FOCUS_BEST_GUESS,

  /* Use the mouse cursor (if within canvas) */
  CLAY_ZOOM_FOCUS_POINTER,

  /* Use the image center */
  CLAY_ZOOM_FOCUS_IMAGE_CENTER,

  /* If the image is centered, retain the centering. Else use
   * _BEST_GUESS
   */
  CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS

} ClayZoomFocus;


#define CLAY_TYPE_MODIFIER_ACTION (clay_modifier_action_get_type ())

GType clay_modifier_action_get_type (void);

typedef enum
{
  CLAY_MODIFIER_ACTION_NONE,                    /*< desc="No action"                                  >*/
  CLAY_MODIFIER_ACTION_PANNING,                 /*< desc="Pan"                                        >*/
  CLAY_MODIFIER_ACTION_ZOOMING,                 /*< desc="Zoom"                                       >*/
  CLAY_MODIFIER_ACTION_ROTATING,                /*< desc="Rotate View"                                >*/
  CLAY_MODIFIER_ACTION_STEP_ROTATING,           /*< desc="Rotate View by 15 degree steps"             >*/
  CLAY_MODIFIER_ACTION_LAYER_PICKING,           /*< desc="Pick a layer"                               >*/

  CLAY_MODIFIER_ACTION_MENU,                    /*< desc="Display the menu"                           >*/

  CLAY_MODIFIER_ACTION_ACTION,                  /*< desc="Custom action"                              >*/

  CLAY_MODIFIER_ACTION_BRUSH_PIXEL_SIZE,        /*< desc="Change brush size in canvas pixels"         >*/
  CLAY_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, /*< desc="Change brush radius' size in canvas pixels" >*/
  CLAY_MODIFIER_ACTION_TOOL_OPACITY,            /*< desc="Change tool opacity"                        >*/
} ClayModifierAction;


/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_HIT_NONE,
  CLAY_HIT_INDIRECT,
  CLAY_HIT_DIRECT
} ClayHit;
