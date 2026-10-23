/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_BASE_ENUMS_H__
#define __CLAY_BASE_ENUMS_H__


/**
 * SECTION: claybaseenums
 * @title: claybaseenums
 * @short_description: Basic Clay enumeration data types.
 *
 * Basic Clay enumeration data types.
 **/


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayAddMaskType:
 * @CLAY_ADD_MASK_WHITE:          White (full opacity)
 * @CLAY_ADD_MASK_BLACK:          Black (full transparency)
 * @CLAY_ADD_MASK_ALPHA:          Layer's alpha channel
 * @CLAY_ADD_MASK_ALPHA_TRANSFER: Transfer layer's alpha channel
 * @CLAY_ADD_MASK_SELECTION:      Selection
 * @CLAY_ADD_MASK_COPY:           Grayscale copy of layer
 * @CLAY_ADD_MASK_CHANNEL:        Channel
 *
 * Modes of initialising a layer mask.
 **/
#define CLAY_TYPE_ADD_MASK_TYPE (clay_add_mask_type_get_type ())

GType clay_add_mask_type_get_type (void);

typedef enum
{
  CLAY_ADD_MASK_WHITE,          /*< desc="_White (full opacity)"           >*/
  CLAY_ADD_MASK_BLACK,          /*< desc="_Black (full transparency)"      >*/
  CLAY_ADD_MASK_ALPHA,          /*< desc="Layer's _alpha channel"          >*/
  CLAY_ADD_MASK_ALPHA_TRANSFER, /*< desc="_Transfer layer's alpha channel" >*/
  CLAY_ADD_MASK_SELECTION,      /*< desc="_Selection"                      >*/
  CLAY_ADD_MASK_COPY,           /*< desc="_Grayscale copy of layer"        >*/
  CLAY_ADD_MASK_CHANNEL         /*< desc="C_hannel"                        >*/
} ClayAddMaskType;


/**
 * ClayBrushGeneratedShape:
 * @CLAY_BRUSH_GENERATED_CIRCLE:  Circle
 * @CLAY_BRUSH_GENERATED_SQUARE:  Square
 * @CLAY_BRUSH_GENERATED_DIAMOND: Diamond
 *
 * Shapes of generated brushes.
 **/
#define CLAY_TYPE_BRUSH_GENERATED_SHAPE (clay_brush_generated_shape_get_type ())

GType clay_brush_generated_shape_get_type (void);

typedef enum
{
  CLAY_BRUSH_GENERATED_CIRCLE,  /*< desc="Circle"  >*/
  CLAY_BRUSH_GENERATED_SQUARE,  /*< desc="Square"  >*/
  CLAY_BRUSH_GENERATED_DIAMOND  /*< desc="Diamond" >*/
} ClayBrushGeneratedShape;


/**
 * ClayCapStyle:
 * @CLAY_CAP_BUTT:   Butt
 * @CLAY_CAP_ROUND:  Round
 * @CLAY_CAP_SQUARE: Square
 *
 * Style of line endings.
 **/
#define CLAY_TYPE_CAP_STYLE (clay_cap_style_get_type ())

GType clay_cap_style_get_type (void);

typedef enum
{
  CLAY_CAP_BUTT,   /*< desc="Butt"   >*/
  CLAY_CAP_ROUND,  /*< desc="Round"  >*/
  CLAY_CAP_SQUARE  /*< desc="Square" >*/
} ClayCapStyle;


/**
 * ClayChannelOps:
 * @CLAY_CHANNEL_OP_ADD:       Add to the current selection
 * @CLAY_CHANNEL_OP_SUBTRACT:  Subtract from the current selection
 * @CLAY_CHANNEL_OP_REPLACE:   Replace the current selection
 * @CLAY_CHANNEL_OP_INTERSECT: Intersect with the current selection
 *
 * Operations to combine channels and selections.
 **/
#define CLAY_TYPE_CHANNEL_OPS (clay_channel_ops_get_type ())

GType clay_channel_ops_get_type (void);

typedef enum
{
  CLAY_CHANNEL_OP_ADD,       /*< desc="Add to the current selection"         >*/
  CLAY_CHANNEL_OP_SUBTRACT,  /*< desc="Subtract from the current selection"  >*/
  CLAY_CHANNEL_OP_REPLACE,   /*< desc="Replace the current selection"        >*/
  CLAY_CHANNEL_OP_INTERSECT  /*< desc="Intersect with the current selection" >*/
} ClayChannelOps;


/**
 * ClayChannelType:
 * @CLAY_CHANNEL_RED:     Red
 * @CLAY_CHANNEL_GREEN:   Green
 * @CLAY_CHANNEL_BLUE:    Blue
 * @CLAY_CHANNEL_GRAY:    Gray
 * @CLAY_CHANNEL_INDEXED: Indexed
 * @CLAY_CHANNEL_ALPHA:   Alpha
 *
 * Channels (as in color components).
 **/
#define CLAY_TYPE_CHANNEL_TYPE (clay_channel_type_get_type ())

GType clay_channel_type_get_type (void);

typedef enum
{
  CLAY_CHANNEL_RED,      /*< desc="Red"     >*/
  CLAY_CHANNEL_GREEN,    /*< desc="Green"   >*/
  CLAY_CHANNEL_BLUE,     /*< desc="Blue"    >*/
  CLAY_CHANNEL_GRAY,     /*< desc="Gray"    >*/
  CLAY_CHANNEL_INDEXED,  /*< desc="Indexed" >*/
  CLAY_CHANNEL_ALPHA     /*< desc="Alpha"   >*/
} ClayChannelType;


/**
 * ClayCheckSize:
 * @CLAY_CHECK_SIZE_SMALL_CHECKS:  Small
 * @CLAY_CHECK_SIZE_MEDIUM_CHECKS: Medium
 * @CLAY_CHECK_SIZE_LARGE_CHECKS:  Large
 *
 * Size of the checkerboard indicating transparency.
 **/
#define CLAY_TYPE_CHECK_SIZE (clay_check_size_get_type ())

GType clay_check_size_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_CHECK_SIZE_SMALL_CHECKS  = 0,  /*< desc="Small"  >*/
  CLAY_CHECK_SIZE_MEDIUM_CHECKS = 1,  /*< desc="Medium" >*/
  CLAY_CHECK_SIZE_LARGE_CHECKS  = 2   /*< desc="Large"  >*/
} ClayCheckSize;


/**
 * ClayCheckType:
 * @CLAY_CHECK_TYPE_LIGHT_CHECKS: Light checks
 * @CLAY_CHECK_TYPE_GRAY_CHECKS:  Mid-tone checks
 * @CLAY_CHECK_TYPE_DARK_CHECKS:  Dark checks
 * @CLAY_CHECK_TYPE_WHITE_ONLY:   White only
 * @CLAY_CHECK_TYPE_GRAY_ONLY:    Gray only
 * @CLAY_CHECK_TYPE_BLACK_ONLY:   Black only
 * @CLAY_CHECK_TYPE_CUSTOM_CHECKS: Custom checks
 *
 * Color/Brightness of the checkerboard indicating transparency.
 **/
#define CLAY_TYPE_CHECK_TYPE (clay_check_type_get_type ())

GType clay_check_type_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_CHECK_TYPE_LIGHT_CHECKS = 0,  /*< desc="Light checks"    >*/
  CLAY_CHECK_TYPE_GRAY_CHECKS  = 1,  /*< desc="Mid-tone checks" >*/
  CLAY_CHECK_TYPE_DARK_CHECKS  = 2,  /*< desc="Dark checks"     >*/
  CLAY_CHECK_TYPE_WHITE_ONLY   = 3,  /*< desc="White only"      >*/
  CLAY_CHECK_TYPE_GRAY_ONLY    = 4,  /*< desc="Gray only"       >*/
  CLAY_CHECK_TYPE_BLACK_ONLY   = 5,  /*< desc="Black only"      >*/
  CLAY_CHECK_TYPE_CUSTOM_CHECKS = 6  /*< desc="Custom checks"   >*/
} ClayCheckType;


/**
 * ClayCloneType:
 * @CLAY_CLONE_IMAGE:   Clone from an image/drawable source
 * @CLAY_CLONE_PATTERN: Clone from a pattern source
 *
 * Clone sources.
 **/
#define CLAY_TYPE_CLONE_TYPE (clay_clone_type_get_type ())

GType clay_clone_type_get_type (void);

typedef enum
{
  CLAY_CLONE_IMAGE,    /*< desc="Image"   >*/
  CLAY_CLONE_PATTERN   /*< desc="Pattern" >*/
} ClayCloneType;


/**
 * ClayColorTag:
 * @CLAY_COLOR_TAG_NONE:   None
 * @CLAY_COLOR_TAG_BLUE:   Blue
 * @CLAY_COLOR_TAG_GREEN:  Green
 * @CLAY_COLOR_TAG_YELLOW: Yellow
 * @CLAY_COLOR_TAG_ORANGE: Orange
 * @CLAY_COLOR_TAG_BROWN:  Brown
 * @CLAY_COLOR_TAG_RED:    Red
 * @CLAY_COLOR_TAG_VIOLET: Violet
 * @CLAY_COLOR_TAG_GRAY:   Gray
 *
 * Possible tag colors.
 *
 * Since: 2.10
 **/
#define CLAY_TYPE_COLOR_TAG (clay_color_tag_get_type ())

GType clay_color_tag_get_type (void);

typedef enum
{
  CLAY_COLOR_TAG_NONE,   /*< desc="None"   >*/
  CLAY_COLOR_TAG_BLUE,   /*< desc="Blue"   >*/
  CLAY_COLOR_TAG_GREEN,  /*< desc="Green"  >*/
  CLAY_COLOR_TAG_YELLOW, /*< desc="Yellow" >*/
  CLAY_COLOR_TAG_ORANGE, /*< desc="Orange" >*/
  CLAY_COLOR_TAG_BROWN,  /*< desc="Brown"  >*/
  CLAY_COLOR_TAG_RED,    /*< desc="Red"    >*/
  CLAY_COLOR_TAG_VIOLET, /*< desc="Violet" >*/
  CLAY_COLOR_TAG_GRAY    /*< desc="Gray"   >*/
} ClayColorTag;


/**
 * ClayComponentType:
 * @CLAY_COMPONENT_TYPE_U8:     8-bit integer
 * @CLAY_COMPONENT_TYPE_U16:    16-bit integer
 * @CLAY_COMPONENT_TYPE_U32:    32-bit integer
 * @CLAY_COMPONENT_TYPE_HALF:   16-bit floating point
 * @CLAY_COMPONENT_TYPE_FLOAT:  32-bit floating point
 * @CLAY_COMPONENT_TYPE_DOUBLE: 64-bit floating point
 *
 * Encoding types of image components.
 *
 * Since: 2.10
 **/
#define CLAY_TYPE_COMPONENT_TYPE (clay_component_type_get_type ())

GType clay_component_type_get_type (void);

typedef enum
{
  CLAY_COMPONENT_TYPE_U8     = 100, /*< desc="8-bit integer"         >*/
  CLAY_COMPONENT_TYPE_U16    = 200, /*< desc="16-bit integer"        >*/
  CLAY_COMPONENT_TYPE_U32    = 300, /*< desc="32-bit integer"        >*/
  CLAY_COMPONENT_TYPE_HALF   = 500, /*< desc="16-bit floating point" >*/
  CLAY_COMPONENT_TYPE_FLOAT  = 600, /*< desc="32-bit floating point" >*/
  CLAY_COMPONENT_TYPE_DOUBLE = 700  /*< desc="64-bit floating point" >*/
} ClayComponentType;


/**
 * ClayConvertPaletteType:
 * @CLAY_CONVERT_PALETTE_GENERATE: Generate optimum palette
 * @CLAY_CONVERT_PALETTE_WEB:      Use web-optimized palette
 * @CLAY_CONVERT_PALETTE_MONO:     Use black and white (1-bit) palette
 * @CLAY_CONVERT_PALETTE_CUSTOM:   Use custom palette
 *
 * Types of palettes for indexed conversion.
 **/
#define CLAY_TYPE_CONVERT_PALETTE_TYPE (clay_convert_palette_type_get_type ())

GType clay_convert_palette_type_get_type (void);

typedef enum
{
  CLAY_CONVERT_PALETTE_GENERATE, /*< desc="_Generate optimum palette"          >*/
  CLAY_CONVERT_PALETTE_WEB,      /*< desc="Use _web-optimized palette"         >*/
  CLAY_CONVERT_PALETTE_MONO,     /*< desc="Use _black and white (1-bit) palette" >*/
  CLAY_CONVERT_PALETTE_CUSTOM    /*< desc="Use custom _palette"                >*/
} ClayConvertPaletteType;


/**
 * ClayConvolveType:
 * @CLAY_CONVOLVE_BLUR:    Blur
 * @CLAY_CONVOLVE_SHARPEN: Sharpen
 *
 * Types of convolutions.
 **/
#define CLAY_TYPE_CONVOLVE_TYPE (clay_convolve_type_get_type ())

GType clay_convolve_type_get_type (void);

typedef enum
{
  CLAY_CONVOLVE_BLUR,    /*< desc="Blur"    >*/
  CLAY_CONVOLVE_SHARPEN  /*< desc="Sharpen" >*/
} ClayConvolveType;


/**
 * ClayDesaturateMode:
 * @CLAY_DESATURATE_LIGHTNESS:  Lightness (HSL)
 * @CLAY_DESATURATE_LUMA:       Luma
 * @CLAY_DESATURATE_AVERAGE:    Average (HSI Intensity)
 * @CLAY_DESATURATE_LUMINANCE:  Luminance
 * @CLAY_DESATURATE_VALUE:      Value (HSV)
 *
 * Grayscale conversion methods.
 **/
#define CLAY_TYPE_DESATURATE_MODE (clay_desaturate_mode_get_type ())

GType clay_desaturate_mode_get_type (void);

typedef enum
{
  CLAY_DESATURATE_LIGHTNESS,   /*< desc="Lightness (HSL)"          >*/
  CLAY_DESATURATE_LUMA,        /*< desc="Luma"                     >*/
  CLAY_DESATURATE_AVERAGE,     /*< desc="Average (HSI Intensity)"  >*/
  CLAY_DESATURATE_LUMINANCE,   /*< desc="Luminance"                >*/
  CLAY_DESATURATE_VALUE        /*< desc="Value (HSV)"              >*/
} ClayDesaturateMode;


/**
 * ClayDodgeBurnType:
 * @CLAY_DODGE_BURN_TYPE_DODGE: Dodge
 * @CLAY_DODGE_BURN_TYPE_BURN:  Burn
 *
 * Methods for the dodge/burn operation.
 **/
#define CLAY_TYPE_DODGE_BURN_TYPE (clay_dodge_burn_type_get_type ())

GType clay_dodge_burn_type_get_type (void);

typedef enum
{
  CLAY_DODGE_BURN_TYPE_DODGE,  /*< desc="Dodge" >*/
  CLAY_DODGE_BURN_TYPE_BURN    /*< desc="Burn"  >*/
} ClayDodgeBurnType;


/**
 * ClayFillType:
 * @CLAY_FILL_FOREGROUND:         Foreground color
 * @CLAY_FILL_BACKGROUND:         Background color
 * @CLAY_FILL_CIELAB_MIDDLE_GRAY: Middle Gray (CIELAB)
 * @CLAY_FILL_WHITE:              White
 * @CLAY_FILL_TRANSPARENT:        Transparency
 * @CLAY_FILL_PATTERN:            Pattern
 *
 * Types of filling.
 **/
#define CLAY_TYPE_FILL_TYPE (clay_fill_type_get_type ())

GType clay_fill_type_get_type (void);

typedef enum
{
  CLAY_FILL_FOREGROUND,         /*< desc="Foreground color" >*/
  CLAY_FILL_BACKGROUND,         /*< desc="Background color" >*/
  CLAY_FILL_CIELAB_MIDDLE_GRAY, /*< desc="Middle Gray (CIELAB)" >*/
  CLAY_FILL_WHITE,              /*< desc="White"            >*/
  CLAY_FILL_TRANSPARENT,        /*< desc="Transparency"     >*/
  CLAY_FILL_PATTERN,            /*< desc="Pattern"          >*/
} ClayFillType;


/**
 * ClayForegroundExtractMode:
 * @CLAY_FOREGROUND_EXTRACT_MATTING: Matting (Since 2.10)
 *
 * Foreground extract engines.
 **/
#define CLAY_TYPE_FOREGROUND_EXTRACT_MODE (clay_foreground_extract_mode_get_type ())

GType clay_foreground_extract_mode_get_type (void);

typedef enum
{
  CLAY_FOREGROUND_EXTRACT_MATTING
} ClayForegroundExtractMode;


/**
 * ClayGradientBlendColorSpace:
 * @CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL: Perceptual RGB
 * @CLAY_GRADIENT_BLEND_RGB_LINEAR:     Linear RGB
 * @CLAY_GRADIENT_BLEND_CIE_LAB:        CIE Lab
 *
 * Color space for blending gradients.
 *
 * Since: 2.10
 */
#define CLAY_TYPE_GRADIENT_BLEND_COLOR_SPACE (clay_gradient_blend_color_space_get_type ())

GType clay_gradient_blend_color_space_get_type (void);

typedef enum
{
  CLAY_GRADIENT_BLEND_RGB_PERCEPTUAL,  /*< desc="Perceptual RGB", nick=rgb-perceptual >*/
  CLAY_GRADIENT_BLEND_RGB_LINEAR,      /*< desc="Linear RGB",     nick=rgb-linear     >*/
  CLAY_GRADIENT_BLEND_CIE_LAB          /*< desc="CIE Lab",        nick=cie-lab        >*/
} ClayGradientBlendColorSpace;


/**
 * ClayGradientSegmentColor:
 * @CLAY_GRADIENT_SEGMENT_RGB:     RGB
 * @CLAY_GRADIENT_SEGMENT_HSV_CCW: HSV (counter-clockwise hue)
 * @CLAY_GRADIENT_SEGMENT_HSV_CW:  HSV (clockwise hue)
 *
 * Coloring types for gradient segments.
 **/
#define CLAY_TYPE_GRADIENT_SEGMENT_COLOR (clay_gradient_segment_color_get_type ())

GType clay_gradient_segment_color_get_type (void);

typedef enum
{
  CLAY_GRADIENT_SEGMENT_RGB,      /*< desc="RGB"                                             >*/
  CLAY_GRADIENT_SEGMENT_HSV_CCW,  /*< desc="HSV (counter-clockwise hue)", abbrev="HSV (ccw)" >*/
  CLAY_GRADIENT_SEGMENT_HSV_CW    /*< desc="HSV (clockwise hue)",         abbrev="HSV (cw)"  >*/
} ClayGradientSegmentColor;


/**
 * ClayGradientSegmentType:
 * @CLAY_GRADIENT_SEGMENT_LINEAR:            Linear
 * @CLAY_GRADIENT_SEGMENT_CURVED:            Curved
 * @CLAY_GRADIENT_SEGMENT_SINE:              Sinusoidal
 * @CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING: Spherical (increasing)
 * @CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING: Spherical (decreasing)
 * @CLAY_GRADIENT_SEGMENT_STEP:              Step
 *
 * Transition functions for gradient segments.
 **/
#define CLAY_TYPE_GRADIENT_SEGMENT_TYPE (clay_gradient_segment_type_get_type ())

GType clay_gradient_segment_type_get_type (void);

typedef enum
{
  CLAY_GRADIENT_SEGMENT_LINEAR,             /*< desc="Linear"                                           >*/
  CLAY_GRADIENT_SEGMENT_CURVED,             /*< desc="Curved"                                           >*/
  CLAY_GRADIENT_SEGMENT_SINE,               /*< desc="Sinusoidal"                                       >*/
  CLAY_GRADIENT_SEGMENT_SPHERE_INCREASING,  /*< desc="Spherical (increasing)", abbrev="Spherical (inc)" >*/
  CLAY_GRADIENT_SEGMENT_SPHERE_DECREASING,  /*< desc="Spherical (decreasing)", abbrev="Spherical (dec)" >*/
  CLAY_GRADIENT_SEGMENT_STEP                /*< desc="Step"                                             >*/
} ClayGradientSegmentType;


/**
 * ClayGradientType:
 * @CLAY_GRADIENT_LINEAR:               Linear
 * @CLAY_GRADIENT_BILINEAR:             Bi-linear
 * @CLAY_GRADIENT_RADIAL:               Radial
 * @CLAY_GRADIENT_SQUARE:               Square
 * @CLAY_GRADIENT_CONICAL_SYMMETRIC:    Conical (symmetric)
 * @CLAY_GRADIENT_CONICAL_ASYMMETRIC:   Conical (asymmetric)
 * @CLAY_GRADIENT_SHAPEBURST_ANGULAR:   Shaped (angular)
 * @CLAY_GRADIENT_SHAPEBURST_SPHERICAL: Shaped (spherical)
 * @CLAY_GRADIENT_SHAPEBURST_DIMPLED:   Shaped (dimpled)
 * @CLAY_GRADIENT_SPIRAL_CLOCKWISE:     Spiral (clockwise)
 * @CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE: Spiral (counter-clockwise)
 *
 * Gradient shapes.
 **/
#define CLAY_TYPE_GRADIENT_TYPE (clay_gradient_type_get_type ())

GType clay_gradient_type_get_type (void);

typedef enum
{
  CLAY_GRADIENT_LINEAR,                /*< desc="Linear"                                              >*/
  CLAY_GRADIENT_BILINEAR,              /*< desc="Bi-linear"                                           >*/
  CLAY_GRADIENT_RADIAL,                /*< desc="Radial"                                              >*/
  CLAY_GRADIENT_SQUARE,                /*< desc="Square"                                              >*/
  CLAY_GRADIENT_CONICAL_SYMMETRIC,     /*< desc="Conical (symmetric)",        abbrev="Conical (sym)"  >*/
  CLAY_GRADIENT_CONICAL_ASYMMETRIC,    /*< desc="Conical (asymmetric)",       abbrev="Conical (asym)" >*/
  CLAY_GRADIENT_SHAPEBURST_ANGULAR,    /*< desc="Shaped (angular)"                                    >*/
  CLAY_GRADIENT_SHAPEBURST_SPHERICAL,  /*< desc="Shaped (spherical)"                                  >*/
  CLAY_GRADIENT_SHAPEBURST_DIMPLED,    /*< desc="Shaped (dimpled)"                                    >*/
  CLAY_GRADIENT_SPIRAL_CLOCKWISE,      /*< desc="Spiral (clockwise)",         abbrev="Spiral (cw)"    >*/
  CLAY_GRADIENT_SPIRAL_ANTICLOCKWISE   /*< desc="Spiral (counter-clockwise)", abbrev="Spiral (ccw)"   >*/
} ClayGradientType;


/**
 * ClayGridStyle:
 * @CLAY_GRID_DOTS:          Intersections (dots)
 * @CLAY_GRID_INTERSECTIONS: Intersections (crosshairs)
 * @CLAY_GRID_ON_OFF_DASH:   Dashed
 * @CLAY_GRID_DOUBLE_DASH:   Double dashed
 * @CLAY_GRID_SOLID:         Solid
 *
 * Rendering types for the display grid.
 **/
#define CLAY_TYPE_GRID_STYLE (clay_grid_style_get_type ())

GType clay_grid_style_get_type (void);

typedef enum
{
  CLAY_GRID_DOTS,           /*< desc="Intersections (dots)"       >*/
  CLAY_GRID_INTERSECTIONS,  /*< desc="Intersections (crosshairs)" >*/
  CLAY_GRID_ON_OFF_DASH,    /*< desc="Dashed"                     >*/
  CLAY_GRID_DOUBLE_DASH,    /*< desc="Double dashed"              >*/
  CLAY_GRID_SOLID           /*< desc="Solid"                      >*/
} ClayGridStyle;


/**
 * ClayHueRange:
 * @CLAY_HUE_RANGE_ALL:     All hues
 * @CLAY_HUE_RANGE_RED:     Red hues
 * @CLAY_HUE_RANGE_YELLOW:  Yellow hues
 * @CLAY_HUE_RANGE_GREEN:   Green hues
 * @CLAY_HUE_RANGE_CYAN:    Cyan hues
 * @CLAY_HUE_RANGE_BLUE:    Blue hues
 * @CLAY_HUE_RANGE_MAGENTA: Magenta hues
 *
 * Hue ranges.
 **/
#define CLAY_TYPE_HUE_RANGE (clay_hue_range_get_type ())

GType clay_hue_range_get_type (void);

typedef enum
{
  CLAY_HUE_RANGE_ALL,
  CLAY_HUE_RANGE_RED,
  CLAY_HUE_RANGE_YELLOW,
  CLAY_HUE_RANGE_GREEN,
  CLAY_HUE_RANGE_CYAN,
  CLAY_HUE_RANGE_BLUE,
  CLAY_HUE_RANGE_MAGENTA
} ClayHueRange;


/**
 * ClayIconType:
 * @CLAY_ICON_TYPE_ICON_NAME:  Icon name
 * @CLAY_ICON_TYPE_PIXBUF:     Inline pixbuf
 * @CLAY_ICON_TYPE_IMAGE_FILE: Image file
 *
 * Icon types for plug-ins to register.
 **/
#define CLAY_TYPE_ICON_TYPE (clay_icon_type_get_type ())

GType clay_icon_type_get_type (void);

typedef enum
{
  CLAY_ICON_TYPE_ICON_NAME,  /*< desc="Icon name"  >*/
  CLAY_ICON_TYPE_PIXBUF,     /*< desc="Pixbuf"     >*/
  CLAY_ICON_TYPE_IMAGE_FILE  /*< desc="Image file" >*/
} ClayIconType;


/**
 * ClayImageBaseType:
 * @CLAY_RGB:     RGB color
 * @CLAY_GRAY:    Grayscale
 * @CLAY_INDEXED: Indexed color
 *
 * Image color models.
 **/
#define CLAY_TYPE_IMAGE_BASE_TYPE (clay_image_base_type_get_type ())

GType clay_image_base_type_get_type (void);

typedef enum
{
  CLAY_RGB,     /*< desc="RGB color"     >*/
  CLAY_GRAY,    /*< desc="Grayscale"     >*/
  CLAY_INDEXED  /*< desc="Indexed color" >*/
} ClayImageBaseType;


/**
 * ClayImageType:
 * @CLAY_RGB_IMAGE:      RGB
 * @CLAY_RGBA_IMAGE:     RGB-alpha
 * @CLAY_GRAY_IMAGE:     Grayscale
 * @CLAY_GRAYA_IMAGE:    Grayscale-alpha
 * @CLAY_INDEXED_IMAGE:  Indexed
 * @CLAY_INDEXEDA_IMAGE: Indexed-alpha
 *
 * Possible drawable types.
 **/
#define CLAY_TYPE_IMAGE_TYPE (clay_image_type_get_type ())

GType clay_image_type_get_type (void);

typedef enum
{
  CLAY_RGB_IMAGE,      /*< desc="RGB"             >*/
  CLAY_RGBA_IMAGE,     /*< desc="RGB-alpha"       >*/
  CLAY_GRAY_IMAGE,     /*< desc="Grayscale"       >*/
  CLAY_GRAYA_IMAGE,    /*< desc="Grayscale-alpha" >*/
  CLAY_INDEXED_IMAGE,  /*< desc="Indexed"         >*/
  CLAY_INDEXEDA_IMAGE  /*< desc="Indexed-alpha"   >*/
} ClayImageType;


/**
 * ClayInkBlobType:
 * @CLAY_INK_BLOB_TYPE_CIRCLE:  Circle
 * @CLAY_INK_BLOB_TYPE_SQUARE:  Square
 * @CLAY_INK_BLOB_TYPE_DIAMOND: Diamond
 *
 * Ink tool tips.
 **/
#define CLAY_TYPE_INK_BLOB_TYPE (clay_ink_blob_type_get_type ())

GType clay_ink_blob_type_get_type (void);

typedef enum
{
  CLAY_INK_BLOB_TYPE_CIRCLE,  /*< desc="Circle"  >*/
  CLAY_INK_BLOB_TYPE_SQUARE,  /*< desc="Square"  >*/
  CLAY_INK_BLOB_TYPE_DIAMOND  /*< desc="Diamond" >*/
} ClayInkBlobType;


/**
 * ClayInterpolationType:
 * @CLAY_INTERPOLATION_NONE:   None
 * @CLAY_INTERPOLATION_LINEAR: Linear
 * @CLAY_INTERPOLATION_CUBIC:  Cubic
 * @CLAY_INTERPOLATION_NOHALO: NoHalo
 * @CLAY_INTERPOLATION_LOHALO: LoHalo
 *
 * Interpolation types.
 **/
#define CLAY_TYPE_INTERPOLATION_TYPE (clay_interpolation_type_get_type ())

GType clay_interpolation_type_get_type (void);

typedef enum
{
  CLAY_INTERPOLATION_NONE,   /*< desc="None"   >*/
  CLAY_INTERPOLATION_LINEAR, /*< desc="Linear" >*/
  CLAY_INTERPOLATION_CUBIC,  /*< desc="Cubic"  >*/
  CLAY_INTERPOLATION_NOHALO, /*< desc="NoHalo" >*/
  CLAY_INTERPOLATION_LOHALO  /*< desc="LoHalo" >*/
} ClayInterpolationType;


/**
 * ClayJoinStyle:
 * @CLAY_JOIN_MITER: Miter
 * @CLAY_JOIN_ROUND: Round
 * @CLAY_JOIN_BEVEL: Bevel
 *
 * Line join styles.
 **/
#define CLAY_TYPE_JOIN_STYLE (clay_join_style_get_type ())

GType clay_join_style_get_type (void);

typedef enum
{
  CLAY_JOIN_MITER,  /*< desc="Miter" >*/
  CLAY_JOIN_ROUND,  /*< desc="Round" >*/
  CLAY_JOIN_BEVEL   /*< desc="Bevel" >*/
} ClayJoinStyle;


/**
 * ClayMaskApplyMode:
 * @CLAY_MASK_APPLY:   Apply the mask
 * @CLAY_MASK_DISCARD: Discard the mask
 *
 * Layer mask apply modes.
 **/
#define CLAY_TYPE_MASK_APPLY_MODE (clay_mask_apply_mode_get_type ())

GType clay_mask_apply_mode_get_type (void);

typedef enum
{
  CLAY_MASK_APPLY,
  CLAY_MASK_DISCARD
} ClayMaskApplyMode;


/**
 * ClayMergeType:
 * @CLAY_EXPAND_AS_NECESSARY:  Expanded as necessary
 * @CLAY_CLIP_TO_IMAGE:        Clipped to image
 * @CLAY_CLIP_TO_BOTTOM_LAYER: Clipped to bottom layer
 * @CLAY_FLATTEN_IMAGE:        Flatten
 *
 * Types of merging layers.
 **/
#define CLAY_TYPE_MERGE_TYPE (clay_merge_type_get_type ())

GType clay_merge_type_get_type (void);

typedef enum
{
  CLAY_EXPAND_AS_NECESSARY,  /*< desc="Expanded as necessary"  >*/
  CLAY_CLIP_TO_IMAGE,        /*< desc="Clipped to image"        >*/
  CLAY_CLIP_TO_BOTTOM_LAYER, /*< desc="Clipped to bottom layer" >*/
  CLAY_FLATTEN_IMAGE         /*< desc="Flatten"                 >*/
} ClayMergeType;


/**
 * ClayMessageHandlerType:
 * @CLAY_MESSAGE_BOX:   A status bar message (if opened) or popup dialog (if not)
 * @CLAY_CONSOLE:       The terminal
 * @CLAY_ERROR_CONSOLE: The error console dockable
 *
 * How to present messages.
 **/
#define CLAY_TYPE_MESSAGE_HANDLER_TYPE (clay_message_handler_type_get_type ())

GType clay_message_handler_type_get_type (void);

typedef enum
{
  CLAY_MESSAGE_BOX,
  CLAY_CONSOLE,
  CLAY_ERROR_CONSOLE
} ClayMessageHandlerType;


/**
 * ClayOffsetType:
 * @CLAY_OFFSET_COLOR:       Color
 * @CLAY_OFFSET_TRANSPARENT: Transparent
 * @CLAY_OFFSET_WRAP_AROUND: Wrap image around
 *
 * Background fill types for the offset operation.
 **/
#define CLAY_TYPE_OFFSET_TYPE (clay_offset_type_get_type ())

GType clay_offset_type_get_type (void);

typedef enum
{
  CLAY_OFFSET_COLOR,
  CLAY_OFFSET_TRANSPARENT,
  CLAY_OFFSET_WRAP_AROUND
} ClayOffsetType;


/**
 * ClayOrientationType:
 * @CLAY_ORIENTATION_HORIZONTAL: Horizontal
 * @CLAY_ORIENTATION_VERTICAL:   Vertical
 * @CLAY_ORIENTATION_UNKNOWN:    Unknown
 *
 * Orientations for various purposes.
 **/
#define CLAY_TYPE_ORIENTATION_TYPE (clay_orientation_type_get_type ())

GType clay_orientation_type_get_type (void);

typedef enum
{
  CLAY_ORIENTATION_HORIZONTAL, /*< desc="Horizontal" >*/
  CLAY_ORIENTATION_VERTICAL,   /*< desc="Vertical"   >*/
  CLAY_ORIENTATION_UNKNOWN     /*< desc="Unknown"    >*/
} ClayOrientationType;


/**
 * ClayPaintApplicationMode:
 * @CLAY_PAINT_CONSTANT:    Constant
 * @CLAY_PAINT_INCREMENTAL: Incremental
 *
 * Paint application modes.
 **/
#define CLAY_TYPE_PAINT_APPLICATION_MODE (clay_paint_application_mode_get_type ())

GType clay_paint_application_mode_get_type (void);

typedef enum
{
  CLAY_PAINT_CONSTANT,    /*< desc="Constant"    >*/
  CLAY_PAINT_INCREMENTAL  /*< desc="Incremental" >*/
} ClayPaintApplicationMode;


/**
 * ClayPDBErrorHandler:
 * @CLAY_PDB_ERROR_HANDLER_INTERNAL: Internal
 * @CLAY_PDB_ERROR_HANDLER_PLUGIN:   Plug-In
 *
 * PDB error handlers.
 **/
#define CLAY_TYPE_PDB_ERROR_HANDLER (clay_pdb_error_handler_get_type ())

GType clay_pdb_error_handler_get_type (void);

typedef enum
{
  CLAY_PDB_ERROR_HANDLER_INTERNAL,
  CLAY_PDB_ERROR_HANDLER_PLUGIN
} ClayPDBErrorHandler;


/**
 * ClayPDBProcType:
 * @CLAY_PDB_PROC_TYPE_INTERNAL:   Internal Clay procedure
 * @CLAY_PDB_PROC_TYPE_PLUGIN:     Clay Plug-In
 * @CLAY_PDB_PROC_TYPE_PERSISTENT: Clay Persistent Plug-in
 * @CLAY_PDB_PROC_TYPE_TEMPORARY:  Temporary Procedure
 *
 * Types of PDB procedures.
 **/
#define CLAY_TYPE_PDB_PROC_TYPE (clay_pdb_proc_type_get_type ())

GType clay_pdb_proc_type_get_type (void);

typedef enum
{
  CLAY_PDB_PROC_TYPE_INTERNAL,   /*< desc="Internal Clay procedure" >*/
  CLAY_PDB_PROC_TYPE_PLUGIN,     /*< desc="Clay Plug-In" >*/
  CLAY_PDB_PROC_TYPE_PERSISTENT, /*< desc="Clay Persistent Plug-In" >*/
  CLAY_PDB_PROC_TYPE_TEMPORARY   /*< desc="Temporary Procedure" >*/
} ClayPDBProcType;


/**
 * ClayPDBStatusType:
 * @CLAY_PDB_EXECUTION_ERROR: Execution error
 * @CLAY_PDB_CALLING_ERROR:   Calling error
 * @CLAY_PDB_PASS_THROUGH:    Pass through
 * @CLAY_PDB_SUCCESS:         Success
 * @CLAY_PDB_CANCEL:          User cancel
 *
 * Return status of PDB calls.
 **/
#define CLAY_TYPE_PDB_STATUS_TYPE (clay_pdb_status_type_get_type ())

GType clay_pdb_status_type_get_type (void);

typedef enum
{
  CLAY_PDB_EXECUTION_ERROR,
  CLAY_PDB_CALLING_ERROR,
  CLAY_PDB_PASS_THROUGH,
  CLAY_PDB_SUCCESS,
  CLAY_PDB_CANCEL
} ClayPDBStatusType;


/**
 * ClayPrecision:
 * @CLAY_PRECISION_U8_LINEAR:         8-bit linear integer
 * @CLAY_PRECISION_U8_NON_LINEAR:     8-bit non-linear integer
 * @CLAY_PRECISION_U8_PERCEPTUAL:     8-bit perceptual integer
 * @CLAY_PRECISION_U16_LINEAR:        16-bit linear integer
 * @CLAY_PRECISION_U16_NON_LINEAR:    16-bit non-linear integer
 * @CLAY_PRECISION_U16_PERCEPTUAL:    16-bit perceptual integer
 * @CLAY_PRECISION_U32_LINEAR:        32-bit linear integer
 * @CLAY_PRECISION_U32_NON_LINEAR:    32-bit non-linear integer
 * @CLAY_PRECISION_U32_PERCEPTUAL:    32-bit perceptual integer
 * @CLAY_PRECISION_HALF_LINEAR:       16-bit linear floating point
 * @CLAY_PRECISION_HALF_NON_LINEAR:   16-bit non-linear floating point
 * @CLAY_PRECISION_HALF_PERCEPTUAL:   16-bit perceptual floating point
 * @CLAY_PRECISION_FLOAT_LINEAR:      32-bit linear floating point
 * @CLAY_PRECISION_FLOAT_NON_LINEAR:  32-bit non-linear floating point
 * @CLAY_PRECISION_FLOAT_PERCEPTUAL:  32-bit perceptual floating point
 * @CLAY_PRECISION_DOUBLE_LINEAR:     64-bit linear floating point
 * @CLAY_PRECISION_DOUBLE_NON_LINEAR: 64-bit non-linear floating point
 * @CLAY_PRECISION_DOUBLE_PERCEPTUAL: 64-bit perceptual floating point
 *
 * Precisions for pixel encoding.
 *
 * Since: 2.10
 **/
#define CLAY_TYPE_PRECISION (clay_precision_get_type ())

GType clay_precision_get_type (void);

typedef enum
{
  CLAY_PRECISION_U8_LINEAR         = 100, /*< desc="8-bit linear integer"         >*/
  CLAY_PRECISION_U8_NON_LINEAR     = 150, /*< desc="8-bit non-linear integer"          >*/
  CLAY_PRECISION_U8_PERCEPTUAL     = 175, /*< desc="8-bit perceptual integer"          >*/
  CLAY_PRECISION_U16_LINEAR        = 200, /*< desc="16-bit linear integer"        >*/
  CLAY_PRECISION_U16_NON_LINEAR    = 250, /*< desc="16-bit non-linear integer"         >*/
  CLAY_PRECISION_U16_PERCEPTUAL    = 275, /*< desc="16-bit perceptual integer"         >*/
  CLAY_PRECISION_U32_LINEAR        = 300, /*< desc="32-bit linear integer"        >*/
  CLAY_PRECISION_U32_NON_LINEAR    = 350, /*< desc="32-bit non-linear integer"         >*/
  CLAY_PRECISION_U32_PERCEPTUAL    = 375, /*< desc="32-bit perceptual integer"         >*/
  CLAY_PRECISION_HALF_LINEAR       = 500, /*< desc="16-bit linear floating point" >*/
  CLAY_PRECISION_HALF_NON_LINEAR   = 550, /*< desc="16-bit non-linear floating point"  >*/
  CLAY_PRECISION_HALF_PERCEPTUAL   = 575, /*< desc="16-bit perceptual floating point"  >*/
  CLAY_PRECISION_FLOAT_LINEAR      = 600, /*< desc="32-bit linear floating point" >*/
  CLAY_PRECISION_FLOAT_NON_LINEAR  = 650, /*< desc="32-bit non-linear floating point"  >*/
  CLAY_PRECISION_FLOAT_PERCEPTUAL  = 675, /*< desc="32-bit perceptual floating point"  >*/
  CLAY_PRECISION_DOUBLE_LINEAR     = 700, /*< desc="64-bit linear floating point" >*/
  CLAY_PRECISION_DOUBLE_NON_LINEAR = 750, /*< desc="64-bit non-linear floating point"  >*/
  CLAY_PRECISION_DOUBLE_PERCEPTUAL = 775, /*< desc="64-bit perceptual floating point"  >*/
} ClayPrecision;


/**
 * ClayProcedureSensitivityMask:
 * @CLAY_PROCEDURE_SENSITIVE_DRAWABLE:     Handles image with one selected drawable.
 * @CLAY_PROCEDURE_SENSITIVE_DRAWABLES:    Handles image with several selected drawables.
 * @CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES: Handles image with no selected drawables.
 * @CLAY_PROCEDURE_SENSITIVE_NO_IMAGE:     Handles no image.
 * @CLAY_PROCEDURE_SENSITIVE_ALWAYS:       The procedure is always sensitive.
 *
 * The cases when a #ClayProcedure should be shown as sensitive.
 **/
#define CLAY_TYPE_PROCEDURE_SENSITIVITY_MASK (clay_procedure_sensitivity_mask_get_type ())

GType clay_procedure_sensitivity_mask_get_type (void);
typedef enum  /*< pdb-skip >*/
{
  CLAY_PROCEDURE_SENSITIVE_DRAWABLE      = 1 << 0,
  CLAY_PROCEDURE_SENSITIVE_DRAWABLES     = 1 << 2,
  CLAY_PROCEDURE_SENSITIVE_NO_DRAWABLES  = 1 << 3,
  CLAY_PROCEDURE_SENSITIVE_NO_IMAGE      = 1 << 4,
  CLAY_PROCEDURE_SENSITIVE_ALWAYS        = G_MAXINT
} ClayProcedureSensitivityMask;


/**
 * ClayProgressCommand:
 * @CLAY_PROGRESS_COMMAND_START:      Start a progress
 * @CLAY_PROGRESS_COMMAND_END:        End the progress
 * @CLAY_PROGRESS_COMMAND_SET_TEXT:   Set the text
 * @CLAY_PROGRESS_COMMAND_SET_VALUE:  Set the percentage
 * @CLAY_PROGRESS_COMMAND_PULSE:      Pulse the progress
 * @CLAY_PROGRESS_COMMAND_GET_WINDOW: Get the window where the progress is shown
 *
 * Commands for the progress API.
 **/
#define CLAY_TYPE_PROGRESS_COMMAND (clay_progress_command_get_type ())

GType clay_progress_command_get_type (void);

typedef enum
{
  CLAY_PROGRESS_COMMAND_START,
  CLAY_PROGRESS_COMMAND_END,
  CLAY_PROGRESS_COMMAND_SET_TEXT,
  CLAY_PROGRESS_COMMAND_SET_VALUE,
  CLAY_PROGRESS_COMMAND_PULSE,
  CLAY_PROGRESS_COMMAND_GET_WINDOW
} ClayProgressCommand;


/**
 * ClayRepeatMode:
 * @CLAY_REPEAT_NONE:       None (extend)
 * @CLAY_REPEAT_TRUNCATE:   None (truncate)
 * @CLAY_REPEAT_SAWTOOTH:   Sawtooth wave
 * @CLAY_REPEAT_TRIANGULAR: Triangular wave
 *
 * Repeat modes for example for gradients.
 **/
#define CLAY_TYPE_REPEAT_MODE (clay_repeat_mode_get_type ())

GType clay_repeat_mode_get_type (void);

typedef enum
{
  CLAY_REPEAT_NONE,       /*< desc="None (extend)"   >*/
  CLAY_REPEAT_TRUNCATE,   /*< desc="None (truncate)" >*/
  CLAY_REPEAT_SAWTOOTH,   /*< desc="Sawtooth wave"   >*/
  CLAY_REPEAT_TRIANGULAR  /*< desc="Triangular wave" >*/
} ClayRepeatMode;


/**
 * ClayRotationType:
 * @CLAY_ROTATE_DEGREES90:  90 degrees
 * @CLAY_ROTATE_DEGREES180: 180 degrees
 * @CLAY_ROTATE_DEGREES270: 270 degrees
 *
 * Types of simple rotations.
 **/
#define CLAY_TYPE_ROTATION_TYPE (clay_rotation_type_get_type ())

GType clay_rotation_type_get_type (void);

/* Due to GObject Introspection we can't have the last part of an identifier
 * start with a digit, since that part will be used in Python as the
 * identifier, and Python doesn't allow that to start with a digit. */
typedef enum
{
  CLAY_ROTATE_DEGREES90,
  CLAY_ROTATE_DEGREES180,
  CLAY_ROTATE_DEGREES270
} ClayRotationType;


/**
 * ClayRunMode:
 * @CLAY_RUN_INTERACTIVE:    Run interactively
 * @CLAY_RUN_NONINTERACTIVE: Run non-interactively
 * @CLAY_RUN_WITH_LAST_VALS: Run with last used values
 *
 * Run modes for plug-ins.
 **/
#define CLAY_TYPE_RUN_MODE (clay_run_mode_get_type ())

GType clay_run_mode_get_type (void);

typedef enum
{
  CLAY_RUN_INTERACTIVE,     /*< desc="Run interactively"         >*/
  CLAY_RUN_NONINTERACTIVE,  /*< desc="Run non-interactively"     >*/
  CLAY_RUN_WITH_LAST_VALS   /*< desc="Run with last used values" >*/
} ClayRunMode;


/**
 * ClaySelectCriterion:
 * @CLAY_SELECT_CRITERION_COMPOSITE:      Composite
 * @CLAY_SELECT_CRITERION_RGB_RED:        Red
 * @CLAY_SELECT_CRITERION_RGB_GREEN:      Green
 * @CLAY_SELECT_CRITERION_RGB_BLUE:       Blue
 * @CLAY_SELECT_CRITERION_HSV_HUE:        HSV Hue
 * @CLAY_SELECT_CRITERION_HSV_SATURATION: HSV Saturation
 * @CLAY_SELECT_CRITERION_HSV_VALUE:      HSV Value
 * @CLAY_SELECT_CRITERION_LCH_LIGHTNESS:  LCh Lightness
 * @CLAY_SELECT_CRITERION_LCH_CHROMA:     LCh Chroma
 * @CLAY_SELECT_CRITERION_LCH_HUE:        LCh Hue
 * @CLAY_SELECT_CRITERION_ALPHA:          Alpha
 *
 * Criterions for color similarity.
 **/
#define CLAY_TYPE_SELECT_CRITERION (clay_select_criterion_get_type ())

GType clay_select_criterion_get_type (void);

typedef enum
{
  CLAY_SELECT_CRITERION_COMPOSITE,      /*< desc="Composite"      >*/
  CLAY_SELECT_CRITERION_RGB_RED,        /*< desc="Red"            >*/
  CLAY_SELECT_CRITERION_RGB_GREEN,      /*< desc="Green"          >*/
  CLAY_SELECT_CRITERION_RGB_BLUE,       /*< desc="Blue"           >*/
  CLAY_SELECT_CRITERION_HSV_HUE,        /*< desc="HSV Hue"        >*/
  CLAY_SELECT_CRITERION_HSV_SATURATION, /*< desc="HSV Saturation" >*/
  CLAY_SELECT_CRITERION_HSV_VALUE,      /*< desc="HSV Value"      >*/
  CLAY_SELECT_CRITERION_LCH_LIGHTNESS,  /*< desc="LCh Lightness"  >*/
  CLAY_SELECT_CRITERION_LCH_CHROMA,     /*< desc="LCh Chroma"     >*/
  CLAY_SELECT_CRITERION_LCH_HUE,        /*< desc="LCh Hue"        >*/
  CLAY_SELECT_CRITERION_ALPHA,          /*< desc="Alpha"          >*/
} ClaySelectCriterion;


/**
 * ClaySizeType:
 * @CLAY_PIXELS: Pixels
 * @CLAY_POINTS: Points
 *
 * Size types for the old-style text API.
 **/
#define CLAY_TYPE_SIZE_TYPE (clay_size_type_get_type ())

GType clay_size_type_get_type (void);

typedef enum
{
  CLAY_PIXELS,  /*< desc="Pixels" >*/
  CLAY_POINTS   /*< desc="Points" >*/
} ClaySizeType;


/**
 * ClayStackTraceMode:
 * @CLAY_STACK_TRACE_NEVER:  Never
 * @CLAY_STACK_TRACE_QUERY:  Ask each time
 * @CLAY_STACK_TRACE_ALWAYS: Always
 *
 * When to generate stack traces in case of an error.
 **/
#define CLAY_TYPE_STACK_TRACE_MODE (clay_stack_trace_mode_get_type ())

GType clay_stack_trace_mode_get_type (void);

typedef enum
{
  CLAY_STACK_TRACE_NEVER,
  CLAY_STACK_TRACE_QUERY,
  CLAY_STACK_TRACE_ALWAYS
} ClayStackTraceMode;


/**
 * ClayStrokeMethod:
 * @CLAY_STROKE_LINE:         Stroke line
 * @CLAY_STROKE_PAINT_METHOD: Stroke with a paint tool
 *
 * Methods of stroking selections and paths.
 **/
#define CLAY_TYPE_STROKE_METHOD (clay_stroke_method_get_type ())

GType clay_stroke_method_get_type (void);

typedef enum
{
  CLAY_STROKE_LINE,         /*< desc="Stroke line"              >*/
  CLAY_STROKE_PAINT_METHOD  /*< desc="Stroke with a paint tool" >*/
} ClayStrokeMethod;


/**
 * ClayTextDirection:
 * @CLAY_TEXT_DIRECTION_LTR: From left to right
 * @CLAY_TEXT_DIRECTION_RTL: From right to left
 * @CLAY_TEXT_DIRECTION_TTB_RTL: Characters are from top to bottom, Lines are from right to left
 * @CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT: Upright characters are from top to bottom, Lines are from right to left
 * @CLAY_TEXT_DIRECTION_TTB_LTR: Characters are from top to bottom, Lines are from left to right
 * @CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT: Upright characters are from top to bottom, Lines are from left to right
 *
 * Text directions.
 **/
#define CLAY_TYPE_TEXT_DIRECTION (clay_text_direction_get_type ())

GType clay_text_direction_get_type (void);

typedef enum
{
  CLAY_TEXT_DIRECTION_LTR,              /*< desc="From left to right"                                     >*/
  CLAY_TEXT_DIRECTION_RTL,              /*< desc="From right to left"                                     >*/
  CLAY_TEXT_DIRECTION_TTB_RTL,          /*< desc="Vertical, right to left (mixed orientation)"  >*/
  CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT,  /*< desc="Vertical, right to left (upright orientation)" >*/
  CLAY_TEXT_DIRECTION_TTB_LTR,          /*< desc="Vertical, left to right (mixed orientation)"  >*/
  CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT   /*< desc="Vertical, left to right (upright orientation)" >*/
} ClayTextDirection;


/**
 * ClayTextHintStyle:
 * @CLAY_TEXT_HINT_STYLE_NONE:   None
 * @CLAY_TEXT_HINT_STYLE_SLIGHT: Slight
 * @CLAY_TEXT_HINT_STYLE_MEDIUM: Medium
 * @CLAY_TEXT_HINT_STYLE_FULL:   Full
 *
 * Text hint strengths.
 **/
#define CLAY_TYPE_TEXT_HINT_STYLE (clay_text_hint_style_get_type ())

GType clay_text_hint_style_get_type (void);

typedef enum
{
  CLAY_TEXT_HINT_STYLE_NONE,     /*< desc="None"   >*/
  CLAY_TEXT_HINT_STYLE_SLIGHT,   /*< desc="Slight" >*/
  CLAY_TEXT_HINT_STYLE_MEDIUM,   /*< desc="Medium" >*/
  CLAY_TEXT_HINT_STYLE_FULL      /*< desc="Full"   >*/
} ClayTextHintStyle;


/**
 * ClayTextJustification:
 * @CLAY_TEXT_JUSTIFY_LEFT:   Left justified
 * @CLAY_TEXT_JUSTIFY_RIGHT:  Right justified
 * @CLAY_TEXT_JUSTIFY_CENTER: Centered
 * @CLAY_TEXT_JUSTIFY_FILL:   Filled
 *
 * Text justifications.
 **/
#define CLAY_TYPE_TEXT_JUSTIFICATION (clay_text_justification_get_type ())

GType clay_text_justification_get_type (void);

typedef enum
{
  CLAY_TEXT_JUSTIFY_LEFT,    /*< desc="Left justified"  >*/
  CLAY_TEXT_JUSTIFY_RIGHT,   /*< desc="Right justified" >*/
  CLAY_TEXT_JUSTIFY_CENTER,  /*< desc="Centered"        >*/
  CLAY_TEXT_JUSTIFY_FILL     /*< desc="Filled"          >*/
} ClayTextJustification;

/**
 * ClayTextOutline:
 * @CLAY_TEXT_OUTLINE_NONE:        Filled
 * @CLAY_TEXT_OUTLINE_STROKE_ONLY: Outlined
 * @CLAY_TEXT_OUTLINE_STROKE_FILL: Outlined and filled
 *
 * Settings for text stroke and fill.
 **/
#define CLAY_TYPE_TEXT_OUTLINE (clay_text_outline_get_type ())

GType clay_text_outline_get_type (void);

typedef enum
{
  CLAY_TEXT_OUTLINE_NONE,        /*< desc="Filled"              >*/
  CLAY_TEXT_OUTLINE_STROKE_ONLY, /*< desc="Outlined"            >*/
  CLAY_TEXT_OUTLINE_STROKE_FILL  /*< desc="Outlined and filled" >*/
} ClayTextOutline;

/**
 * ClayTextOutlineDirection:
 * @CLAY_TEXT_OUTLINE_DIRECTION_OUTER:    Outer
 * @CLAY_TEXT_OUTLINE_DIRECTION_INNER:    Inner
 * @CLAY_TEXT_OUTLINE_DIRECTION_CENTERED: Centered
 *
 * Options for how the text outline's stroke is drawn.
 **/
#define CLAY_TYPE_TEXT_OUTLINE_DIRECTION (clay_text_outline_direction_get_type ())

GType clay_text_outline_direction_get_type (void);

typedef enum
{
  CLAY_TEXT_OUTLINE_DIRECTION_OUTER,   /*< desc="Outer"    >*/
  CLAY_TEXT_OUTLINE_DIRECTION_INNER,   /*< desc="Inner"    >*/
  CLAY_TEXT_OUTLINE_DIRECTION_CENTERED /*< desc="Centered" >*/
} ClayTextOutlineDirection;

/**
 * ClayTransferMode:
 * @CLAY_TRANSFER_SHADOWS:    Shadows
 * @CLAY_TRANSFER_MIDTONES:   Midtones
 * @CLAY_TRANSFER_HIGHLIGHTS: Highlights
 *
 * For choosing which brightness ranges to transform.
 **/
#define CLAY_TYPE_TRANSFER_MODE (clay_transfer_mode_get_type ())

GType clay_transfer_mode_get_type (void);

typedef enum
{
  CLAY_TRANSFER_SHADOWS,     /*< desc="Shadows"    >*/
  CLAY_TRANSFER_MIDTONES,    /*< desc="Midtones"   >*/
  CLAY_TRANSFER_HIGHLIGHTS   /*< desc="Highlights" >*/
} ClayTransferMode;


/**
 * ClayTransformDirection:
 * @CLAY_TRANSFORM_FORWARD:  Normal (Forward)
 * @CLAY_TRANSFORM_BACKWARD: Corrective (Backward)
 *
 * Transform directions.
 **/
#define CLAY_TYPE_TRANSFORM_DIRECTION (clay_transform_direction_get_type ())

GType clay_transform_direction_get_type (void);

typedef enum
{
  CLAY_TRANSFORM_FORWARD,   /*< desc="Normal (Forward)" >*/
  CLAY_TRANSFORM_BACKWARD   /*< desc="Corrective (Backward)" >*/
} ClayTransformDirection;


/**
 * ClayTransformResize:
 * @CLAY_TRANSFORM_RESIZE_ADJUST:           Adjust
 * @CLAY_TRANSFORM_RESIZE_CLIP:             Clip
 * @CLAY_TRANSFORM_RESIZE_CROP:             Crop to result
 * @CLAY_TRANSFORM_RESIZE_CROP_WITH_ASPECT: Crop with aspect
 *
 * Ways of clipping the result when transforming drawables.
 **/
#define CLAY_TYPE_TRANSFORM_RESIZE (clay_transform_resize_get_type ())

GType clay_transform_resize_get_type (void);

typedef enum
{
  CLAY_TRANSFORM_RESIZE_ADJUST,           /*< desc="Adjust"           >*/
  CLAY_TRANSFORM_RESIZE_CLIP,             /*< desc="Clip"             >*/
  CLAY_TRANSFORM_RESIZE_CROP,             /*< desc="Crop to result"   >*/
  CLAY_TRANSFORM_RESIZE_CROP_WITH_ASPECT  /*< desc="Crop with aspect" >*/
} ClayTransformResize;


/**
 * ClayUnitID:
 * @CLAY_UNIT_PIXEL:   Pixels
 * @CLAY_UNIT_INCH:    Inches
 * @CLAY_UNIT_MM:      Millimeters
 * @CLAY_UNIT_POINT:   Points
 * @CLAY_UNIT_PICA:    Picas
 * @CLAY_UNIT_END:     Marker for end-of-builtin-units
 * @CLAY_UNIT_PERCENT: Pseudo-unit percent
 *
 * Integer IDs of built-in units used for dimensions in images. These
 * IDs are meant to stay stable but user-created units IDs may change
 * from one session to another.
 *
 * It means you may compare these with the integer returned by
 * [method@Clay.Unit.get_id] and it will stay stable across sessions.
 * For all other non built-in units, you should be careful at comparing
 * IDs across sessions.
 **/
#define CLAY_TYPE_UNIT_ID (clay_unit_id_get_type ())

GType clay_unit_id_get_type (void);

typedef enum
{
  CLAY_UNIT_PIXEL   = 0,

  CLAY_UNIT_INCH    = 1,
  CLAY_UNIT_MM      = 2,
  CLAY_UNIT_POINT   = 3,
  CLAY_UNIT_PICA    = 4,

  CLAY_UNIT_END     = 5,

  CLAY_UNIT_PERCENT = 65536 /*< pdb-skip >*/
} ClayUnitID;


/**
 * ClayPathStrokeType:
 * @CLAY_PATH_STROKE_TYPE_BEZIER: A bezier stroke
 *
 * Possible type of strokes in path objects.
 **/
#define CLAY_TYPE_PATH_STROKE_TYPE (clay_path_stroke_type_get_type ())

GType clay_path_stroke_type_get_type (void);

typedef enum
{
  CLAY_PATH_STROKE_TYPE_BEZIER
} ClayPathStrokeType;


/**
 * ClayExportCapabilities:
 * @CLAY_EXPORT_CAN_HANDLE_RGB:                 Handles RGB images
 * @CLAY_EXPORT_CAN_HANDLE_GRAY:                Handles grayscale images
 * @CLAY_EXPORT_CAN_HANDLE_INDEXED:             Handles indexed images
 * @CLAY_EXPORT_CAN_HANDLE_BITMAP:              Handles two-color indexed images
 * @CLAY_EXPORT_CAN_HANDLE_ALPHA:               Handles alpha channels
 * @CLAY_EXPORT_CAN_HANDLE_LAYERS:              Handles layers
 * @CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION: Handles animation of layers
 * @CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS:       Handles layer effects
 * @CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS:         Handles layer masks
 * @CLAY_EXPORT_NEEDS_ALPHA:                    Needs alpha channels
 * @CLAY_EXPORT_NEEDS_CROP:                     Needs to crop content to image bounds
 *
 * The types of images and layers an export procedure can handle
 **/
#define CLAY_TYPE_EXPORT_CAPABILITIES (clay_export_capabilities_get_type ())

GType clay_export_capabilities_get_type (void);

typedef enum
{
  CLAY_EXPORT_CAN_HANDLE_RGB                 = 1 << 0,
  CLAY_EXPORT_CAN_HANDLE_GRAY                = 1 << 1,
  CLAY_EXPORT_CAN_HANDLE_INDEXED             = 1 << 2,
  CLAY_EXPORT_CAN_HANDLE_BITMAP              = 1 << 3,
  CLAY_EXPORT_CAN_HANDLE_ALPHA               = 1 << 4,
  CLAY_EXPORT_CAN_HANDLE_LAYERS              = 1 << 5,
  CLAY_EXPORT_CAN_HANDLE_LAYERS_AS_ANIMATION = 1 << 6,
  CLAY_EXPORT_CAN_HANDLE_LAYER_MASKS         = 1 << 7,
  CLAY_EXPORT_CAN_HANDLE_LAYER_EFFECTS       = 1 << 8,
  CLAY_EXPORT_NEEDS_ALPHA                    = 1 << 9,
  CLAY_EXPORT_NEEDS_CROP                     = 1 << 10
} ClayExportCapabilities;


/**
 * ClayFileChooserAction:
 * @CLAY_FILE_CHOOSER_ACTION_ANY:           No restriction.
 * @CLAY_FILE_CHOOSER_ACTION_OPEN:          Opens an existing file.
 * @CLAY_FILE_CHOOSER_ACTION_SAVE:          Saves a file (over a new file or an existing one.
 * @CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER: Picks an existing folder.
 * @CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER: Picks an existing or new folder.
 *
 * A type of file to choose from when actions are expected to choose a
 * file. This is basically a mapping to %GtkFileChooserAction except for
 * [enum@Clay.FileChooserAction.ANY] which should not be used for any
 * GUI functions since we can't know what you are looking for.
 **/
#define CLAY_TYPE_FILE_CHOOSER_ACTION (clay_file_chooser_action_get_type ())

GType clay_file_chooser_action_get_type (void);

typedef enum
{
  CLAY_FILE_CHOOSER_ACTION_ANY           = -1,
  CLAY_FILE_CHOOSER_ACTION_OPEN          = 0,
  CLAY_FILE_CHOOSER_ACTION_SAVE          = 1,
  CLAY_FILE_CHOOSER_ACTION_SELECT_FOLDER = 2,
  CLAY_FILE_CHOOSER_ACTION_CREATE_FOLDER = 3,
} ClayFileChooserAction;


G_END_DECLS

#endif  /* __CLAY_BASE_ENUMS_H__ */
