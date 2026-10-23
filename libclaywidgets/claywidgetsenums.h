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

#ifndef __CLAY_WIDGETS_ENUMS_H__
#define __CLAY_WIDGETS_ENUMS_H__


G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/**
 * ClayAspectType:
 * @CLAY_ASPECT_SQUARE:    it's a 1:1 square
 * @CLAY_ASPECT_PORTRAIT:  it's higher than it's wide
 * @CLAY_ASPECT_LANDSCAPE: it's wider than it's high
 *
 * Aspect ratios.
 **/
#define CLAY_TYPE_ASPECT_TYPE (clay_aspect_type_get_type ())

GType clay_aspect_type_get_type (void);

typedef enum
{
  CLAY_ASPECT_SQUARE,    /*< desc="Square"    >*/
  CLAY_ASPECT_PORTRAIT,  /*< desc="Portrait"  >*/
  CLAY_ASPECT_LANDSCAPE  /*< desc="Landscape" >*/
} ClayAspectType;


/**
 * ClayChainPosition:
 * @CLAY_CHAIN_TOP:    the chain is on top
 * @CLAY_CHAIN_LEFT:   the chain is to the left
 * @CLAY_CHAIN_BOTTOM: the chain is on bottom
 * @CLAY_CHAIN_RIGHT:  the chain is to the right
 *
 * Possible chain positions for #ClayChainButton.
 **/
#define CLAY_TYPE_CHAIN_POSITION (clay_chain_position_get_type ())

GType clay_chain_position_get_type (void);

typedef enum
{
  CLAY_CHAIN_TOP,
  CLAY_CHAIN_LEFT,
  CLAY_CHAIN_BOTTOM,
  CLAY_CHAIN_RIGHT
} ClayChainPosition;


/**
 * ClayColorAreaType:
 * @CLAY_COLOR_AREA_FLAT:         don't display transparency
 * @CLAY_COLOR_AREA_SMALL_CHECKS: display transparency using small checks
 * @CLAY_COLOR_AREA_LARGE_CHECKS: display transparency using large checks
 *
 * The types of transparency display for #ClayColorArea.
 **/
#define CLAY_TYPE_COLOR_AREA_TYPE (clay_color_area_type_get_type ())

GType clay_color_area_type_get_type (void);

typedef enum
{
  CLAY_COLOR_AREA_FLAT = 0,
  CLAY_COLOR_AREA_SMALL_CHECKS,
  CLAY_COLOR_AREA_LARGE_CHECKS
} ClayColorAreaType;


/**
 * ClayColorSelectorChannel:
 * @CLAY_COLOR_SELECTOR_HUE:            the hue channel
 * @CLAY_COLOR_SELECTOR_SATURATION:     the saturation channel
 * @CLAY_COLOR_SELECTOR_VALUE:          the value channel
 * @CLAY_COLOR_SELECTOR_RED:            the red channel
 * @CLAY_COLOR_SELECTOR_GREEN:          the green channel
 * @CLAY_COLOR_SELECTOR_BLUE:           the blue channel
 * @CLAY_COLOR_SELECTOR_ALPHA:          the alpha channel
 * @CLAY_COLOR_SELECTOR_LCH_LIGHTNESS:  the lightness channel
 * @CLAY_COLOR_SELECTOR_LCH_CHROMA:     the chroma channel
 * @CLAY_COLOR_SELECTOR_LCH_HUE:        the hue channel
 *
 * An enum to specify the types of color channels edited in
 * #ClayColorSelector widgets.
 **/
#define CLAY_TYPE_COLOR_SELECTOR_CHANNEL (clay_color_selector_channel_get_type ())

GType clay_color_selector_channel_get_type (void);

typedef enum
{
  CLAY_COLOR_SELECTOR_HUE,           /*< desc="_H", help="HSV Hue"        >*/
  CLAY_COLOR_SELECTOR_SATURATION,    /*< desc="_S", help="HSV Saturation" >*/
  CLAY_COLOR_SELECTOR_VALUE,         /*< desc="_V", help="HSV Value"      >*/
  CLAY_COLOR_SELECTOR_RED,           /*< desc="_R", help="Red"            >*/
  CLAY_COLOR_SELECTOR_GREEN,         /*< desc="_G", help="Green"          >*/
  CLAY_COLOR_SELECTOR_BLUE,          /*< desc="_B", help="Blue"           >*/
  CLAY_COLOR_SELECTOR_ALPHA,         /*< desc="_A", help="Alpha"          >*/
  CLAY_COLOR_SELECTOR_LCH_LIGHTNESS, /*< desc="_L", help="LCh Lightness"  >*/
  CLAY_COLOR_SELECTOR_LCH_CHROMA,    /*< desc="_C", help="LCh Chroma"     >*/
  CLAY_COLOR_SELECTOR_LCH_HUE        /*< desc="_h", help="LCh Hue"        >*/
} ClayColorSelectorChannel;


/**
 * ClayColorSelectorModel:
 * @CLAY_COLOR_SELECTOR_MODEL_RGB: RGB color model
 * @CLAY_COLOR_SELECTOR_MODEL_LCH: CIE LCh color model
 * @CLAY_COLOR_SELECTOR_MODEL_HSV: HSV color model
 *
 * An enum to specify the types of color spaces edited in
 * #ClayColorSelector widgets.
 *
 * Since: 2.10
 **/
#define CLAY_TYPE_COLOR_SELECTOR_MODEL (clay_color_selector_model_get_type ())

GType clay_color_selector_model_get_type (void);

typedef enum
{
  CLAY_COLOR_SELECTOR_MODEL_RGB, /*< desc="RGB", help="RGB color model"     >*/
  CLAY_COLOR_SELECTOR_MODEL_LCH, /*< desc="LCH", help="CIE LCh color model" >*/
  CLAY_COLOR_SELECTOR_MODEL_HSV  /*< desc="HSV", help="HSV color model"     >*/
} ClayColorSelectorModel;


/**
 * ClayIntComboBoxLayout:
 * @CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY:   show icons only
 * @CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED: show icons and abbreviated labels,
 *                                         when available
 * @CLAY_INT_COMBO_BOX_LAYOUT_FULL:        show icons and full labels
 *
 * Possible layouts for #ClayIntComboBox.
 *
 * Since: 2.10
 **/
#define CLAY_TYPE_INT_COMBO_BOX_LAYOUT (clay_int_combo_box_layout_get_type ())

GType clay_int_combo_box_layout_get_type (void);

typedef enum
{
  CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY,
  CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED,
  CLAY_INT_COMBO_BOX_LAYOUT_FULL
} ClayIntComboBoxLayout;


/**
 * ClayPageSelectorTarget:
 * @CLAY_PAGE_SELECTOR_TARGET_LAYERS: import as layers of one image
 * @CLAY_PAGE_SELECTOR_TARGET_IMAGES: import as separate images
 *
 * Import targets for #ClayPageSelector.
 **/
#define CLAY_TYPE_PAGE_SELECTOR_TARGET (clay_page_selector_target_get_type ())

GType clay_page_selector_target_get_type (void);

typedef enum
{
  CLAY_PAGE_SELECTOR_TARGET_LAYERS, /*< desc="Layers" >*/
  CLAY_PAGE_SELECTOR_TARGET_IMAGES  /*< desc="Images" >*/
} ClayPageSelectorTarget;


/**
 * ClaySizeEntryUpdatePolicy:
 * @CLAY_SIZE_ENTRY_UPDATE_NONE:       the size entry's meaning is up to the user
 * @CLAY_SIZE_ENTRY_UPDATE_SIZE:       the size entry displays values
 * @CLAY_SIZE_ENTRY_UPDATE_RESOLUTION: the size entry displays resolutions
 *
 * Update policies for #ClaySizeEntry.
 **/
#define CLAY_TYPE_SIZE_ENTRY_UPDATE_POLICY (clay_size_entry_update_policy_get_type ())

GType clay_size_entry_update_policy_get_type (void);

typedef enum
{
  CLAY_SIZE_ENTRY_UPDATE_NONE       = 0,
  CLAY_SIZE_ENTRY_UPDATE_SIZE       = 1,
  CLAY_SIZE_ENTRY_UPDATE_RESOLUTION = 2
} ClaySizeEntryUpdatePolicy;


/**
 * ClayZoomType:
 * @CLAY_ZOOM_IN:       zoom in
 * @CLAY_ZOOM_OUT:      zoom out
 * @CLAY_ZOOM_IN_MORE:  zoom in a lot
 * @CLAY_ZOOM_OUT_MORE: zoom out a lot
 * @CLAY_ZOOM_IN_MAX:   zoom in as far as possible
 * @CLAY_ZOOM_OUT_MAX:  zoom out as far as possible
 * @CLAY_ZOOM_TO:       zoom to a specific zoom factor
 * @CLAY_ZOOM_SMOOTH:   zoom smoothly from a smooth scroll event
 * @CLAY_ZOOM_PINCH:    zoom smoothly from a touchpad pinch gesture
 *
 * the zoom types for #ClayZoomModel.
 **/
#define CLAY_TYPE_ZOOM_TYPE (clay_zoom_type_get_type ())

GType clay_zoom_type_get_type (void);

typedef enum
{
  CLAY_ZOOM_IN,        /*< desc="Zoom in"  >*/
  CLAY_ZOOM_OUT,       /*< desc="Zoom out" >*/
  CLAY_ZOOM_IN_MORE,   /*< skip >*/
  CLAY_ZOOM_OUT_MORE,  /*< skip >*/
  CLAY_ZOOM_IN_MAX,    /*< skip >*/
  CLAY_ZOOM_OUT_MAX,   /*< skip >*/
  CLAY_ZOOM_TO,        /*< skip >*/
  CLAY_ZOOM_SMOOTH,    /*< skip >*/
  CLAY_ZOOM_PINCH,     /*< skip >*/
} ClayZoomType;


G_END_DECLS

#endif  /* __CLAY_WIDGETS_ENUMS_H__ */
