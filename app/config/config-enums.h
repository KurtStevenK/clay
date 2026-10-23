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


#define CLAY_TYPE_CANVAS_PADDING_MODE (clay_canvas_padding_mode_get_type ())

GType clay_canvas_padding_mode_get_type (void);

typedef enum
{
  CLAY_CANVAS_PADDING_MODE_DEFAULT,      /*< desc="From theme"        >*/
  CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK,  /*< desc="Light check color" >*/
  CLAY_CANVAS_PADDING_MODE_DARK_CHECK,   /*< desc="Dark check color"  >*/
  CLAY_CANVAS_PADDING_MODE_CUSTOM,       /*< desc="Custom color"      >*/
  CLAY_CANVAS_PADDING_MODE_RESET = -1    /*< skip >*/
} ClayCanvasPaddingMode;


#define CLAY_TYPE_CURSOR_MODE (clay_cursor_mode_get_type ())

GType clay_cursor_mode_get_type (void);

typedef enum
{
  CLAY_CURSOR_MODE_TOOL_ICON,       /*< desc="Tool icon"                >*/
  CLAY_CURSOR_MODE_TOOL_CROSSHAIR,  /*< desc="Tool icon with crosshair" >*/
  CLAY_CURSOR_MODE_CROSSHAIR,       /*< desc="Crosshair only"           >*/
} ClayCursorMode;


#define CLAY_TYPE_EXPORT_FILE_TYPE (clay_export_file_type_get_type ())

GType clay_export_file_type_get_type (void);

typedef enum
{
  CLAY_EXPORT_FILE_PNG,  /*< desc="PNG Image"                >*/
  CLAY_EXPORT_FILE_JPG,  /*< desc="JPEG Image"               >*/
  CLAY_EXPORT_FILE_ORA,  /*< desc="OpenRaster Image"         >*/
  CLAY_EXPORT_FILE_PSD,  /*< desc="Photoshop Image"          >*/
  CLAY_EXPORT_FILE_PDF,  /*< desc="Portable Document Format" >*/
  CLAY_EXPORT_FILE_TIF,  /*< desc="TIFF Image"               >*/
  CLAY_EXPORT_FILE_BMP,  /*< desc="Windows BMP Image"        >*/
  CLAY_EXPORT_FILE_WEBP, /*< desc="WebP Image"               >*/
} ClayExportFileType;


#define CLAY_TYPE_HANDEDNESS (clay_handedness_get_type ())

GType clay_handedness_get_type (void);

typedef enum
{
  CLAY_HANDEDNESS_LEFT, /*< desc="Left-handed"  >*/
  CLAY_HANDEDNESS_RIGHT /*< desc="Right-handed" >*/
} ClayHandedness;


#define CLAY_TYPE_ICON_SIZE (clay_icon_size_get_type ())

GType clay_icon_size_get_type (void);

typedef enum
{
  CLAY_ICON_SIZE_SMALL,   /*< desc="Small size"       > */
  CLAY_ICON_SIZE_MEDIUM,  /*< desc="Medium size"      > */
  CLAY_ICON_SIZE_LARGE,   /*< desc="Large size"       > */
  CLAY_ICON_SIZE_HUGE     /*< desc="Huge size"        > */
} ClayIconSize;


#define CLAY_TYPE_POSITION (clay_position_get_type ())

GType clay_position_get_type (void);

typedef enum
{
  CLAY_POSITION_TOP,    /*< desc="Top" >*/
  CLAY_POSITION_BOTTOM, /*< desc="Bottom" >*/
  CLAY_POSITION_LEFT,   /*< desc="Left"  >*/
  CLAY_POSITION_RIGHT   /*< desc="Right" >*/
} ClayPosition;

#define CLAY_TYPE_DRAG_ZOOM_MODE (clay_drag_zoom_mode_get_type ())

GType clay_drag_zoom_mode_get_type (void);

typedef enum
{
  PROP_DRAG_ZOOM_MODE_DISTANCE,  /*< desc="By distance" >*/
  PROP_DRAG_ZOOM_MODE_DURATION,  /*< desc="By duration" >*/
} ClayDragZoomMode;

#define CLAY_TYPE_SPACE_BAR_ACTION (clay_space_bar_action_get_type ())

GType clay_space_bar_action_get_type (void);

typedef enum
{
  CLAY_SPACE_BAR_ACTION_NONE,  /*< desc="No action"           >*/
  CLAY_SPACE_BAR_ACTION_PAN,   /*< desc="Pan view"            >*/
  CLAY_SPACE_BAR_ACTION_MOVE   /*< desc="Switch to Move tool" >*/
} ClaySpaceBarAction;


#define CLAY_TYPE_WINDOW_HINT (clay_window_hint_get_type ())

GType clay_window_hint_get_type (void);

typedef enum
{
  CLAY_WINDOW_HINT_NORMAL,     /*< desc="Normal window"  >*/
  CLAY_WINDOW_HINT_UTILITY,    /*< desc="Utility window" >*/
  CLAY_WINDOW_HINT_KEEP_ABOVE  /*< desc="Keep above"     >*/
} ClayWindowHint;


#define CLAY_TYPE_ZOOM_QUALITY (clay_zoom_quality_get_type ())

GType clay_zoom_quality_get_type (void);

typedef enum
{
  CLAY_ZOOM_QUALITY_LOW,   /*< desc="Low"  >*/
  CLAY_ZOOM_QUALITY_HIGH   /*< desc="High" >*/
} ClayZoomQuality;

#define CLAY_TYPE_THEME_SCHEME (clay_theme_scheme_get_type ())

GType clay_theme_scheme_get_type (void);

typedef enum
{
  CLAY_THEME_LIGHT,  /*< desc="Light Colors"  >*/
  CLAY_THEME_GRAY,   /*< desc="Dim Colors"    >*/
  CLAY_THEME_DARK,   /*< desc="Dark Colors"   >*/
  CLAY_THEME_SYSTEM, /*< desc="System Colors" >*/
  CLAY_THEME_BROWN, /*< desc="Brown" >*/
  CLAY_THEME_PARCHMENT, /*< desc="Parchment" >*/
  CLAY_THEME_CAPPUCCINO, /*< desc="Cappuccino" >*/
  CLAY_THEME_ESPRESSO, /*< desc="Espresso" >*/
  CLAY_THEME_MIDNIGHT, /*< desc="Midnight" >*/
  CLAY_THEME_FOREST, /*< desc="Forest" >*/
  CLAY_THEME_ROSE, /*< desc="Rose" >*/
  CLAY_THEME_SUNSET, /*< desc="Sunset" >*/
  CLAY_THEME_VIOLET, /*< desc="Violet" >*/
  CLAY_THEME_NEON, /*< desc="Neon" >*/
  CLAY_THEME_MATRIX, /*< desc="Matrix" >*/
  CLAY_THEME_UBUNTU, /*< desc="Ubuntu" >*/
  CLAY_THEME_DRACULA, /*< desc="Dracula" >*/
  CLAY_THEME_NORD, /*< desc="Nord" >*/
  CLAY_THEME_SOLARIZED, /*< desc="Solarized" >*/
  CLAY_THEME_GRUVBOX, /*< desc="Gruvbox" >*/
  CLAY_THEME_OCEAN, /*< desc="Ocean" >*/
  CLAY_THEME_SAKURA, /*< desc="Sakura" >*/
  CLAY_THEME_AMBER, /*< desc="Amber" >*/
  CLAY_THEME_SEPIA, /*< desc="Sepia" >*/
} ClayThemeScheme;
