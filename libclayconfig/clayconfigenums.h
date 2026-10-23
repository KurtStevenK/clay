/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayconfigenums.h
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
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

#ifndef __CLAY_CONFIG_ENUMS_H__
#define __CLAY_CONFIG_ENUMS_H__


#define CLAY_TYPE_COLOR_MANAGEMENT_MODE (clay_color_management_mode_get_type ())

GType clay_color_management_mode_get_type (void);

/**
 * ClayColorManagementMode:
 * @CLAY_COLOR_MANAGEMENT_OFF:       Color management is off
 * @CLAY_COLOR_MANAGEMENT_DISPLAY:   Color managed display
 * @CLAY_COLOR_MANAGEMENT_SOFTPROOF: Soft-proofing
 *
 * Modes of color management.
 **/
typedef enum
{
  CLAY_COLOR_MANAGEMENT_OFF,       /*< desc="No color management"   >*/
  CLAY_COLOR_MANAGEMENT_DISPLAY,   /*< desc="Color-managed display" >*/
  CLAY_COLOR_MANAGEMENT_SOFTPROOF  /*< desc="Soft-proofing"      >*/
} ClayColorManagementMode;


#define CLAY_TYPE_COLOR_RENDERING_INTENT (clay_color_rendering_intent_get_type ())

GType clay_color_rendering_intent_get_type (void);

/**
 * ClayColorRenderingIntent:
 * @CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL:            Perceptual
 * @CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC: Relative colorimetric
 * @CLAY_COLOR_RENDERING_INTENT_SATURATION:            Saturation
 * @CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC: Absolute colorimetric
 *
 * Intents for color management.
 **/
typedef enum
{
  CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL,            /*< desc="Perceptual"            >*/
  CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, /*< desc="Relative colorimetric" >*/
  CLAY_COLOR_RENDERING_INTENT_SATURATION,            /*< desc="Saturation"            >*/
  CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC  /*< desc="Absolute colorimetric" >*/
} ClayColorRenderingIntent;


#endif /* __CLAY_CONFIG_ENUMS_H__ */
