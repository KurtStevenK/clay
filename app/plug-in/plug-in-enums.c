
/* Generated data (by clay-mkenums) */

#include "stamp-plug-in-enums.h"
#include "config.h"
#include <gio/gio.h>
#include "libclaybase/claybase.h"
#include "plug-in-enums.h"
#include "clay-intl.h"

/* enumerations from "plug-in-enums.h" */
GType
clay_plug_in_image_type_get_type (void)
{
  static const GFlagsValue values[] =
  {
    { CLAY_PLUG_IN_RGB_IMAGE, "CLAY_PLUG_IN_RGB_IMAGE", "rgb-image" },
    { CLAY_PLUG_IN_GRAY_IMAGE, "CLAY_PLUG_IN_GRAY_IMAGE", "gray-image" },
    { CLAY_PLUG_IN_INDEXED_IMAGE, "CLAY_PLUG_IN_INDEXED_IMAGE", "indexed-image" },
    { CLAY_PLUG_IN_RGBA_IMAGE, "CLAY_PLUG_IN_RGBA_IMAGE", "rgba-image" },
    { CLAY_PLUG_IN_GRAYA_IMAGE, "CLAY_PLUG_IN_GRAYA_IMAGE", "graya-image" },
    { CLAY_PLUG_IN_INDEXEDA_IMAGE, "CLAY_PLUG_IN_INDEXEDA_IMAGE", "indexeda-image" },
    { 0, NULL, NULL }
  };

  static const ClayFlagsDesc descs[] =
  {
    { CLAY_PLUG_IN_RGB_IMAGE, "CLAY_PLUG_IN_RGB_IMAGE", NULL },
    { CLAY_PLUG_IN_GRAY_IMAGE, "CLAY_PLUG_IN_GRAY_IMAGE", NULL },
    { CLAY_PLUG_IN_INDEXED_IMAGE, "CLAY_PLUG_IN_INDEXED_IMAGE", NULL },
    { CLAY_PLUG_IN_RGBA_IMAGE, "CLAY_PLUG_IN_RGBA_IMAGE", NULL },
    { CLAY_PLUG_IN_GRAYA_IMAGE, "CLAY_PLUG_IN_GRAYA_IMAGE", NULL },
    { CLAY_PLUG_IN_INDEXEDA_IMAGE, "CLAY_PLUG_IN_INDEXEDA_IMAGE", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_flags_register_static ("ClayPlugInImageType", values);
      clay_type_set_translation_context (type, "plug-in-image-type");
      clay_flags_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_plug_in_call_mode_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_PLUG_IN_CALL_NONE, "CLAY_PLUG_IN_CALL_NONE", "none" },
    { CLAY_PLUG_IN_CALL_RUN, "CLAY_PLUG_IN_CALL_RUN", "run" },
    { CLAY_PLUG_IN_CALL_QUERY, "CLAY_PLUG_IN_CALL_QUERY", "query" },
    { CLAY_PLUG_IN_CALL_INIT, "CLAY_PLUG_IN_CALL_INIT", "init" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_PLUG_IN_CALL_NONE, "CLAY_PLUG_IN_CALL_NONE", NULL },
    { CLAY_PLUG_IN_CALL_RUN, "CLAY_PLUG_IN_CALL_RUN", NULL },
    { CLAY_PLUG_IN_CALL_QUERY, "CLAY_PLUG_IN_CALL_QUERY", NULL },
    { CLAY_PLUG_IN_CALL_INIT, "CLAY_PLUG_IN_CALL_INIT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayPlugInCallMode", values);
      clay_type_set_translation_context (type, "plug-in-call-mode");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}

GType
clay_file_procedure_group_get_type (void)
{
  static const GEnumValue values[] =
  {
    { CLAY_FILE_PROCEDURE_GROUP_NONE, "CLAY_FILE_PROCEDURE_GROUP_NONE", "none" },
    { CLAY_FILE_PROCEDURE_GROUP_ANY, "CLAY_FILE_PROCEDURE_GROUP_ANY", "any" },
    { CLAY_FILE_PROCEDURE_GROUP_OPEN, "CLAY_FILE_PROCEDURE_GROUP_OPEN", "open" },
    { CLAY_FILE_PROCEDURE_GROUP_SAVE, "CLAY_FILE_PROCEDURE_GROUP_SAVE", "save" },
    { CLAY_FILE_PROCEDURE_GROUP_EXPORT, "CLAY_FILE_PROCEDURE_GROUP_EXPORT", "export" },
    { 0, NULL, NULL }
  };

  static const ClayEnumDesc descs[] =
  {
    { CLAY_FILE_PROCEDURE_GROUP_NONE, "CLAY_FILE_PROCEDURE_GROUP_NONE", NULL },
    { CLAY_FILE_PROCEDURE_GROUP_ANY, "CLAY_FILE_PROCEDURE_GROUP_ANY", NULL },
    { CLAY_FILE_PROCEDURE_GROUP_OPEN, "CLAY_FILE_PROCEDURE_GROUP_OPEN", NULL },
    { CLAY_FILE_PROCEDURE_GROUP_SAVE, "CLAY_FILE_PROCEDURE_GROUP_SAVE", NULL },
    { CLAY_FILE_PROCEDURE_GROUP_EXPORT, "CLAY_FILE_PROCEDURE_GROUP_EXPORT", NULL },
    { 0, NULL, NULL }
  };

  static GType type = 0;

  if (G_UNLIKELY (! type))
    {
      type = g_enum_register_static ("ClayFileProcedureGroup", values);
      clay_type_set_translation_context (type, "file-procedure-group");
      clay_enum_set_value_descriptions (type, descs);
    }

  return type;
}


/* Generated data ends here */

