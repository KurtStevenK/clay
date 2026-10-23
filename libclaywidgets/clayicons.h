/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayicons.h
 * Copyright (C) 2001-2015 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_ICONS_H__
#define __CLAY_ICONS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  random actions that don't fit in any category  */

#define CLAY_ICON_ATTACH                    "clay-attach"
#define CLAY_ICON_DETACH                    "clay-detach"
#define CLAY_ICON_INVERT                    "clay-invert"
#define CLAY_ICON_RECORD                    "media-record"
#define CLAY_ICON_RESET                     "clay-reset"
#define CLAY_ICON_SHRED                     "clay-shred"


/*  random states/things that don't fit in any category  */

#define CLAY_ICON_BUSINESS_CARD             "clay-business-card"
#define CLAY_ICON_CHAR_PICKER               "clay-char-picker"
#define CLAY_ICON_CURSOR                    "clay-cursor"
#define CLAY_ICON_DISPLAY                   "clay-display"
#define CLAY_ICON_GEGL                      "clay-gegl"
#define CLAY_ICON_LINKED                    "clay-linked"
#define CLAY_ICON_MARKER                    "clay-marker"
#define CLAY_ICON_SMARTPHONE                "clay-smartphone"
#define CLAY_ICON_TRANSPARENCY              "clay-transparency"
#define CLAY_ICON_VIDEO                     "clay-video"
#define CLAY_ICON_VISIBLE                   "clay-visible"
#define CLAY_ICON_WEB                       "clay-web"


/*  random objects/entities that don't fit in any category  */

#define CLAY_ICON_BRUSH                     CLAY_ICON_TOOL_PAINTBRUSH
#define CLAY_ICON_BUFFER                    CLAY_ICON_EDIT_PASTE
#define CLAY_ICON_COLORMAP                  "clay-colormap"
#define CLAY_ICON_DYNAMICS                  "clay-dynamics"
#define CLAY_ICON_FILE_MANAGER              "clay-file-manager"
#define CLAY_ICON_FONT                      "gtk-select-font"
#define CLAY_ICON_GRADIENT                  CLAY_ICON_TOOL_GRADIENT
#define CLAY_ICON_GRID                      "clay-grid"
#define CLAY_ICON_INPUT_DEVICE              "clay-input-device"
#define CLAY_ICON_MYPAINT_BRUSH             CLAY_ICON_TOOL_MYPAINT_BRUSH
#define CLAY_ICON_PALETTE                   "gtk-select-color"
#define CLAY_ICON_PATTERN                   "clay-pattern"
#define CLAY_ICON_PLUGIN                    "clay-plugin"
#define CLAY_ICON_SAMPLE_POINT              "clay-sample-point"
#define CLAY_ICON_SYMMETRY                  "clay-symmetry"
#define CLAY_ICON_TEMPLATE                  "clay-template"
#define CLAY_ICON_TOOL_PRESET               "clay-tool-preset"


/*  not really icons  */

#define CLAY_ICON_FRAME                     "clay-frame"
#define CLAY_ICON_TEXTURE                   "clay-texture"


/*  icons that follow, or at least try to follow the FDO naming and
 *  category conventions; and groups of icons with a common prefix;
 *  all sorted alphabetically
 *
 *  see also:
 *  https://specifications.freedesktop.org/icon-naming-spec/latest/ar01s04.html
 *
 *  When icons are available as standard Freedesktop icons, we use these
 *  in priority. As a fallback, we use standard GTK+ icons. As last
 *  fallback, we create our own icons under the "clay-" namespace.
 */

#define CLAY_ICON_APPLICATION_EXIT          "application-exit"

#define CLAY_ICON_ASPECT_PORTRAIT           "clay-portrait"
#define CLAY_ICON_ASPECT_LANDSCAPE          "clay-landscape"

#define CLAY_ICON_CAP_BUTT                  "clay-cap-butt"
#define CLAY_ICON_CAP_ROUND                 "clay-cap-round"
#define CLAY_ICON_CAP_SQUARE                "clay-cap-square"

#define CLAY_ICON_CENTER                    "clay-center"
#define CLAY_ICON_CENTER_HORIZONTAL         "clay-hcenter"
#define CLAY_ICON_CENTER_VERTICAL           "clay-vcenter"

#define CLAY_ICON_CHAIN_HORIZONTAL          "clay-hchain"
#define CLAY_ICON_CHAIN_HORIZONTAL_BROKEN   "clay-hchain-broken"
#define CLAY_ICON_CHAIN_VERTICAL            "clay-vchain"
#define CLAY_ICON_CHAIN_VERTICAL_BROKEN     "clay-vchain-broken"

#define CLAY_ICON_CHANNEL                   "clay-channel"
#define CLAY_ICON_CHANNEL_ALPHA             "clay-channel-alpha"
#define CLAY_ICON_CHANNEL_BLUE              "clay-channel-blue"
#define CLAY_ICON_CHANNEL_GRAY              "clay-channel-gray"
#define CLAY_ICON_CHANNEL_GREEN             "clay-channel-green"
#define CLAY_ICON_CHANNEL_INDEXED           "clay-channel-indexed"
#define CLAY_ICON_CHANNEL_RED               "clay-channel-red"

#define CLAY_ICON_CLOSE                     "clay-close"
#define CLAY_ICON_CLOSE_ALL                 "clay-close-all"

#define CLAY_ICON_COLOR_PICKER_BLACK        "clay-color-picker-black"
#define CLAY_ICON_COLOR_PICKER_GRAY         "clay-color-picker-gray"
#define CLAY_ICON_COLOR_PICKER_WHITE        "clay-color-picker-white"
#define CLAY_ICON_COLOR_PICK_FROM_SCREEN    "clay-color-pick-from-screen"

#define CLAY_ICON_COLOR_SELECTOR_CMYK       "clay-color-cmyk"
#define CLAY_ICON_COLOR_SELECTOR_TRIANGLE   "clay-color-triangle"
#define CLAY_ICON_COLOR_SELECTOR_WATER      "clay-color-water"

#define CLAY_ICON_COLOR_SPACE_LINEAR        "clay-color-space-linear"
#define CLAY_ICON_COLOR_SPACE_NON_LINEAR    "clay-color-space-non-linear"
#define CLAY_ICON_COLOR_SPACE_PERCEPTUAL    "clay-color-space-perceptual"

#define CLAY_ICON_COLORS_DEFAULT            "clay-default-colors"
#define CLAY_ICON_COLORS_SWAP               "clay-swap-colors"

#define CLAY_ICON_CONTROLLER                "clay-controller"
#define CLAY_ICON_CONTROLLER_KEYBOARD       "clay-controller-keyboard"
#define CLAY_ICON_CONTROLLER_LINUX_INPUT    "clay-controller-linux-input"
#define CLAY_ICON_CONTROLLER_MIDI           "clay-controller-midi"
#define CLAY_ICON_CONTROLLER_MOUSE          CLAY_ICON_CURSOR
#define CLAY_ICON_CONTROLLER_WHEEL          "clay-controller-wheel"

#define CLAY_ICON_CONVERT_RGB               "clay-convert-rgb"
#define CLAY_ICON_CONVERT_GRAYSCALE         "clay-convert-grayscale"
#define CLAY_ICON_CONVERT_INDEXED           "clay-convert-indexed"
#define CLAY_ICON_CONVERT_PRECISION         CLAY_ICON_CONVERT_RGB

#define CLAY_ICON_CURVE_FREE                "clay-curve-free"
#define CLAY_ICON_CURVE_SMOOTH              "clay-curve-smooth"

#define CLAY_ICON_DIALOG_CHANNELS           "clay-channels"
#define CLAY_ICON_DIALOG_DASHBOARD          "clay-dashboard"
#define CLAY_ICON_DIALOG_DEVICE_STATUS      "clay-device-status"
#define CLAY_ICON_DIALOG_ERROR              "dialog-error"
#define CLAY_ICON_DIALOG_IMAGES             "clay-images"
#define CLAY_ICON_DIALOG_INFORMATION        "dialog-information"
#define CLAY_ICON_DIALOG_LAYERS             "clay-layers"
#define CLAY_ICON_DIALOG_NAVIGATION         "clay-navigation"
#define CLAY_ICON_DIALOG_PATHS              "clay-paths"
#define CLAY_ICON_DIALOG_QUESTION           "dialog-question"
#define CLAY_ICON_DIALOG_RESHOW_FILTER      "clay-reshow-filter"
#define CLAY_ICON_DIALOG_TOOLS              "clay-tools"
#define CLAY_ICON_DIALOG_TOOL_OPTIONS       "clay-tool-options"
#define CLAY_ICON_DIALOG_UNDO_HISTORY       "clay-undo-history"
#define CLAY_ICON_DIALOG_WARNING            "dialog-warning"

#define CLAY_ICON_DISPLAY_FILTER              "clay-display-filter"
#define CLAY_ICON_DISPLAY_FILTER_CLIP_WARNING "clay-display-filter-clip-warning"
#define CLAY_ICON_DISPLAY_FILTER_COLORBLIND   "clay-display-filter-colorblind"
#define CLAY_ICON_DISPLAY_FILTER_CONTRAST     "clay-display-filter-contrast"
#define CLAY_ICON_DISPLAY_FILTER_GAMMA        "clay-display-filter-gamma"
#define CLAY_ICON_DISPLAY_FILTER_LCMS         "clay-display-filter-lcms"
#define CLAY_ICON_DISPLAY_FILTER_PROOF        "clay-display-filter-proof"

#define CLAY_ICON_LOCK                      "clay-lock"
#define CLAY_ICON_LOCK_ALPHA                "clay-lock-alpha"
#define CLAY_ICON_LOCK_CONTENT              "clay-lock-content"
#define CLAY_ICON_LOCK_PATH                 "clay-lock-path"
#define CLAY_ICON_LOCK_POSITION             "clay-lock-position"
#define CLAY_ICON_LOCK_VISIBILITY           "clay-lock-visibility"
#define CLAY_ICON_LOCK_MULTI                "clay-lock-multi"

#define CLAY_ICON_DOCUMENT_NEW              "document-new"
#define CLAY_ICON_DOCUMENT_OPEN             "document-open"
#define CLAY_ICON_DOCUMENT_OPEN_RECENT      "document-open-recent"
#define CLAY_ICON_DOCUMENT_PAGE_SETUP       "document-page-setup"
#define CLAY_ICON_DOCUMENT_PRINT            "document-print"
#define CLAY_ICON_DOCUMENT_PRINT_RESOLUTION "document-print"
#define CLAY_ICON_DOCUMENT_PROPERTIES       "document-properties"
#define CLAY_ICON_DOCUMENT_REVERT           "document-revert"
#define CLAY_ICON_DOCUMENT_SAVE             "document-save"
#define CLAY_ICON_DOCUMENT_SAVE_AS          "document-save-as"

#define CLAY_ICON_EDIT                      "gtk-edit"
#define CLAY_ICON_EDIT_CLEAR                "edit-clear"
#define CLAY_ICON_EDIT_COPY                 "edit-copy"
#define CLAY_ICON_EDIT_CUT                  "edit-cut"
#define CLAY_ICON_EDIT_DELETE               "edit-delete"
#define CLAY_ICON_EDIT_FIND                 "edit-find"
#define CLAY_ICON_EDIT_PASTE                "edit-paste"
#define CLAY_ICON_EDIT_PASTE_AS_NEW         "clay-paste-as-new"
#define CLAY_ICON_EDIT_PASTE_INTO           "clay-paste-into"
#define CLAY_ICON_EDIT_REDO                 "edit-redo"
#define CLAY_ICON_EDIT_UNDO                 "edit-undo"

#define CLAY_ICON_EFFECT                    "clay-effects"

#define CLAY_ICON_EVEN_HORIZONTAL_GAP       "clay-even-horizontal-gap"
#define CLAY_ICON_EVEN_VERTICAL_GAP         "clay-even-vertical-gap"

#define CLAY_ICON_FILL_HORIZONTAL           "clay-hfill"
#define CLAY_ICON_FILL_VERTICAL             "clay-vfill"

#define CLAY_ICON_FOLDER_NEW                "folder-new"

#define CLAY_ICON_FORMAT_INDENT_MORE         "format-indent-more"
#define CLAY_ICON_FORMAT_INDENT_LESS         "format-indent-less"
#define CLAY_ICON_FORMAT_JUSTIFY_CENTER      "format-justify-center"
#define CLAY_ICON_FORMAT_JUSTIFY_FILL        "format-justify-fill"
#define CLAY_ICON_FORMAT_JUSTIFY_LEFT        "format-justify-left"
#define CLAY_ICON_FORMAT_JUSTIFY_RIGHT       "format-justify-right"
#define CLAY_ICON_FORMAT_TEXT_BOLD           "format-text-bold"
#define CLAY_ICON_FORMAT_TEXT_ITALIC         "format-text-italic"
#define CLAY_ICON_FORMAT_TEXT_STRIKETHROUGH  "format-text-strikethrough"
#define CLAY_ICON_FORMAT_TEXT_UNDERLINE      "format-text-underline"
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_LTR  "format-text-direction-ltr"
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_RTL  "format-text-direction-rtl"
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL           "clay-text-dir-ttb-rtl" /* use FDO */
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT   "clay-text-dir-ttb-rtl-upright" /* use FDO */
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR           "clay-text-dir-ttb-ltr" /* use FDO */
#define CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT   "clay-text-dir-ttb-ltr-upright" /* use FDO */
#define CLAY_ICON_FORMAT_TEXT_SPACING_LETTER "clay-letter-spacing"
#define CLAY_ICON_FORMAT_TEXT_SPACING_LINE   "clay-line-spacing"

#define CLAY_ICON_GRADIENT_LINEAR               "clay-gradient-linear"
#define CLAY_ICON_GRADIENT_BILINEAR             "clay-gradient-bilinear"
#define CLAY_ICON_GRADIENT_RADIAL               "clay-gradient-radial"
#define CLAY_ICON_GRADIENT_SQUARE               "clay-gradient-square"
#define CLAY_ICON_GRADIENT_CONICAL_SYMMETRIC    "clay-gradient-conical-symmetric"
#define CLAY_ICON_GRADIENT_CONICAL_ASYMMETRIC   "clay-gradient-conical-asymmetric"
#define CLAY_ICON_GRADIENT_SHAPEBURST_ANGULAR   "clay-gradient-shapeburst-angular"
#define CLAY_ICON_GRADIENT_SHAPEBURST_SPHERICAL "clay-gradient-shapeburst-spherical"
#define CLAY_ICON_GRADIENT_SHAPEBURST_DIMPLED   "clay-gradient-shapeburst-dimpled"
#define CLAY_ICON_GRADIENT_SPIRAL_CLOCKWISE     "clay-gradient-spiral-clockwise"
#define CLAY_ICON_GRADIENT_SPIRAL_ANTICLOCKWISE "clay-gradient-spiral-anticlockwise"

#define CLAY_ICON_GRAVITY_EAST              "clay-gravity-east"
#define CLAY_ICON_GRAVITY_NORTH             "clay-gravity-north"
#define CLAY_ICON_GRAVITY_NORTH_EAST        "clay-gravity-north-east"
#define CLAY_ICON_GRAVITY_NORTH_WEST        "clay-gravity-north-west"
#define CLAY_ICON_GRAVITY_SOUTH             "clay-gravity-south"
#define CLAY_ICON_GRAVITY_SOUTH_EAST        "clay-gravity-south-east"
#define CLAY_ICON_GRAVITY_SOUTH_WEST        "clay-gravity-south-west"
#define CLAY_ICON_GRAVITY_WEST              "clay-gravity-west"

#define CLAY_ICON_GO_BOTTOM                 "go-bottom"
#define CLAY_ICON_GO_DOWN                   "go-down"
#define CLAY_ICON_GO_FIRST                  "go-first"
#define CLAY_ICON_GO_HOME                   "go-home"
#define CLAY_ICON_GO_LAST                   "go-last"
#define CLAY_ICON_GO_TOP                    "go-top"
#define CLAY_ICON_GO_UP                     "go-up"
#define CLAY_ICON_GO_PREVIOUS               "go-previous"
#define CLAY_ICON_GO_NEXT                   "go-next"

#define CLAY_ICON_HELP                      "system-help"
#define CLAY_ICON_HELP_ABOUT                "help-about"
#define CLAY_ICON_HELP_USER_MANUAL          "clay-user-manual"

#define CLAY_ICON_HISTOGRAM                 "clay-histogram"
#define CLAY_ICON_HISTOGRAM_LINEAR          "clay-histogram-linear"
#define CLAY_ICON_HISTOGRAM_LOGARITHMIC     "clay-histogram-logarithmic"

#define CLAY_ICON_IMAGE                     "clay-image"
#define CLAY_ICON_IMAGE_OPEN                "clay-image-open"
#define CLAY_ICON_IMAGE_RELOAD              "clay-image-reload"

#define CLAY_ICON_JOIN_MITER                "clay-join-miter"
#define CLAY_ICON_JOIN_ROUND                "clay-join-round"
#define CLAY_ICON_JOIN_BEVEL                "clay-join-bevel"

#define CLAY_ICON_LAYER                     "clay-layer"
#define CLAY_ICON_LAYER_ANCHOR              "clay-anchor"
#define CLAY_ICON_LAYER_FLOATING_SELECTION  "clay-floating-selection"
/* TODO: create "clay-link-layer" */
#define CLAY_ICON_LAYER_LINK_LAYER          "emblem-symbolic-link"
#define CLAY_ICON_LAYER_MASK                "clay-layer-mask"
#define CLAY_ICON_LAYER_MERGE_DOWN          "clay-merge-down"
#define CLAY_ICON_LAYER_TEXT_LAYER          "clay-text-layer"
#define CLAY_ICON_LAYER_TO_IMAGESIZE        "clay-layer-to-imagesize"
/* TODO: create "clay-vector-layer" */
#define CLAY_ICON_LAYER_VECTOR_LAYER        "clay-tool-path"

#define CLAY_ICON_LIST                      "clay-list"
#define CLAY_ICON_LIST_ADD                  "list-add"
#define CLAY_ICON_LIST_REMOVE               "list-remove"

#define CLAY_ICON_MENU_LEFT                 "clay-menu-left"
#define CLAY_ICON_MENU_RIGHT                "clay-menu-right"

#define CLAY_ICON_OBJECT_DUPLICATE          "clay-duplicate"
#define CLAY_ICON_OBJECT_FLIP_HORIZONTAL    "object-flip-horizontal"
#define CLAY_ICON_OBJECT_FLIP_VERTICAL      "object-flip-vertical"
#define CLAY_ICON_OBJECT_RESIZE             "clay-resize"
#define CLAY_ICON_OBJECT_ROTATE_180         "clay-rotate-180"
#define CLAY_ICON_OBJECT_ROTATE_270         "object-rotate-left"
#define CLAY_ICON_OBJECT_ROTATE_90          "object-rotate-right"
#define CLAY_ICON_OBJECT_SCALE              "clay-scale"

#define CLAY_ICON_PATH                      "clay-path"
#define CLAY_ICON_PATH_STROKE               "clay-path-stroke"

#define CLAY_ICON_PIVOT_CENTER              "clay-pivot-center"
#define CLAY_ICON_PIVOT_EAST                "clay-pivot-east"
#define CLAY_ICON_PIVOT_NORTH               "clay-pivot-north"
#define CLAY_ICON_PIVOT_NORTH_EAST          "clay-pivot-north-east"
#define CLAY_ICON_PIVOT_NORTH_WEST          "clay-pivot-north-west"
#define CLAY_ICON_PIVOT_SOUTH               "clay-pivot-south"
#define CLAY_ICON_PIVOT_SOUTH_EAST          "clay-pivot-south-east"
#define CLAY_ICON_PIVOT_SOUTH_WEST          "clay-pivot-south-west"
#define CLAY_ICON_PIVOT_WEST                "clay-pivot-west"

#define CLAY_ICON_PREFERENCES_SYSTEM        "preferences-system"

#define CLAY_ICON_PROCESS_STOP              "process-stop"

#define CLAY_ICON_QUICK_MASK_OFF            "clay-quick-mask-off"
#define CLAY_ICON_QUICK_MASK_ON             "clay-quick-mask-on"

#define CLAY_ICON_SELECTION                 "clay-selection"
#define CLAY_ICON_SELECTION_ADD             "clay-selection-add"
#define CLAY_ICON_SELECTION_ALL             "clay-selection-all"
#define CLAY_ICON_SELECTION_BORDER          "clay-selection-border"
#define CLAY_ICON_SELECTION_GROW            "clay-selection-grow"
#define CLAY_ICON_SELECTION_INTERSECT       "clay-selection-intersect"
#define CLAY_ICON_SELECTION_NONE            "clay-selection-none"
#define CLAY_ICON_SELECTION_REPLACE         "clay-selection-replace"
#define CLAY_ICON_SELECTION_SHRINK          "clay-selection-shrink"
#define CLAY_ICON_SELECTION_STROKE          "clay-selection-stroke"
#define CLAY_ICON_SELECTION_SUBTRACT        "clay-selection-subtract"
#define CLAY_ICON_SELECTION_TO_CHANNEL      "clay-selection-to-channel"
#define CLAY_ICON_SELECTION_TO_PATH         "clay-selection-to-path"

#define CLAY_ICON_SHAPE_CIRCLE              "clay-shape-circle"
#define CLAY_ICON_SHAPE_DIAMOND             "clay-shape-diamond"
#define CLAY_ICON_SHAPE_SQUARE              "clay-shape-square"

#define CLAY_ICON_SYSTEM_RUN                "system-run"

#define CLAY_ICON_TOOL_AIRBRUSH             "clay-tool-airbrush"
#define CLAY_ICON_TOOL_ALIGN                "clay-tool-align"
#define CLAY_ICON_TOOL_BLUR                 "clay-tool-blur"
#define CLAY_ICON_TOOL_BRIGHTNESS_CONTRAST  "clay-tool-brightness-contrast"
#define CLAY_ICON_TOOL_BUCKET_FILL          "clay-tool-bucket-fill"
#define CLAY_ICON_TOOL_BY_COLOR_SELECT      "clay-tool-by-color-select"
#define CLAY_ICON_TOOL_CAGE                 "clay-tool-cage"
#define CLAY_ICON_TOOL_CLONE                "clay-tool-clone"
#define CLAY_ICON_TOOL_COLORIZE             "clay-tool-colorize"
#define CLAY_ICON_TOOL_COLOR_BALANCE        "clay-tool-color-balance"
#define CLAY_ICON_TOOL_COLOR_PICKER         "clay-tool-color-picker"
#define CLAY_ICON_TOOL_COLOR_TEMPERATURE    "clay-tool-color-temperature"
#define CLAY_ICON_TOOL_CROP                 "clay-tool-crop"
#define CLAY_ICON_TOOL_CURVES               "clay-tool-curves"
#define CLAY_ICON_TOOL_DESATURATE           "clay-tool-desaturate"
#define CLAY_ICON_TOOL_DODGE                "clay-tool-dodge"
#define CLAY_ICON_TOOL_ELLIPSE_SELECT       "clay-tool-ellipse-select"
#define CLAY_ICON_TOOL_ERASER               "clay-tool-eraser"
#define CLAY_ICON_TOOL_EXPOSURE             "clay-tool-exposure"
#define CLAY_ICON_TOOL_FLIP                 "clay-tool-flip"
#define CLAY_ICON_TOOL_FOREGROUND_SELECT    "clay-tool-foreground-select"
#define CLAY_ICON_TOOL_FREE_SELECT          "clay-tool-free-select"
#define CLAY_ICON_TOOL_FUZZY_SELECT         "clay-tool-fuzzy-select"
#define CLAY_ICON_TOOL_GRADIENT             "clay-tool-gradient"
#define CLAY_ICON_TOOL_HANDLE_TRANSFORM     "clay-tool-handle-transform"
#define CLAY_ICON_TOOL_HEAL                 "clay-tool-heal"
#define CLAY_ICON_TOOL_HUE_SATURATION       "clay-tool-hue-saturation"
#define CLAY_ICON_TOOL_INK                  "clay-tool-ink"
#define CLAY_ICON_TOOL_ISCISSORS            "clay-tool-iscissors"
#define CLAY_ICON_TOOL_LEVELS               "clay-tool-levels"
#define CLAY_ICON_TOOL_MEASURE              "clay-tool-measure"
#define CLAY_ICON_TOOL_MOVE                 "clay-tool-move"
#define CLAY_ICON_TOOL_MYPAINT_BRUSH        "clay-tool-mypaint-brush"
#define CLAY_ICON_TOOL_N_POINT_DEFORMATION  "clay-tool-n-point-deformation"
#define CLAY_ICON_TOOL_OFFSET               "clay-tool-offset"
#define CLAY_ICON_TOOL_PAINTBRUSH           "clay-tool-paintbrush"
#define CLAY_ICON_TOOL_PAINT_SELECT         "clay-tool-paint-select"
#define CLAY_ICON_TOOL_PATH                 "clay-tool-path"
#define CLAY_ICON_TOOL_PENCIL               "clay-tool-pencil"
#define CLAY_ICON_TOOL_PERSPECTIVE          "clay-tool-perspective"
#define CLAY_ICON_TOOL_PERSPECTIVE_CLONE    "clay-tool-perspective-clone"
#define CLAY_ICON_TOOL_POSTERIZE            "clay-tool-posterize"
#define CLAY_ICON_TOOL_RECT_SELECT          "clay-tool-rect-select"
#define CLAY_ICON_TOOL_ROTATE               "clay-tool-rotate"
#define CLAY_ICON_TOOL_SCALE                "clay-tool-scale"
#define CLAY_ICON_TOOL_SEAMLESS_CLONE       "clay-tool-seamless-clone"
#define CLAY_ICON_TOOL_SHADOWS_HIGHLIGHTS   "clay-tool-shadows-highlights"
#define CLAY_ICON_TOOL_SHEAR                "clay-tool-shear"
#define CLAY_ICON_TOOL_SMUDGE               "clay-tool-smudge"
#define CLAY_ICON_TOOL_TEXT                 "clay-tool-text"
#define CLAY_ICON_TOOL_THRESHOLD            "clay-tool-threshold"
#define CLAY_ICON_TOOL_TRANSFORM_3D         "clay-tool-transform-3d"
#define CLAY_ICON_TOOL_UNIFIED_TRANSFORM    "clay-tool-unified-transform"
#define CLAY_ICON_TOOL_WARP                 "clay-tool-warp"
#define CLAY_ICON_TOOL_ZOOM                 "clay-tool-zoom"

#define CLAY_ICON_TRANSFORM_3D_CAMERA       "clay-transform-3d-camera"
#define CLAY_ICON_TRANSFORM_3D_MOVE         "clay-transform-3d-move"
#define CLAY_ICON_TRANSFORM_3D_ROTATE       "clay-transform-3d-rotate"

#define CLAY_ICON_VIEW_REFRESH              "view-refresh"
#define CLAY_ICON_VIEW_FULLSCREEN           "view-fullscreen"
#define CLAY_ICON_VIEW_SHRINK_WRAP          "view-shrink-wrap"
#define CLAY_ICON_VIEW_ZOOM_FILL            "view-zoom-fill"

#define CLAY_ICON_WILBER                    "clay-wilber"
#define CLAY_ICON_WILBER_EEK                "clay-wilber-eek"

#define CLAY_ICON_WINDOW_CLOSE              "window-close"
#define CLAY_ICON_WINDOW_MOVE_TO_SCREEN     "clay-move-to-screen"
#define CLAY_ICON_WINDOW_NEW                "window-new"

#define CLAY_ICON_ZOOM_IN                   "zoom-in"
#define CLAY_ICON_ZOOM_ORIGINAL             "zoom-original"
#define CLAY_ICON_ZOOM_OUT                  "zoom-out"
#define CLAY_ICON_ZOOM_FIT_BEST             "zoom-fit-best"
#define CLAY_ICON_ZOOM_FOLLOW_WINDOW        "clay-zoom-follow-window"


void     clay_icons_init           (void);

gboolean clay_icons_set_icon_theme (GFile *path);


G_END_DECLS

#endif /* __CLAY_ICONS_H__ */
