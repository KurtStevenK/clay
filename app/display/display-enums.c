
/* Generated data (by clay-mkenums) */

#include "stamp-display-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "display-enums.h"
#include "clay-intl.h"

/* enumerations from "display-enums.h" */
GType
clay_button_press_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BUTTON_PRESS_NORMAL, "CLAY_BUTTON_PRESS_NORMAL", "normal" },
    { CLAY_BUTTON_PRESS_DOUBLE, "CLAY_BUTTON_PRESS_DOUBLE", "double" },
    { CLAY_BUTTON_PRESS_TRIPLE, "CLAY_BUTTON_PRESS_TRIPLE", "triple" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BUTTON_PRESS_NORMAL, "CLAY_BUTTON_PRESS_NORMAL", NULL },
    { CLAY_BUTTON_PRESS_DOUBLE, "CLAY_BUTTON_PRESS_DOUBLE", NULL },
    { CLAY_BUTTON_PRESS_TRIPLE, "CLAY_BUTTON_PRESS_TRIPLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayButtonPressType", values);
      clay_type_set_translation_context (type, "button-press-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_button_release_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BUTTON_RELEASE_NORMAL, "CLAY_BUTTON_RELEASE_NORMAL", "normal" },
    { CLAY_BUTTON_RELEASE_CANCEL, "CLAY_BUTTON_RELEASE_CANCEL", "cancel" },
    { CLAY_BUTTON_RELEASE_CLICK, "CLAY_BUTTON_RELEASE_CLICK", "click" },
    { CLAY_BUTTON_RELEASE_NO_MOTION, "CLAY_BUTTON_RELEASE_NO_MOTION", "no-motion" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BUTTON_RELEASE_NORMAL, "CLAY_BUTTON_RELEASE_NORMAL", NULL },
    { CLAY_BUTTON_RELEASE_CANCEL, "CLAY_BUTTON_RELEASE_CANCEL", NULL },
    { CLAY_BUTTON_RELEASE_CLICK, "CLAY_BUTTON_RELEASE_CLICK", NULL },
    { CLAY_BUTTON_RELEASE_NO_MOTION, "CLAY_BUTTON_RELEASE_NO_MOTION", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayButtonReleaseType", values);
      clay_type_set_translation_context (type, "button-release-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_compass_orientation_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COMPASS_ORIENTATION_AUTO, "CLAY_COMPASS_ORIENTATION_AUTO", "auto" },
    { CLAY_COMPASS_ORIENTATION_HORIZONTAL, "CLAY_COMPASS_ORIENTATION_HORIZONTAL", "horizontal" },
    { CLAY_COMPASS_ORIENTATION_VERTICAL, "CLAY_COMPASS_ORIENTATION_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COMPASS_ORIENTATION_AUTO, NC_("compass-orientation", "Auto"), NULL },
    { CLAY_COMPASS_ORIENTATION_HORIZONTAL, NC_("compass-orientation", "Horizontal"), NULL },
    { CLAY_COMPASS_ORIENTATION_VERTICAL, NC_("compass-orientation", "Vertical"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCompassOrientation", values);
      clay_type_set_translation_context (type, "compass-orientation");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_cursor_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CURSOR_PRECISION_PIXEL_CENTER, "CLAY_CURSOR_PRECISION_PIXEL_CENTER", "pixel-center" },
    { CLAY_CURSOR_PRECISION_PIXEL_BORDER, "CLAY_CURSOR_PRECISION_PIXEL_BORDER", "pixel-border" },
    { CLAY_CURSOR_PRECISION_SUBPIXEL, "CLAY_CURSOR_PRECISION_SUBPIXEL", "subpixel" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CURSOR_PRECISION_PIXEL_CENTER, "CLAY_CURSOR_PRECISION_PIXEL_CENTER", NULL },
    { CLAY_CURSOR_PRECISION_PIXEL_BORDER, "CLAY_CURSOR_PRECISION_PIXEL_BORDER", NULL },
    { CLAY_CURSOR_PRECISION_SUBPIXEL, "CLAY_CURSOR_PRECISION_SUBPIXEL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCursorPrecision", values);
      clay_type_set_translation_context (type, "cursor-precision");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_guides_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_GUIDES_NONE, "CLAY_GUIDES_NONE", "none" },
    { CLAY_GUIDES_CENTER_LINES, "CLAY_GUIDES_CENTER_LINES", "center-lines" },
    { CLAY_GUIDES_THIRDS, "CLAY_GUIDES_THIRDS", "thirds" },
    { CLAY_GUIDES_FIFTHS, "CLAY_GUIDES_FIFTHS", "fifths" },
    { CLAY_GUIDES_GOLDEN, "CLAY_GUIDES_GOLDEN", "golden" },
    { CLAY_GUIDES_DIAGONALS, "CLAY_GUIDES_DIAGONALS", "diagonals" },
    { CLAY_GUIDES_N_LINES, "CLAY_GUIDES_N_LINES", "n-lines" },
    { CLAY_GUIDES_SPACING, "CLAY_GUIDES_SPACING", "spacing" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_GUIDES_NONE, NC_("guides-type", "No guides"), NULL },
    { CLAY_GUIDES_CENTER_LINES, NC_("guides-type", "Center lines"), NULL },
    { CLAY_GUIDES_THIRDS, NC_("guides-type", "Rule of thirds"), NULL },
    { CLAY_GUIDES_FIFTHS, NC_("guides-type", "Rule of fifths"), NULL },
    { CLAY_GUIDES_GOLDEN, NC_("guides-type", "Golden sections"), NULL },
    { CLAY_GUIDES_DIAGONALS, NC_("guides-type", "Diagonal lines"), NULL },
    { CLAY_GUIDES_N_LINES, NC_("guides-type", "Number of lines"), NULL },
    { CLAY_GUIDES_SPACING, NC_("guides-type", "Line spacing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayGuidesType", values);
      clay_type_set_translation_context (type, "guides-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_handle_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HANDLE_SQUARE, "CLAY_HANDLE_SQUARE", "square" },
    { CLAY_HANDLE_DASHED_SQUARE, "CLAY_HANDLE_DASHED_SQUARE", "dashed-square" },
    { CLAY_HANDLE_FILLED_SQUARE, "CLAY_HANDLE_FILLED_SQUARE", "filled-square" },
    { CLAY_HANDLE_CIRCLE, "CLAY_HANDLE_CIRCLE", "circle" },
    { CLAY_HANDLE_DASHED_CIRCLE, "CLAY_HANDLE_DASHED_CIRCLE", "dashed-circle" },
    { CLAY_HANDLE_FILLED_CIRCLE, "CLAY_HANDLE_FILLED_CIRCLE", "filled-circle" },
    { CLAY_HANDLE_DIAMOND, "CLAY_HANDLE_DIAMOND", "diamond" },
    { CLAY_HANDLE_DASHED_DIAMOND, "CLAY_HANDLE_DASHED_DIAMOND", "dashed-diamond" },
    { CLAY_HANDLE_FILLED_DIAMOND, "CLAY_HANDLE_FILLED_DIAMOND", "filled-diamond" },
    { CLAY_HANDLE_CROSS, "CLAY_HANDLE_CROSS", "cross" },
    { CLAY_HANDLE_CROSSHAIR, "CLAY_HANDLE_CROSSHAIR", "crosshair" },
    { CLAY_HANDLE_DROP, "CLAY_HANDLE_DROP", "drop" },
    { CLAY_HANDLE_FILLED_DROP, "CLAY_HANDLE_FILLED_DROP", "filled-drop" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HANDLE_SQUARE, "CLAY_HANDLE_SQUARE", NULL },
    { CLAY_HANDLE_DASHED_SQUARE, "CLAY_HANDLE_DASHED_SQUARE", NULL },
    { CLAY_HANDLE_FILLED_SQUARE, "CLAY_HANDLE_FILLED_SQUARE", NULL },
    { CLAY_HANDLE_CIRCLE, "CLAY_HANDLE_CIRCLE", NULL },
    { CLAY_HANDLE_DASHED_CIRCLE, "CLAY_HANDLE_DASHED_CIRCLE", NULL },
    { CLAY_HANDLE_FILLED_CIRCLE, "CLAY_HANDLE_FILLED_CIRCLE", NULL },
    { CLAY_HANDLE_DIAMOND, "CLAY_HANDLE_DIAMOND", NULL },
    { CLAY_HANDLE_DASHED_DIAMOND, "CLAY_HANDLE_DASHED_DIAMOND", NULL },
    { CLAY_HANDLE_FILLED_DIAMOND, "CLAY_HANDLE_FILLED_DIAMOND", NULL },
    { CLAY_HANDLE_CROSS, "CLAY_HANDLE_CROSS", NULL },
    { CLAY_HANDLE_CROSSHAIR, "CLAY_HANDLE_CROSSHAIR", NULL },
    { CLAY_HANDLE_DROP, "CLAY_HANDLE_DROP", NULL },
    { CLAY_HANDLE_FILLED_DROP, "CLAY_HANDLE_FILLED_DROP", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHandleType", values);
      clay_type_set_translation_context (type, "handle-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_handle_anchor_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HANDLE_ANCHOR_CENTER, "CLAY_HANDLE_ANCHOR_CENTER", "center" },
    { CLAY_HANDLE_ANCHOR_NORTH, "CLAY_HANDLE_ANCHOR_NORTH", "north" },
    { CLAY_HANDLE_ANCHOR_NORTH_WEST, "CLAY_HANDLE_ANCHOR_NORTH_WEST", "north-west" },
    { CLAY_HANDLE_ANCHOR_NORTH_EAST, "CLAY_HANDLE_ANCHOR_NORTH_EAST", "north-east" },
    { CLAY_HANDLE_ANCHOR_SOUTH, "CLAY_HANDLE_ANCHOR_SOUTH", "south" },
    { CLAY_HANDLE_ANCHOR_SOUTH_WEST, "CLAY_HANDLE_ANCHOR_SOUTH_WEST", "south-west" },
    { CLAY_HANDLE_ANCHOR_SOUTH_EAST, "CLAY_HANDLE_ANCHOR_SOUTH_EAST", "south-east" },
    { CLAY_HANDLE_ANCHOR_WEST, "CLAY_HANDLE_ANCHOR_WEST", "west" },
    { CLAY_HANDLE_ANCHOR_EAST, "CLAY_HANDLE_ANCHOR_EAST", "east" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HANDLE_ANCHOR_CENTER, "CLAY_HANDLE_ANCHOR_CENTER", NULL },
    { CLAY_HANDLE_ANCHOR_NORTH, "CLAY_HANDLE_ANCHOR_NORTH", NULL },
    { CLAY_HANDLE_ANCHOR_NORTH_WEST, "CLAY_HANDLE_ANCHOR_NORTH_WEST", NULL },
    { CLAY_HANDLE_ANCHOR_NORTH_EAST, "CLAY_HANDLE_ANCHOR_NORTH_EAST", NULL },
    { CLAY_HANDLE_ANCHOR_SOUTH, "CLAY_HANDLE_ANCHOR_SOUTH", NULL },
    { CLAY_HANDLE_ANCHOR_SOUTH_WEST, "CLAY_HANDLE_ANCHOR_SOUTH_WEST", NULL },
    { CLAY_HANDLE_ANCHOR_SOUTH_EAST, "CLAY_HANDLE_ANCHOR_SOUTH_EAST", NULL },
    { CLAY_HANDLE_ANCHOR_WEST, "CLAY_HANDLE_ANCHOR_WEST", NULL },
    { CLAY_HANDLE_ANCHOR_EAST, "CLAY_HANDLE_ANCHOR_EAST", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHandleAnchor", values);
      clay_type_set_translation_context (type, "handle-anchor");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_limit_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_LIMIT_CIRCLE, "CLAY_LIMIT_CIRCLE", "circle" },
    { CLAY_LIMIT_SQUARE, "CLAY_LIMIT_SQUARE", "square" },
    { CLAY_LIMIT_DIAMOND, "CLAY_LIMIT_DIAMOND", "diamond" },
    { CLAY_LIMIT_HORIZONTAL, "CLAY_LIMIT_HORIZONTAL", "horizontal" },
    { CLAY_LIMIT_VERTICAL, "CLAY_LIMIT_VERTICAL", "vertical" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_LIMIT_CIRCLE, "CLAY_LIMIT_CIRCLE", NULL },
    { CLAY_LIMIT_SQUARE, "CLAY_LIMIT_SQUARE", NULL },
    { CLAY_LIMIT_DIAMOND, "CLAY_LIMIT_DIAMOND", NULL },
    { CLAY_LIMIT_HORIZONTAL, "CLAY_LIMIT_HORIZONTAL", NULL },
    { CLAY_LIMIT_VERTICAL, "CLAY_LIMIT_VERTICAL", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayLimitType", values);
      clay_type_set_translation_context (type, "limit-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_path_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PATH_STYLE_DEFAULT, "CLAY_PATH_STYLE_DEFAULT", "default" },
    { CLAY_PATH_STYLE_PATH, "CLAY_PATH_STYLE_PATH", "path" },
    { CLAY_PATH_STYLE_OUTLINE, "CLAY_PATH_STYLE_OUTLINE", "outline" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PATH_STYLE_DEFAULT, "CLAY_PATH_STYLE_DEFAULT", NULL },
    { CLAY_PATH_STYLE_PATH, "CLAY_PATH_STYLE_PATH", NULL },
    { CLAY_PATH_STYLE_OUTLINE, "CLAY_PATH_STYLE_OUTLINE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPathStyle", values);
      clay_type_set_translation_context (type, "path-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_rectangle_constraint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RECTANGLE_CONSTRAIN_NONE, "CLAY_RECTANGLE_CONSTRAIN_NONE", "none" },
    { CLAY_RECTANGLE_CONSTRAIN_IMAGE, "CLAY_RECTANGLE_CONSTRAIN_IMAGE", "image" },
    { CLAY_RECTANGLE_CONSTRAIN_DRAWABLE, "CLAY_RECTANGLE_CONSTRAIN_DRAWABLE", "drawable" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RECTANGLE_CONSTRAIN_NONE, "CLAY_RECTANGLE_CONSTRAIN_NONE", NULL },
    { CLAY_RECTANGLE_CONSTRAIN_IMAGE, "CLAY_RECTANGLE_CONSTRAIN_IMAGE", NULL },
    { CLAY_RECTANGLE_CONSTRAIN_DRAWABLE, "CLAY_RECTANGLE_CONSTRAIN_DRAWABLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRectangleConstraint", values);
      clay_type_set_translation_context (type, "rectangle-constraint");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_rectangle_fixed_rule_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RECTANGLE_FIXED_ASPECT, "CLAY_RECTANGLE_FIXED_ASPECT", "aspect" },
    { CLAY_RECTANGLE_FIXED_WIDTH, "CLAY_RECTANGLE_FIXED_WIDTH", "width" },
    { CLAY_RECTANGLE_FIXED_HEIGHT, "CLAY_RECTANGLE_FIXED_HEIGHT", "height" },
    { CLAY_RECTANGLE_FIXED_SIZE, "CLAY_RECTANGLE_FIXED_SIZE", "size" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RECTANGLE_FIXED_ASPECT, NC_("rectangle-fixed-rule", "Aspect ratio"), NULL },
    { CLAY_RECTANGLE_FIXED_WIDTH, NC_("rectangle-fixed-rule", "Width"), NULL },
    { CLAY_RECTANGLE_FIXED_HEIGHT, NC_("rectangle-fixed-rule", "Height"), NULL },
    { CLAY_RECTANGLE_FIXED_SIZE, NC_("rectangle-fixed-rule", "Size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRectangleFixedRule", values);
      clay_type_set_translation_context (type, "rectangle-fixed-rule");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_rectangle_precision_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_RECTANGLE_PRECISION_INT, "CLAY_RECTANGLE_PRECISION_INT", "int" },
    { CLAY_RECTANGLE_PRECISION_DOUBLE, "CLAY_RECTANGLE_PRECISION_DOUBLE", "double" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_RECTANGLE_PRECISION_INT, "CLAY_RECTANGLE_PRECISION_INT", NULL },
    { CLAY_RECTANGLE_PRECISION_DOUBLE, "CLAY_RECTANGLE_PRECISION_DOUBLE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayRectanglePrecision", values);
      clay_type_set_translation_context (type, "rectangle-precision");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_3d_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_3D_MODE_CAMERA, "CLAY_TRANSFORM_3D_MODE_CAMERA", "camera" },
    { CLAY_TRANSFORM_3D_MODE_MOVE, "CLAY_TRANSFORM_3D_MODE_MOVE", "move" },
    { CLAY_TRANSFORM_3D_MODE_ROTATE, "CLAY_TRANSFORM_3D_MODE_ROTATE", "rotate" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_3D_MODE_CAMERA, "CLAY_TRANSFORM_3D_MODE_CAMERA", NULL },
    { CLAY_TRANSFORM_3D_MODE_MOVE, "CLAY_TRANSFORM_3D_MODE_MOVE", NULL },
    { CLAY_TRANSFORM_3D_MODE_ROTATE, "CLAY_TRANSFORM_3D_MODE_ROTATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransform3DMode", values);
      clay_type_set_translation_context (type, "transform3-dmode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_function_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TRANSFORM_FUNCTION_NONE, "CLAY_TRANSFORM_FUNCTION_NONE", "none" },
    { CLAY_TRANSFORM_FUNCTION_MOVE, "CLAY_TRANSFORM_FUNCTION_MOVE", "move" },
    { CLAY_TRANSFORM_FUNCTION_SCALE, "CLAY_TRANSFORM_FUNCTION_SCALE", "scale" },
    { CLAY_TRANSFORM_FUNCTION_ROTATE, "CLAY_TRANSFORM_FUNCTION_ROTATE", "rotate" },
    { CLAY_TRANSFORM_FUNCTION_SHEAR, "CLAY_TRANSFORM_FUNCTION_SHEAR", "shear" },
    { CLAY_TRANSFORM_FUNCTION_PERSPECTIVE, "CLAY_TRANSFORM_FUNCTION_PERSPECTIVE", "perspective" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TRANSFORM_FUNCTION_NONE, "CLAY_TRANSFORM_FUNCTION_NONE", NULL },
    { CLAY_TRANSFORM_FUNCTION_MOVE, "CLAY_TRANSFORM_FUNCTION_MOVE", NULL },
    { CLAY_TRANSFORM_FUNCTION_SCALE, "CLAY_TRANSFORM_FUNCTION_SCALE", NULL },
    { CLAY_TRANSFORM_FUNCTION_ROTATE, "CLAY_TRANSFORM_FUNCTION_ROTATE", NULL },
    { CLAY_TRANSFORM_FUNCTION_SHEAR, "CLAY_TRANSFORM_FUNCTION_SHEAR", NULL },
    { CLAY_TRANSFORM_FUNCTION_PERSPECTIVE, "CLAY_TRANSFORM_FUNCTION_PERSPECTIVE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransformFunction", values);
      clay_type_set_translation_context (type, "transform-function");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_transform_handle_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HANDLE_MODE_ADD_TRANSFORM, "CLAY_HANDLE_MODE_ADD_TRANSFORM", "add-transform" },
    { CLAY_HANDLE_MODE_MOVE, "CLAY_HANDLE_MODE_MOVE", "move" },
    { CLAY_HANDLE_MODE_REMOVE, "CLAY_HANDLE_MODE_REMOVE", "remove" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HANDLE_MODE_ADD_TRANSFORM, NC_("transform-handle-mode", "Add / Transform"), NULL },
    { CLAY_HANDLE_MODE_MOVE, NC_("transform-handle-mode", "Move"), NULL },
    { CLAY_HANDLE_MODE_REMOVE, NC_("transform-handle-mode", "Remove"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTransformHandleMode", values);
      clay_type_set_translation_context (type, "transform-handle-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_path_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PATH_MODE_DESIGN, "CLAY_PATH_MODE_DESIGN", "design" },
    { CLAY_PATH_MODE_EDIT, "CLAY_PATH_MODE_EDIT", "edit" },
    { CLAY_PATH_MODE_MOVE, "CLAY_PATH_MODE_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PATH_MODE_DESIGN, NC_("path-mode", "Design"), NULL },
    { CLAY_PATH_MODE_EDIT, NC_("path-mode", "Edit"), NULL },
    { CLAY_PATH_MODE_MOVE, NC_("path-mode", "Move"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPathMode", values);
      clay_type_set_translation_context (type, "path-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_zoom_focus_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ZOOM_FOCUS_BEST_GUESS, "CLAY_ZOOM_FOCUS_BEST_GUESS", "best-guess" },
    { CLAY_ZOOM_FOCUS_POINTER, "CLAY_ZOOM_FOCUS_POINTER", "pointer" },
    { CLAY_ZOOM_FOCUS_IMAGE_CENTER, "CLAY_ZOOM_FOCUS_IMAGE_CENTER", "image-center" },
    { CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", "retain-centering-else-best-guess" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ZOOM_FOCUS_BEST_GUESS, "CLAY_ZOOM_FOCUS_BEST_GUESS", NULL },
    { CLAY_ZOOM_FOCUS_POINTER, "CLAY_ZOOM_FOCUS_POINTER", NULL },
    { CLAY_ZOOM_FOCUS_IMAGE_CENTER, "CLAY_ZOOM_FOCUS_IMAGE_CENTER", NULL },
    { CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS, "CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayZoomFocus", values);
      clay_type_set_translation_context (type, "zoom-focus");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_modifier_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_MODIFIER_ACTION_NONE, "CLAY_MODIFIER_ACTION_NONE", "none" },
    { CLAY_MODIFIER_ACTION_PANNING, "CLAY_MODIFIER_ACTION_PANNING", "panning" },
    { CLAY_MODIFIER_ACTION_ZOOMING, "CLAY_MODIFIER_ACTION_ZOOMING", "zooming" },
    { CLAY_MODIFIER_ACTION_ROTATING, "CLAY_MODIFIER_ACTION_ROTATING", "rotating" },
    { CLAY_MODIFIER_ACTION_STEP_ROTATING, "CLAY_MODIFIER_ACTION_STEP_ROTATING", "step-rotating" },
    { CLAY_MODIFIER_ACTION_LAYER_PICKING, "CLAY_MODIFIER_ACTION_LAYER_PICKING", "layer-picking" },
    { CLAY_MODIFIER_ACTION_MENU, "CLAY_MODIFIER_ACTION_MENU", "menu" },
    { CLAY_MODIFIER_ACTION_ACTION, "CLAY_MODIFIER_ACTION_ACTION", "action" },
    { CLAY_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, "CLAY_MODIFIER_ACTION_BRUSH_PIXEL_SIZE", "brush-pixel-size" },
    { CLAY_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, "CLAY_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE", "brush-radius-pixel-size" },
    { CLAY_MODIFIER_ACTION_TOOL_OPACITY, "CLAY_MODIFIER_ACTION_TOOL_OPACITY", "tool-opacity" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_MODIFIER_ACTION_NONE, NC_("modifier-action", "No action"), NULL },
    { CLAY_MODIFIER_ACTION_PANNING, NC_("modifier-action", "Pan"), NULL },
    { CLAY_MODIFIER_ACTION_ZOOMING, NC_("modifier-action", "Zoom"), NULL },
    { CLAY_MODIFIER_ACTION_ROTATING, NC_("modifier-action", "Rotate View"), NULL },
    { CLAY_MODIFIER_ACTION_STEP_ROTATING, NC_("modifier-action", "Rotate View by 15 degree steps"), NULL },
    { CLAY_MODIFIER_ACTION_LAYER_PICKING, NC_("modifier-action", "Pick a layer"), NULL },
    { CLAY_MODIFIER_ACTION_MENU, NC_("modifier-action", "Display the menu"), NULL },
    { CLAY_MODIFIER_ACTION_ACTION, NC_("modifier-action", "Custom action"), NULL },
    { CLAY_MODIFIER_ACTION_BRUSH_PIXEL_SIZE, NC_("modifier-action", "Change brush size in canvas pixels"), NULL },
    { CLAY_MODIFIER_ACTION_BRUSH_RADIUS_PIXEL_SIZE, NC_("modifier-action", "Change brush radius' size in canvas pixels"), NULL },
    { CLAY_MODIFIER_ACTION_TOOL_OPACITY, NC_("modifier-action", "Change tool opacity"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayModifierAction", values);
      clay_type_set_translation_context (type, "modifier-action");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

