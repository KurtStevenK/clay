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
#include "widgets/clayhelp-ids.h"
#include "widgets/claysamplepointeditor.h"

#include "sample-points-actions.h"
#include "sample-points-commands.h"

#include "clay-intl.h"


static const ClayToggleActionEntry sample_points_toggle_actions[] =
{
  { "sample-points-sample-merged", NULL,
    NC_("sample-points-action", "_Sample Merged"), NULL, { NULL },
    NC_("sample-points-action",
        "Use the composite color of all visible layers"),
    sample_points_sample_merged_cmd_callback,
    TRUE,
    CLAY_HELP_SAMPLE_POINT_SAMPLE_MERGED }
};


void
sample_points_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_toggle_actions (group, "sample-points-action",
                                        sample_points_toggle_actions,
                                        G_N_ELEMENTS (sample_points_toggle_actions));
}

void
sample_points_actions_update (ClayActionGroup *group,
                              gpointer         data)
{
  ClaySamplePointEditor *editor = CLAY_SAMPLE_POINT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        clay_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("sample-points-sample-merged",
              clay_sample_point_editor_get_sample_merged (editor));

#undef SET_ACTIVE
}
