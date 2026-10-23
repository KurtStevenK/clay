/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayCoreConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "operations/operations-enums.h"
#include "core/core-enums.h"

#include "config/claygeglconfig.h"


#define CLAY_TYPE_CORE_CONFIG            (clay_core_config_get_type ())
#define CLAY_CORE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CORE_CONFIG, ClayCoreConfig))
#define CLAY_CORE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CORE_CONFIG, ClayCoreConfigClass))
#define CLAY_IS_CORE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CORE_CONFIG))
#define CLAY_IS_CORE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CORE_CONFIG))


typedef struct _ClayCoreConfigClass ClayCoreConfigClass;

struct _ClayCoreConfig
{
  ClayGeglConfig          parent_instance;

  gchar                  *language;
  gchar                  *prev_language;
  ClayInterpolationType   interpolation_type;
  gint                    default_threshold;
  gchar                  *plug_in_path;
  gchar                  *module_path;
  gchar                  *interpreter_path;
  gchar                  *environ_path;
  gchar                  *brush_path;
  gchar                  *brush_path_writable;
  gchar                  *dynamics_path;
  gchar                  *dynamics_path_writable;
  gchar                  *mypaint_brush_path;
  gchar                  *mypaint_brush_path_writable;
  gchar                  *pattern_path;
  gchar                  *pattern_path_writable;
  gchar                  *palette_path;
  gchar                  *palette_path_writable;
  gchar                  *gradient_path;
  gchar                  *gradient_path_writable;
  gchar                  *tool_preset_path;
  gchar                  *tool_preset_path_writable;
  gchar                  *font_path;
  gchar                  *font_path_writable;  /*  unused  */
  gchar                  *default_brush;
  gchar                  *default_dynamics;
  gchar                  *default_mypaint_brush;
  gchar                  *default_pattern;
  gchar                  *default_palette;
  gchar                  *default_tool_preset;
  gchar                  *default_gradient;
  gchar                  *default_font;
  gboolean                global_brush;
  gboolean                global_dynamics;
  gboolean                global_pattern;
  gboolean                global_palette;
  gboolean                global_gradient;
  gboolean                global_font;
  gboolean                global_expand;
  ClayTemplate           *default_image;
  ClayGrid               *default_grid;
  gint                    levels_of_undo;
  guint64                 undo_size;
  ClayViewSize            undo_preview_size;
  gint                    filter_history_size;
  gchar                  *plug_in_rc_path;
  gboolean                layer_previews;
  gboolean                group_layer_previews;
  ClayViewSize            layer_preview_size;
  ClayThumbnailSize       thumbnail_size;
  guint64                 thumbnail_filesize_limit;
  ClayColorConfig        *color_management;
  gboolean                save_document_history;
  GeglColor              *quick_mask_color;
  gboolean                import_promote_float;
  gboolean                import_promote_dither;
  gboolean                import_add_alpha;
  gchar                  *import_raw_plug_in;
  ClayExportFileType      export_file_type;
  gboolean                export_color_profile;
  gboolean                export_comment;
  gboolean                export_thumbnail;
  gboolean                export_metadata_exif;
  gboolean                export_metadata_xmp;
  gboolean                export_metadata_iptc;
  gboolean                export_update_metadata;
  ClayDebugPolicy         debug_policy;
#ifdef G_OS_WIN32
  ClayWin32PointerInputAPI win32_pointer_input_api;
#endif
  ClaySelectMethod        items_select_method;

  gboolean                check_updates;
  gint64                  check_update_timestamp;
  gchar                  *last_known_release;
  gint64                  last_release_timestamp;
  gchar                  *last_release_comment;
  gint                    last_revision;

  gchar                  *config_version;
};

struct _ClayCoreConfigClass
{
  ClayGeglConfigClass  parent_class;
};


GType  clay_core_config_get_type (void);
