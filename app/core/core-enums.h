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

#if 0
   This file is parsed by two scripts, enumgen.pl in pdb,
   and clay-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libclay and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * these enums are registered with the type system
 */


#define CLAY_TYPE_ALIGN_REFERENCE_TYPE (clay_align_reference_type_get_type ())

GType clay_align_reference_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_ALIGN_REFERENCE_IMAGE,          /*< desc="Image"                   >*/
  CLAY_ALIGN_REFERENCE_SELECTION,      /*< desc="Selection"               >*/
  CLAY_ALIGN_REFERENCE_PICK,           /*< desc="Picked reference object" >*/
} ClayAlignReferenceType;


#define CLAY_TYPE_ALIGNMENT_TYPE (clay_alignment_type_get_type ())

GType clay_alignment_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_ALIGN_LEFT,                     /*< desc="Align to the left"                                 >*/
  CLAY_ALIGN_HCENTER,                  /*< desc="Center horizontally"                               >*/
  CLAY_ALIGN_RIGHT,                    /*< desc="Align to the right"                                >*/
  CLAY_ALIGN_TOP,                      /*< desc="Align to the top"                                  >*/
  CLAY_ALIGN_VCENTER,                  /*< desc="Center vertically"                                 >*/
  CLAY_ALIGN_BOTTOM,                   /*< desc="Align to the bottom"                               >*/
  CLAY_ARRANGE_HFILL,                  /*< desc="Distribute anchor points horizontally evenly"      >*/
  CLAY_ARRANGE_VFILL,                  /*< desc="Distribute anchor points vertically evenly"        >*/
  CLAY_DISTRIBUTE_EVEN_HORIZONTAL_GAP, /*< desc="Distribute horizontally with even horizontal gaps" >*/
  CLAY_DISTRIBUTE_EVEN_VERTICAL_GAP,   /*< desc="Distribute vertically with even vertical gaps"     >*/
} ClayAlignmentType;


/**
 * ClayBucketFillMode:
 * @CLAY_BUCKET_FILL_FG:      FG color fill
 * @CLAY_BUCKET_FILL_BG:      BG color fill
 * @CLAY_BUCKET_FILL_PATTERN: Pattern fill
 *
 * Bucket fill modes.
 */
#define CLAY_TYPE_BUCKET_FILL_MODE (clay_bucket_fill_mode_get_type ())

GType clay_bucket_fill_mode_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_BUCKET_FILL_FG,      /*< desc="FG color fill" >*/
  CLAY_BUCKET_FILL_BG,      /*< desc="BG color fill" >*/
  CLAY_BUCKET_FILL_PATTERN  /*< desc="Pattern fill"  >*/
} ClayBucketFillMode;


#define CLAY_TYPE_CHANNEL_BORDER_STYLE (clay_channel_border_style_get_type ())

GType clay_channel_border_style_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_CHANNEL_BORDER_STYLE_HARD,     /*< desc="Hard"      >*/
  CLAY_CHANNEL_BORDER_STYLE_SMOOTH,   /*< desc="Smooth"    >*/
  CLAY_CHANNEL_BORDER_STYLE_FEATHERED /*< desc="Feathered" >*/
} ClayChannelBorderStyle;


/*  Note: when appending values here, don't forget to update
 *  ClayColorFrame and other places use this enum to create combo
 *  boxes.
 */
#define CLAY_TYPE_COLOR_PICK_MODE (clay_color_pick_mode_get_type ())

GType clay_color_pick_mode_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_COLOR_PICK_MODE_PIXEL,       /*< desc="Pixel"        >*/
  CLAY_COLOR_PICK_MODE_RGB_PERCENT, /*< desc="RGB (%)"      >*/
  CLAY_COLOR_PICK_MODE_RGB_U8,      /*< desc="RGB (0..255)" >*/
  CLAY_COLOR_PICK_MODE_GRAYSCALE,   /*< desc="Grayscale (%)">*/
  CLAY_COLOR_PICK_MODE_HSV,         /*< desc="HSV"          >*/
  CLAY_COLOR_PICK_MODE_LCH,         /*< desc="CIE LCh"      >*/
  CLAY_COLOR_PICK_MODE_LAB,         /*< desc="CIE LAB"      >*/
  CLAY_COLOR_PICK_MODE_CMYK,        /*< desc="CMYK"         >*/
  CLAY_COLOR_PICK_MODE_XYY,         /*< desc="CIE xyY"      >*/
  CLAY_COLOR_PICK_MODE_YUV,         /*< desc="CIE Yu'v'"      >*/

  CLAY_COLOR_PICK_MODE_LAST = CLAY_COLOR_PICK_MODE_YUV /*< skip >*/
} ClayColorPickMode;


#define CLAY_TYPE_COLOR_PROFILE_POLICY (clay_color_profile_policy_get_type ())

GType clay_color_profile_policy_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_COLOR_PROFILE_POLICY_ASK,               /*< desc="Ask what to do"                                                          >*/
  CLAY_COLOR_PROFILE_POLICY_KEEP,              /*< desc="Keep embedded profile"                                                   >*/
  CLAY_COLOR_PROFILE_POLICY_CONVERT_BUILTIN,   /*< desc="Convert to built-in sRGB or grayscale profile"                           >*/
  CLAY_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, /*< desc="Convert to preferred RGB or grayscale profile (defaulting to built-in)" >*/
} ClayColorProfilePolicy;


#define CLAY_TYPE_COMPONENT_MASK (clay_component_mask_get_type ())

GType clay_component_mask_get_type (void);

typedef enum /*< pdb-skip >*/
{
  CLAY_COMPONENT_MASK_RED   = 1 << 0,
  CLAY_COMPONENT_MASK_GREEN = 1 << 1,
  CLAY_COMPONENT_MASK_BLUE  = 1 << 2,
  CLAY_COMPONENT_MASK_ALPHA = 1 << 3,

  CLAY_COMPONENT_MASK_ALL = (CLAY_COMPONENT_MASK_RED   |
                             CLAY_COMPONENT_MASK_GREEN |
                             CLAY_COMPONENT_MASK_BLUE  |
                             CLAY_COMPONENT_MASK_ALPHA)
} ClayComponentMask;


#define CLAY_TYPE_CONTAINER_POLICY (clay_container_policy_get_type ())

GType clay_container_policy_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_CONTAINER_POLICY_STRONG,
  CLAY_CONTAINER_POLICY_WEAK
} ClayContainerPolicy;


#define CLAY_TYPE_CONVERT_DITHER_TYPE (clay_convert_dither_type_get_type ())

GType clay_convert_dither_type_get_type (void);

typedef enum
{
  CLAY_CONVERT_DITHER_NONE,        /*< desc="None"                                     >*/
  CLAY_CONVERT_DITHER_FS,          /*< desc="Floyd-Steinberg (normal)"                 >*/
  CLAY_CONVERT_DITHER_FS_LOWBLEED, /*< desc="Floyd-Steinberg (reduced color bleeding)" >*/
  CLAY_CONVERT_DITHER_FIXED,       /*< desc="Positioned"                               >*/
  CLAY_CONVERT_DITHER_NODESTRUCT   /*< pdb-skip, skip >*/
} ClayConvertDitherType;


#define CLAY_TYPE_CONVOLUTION_TYPE (clay_convolution_type_get_type ())

GType clay_convolution_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_NORMAL_CONVOL,      /*  Negative numbers truncated  */
  CLAY_ABSOLUTE_CONVOL,    /*  Absolute value              */
  CLAY_NEGATIVE_CONVOL     /*  add 127 to values           */
} ClayConvolutionType;


#define CLAY_TYPE_CURVE_POINT_TYPE (clay_curve_point_type_get_type ())

GType clay_curve_point_type_get_type (void);

typedef enum
{
  CLAY_CURVE_POINT_SMOOTH,   /*< desc="Smooth" >*/
  CLAY_CURVE_POINT_CORNER    /*< desc="Corner" >*/
} ClayCurvePointType;


#define CLAY_TYPE_CURVE_TYPE (clay_curve_type_get_type ())

GType clay_curve_type_get_type (void);

typedef enum
{
  CLAY_CURVE_SMOOTH,   /*< desc="Smooth"   >*/
  CLAY_CURVE_FREE      /*< desc="Freehand" >*/
} ClayCurveType;


#define CLAY_TYPE_DASH_PRESET (clay_dash_preset_get_type ())

GType clay_dash_preset_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_DASH_CUSTOM,       /*< desc="Custom"         >*/
  CLAY_DASH_LINE,         /*< desc="Line"           >*/
  CLAY_DASH_LONG_DASH,    /*< desc="Long dashes"    >*/
  CLAY_DASH_MEDIUM_DASH,  /*< desc="Medium dashes"  >*/
  CLAY_DASH_SHORT_DASH,   /*< desc="Short dashes"   >*/
  CLAY_DASH_SPARSE_DOTS,  /*< desc="Sparse dots"    >*/
  CLAY_DASH_NORMAL_DOTS,  /*< desc="Normal dots"    >*/
  CLAY_DASH_DENSE_DOTS,   /*< desc="Dense dots"     >*/
  CLAY_DASH_STIPPLES,     /*< desc="Stipples"       >*/
  CLAY_DASH_DASH_DOT,     /*< desc="Dash, dot"      >*/
  CLAY_DASH_DASH_DOT_DOT  /*< desc="Dash, dot, dot" >*/
} ClayDashPreset;


#define CLAY_TYPE_DEBUG_POLICY (clay_debug_policy_get_type ())

GType clay_debug_policy_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_DEBUG_POLICY_WARNING,    /*< desc="Debug warnings, critical errors and crashes" >*/
  CLAY_DEBUG_POLICY_CRITICAL,   /*< desc="Debug critical errors and crashes"           >*/
  CLAY_DEBUG_POLICY_FATAL,      /*< desc="Debug crashes only"                          >*/
  CLAY_DEBUG_POLICY_NEVER       /*< desc="Never debug Clay"                            >*/
} ClayDebugPolicy;


#define CLAY_TYPE_DIRTY_MASK (clay_dirty_mask_get_type ())

GType clay_dirty_mask_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_DIRTY_NONE            = 0,

  CLAY_DIRTY_IMAGE           = 1 << 0,
  CLAY_DIRTY_IMAGE_SIZE      = 1 << 1,
  CLAY_DIRTY_IMAGE_META      = 1 << 2,
  CLAY_DIRTY_IMAGE_STRUCTURE = 1 << 3,
  CLAY_DIRTY_ITEM            = 1 << 4,
  CLAY_DIRTY_ITEM_META       = 1 << 5,
  CLAY_DIRTY_DRAWABLE        = 1 << 6,
  CLAY_DIRTY_PATH            = 1 << 7,
  CLAY_DIRTY_SELECTION       = 1 << 8,
  CLAY_DIRTY_ACTIVE_DRAWABLE = 1 << 9,

  CLAY_DIRTY_ALL             = 0xffff
} ClayDirtyMask;


#define CLAY_TYPE_DYNAMICS_OUTPUT_TYPE (clay_dynamics_output_type_get_type ())

GType clay_dynamics_output_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_DYNAMICS_OUTPUT_OPACITY,      /*< desc="Opacity"      >*/
  CLAY_DYNAMICS_OUTPUT_SIZE,         /*< desc="Size"         >*/
  CLAY_DYNAMICS_OUTPUT_ANGLE,        /*< desc="Angle"        >*/
  CLAY_DYNAMICS_OUTPUT_COLOR,        /*< desc="Color"        >*/
  CLAY_DYNAMICS_OUTPUT_HARDNESS,     /*< desc="Hardness"     >*/
  CLAY_DYNAMICS_OUTPUT_FORCE,        /*< desc="Force"        >*/
  CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO, /*< desc="Aspect ratio" >*/
  CLAY_DYNAMICS_OUTPUT_SPACING,      /*< desc="Spacing"      >*/
  CLAY_DYNAMICS_OUTPUT_RATE,         /*< desc="Rate"         >*/
  CLAY_DYNAMICS_OUTPUT_FLOW,         /*< desc="Flow"         >*/
  CLAY_DYNAMICS_OUTPUT_JITTER,       /*< desc="Jitter"       >*/
} ClayDynamicsOutputType;


#define CLAY_TYPE_CUSTOM_STYLE (clay_custom_style_get_type ())

GType clay_custom_style_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_CUSTOM_STYLE_SOLID_COLOR,  /*< desc="Solid color" >*/
  CLAY_CUSTOM_STYLE_PATTERN       /*< desc="Pattern"          >*/
} ClayCustomStyle;

#define CLAY_TYPE_FILL_STYLE (clay_fill_style_get_type ())

GType clay_fill_style_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_FILL_STYLE_FG_COLOR,  /*< desc="Foreground color" >*/
  CLAY_FILL_STYLE_BG_COLOR,  /*< desc="Background color" >*/
  CLAY_FILL_STYLE_PATTERN    /*< desc="Pattern"          >*/
} ClayFillStyle;


#define CLAY_TYPE_FILTER_REGION (clay_filter_region_get_type ())

GType clay_filter_region_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_FILTER_REGION_SELECTION, /*< desc="Use the selection as input"    >*/
  CLAY_FILTER_REGION_DRAWABLE   /*< desc="Use the entire layer as input" >*/
} ClayFilterRegion;


#define CLAY_TYPE_GRADIENT_COLOR (clay_gradient_color_get_type ())

GType clay_gradient_color_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_GRADIENT_COLOR_FIXED,                  /*< desc="Fixed"                                           >*/
  CLAY_GRADIENT_COLOR_FOREGROUND,             /*< desc="Foreground color",               abbrev="FG"     >*/
  CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, /*< desc="Foreground color (transparent)",  abbrev="FG (t)" >*/
  CLAY_GRADIENT_COLOR_BACKGROUND,             /*< desc="Background color",               abbrev="BG"     >*/
  CLAY_GRADIENT_COLOR_BACKGROUND_TRANSPARENT  /*< desc="Background color (transparent)", abbrev="BG (t)" >*/
} ClayGradientColor;


#define CLAY_TYPE_GRAVITY_TYPE (clay_gravity_type_get_type ())

GType clay_gravity_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_GRAVITY_NONE,
  CLAY_GRAVITY_NORTH_WEST,
  CLAY_GRAVITY_NORTH,
  CLAY_GRAVITY_NORTH_EAST,
  CLAY_GRAVITY_WEST,
  CLAY_GRAVITY_CENTER,
  CLAY_GRAVITY_EAST,
  CLAY_GRAVITY_SOUTH_WEST,
  CLAY_GRAVITY_SOUTH,
  CLAY_GRAVITY_SOUTH_EAST
} ClayGravityType;


#define CLAY_TYPE_GUIDE_STYLE (clay_guide_style_get_type ())

GType clay_guide_style_get_type (void);

typedef enum /*< pdb-skip >*/
{
  CLAY_GUIDE_STYLE_NONE,
  CLAY_GUIDE_STYLE_NORMAL,
  CLAY_GUIDE_STYLE_MIRROR,
  CLAY_GUIDE_STYLE_MANDALA,
  CLAY_GUIDE_STYLE_SPLIT_VIEW
} ClayGuideStyle;


#define CLAY_TYPE_HISTOGRAM_CHANNEL (clay_histogram_channel_get_type ())

GType clay_histogram_channel_get_type (void);

typedef enum
{
  CLAY_HISTOGRAM_VALUE     = 0,  /*< desc="Value"         >*/
  CLAY_HISTOGRAM_RED       = 1,  /*< desc="Red"           >*/
  CLAY_HISTOGRAM_GREEN     = 2,  /*< desc="Green"         >*/
  CLAY_HISTOGRAM_BLUE      = 3,  /*< desc="Blue"          >*/
  CLAY_HISTOGRAM_ALPHA     = 4,  /*< desc="Alpha"         >*/
  CLAY_HISTOGRAM_LUMINANCE = 5,  /*< desc="Luminance"     >*/
  CLAY_HISTOGRAM_RGB       = 6   /*< desc="RGB", pdb-skip >*/
} ClayHistogramChannel;


#define CLAY_TYPE_ITEM_SET (clay_item_set_get_type ())

GType clay_item_set_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_ITEM_SET_NONE,        /*< desc="None"               >*/
  CLAY_ITEM_SET_ALL,         /*< desc="All layers"         >*/
  CLAY_ITEM_SET_IMAGE_SIZED, /*< desc="Image-sized layers" >*/
  CLAY_ITEM_SET_VISIBLE,     /*< desc="All visible layers" >*/
} ClayItemSet;


#define CLAY_TYPE_MATTING_ENGINE (clay_matting_engine_get_type ())

GType clay_matting_engine_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_MATTING_ENGINE_GLOBAL,  /*< desc="Matting Global" >*/
  CLAY_MATTING_ENGINE_LEVIN,   /*< desc="Matting Levin" >*/
} ClayMattingEngine;


#define CLAY_TYPE_MESSAGE_SEVERITY (clay_message_severity_get_type ())

GType clay_message_severity_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_MESSAGE_INFO,        /*< desc="Message"  >*/
  CLAY_MESSAGE_WARNING,     /*< desc="Warning"  >*/
  CLAY_MESSAGE_ERROR,       /*< desc="Error"    >*/
  CLAY_MESSAGE_BUG_WARNING, /*< desc="WARNING"  >*/
  CLAY_MESSAGE_BUG_CRITICAL /*< desc="CRITICAL" >*/
} ClayMessageSeverity;


#define CLAY_TYPE_METADATA_ROTATION_POLICY (clay_metadata_rotation_policy_get_type ())

GType clay_metadata_rotation_policy_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_METADATA_ROTATION_POLICY_ASK,    /*< desc="Ask what to do"                         >*/
  CLAY_METADATA_ROTATION_POLICY_KEEP,   /*< desc="Discard metadata without rotating"      >*/
  CLAY_METADATA_ROTATION_POLICY_ROTATE  /*< desc="Rotate the image then discard metadata" >*/
} ClayMetadataRotationPolicy;


#define CLAY_TYPE_PASTE_TYPE (clay_paste_type_get_type ())

GType clay_paste_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_PASTE_TYPE_FLOATING,
  CLAY_PASTE_TYPE_FLOATING_IN_PLACE,
  CLAY_PASTE_TYPE_FLOATING_INTO,
  CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE,
  CLAY_PASTE_TYPE_NEW_LAYER,
  CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE,
  CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING,
  CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE,
  CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING,
  CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE,
} ClayPasteType;


#define CLAY_TYPE_WIN32_POINTER_INPUT_API (clay_win32_pointer_input_api_get_type ())

GType clay_win32_pointer_input_api_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_WIN32_POINTER_INPUT_API_WINTAB,      /*< desc="Wintab"      >*/
  CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK  /*< desc="Windows Ink" >*/
} ClayWin32PointerInputAPI;


#define CLAY_TYPE_THUMBNAIL_SIZE (clay_thumbnail_size_get_type ())

GType clay_thumbnail_size_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_THUMBNAIL_SIZE_NONE    = 0,    /*< desc="No thumbnails"    >*/
  CLAY_THUMBNAIL_SIZE_NORMAL  = 128,  /*< desc="Normal (128x128)" >*/
  CLAY_THUMBNAIL_SIZE_LARGE   = 256   /*< desc="Large (256x256)"  >*/
} ClayThumbnailSize;


#define CLAY_TYPE_TRC_TYPE (clay_trc_type_get_type ())

GType clay_trc_type_get_type (void);

typedef enum
{
  CLAY_TRC_LINEAR,     /*< desc="Linear"     >*/
  CLAY_TRC_NON_LINEAR, /*< desc="Non-Linear" >*/
  CLAY_TRC_PERCEPTUAL  /*< desc="Perceptual" >*/
} ClayTRCType;


#define CLAY_TYPE_UNDO_EVENT (clay_undo_event_get_type ())

GType clay_undo_event_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_UNDO_EVENT_UNDO_PUSHED,  /* a new undo has been added to the undo stack */
  CLAY_UNDO_EVENT_UNDO_EXPIRED, /* an undo has been freed from the undo stack  */
  CLAY_UNDO_EVENT_REDO_EXPIRED, /* a redo has been freed from the redo stack   */
  CLAY_UNDO_EVENT_UNDO,         /* an undo has been executed                   */
  CLAY_UNDO_EVENT_REDO,         /* a redo has been executed                    */
  CLAY_UNDO_EVENT_UNDO_FREE,    /* all undo and redo info has been cleared     */
  CLAY_UNDO_EVENT_UNDO_FREEZE,  /* undo has been frozen                        */
  CLAY_UNDO_EVENT_UNDO_THAW     /* undo has been thawn                         */
} ClayUndoEvent;


#define CLAY_TYPE_UNDO_MODE (clay_undo_mode_get_type ())

GType clay_undo_mode_get_type (void);

typedef enum /*< pdb-skip >*/
{
  CLAY_UNDO_MODE_UNDO,
  CLAY_UNDO_MODE_REDO
} ClayUndoMode;


#define CLAY_TYPE_UNDO_TYPE (clay_undo_type_get_type ())

GType clay_undo_type_get_type (void);

typedef enum /*< pdb-skip >*/
{
  /* Type NO_UNDO_GROUP (0) is special - in the clayimage structure it
   * means there is no undo group currently being added to.
   */
  CLAY_UNDO_GROUP_NONE = 0,              /*< desc="<<invalid>>"                    >*/

  CLAY_UNDO_GROUP_FIRST = CLAY_UNDO_GROUP_NONE, /*< skip >*/

  CLAY_UNDO_GROUP_IMAGE_SCALE,           /*< desc="Scale image"                    >*/
  CLAY_UNDO_GROUP_IMAGE_RESIZE,          /*< desc="Resize image"                   >*/
  CLAY_UNDO_GROUP_IMAGE_FLIP,            /*< desc="Flip image"                     >*/
  CLAY_UNDO_GROUP_IMAGE_ROTATE,          /*< desc="Rotate image"                   >*/
  CLAY_UNDO_GROUP_IMAGE_TRANSFORM,       /*< desc="Transform image"                >*/
  CLAY_UNDO_GROUP_IMAGE_CROP,            /*< desc="Crop image"                     >*/
  CLAY_UNDO_GROUP_IMAGE_CONVERT,         /*< desc="Convert image"                  >*/
  CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,     /*< desc="Remove item"                    >*/
  CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER,    /*< desc="Reorder item"                   >*/
  CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,    /*< desc="Merge layers"                   >*/
  CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE,     /*< desc="Merge paths"                    >*/
  CLAY_UNDO_GROUP_IMAGE_QUICK_MASK,      /*< desc="Quick Mask"                     >*/
  CLAY_UNDO_GROUP_IMAGE_GRID,            /*< desc="Grid"                           >*/
  CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP,  /*< desc="Colormap remapping"             >*/
  CLAY_UNDO_GROUP_GUIDE,                 /*< desc="Guide"                          >*/
  CLAY_UNDO_GROUP_SAMPLE_POINT,          /*< desc="Sample Point"                   >*/
  CLAY_UNDO_GROUP_DRAWABLE,              /*< desc="Layer/Channel"                  >*/
  CLAY_UNDO_GROUP_DRAWABLE_MOD,          /*< desc="Layer/Channel modification"     >*/
  CLAY_UNDO_GROUP_MASK,                  /*< desc="Selection mask"                 >*/
  CLAY_UNDO_GROUP_ITEM_VISIBILITY,       /*< desc="Item visibility"                >*/
  CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS,    /*< desc="Lock/Unlock contents"           >*/
  CLAY_UNDO_GROUP_ITEM_LOCK_POSITION,    /*< desc="Lock/Unlock position"           >*/
  CLAY_UNDO_GROUP_ITEM_LOCK_VISIBILITY,  /*< desc="Lock/Unlock visibility"         >*/
  CLAY_UNDO_GROUP_ITEM_PROPERTIES,       /*< desc="Item properties"                >*/
  CLAY_UNDO_GROUP_ITEM_DISPLACE,         /*< desc="Move item"                      >*/
  CLAY_UNDO_GROUP_ITEM_SCALE,            /*< desc="Scale item"                     >*/
  CLAY_UNDO_GROUP_ITEM_RESIZE,           /*< desc="Resize item"                    >*/
  CLAY_UNDO_GROUP_LAYER_ADD,             /*< desc="Add layer"                      >*/
  CLAY_UNDO_GROUP_LAYER_ADD_ALPHA,       /*< desc="Add alpha channel"              >*/
  CLAY_UNDO_GROUP_LAYER_ADD_MASK,        /*< desc="Add layer masks"                >*/
  CLAY_UNDO_GROUP_LAYER_APPLY_MASK,      /*< desc="Apply layer masks"              >*/
  CLAY_UNDO_GROUP_LAYER_REMOVE_ALPHA,    /*< desc="Remove alpha channel"           >*/
  CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA,      /*< desc="Lock/Unlock alpha channels"     >*/
  CLAY_UNDO_GROUP_LAYER_OPACITY,         /*< desc="Set layers opacity"             >*/
  CLAY_UNDO_GROUP_LAYER_MODE,            /*< desc="Set layers mode"                >*/
  CLAY_UNDO_GROUP_CHANNEL_ADD,           /*< desc="Add channels"                   >*/
  CLAY_UNDO_GROUP_FS_TO_LAYER,           /*< desc="Floating selection to layer"    >*/
  CLAY_UNDO_GROUP_FS_FLOAT,              /*< desc="Float selection"                >*/
  CLAY_UNDO_GROUP_FS_ANCHOR,             /*< desc="Anchor floating selection"      >*/
  CLAY_UNDO_GROUP_EDIT_PASTE,            /*< desc="Paste"                          >*/
  CLAY_UNDO_GROUP_EDIT_CUT,              /*< desc="Cut"                            >*/
  CLAY_UNDO_GROUP_TEXT,                  /*< desc="Text"                           >*/
  CLAY_UNDO_GROUP_VECTOR,                /*< desc="Vector"                         >*/
  CLAY_UNDO_GROUP_TRANSFORM,             /*< desc="Transform"                      >*/
  CLAY_UNDO_GROUP_PAINT,                 /*< desc="Paint"                          >*/
  CLAY_UNDO_GROUP_PARASITE_ATTACH,       /*< desc="Attach parasite"                >*/
  CLAY_UNDO_GROUP_PARASITE_REMOVE,       /*< desc="Remove parasite"                >*/
  CLAY_UNDO_GROUP_PATHS_IMPORT,          /*< desc="Import paths"                   >*/
  CLAY_UNDO_GROUP_FILTER_VISIBILITY,     /*< desc="Effects visibility"             >*/
  CLAY_UNDO_GROUP_MISC,                  /*< desc="Plug-In"                        >*/

  CLAY_UNDO_GROUP_LAST = CLAY_UNDO_GROUP_MISC, /*< skip >*/

  /*  Undo types which actually do something  */

  CLAY_UNDO_IMAGE_TYPE,                  /*< desc="Image type"                     >*/
  CLAY_UNDO_IMAGE_PRECISION,             /*< desc="Image precision"                >*/
  CLAY_UNDO_IMAGE_SIZE,                  /*< desc="Image size"                     >*/
  CLAY_UNDO_IMAGE_RESOLUTION,            /*< desc="Image resolution change"        >*/
  CLAY_UNDO_IMAGE_GRID,                  /*< desc="Grid"                           >*/
  CLAY_UNDO_IMAGE_METADATA,              /*< desc="Change metadata"                >*/
  CLAY_UNDO_IMAGE_COLORMAP,              /*< desc="Change indexed palette"         >*/
  CLAY_UNDO_IMAGE_HIDDEN_PROFILE,        /*< desc="Hide/Unhide color profile"      >*/
  CLAY_UNDO_GUIDE,                       /*< desc="Guide"                          >*/
  CLAY_UNDO_SAMPLE_POINT,                /*< desc="Sample Point"                   >*/
  CLAY_UNDO_DRAWABLE,                    /*< desc="Layer/Channel"                  >*/
  CLAY_UNDO_DRAWABLE_MOD,                /*< desc="Layer/Channel modification"     >*/
  CLAY_UNDO_DRAWABLE_FORMAT,             /*< desc="Layer/Channel format"           >*/
  CLAY_UNDO_MASK,                        /*< desc="Selection mask"                 >*/
  CLAY_UNDO_ITEM_REORDER,                /*< desc="Reorder item"                   >*/
  CLAY_UNDO_ITEM_RENAME,                 /*< desc="Rename item"                    >*/
  CLAY_UNDO_ITEM_DISPLACE,               /*< desc="Move item"                      >*/
  CLAY_UNDO_ITEM_VISIBILITY,             /*< desc="Item visibility"                >*/
  CLAY_UNDO_ITEM_COLOR_TAG,              /*< desc="Item color tag"                 >*/
  CLAY_UNDO_ITEM_LOCK_CONTENT,           /*< desc="Lock/Unlock content"            >*/
  CLAY_UNDO_ITEM_LOCK_POSITION,          /*< desc="Lock/Unlock position"           >*/
  CLAY_UNDO_ITEM_LOCK_VISIBILITY,        /*< desc="Lock/Unlock visibility"         >*/
  CLAY_UNDO_LAYER_ADD,                   /*< desc="New layer"                      >*/
  CLAY_UNDO_LAYER_REMOVE,                /*< desc="Delete layer"                   >*/
  CLAY_UNDO_LAYER_MODE,                  /*< desc="Set layer mode"                 >*/
  CLAY_UNDO_LAYER_OPACITY,               /*< desc="Set layer opacity"              >*/
  CLAY_UNDO_LAYER_LOCK_ALPHA,            /*< desc="Lock/Unlock alpha channel"      >*/
  CLAY_UNDO_LINK_LAYER,                  /*< desc="Link layer"                     >*/
  CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE,  /*< desc="Suspend group layer resize"     >*/
  CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE,   /*< desc="Resume group layer resize"      >*/
  CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK,    /*< desc="Suspend group layer mask"       >*/
  CLAY_UNDO_GROUP_LAYER_RESUME_MASK,     /*< desc="Resume group layer mask"        >*/
  CLAY_UNDO_GROUP_LAYER_START_TRANSFORM, /*< desc="Start transforming group layer" >*/
  CLAY_UNDO_GROUP_LAYER_END_TRANSFORM,   /*< desc="End transforming group layer"   >*/
  CLAY_UNDO_GROUP_LAYER_CONVERT,         /*< desc="Convert group layer"            >*/
  CLAY_UNDO_TEXT_LAYER,                  /*< desc="Text layer"                     >*/
  CLAY_UNDO_TEXT_LAYER_CONVERT,          /*< desc="Convert text layer"             >*/
  CLAY_UNDO_VECTOR_LAYER,                /*< desc="Vector layer"                   >*/
  CLAY_UNDO_LAYER_MASK_ADD,              /*< desc="Add layer masks"                >*/
  CLAY_UNDO_LAYER_MASK_REMOVE,           /*< desc="Delete layer masks"             >*/
  CLAY_UNDO_LAYER_MASK_APPLY,            /*< desc="Apply layer masks"              >*/
  CLAY_UNDO_LAYER_MASK_SHOW,             /*< desc="Show layer masks"               >*/
  CLAY_UNDO_CHANNEL_ADD,                 /*< desc="New channel"                    >*/
  CLAY_UNDO_CHANNEL_REMOVE,              /*< desc="Delete channel"                 >*/
  CLAY_UNDO_CHANNEL_COLOR,               /*< desc="Channel color"                  >*/
  CLAY_UNDO_PATH_ADD,                    /*< desc="New path"                       >*/
  CLAY_UNDO_PATH_REMOVE,                 /*< desc="Delete path"                    >*/
  CLAY_UNDO_PATH_MOD,                    /*< desc="Path modification"              >*/
  CLAY_UNDO_FS_TO_LAYER,                 /*< desc="Floating selection to layer"    >*/
  CLAY_UNDO_TRANSFORM_GRID,              /*< desc="Transform grid"                 >*/
  CLAY_UNDO_PAINT,                       /*< desc="Paint"                          >*/
  CLAY_UNDO_INK,                         /*< desc="Ink"                            >*/
  CLAY_UNDO_FOREGROUND_SELECT,           /*< desc="Select foreground"              >*/
  CLAY_UNDO_PARASITE_ATTACH,             /*< desc="Attach parasite"                >*/
  CLAY_UNDO_PARASITE_REMOVE,             /*< desc="Remove parasite"                >*/
  CLAY_UNDO_FILTER_ADD,                  /*< desc="Add effect"                     >*/
  CLAY_UNDO_FILTER_REMOVE,               /*< desc="Remove effect"                  >*/
  CLAY_UNDO_FILTER_REORDER,              /*< desc="Reorder effect"                 >*/
  CLAY_UNDO_FILTER_MODIFIED,             /*< desc="Effect modification"            >*/
  CLAY_UNDO_FILTER_VISIBILITY,           /*< desc="Effect visibility"              >*/
  CLAY_UNDO_RASTERIZABLE,                /*< desc="Text, link or vector layer"     >*/

  CLAY_UNDO_CANT                         /*< desc="Not undoable"                   >*/
} ClayUndoType;


#define CLAY_TYPE_VIEW_SIZE (clay_view_size_get_type ())

GType clay_view_size_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_VIEW_SIZE_TINY        = 12,   /*< desc="Tiny"        >*/
  CLAY_VIEW_SIZE_EXTRA_SMALL = 16,   /*< desc="Very small"  >*/
  CLAY_VIEW_SIZE_SMALL       = 24,   /*< desc="Small"       >*/
  CLAY_VIEW_SIZE_MEDIUM      = 32,   /*< desc="Medium"      >*/
  CLAY_VIEW_SIZE_LARGE       = 48,   /*< desc="Large"       >*/
  CLAY_VIEW_SIZE_EXTRA_LARGE = 64,   /*< desc="Very large"  >*/
  CLAY_VIEW_SIZE_HUGE        = 96,   /*< desc="Huge"        >*/
  CLAY_VIEW_SIZE_ENORMOUS    = 128,  /*< desc="Enormous"    >*/
  CLAY_VIEW_SIZE_GIGANTIC    = 192   /*< desc="Gigantic"    >*/
} ClayViewSize;


#define CLAY_TYPE_VIEW_TYPE (clay_view_type_get_type ())

GType clay_view_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_VIEW_TYPE_LIST,  /*< desc="View as list" >*/
  CLAY_VIEW_TYPE_GRID   /*< desc="View as grid" >*/
} ClayViewType;


#define CLAY_TYPE_SELECT_METHOD (clay_select_method_get_type ())

GType clay_select_method_get_type (void);

typedef enum /*< pdb-skip >*/
{
  CLAY_SELECT_PLAIN_TEXT,    /*< desc="Selection by basic text search"         >*/
  CLAY_SELECT_REGEX_PATTERN, /*< desc="Selection by regular expression search" >*/
  CLAY_SELECT_GLOB_PATTERN,  /*< desc="Selection by glob pattern search"       >*/
} ClaySelectMethod;

/*
 * non-registered enums; register them if needed
 */


typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_CONTEXT_PROP_FIRST       =  2,

  CLAY_CONTEXT_PROP_IMAGE       =  CLAY_CONTEXT_PROP_FIRST,
  CLAY_CONTEXT_PROP_DISPLAY     =  3,
  CLAY_CONTEXT_PROP_TOOL        =  4,
  CLAY_CONTEXT_PROP_PAINT_INFO  =  5,
  CLAY_CONTEXT_PROP_FOREGROUND  =  6,
  CLAY_CONTEXT_PROP_BACKGROUND  =  7,
  CLAY_CONTEXT_PROP_OPACITY     =  8,
  CLAY_CONTEXT_PROP_PAINT_MODE  =  9,
  CLAY_CONTEXT_PROP_BRUSH       = 10,
  CLAY_CONTEXT_PROP_DYNAMICS    = 11,
  CLAY_CONTEXT_PROP_MYBRUSH     = 12,
  CLAY_CONTEXT_PROP_PATTERN     = 13,
  CLAY_CONTEXT_PROP_GRADIENT    = 14,
  CLAY_CONTEXT_PROP_PALETTE     = 15,
  CLAY_CONTEXT_PROP_FONT        = 16,
  CLAY_CONTEXT_PROP_TOOL_PRESET = 17,
  CLAY_CONTEXT_PROP_BUFFER      = 18,
  CLAY_CONTEXT_PROP_IMAGEFILE   = 19,
  CLAY_CONTEXT_PROP_TEMPLATE    = 20,
  CLAY_CONTEXT_PROP_EXPAND      = 21,

  CLAY_CONTEXT_PROP_LAST        = CLAY_CONTEXT_PROP_TEMPLATE
} ClayContextPropType;


typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_CONTEXT_PROP_MASK_IMAGE       = 1 <<  2,
  CLAY_CONTEXT_PROP_MASK_DISPLAY     = 1 <<  3,
  CLAY_CONTEXT_PROP_MASK_TOOL        = 1 <<  4,
  CLAY_CONTEXT_PROP_MASK_PAINT_INFO  = 1 <<  5,
  CLAY_CONTEXT_PROP_MASK_FOREGROUND  = 1 <<  6,
  CLAY_CONTEXT_PROP_MASK_BACKGROUND  = 1 <<  7,
  CLAY_CONTEXT_PROP_MASK_OPACITY     = 1 <<  8,
  CLAY_CONTEXT_PROP_MASK_PAINT_MODE  = 1 <<  9,
  CLAY_CONTEXT_PROP_MASK_BRUSH       = 1 << 10,
  CLAY_CONTEXT_PROP_MASK_DYNAMICS    = 1 << 11,
  CLAY_CONTEXT_PROP_MASK_MYBRUSH     = 1 << 12,
  CLAY_CONTEXT_PROP_MASK_PATTERN     = 1 << 13,
  CLAY_CONTEXT_PROP_MASK_GRADIENT    = 1 << 14,
  CLAY_CONTEXT_PROP_MASK_PALETTE     = 1 << 15,
  CLAY_CONTEXT_PROP_MASK_FONT        = 1 << 16,
  CLAY_CONTEXT_PROP_MASK_TOOL_PRESET = 1 << 17,
  CLAY_CONTEXT_PROP_MASK_BUFFER      = 1 << 18,
  CLAY_CONTEXT_PROP_MASK_IMAGEFILE   = 1 << 19,
  CLAY_CONTEXT_PROP_MASK_TEMPLATE    = 1 << 20,
  CLAY_CONTEXT_PROP_MASK_EXPAND      = 1 << 21,

  /*  aliases  */
  CLAY_CONTEXT_PROP_MASK_PAINT = (CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                                  CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                                  CLAY_CONTEXT_PROP_MASK_OPACITY    |
                                  CLAY_CONTEXT_PROP_MASK_PAINT_MODE |
                                  CLAY_CONTEXT_PROP_MASK_BRUSH      |
                                  CLAY_CONTEXT_PROP_MASK_DYNAMICS   |
                                  CLAY_CONTEXT_PROP_MASK_PATTERN    |
                                  CLAY_CONTEXT_PROP_MASK_GRADIENT   |
                                  CLAY_CONTEXT_PROP_MASK_EXPAND),

  CLAY_CONTEXT_PROP_MASK_ALL   = (CLAY_CONTEXT_PROP_MASK_IMAGE       |
                                  CLAY_CONTEXT_PROP_MASK_DISPLAY     |
                                  CLAY_CONTEXT_PROP_MASK_TOOL        |
                                  CLAY_CONTEXT_PROP_MASK_PAINT_INFO  |
                                  CLAY_CONTEXT_PROP_MASK_MYBRUSH     |
                                  CLAY_CONTEXT_PROP_MASK_PALETTE     |
                                  CLAY_CONTEXT_PROP_MASK_FONT        |
                                  CLAY_CONTEXT_PROP_MASK_TOOL_PRESET |
                                  CLAY_CONTEXT_PROP_MASK_BUFFER      |
                                  CLAY_CONTEXT_PROP_MASK_IMAGEFILE   |
                                  CLAY_CONTEXT_PROP_MASK_TEMPLATE    |
                                  CLAY_CONTEXT_PROP_MASK_PAINT)
} ClayContextPropMask;


typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_IMAGE_SCALE_OK,
  CLAY_IMAGE_SCALE_TOO_SMALL,
  CLAY_IMAGE_SCALE_TOO_BIG
} ClayImageScaleCheckType;


typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_ITEM_TYPE_LAYERS   = 1 << 0,
  CLAY_ITEM_TYPE_CHANNELS = 1 << 1,
  CLAY_ITEM_TYPE_PATHS    = 1 << 2,

  CLAY_ITEM_TYPE_ALL      = (CLAY_ITEM_TYPE_LAYERS   |
                             CLAY_ITEM_TYPE_CHANNELS |
                             CLAY_ITEM_TYPE_PATHS)
} ClayItemTypeMask;
