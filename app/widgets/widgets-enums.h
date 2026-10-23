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


/*
 * enums that are registered with the type system
 */

#define CLAY_TYPE_ACTIVE_COLOR (clay_active_color_get_type ())

GType clay_active_color_get_type (void);

typedef enum
{
  CLAY_ACTIVE_COLOR_FOREGROUND, /*< desc="Foreground" >*/
  CLAY_ACTIVE_COLOR_BACKGROUND  /*< desc="Background" >*/
} ClayActiveColor;


#define CLAY_TYPE_CIRCLE_BACKGROUND (clay_circle_background_get_type ())

GType clay_circle_background_get_type (void);

typedef enum
{
  CLAY_CIRCLE_BACKGROUND_PLAIN, /*< desc="Plain" >*/
  CLAY_CIRCLE_BACKGROUND_HSV    /*< desc="HSV"   >*/
} ClayCircleBackground;


#define CLAY_TYPE_COLOR_DIALOG_STATE (clay_color_dialog_state_get_type ())

GType clay_color_dialog_state_get_type (void);

typedef enum
{
  CLAY_COLOR_DIALOG_OK,
  CLAY_COLOR_DIALOG_CANCEL,
  CLAY_COLOR_DIALOG_UPDATE
} ClayColorDialogState;


#define CLAY_TYPE_COLOR_PICK_TARGET (clay_color_pick_target_get_type ())

GType clay_color_pick_target_get_type (void);

typedef enum
{
  CLAY_COLOR_PICK_TARGET_NONE,       /*< desc="Pick only"            >*/
  CLAY_COLOR_PICK_TARGET_FOREGROUND, /*< desc="Set foreground color" >*/
  CLAY_COLOR_PICK_TARGET_BACKGROUND, /*< desc="Set background color" >*/
  CLAY_COLOR_PICK_TARGET_PALETTE     /*< desc="Add to palette"       >*/
} ClayColorPickTarget;


#define CLAY_TYPE_COLOR_PICK_STATE (clay_color_pick_state_get_type ())

GType clay_color_pick_state_get_type (void);

typedef enum
{
  CLAY_COLOR_PICK_STATE_START,
  CLAY_COLOR_PICK_STATE_UPDATE,
  CLAY_COLOR_PICK_STATE_END
} ClayColorPickState;


#define CLAY_TYPE_HISTOGRAM_SCALE (clay_histogram_scale_get_type ())

GType clay_histogram_scale_get_type (void);

typedef enum
{
  CLAY_HISTOGRAM_SCALE_LINEAR,       /*< desc="Linear histogram"      >*/
  CLAY_HISTOGRAM_SCALE_LOGARITHMIC   /*< desc="Logarithmic histogram" >*/
} ClayHistogramScale;


#define CLAY_TYPE_TAB_STYLE (clay_tab_style_get_type ())

GType clay_tab_style_get_type (void);

typedef enum
{
  CLAY_TAB_STYLE_ICON,          /*< desc="Icon"           >*/
  CLAY_TAB_STYLE_PREVIEW,       /*< desc="Current status" >*/
  CLAY_TAB_STYLE_NAME,          /*< desc="Text"           >*/
  CLAY_TAB_STYLE_BLURB,         /*< desc="Description"    >*/
  CLAY_TAB_STYLE_ICON_NAME,     /*< desc="Icon & text"    >*/
  CLAY_TAB_STYLE_ICON_BLURB,    /*< desc="Icon & desc"    >*/
  CLAY_TAB_STYLE_PREVIEW_NAME,  /*< desc="Status & text"  >*/
  CLAY_TAB_STYLE_PREVIEW_BLURB  /*< desc="Status & desc"  >*/
} ClayTabStyle;


#define CLAY_TYPE_TAG_ENTRY_MODE       (clay_tag_entry_mode_get_type ())

GType clay_tag_entry_mode_get_type (void);

typedef enum
{
  CLAY_TAG_ENTRY_MODE_QUERY,
  CLAY_TAG_ENTRY_MODE_ASSIGN,
} ClayTagEntryMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip >*/
{
  CLAY_VIEW_BG_CHECKS,
  CLAY_VIEW_BG_WHITE,
  CLAY_VIEW_BG_STYLE
} ClayViewBG;

typedef enum  /*< skip >*/
{
  CLAY_VIEW_BORDER_BLACK,
  CLAY_VIEW_BORDER_WHITE,
  CLAY_VIEW_BORDER_RED,
  CLAY_VIEW_BORDER_GREEN,
  CLAY_VIEW_BORDER_STYLE_FG,
  CLAY_VIEW_BORDER_STYLE_BG
} ClayViewBorderType;

typedef enum  /*< skip >*/
{
  CLAY_DND_TYPE_NONE         = 0,
  CLAY_DND_TYPE_URI_LIST     = 1,
  CLAY_DND_TYPE_TEXT_PLAIN   = 2,
  CLAY_DND_TYPE_NETSCAPE_URL = 3,
  CLAY_DND_TYPE_XDS          = 4,
  CLAY_DND_TYPE_COLOR        = 5,
  CLAY_DND_TYPE_SVG          = 6,
  CLAY_DND_TYPE_SVG_XML      = 7,
  CLAY_DND_TYPE_PIXBUF       = 8,
  CLAY_DND_TYPE_IMAGE        = 9,
  CLAY_DND_TYPE_COMPONENT    = 10,
  CLAY_DND_TYPE_LAYER        = 11,
  CLAY_DND_TYPE_CHANNEL      = 12,
  CLAY_DND_TYPE_LAYER_MASK   = 13,
  CLAY_DND_TYPE_PATH         = 14,
  CLAY_DND_TYPE_BRUSH        = 15,
  CLAY_DND_TYPE_PATTERN      = 16,
  CLAY_DND_TYPE_GRADIENT     = 17,
  CLAY_DND_TYPE_PALETTE      = 18,
  CLAY_DND_TYPE_FONT         = 19,
  CLAY_DND_TYPE_BUFFER       = 20,
  CLAY_DND_TYPE_IMAGEFILE    = 21,
  CLAY_DND_TYPE_TEMPLATE     = 22,
  CLAY_DND_TYPE_TOOL_ITEM    = 23,
  CLAY_DND_TYPE_NOTEBOOK_TAB = 24,

  CLAY_DND_TYPE_LAYER_LIST   = 25,
  CLAY_DND_TYPE_CHANNEL_LIST = 26,
  CLAY_DND_TYPE_PATH_LIST    = 27,

  CLAY_DND_TYPE_LAST         = CLAY_DND_TYPE_PATH_LIST
} ClayDndType;

typedef enum  /*< skip >*/
{
  CLAY_DROP_NONE,
  CLAY_DROP_ABOVE,
  CLAY_DROP_BELOW
} ClayDropType;

typedef enum  /*< skip >*/
{
  CLAY_CURSOR_NONE = 1024,  /* (GDK_LAST_CURSOR + 2) yes, this is insane */
  CLAY_CURSOR_MOUSE,
  CLAY_CURSOR_CROSSHAIR,
  CLAY_CURSOR_CROSSHAIR_SMALL,
  CLAY_CURSOR_BAD,
  CLAY_CURSOR_MOVE,
  CLAY_CURSOR_ZOOM,
  CLAY_CURSOR_COLOR_PICKER,
  CLAY_CURSOR_CORNER_TOP,
  CLAY_CURSOR_CORNER_TOP_RIGHT,
  CLAY_CURSOR_CORNER_RIGHT,
  CLAY_CURSOR_CORNER_BOTTOM_RIGHT,
  CLAY_CURSOR_CORNER_BOTTOM,
  CLAY_CURSOR_CORNER_BOTTOM_LEFT,
  CLAY_CURSOR_CORNER_LEFT,
  CLAY_CURSOR_CORNER_TOP_LEFT,
  CLAY_CURSOR_SIDE_TOP,
  CLAY_CURSOR_SIDE_TOP_RIGHT,
  CLAY_CURSOR_SIDE_RIGHT,
  CLAY_CURSOR_SIDE_BOTTOM_RIGHT,
  CLAY_CURSOR_SIDE_BOTTOM,
  CLAY_CURSOR_SIDE_BOTTOM_LEFT,
  CLAY_CURSOR_SIDE_LEFT,
  CLAY_CURSOR_SIDE_TOP_LEFT,
  CLAY_CURSOR_SINGLE_DOT,
  CLAY_CURSOR_LAST
} ClayCursorType;

typedef enum  /*< skip >*/
{
  CLAY_TOOL_CURSOR_NONE,
  CLAY_TOOL_CURSOR_RECT_SELECT,
  CLAY_TOOL_CURSOR_ELLIPSE_SELECT,
  CLAY_TOOL_CURSOR_FREE_SELECT,
  CLAY_TOOL_CURSOR_POLYGON_SELECT,
  CLAY_TOOL_CURSOR_FUZZY_SELECT,
  CLAY_TOOL_CURSOR_PATHS,
  CLAY_TOOL_CURSOR_PATHS_ANCHOR,
  CLAY_TOOL_CURSOR_PATHS_CONTROL,
  CLAY_TOOL_CURSOR_PATHS_SEGMENT,
  CLAY_TOOL_CURSOR_ISCISSORS,
  CLAY_TOOL_CURSOR_MOVE,
  CLAY_TOOL_CURSOR_ZOOM,
  CLAY_TOOL_CURSOR_CROP,
  CLAY_TOOL_CURSOR_RESIZE,
  CLAY_TOOL_CURSOR_ROTATE,
  CLAY_TOOL_CURSOR_SHEAR,
  CLAY_TOOL_CURSOR_PERSPECTIVE,
  CLAY_TOOL_CURSOR_TRANSFORM_3D_CAMERA,
  CLAY_TOOL_CURSOR_FLIP_HORIZONTAL,
  CLAY_TOOL_CURSOR_FLIP_VERTICAL,
  CLAY_TOOL_CURSOR_TEXT,
  CLAY_TOOL_CURSOR_COLOR_PICKER,
  CLAY_TOOL_CURSOR_BUCKET_FILL,
  CLAY_TOOL_CURSOR_GRADIENT,
  CLAY_TOOL_CURSOR_PENCIL,
  CLAY_TOOL_CURSOR_PAINTBRUSH,
  CLAY_TOOL_CURSOR_AIRBRUSH,
  CLAY_TOOL_CURSOR_INK,
  CLAY_TOOL_CURSOR_CLONE,
  CLAY_TOOL_CURSOR_HEAL,
  CLAY_TOOL_CURSOR_ERASER,
  CLAY_TOOL_CURSOR_SMUDGE,
  CLAY_TOOL_CURSOR_BLUR,
  CLAY_TOOL_CURSOR_DODGE,
  CLAY_TOOL_CURSOR_BURN,
  CLAY_TOOL_CURSOR_MEASURE,
  CLAY_TOOL_CURSOR_WARP,
  CLAY_TOOL_CURSOR_HAND,
  CLAY_TOOL_CURSOR_LAST
} ClayToolCursorType;

typedef enum  /*< skip >*/
{
  CLAY_CURSOR_MODIFIER_NONE,
  CLAY_CURSOR_MODIFIER_BAD,
  CLAY_CURSOR_MODIFIER_PLUS,
  CLAY_CURSOR_MODIFIER_MINUS,
  CLAY_CURSOR_MODIFIER_INTERSECT,
  CLAY_CURSOR_MODIFIER_MOVE,
  CLAY_CURSOR_MODIFIER_RESIZE,
  CLAY_CURSOR_MODIFIER_ROTATE,
  CLAY_CURSOR_MODIFIER_ZOOM,
  CLAY_CURSOR_MODIFIER_CONTROL,
  CLAY_CURSOR_MODIFIER_ANCHOR,
  CLAY_CURSOR_MODIFIER_FOREGROUND,
  CLAY_CURSOR_MODIFIER_BACKGROUND,
  CLAY_CURSOR_MODIFIER_PATTERN,
  CLAY_CURSOR_MODIFIER_JOIN,
  CLAY_CURSOR_MODIFIER_SELECT,
  CLAY_CURSOR_MODIFIER_LAST
} ClayCursorModifier;

typedef enum  /*< skip >*/
{
  CLAY_DEVICE_VALUE_MODE       = 1 << 0,
  CLAY_DEVICE_VALUE_AXES       = 1 << 1,
  CLAY_DEVICE_VALUE_KEYS       = 1 << 2,
  CLAY_DEVICE_VALUE_TOOL       = 1 << 3,
  CLAY_DEVICE_VALUE_FOREGROUND = 1 << 4,
  CLAY_DEVICE_VALUE_BACKGROUND = 1 << 5,
  CLAY_DEVICE_VALUE_BRUSH      = 1 << 6,
  CLAY_DEVICE_VALUE_PATTERN    = 1 << 7,
  CLAY_DEVICE_VALUE_GRADIENT   = 1 << 8
} ClayDeviceValues;

typedef enum  /*< skip >*/
{
  CLAY_DIALOGS_SHOWN,
  CLAY_DIALOGS_HIDDEN_EXPLICITLY,  /* user used the Tab key to hide dialogs */
  CLAY_DIALOGS_HIDDEN_WITH_DISPLAY /* dialogs are hidden with the display   */
} ClayDialogsState;

typedef enum  /*< skip >*/
{
  CLAY_DASHBOARD_UPDATE_INTERVAL_0_25_SEC =    250,
  CLAY_DASHBOARD_UPDATE_INTERVAL_0_5_SEC  =    500,
  CLAY_DASHBOARD_UPDATE_INTERVAL_1_SEC    =   1000,
  CLAY_DASHBOARD_UPDATE_INTERVAL_2_SEC    =   2000,
  CLAY_DASHBOARD_UPDATE_INTERVAL_4_SEC    =   4000
} ClayDashboardUpdateInteval;

typedef enum  /*< skip >*/
{
  CLAY_DASHBOARD_HISTORY_DURATION_15_SEC  =  15000,
  CLAY_DASHBOARD_HISTORY_DURATION_30_SEC  =  30000,
  CLAY_DASHBOARD_HISTORY_DURATION_60_SEC  =  60000,
  CLAY_DASHBOARD_HISTORY_DURATION_120_SEC = 120000,
  CLAY_DASHBOARD_HISTORY_DURATION_240_SEC = 240000
} ClayDashboardHistoryDuration;
