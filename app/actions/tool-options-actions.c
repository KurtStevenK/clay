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

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claylist.h"
#include "core/claytoolinfo.h"
#include "core/claytoolpreset.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "tool-options-actions.h"
#include "tool-options-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void tool_options_actions_update_presets (ClayActionGroup    *group,
                                                 const gchar        *action_prefix,
                                                 ClayActionCallback  callback,
                                                 const gchar        *help_id,
                                                 ClayContainer      *presets,
                                                 gboolean            need_writable,
                                                 gboolean            need_deletable);


/*  global variables  */

static const ClayActionEntry tool_options_actions[] =
{
  { "tool-options-save-new-preset", CLAY_ICON_DOCUMENT_NEW,
    NC_("tool-options-action", "_New Tool Preset..."), NULL, { NULL }, NULL,
    tool_options_save_new_preset_cmd_callback,
    CLAY_HELP_TOOL_OPTIONS_SAVE },

  { "tool-options-reset", CLAY_ICON_RESET,
    NC_("tool-options-action", "R_eset Tool Options"), NULL, { NULL },
    NC_("tool-options-action", "Reset to default values"),
    tool_options_reset_cmd_callback,
    CLAY_HELP_TOOL_OPTIONS_RESET },

  { "tool-options-reset-all", CLAY_ICON_RESET,
    NC_("tool-options-action", "Reset _all Tool Options"), NULL, { NULL },
    NC_("tool-options-action", "Reset all tool options"),
    tool_options_reset_all_cmd_callback,
    CLAY_HELP_TOOL_OPTIONS_RESET }
};


/*  public functions  */

#define SET_VISIBLE(action,condition) \
        clay_action_group_set_action_visible (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_HIDE_EMPTY(action,condition) \
        clay_action_group_set_action_hide_empty (group, action, (condition) != 0)

void
tool_options_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "tool-options-action",
                                 tool_options_actions,
                                 G_N_ELEMENTS (tool_options_actions));
}

void
tool_options_actions_update (ClayActionGroup *group,
                             gpointer         data)
{
  ClayContext  *context   = clay_get_user_context (group->clay);
  ClayToolInfo *tool_info = clay_context_get_tool (context);

  tool_options_actions_update_presets (group, "tool-options-save-preset",
                                       tool_options_save_preset_cmd_callback,
                                       CLAY_HELP_TOOL_OPTIONS_SAVE,
                                       tool_info->presets,
                                       TRUE /* writable */,
                                       FALSE /* deletable */);

  tool_options_actions_update_presets (group, "tool-options-restore-preset",
                                       tool_options_restore_preset_cmd_callback,
                                       CLAY_HELP_TOOL_OPTIONS_RESTORE,
                                       tool_info->presets,
                                       FALSE /* writable */,
                                       FALSE /* deletable */);

  tool_options_actions_update_presets (group, "tool-options-edit-preset",
                                       tool_options_edit_preset_cmd_callback,
                                       CLAY_HELP_TOOL_OPTIONS_EDIT,
                                       tool_info->presets,
                                       FALSE /* writable */,
                                       FALSE /* deletable */);

  tool_options_actions_update_presets (group, "tool-options-delete-preset",
                                       tool_options_delete_preset_cmd_callback,
                                       CLAY_HELP_TOOL_OPTIONS_DELETE,
                                       tool_info->presets,
                                       FALSE /* writable */,
                                       TRUE /* deletable */);
}


/*  private function  */

static void
tool_options_actions_update_presets (ClayActionGroup    *group,
                                     const gchar        *action_prefix,
                                     ClayActionCallback  callback,
                                     const gchar        *help_id,
                                     ClayContainer      *presets,
                                     gboolean            need_writable,
                                     gboolean            need_deletable)
{
  GList *list;
  gint   n_children = 0;
  gint   i;

  for (i = 0; ; i++)
    {
      gchar      *action_name;
      ClayAction *action;

      action_name = g_strdup_printf ("%s-%03d", action_prefix, i);
      action = clay_action_group_get_action (group, action_name);
      g_free (action_name);

      if (! action)
        break;

      clay_action_group_remove_action (group, action);
    }

  if (presets)
    n_children = clay_container_get_n_children (presets);

  if (n_children > 0)
    {
      ClayEnumActionEntry entry = { 0 };

      entry.name           = NULL;
      entry.label          = NULL;
      entry.tooltip        = NULL;
      entry.value          = 0;
      entry.value_variable = FALSE;
      entry.help_id        = help_id;

      for (list = CLAY_LIST (presets)->queue->head, i = 0;
           list;
           list = g_list_next (list), i++)
        {
          ClayObject *preset = list->data;
          GdkPixbuf  *pixbuf = NULL;

          entry.name      = g_strdup_printf ("%s-%03d", action_prefix, i);
          entry.label     = clay_object_get_name (preset);
          entry.icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (preset));
          entry.value     = i;

          g_object_get (preset, "icon-pixbuf", &pixbuf, NULL);

          clay_action_group_add_enum_actions (group, NULL, &entry, 1, callback);

          if (need_writable)
            SET_SENSITIVE (entry.name,
                           clay_data_is_writable (CLAY_DATA (preset)));

          if (need_deletable)
            SET_SENSITIVE (entry.name,
                           clay_data_is_deletable (CLAY_DATA (preset)));

          if (pixbuf)
            clay_action_group_set_action_pixbuf (group, entry.name, pixbuf);

          g_free ((gchar *) entry.name);
        }
    }
}

#undef SET_VISIBLE
#undef SET_SENSITIVE
#undef SET_HIDE_EMPTY
