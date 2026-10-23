
/* Generated data (by clay-mkenums) */

#include "stamp-config-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "config-enums.h"
#include "clay-intl.h"

/* enumerations from "config-enums.h" */
GType
clay_canvas_padding_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CANVAS_PADDING_MODE_DEFAULT, "CLAY_CANVAS_PADDING_MODE_DEFAULT", "default" },
    { CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK, "CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK", "light-check" },
    { CLAY_CANVAS_PADDING_MODE_DARK_CHECK, "CLAY_CANVAS_PADDING_MODE_DARK_CHECK", "dark-check" },
    { CLAY_CANVAS_PADDING_MODE_CUSTOM, "CLAY_CANVAS_PADDING_MODE_CUSTOM", "custom" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CANVAS_PADDING_MODE_DEFAULT, NC_("canvas-padding-mode", "From theme"), NULL },
    { CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK, NC_("canvas-padding-mode", "Light check color"), NULL },
    { CLAY_CANVAS_PADDING_MODE_DARK_CHECK, NC_("canvas-padding-mode", "Dark check color"), NULL },
    { CLAY_CANVAS_PADDING_MODE_CUSTOM, NC_("canvas-padding-mode", "Custom color"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCanvasPaddingMode", values);
      clay_type_set_translation_context (type, "canvas-padding-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_cursor_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CURSOR_MODE_TOOL_ICON, "CLAY_CURSOR_MODE_TOOL_ICON", "tool-icon" },
    { CLAY_CURSOR_MODE_TOOL_CROSSHAIR, "CLAY_CURSOR_MODE_TOOL_CROSSHAIR", "tool-crosshair" },
    { CLAY_CURSOR_MODE_CROSSHAIR, "CLAY_CURSOR_MODE_CROSSHAIR", "crosshair" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CURSOR_MODE_TOOL_ICON, NC_("cursor-mode", "Tool icon"), NULL },
    { CLAY_CURSOR_MODE_TOOL_CROSSHAIR, NC_("cursor-mode", "Tool icon with crosshair"), NULL },
    { CLAY_CURSOR_MODE_CROSSHAIR, NC_("cursor-mode", "Crosshair only"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCursorMode", values);
      clay_type_set_translation_context (type, "cursor-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_export_file_type_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_EXPORT_FILE_PNG, "CLAY_EXPORT_FILE_PNG", "png" },
    { CLAY_EXPORT_FILE_JPG, "CLAY_EXPORT_FILE_JPG", "jpg" },
    { CLAY_EXPORT_FILE_ORA, "CLAY_EXPORT_FILE_ORA", "ora" },
    { CLAY_EXPORT_FILE_PSD, "CLAY_EXPORT_FILE_PSD", "psd" },
    { CLAY_EXPORT_FILE_PDF, "CLAY_EXPORT_FILE_PDF", "pdf" },
    { CLAY_EXPORT_FILE_TIF, "CLAY_EXPORT_FILE_TIF", "tif" },
    { CLAY_EXPORT_FILE_BMP, "CLAY_EXPORT_FILE_BMP", "bmp" },
    { CLAY_EXPORT_FILE_WEBP, "CLAY_EXPORT_FILE_WEBP", "webp" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_EXPORT_FILE_PNG, NC_("export-file-type", "PNG Image"), NULL },
    { CLAY_EXPORT_FILE_JPG, NC_("export-file-type", "JPEG Image"), NULL },
    { CLAY_EXPORT_FILE_ORA, NC_("export-file-type", "OpenRaster Image"), NULL },
    { CLAY_EXPORT_FILE_PSD, NC_("export-file-type", "Photoshop Image"), NULL },
    { CLAY_EXPORT_FILE_PDF, NC_("export-file-type", "Portable Document Format"), NULL },
    { CLAY_EXPORT_FILE_TIF, NC_("export-file-type", "TIFF Image"), NULL },
    { CLAY_EXPORT_FILE_BMP, NC_("export-file-type", "Windows BMP Image"), NULL },
    { CLAY_EXPORT_FILE_WEBP, NC_("export-file-type", "WebP Image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayExportFileType", values);
      clay_type_set_translation_context (type, "export-file-type");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_handedness_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HANDEDNESS_LEFT, "CLAY_HANDEDNESS_LEFT", "left" },
    { CLAY_HANDEDNESS_RIGHT, "CLAY_HANDEDNESS_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HANDEDNESS_LEFT, NC_("handedness", "Left-handed"), NULL },
    { CLAY_HANDEDNESS_RIGHT, NC_("handedness", "Right-handed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHandedness", values);
      clay_type_set_translation_context (type, "handedness");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_icon_size_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ICON_SIZE_SMALL, "CLAY_ICON_SIZE_SMALL", "small" },
    { CLAY_ICON_SIZE_MEDIUM, "CLAY_ICON_SIZE_MEDIUM", "medium" },
    { CLAY_ICON_SIZE_LARGE, "CLAY_ICON_SIZE_LARGE", "large" },
    { CLAY_ICON_SIZE_HUGE, "CLAY_ICON_SIZE_HUGE", "huge" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ICON_SIZE_SMALL, NC_("icon-size", "Small size"), NULL },
    { CLAY_ICON_SIZE_MEDIUM, NC_("icon-size", "Medium size"), NULL },
    { CLAY_ICON_SIZE_LARGE, NC_("icon-size", "Large size"), NULL },
    { CLAY_ICON_SIZE_HUGE, NC_("icon-size", "Huge size"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayIconSize", values);
      clay_type_set_translation_context (type, "icon-size");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_position_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_POSITION_TOP, "CLAY_POSITION_TOP", "top" },
    { CLAY_POSITION_BOTTOM, "CLAY_POSITION_BOTTOM", "bottom" },
    { CLAY_POSITION_LEFT, "CLAY_POSITION_LEFT", "left" },
    { CLAY_POSITION_RIGHT, "CLAY_POSITION_RIGHT", "right" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_POSITION_TOP, NC_("position", "Top"), NULL },
    { CLAY_POSITION_BOTTOM, NC_("position", "Bottom"), NULL },
    { CLAY_POSITION_LEFT, NC_("position", "Left"), NULL },
    { CLAY_POSITION_RIGHT, NC_("position", "Right"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPosition", values);
      clay_type_set_translation_context (type, "position");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_drag_zoom_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, "PROP_DRAG_ZOOM_MODE_DISTANCE", "distance" },
    { PROP_DRAG_ZOOM_MODE_DURATION, "PROP_DRAG_ZOOM_MODE_DURATION", "duration" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { PROP_DRAG_ZOOM_MODE_DISTANCE, NC_("drag-zoom-mode", "By distance"), NULL },
    { PROP_DRAG_ZOOM_MODE_DURATION, NC_("drag-zoom-mode", "By duration"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayDragZoomMode", values);
      clay_type_set_translation_context (type, "drag-zoom-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_space_bar_action_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_SPACE_BAR_ACTION_NONE, "CLAY_SPACE_BAR_ACTION_NONE", "none" },
    { CLAY_SPACE_BAR_ACTION_PAN, "CLAY_SPACE_BAR_ACTION_PAN", "pan" },
    { CLAY_SPACE_BAR_ACTION_MOVE, "CLAY_SPACE_BAR_ACTION_MOVE", "move" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_SPACE_BAR_ACTION_NONE, NC_("space-bar-action", "No action"), NULL },
    { CLAY_SPACE_BAR_ACTION_PAN, NC_("space-bar-action", "Pan view"), NULL },
    { CLAY_SPACE_BAR_ACTION_MOVE, NC_("space-bar-action", "Switch to Move tool"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySpaceBarAction", values);
      clay_type_set_translation_context (type, "space-bar-action");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_window_hint_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_WINDOW_HINT_NORMAL, "CLAY_WINDOW_HINT_NORMAL", "normal" },
    { CLAY_WINDOW_HINT_UTILITY, "CLAY_WINDOW_HINT_UTILITY", "utility" },
    { CLAY_WINDOW_HINT_KEEP_ABOVE, "CLAY_WINDOW_HINT_KEEP_ABOVE", "keep-above" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_WINDOW_HINT_NORMAL, NC_("window-hint", "Normal window"), NULL },
    { CLAY_WINDOW_HINT_UTILITY, NC_("window-hint", "Utility window"), NULL },
    { CLAY_WINDOW_HINT_KEEP_ABOVE, NC_("window-hint", "Keep above"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayWindowHint", values);
      clay_type_set_translation_context (type, "window-hint");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_zoom_quality_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ZOOM_QUALITY_LOW, "CLAY_ZOOM_QUALITY_LOW", "low" },
    { CLAY_ZOOM_QUALITY_HIGH, "CLAY_ZOOM_QUALITY_HIGH", "high" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ZOOM_QUALITY_LOW, NC_("zoom-quality", "Low"), NULL },
    { CLAY_ZOOM_QUALITY_HIGH, NC_("zoom-quality", "High"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayZoomQuality", values);
      clay_type_set_translation_context (type, "zoom-quality");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_theme_scheme_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_THEME_LIGHT, "CLAY_THEME_LIGHT", "light" },
    { CLAY_THEME_GRAY, "CLAY_THEME_GRAY", "gray" },
    { CLAY_THEME_DARK, "CLAY_THEME_DARK", "dark" },
    { CLAY_THEME_SYSTEM, "CLAY_THEME_SYSTEM", "system" },
    { CLAY_THEME_BROWN, "CLAY_THEME_BROWN", "brown" },
    { CLAY_THEME_PARCHMENT, "CLAY_THEME_PARCHMENT", "parchment" },
    { CLAY_THEME_CAPPUCCINO, "CLAY_THEME_CAPPUCCINO", "cappuccino" },
    { CLAY_THEME_ESPRESSO, "CLAY_THEME_ESPRESSO", "espresso" },
    { CLAY_THEME_MIDNIGHT, "CLAY_THEME_MIDNIGHT", "midnight" },
    { CLAY_THEME_FOREST, "CLAY_THEME_FOREST", "forest" },
    { CLAY_THEME_ROSE, "CLAY_THEME_ROSE", "rose" },
    { CLAY_THEME_SUNSET, "CLAY_THEME_SUNSET", "sunset" },
    { CLAY_THEME_VIOLET, "CLAY_THEME_VIOLET", "violet" },
    { CLAY_THEME_NEON, "CLAY_THEME_NEON", "neon" },
    { CLAY_THEME_MATRIX, "CLAY_THEME_MATRIX", "matrix" },
    { CLAY_THEME_UBUNTU, "CLAY_THEME_UBUNTU", "ubuntu" },
    { CLAY_THEME_DRACULA, "CLAY_THEME_DRACULA", "dracula" },
    { CLAY_THEME_NORD, "CLAY_THEME_NORD", "nord" },
    { CLAY_THEME_SOLARIZED, "CLAY_THEME_SOLARIZED", "solarized" },
    { CLAY_THEME_GRUVBOX, "CLAY_THEME_GRUVBOX", "gruvbox" },
    { CLAY_THEME_OCEAN, "CLAY_THEME_OCEAN", "ocean" },
    { CLAY_THEME_SAKURA, "CLAY_THEME_SAKURA", "sakura" },
    { CLAY_THEME_AMBER, "CLAY_THEME_AMBER", "amber" },
    { CLAY_THEME_SEPIA, "CLAY_THEME_SEPIA", "sepia" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_THEME_LIGHT, NC_("theme-scheme", "Light Colors"), NULL },
    { CLAY_THEME_GRAY, NC_("theme-scheme", "Dim Colors"), NULL },
    { CLAY_THEME_DARK, NC_("theme-scheme", "Dark Colors"), NULL },
    { CLAY_THEME_SYSTEM, NC_("theme-scheme", "System Colors"), NULL },
    { CLAY_THEME_BROWN, NC_("theme-scheme", "Brown"), NULL },
    { CLAY_THEME_PARCHMENT, NC_("theme-scheme", "Parchment"), NULL },
    { CLAY_THEME_CAPPUCCINO, NC_("theme-scheme", "Cappuccino"), NULL },
    { CLAY_THEME_ESPRESSO, NC_("theme-scheme", "Espresso"), NULL },
    { CLAY_THEME_MIDNIGHT, NC_("theme-scheme", "Midnight"), NULL },
    { CLAY_THEME_FOREST, NC_("theme-scheme", "Forest"), NULL },
    { CLAY_THEME_ROSE, NC_("theme-scheme", "Rose"), NULL },
    { CLAY_THEME_SUNSET, NC_("theme-scheme", "Sunset"), NULL },
    { CLAY_THEME_VIOLET, NC_("theme-scheme", "Violet"), NULL },
    { CLAY_THEME_NEON, NC_("theme-scheme", "Neon"), NULL },
    { CLAY_THEME_MATRIX, NC_("theme-scheme", "Matrix"), NULL },
    { CLAY_THEME_UBUNTU, NC_("theme-scheme", "Ubuntu"), NULL },
    { CLAY_THEME_DRACULA, NC_("theme-scheme", "Dracula"), NULL },
    { CLAY_THEME_NORD, NC_("theme-scheme", "Nord"), NULL },
    { CLAY_THEME_SOLARIZED, NC_("theme-scheme", "Solarized"), NULL },
    { CLAY_THEME_GRUVBOX, NC_("theme-scheme", "Gruvbox"), NULL },
    { CLAY_THEME_OCEAN, NC_("theme-scheme", "Ocean"), NULL },
    { CLAY_THEME_SAKURA, NC_("theme-scheme", "Sakura"), NULL },
    { CLAY_THEME_AMBER, NC_("theme-scheme", "Amber"), NULL },
    { CLAY_THEME_SEPIA, NC_("theme-scheme", "Sepia"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayThemeScheme", values);
      clay_type_set_translation_context (type, "theme-scheme");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

