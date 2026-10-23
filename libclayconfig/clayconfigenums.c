
/* Generated data (by clay-mkenums) */

#include "stamp-clayconfigenums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "clayconfigenums.h"
#include "libclay/libclay-intl.h"

/* enumerations from "clayconfigenums.h" */
GType
clay_color_management_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_MANAGEMENT_OFF, "CLAY_COLOR_MANAGEMENT_OFF", "off" },
    { CLAY_COLOR_MANAGEMENT_DISPLAY, "CLAY_COLOR_MANAGEMENT_DISPLAY", "display" },
    { CLAY_COLOR_MANAGEMENT_SOFTPROOF, "CLAY_COLOR_MANAGEMENT_SOFTPROOF", "softproof" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_MANAGEMENT_OFF, NC_("color-management-mode", "No color management"), NULL },
    { CLAY_COLOR_MANAGEMENT_DISPLAY, NC_("color-management-mode", "Color-managed display"), NULL },
    { CLAY_COLOR_MANAGEMENT_SOFTPROOF, NC_("color-management-mode", "Soft-proofing"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorManagementMode", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-management-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_color_rendering_intent_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL, "CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL", "perceptual" },
    { CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, "CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC", "relative-colorimetric" },
    { CLAY_COLOR_RENDERING_INTENT_SATURATION, "CLAY_COLOR_RENDERING_INTENT_SATURATION", "saturation" },
    { CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, "CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC", "absolute-colorimetric" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_COLOR_RENDERING_INTENT_PERCEPTUAL, NC_("color-rendering-intent", "Perceptual"), NULL },
    { CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC, NC_("color-rendering-intent", "Relative colorimetric"), NULL },
    { CLAY_COLOR_RENDERING_INTENT_SATURATION, NC_("color-rendering-intent", "Saturation"), NULL },
    { CLAY_COLOR_RENDERING_INTENT_ABSOLUTE_COLORIMETRIC, NC_("color-rendering-intent", "Absolute colorimetric"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayColorRenderingIntent", values);
      clay_type_set_translation_domain (type, GETTEXT_PACKAGE "-libclay");
      clay_type_set_translation_context (type, "color-rendering-intent");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

