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

#include "widgets/clayactiongroup.h"
#include "widgets/clayerrorconsole.h"
#include "widgets/clayhelp-ids.h"

#include "error-console-actions.h"
#include "error-console-commands.h"

#include "clay-intl.h"


static const ClayActionEntry error_console_actions[] =
{
  { "error-console-clear", CLAY_ICON_EDIT_CLEAR,
    NC_("error-console-action", "_Clear"), NULL, { NULL },
    NC_("error-console-action", "Clear error console"),
    error_console_clear_cmd_callback,
    CLAY_HELP_ERRORS_CLEAR },

  { "error-console-select-all", NULL,
    NC_("error-console-action", "Select _All"), NULL, { NULL },
    NC_("error-console-action", "Select all error messages"),
    error_console_select_all_cmd_callback,
    CLAY_HELP_ERRORS_SELECT_ALL },

  { "error-console-highlight", NULL,
    NC_("error-console-action", "_Highlight"), NULL, { NULL }, NULL, NULL,
    CLAY_HELP_ERRORS_HIGHLIGHT }
};

static const ClayEnumActionEntry error_console_save_actions[] =
{
  { "error-console-save-all", CLAY_ICON_DOCUMENT_SAVE_AS,
    NC_("error-console-action", "_Save Error Log to File..."), NULL, { NULL },
    NC_("error-console-action", "Write all error messages to a file"),
    FALSE, FALSE,
    CLAY_HELP_ERRORS_SAVE },

  { "error-console-save-selection", CLAY_ICON_DOCUMENT_SAVE_AS,
    NC_("error-console-action", "Save S_election to File..."), NULL, { NULL },
    NC_("error-console-action", "Write the selected error messages to a file"),
    TRUE, FALSE,
    CLAY_HELP_ERRORS_SAVE }
};

static const ClayToggleActionEntry error_console_highlight_actions[] =
{
  { "error-console-highlight-error", NULL,
    NC_("error-console-action", "_Errors"), NULL, { NULL },
    NC_("error-console-action", "Highlight error console on errors"),
    error_console_highlight_error_cmd_callback,
    FALSE,
    CLAY_HELP_ERRORS_HIGHLIGHT },

  { "error-console-highlight-warning", NULL,
    NC_("error-console-action", "_Warnings"), NULL, { NULL },
    NC_("error-console-action", "Highlight error console on warnings"),
    error_console_highlight_warning_cmd_callback,
    FALSE,
    CLAY_HELP_ERRORS_HIGHLIGHT },

  { "error-console-highlight-info", NULL,
    NC_("error-console-action", "_Messages"), NULL, { NULL },
    NC_("error-console-action", "Highlight error console on messages"),
    error_console_highlight_info_cmd_callback,
    FALSE,
    CLAY_HELP_ERRORS_HIGHLIGHT }
};


void
error_console_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "error-console-action",
                                 error_console_actions,
                                 G_N_ELEMENTS (error_console_actions));

  clay_action_group_add_enum_actions (group, "error-console-action",
                                      error_console_save_actions,
                                      G_N_ELEMENTS (error_console_save_actions),
                                      error_console_save_cmd_callback);

  clay_action_group_add_toggle_actions (group, "error-console-action",
                                        error_console_highlight_actions,
                                        G_N_ELEMENTS (error_console_highlight_actions));
}

void
error_console_actions_update (ClayActionGroup *group,
                              gpointer         data)
{
  ClayErrorConsole *console = CLAY_ERROR_CONSOLE (data);
  gboolean          selection;

  selection = gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                                    NULL, NULL);

#define SET_ACTIVE(action,condition)                                           \
        clay_action_group_set_action_active (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("error-console-clear",          TRUE);
  SET_SENSITIVE ("error-console-select-all",     TRUE);
  SET_SENSITIVE ("error-console-save-all",       TRUE);
  SET_SENSITIVE ("error-console-save-selection", selection);
  SET_SENSITIVE ("error-console-highlight",      TRUE);

  SET_SENSITIVE ("error-console-highlight-error", TRUE);
  SET_ACTIVE ("error-console-highlight-error",
              console->highlight[CLAY_MESSAGE_ERROR]);

  SET_SENSITIVE ("error-console-highlight-warning", TRUE);
  SET_ACTIVE ("error-console-highlight-warning",
              console->highlight[CLAY_MESSAGE_WARNING]);

  SET_SENSITIVE ("error-console-highlight-info", TRUE);
  SET_ACTIVE ("error-console-highlight-info",
              console->highlight[CLAY_MESSAGE_INFO]);

#undef SET_ACTIVE
#undef SET_SENSITIVE
}
