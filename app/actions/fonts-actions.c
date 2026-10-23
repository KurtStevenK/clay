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

#include "text/clayfont.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "data-commands.h"
#include "fonts-actions.h"

#include "clay-intl.h"


static const ClayActionEntry fonts_actions[] =
{
  { "fonts-refresh", CLAY_ICON_VIEW_REFRESH,
    NC_("fonts-action", "_Rescan Font List"), NULL, { NULL },
    NC_("fonts-action", "Rescan the installed fonts"),
    data_refresh_cmd_callback,
    CLAY_HELP_FONT_REFRESH }
};


void
fonts_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "fonts-action",
                                 fonts_actions,
                                 G_N_ELEMENTS (fonts_actions));
}

void
fonts_actions_update (ClayActionGroup *group,
                      gpointer         data)
{
#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("fonts-refresh", TRUE);

#undef SET_SENSITIVE
}
