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

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "dynamics-actions.h"

#include "clay-intl.h"


static const ClayActionEntry dynamics_actions[] =
{
  { "dynamics-new", CLAY_ICON_DOCUMENT_NEW,
    NC_("dynamics-action", "_New Dynamics"), NULL, { NULL },
    NC_("dynamics-action", "Create a new dynamics"),
    data_new_cmd_callback,
    CLAY_HELP_DYNAMICS_NEW },

  { "dynamics-duplicate", CLAY_ICON_OBJECT_DUPLICATE,
    NC_("dynamics-action", "D_uplicate Dynamics"), NULL, { NULL },
    NC_("dynamics-action", "Duplicate this dynamics"),
    data_duplicate_cmd_callback,
    CLAY_HELP_DYNAMICS_DUPLICATE },

  { "dynamics-copy-location", CLAY_ICON_EDIT_COPY,
    NC_("dynamics-action", "Copy Dynamics _Location"), NULL, { NULL },
    NC_("dynamics-action", "Copy dynamics file location to clipboard"),
    data_copy_location_cmd_callback,
    CLAY_HELP_DYNAMICS_COPY_LOCATION },

  { "dynamics-show-in-file-manager", CLAY_ICON_FILE_MANAGER,
    NC_("dynamics-action", "Show in _File Manager"), NULL, { NULL },
    NC_("dynamics-action", "Show dynamics file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    CLAY_HELP_DYNAMICS_SHOW_IN_FILE_MANAGER },

  { "dynamics-delete", CLAY_ICON_EDIT_DELETE,
    NC_("dynamics-action", "_Delete Dynamics"), NULL, { NULL },
    NC_("dynamics-action", "Delete this dynamics"),
    data_delete_cmd_callback,
    CLAY_HELP_DYNAMICS_DELETE },

  { "dynamics-refresh", CLAY_ICON_VIEW_REFRESH,
    NC_("dynamics-action", "_Refresh Dynamics"), NULL, { NULL },
    NC_("dynamics-action", "Refresh dynamics"),
    data_refresh_cmd_callback,
    CLAY_HELP_DYNAMICS_REFRESH }
};

static const ClayStringActionEntry dynamics_edit_actions[] =
{
  { "dynamics-edit", CLAY_ICON_EDIT,
    NC_("dynamics-action", "_Edit Dynamics..."), NULL, { NULL },
    NC_("dynamics-action", "Edit this dynamics"),
    "clay-dynamics-editor",
    CLAY_HELP_DYNAMICS_EDIT }
};


void
dynamics_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "dynamics-action",
                                 dynamics_actions,
                                 G_N_ELEMENTS (dynamics_actions));

  clay_action_group_add_string_actions (group, "dynamics-action",
                                        dynamics_edit_actions,
                                        G_N_ELEMENTS (dynamics_edit_actions),
                                        data_edit_cmd_callback);
}

void
dynamics_actions_update (ClayActionGroup *group,
                         gpointer         user_data)
{
  ClayContext  *context  = action_data_get_context (user_data);
  ClayDynamics *dynamics = NULL;
  ClayData     *data     = NULL;
  GFile        *file     = NULL;

  if (context)
    {
      dynamics = clay_context_get_dynamics (context);

      if (dynamics)
        {
          data = CLAY_DATA (dynamics);

          file = clay_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("dynamics-edit",                 dynamics);
  SET_SENSITIVE ("dynamics-duplicate",            dynamics && clay_data_is_duplicatable (data));
  SET_SENSITIVE ("dynamics-copy-location",        file);
  SET_SENSITIVE ("dynamics-show-in-file-manager", file);
  SET_SENSITIVE ("dynamics-delete",               dynamics && clay_data_is_deletable (data));

#undef SET_SENSITIVE
}
