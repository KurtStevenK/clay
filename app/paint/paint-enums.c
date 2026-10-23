
/* Generated data (by clay-mkenums) */

#include "stamp-paint-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "paint-enums.h"
#include "clay-intl.h"

/* enumerations from "paint-enums.h" */
GType
clay_brush_application_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_BRUSH_HARD, "CLAY_BRUSH_HARD", "hard" },
    { CLAY_BRUSH_SOFT, "CLAY_BRUSH_SOFT", "soft" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_BRUSH_HARD, "CLAY_BRUSH_HARD", NULL },
    { CLAY_BRUSH_SOFT, "CLAY_BRUSH_SOFT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayBrushApplicationMode", values);
      clay_type_set_translation_context (type, "brush-application-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_perspective_clone_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PERSPECTIVE_CLONE_MODE_ADJUST, "CLAY_PERSPECTIVE_CLONE_MODE_ADJUST", "adjust" },
    { CLAY_PERSPECTIVE_CLONE_MODE_PAINT, "CLAY_PERSPECTIVE_CLONE_MODE_PAINT", "paint" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PERSPECTIVE_CLONE_MODE_ADJUST, NC_("perspective-clone-mode", "Modify Perspective"), NULL },
    { CLAY_PERSPECTIVE_CLONE_MODE_PAINT, NC_("perspective-clone-mode", "Perspective Clone"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPerspectiveCloneMode", values);
      clay_type_set_translation_context (type, "perspective-clone-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_source_align_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_SOURCE_ALIGN_NO, "CLAY_SOURCE_ALIGN_NO", "no" },
    { CLAY_SOURCE_ALIGN_YES, "CLAY_SOURCE_ALIGN_YES", "yes" },
    { CLAY_SOURCE_ALIGN_REGISTERED, "CLAY_SOURCE_ALIGN_REGISTERED", "registered" },
    { CLAY_SOURCE_ALIGN_FIXED, "CLAY_SOURCE_ALIGN_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_SOURCE_ALIGN_NO, NC_("source-align-mode", "None"), NULL },
    { CLAY_SOURCE_ALIGN_YES, NC_("source-align-mode", "Aligned"), NULL },
    { CLAY_SOURCE_ALIGN_REGISTERED, NC_("source-align-mode", "Registered"), NULL },
    { CLAY_SOURCE_ALIGN_FIXED, NC_("source-align-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClaySourceAlignMode", values);
      clay_type_set_translation_context (type, "source-align-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

