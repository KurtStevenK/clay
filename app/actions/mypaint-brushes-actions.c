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
#include "core/claymybrush.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "mypaint-brushes-actions.h"

#include "clay-intl.h"


static const ClayActionEntry mypaint_brushes_actions[] =
{
  { "mypaint-brushes-new", CLAY_ICON_DOCUMENT_NEW,
    NC_("mypaint-brushes-action", "_New MyPaint Brush"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Create a new MyPaint brush"),
    data_new_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_NEW },

  { "mypaint-brushes-duplicate", CLAY_ICON_OBJECT_DUPLICATE,
    NC_("mypaint-brushes-action", "D_uplicate MyPaint Brush"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Duplicate this MyPaint brush"),
    data_duplicate_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_DUPLICATE },

  { "mypaint-brushes-copy-location", CLAY_ICON_EDIT_COPY,
    NC_("mypaint-brushes-action", "Copy MyPaint Brush _Location"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Copy MyPaint brush file location to clipboard"),
    data_copy_location_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_COPY_LOCATION },

  { "mypaint-brushes-show-in-file-manager", CLAY_ICON_FILE_MANAGER,
    NC_("mypaint-brushes-action", "Show in _File Manager"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Show MyPaint brush file location in the file manager"),
    data_show_in_file_manager_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_SHOW_IN_FILE_MANAGER },

  { "mypaint-brushes-delete", CLAY_ICON_EDIT_DELETE,
    NC_("mypaint-brushes-action", "_Delete MyPaint Brush"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Delete this MyPaint brush"),
    data_delete_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_DELETE },

  { "mypaint-brushes-refresh", CLAY_ICON_VIEW_REFRESH,
    NC_("mypaint-brushes-action", "_Refresh MyPaint Brushes"), NULL, { NULL },
    NC_("mypaint-brushes-action", "Refresh MyPaint brushes"),
    data_refresh_cmd_callback,
    CLAY_HELP_MYPAINT_BRUSH_REFRESH }
};

static const ClayStringActionEntry mypaint_brushes_edit_actions[] =
{
  { "mypaint-brushes-edit", CLAY_ICON_EDIT,
    NC_("mypaint-brushes-action", "_Edit MyPaint Brush..."), NULL, { NULL },
    NC_("mypaint-brushes-action", "Edit MyPaint brush"),
    "clay-mybrush-editor",
    CLAY_HELP_MYPAINT_BRUSH_EDIT }
};


void
mypaint_brushes_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "mypaint-brushes-action",
                                 mypaint_brushes_actions,
                                 G_N_ELEMENTS (mypaint_brushes_actions));

  clay_action_group_add_string_actions (group, "mypaint-brushes-action",
                                        mypaint_brushes_edit_actions,
                                        G_N_ELEMENTS (mypaint_brushes_edit_actions),
                                        data_edit_cmd_callback);
}

void
mypaint_brushes_actions_update (ClayActionGroup *group,
                                gpointer         user_data)
{
  ClayContext *context = action_data_get_context (user_data);
  ClayMybrush *brush   = NULL;
  ClayData    *data    = NULL;
  GFile       *file    = NULL;

  if (context)
    {
      brush = clay_context_get_mybrush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          brush = NULL;
        }

      if (brush)
        {
          data = CLAY_DATA (brush);

          file = clay_data_get_file (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("mypaint-brushes-edit",                 brush && FALSE);
  SET_SENSITIVE ("mypaint-brushes-duplicate",            brush && clay_data_is_duplicatable (data));
  SET_SENSITIVE ("mypaint-brushes-copy-location",        file);
  SET_SENSITIVE ("mypaint-brushes-show-in-file-manager", file);
  SET_SENSITIVE ("mypaint-brushes-delete",               brush && clay_data_is_deletable (data));

#undef SET_SENSITIVE
}
