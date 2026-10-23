
/* Generated data (by clay-mkenums) */

#include "stamp-core-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "core-enums.h"
#include "clay-intl.h"

/* enumerations from "core-enums.h" */
GType
clay_align_reference_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ALIGN_REFERENCE_IMAGE, "CLAY_ALIGN_REFERENCE_IMAGE", "image" },
    { CLAY_ALIGN_REFERENCE_SELECTION, "CLAY_ALIGN_REFERENCE_SELECTION", "selection" },
    { CLAY_ALIGN_REFERENCE_PICK, "CLAY_ALIGN_REFERENCE_PICK", "pick" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ALIGN_REFERENCE_IMAGE, NC_("align-reference-type", "Image"), NULL },
    { CLAY_ALIGN_REFERENCE_SELECTION, NC_("align-reference-type", "Selection"), NULL },
    { CLAY_ALIGN_REFERENCE_PICK, NC_("align-reference-type", "Picked reference object"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayAlignReferenceType", values);
      clay_type_set_translation_context (type, "align-reference-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_alignment_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ALIGN_LEFT, "CLAY_ALIGN_LEFT", "align-left" },
    { CLAY_ALIGN_HCENTER, "CLAY_ALIGN_HCENTER", "align-hcenter" },
    { CLAY_ALIGN_RIGHT, "CLAY_ALIGN_RIGHT", "align-right" },
    { CLAY_ALIGN_TOP, "CLAY_ALIGN_TOP", "align-top" },
    { CLAY_ALIGN_VCENTER, "CLAY_ALIGN_VCENTER", "align-vcenter" },
    { CLAY_ALIGN_BOTTOM, "CLAY_ALIGN_BOTTOM", "align-bottom" },
    { CLAY_ARRANGE_HFILL, "CLAY_ARRANGE_HFILL", "arrange-hfill" },
    { CLAY_ARRANGE_VFILL, "CLAY_ARRANGE_VFILL", "arrange-vfill" },
    { CLAY_DISTRIBUTE_EVEN_HORIZONTAL_GAP, "CLAY_DISTRIBUTE_EVEN_HORIZONTAL_GAP", "distribute-even-horizontal-gap" },
    { CLAY_DISTRIBUTE_EVEN_VERTICAL_GAP, "CLAY_DISTRIBUTE_EVEN_VERTICAL_GAP", "distribute-even-vertical-gap" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ALIGN_LEFT, NC_("alignment-type", "Align to the left"), NULL },
    { CLAY_ALIGN_HCENTER, NC_("alignment-type", "Center horizontally"), NULL },
    { CLAY_ALIGN_RIGHT, NC_("alignment-type", "Align to the right"), NULL },
    { CLAY_ALIGN_TOP, NC_("alignment-type", "Align to the top"), NULL },
    { CLAY_ALIGN_VCENTER, NC_("alignment-type", "Center vertically"), NULL },
    { CLAY_ALIGN_BOTTOM, NC_("alignment-type", "Align to the bottom"), NULL },
    { CLAY_ARRANGE_HFILL, NC_("alignment-type", "Distribute anchor points horizontally evenly"), NULL },
    { CLAY_ARRANGE_VFILL, NC_("alignment-type", "Distribute anchor points vertically evenly"), NULL },
    { CLAY_DISTRIBUTE_EVEN_HORIZONTAL_GAP, NC_("alignment-type", "Distribute horizontally with even horizontal gaps"), NULL },
    { CLAY_DISTRIBUTE_EVEN_VERTICAL_GAP, NC_("alignment-type", "Distribute vertically with even vertical gaps"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayAlignmentType", values);
      clay_type_set_translation_context (type, "alignment-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_bucket_fill_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BUCKET_FILL_FG, "CLAY_BUCKET_FILL_FG", "fg" },
    { CLAY_BUCKET_FILL_BG, "CLAY_BUCKET_FILL_BG", "bg" },
    { CLAY_BUCKET_FILL_PATTERN, "CLAY_BUCKET_FILL_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BUCKET_FILL_FG, NC_("bucket-fill-mode", "FG color fill"), NULL },
    { CLAY_BUCKET_FILL_BG, NC_("bucket-fill-mode", "BG color fill"), NULL },
    { CLAY_BUCKET_FILL_PATTERN, NC_("bucket-fill-mode", "Pattern fill"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayBucketFillMode", values);
      clay_type_set_translation_context (type, "bucket-fill-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_channel_border_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CHANNEL_BORDER_STYLE_HARD, "CLAY_CHANNEL_BORDER_STYLE_HARD", "hard" },
    { CLAY_CHANNEL_BORDER_STYLE_SMOOTH, "CLAY_CHANNEL_BORDER_STYLE_SMOOTH", "smooth" },
    { CLAY_CHANNEL_BORDER_STYLE_FEATHERED, "CLAY_CHANNEL_BORDER_STYLE_FEATHERED", "feathered" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CHANNEL_BORDER_STYLE_HARD, NC_("channel-border-style", "Hard"), NULL },
    { CLAY_CHANNEL_BORDER_STYLE_SMOOTH, NC_("channel-border-style", "Smooth"), NULL },
    { CLAY_CHANNEL_BORDER_STYLE_FEATHERED, NC_("channel-border-style", "Feathered"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayChannelBorderStyle", values);
      clay_type_set_translation_context (type, "channel-border-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_pick_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_PICK_MODE_PIXEL, "CLAY_COLOR_PICK_MODE_PIXEL", "pixel" },
    { CLAY_COLOR_PICK_MODE_RGB_PERCENT, "CLAY_COLOR_PICK_MODE_RGB_PERCENT", "rgb-percent" },
    { CLAY_COLOR_PICK_MODE_RGB_U8, "CLAY_COLOR_PICK_MODE_RGB_U8", "rgb-u8" },
    { CLAY_COLOR_PICK_MODE_GRAYSCALE, "CLAY_COLOR_PICK_MODE_GRAYSCALE", "grayscale" },
    { CLAY_COLOR_PICK_MODE_HSV, "CLAY_COLOR_PICK_MODE_HSV", "hsv" },
    { CLAY_COLOR_PICK_MODE_LCH, "CLAY_COLOR_PICK_MODE_LCH", "lch" },
    { CLAY_COLOR_PICK_MODE_LAB, "CLAY_COLOR_PICK_MODE_LAB", "lab" },
    { CLAY_COLOR_PICK_MODE_CMYK, "CLAY_COLOR_PICK_MODE_CMYK", "cmyk" },
    { CLAY_COLOR_PICK_MODE_XYY, "CLAY_COLOR_PICK_MODE_XYY", "xyy" },
    { CLAY_COLOR_PICK_MODE_YUV, "CLAY_COLOR_PICK_MODE_YUV", "yuv" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_PICK_MODE_PIXEL, NC_("color-pick-mode", "Pixel"), NULL },
    { CLAY_COLOR_PICK_MODE_RGB_PERCENT, NC_("color-pick-mode", "RGB (%)"), NULL },
    { CLAY_COLOR_PICK_MODE_RGB_U8, NC_("color-pick-mode", "RGB (0..255)"), NULL },
    { CLAY_COLOR_PICK_MODE_GRAYSCALE, NC_("color-pick-mode", "Grayscale (%)"), NULL },
    { CLAY_COLOR_PICK_MODE_HSV, NC_("color-pick-mode", "HSV"), NULL },
    { CLAY_COLOR_PICK_MODE_LCH, NC_("color-pick-mode", "CIE LCh"), NULL },
    { CLAY_COLOR_PICK_MODE_LAB, NC_("color-pick-mode", "CIE LAB"), NULL },
    { CLAY_COLOR_PICK_MODE_CMYK, NC_("color-pick-mode", "CMYK"), NULL },
    { CLAY_COLOR_PICK_MODE_XYY, NC_("color-pick-mode", "CIE xyY"), NULL },
    { CLAY_COLOR_PICK_MODE_YUV, NC_("color-pick-mode", "CIE Yu'v'"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorPickMode", values);
      clay_type_set_translation_context (type, "color-pick-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_profile_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_PROFILE_POLICY_ASK, "CLAY_COLOR_PROFILE_POLICY_ASK", "ask" },
    { CLAY_COLOR_PROFILE_POLICY_KEEP, "CLAY_COLOR_PROFILE_POLICY_KEEP", "keep" },
    { CLAY_COLOR_PROFILE_POLICY_CONVERT_BUILTIN, "CLAY_COLOR_PROFILE_POLICY_CONVERT_BUILTIN", "convert-builtin" },
    { CLAY_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, "CLAY_COLOR_PROFILE_POLICY_CONVERT_PREFERRED", "convert-preferred" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_PROFILE_POLICY_ASK, NC_("color-profile-policy", "Ask what to do"), NULL },
    { CLAY_COLOR_PROFILE_POLICY_KEEP, NC_("color-profile-policy", "Keep embedded profile"), NULL },
    { CLAY_COLOR_PROFILE_POLICY_CONVERT_BUILTIN, NC_("color-profile-policy", "Convert to built-in sRGB or grayscale profile"), NULL },
    { CLAY_COLOR_PROFILE_POLICY_CONVERT_PREFERRED, NC_("color-profile-policy", "Convert to preferred RGB or grayscale profile (defaulting to built-in)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorProfilePolicy", values);
      clay_type_set_translation_context (type, "color-profile-policy");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_component_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_COMPONENT_MASK_RED, "CLAY_COMPONENT_MASK_RED", "red" },
    { CLAY_COMPONENT_MASK_GREEN, "CLAY_COMPONENT_MASK_GREEN", "green" },
    { CLAY_COMPONENT_MASK_BLUE, "CLAY_COMPONENT_MASK_BLUE", "blue" },
    { CLAY_COMPONENT_MASK_ALPHA, "CLAY_COMPONENT_MASK_ALPHA", "alpha" },
    { CLAY_COMPONENT_MASK_ALL, "CLAY_COMPONENT_MASK_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_COMPONENT_MASK_RED, "CLAY_COMPONENT_MASK_RED", NULL },
    { CLAY_COMPONENT_MASK_GREEN, "CLAY_COMPONENT_MASK_GREEN", NULL },
    { CLAY_COMPONENT_MASK_BLUE, "CLAY_COMPONENT_MASK_BLUE", NULL },
    { CLAY_COMPONENT_MASK_ALPHA, "CLAY_COMPONENT_MASK_ALPHA", NULL },
    { CLAY_COMPONENT_MASK_ALL, "CLAY_COMPONENT_MASK_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayComponentMask", values);
      clay_type_set_translation_context (type, "component-mask");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_container_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CONTAINER_POLICY_STRONG, "CLAY_CONTAINER_POLICY_STRONG", "strong" },
    { CLAY_CONTAINER_POLICY_WEAK, "CLAY_CONTAINER_POLICY_WEAK", "weak" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CONTAINER_POLICY_STRONG, "CLAY_CONTAINER_POLICY_STRONG", NULL },
    { CLAY_CONTAINER_POLICY_WEAK, "CLAY_CONTAINER_POLICY_WEAK", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayContainerPolicy", values);
      clay_type_set_translation_context (type, "container-policy");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_convert_dither_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CONVERT_DITHER_NONE, "CLAY_CONVERT_DITHER_NONE", "none" },
    { CLAY_CONVERT_DITHER_FS, "CLAY_CONVERT_DITHER_FS", "fs" },
    { CLAY_CONVERT_DITHER_FS_LOWBLEED, "CLAY_CONVERT_DITHER_FS_LOWBLEED", "fs-lowbleed" },
    { CLAY_CONVERT_DITHER_FIXED, "CLAY_CONVERT_DITHER_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CONVERT_DITHER_NONE, NC_("convert-dither-type", "None"), NULL },
    { CLAY_CONVERT_DITHER_FS, NC_("convert-dither-type", "Floyd-Steinberg (normal)"), NULL },
    { CLAY_CONVERT_DITHER_FS_LOWBLEED, NC_("convert-dither-type", "Floyd-Steinberg (reduced color bleeding)"), NULL },
    { CLAY_CONVERT_DITHER_FIXED, NC_("convert-dither-type", "Positioned"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayConvertDitherType", values);
      clay_type_set_translation_context (type, "convert-dither-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_convolution_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_NORMAL_CONVOL, "CLAY_NORMAL_CONVOL", "normal-convol" },
    { CLAY_ABSOLUTE_CONVOL, "CLAY_ABSOLUTE_CONVOL", "absolute-convol" },
    { CLAY_NEGATIVE_CONVOL, "CLAY_NEGATIVE_CONVOL", "negative-convol" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_NORMAL_CONVOL, "CLAY_NORMAL_CONVOL", NULL },
    { CLAY_ABSOLUTE_CONVOL, "CLAY_ABSOLUTE_CONVOL", NULL },
    { CLAY_NEGATIVE_CONVOL, "CLAY_NEGATIVE_CONVOL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayConvolutionType", values);
      clay_type_set_translation_context (type, "convolution-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_curve_point_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CURVE_POINT_SMOOTH, "CLAY_CURVE_POINT_SMOOTH", "smooth" },
    { CLAY_CURVE_POINT_CORNER, "CLAY_CURVE_POINT_CORNER", "corner" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CURVE_POINT_SMOOTH, NC_("curve-point-type", "Smooth"), NULL },
    { CLAY_CURVE_POINT_CORNER, NC_("curve-point-type", "Corner"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCurvePointType", values);
      clay_type_set_translation_context (type, "curve-point-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_curve_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CURVE_SMOOTH, "CLAY_CURVE_SMOOTH", "smooth" },
    { CLAY_CURVE_FREE, "CLAY_CURVE_FREE", "free" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CURVE_SMOOTH, NC_("curve-type", "Smooth"), NULL },
    { CLAY_CURVE_FREE, NC_("curve-type", "Freehand"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCurveType", values);
      clay_type_set_translation_context (type, "curve-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_dash_preset_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_DASH_CUSTOM, "CLAY_DASH_CUSTOM", "custom" },
    { CLAY_DASH_LINE, "CLAY_DASH_LINE", "line" },
    { CLAY_DASH_LONG_DASH, "CLAY_DASH_LONG_DASH", "long-dash" },
    { CLAY_DASH_MEDIUM_DASH, "CLAY_DASH_MEDIUM_DASH", "medium-dash" },
    { CLAY_DASH_SHORT_DASH, "CLAY_DASH_SHORT_DASH", "short-dash" },
    { CLAY_DASH_SPARSE_DOTS, "CLAY_DASH_SPARSE_DOTS", "sparse-dots" },
    { CLAY_DASH_NORMAL_DOTS, "CLAY_DASH_NORMAL_DOTS", "normal-dots" },
    { CLAY_DASH_DENSE_DOTS, "CLAY_DASH_DENSE_DOTS", "dense-dots" },
    { CLAY_DASH_STIPPLES, "CLAY_DASH_STIPPLES", "stipples" },
    { CLAY_DASH_DASH_DOT, "CLAY_DASH_DASH_DOT", "dash-dot" },
    { CLAY_DASH_DASH_DOT_DOT, "CLAY_DASH_DASH_DOT_DOT", "dash-dot-dot" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_DASH_CUSTOM, NC_("dash-preset", "Custom"), NULL },
    { CLAY_DASH_LINE, NC_("dash-preset", "Line"), NULL },
    { CLAY_DASH_LONG_DASH, NC_("dash-preset", "Long dashes"), NULL },
    { CLAY_DASH_MEDIUM_DASH, NC_("dash-preset", "Medium dashes"), NULL },
    { CLAY_DASH_SHORT_DASH, NC_("dash-preset", "Short dashes"), NULL },
    { CLAY_DASH_SPARSE_DOTS, NC_("dash-preset", "Sparse dots"), NULL },
    { CLAY_DASH_NORMAL_DOTS, NC_("dash-preset", "Normal dots"), NULL },
    { CLAY_DASH_DENSE_DOTS, NC_("dash-preset", "Dense dots"), NULL },
    { CLAY_DASH_STIPPLES, NC_("dash-preset", "Stipples"), NULL },
    { CLAY_DASH_DASH_DOT, NC_("dash-preset", "Dash, dot"), NULL },
    { CLAY_DASH_DASH_DOT_DOT, NC_("dash-preset", "Dash, dot, dot"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDashPreset", values);
      clay_type_set_translation_context (type, "dash-preset");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_debug_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_DEBUG_POLICY_WARNING, "CLAY_DEBUG_POLICY_WARNING", "warning" },
    { CLAY_DEBUG_POLICY_CRITICAL, "CLAY_DEBUG_POLICY_CRITICAL", "critical" },
    { CLAY_DEBUG_POLICY_FATAL, "CLAY_DEBUG_POLICY_FATAL", "fatal" },
    { CLAY_DEBUG_POLICY_NEVER, "CLAY_DEBUG_POLICY_NEVER", "never" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_DEBUG_POLICY_WARNING, NC_("debug-policy", "Debug warnings, critical errors and crashes"), NULL },
    { CLAY_DEBUG_POLICY_CRITICAL, NC_("debug-policy", "Debug critical errors and crashes"), NULL },
    { CLAY_DEBUG_POLICY_FATAL, NC_("debug-policy", "Debug crashes only"), NULL },
    { CLAY_DEBUG_POLICY_NEVER, NC_("debug-policy", "Never debug Clay"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDebugPolicy", values);
      clay_type_set_translation_context (type, "debug-policy");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_dirty_mask_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_DIRTY_NONE, "CLAY_DIRTY_NONE", "none" },
    { CLAY_DIRTY_IMAGE, "CLAY_DIRTY_IMAGE", "image" },
    { CLAY_DIRTY_IMAGE_SIZE, "CLAY_DIRTY_IMAGE_SIZE", "image-size" },
    { CLAY_DIRTY_IMAGE_META, "CLAY_DIRTY_IMAGE_META", "image-meta" },
    { CLAY_DIRTY_IMAGE_STRUCTURE, "CLAY_DIRTY_IMAGE_STRUCTURE", "image-structure" },
    { CLAY_DIRTY_ITEM, "CLAY_DIRTY_ITEM", "item" },
    { CLAY_DIRTY_ITEM_META, "CLAY_DIRTY_ITEM_META", "item-meta" },
    { CLAY_DIRTY_DRAWABLE, "CLAY_DIRTY_DRAWABLE", "drawable" },
    { CLAY_DIRTY_PATH, "CLAY_DIRTY_PATH", "path" },
    { CLAY_DIRTY_SELECTION, "CLAY_DIRTY_SELECTION", "selection" },
    { CLAY_DIRTY_ACTIVE_DRAWABLE, "CLAY_DIRTY_ACTIVE_DRAWABLE", "active-drawable" },
    { CLAY_DIRTY_ALL, "CLAY_DIRTY_ALL", "all" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_DIRTY_NONE, "CLAY_DIRTY_NONE", NULL },
    { CLAY_DIRTY_IMAGE, "CLAY_DIRTY_IMAGE", NULL },
    { CLAY_DIRTY_IMAGE_SIZE, "CLAY_DIRTY_IMAGE_SIZE", NULL },
    { CLAY_DIRTY_IMAGE_META, "CLAY_DIRTY_IMAGE_META", NULL },
    { CLAY_DIRTY_IMAGE_STRUCTURE, "CLAY_DIRTY_IMAGE_STRUCTURE", NULL },
    { CLAY_DIRTY_ITEM, "CLAY_DIRTY_ITEM", NULL },
    { CLAY_DIRTY_ITEM_META, "CLAY_DIRTY_ITEM_META", NULL },
    { CLAY_DIRTY_DRAWABLE, "CLAY_DIRTY_DRAWABLE", NULL },
    { CLAY_DIRTY_PATH, "CLAY_DIRTY_PATH", NULL },
    { CLAY_DIRTY_SELECTION, "CLAY_DIRTY_SELECTION", NULL },
    { CLAY_DIRTY_ACTIVE_DRAWABLE, "CLAY_DIRTY_ACTIVE_DRAWABLE", NULL },
    { CLAY_DIRTY_ALL, "CLAY_DIRTY_ALL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayDirtyMask", values);
      clay_type_set_translation_context (type, "dirty-mask");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_dynamics_output_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_DYNAMICS_OUTPUT_OPACITY, "CLAY_DYNAMICS_OUTPUT_OPACITY", "opacity" },
    { CLAY_DYNAMICS_OUTPUT_SIZE, "CLAY_DYNAMICS_OUTPUT_SIZE", "size" },
    { CLAY_DYNAMICS_OUTPUT_ANGLE, "CLAY_DYNAMICS_OUTPUT_ANGLE", "angle" },
    { CLAY_DYNAMICS_OUTPUT_COLOR, "CLAY_DYNAMICS_OUTPUT_COLOR", "color" },
    { CLAY_DYNAMICS_OUTPUT_HARDNESS, "CLAY_DYNAMICS_OUTPUT_HARDNESS", "hardness" },
    { CLAY_DYNAMICS_OUTPUT_FORCE, "CLAY_DYNAMICS_OUTPUT_FORCE", "force" },
    { CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO, "CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO", "aspect-ratio" },
    { CLAY_DYNAMICS_OUTPUT_SPACING, "CLAY_DYNAMICS_OUTPUT_SPACING", "spacing" },
    { CLAY_DYNAMICS_OUTPUT_RATE, "CLAY_DYNAMICS_OUTPUT_RATE", "rate" },
    { CLAY_DYNAMICS_OUTPUT_FLOW, "CLAY_DYNAMICS_OUTPUT_FLOW", "flow" },
    { CLAY_DYNAMICS_OUTPUT_JITTER, "CLAY_DYNAMICS_OUTPUT_JITTER", "jitter" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_DYNAMICS_OUTPUT_OPACITY, NC_("dynamics-output-type", "Opacity"), NULL },
    { CLAY_DYNAMICS_OUTPUT_SIZE, NC_("dynamics-output-type", "Size"), NULL },
    { CLAY_DYNAMICS_OUTPUT_ANGLE, NC_("dynamics-output-type", "Angle"), NULL },
    { CLAY_DYNAMICS_OUTPUT_COLOR, NC_("dynamics-output-type", "Color"), NULL },
    { CLAY_DYNAMICS_OUTPUT_HARDNESS, NC_("dynamics-output-type", "Hardness"), NULL },
    { CLAY_DYNAMICS_OUTPUT_FORCE, NC_("dynamics-output-type", "Force"), NULL },
    { CLAY_DYNAMICS_OUTPUT_ASPECT_RATIO, NC_("dynamics-output-type", "Aspect ratio"), NULL },
    { CLAY_DYNAMICS_OUTPUT_SPACING, NC_("dynamics-output-type", "Spacing"), NULL },
    { CLAY_DYNAMICS_OUTPUT_RATE, NC_("dynamics-output-type", "Rate"), NULL },
    { CLAY_DYNAMICS_OUTPUT_FLOW, NC_("dynamics-output-type", "Flow"), NULL },
    { CLAY_DYNAMICS_OUTPUT_JITTER, NC_("dynamics-output-type", "Jitter"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDynamicsOutputType", values);
      clay_type_set_translation_context (type, "dynamics-output-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_custom_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CUSTOM_STYLE_SOLID_COLOR, "CLAY_CUSTOM_STYLE_SOLID_COLOR", "solid-color" },
    { CLAY_CUSTOM_STYLE_PATTERN, "CLAY_CUSTOM_STYLE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CUSTOM_STYLE_SOLID_COLOR, NC_("custom-style", "Solid color"), NULL },
    { CLAY_CUSTOM_STYLE_PATTERN, NC_("custom-style", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCustomStyle", values);
      clay_type_set_translation_context (type, "custom-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_fill_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FILL_STYLE_FG_COLOR, "CLAY_FILL_STYLE_FG_COLOR", "fg-color" },
    { CLAY_FILL_STYLE_BG_COLOR, "CLAY_FILL_STYLE_BG_COLOR", "bg-color" },
    { CLAY_FILL_STYLE_PATTERN, "CLAY_FILL_STYLE_PATTERN", "pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FILL_STYLE_FG_COLOR, NC_("fill-style", "Foreground color"), NULL },
    { CLAY_FILL_STYLE_BG_COLOR, NC_("fill-style", "Background color"), NULL },
    { CLAY_FILL_STYLE_PATTERN, NC_("fill-style", "Pattern"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayFillStyle", values);
      clay_type_set_translation_context (type, "fill-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_filter_region_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FILTER_REGION_SELECTION, "CLAY_FILTER_REGION_SELECTION", "selection" },
    { CLAY_FILTER_REGION_DRAWABLE, "CLAY_FILTER_REGION_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FILTER_REGION_SELECTION, NC_("filter-region", "Use the selection as input"), NULL },
    { CLAY_FILTER_REGION_DRAWABLE, NC_("filter-region", "Use the entire layer as input"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayFilterRegion", values);
      clay_type_set_translation_context (type, "filter-region");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gradient_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRADIENT_COLOR_FIXED, "CLAY_GRADIENT_COLOR_FIXED", "fixed" },
    { CLAY_GRADIENT_COLOR_FOREGROUND, "CLAY_GRADIENT_COLOR_FOREGROUND", "foreground" },
    { CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, "CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT", "foreground-transparent" },
    { CLAY_GRADIENT_COLOR_BACKGROUND, "CLAY_GRADIENT_COLOR_BACKGROUND", "background" },
    { CLAY_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, "CLAY_GRADIENT_COLOR_BACKGROUND_TRANSPARENT", "background-transparent" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRADIENT_COLOR_FIXED, NC_("gradient-color", "Fixed"), NULL },
    { CLAY_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "Foreground color"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color".
       Keep it short. */
    { CLAY_GRADIENT_COLOR_FOREGROUND, NC_("gradient-color", "FG"), NULL },
    { CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "Foreground color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Foreground color (transparent)".
       Keep it short. */
    { CLAY_GRADIENT_COLOR_FOREGROUND_TRANSPARENT, NC_("gradient-color", "FG (t)"), NULL },
    { CLAY_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "Background color"), NULL },
    /* Translators: this is an abbreviated version of "Background color".
       Keep it short. */
    { CLAY_GRADIENT_COLOR_BACKGROUND, NC_("gradient-color", "BG"), NULL },
    { CLAY_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "Background color (transparent)"), NULL },
    /* Translators: this is an abbreviated version of "Background color (transparent)".
       Keep it short. */
    { CLAY_GRADIENT_COLOR_BACKGROUND_TRANSPARENT, NC_("gradient-color", "BG (t)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGradientColor", values);
      clay_type_set_translation_context (type, "gradient-color");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_gravity_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GRAVITY_NONE, "CLAY_GRAVITY_NONE", "none" },
    { CLAY_GRAVITY_NORTH_WEST, "CLAY_GRAVITY_NORTH_WEST", "north-west" },
    { CLAY_GRAVITY_NORTH, "CLAY_GRAVITY_NORTH", "north" },
    { CLAY_GRAVITY_NORTH_EAST, "CLAY_GRAVITY_NORTH_EAST", "north-east" },
    { CLAY_GRAVITY_WEST, "CLAY_GRAVITY_WEST", "west" },
    { CLAY_GRAVITY_CENTER, "CLAY_GRAVITY_CENTER", "center" },
    { CLAY_GRAVITY_EAST, "CLAY_GRAVITY_EAST", "east" },
    { CLAY_GRAVITY_SOUTH_WEST, "CLAY_GRAVITY_SOUTH_WEST", "south-west" },
    { CLAY_GRAVITY_SOUTH, "CLAY_GRAVITY_SOUTH", "south" },
    { CLAY_GRAVITY_SOUTH_EAST, "CLAY_GRAVITY_SOUTH_EAST", "south-east" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GRAVITY_NONE, "CLAY_GRAVITY_NONE", NULL },
    { CLAY_GRAVITY_NORTH_WEST, "CLAY_GRAVITY_NORTH_WEST", NULL },
    { CLAY_GRAVITY_NORTH, "CLAY_GRAVITY_NORTH", NULL },
    { CLAY_GRAVITY_NORTH_EAST, "CLAY_GRAVITY_NORTH_EAST", NULL },
    { CLAY_GRAVITY_WEST, "CLAY_GRAVITY_WEST", NULL },
    { CLAY_GRAVITY_CENTER, "CLAY_GRAVITY_CENTER", NULL },
    { CLAY_GRAVITY_EAST, "CLAY_GRAVITY_EAST", NULL },
    { CLAY_GRAVITY_SOUTH_WEST, "CLAY_GRAVITY_SOUTH_WEST", NULL },
    { CLAY_GRAVITY_SOUTH, "CLAY_GRAVITY_SOUTH", NULL },
    { CLAY_GRAVITY_SOUTH_EAST, "CLAY_GRAVITY_SOUTH_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGravityType", values);
      clay_type_set_translation_context (type, "gravity-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_guide_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GUIDE_STYLE_NONE, "CLAY_GUIDE_STYLE_NONE", "none" },
    { CLAY_GUIDE_STYLE_NORMAL, "CLAY_GUIDE_STYLE_NORMAL", "normal" },
    { CLAY_GUIDE_STYLE_MIRROR, "CLAY_GUIDE_STYLE_MIRROR", "mirror" },
    { CLAY_GUIDE_STYLE_MANDALA, "CLAY_GUIDE_STYLE_MANDALA", "mandala" },
    { CLAY_GUIDE_STYLE_SPLIT_VIEW, "CLAY_GUIDE_STYLE_SPLIT_VIEW", "split-view" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GUIDE_STYLE_NONE, "CLAY_GUIDE_STYLE_NONE", NULL },
    { CLAY_GUIDE_STYLE_NORMAL, "CLAY_GUIDE_STYLE_NORMAL", NULL },
    { CLAY_GUIDE_STYLE_MIRROR, "CLAY_GUIDE_STYLE_MIRROR", NULL },
    { CLAY_GUIDE_STYLE_MANDALA, "CLAY_GUIDE_STYLE_MANDALA", NULL },
    { CLAY_GUIDE_STYLE_SPLIT_VIEW, "CLAY_GUIDE_STYLE_SPLIT_VIEW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGuideStyle", values);
      clay_type_set_translation_context (type, "guide-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_histogram_channel_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HISTOGRAM_VALUE, "CLAY_HISTOGRAM_VALUE", "value" },
    { CLAY_HISTOGRAM_RED, "CLAY_HISTOGRAM_RED", "red" },
    { CLAY_HISTOGRAM_GREEN, "CLAY_HISTOGRAM_GREEN", "green" },
    { CLAY_HISTOGRAM_BLUE, "CLAY_HISTOGRAM_BLUE", "blue" },
    { CLAY_HISTOGRAM_ALPHA, "CLAY_HISTOGRAM_ALPHA", "alpha" },
    { CLAY_HISTOGRAM_LUMINANCE, "CLAY_HISTOGRAM_LUMINANCE", "luminance" },
    { CLAY_HISTOGRAM_RGB, "CLAY_HISTOGRAM_RGB", "rgb" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HISTOGRAM_VALUE, NC_("histogram-channel", "Value"), NULL },
    { CLAY_HISTOGRAM_RED, NC_("histogram-channel", "Red"), NULL },
    { CLAY_HISTOGRAM_GREEN, NC_("histogram-channel", "Green"), NULL },
    { CLAY_HISTOGRAM_BLUE, NC_("histogram-channel", "Blue"), NULL },
    { CLAY_HISTOGRAM_ALPHA, NC_("histogram-channel", "Alpha"), NULL },
    { CLAY_HISTOGRAM_LUMINANCE, NC_("histogram-channel", "Luminance"), NULL },
    { CLAY_HISTOGRAM_RGB, NC_("histogram-channel", "RGB"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHistogramChannel", values);
      clay_type_set_translation_context (type, "histogram-channel");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_item_set_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ITEM_SET_NONE, "CLAY_ITEM_SET_NONE", "none" },
    { CLAY_ITEM_SET_ALL, "CLAY_ITEM_SET_ALL", "all" },
    { CLAY_ITEM_SET_IMAGE_SIZED, "CLAY_ITEM_SET_IMAGE_SIZED", "image-sized" },
    { CLAY_ITEM_SET_VISIBLE, "CLAY_ITEM_SET_VISIBLE", "visible" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ITEM_SET_NONE, NC_("item-set", "None"), NULL },
    { CLAY_ITEM_SET_ALL, NC_("item-set", "All layers"), NULL },
    { CLAY_ITEM_SET_IMAGE_SIZED, NC_("item-set", "Image-sized layers"), NULL },
    { CLAY_ITEM_SET_VISIBLE, NC_("item-set", "All visible layers"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayItemSet", values);
      clay_type_set_translation_context (type, "item-set");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_matting_engine_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MATTING_ENGINE_GLOBAL, "CLAY_MATTING_ENGINE_GLOBAL", "global" },
    { CLAY_MATTING_ENGINE_LEVIN, "CLAY_MATTING_ENGINE_LEVIN", "levin" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MATTING_ENGINE_GLOBAL, NC_("matting-engine", "Matting Global"), NULL },
    { CLAY_MATTING_ENGINE_LEVIN, NC_("matting-engine", "Matting Levin"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMattingEngine", values);
      clay_type_set_translation_context (type, "matting-engine");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_message_severity_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MESSAGE_INFO, "CLAY_MESSAGE_INFO", "info" },
    { CLAY_MESSAGE_WARNING, "CLAY_MESSAGE_WARNING", "warning" },
    { CLAY_MESSAGE_ERROR, "CLAY_MESSAGE_ERROR", "error" },
    { CLAY_MESSAGE_BUG_WARNING, "CLAY_MESSAGE_BUG_WARNING", "bug-warning" },
    { CLAY_MESSAGE_BUG_CRITICAL, "CLAY_MESSAGE_BUG_CRITICAL", "bug-critical" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MESSAGE_INFO, NC_("message-severity", "Message"), NULL },
    { CLAY_MESSAGE_WARNING, NC_("message-severity", "Warning"), NULL },
    { CLAY_MESSAGE_ERROR, NC_("message-severity", "Error"), NULL },
    { CLAY_MESSAGE_BUG_WARNING, NC_("message-severity", "WARNING"), NULL },
    { CLAY_MESSAGE_BUG_CRITICAL, NC_("message-severity", "CRITICAL"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMessageSeverity", values);
      clay_type_set_translation_context (type, "message-severity");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_metadata_rotation_policy_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_METADATA_ROTATION_POLICY_ASK, "CLAY_METADATA_ROTATION_POLICY_ASK", "ask" },
    { CLAY_METADATA_ROTATION_POLICY_KEEP, "CLAY_METADATA_ROTATION_POLICY_KEEP", "keep" },
    { CLAY_METADATA_ROTATION_POLICY_ROTATE, "CLAY_METADATA_ROTATION_POLICY_ROTATE", "rotate" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_METADATA_ROTATION_POLICY_ASK, NC_("metadata-rotation-policy", "Ask what to do"), NULL },
    { CLAY_METADATA_ROTATION_POLICY_KEEP, NC_("metadata-rotation-policy", "Discard metadata without rotating"), NULL },
    { CLAY_METADATA_ROTATION_POLICY_ROTATE, NC_("metadata-rotation-policy", "Rotate the image then discard metadata"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMetadataRotationPolicy", values);
      clay_type_set_translation_context (type, "metadata-rotation-policy");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_paste_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PASTE_TYPE_FLOATING, "CLAY_PASTE_TYPE_FLOATING", "floating" },
    { CLAY_PASTE_TYPE_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_FLOATING_IN_PLACE", "floating-in-place" },
    { CLAY_PASTE_TYPE_FLOATING_INTO, "CLAY_PASTE_TYPE_FLOATING_INTO", "floating-into" },
    { CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE", "floating-into-in-place" },
    { CLAY_PASTE_TYPE_NEW_LAYER, "CLAY_PASTE_TYPE_NEW_LAYER", "new-layer" },
    { CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE, "CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE", "new-layer-in-place" },
    { CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING, "CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING", "new-layer-or-floating" },
    { CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE", "new-layer-or-floating-in-place" },
    { CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, "CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING", "new-merged-layer-or-floating" },
    { CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE", "new-merged-layer-or-floating-in-place" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PASTE_TYPE_FLOATING, "CLAY_PASTE_TYPE_FLOATING", NULL },
    { CLAY_PASTE_TYPE_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_FLOATING_IN_PLACE", NULL },
    { CLAY_PASTE_TYPE_FLOATING_INTO, "CLAY_PASTE_TYPE_FLOATING_INTO", NULL },
    { CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE, "CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE", NULL },
    { CLAY_PASTE_TYPE_NEW_LAYER, "CLAY_PASTE_TYPE_NEW_LAYER", NULL },
    { CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE, "CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE", NULL },
    { CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING, "CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING", NULL },
    { CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE", NULL },
    { CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING, "CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING", NULL },
    { CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE, "CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPasteType", values);
      clay_type_set_translation_context (type, "paste-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_win32_pointer_input_api_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_WIN32_POINTER_INPUT_API_WINTAB, "CLAY_WIN32_POINTER_INPUT_API_WINTAB", "wintab" },
    { CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK, "CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK", "windows-ink" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_WIN32_POINTER_INPUT_API_WINTAB, NC_("win32-pointer-input-api", "Wintab"), NULL },
    { CLAY_WIN32_POINTER_INPUT_API_WINDOWS_INK, NC_("win32-pointer-input-api", "Windows Ink"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayWin32PointerInputAPI", values);
      clay_type_set_translation_context (type, "win32-pointer-input-api");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_thumbnail_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_THUMBNAIL_SIZE_NONE, "CLAY_THUMBNAIL_SIZE_NONE", "none" },
    { CLAY_THUMBNAIL_SIZE_NORMAL, "CLAY_THUMBNAIL_SIZE_NORMAL", "normal" },
    { CLAY_THUMBNAIL_SIZE_LARGE, "CLAY_THUMBNAIL_SIZE_LARGE", "large" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_THUMBNAIL_SIZE_NONE, NC_("thumbnail-size", "No thumbnails"), NULL },
    { CLAY_THUMBNAIL_SIZE_NORMAL, NC_("thumbnail-size", "Normal (128x128)"), NULL },
    { CLAY_THUMBNAIL_SIZE_LARGE, NC_("thumbnail-size", "Large (256x256)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayThumbnailSize", values);
      clay_type_set_translation_context (type, "thumbnail-size");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_trc_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRC_LINEAR, "CLAY_TRC_LINEAR", "linear" },
    { CLAY_TRC_NON_LINEAR, "CLAY_TRC_NON_LINEAR", "non-linear" },
    { CLAY_TRC_PERCEPTUAL, "CLAY_TRC_PERCEPTUAL", "perceptual" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRC_LINEAR, NC_("trc-type", "Linear"), NULL },
    { CLAY_TRC_NON_LINEAR, NC_("trc-type", "Non-Linear"), NULL },
    { CLAY_TRC_PERCEPTUAL, NC_("trc-type", "Perceptual"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTRCType", values);
      clay_type_set_translation_context (type, "trc-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_undo_event_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_UNDO_EVENT_UNDO_PUSHED, "CLAY_UNDO_EVENT_UNDO_PUSHED", "undo-pushed" },
    { CLAY_UNDO_EVENT_UNDO_EXPIRED, "CLAY_UNDO_EVENT_UNDO_EXPIRED", "undo-expired" },
    { CLAY_UNDO_EVENT_REDO_EXPIRED, "CLAY_UNDO_EVENT_REDO_EXPIRED", "redo-expired" },
    { CLAY_UNDO_EVENT_UNDO, "CLAY_UNDO_EVENT_UNDO", "undo" },
    { CLAY_UNDO_EVENT_REDO, "CLAY_UNDO_EVENT_REDO", "redo" },
    { CLAY_UNDO_EVENT_UNDO_FREE, "CLAY_UNDO_EVENT_UNDO_FREE", "undo-free" },
    { CLAY_UNDO_EVENT_UNDO_FREEZE, "CLAY_UNDO_EVENT_UNDO_FREEZE", "undo-freeze" },
    { CLAY_UNDO_EVENT_UNDO_THAW, "CLAY_UNDO_EVENT_UNDO_THAW", "undo-thaw" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_UNDO_EVENT_UNDO_PUSHED, "CLAY_UNDO_EVENT_UNDO_PUSHED", NULL },
    { CLAY_UNDO_EVENT_UNDO_EXPIRED, "CLAY_UNDO_EVENT_UNDO_EXPIRED", NULL },
    { CLAY_UNDO_EVENT_REDO_EXPIRED, "CLAY_UNDO_EVENT_REDO_EXPIRED", NULL },
    { CLAY_UNDO_EVENT_UNDO, "CLAY_UNDO_EVENT_UNDO", NULL },
    { CLAY_UNDO_EVENT_REDO, "CLAY_UNDO_EVENT_REDO", NULL },
    { CLAY_UNDO_EVENT_UNDO_FREE, "CLAY_UNDO_EVENT_UNDO_FREE", NULL },
    { CLAY_UNDO_EVENT_UNDO_FREEZE, "CLAY_UNDO_EVENT_UNDO_FREEZE", NULL },
    { CLAY_UNDO_EVENT_UNDO_THAW, "CLAY_UNDO_EVENT_UNDO_THAW", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayUndoEvent", values);
      clay_type_set_translation_context (type, "undo-event");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_undo_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_UNDO_MODE_UNDO, "CLAY_UNDO_MODE_UNDO", "undo" },
    { CLAY_UNDO_MODE_REDO, "CLAY_UNDO_MODE_REDO", "redo" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_UNDO_MODE_UNDO, "CLAY_UNDO_MODE_UNDO", NULL },
    { CLAY_UNDO_MODE_REDO, "CLAY_UNDO_MODE_REDO", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayUndoMode", values);
      clay_type_set_translation_context (type, "undo-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_undo_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_UNDO_GROUP_NONE, "CLAY_UNDO_GROUP_NONE", "group-none" },
    { CLAY_UNDO_GROUP_IMAGE_SCALE, "CLAY_UNDO_GROUP_IMAGE_SCALE", "group-image-scale" },
    { CLAY_UNDO_GROUP_IMAGE_RESIZE, "CLAY_UNDO_GROUP_IMAGE_RESIZE", "group-image-resize" },
    { CLAY_UNDO_GROUP_IMAGE_FLIP, "CLAY_UNDO_GROUP_IMAGE_FLIP", "group-image-flip" },
    { CLAY_UNDO_GROUP_IMAGE_ROTATE, "CLAY_UNDO_GROUP_IMAGE_ROTATE", "group-image-rotate" },
    { CLAY_UNDO_GROUP_IMAGE_TRANSFORM, "CLAY_UNDO_GROUP_IMAGE_TRANSFORM", "group-image-transform" },
    { CLAY_UNDO_GROUP_IMAGE_CROP, "CLAY_UNDO_GROUP_IMAGE_CROP", "group-image-crop" },
    { CLAY_UNDO_GROUP_IMAGE_CONVERT, "CLAY_UNDO_GROUP_IMAGE_CONVERT", "group-image-convert" },
    { CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE, "CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE", "group-image-item-remove" },
    { CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER, "CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER", "group-image-item-reorder" },
    { CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE, "CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE", "group-image-layers-merge" },
    { CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE, "CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE", "group-image-paths-merge" },
    { CLAY_UNDO_GROUP_IMAGE_QUICK_MASK, "CLAY_UNDO_GROUP_IMAGE_QUICK_MASK", "group-image-quick-mask" },
    { CLAY_UNDO_GROUP_IMAGE_GRID, "CLAY_UNDO_GROUP_IMAGE_GRID", "group-image-grid" },
    { CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP, "CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP", "group-image-colormap-remap" },
    { CLAY_UNDO_GROUP_GUIDE, "CLAY_UNDO_GROUP_GUIDE", "group-guide" },
    { CLAY_UNDO_GROUP_SAMPLE_POINT, "CLAY_UNDO_GROUP_SAMPLE_POINT", "group-sample-point" },
    { CLAY_UNDO_GROUP_DRAWABLE, "CLAY_UNDO_GROUP_DRAWABLE", "group-drawable" },
    { CLAY_UNDO_GROUP_DRAWABLE_MOD, "CLAY_UNDO_GROUP_DRAWABLE_MOD", "group-drawable-mod" },
    { CLAY_UNDO_GROUP_MASK, "CLAY_UNDO_GROUP_MASK", "group-mask" },
    { CLAY_UNDO_GROUP_ITEM_VISIBILITY, "CLAY_UNDO_GROUP_ITEM_VISIBILITY", "group-item-visibility" },
    { CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS, "CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS", "group-item-lock-contents" },
    { CLAY_UNDO_GROUP_ITEM_LOCK_POSITION, "CLAY_UNDO_GROUP_ITEM_LOCK_POSITION", "group-item-lock-position" },
    { CLAY_UNDO_GROUP_ITEM_LOCK_VISIBILITY, "CLAY_UNDO_GROUP_ITEM_LOCK_VISIBILITY", "group-item-lock-visibility" },
    { CLAY_UNDO_GROUP_ITEM_PROPERTIES, "CLAY_UNDO_GROUP_ITEM_PROPERTIES", "group-item-properties" },
    { CLAY_UNDO_GROUP_ITEM_DISPLACE, "CLAY_UNDO_GROUP_ITEM_DISPLACE", "group-item-displace" },
    { CLAY_UNDO_GROUP_ITEM_SCALE, "CLAY_UNDO_GROUP_ITEM_SCALE", "group-item-scale" },
    { CLAY_UNDO_GROUP_ITEM_RESIZE, "CLAY_UNDO_GROUP_ITEM_RESIZE", "group-item-resize" },
    { CLAY_UNDO_GROUP_LAYER_ADD, "CLAY_UNDO_GROUP_LAYER_ADD", "group-layer-add" },
    { CLAY_UNDO_GROUP_LAYER_ADD_ALPHA, "CLAY_UNDO_GROUP_LAYER_ADD_ALPHA", "group-layer-add-alpha" },
    { CLAY_UNDO_GROUP_LAYER_ADD_MASK, "CLAY_UNDO_GROUP_LAYER_ADD_MASK", "group-layer-add-mask" },
    { CLAY_UNDO_GROUP_LAYER_APPLY_MASK, "CLAY_UNDO_GROUP_LAYER_APPLY_MASK", "group-layer-apply-mask" },
    { CLAY_UNDO_GROUP_LAYER_REMOVE_ALPHA, "CLAY_UNDO_GROUP_LAYER_REMOVE_ALPHA", "group-layer-remove-alpha" },
    { CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA, "CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA", "group-layer-lock-alpha" },
    { CLAY_UNDO_GROUP_LAYER_OPACITY, "CLAY_UNDO_GROUP_LAYER_OPACITY", "group-layer-opacity" },
    { CLAY_UNDO_GROUP_LAYER_MODE, "CLAY_UNDO_GROUP_LAYER_MODE", "group-layer-mode" },
    { CLAY_UNDO_GROUP_CHANNEL_ADD, "CLAY_UNDO_GROUP_CHANNEL_ADD", "group-channel-add" },
    { CLAY_UNDO_GROUP_FS_TO_LAYER, "CLAY_UNDO_GROUP_FS_TO_LAYER", "group-fs-to-layer" },
    { CLAY_UNDO_GROUP_FS_FLOAT, "CLAY_UNDO_GROUP_FS_FLOAT", "group-fs-float" },
    { CLAY_UNDO_GROUP_FS_ANCHOR, "CLAY_UNDO_GROUP_FS_ANCHOR", "group-fs-anchor" },
    { CLAY_UNDO_GROUP_EDIT_PASTE, "CLAY_UNDO_GROUP_EDIT_PASTE", "group-edit-paste" },
    { CLAY_UNDO_GROUP_EDIT_CUT, "CLAY_UNDO_GROUP_EDIT_CUT", "group-edit-cut" },
    { CLAY_UNDO_GROUP_TEXT, "CLAY_UNDO_GROUP_TEXT", "group-text" },
    { CLAY_UNDO_GROUP_VECTOR, "CLAY_UNDO_GROUP_VECTOR", "group-vector" },
    { CLAY_UNDO_GROUP_TRANSFORM, "CLAY_UNDO_GROUP_TRANSFORM", "group-transform" },
    { CLAY_UNDO_GROUP_PAINT, "CLAY_UNDO_GROUP_PAINT", "group-paint" },
    { CLAY_UNDO_GROUP_PARASITE_ATTACH, "CLAY_UNDO_GROUP_PARASITE_ATTACH", "group-parasite-attach" },
    { CLAY_UNDO_GROUP_PARASITE_REMOVE, "CLAY_UNDO_GROUP_PARASITE_REMOVE", "group-parasite-remove" },
    { CLAY_UNDO_GROUP_PATHS_IMPORT, "CLAY_UNDO_GROUP_PATHS_IMPORT", "group-paths-import" },
    { CLAY_UNDO_GROUP_FILTER_VISIBILITY, "CLAY_UNDO_GROUP_FILTER_VISIBILITY", "group-filter-visibility" },
    { CLAY_UNDO_GROUP_MISC, "CLAY_UNDO_GROUP_MISC", "group-misc" },
    { CLAY_UNDO_IMAGE_TYPE, "CLAY_UNDO_IMAGE_TYPE", "image-type" },
    { CLAY_UNDO_IMAGE_PRECISION, "CLAY_UNDO_IMAGE_PRECISION", "image-precision" },
    { CLAY_UNDO_IMAGE_SIZE, "CLAY_UNDO_IMAGE_SIZE", "image-size" },
    { CLAY_UNDO_IMAGE_RESOLUTION, "CLAY_UNDO_IMAGE_RESOLUTION", "image-resolution" },
    { CLAY_UNDO_IMAGE_GRID, "CLAY_UNDO_IMAGE_GRID", "image-grid" },
    { CLAY_UNDO_IMAGE_METADATA, "CLAY_UNDO_IMAGE_METADATA", "image-metadata" },
    { CLAY_UNDO_IMAGE_COLORMAP, "CLAY_UNDO_IMAGE_COLORMAP", "image-colormap" },
    { CLAY_UNDO_IMAGE_HIDDEN_PROFILE, "CLAY_UNDO_IMAGE_HIDDEN_PROFILE", "image-hidden-profile" },
    { CLAY_UNDO_GUIDE, "CLAY_UNDO_GUIDE", "guide" },
    { CLAY_UNDO_SAMPLE_POINT, "CLAY_UNDO_SAMPLE_POINT", "sample-point" },
    { CLAY_UNDO_DRAWABLE, "CLAY_UNDO_DRAWABLE", "drawable" },
    { CLAY_UNDO_DRAWABLE_MOD, "CLAY_UNDO_DRAWABLE_MOD", "drawable-mod" },
    { CLAY_UNDO_DRAWABLE_FORMAT, "CLAY_UNDO_DRAWABLE_FORMAT", "drawable-format" },
    { CLAY_UNDO_MASK, "CLAY_UNDO_MASK", "mask" },
    { CLAY_UNDO_ITEM_REORDER, "CLAY_UNDO_ITEM_REORDER", "item-reorder" },
    { CLAY_UNDO_ITEM_RENAME, "CLAY_UNDO_ITEM_RENAME", "item-rename" },
    { CLAY_UNDO_ITEM_DISPLACE, "CLAY_UNDO_ITEM_DISPLACE", "item-displace" },
    { CLAY_UNDO_ITEM_VISIBILITY, "CLAY_UNDO_ITEM_VISIBILITY", "item-visibility" },
    { CLAY_UNDO_ITEM_COLOR_TAG, "CLAY_UNDO_ITEM_COLOR_TAG", "item-color-tag" },
    { CLAY_UNDO_ITEM_LOCK_CONTENT, "CLAY_UNDO_ITEM_LOCK_CONTENT", "item-lock-content" },
    { CLAY_UNDO_ITEM_LOCK_POSITION, "CLAY_UNDO_ITEM_LOCK_POSITION", "item-lock-position" },
    { CLAY_UNDO_ITEM_LOCK_VISIBILITY, "CLAY_UNDO_ITEM_LOCK_VISIBILITY", "item-lock-visibility" },
    { CLAY_UNDO_LAYER_ADD, "CLAY_UNDO_LAYER_ADD", "layer-add" },
    { CLAY_UNDO_LAYER_REMOVE, "CLAY_UNDO_LAYER_REMOVE", "layer-remove" },
    { CLAY_UNDO_LAYER_MODE, "CLAY_UNDO_LAYER_MODE", "layer-mode" },
    { CLAY_UNDO_LAYER_OPACITY, "CLAY_UNDO_LAYER_OPACITY", "layer-opacity" },
    { CLAY_UNDO_LAYER_LOCK_ALPHA, "CLAY_UNDO_LAYER_LOCK_ALPHA", "layer-lock-alpha" },
    { CLAY_UNDO_LINK_LAYER, "CLAY_UNDO_LINK_LAYER", "link-layer" },
    { CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE, "CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE", "group-layer-suspend-resize" },
    { CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE, "CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE", "group-layer-resume-resize" },
    { CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK, "CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK", "group-layer-suspend-mask" },
    { CLAY_UNDO_GROUP_LAYER_RESUME_MASK, "CLAY_UNDO_GROUP_LAYER_RESUME_MASK", "group-layer-resume-mask" },
    { CLAY_UNDO_GROUP_LAYER_START_TRANSFORM, "CLAY_UNDO_GROUP_LAYER_START_TRANSFORM", "group-layer-start-transform" },
    { CLAY_UNDO_GROUP_LAYER_END_TRANSFORM, "CLAY_UNDO_GROUP_LAYER_END_TRANSFORM", "group-layer-end-transform" },
    { CLAY_UNDO_GROUP_LAYER_CONVERT, "CLAY_UNDO_GROUP_LAYER_CONVERT", "group-layer-convert" },
    { CLAY_UNDO_TEXT_LAYER, "CLAY_UNDO_TEXT_LAYER", "text-layer" },
    { CLAY_UNDO_TEXT_LAYER_CONVERT, "CLAY_UNDO_TEXT_LAYER_CONVERT", "text-layer-convert" },
    { CLAY_UNDO_VECTOR_LAYER, "CLAY_UNDO_VECTOR_LAYER", "vector-layer" },
    { CLAY_UNDO_LAYER_MASK_ADD, "CLAY_UNDO_LAYER_MASK_ADD", "layer-mask-add" },
    { CLAY_UNDO_LAYER_MASK_REMOVE, "CLAY_UNDO_LAYER_MASK_REMOVE", "layer-mask-remove" },
    { CLAY_UNDO_LAYER_MASK_APPLY, "CLAY_UNDO_LAYER_MASK_APPLY", "layer-mask-apply" },
    { CLAY_UNDO_LAYER_MASK_SHOW, "CLAY_UNDO_LAYER_MASK_SHOW", "layer-mask-show" },
    { CLAY_UNDO_CHANNEL_ADD, "CLAY_UNDO_CHANNEL_ADD", "channel-add" },
    { CLAY_UNDO_CHANNEL_REMOVE, "CLAY_UNDO_CHANNEL_REMOVE", "channel-remove" },
    { CLAY_UNDO_CHANNEL_COLOR, "CLAY_UNDO_CHANNEL_COLOR", "channel-color" },
    { CLAY_UNDO_PATH_ADD, "CLAY_UNDO_PATH_ADD", "path-add" },
    { CLAY_UNDO_PATH_REMOVE, "CLAY_UNDO_PATH_REMOVE", "path-remove" },
    { CLAY_UNDO_PATH_MOD, "CLAY_UNDO_PATH_MOD", "path-mod" },
    { CLAY_UNDO_FS_TO_LAYER, "CLAY_UNDO_FS_TO_LAYER", "fs-to-layer" },
    { CLAY_UNDO_TRANSFORM_GRID, "CLAY_UNDO_TRANSFORM_GRID", "transform-grid" },
    { CLAY_UNDO_PAINT, "CLAY_UNDO_PAINT", "paint" },
    { CLAY_UNDO_INK, "CLAY_UNDO_INK", "ink" },
    { CLAY_UNDO_FOREGROUND_SELECT, "CLAY_UNDO_FOREGROUND_SELECT", "foreground-select" },
    { CLAY_UNDO_PARASITE_ATTACH, "CLAY_UNDO_PARASITE_ATTACH", "parasite-attach" },
    { CLAY_UNDO_PARASITE_REMOVE, "CLAY_UNDO_PARASITE_REMOVE", "parasite-remove" },
    { CLAY_UNDO_FILTER_ADD, "CLAY_UNDO_FILTER_ADD", "filter-add" },
    { CLAY_UNDO_FILTER_REMOVE, "CLAY_UNDO_FILTER_REMOVE", "filter-remove" },
    { CLAY_UNDO_FILTER_REORDER, "CLAY_UNDO_FILTER_REORDER", "filter-reorder" },
    { CLAY_UNDO_FILTER_MODIFIED, "CLAY_UNDO_FILTER_MODIFIED", "filter-modified" },
    { CLAY_UNDO_FILTER_VISIBILITY, "CLAY_UNDO_FILTER_VISIBILITY", "filter-visibility" },
    { CLAY_UNDO_RASTERIZABLE, "CLAY_UNDO_RASTERIZABLE", "rasterizable" },
    { CLAY_UNDO_CANT, "CLAY_UNDO_CANT", "cant" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_UNDO_GROUP_NONE, NC_("undo-type", "<<invalid>>"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_SCALE, NC_("undo-type", "Scale image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_RESIZE, NC_("undo-type", "Resize image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_FLIP, NC_("undo-type", "Flip image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_ROTATE, NC_("undo-type", "Rotate image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_TRANSFORM, NC_("undo-type", "Transform image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_CROP, NC_("undo-type", "Crop image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_CONVERT, NC_("undo-type", "Convert image"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE, NC_("undo-type", "Remove item"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE, NC_("undo-type", "Merge layers"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_PATHS_MERGE, NC_("undo-type", "Merge paths"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_QUICK_MASK, NC_("undo-type", "Quick Mask"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { CLAY_UNDO_GROUP_IMAGE_COLORMAP_REMAP, NC_("undo-type", "Colormap remapping"), NULL },
    { CLAY_UNDO_GROUP_GUIDE, NC_("undo-type", "Guide"), NULL },
    { CLAY_UNDO_GROUP_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { CLAY_UNDO_GROUP_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { CLAY_UNDO_GROUP_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { CLAY_UNDO_GROUP_MASK, NC_("undo-type", "Selection mask"), NULL },
    { CLAY_UNDO_GROUP_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS, NC_("undo-type", "Lock/Unlock contents"), NULL },
    { CLAY_UNDO_GROUP_ITEM_LOCK_POSITION, NC_("undo-type", "Lock/Unlock position"), NULL },
    { CLAY_UNDO_GROUP_ITEM_LOCK_VISIBILITY, NC_("undo-type", "Lock/Unlock visibility"), NULL },
    { CLAY_UNDO_GROUP_ITEM_PROPERTIES, NC_("undo-type", "Item properties"), NULL },
    { CLAY_UNDO_GROUP_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { CLAY_UNDO_GROUP_ITEM_SCALE, NC_("undo-type", "Scale item"), NULL },
    { CLAY_UNDO_GROUP_ITEM_RESIZE, NC_("undo-type", "Resize item"), NULL },
    { CLAY_UNDO_GROUP_LAYER_ADD, NC_("undo-type", "Add layer"), NULL },
    { CLAY_UNDO_GROUP_LAYER_ADD_ALPHA, NC_("undo-type", "Add alpha channel"), NULL },
    { CLAY_UNDO_GROUP_LAYER_ADD_MASK, NC_("undo-type", "Add layer masks"), NULL },
    { CLAY_UNDO_GROUP_LAYER_APPLY_MASK, NC_("undo-type", "Apply layer masks"), NULL },
    { CLAY_UNDO_GROUP_LAYER_REMOVE_ALPHA, NC_("undo-type", "Remove alpha channel"), NULL },
    { CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channels"), NULL },
    { CLAY_UNDO_GROUP_LAYER_OPACITY, NC_("undo-type", "Set layers opacity"), NULL },
    { CLAY_UNDO_GROUP_LAYER_MODE, NC_("undo-type", "Set layers mode"), NULL },
    { CLAY_UNDO_GROUP_CHANNEL_ADD, NC_("undo-type", "Add channels"), NULL },
    { CLAY_UNDO_GROUP_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { CLAY_UNDO_GROUP_FS_FLOAT, NC_("undo-type", "Float selection"), NULL },
    { CLAY_UNDO_GROUP_FS_ANCHOR, NC_("undo-type", "Anchor floating selection"), NULL },
    { CLAY_UNDO_GROUP_EDIT_PASTE, NC_("undo-type", "Paste"), NULL },
    { CLAY_UNDO_GROUP_EDIT_CUT, NC_("undo-type", "Cut"), NULL },
    { CLAY_UNDO_GROUP_TEXT, NC_("undo-type", "Text"), NULL },
    { CLAY_UNDO_GROUP_VECTOR, NC_("undo-type", "Vector"), NULL },
    { CLAY_UNDO_GROUP_TRANSFORM, NC_("undo-type", "Transform"), NULL },
    { CLAY_UNDO_GROUP_PAINT, NC_("undo-type", "Paint"), NULL },
    { CLAY_UNDO_GROUP_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { CLAY_UNDO_GROUP_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { CLAY_UNDO_GROUP_PATHS_IMPORT, NC_("undo-type", "Import paths"), NULL },
    { CLAY_UNDO_GROUP_FILTER_VISIBILITY, NC_("undo-type", "Effects visibility"), NULL },
    { CLAY_UNDO_GROUP_MISC, NC_("undo-type", "Plug-In"), NULL },
    { CLAY_UNDO_IMAGE_TYPE, NC_("undo-type", "Image type"), NULL },
    { CLAY_UNDO_IMAGE_PRECISION, NC_("undo-type", "Image precision"), NULL },
    { CLAY_UNDO_IMAGE_SIZE, NC_("undo-type", "Image size"), NULL },
    { CLAY_UNDO_IMAGE_RESOLUTION, NC_("undo-type", "Image resolution change"), NULL },
    { CLAY_UNDO_IMAGE_GRID, NC_("undo-type", "Grid"), NULL },
    { CLAY_UNDO_IMAGE_METADATA, NC_("undo-type", "Change metadata"), NULL },
    { CLAY_UNDO_IMAGE_COLORMAP, NC_("undo-type", "Change indexed palette"), NULL },
    { CLAY_UNDO_IMAGE_HIDDEN_PROFILE, NC_("undo-type", "Hide/Unhide color profile"), NULL },
    { CLAY_UNDO_GUIDE, NC_("undo-type", "Guide"), NULL },
    { CLAY_UNDO_SAMPLE_POINT, NC_("undo-type", "Sample Point"), NULL },
    { CLAY_UNDO_DRAWABLE, NC_("undo-type", "Layer/Channel"), NULL },
    { CLAY_UNDO_DRAWABLE_MOD, NC_("undo-type", "Layer/Channel modification"), NULL },
    { CLAY_UNDO_DRAWABLE_FORMAT, NC_("undo-type", "Layer/Channel format"), NULL },
    { CLAY_UNDO_MASK, NC_("undo-type", "Selection mask"), NULL },
    { CLAY_UNDO_ITEM_REORDER, NC_("undo-type", "Reorder item"), NULL },
    { CLAY_UNDO_ITEM_RENAME, NC_("undo-type", "Rename item"), NULL },
    { CLAY_UNDO_ITEM_DISPLACE, NC_("undo-type", "Move item"), NULL },
    { CLAY_UNDO_ITEM_VISIBILITY, NC_("undo-type", "Item visibility"), NULL },
    { CLAY_UNDO_ITEM_COLOR_TAG, NC_("undo-type", "Item color tag"), NULL },
    { CLAY_UNDO_ITEM_LOCK_CONTENT, NC_("undo-type", "Lock/Unlock content"), NULL },
    { CLAY_UNDO_ITEM_LOCK_POSITION, NC_("undo-type", "Lock/Unlock position"), NULL },
    { CLAY_UNDO_ITEM_LOCK_VISIBILITY, NC_("undo-type", "Lock/Unlock visibility"), NULL },
    { CLAY_UNDO_LAYER_ADD, NC_("undo-type", "New layer"), NULL },
    { CLAY_UNDO_LAYER_REMOVE, NC_("undo-type", "Delete layer"), NULL },
    { CLAY_UNDO_LAYER_MODE, NC_("undo-type", "Set layer mode"), NULL },
    { CLAY_UNDO_LAYER_OPACITY, NC_("undo-type", "Set layer opacity"), NULL },
    { CLAY_UNDO_LAYER_LOCK_ALPHA, NC_("undo-type", "Lock/Unlock alpha channel"), NULL },
    { CLAY_UNDO_LINK_LAYER, NC_("undo-type", "Link layer"), NULL },
    { CLAY_UNDO_GROUP_LAYER_SUSPEND_RESIZE, NC_("undo-type", "Suspend group layer resize"), NULL },
    { CLAY_UNDO_GROUP_LAYER_RESUME_RESIZE, NC_("undo-type", "Resume group layer resize"), NULL },
    { CLAY_UNDO_GROUP_LAYER_SUSPEND_MASK, NC_("undo-type", "Suspend group layer mask"), NULL },
    { CLAY_UNDO_GROUP_LAYER_RESUME_MASK, NC_("undo-type", "Resume group layer mask"), NULL },
    { CLAY_UNDO_GROUP_LAYER_START_TRANSFORM, NC_("undo-type", "Start transforming group layer"), NULL },
    { CLAY_UNDO_GROUP_LAYER_END_TRANSFORM, NC_("undo-type", "End transforming group layer"), NULL },
    { CLAY_UNDO_GROUP_LAYER_CONVERT, NC_("undo-type", "Convert group layer"), NULL },
    { CLAY_UNDO_TEXT_LAYER, NC_("undo-type", "Text layer"), NULL },
    { CLAY_UNDO_TEXT_LAYER_CONVERT, NC_("undo-type", "Convert text layer"), NULL },
    { CLAY_UNDO_VECTOR_LAYER, NC_("undo-type", "Vector layer"), NULL },
    { CLAY_UNDO_LAYER_MASK_ADD, NC_("undo-type", "Add layer masks"), NULL },
    { CLAY_UNDO_LAYER_MASK_REMOVE, NC_("undo-type", "Delete layer masks"), NULL },
    { CLAY_UNDO_LAYER_MASK_APPLY, NC_("undo-type", "Apply layer masks"), NULL },
    { CLAY_UNDO_LAYER_MASK_SHOW, NC_("undo-type", "Show layer masks"), NULL },
    { CLAY_UNDO_CHANNEL_ADD, NC_("undo-type", "New channel"), NULL },
    { CLAY_UNDO_CHANNEL_REMOVE, NC_("undo-type", "Delete channel"), NULL },
    { CLAY_UNDO_CHANNEL_COLOR, NC_("undo-type", "Channel color"), NULL },
    { CLAY_UNDO_PATH_ADD, NC_("undo-type", "New path"), NULL },
    { CLAY_UNDO_PATH_REMOVE, NC_("undo-type", "Delete path"), NULL },
    { CLAY_UNDO_PATH_MOD, NC_("undo-type", "Path modification"), NULL },
    { CLAY_UNDO_FS_TO_LAYER, NC_("undo-type", "Floating selection to layer"), NULL },
    { CLAY_UNDO_TRANSFORM_GRID, NC_("undo-type", "Transform grid"), NULL },
    { CLAY_UNDO_PAINT, NC_("undo-type", "Paint"), NULL },
    { CLAY_UNDO_INK, NC_("undo-type", "Ink"), NULL },
    { CLAY_UNDO_FOREGROUND_SELECT, NC_("undo-type", "Select foreground"), NULL },
    { CLAY_UNDO_PARASITE_ATTACH, NC_("undo-type", "Attach parasite"), NULL },
    { CLAY_UNDO_PARASITE_REMOVE, NC_("undo-type", "Remove parasite"), NULL },
    { CLAY_UNDO_FILTER_ADD, NC_("undo-type", "Add effect"), NULL },
    { CLAY_UNDO_FILTER_REMOVE, NC_("undo-type", "Remove effect"), NULL },
    { CLAY_UNDO_FILTER_REORDER, NC_("undo-type", "Reorder effect"), NULL },
    { CLAY_UNDO_FILTER_MODIFIED, NC_("undo-type", "Effect modification"), NULL },
    { CLAY_UNDO_FILTER_VISIBILITY, NC_("undo-type", "Effect visibility"), NULL },
    { CLAY_UNDO_RASTERIZABLE, NC_("undo-type", "Text, link or vector layer"), NULL },
    { CLAY_UNDO_CANT, NC_("undo-type", "Not undoable"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayUndoType", values);
      clay_type_set_translation_context (type, "undo-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_view_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_VIEW_SIZE_TINY, "CLAY_VIEW_SIZE_TINY", "tiny" },
    { CLAY_VIEW_SIZE_EXTRA_SMALL, "CLAY_VIEW_SIZE_EXTRA_SMALL", "extra-small" },
    { CLAY_VIEW_SIZE_SMALL, "CLAY_VIEW_SIZE_SMALL", "small" },
    { CLAY_VIEW_SIZE_MEDIUM, "CLAY_VIEW_SIZE_MEDIUM", "medium" },
    { CLAY_VIEW_SIZE_LARGE, "CLAY_VIEW_SIZE_LARGE", "large" },
    { CLAY_VIEW_SIZE_EXTRA_LARGE, "CLAY_VIEW_SIZE_EXTRA_LARGE", "extra-large" },
    { CLAY_VIEW_SIZE_HUGE, "CLAY_VIEW_SIZE_HUGE", "huge" },
    { CLAY_VIEW_SIZE_ENORMOUS, "CLAY_VIEW_SIZE_ENORMOUS", "enormous" },
    { CLAY_VIEW_SIZE_GIGANTIC, "CLAY_VIEW_SIZE_GIGANTIC", "gigantic" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_VIEW_SIZE_TINY, NC_("view-size", "Tiny"), NULL },
    { CLAY_VIEW_SIZE_EXTRA_SMALL, NC_("view-size", "Very small"), NULL },
    { CLAY_VIEW_SIZE_SMALL, NC_("view-size", "Small"), NULL },
    { CLAY_VIEW_SIZE_MEDIUM, NC_("view-size", "Medium"), NULL },
    { CLAY_VIEW_SIZE_LARGE, NC_("view-size", "Large"), NULL },
    { CLAY_VIEW_SIZE_EXTRA_LARGE, NC_("view-size", "Very large"), NULL },
    { CLAY_VIEW_SIZE_HUGE, NC_("view-size", "Huge"), NULL },
    { CLAY_VIEW_SIZE_ENORMOUS, NC_("view-size", "Enormous"), NULL },
    { CLAY_VIEW_SIZE_GIGANTIC, NC_("view-size", "Gigantic"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayViewSize", values);
      clay_type_set_translation_context (type, "view-size");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_view_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_VIEW_TYPE_LIST, "CLAY_VIEW_TYPE_LIST", "list" },
    { CLAY_VIEW_TYPE_GRID, "CLAY_VIEW_TYPE_GRID", "grid" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_VIEW_TYPE_LIST, NC_("view-type", "View as list"), NULL },
    { CLAY_VIEW_TYPE_GRID, NC_("view-type", "View as grid"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayViewType", values);
      clay_type_set_translation_context (type, "view-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_select_method_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_SELECT_PLAIN_TEXT, "CLAY_SELECT_PLAIN_TEXT", "plain-text" },
    { CLAY_SELECT_REGEX_PATTERN, "CLAY_SELECT_REGEX_PATTERN", "regex-pattern" },
    { CLAY_SELECT_GLOB_PATTERN, "CLAY_SELECT_GLOB_PATTERN", "glob-pattern" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_SELECT_PLAIN_TEXT, NC_("select-method", "Selection by basic text search"), NULL },
    { CLAY_SELECT_REGEX_PATTERN, NC_("select-method", "Selection by regular expression search"), NULL },
    { CLAY_SELECT_GLOB_PATTERN, NC_("select-method", "Selection by glob pattern search"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySelectMethod", values);
      clay_type_set_translation_context (type, "select-method");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

