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
 * these enums are registered with the type system
 */

/**
 * ClayBucketFillArea:
 * @CLAY_BUCKET_FILL_SELECTION:      Fill whole selection
 * @CLAY_BUCKET_FILL_SIMILAR_COLORS: Fill similar colors
 * @CLAY_BUCKET_FILL_LINE_ART:       Fill by line art detection
 *
 * Bucket fill area.
 */
#define CLAY_TYPE_BUCKET_FILL_AREA (clay_bucket_fill_area_get_type ())

GType clay_bucket_fill_area_get_type (void);

typedef enum
{
  CLAY_BUCKET_FILL_SELECTION,       /*< desc="Fill whole selection" >*/
  CLAY_BUCKET_FILL_SIMILAR_COLORS,  /*< desc="Fill similar colors" >*/
  CLAY_BUCKET_FILL_LINE_ART         /*< desc="Fill by line art detection" >*/
} ClayBucketFillArea;


/**
 * ClayLineArtSource:
 * @CLAY_LINE_ART_SOURCE_SAMPLE_MERGED: All visible layers
 * @CLAY_LINE_ART_SOURCE_ACTIVE_LAYER:  Active layer
 * @CLAY_LINE_ART_SOURCE_LOWER_LAYER:   Layer below the active one
 * @CLAY_LINE_ART_SOURCE_UPPER_LAYER:   Layer above the active one
 *
 * Bucket fill area.
 */
#define CLAY_TYPE_LINE_ART_SOURCE (clay_line_art_source_get_type ())

GType clay_line_art_source_get_type (void);

typedef enum
{
  CLAY_LINE_ART_SOURCE_SAMPLE_MERGED, /*< desc="All visible layers" >*/
  CLAY_LINE_ART_SOURCE_ACTIVE_LAYER,  /*< desc="Selected layer" >*/
  CLAY_LINE_ART_SOURCE_LOWER_LAYER,   /*< desc="Layer below the selected one" >*/
  CLAY_LINE_ART_SOURCE_UPPER_LAYER    /*< desc="Layer above the selected one" >*/
} ClayLineArtSource;


#define CLAY_TYPE_RECT_SELECT_MODE (clay_rect_select_mode_get_type ())

GType clay_rect_select_mode_get_type (void);

typedef enum
{
  CLAY_RECT_SELECT_MODE_FREE,        /*< desc="Free select"        >*/
  CLAY_RECT_SELECT_MODE_FIXED_SIZE,  /*< desc="Fixed size"         >*/
  CLAY_RECT_SELECT_MODE_FIXED_RATIO  /*< desc="Fixed aspect ratio" >*/
} ClayRectSelectMode;


#define CLAY_TYPE_TRANSFORM_TYPE (clay_transform_type_get_type ())

GType clay_transform_type_get_type (void);

typedef enum
{
  CLAY_TRANSFORM_TYPE_LAYER,     /*< desc="Layer"     >*/
  CLAY_TRANSFORM_TYPE_SELECTION, /*< desc="Selection" >*/
  CLAY_TRANSFORM_TYPE_PATH,      /*< desc="Path"      >*/
  CLAY_TRANSFORM_TYPE_IMAGE      /*< desc="Image"     >*/
} ClayTransformType;


#define CLAY_TYPE_TOOL_ACTION (clay_tool_action_get_type ())

GType clay_tool_action_get_type (void);

typedef enum
{
  CLAY_TOOL_ACTION_PAUSE,
  CLAY_TOOL_ACTION_RESUME,
  CLAY_TOOL_ACTION_HALT,
  CLAY_TOOL_ACTION_COMMIT
} ClayToolAction;


#define CLAY_TYPE_TOOL_ACTIVE_MODIFIERS (clay_tool_active_modifiers_get_type ())

GType clay_tool_active_modifiers_get_type (void);

typedef enum
{
  CLAY_TOOL_ACTIVE_MODIFIERS_OFF,
  CLAY_TOOL_ACTIVE_MODIFIERS_SAME,
  CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE,
} ClayToolActiveModifiers;


#define CLAY_TYPE_MATTING_DRAW_MODE (clay_matting_draw_mode_get_type ())

GType clay_matting_draw_mode_get_type (void);

typedef enum
{
 CLAY_MATTING_DRAW_MODE_FOREGROUND,   /*< desc="Draw foreground" >*/
 CLAY_MATTING_DRAW_MODE_BACKGROUND,   /*< desc="Draw background" >*/
 CLAY_MATTING_DRAW_MODE_UNKNOWN,      /*< desc="Draw unknown" >*/
} ClayMattingDrawMode;


#define CLAY_TYPE_MATTING_PREVIEW_MODE (clay_matting_preview_mode_get_type ())

GType clay_matting_preview_mode_get_type (void);

typedef enum
{
 CLAY_MATTING_PREVIEW_MODE_ON_COLOR,        /*< desc="Color" >*/
 CLAY_MATTING_PREVIEW_MODE_GRAYSCALE,       /*< desc="Grayscale" >*/
} ClayMattingPreviewMode;


#define CLAY_TYPE_TRANSFORM_3D_LENS_MODE (clay_transform_3d_lens_mode_get_type ())

GType clay_transform_3d_lens_mode_get_type (void);

typedef enum /*< lowercase_name=clay_transform_3d_lens_mode >*/
{
  CLAY_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, /*< desc="Focal length" >*/
  CLAY_TRANSFORM_3D_LENS_MODE_FOV_IMAGE,    /*< desc="Field of view (relative to image)", abbrev="FOV (image)" >*/
  CLAY_TRANSFORM_3D_LENS_MODE_FOV_ITEM,     /*< desc="Field of view (relative to item)", abbrev="FOV (item)" >*/
} Clay3DTransformLensMode;


#define CLAY_TYPE_WARP_BEHAVIOR (clay_warp_behavior_get_type ())

GType clay_warp_behavior_get_type (void);

typedef enum
{
  CLAY_WARP_BEHAVIOR_MOVE,      /*< desc="Move pixels" >*/
  CLAY_WARP_BEHAVIOR_GROW,      /*< desc="Grow area" >*/
  CLAY_WARP_BEHAVIOR_SHRINK,    /*< desc="Shrink area" >*/
  CLAY_WARP_BEHAVIOR_SWIRL_CW,  /*< desc="Swirl clockwise" >*/
  CLAY_WARP_BEHAVIOR_SWIRL_CCW, /*< desc="Swirl counter-clockwise" >*/
  CLAY_WARP_BEHAVIOR_ERASE,     /*< desc="Erase warping" >*/
  CLAY_WARP_BEHAVIOR_SMOOTH     /*< desc="Smooth warping" >*/
} ClayWarpBehavior;


/*
 * non-registered enums; register them if needed
 */

typedef enum /*< skip >*/
{
  SELECTION_SELECT,
  SELECTION_MOVE_MASK,
  SELECTION_MOVE,
  SELECTION_MOVE_COPY,
  SELECTION_ANCHOR
} SelectFunction;

/*  Modes of ClayEditSelectionTool  */
typedef enum /*< skip >*/
{
  CLAY_TRANSLATE_MODE_PATH,
  CLAY_TRANSLATE_MODE_CHANNEL,
  CLAY_TRANSLATE_MODE_LAYER_MASK,
  CLAY_TRANSLATE_MODE_MASK,
  CLAY_TRANSLATE_MODE_MASK_TO_LAYER,
  CLAY_TRANSLATE_MODE_MASK_COPY_TO_LAYER,
  CLAY_TRANSLATE_MODE_LAYER,
  CLAY_TRANSLATE_MODE_FLOATING_SEL
} ClayTranslateMode;

/*  Motion event report modes  */
typedef enum /*< skip >*/
{
  CLAY_MOTION_MODE_EXACT,
  CLAY_MOTION_MODE_COMPRESS
} ClayMotionMode;
