
/* Generated data (by clay-mkenums) */

#include "stamp-claywidgetsenums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "claywidgetsenums.h"
#include "libclay/libclay-intl.h"

/* enumerations from "claywidgetsenums.h" */
GType
clay_aspect_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ASPECT_SQUARE, "CLAY_ASPECT_SQUARE", "square" },
    { CLAY_ASPECT_PORTRAIT, "CLAY_ASPECT_PORTRAIT", "portrait" },
    { CLAY_ASPECT_LANDSCAPE, "CLAY_ASPECT_LANDSCAPE", "landscape" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ASPECT_SQUARE, NC_("aspect-type", "Square"), NULL },
    { CLAY_ASPECT_PORTRAIT, NC_("aspect-type", "Portrait"), NULL },
    { CLAY_ASPECT_LANDSCAPE, NC_("aspect-type", "Landscape"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayAspectType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "aspect-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_chain_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHAIN_TOP, "CLAY_CHAIN_TOP", "top" },
    { CLAY_CHAIN_LEFT, "CLAY_CHAIN_LEFT", "left" },
    { CLAY_CHAIN_BOTTOM, "CLAY_CHAIN_BOTTOM", "bottom" },
    { CLAY_CHAIN_RIGHT, "CLAY_CHAIN_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHAIN_TOP, "CLAY_CHAIN_TOP", NULL },
    { CLAY_CHAIN_LEFT, "CLAY_CHAIN_LEFT", NULL },
    { CLAY_CHAIN_BOTTOM, "CLAY_CHAIN_BOTTOM", NULL },
    { CLAY_CHAIN_RIGHT, "CLAY_CHAIN_RIGHT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayChainPosition", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "chain-position");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_area_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_AREA_FLAT, "CLAY_COLOR_AREA_FLAT", "flat" },
    { CLAY_COLOR_AREA_SMALL_CHECKS, "CLAY_COLOR_AREA_SMALL_CHECKS", "small-checks" },
    { CLAY_COLOR_AREA_LARGE_CHECKS, "CLAY_COLOR_AREA_LARGE_CHECKS", "large-checks" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_AREA_FLAT, "CLAY_COLOR_AREA_FLAT", NULL },
    { CLAY_COLOR_AREA_SMALL_CHECKS, "CLAY_COLOR_AREA_SMALL_CHECKS", NULL },
    { CLAY_COLOR_AREA_LARGE_CHECKS, "CLAY_COLOR_AREA_LARGE_CHECKS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorAreaType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-area-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_selector_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_SELECTOR_HUE, "CLAY_COLOR_SELECTOR_HUE", "hue" },
    { CLAY_COLOR_SELECTOR_SATURATION, "CLAY_COLOR_SELECTOR_SATURATION", "saturation" },
    { CLAY_COLOR_SELECTOR_VALUE, "CLAY_COLOR_SELECTOR_VALUE", "value" },
    { CLAY_COLOR_SELECTOR_RED, "CLAY_COLOR_SELECTOR_RED", "red" },
    { CLAY_COLOR_SELECTOR_GREEN, "CLAY_COLOR_SELECTOR_GREEN", "green" },
    { CLAY_COLOR_SELECTOR_BLUE, "CLAY_COLOR_SELECTOR_BLUE", "blue" },
    { CLAY_COLOR_SELECTOR_ALPHA, "CLAY_COLOR_SELECTOR_ALPHA", "alpha" },
    { CLAY_COLOR_SELECTOR_LCH_LIGHTNESS, "CLAY_COLOR_SELECTOR_LCH_LIGHTNESS", "lch-lightness" },
    { CLAY_COLOR_SELECTOR_LCH_CHROMA, "CLAY_COLOR_SELECTOR_LCH_CHROMA", "lch-chroma" },
    { CLAY_COLOR_SELECTOR_LCH_HUE, "CLAY_COLOR_SELECTOR_LCH_HUE", "lch-hue" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_SELECTOR_HUE, NC_("color-selector-channel", "_H"), N_("HSV Hue") },
    { CLAY_COLOR_SELECTOR_SATURATION, NC_("color-selector-channel", "_S"), N_("HSV Saturation") },
    { CLAY_COLOR_SELECTOR_VALUE, NC_("color-selector-channel", "_V"), N_("HSV Value") },
    { CLAY_COLOR_SELECTOR_RED, NC_("color-selector-channel", "_R"), N_("Red") },
    { CLAY_COLOR_SELECTOR_GREEN, NC_("color-selector-channel", "_G"), N_("Green") },
    { CLAY_COLOR_SELECTOR_BLUE, NC_("color-selector-channel", "_B"), N_("Blue") },
    { CLAY_COLOR_SELECTOR_ALPHA, NC_("color-selector-channel", "_A"), N_("Alpha") },
    { CLAY_COLOR_SELECTOR_LCH_LIGHTNESS, NC_("color-selector-channel", "_L"), N_("LCh Lightness") },
    { CLAY_COLOR_SELECTOR_LCH_CHROMA, NC_("color-selector-channel", "_C"), N_("LCh Chroma") },
    { CLAY_COLOR_SELECTOR_LCH_HUE, NC_("color-selector-channel", "_h"), N_("LCh Hue") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorSelectorChannel", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-selector-channel");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_selector_model_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_SELECTOR_MODEL_RGB, "CLAY_COLOR_SELECTOR_MODEL_RGB", "rgb" },
    { CLAY_COLOR_SELECTOR_MODEL_LCH, "CLAY_COLOR_SELECTOR_MODEL_LCH", "lch" },
    { CLAY_COLOR_SELECTOR_MODEL_HSV, "CLAY_COLOR_SELECTOR_MODEL_HSV", "hsv" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_SELECTOR_MODEL_RGB, NC_("color-selector-model", "RGB"), N_("RGB color model") },
    { CLAY_COLOR_SELECTOR_MODEL_LCH, NC_("color-selector-model", "LCH"), N_("CIE LCh color model") },
    { CLAY_COLOR_SELECTOR_MODEL_HSV, NC_("color-selector-model", "HSV"), N_("HSV color model") },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorSelectorModel", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-selector-model");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_int_combo_box_layout_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY, "CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY", "icon-only" },
    { CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED, "CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED", "abbreviated" },
    { CLAY_INT_COMBO_BOX_LAYOUT_FULL, "CLAY_INT_COMBO_BOX_LAYOUT_FULL", "full" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY, "CLAY_INT_COMBO_BOX_LAYOUT_ICON_ONLY", NULL },
    { CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED, "CLAY_INT_COMBO_BOX_LAYOUT_ABBREVIATED", NULL },
    { CLAY_INT_COMBO_BOX_LAYOUT_FULL, "CLAY_INT_COMBO_BOX_LAYOUT_FULL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayIntComboBoxLayout", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "int-combo-box-layout");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_page_selector_target_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PAGE_SELECTOR_TARGET_LAYERS, "CLAY_PAGE_SELECTOR_TARGET_LAYERS", "layers" },
    { CLAY_PAGE_SELECTOR_TARGET_IMAGES, "CLAY_PAGE_SELECTOR_TARGET_IMAGES", "images" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PAGE_SELECTOR_TARGET_LAYERS, NC_("page-selector-target", "Layers"), NULL },
    { CLAY_PAGE_SELECTOR_TARGET_IMAGES, NC_("page-selector-target", "Images"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPageSelectorTarget", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "page-selector-target");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_size_entry_update_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_SIZE_ENTRY_UPDATE_NONE, "CLAY_SIZE_ENTRY_UPDATE_NONE", "none" },
    { CLAY_SIZE_ENTRY_UPDATE_SIZE, "CLAY_SIZE_ENTRY_UPDATE_SIZE", "size" },
    { CLAY_SIZE_ENTRY_UPDATE_RESOLUTION, "CLAY_SIZE_ENTRY_UPDATE_RESOLUTION", "resolution" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_SIZE_ENTRY_UPDATE_NONE, "CLAY_SIZE_ENTRY_UPDATE_NONE", NULL },
    { CLAY_SIZE_ENTRY_UPDATE_SIZE, "CLAY_SIZE_ENTRY_UPDATE_SIZE", NULL },
    { CLAY_SIZE_ENTRY_UPDATE_RESOLUTION, "CLAY_SIZE_ENTRY_UPDATE_RESOLUTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySizeEntryUpdatePolicy", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "size-entry-update-policy");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_zoom_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ZOOM_IN, "CLAY_ZOOM_IN", "in" },
    { CLAY_ZOOM_OUT, "CLAY_ZOOM_OUT", "out" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ZOOM_IN, NC_("zoom-type", "Zoom in"), NULL },
    { CLAY_ZOOM_OUT, NC_("zoom-type", "Zoom out"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayZoomType", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "zoom-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

