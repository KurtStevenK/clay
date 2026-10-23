
/* Generated data (by clay-mkenums) */

#include "stamp-tools-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "core/core-enums.h"
#include "tools-enums.h"
#include "clay-intl.h"

/* enumerations from "tools-enums.h" */
GType
clay_bucket_fill_area_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BUCKET_FILL_SELECTION, "CLAY_BUCKET_FILL_SELECTION", "selection" },
    { CLAY_BUCKET_FILL_SIMILAR_COLORS, "CLAY_BUCKET_FILL_SIMILAR_COLORS", "similar-colors" },
    { CLAY_BUCKET_FILL_LINE_ART, "CLAY_BUCKET_FILL_LINE_ART", "line-art" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BUCKET_FILL_SELECTION, NC_("bucket-fill-area", "Fill whole selection"), NULL },
    { CLAY_BUCKET_FILL_SIMILAR_COLORS, NC_("bucket-fill-area", "Fill similar colors"), NULL },
    { CLAY_BUCKET_FILL_LINE_ART, NC_("bucket-fill-area", "Fill by line art detection"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayBucketFillArea", values);
      clay_type_set_translation_context (type, "bucket-fill-area");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_line_art_source_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LINE_ART_SOURCE_SAMPLE_MERGED, "CLAY_LINE_ART_SOURCE_SAMPLE_MERGED", "sample-merged" },
    { CLAY_LINE_ART_SOURCE_ACTIVE_LAYER, "CLAY_LINE_ART_SOURCE_ACTIVE_LAYER", "active-layer" },
    { CLAY_LINE_ART_SOURCE_LOWER_LAYER, "CLAY_LINE_ART_SOURCE_LOWER_LAYER", "lower-layer" },
    { CLAY_LINE_ART_SOURCE_UPPER_LAYER, "CLAY_LINE_ART_SOURCE_UPPER_LAYER", "upper-layer" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LINE_ART_SOURCE_SAMPLE_MERGED, NC_("line-art-source", "All visible layers"), NULL },
    { CLAY_LINE_ART_SOURCE_ACTIVE_LAYER, NC_("line-art-source", "Selected layer"), NULL },
    { CLAY_LINE_ART_SOURCE_LOWER_LAYER, NC_("line-art-source", "Layer below the selected one"), NULL },
    { CLAY_LINE_ART_SOURCE_UPPER_LAYER, NC_("line-art-source", "Layer above the selected one"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLineArtSource", values);
      clay_type_set_translation_context (type, "line-art-source");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_rect_select_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RECT_SELECT_MODE_FREE, "CLAY_RECT_SELECT_MODE_FREE", "free" },
    { CLAY_RECT_SELECT_MODE_FIXED_SIZE, "CLAY_RECT_SELECT_MODE_FIXED_SIZE", "fixed-size" },
    { CLAY_RECT_SELECT_MODE_FIXED_RATIO, "CLAY_RECT_SELECT_MODE_FIXED_RATIO", "fixed-ratio" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RECT_SELECT_MODE_FREE, NC_("rect-select-mode", "Free select"), NULL },
    { CLAY_RECT_SELECT_MODE_FIXED_SIZE, NC_("rect-select-mode", "Fixed size"), NULL },
    { CLAY_RECT_SELECT_MODE_FIXED_RATIO, NC_("rect-select-mode", "Fixed aspect ratio"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRectSelectMode", values);
      clay_type_set_translation_context (type, "rect-select-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_TYPE_LAYER, "CLAY_TRANSFORM_TYPE_LAYER", "layer" },
    { CLAY_TRANSFORM_TYPE_SELECTION, "CLAY_TRANSFORM_TYPE_SELECTION", "selection" },
    { CLAY_TRANSFORM_TYPE_PATH, "CLAY_TRANSFORM_TYPE_PATH", "path" },
    { CLAY_TRANSFORM_TYPE_IMAGE, "CLAY_TRANSFORM_TYPE_IMAGE", "image" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_TYPE_LAYER, NC_("transform-type", "Layer"), NULL },
    { CLAY_TRANSFORM_TYPE_SELECTION, NC_("transform-type", "Selection"), NULL },
    { CLAY_TRANSFORM_TYPE_PATH, NC_("transform-type", "Path"), NULL },
    { CLAY_TRANSFORM_TYPE_IMAGE, NC_("transform-type", "Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransformType", values);
      clay_type_set_translation_context (type, "transform-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_tool_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TOOL_ACTION_PAUSE, "CLAY_TOOL_ACTION_PAUSE", "pause" },
    { CLAY_TOOL_ACTION_RESUME, "CLAY_TOOL_ACTION_RESUME", "resume" },
    { CLAY_TOOL_ACTION_HALT, "CLAY_TOOL_ACTION_HALT", "halt" },
    { CLAY_TOOL_ACTION_COMMIT, "CLAY_TOOL_ACTION_COMMIT", "commit" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TOOL_ACTION_PAUSE, "CLAY_TOOL_ACTION_PAUSE", NULL },
    { CLAY_TOOL_ACTION_RESUME, "CLAY_TOOL_ACTION_RESUME", NULL },
    { CLAY_TOOL_ACTION_HALT, "CLAY_TOOL_ACTION_HALT", NULL },
    { CLAY_TOOL_ACTION_COMMIT, "CLAY_TOOL_ACTION_COMMIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayToolAction", values);
      clay_type_set_translation_context (type, "tool-action");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_tool_active_modifiers_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TOOL_ACTIVE_MODIFIERS_OFF, "CLAY_TOOL_ACTIVE_MODIFIERS_OFF", "off" },
    { CLAY_TOOL_ACTIVE_MODIFIERS_SAME, "CLAY_TOOL_ACTIVE_MODIFIERS_SAME", "same" },
    { CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE, "CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE", "separate" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TOOL_ACTIVE_MODIFIERS_OFF, "CLAY_TOOL_ACTIVE_MODIFIERS_OFF", NULL },
    { CLAY_TOOL_ACTIVE_MODIFIERS_SAME, "CLAY_TOOL_ACTIVE_MODIFIERS_SAME", NULL },
    { CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE, "CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayToolActiveModifiers", values);
      clay_type_set_translation_context (type, "tool-active-modifiers");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_matting_draw_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MATTING_DRAW_MODE_FOREGROUND, "CLAY_MATTING_DRAW_MODE_FOREGROUND", "foreground" },
    { CLAY_MATTING_DRAW_MODE_BACKGROUND, "CLAY_MATTING_DRAW_MODE_BACKGROUND", "background" },
    { CLAY_MATTING_DRAW_MODE_UNKNOWN, "CLAY_MATTING_DRAW_MODE_UNKNOWN", "unknown" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MATTING_DRAW_MODE_FOREGROUND, NC_("matting-draw-mode", "Draw foreground"), NULL },
    { CLAY_MATTING_DRAW_MODE_BACKGROUND, NC_("matting-draw-mode", "Draw background"), NULL },
    { CLAY_MATTING_DRAW_MODE_UNKNOWN, NC_("matting-draw-mode", "Draw unknown"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMattingDrawMode", values);
      clay_type_set_translation_context (type, "matting-draw-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_matting_preview_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MATTING_PREVIEW_MODE_ON_COLOR, "CLAY_MATTING_PREVIEW_MODE_ON_COLOR", "on-color" },
    { CLAY_MATTING_PREVIEW_MODE_GRAYSCALE, "CLAY_MATTING_PREVIEW_MODE_GRAYSCALE", "grayscale" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MATTING_PREVIEW_MODE_ON_COLOR, NC_("matting-preview-mode", "Color"), NULL },
    { CLAY_MATTING_PREVIEW_MODE_GRAYSCALE, NC_("matting-preview-mode", "Grayscale"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayMattingPreviewMode", values);
      clay_type_set_translation_context (type, "matting-preview-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_3d_lens_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, "CLAY_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH", "focal-length" },
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, "CLAY_TRANSFORM_3D_LENS_MODE_FOV_IMAGE", "fov-image" },
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_ITEM, "CLAY_TRANSFORM_3D_LENS_MODE_FOV_ITEM", "fov-item" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH, NC_("3-dtransform-lens-mode", "Focal length"), NULL },
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "Field of view (relative to image)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to image)".
       Keep it short. */
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_IMAGE, NC_("3-dtransform-lens-mode", "FOV (image)"), NULL },
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "Field of view (relative to item)"), NULL },
    /* Translators: this is an abbreviated version of "Field of view (relative to item)".
       Keep it short. */
    { CLAY_TRANSFORM_3D_LENS_MODE_FOV_ITEM, NC_("3-dtransform-lens-mode", "FOV (item)"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("Clay3DTransformLensMode", values);
      clay_type_set_translation_context (type, "3-dtransform-lens-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_warp_behavior_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_WARP_BEHAVIOR_MOVE, "CLAY_WARP_BEHAVIOR_MOVE", "move" },
    { CLAY_WARP_BEHAVIOR_GROW, "CLAY_WARP_BEHAVIOR_GROW", "grow" },
    { CLAY_WARP_BEHAVIOR_SHRINK, "CLAY_WARP_BEHAVIOR_SHRINK", "shrink" },
    { CLAY_WARP_BEHAVIOR_SWIRL_CW, "CLAY_WARP_BEHAVIOR_SWIRL_CW", "swirl-cw" },
    { CLAY_WARP_BEHAVIOR_SWIRL_CCW, "CLAY_WARP_BEHAVIOR_SWIRL_CCW", "swirl-ccw" },
    { CLAY_WARP_BEHAVIOR_ERASE, "CLAY_WARP_BEHAVIOR_ERASE", "erase" },
    { CLAY_WARP_BEHAVIOR_SMOOTH, "CLAY_WARP_BEHAVIOR_SMOOTH", "smooth" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_WARP_BEHAVIOR_MOVE, NC_("warp-behavior", "Move pixels"), NULL },
    { CLAY_WARP_BEHAVIOR_GROW, NC_("warp-behavior", "Grow area"), NULL },
    { CLAY_WARP_BEHAVIOR_SHRINK, NC_("warp-behavior", "Shrink area"), NULL },
    { CLAY_WARP_BEHAVIOR_SWIRL_CW, NC_("warp-behavior", "Swirl clockwise"), NULL },
    { CLAY_WARP_BEHAVIOR_SWIRL_CCW, NC_("warp-behavior", "Swirl counter-clockwise"), NULL },
    { CLAY_WARP_BEHAVIOR_ERASE, NC_("warp-behavior", "Erase warping"), NULL },
    { CLAY_WARP_BEHAVIOR_SMOOTH, NC_("warp-behavior", "Smooth warping"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayWarpBehavior", values);
      clay_type_set_translation_context (type, "warp-behavior");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

