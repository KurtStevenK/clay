
/* Generated data (by clay-mkenums) */

#include "stamp-widgets-enums.h"
#include "config.h"
#include <gtk/gtk.h>
#include "libclaybase/claybase.h"
#include "widgets-enums.h"
#include "clay-intl.h"

/* enumerations from "widgets-enums.h" */
GType
clay_active_color_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_ACTIVE_COLOR_FOREGROUND, "CLAY_ACTIVE_COLOR_FOREGROUND", "foreground" },
    { CLAY_ACTIVE_COLOR_BACKGROUND, "CLAY_ACTIVE_COLOR_BACKGROUND", "background" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_ACTIVE_COLOR_FOREGROUND, NC_("active-color", "Foreground"), NULL },
    { CLAY_ACTIVE_COLOR_BACKGROUND, NC_("active-color", "Background"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayActiveColor", values);
      clay_type_set_translation_context (type, "active-color");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_circle_background_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CIRCLE_BACKGROUND_PLAIN, "CLAY_CIRCLE_BACKGROUND_PLAIN", "plain" },
    { CLAY_CIRCLE_BACKGROUND_HSV, "CLAY_CIRCLE_BACKGROUND_HSV", "hsv" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CIRCLE_BACKGROUND_PLAIN, NC_("circle-background", "Plain"), NULL },
    { CLAY_CIRCLE_BACKGROUND_HSV, NC_("circle-background", "HSV"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCircleBackground", values);
      clay_type_set_translation_context (type, "circle-background");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_dialog_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_DIALOG_OK, "CLAY_COLOR_DIALOG_OK", "ok" },
    { CLAY_COLOR_DIALOG_CANCEL, "CLAY_COLOR_DIALOG_CANCEL", "cancel" },
    { CLAY_COLOR_DIALOG_UPDATE, "CLAY_COLOR_DIALOG_UPDATE", "update" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_DIALOG_OK, "CLAY_COLOR_DIALOG_OK", NULL },
    { CLAY_COLOR_DIALOG_CANCEL, "CLAY_COLOR_DIALOG_CANCEL", NULL },
    { CLAY_COLOR_DIALOG_UPDATE, "CLAY_COLOR_DIALOG_UPDATE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorDialogState", values);
      clay_type_set_translation_context (type, "color-dialog-state");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_pick_target_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_PICK_TARGET_NONE, "CLAY_COLOR_PICK_TARGET_NONE", "none" },
    { CLAY_COLOR_PICK_TARGET_FOREGROUND, "CLAY_COLOR_PICK_TARGET_FOREGROUND", "foreground" },
    { CLAY_COLOR_PICK_TARGET_BACKGROUND, "CLAY_COLOR_PICK_TARGET_BACKGROUND", "background" },
    { CLAY_COLOR_PICK_TARGET_PALETTE, "CLAY_COLOR_PICK_TARGET_PALETTE", "palette" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_PICK_TARGET_NONE, NC_("color-pick-target", "Pick only"), NULL },
    { CLAY_COLOR_PICK_TARGET_FOREGROUND, NC_("color-pick-target", "Set foreground color"), NULL },
    { CLAY_COLOR_PICK_TARGET_BACKGROUND, NC_("color-pick-target", "Set background color"), NULL },
    { CLAY_COLOR_PICK_TARGET_PALETTE, NC_("color-pick-target", "Add to palette"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorPickTarget", values);
      clay_type_set_translation_context (type, "color-pick-target");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_pick_state_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_PICK_STATE_START, "CLAY_COLOR_PICK_STATE_START", "start" },
    { CLAY_COLOR_PICK_STATE_UPDATE, "CLAY_COLOR_PICK_STATE_UPDATE", "update" },
    { CLAY_COLOR_PICK_STATE_END, "CLAY_COLOR_PICK_STATE_END", "end" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_PICK_STATE_START, "CLAY_COLOR_PICK_STATE_START", NULL },
    { CLAY_COLOR_PICK_STATE_UPDATE, "CLAY_COLOR_PICK_STATE_UPDATE", NULL },
    { CLAY_COLOR_PICK_STATE_END, "CLAY_COLOR_PICK_STATE_END", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorPickState", values);
      clay_type_set_translation_context (type, "color-pick-state");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_histogram_scale_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_HISTOGRAM_SCALE_LINEAR, "CLAY_HISTOGRAM_SCALE_LINEAR", "linear" },
    { CLAY_HISTOGRAM_SCALE_LOGARITHMIC, "CLAY_HISTOGRAM_SCALE_LOGARITHMIC", "logarithmic" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_HISTOGRAM_SCALE_LINEAR, NC_("histogram-scale", "Linear histogram"), NULL },
    { CLAY_HISTOGRAM_SCALE_LOGARITHMIC, NC_("histogram-scale", "Logarithmic histogram"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayHistogramScale", values);
      clay_type_set_translation_context (type, "histogram-scale");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_tab_style_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TAB_STYLE_ICON, "CLAY_TAB_STYLE_ICON", "icon" },
    { CLAY_TAB_STYLE_PREVIEW, "CLAY_TAB_STYLE_PREVIEW", "preview" },
    { CLAY_TAB_STYLE_NAME, "CLAY_TAB_STYLE_NAME", "name" },
    { CLAY_TAB_STYLE_BLURB, "CLAY_TAB_STYLE_BLURB", "blurb" },
    { CLAY_TAB_STYLE_ICON_NAME, "CLAY_TAB_STYLE_ICON_NAME", "icon-name" },
    { CLAY_TAB_STYLE_ICON_BLURB, "CLAY_TAB_STYLE_ICON_BLURB", "icon-blurb" },
    { CLAY_TAB_STYLE_PREVIEW_NAME, "CLAY_TAB_STYLE_PREVIEW_NAME", "preview-name" },
    { CLAY_TAB_STYLE_PREVIEW_BLURB, "CLAY_TAB_STYLE_PREVIEW_BLURB", "preview-blurb" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TAB_STYLE_ICON, NC_("tab-style", "Icon"), NULL },
    { CLAY_TAB_STYLE_PREVIEW, NC_("tab-style", "Current status"), NULL },
    { CLAY_TAB_STYLE_NAME, NC_("tab-style", "Text"), NULL },
    { CLAY_TAB_STYLE_BLURB, NC_("tab-style", "Description"), NULL },
    { CLAY_TAB_STYLE_ICON_NAME, NC_("tab-style", "Icon & text"), NULL },
    { CLAY_TAB_STYLE_ICON_BLURB, NC_("tab-style", "Icon & desc"), NULL },
    { CLAY_TAB_STYLE_PREVIEW_NAME, NC_("tab-style", "Status & text"), NULL },
    { CLAY_TAB_STYLE_PREVIEW_BLURB, NC_("tab-style", "Status & desc"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTabStyle", values);
      clay_type_set_translation_context (type, "tab-style");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_tag_entry_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TAG_ENTRY_MODE_QUERY, "CLAY_TAG_ENTRY_MODE_QUERY", "query" },
    { CLAY_TAG_ENTRY_MODE_ASSIGN, "CLAY_TAG_ENTRY_MODE_ASSIGN", "assign" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TAG_ENTRY_MODE_QUERY, "CLAY_TAG_ENTRY_MODE_QUERY", NULL },
    { CLAY_TAG_ENTRY_MODE_ASSIGN, "CLAY_TAG_ENTRY_MODE_ASSIGN", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTagEntryMode", values);
      clay_type_set_translation_context (type, "tag-entry-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

