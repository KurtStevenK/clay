
/* Generated data (by clay-mkenums) */

#include "stamp-clay-gegl-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "core/core-enums.h"
#include "clay-gegl-enums.h"
#include "clay-intl.h"

/* enumerations from "clay-gegl-enums.h" */
GType
clay_cage_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_CAGE_MODE_CAGE_CHANGE, "CLAY_CAGE_MODE_CAGE_CHANGE", "cage-change" },
    { CLAY_CAGE_MODE_DEFORM, "CLAY_CAGE_MODE_DEFORM", "deform" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_CAGE_MODE_CAGE_CHANGE, NC_("cage-mode", "Create or adjust the cage"), NULL },
    { CLAY_CAGE_MODE_DEFORM, NC_("cage-mode", "Deform the cage\nto deform the image"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayCageMode", values);
      clay_type_set_translation_context (type, "cage-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

