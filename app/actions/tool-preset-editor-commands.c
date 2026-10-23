/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claytoolinfo.h"
#include "core/claytoolpreset.h"

#include "widgets/claydataeditor.h"

#include "tool-preset-editor-commands.h"

#include "clay-intl.h"


/*  public functions  */

void
tool_preset_editor_save_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayDataEditor *editor  = CLAY_DATA_EDITOR (data);
  ClayContext    *context = editor->context;
  ClayToolPreset *preset;
  ClayToolInfo   *tool_info;

  preset    = CLAY_TOOL_PRESET (clay_data_editor_get_data (editor));
  tool_info = clay_context_get_tool (clay_get_user_context (context->clay));

  if (tool_info && preset)
    {
      ClayToolInfo *preset_tool;

      preset_tool =  clay_context_get_tool (CLAY_CONTEXT (preset->tool_options));

      if (tool_info != preset_tool)
        {
          clay_message (context->clay,
                        G_OBJECT (editor), CLAY_MESSAGE_WARNING,
                        _("Can't save '%s' tool options to an "
                          "existing '%s' tool preset."),
                        tool_info->label,
                        preset_tool->label);
          return;
        }

      clay_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);
    }
}

void
tool_preset_editor_restore_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayDataEditor *editor  = CLAY_DATA_EDITOR (data);
  ClayContext    *context = editor->context;
  ClayToolPreset *preset;

  preset = CLAY_TOOL_PRESET (clay_data_editor_get_data (editor));

  if (preset)
    clay_context_tool_preset_changed (clay_get_user_context (context->clay));
}
