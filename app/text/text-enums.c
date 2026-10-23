
/* Generated data (by clay-mkenums) */

#include "stamp-text-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "text-enums.h"
#include "clay-intl.h"

/* enumerations from "text-enums.h" */
GType
clay_text_box_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_TEXT_BOX_DYNAMIC, "CLAY_TEXT_BOX_DYNAMIC", "dynamic" },
    { CLAY_TEXT_BOX_FIXED, "CLAY_TEXT_BOX_FIXED", "fixed" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_TEXT_BOX_DYNAMIC, NC_("text-box-mode", "Dynamic"), NULL },
    { CLAY_TEXT_BOX_FIXED, NC_("text-box-mode", "Fixed"), NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayTextBoxMode", values);
      clay_type_set_translation_context (type, "text-box-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

