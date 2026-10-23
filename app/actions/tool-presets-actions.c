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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/claycontext.h"
#include "core/claydata.h"
#include "core/claytoolpreset.h"
#include "core/claytooloptions.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "tool-presets-actions.h"
#include "tool-presets-commands.h"

#include "clay-intl.h"


static const ClayActionEntry tool_presets_actions[] =
{
  { "tool-presets-new", CLAY_ICON_DOCUMENT_NEW,
    NC_("tool-presets-action", "_New Tool Preset"), NULL, { NULL },
    NC_("tool-presets-action", "Create a new tool preset"),
    data_new_cmd_callback,
    CLAY_HELP_TOOL_PRESET_NEW },

  { "tool-presets-duplicate", CLAY_ICON_OBJECT_DUPLICATE,
    NC_("tool-presets-action", "D_uplicate Tool Preset"), NULL, { NULL },
    NC_("tool-presets-action", "Duplicate this tool preset"),
    data_duplicate_cmd_callback,
    CLAY_HELP_TOOL_PRESET_DUPLICATE },

  { "tool-presets-copy-location", CLAY_ICON_EDIT_COPY,
    NC_("tool-presets-action", "Copy Tool Preset _Location"), NULL, { NULL },
    NC_("tool-presets-action", "Copy tool preset file location to clipboard"),
    data_copy_location_cmd_callback,
    CLAY_HELP_TOOL_PRESET_COPY_LOCATION },

  { "tool-presets-show-in-file-manager", CLAY_ICON_FILE_MANAGER,
    NC_("tool-presets-action", "Show in _File Manager"), NULL, { NULL },
    NC_("tool-presets-action", "Show tool preset file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    CLAY_HELP_TOOL_PRESET_SHOW_IN_FILE_MANAGER },

  { "tool-presets-save", CLAY_ICON_DOCUMENT_SAVE,
    NC_("tool-presets-action", "_Save Tool Options to Preset"), NULL, { NULL },
    NC_("tool-presets-action", "Save the active tool options to this "
        "tool preset"),
    tool_presets_save_cmd_callback,
    CLAY_HELP_TOOL_PRESET_SAVE },

  { "tool-presets-restore", CLAY_ICON_DOCUMENT_REVERT,
    NC_("tool-presets-action", "_Restore Tool Preset"), NULL, { NULL },
    NC_("tool-presets-action", "Restore this tool preset"),
    tool_presets_restore_cmd_callback,
    CLAY_HELP_TOOL_PRESET_RESTORE },

  { "tool-presets-delete", CLAY_ICON_EDIT_DELETE,
    NC_("tool-presets-action", "_Delete Tool Preset"), NULL, { NULL },
    NC_("tool-presets-action", "Delete this tool preset"),
    data_delete_cmd_callback,
    CLAY_HELP_TOOL_PRESET_DELETE },

  { "tool-presets-refresh", CLAY_ICON_VIEW_REFRESH,
    NC_("tool-presets-action", "_Refresh Tool Presets"), NULL, { NULL },
    NC_("tool-presets-action", "Refresh tool presets"),
    data_refresh_cmd_callback,
    CLAY_HELP_TOOL_PRESET_REFRESH }
};

static const ClayStringActionEntry tool_presets_edit_actions[] =
{
  { "tool-presets-edit", CLAY_ICON_EDIT,
    NC_("tool-presets-action", "_Edit Tool Preset..."), NULL, { NULL },
    NC_("tool-presets-action", "Edit this tool preset"),
    "clay-tool-preset-editor",
    CLAY_HELP_TOOL_PRESET_EDIT }
};


void
tool_presets_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "tool-presets-action",
                                 tool_presets_actions,
                                 G_N_ELEMENTS (tool_presets_actions));

  clay_action_group_add_string_actions (group, "tool-presets-action",
                                        tool_presets_edit_actions,
                                        G_N_ELEMENTS (tool_presets_edit_actions),
                                        data_edit_cmd_callback);
}

void
tool_presets_actions_update (ClayActionGroup *group,
                             gpointer         user_data)
{
  ClayContext    *context     = action_data_get_context (user_data);
  ClayToolPreset *tool_preset = NULL;
  ClayData       *data        = NULL;
  GFile          *file        = NULL;

  if (context)
    {
      tool_preset = clay_context_get_tool_preset (context);

      if (tool_preset)
        {
          data = CLAY_DATA (tool_preset);

          file = clay_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("tool-presets-edit",                 tool_preset);
  SET_SENSITIVE ("tool-presets-duplicate",            tool_preset && clay_data_is_duplicatable (data));
  SET_SENSITIVE ("tool-presets-copy-location",        file);
  SET_SENSITIVE ("tool-presets-show-in-file-manager", file);
  SET_SENSITIVE ("tool-presets-save",                 tool_preset);
  SET_SENSITIVE ("tool-presets-restore",              tool_preset);
  SET_SENSITIVE ("tool-presets-delete",               tool_preset && clay_data_is_deletable (data));

#undef SET_SENSITIVE
}
