/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * operations-enums.h
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


#define CLAY_TYPE_LAYER_COLOR_SPACE (clay_layer_color_space_get_type ())

GType clay_layer_color_space_get_type (void);

typedef enum
{
  CLAY_LAYER_COLOR_SPACE_AUTO,           /*< desc="Auto"             >*/
  CLAY_LAYER_COLOR_SPACE_RGB_LINEAR,     /*< desc="RGB (linear)"     >*/
  CLAY_LAYER_COLOR_SPACE_RGB_NON_LINEAR, /*< desc="RGB (from color profile)" >*/
  CLAY_LAYER_COLOR_SPACE_LAB,            /*< desc="LAB"              >*/
  CLAY_LAYER_COLOR_SPACE_RGB_PERCEPTUAL, /*< desc="RGB (perceptual)" >*/
  CLAY_LAYER_COLOR_SPACE_LAST,           /*< pdb-skip, skip          >*/
} ClayLayerColorSpace;


#define CLAY_TYPE_LAYER_COMPOSITE_MODE (clay_layer_composite_mode_get_type ())

GType clay_layer_composite_mode_get_type (void);

typedef enum
{
  CLAY_LAYER_COMPOSITE_AUTO,             /*< desc="Auto"             >*/
  CLAY_LAYER_COMPOSITE_UNION,            /*< desc="Union"            >*/
  CLAY_LAYER_COMPOSITE_CLIP_TO_BACKDROP, /*< desc="Clip to backdrop" >*/
  CLAY_LAYER_COMPOSITE_CLIP_TO_LAYER,    /*< desc="Clip to layer"    >*/
  CLAY_LAYER_COMPOSITE_INTERSECTION      /*< desc="Intersection"     >*/
} ClayLayerCompositeMode;


#define CLAY_TYPE_LAYER_MODE (clay_layer_mode_get_type ())

GType clay_layer_mode_get_type (void);

typedef enum
{
  /*  Modes that exist since ancient times  */
  CLAY_LAYER_MODE_NORMAL_LEGACY,         /*< desc="Normal (legacy)",         abbrev="Normal (l)"            >*/
  CLAY_LAYER_MODE_DISSOLVE,              /*< desc="Dissolve"                                                >*/
  CLAY_LAYER_MODE_BEHIND_LEGACY,         /*< desc="Behind (legacy)",         abbrev="Behind (l)"            >*/
  CLAY_LAYER_MODE_MULTIPLY_LEGACY,       /*< desc="Multiply (legacy)",       abbrev="Multiply (l)"          >*/
  CLAY_LAYER_MODE_SCREEN_LEGACY,         /*< desc="Screen (legacy)",         abbrev="Screen (l)"            >*/
  CLAY_LAYER_MODE_OVERLAY_LEGACY,        /*< desc="Old broken Overlay",      abbrev="Old Overlay"           >*/
  CLAY_LAYER_MODE_DIFFERENCE_LEGACY,     /*< desc="Difference (legacy)",     abbrev="Difference (l)"        >*/
  CLAY_LAYER_MODE_ADDITION_LEGACY,       /*< desc="Addition (legacy)",       abbrev="Addition (l)"          >*/
  CLAY_LAYER_MODE_SUBTRACT_LEGACY,       /*< desc="Subtract (legacy)",       abbrev="Subtract (l)"          >*/
  CLAY_LAYER_MODE_DARKEN_ONLY_LEGACY,    /*< desc="Darken only (legacy)",    abbrev="Darken only (l)"       >*/
  CLAY_LAYER_MODE_LIGHTEN_ONLY_LEGACY,   /*< desc="Lighten only (legacy)",   abbrev="Lighten only (l)"      >*/
  CLAY_LAYER_MODE_HSV_HUE_LEGACY,        /*< desc="HSV Hue (legacy)",        abbrev="HSV Hue (l)"           >*/
  CLAY_LAYER_MODE_HSV_SATURATION_LEGACY, /*< desc="HSV Saturation (legacy)", abbrev="HSV Saturation (l)"    >*/
  CLAY_LAYER_MODE_HSL_COLOR_LEGACY,      /*< desc="HSL Color (legacy)",      abbrev="HSL Color (l)"         >*/
  CLAY_LAYER_MODE_HSV_VALUE_LEGACY,      /*< desc="HSV Value (legacy)",      abbrev="HSV Value (l)"         >*/
  CLAY_LAYER_MODE_DIVIDE_LEGACY,         /*< desc="Divide (legacy)",         abbrev="Divide (l)"            >*/
  CLAY_LAYER_MODE_DODGE_LEGACY,          /*< desc="Dodge (legacy)",          abbrev="Dodge (l)"             >*/
  CLAY_LAYER_MODE_BURN_LEGACY,           /*< desc="Burn (legacy)",           abbrev="Burn (l)"              >*/
  CLAY_LAYER_MODE_HARDLIGHT_LEGACY,      /*< desc="Hard light (legacy)",     abbrev="Hard light (l)"        >*/

  /*  Since 2.8 (XCF version 2)  */
  CLAY_LAYER_MODE_SOFTLIGHT_LEGACY,      /*< desc="Soft light (legacy)",     abbrev="Soft light (l)"        >*/
  CLAY_LAYER_MODE_GRAIN_EXTRACT_LEGACY,  /*< desc="Grain extract (legacy)",  abbrev="Grain extract (l)"     >*/
  CLAY_LAYER_MODE_GRAIN_MERGE_LEGACY,    /*< desc="Grain merge (legacy)",    abbrev="Grain merge (l)"       >*/
  CLAY_LAYER_MODE_COLOR_ERASE_LEGACY,    /*< desc="Color erase (legacy)",    abbrev="Color erase (l)"       >*/

  /*  Since 2.10 (XCF version 9) */
  CLAY_LAYER_MODE_OVERLAY,               /*< desc="Overlay"                                                 >*/
  CLAY_LAYER_MODE_LCH_HUE,               /*< desc="LCh Hue"                                                 >*/
  CLAY_LAYER_MODE_LCH_CHROMA,            /*< desc="LCh Chroma"                                              >*/
  CLAY_LAYER_MODE_LCH_COLOR,             /*< desc="LCh Color"                                               >*/
  CLAY_LAYER_MODE_LCH_LIGHTNESS,         /*< desc="LCh Lightness"                                           >*/

  /*  Since 2.10 (XCF version 10)  */
  CLAY_LAYER_MODE_NORMAL,                /*< desc="Normal"                                                  >*/
  CLAY_LAYER_MODE_BEHIND,                /*< desc="Behind"                                                  >*/
  CLAY_LAYER_MODE_MULTIPLY,              /*< desc="Multiply"                                                >*/
  CLAY_LAYER_MODE_SCREEN,                /*< desc="Screen"                                                  >*/
  CLAY_LAYER_MODE_DIFFERENCE,            /*< desc="Difference"                                              >*/
  CLAY_LAYER_MODE_ADDITION,              /*< desc="Addition"                                                >*/
  CLAY_LAYER_MODE_SUBTRACT,              /*< desc="Subtract"                                                >*/
  CLAY_LAYER_MODE_DARKEN_ONLY,           /*< desc="Darken only"                                             >*/
  CLAY_LAYER_MODE_LIGHTEN_ONLY,          /*< desc="Lighten only"                                            >*/
  CLAY_LAYER_MODE_HSV_HUE,               /*< desc="HSV Hue"                                                 >*/
  CLAY_LAYER_MODE_HSV_SATURATION,        /*< desc="HSV Saturation"                                          >*/
  CLAY_LAYER_MODE_HSL_COLOR,             /*< desc="HSL Color"                                               >*/
  CLAY_LAYER_MODE_HSV_VALUE,             /*< desc="HSV Value"                                               >*/
  CLAY_LAYER_MODE_DIVIDE,                /*< desc="Divide"                                                  >*/
  CLAY_LAYER_MODE_DODGE,                 /*< desc="Dodge"                                                   >*/
  CLAY_LAYER_MODE_BURN,                  /*< desc="Burn"                                                    >*/
  CLAY_LAYER_MODE_HARDLIGHT,             /*< desc="Hard light"                                              >*/
  CLAY_LAYER_MODE_SOFTLIGHT,             /*< desc="Soft light"                                              >*/
  CLAY_LAYER_MODE_GRAIN_EXTRACT,         /*< desc="Grain extract"                                           >*/
  CLAY_LAYER_MODE_GRAIN_MERGE,           /*< desc="Grain merge"                                             >*/
  CLAY_LAYER_MODE_VIVID_LIGHT,           /*< desc="Vivid light"                                             >*/
  CLAY_LAYER_MODE_PIN_LIGHT,             /*< desc="Pin light"                                               >*/
  CLAY_LAYER_MODE_LINEAR_LIGHT,          /*< desc="Linear light"                                            >*/
  CLAY_LAYER_MODE_HARD_MIX,              /*< desc="Hard mix"                                                >*/
  CLAY_LAYER_MODE_EXCLUSION,             /*< desc="Exclusion"                                               >*/
  CLAY_LAYER_MODE_LINEAR_BURN,           /*< desc="Linear burn"                                             >*/
  CLAY_LAYER_MODE_LUMA_DARKEN_ONLY,      /*< desc="Luma/Luminance darken only",  abbrev="Luma darken only"  >*/
  CLAY_LAYER_MODE_LUMA_LIGHTEN_ONLY,     /*< desc="Luma/Luminance lighten only", abbrev="Luma lighten only" >*/
  CLAY_LAYER_MODE_LUMINANCE,             /*< desc="Luminance"                                               >*/
  CLAY_LAYER_MODE_COLOR_ERASE,           /*< desc="Color erase"                                             >*/
  CLAY_LAYER_MODE_ERASE,                 /*< desc="Erase"                                                   >*/
  CLAY_LAYER_MODE_MERGE,                 /*< desc="Merge"                                                   >*/
  CLAY_LAYER_MODE_SPLIT,                 /*< desc="Split"                                                   >*/
  CLAY_LAYER_MODE_PASS_THROUGH,          /*< desc="Pass through"                                            >*/

  /*  Mode only used for drawable filters. */
  CLAY_LAYER_MODE_REPLACE,               /*< desc="Replace"                                                 >*/

  /*  Since 3.0 (paint mode only) */
  CLAY_LAYER_MODE_OVERWRITE,             /*< desc="Overwrite"                                               >*/

  /*  Internal modes, not available to the PDB, must be kept at the end  */
  CLAY_LAYER_MODE_ANTI_ERASE,            /*< pdb-skip, desc="Anti erase"                                    >*/

  /*  Layer mode menu separator  */
  CLAY_LAYER_MODE_SEPARATOR = -1         /*< pdb-skip, skip                                                 >*/
} ClayLayerMode;


#define CLAY_TYPE_LAYER_MODE_GROUP (clay_layer_mode_group_get_type ())

GType clay_layer_mode_group_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_LAYER_MODE_GROUP_DEFAULT,     /*< desc="Default"      >*/
  CLAY_LAYER_MODE_GROUP_LEGACY,      /*< desc="Legacy"       >*/
} ClayLayerModeGroup;


#define CLAY_TYPE_LAYER_MODE_CONTEXT (clay_layer_mode_context_get_type ())

GType clay_layer_mode_context_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_LAYER_MODE_CONTEXT_LAYER  = 1 << 0,
  CLAY_LAYER_MODE_CONTEXT_GROUP  = 1 << 1,
  CLAY_LAYER_MODE_CONTEXT_PAINT  = 1 << 2,
  CLAY_LAYER_MODE_CONTEXT_FILTER = 1 << 3,

  CLAY_LAYER_MODE_CONTEXT_ALL = (CLAY_LAYER_MODE_CONTEXT_LAYER |
                                 CLAY_LAYER_MODE_CONTEXT_GROUP |
                                 CLAY_LAYER_MODE_CONTEXT_PAINT |
                                 CLAY_LAYER_MODE_CONTEXT_FILTER)
} ClayLayerModeContext;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_LAYER_COMPOSITE_REGION_INTERSECTION = 0,
  CLAY_LAYER_COMPOSITE_REGION_DESTINATION  = 1 << 0,
  CLAY_LAYER_COMPOSITE_REGION_SOURCE       = 1 << 1,
  CLAY_LAYER_COMPOSITE_REGION_UNION        = (CLAY_LAYER_COMPOSITE_REGION_DESTINATION |
                                              CLAY_LAYER_COMPOSITE_REGION_SOURCE),
} ClayLayerCompositeRegion;

typedef enum  /*< pdb-skip, skip >*/
{
  CLAY_LAYER_MODE_FLAG_LEGACY                    =  1 << 0,
  CLAY_LAYER_MODE_FLAG_BLEND_SPACE_IMMUTABLE     =  1 << 1,
  CLAY_LAYER_MODE_FLAG_COMPOSITE_SPACE_IMMUTABLE =  1 << 2,
  CLAY_LAYER_MODE_FLAG_COMPOSITE_MODE_IMMUTABLE  =  1 << 3,
  CLAY_LAYER_MODE_FLAG_SUBTRACTIVE               =  1 << 4,
  CLAY_LAYER_MODE_FLAG_ALPHA_ONLY                =  1 << 5,
  CLAY_LAYER_MODE_FLAG_TRIVIAL                   =  1 << 6
} ClayLayerModeFlags;
