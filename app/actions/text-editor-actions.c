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
#include "widgets/claytexteditor.h"
#include "widgets/clayhelp-ids.h"

#include "text-editor-actions.h"
#include "text-editor-commands.h"

#include "clay-intl.h"


static const ClayActionEntry text_editor_actions[] =
{
  { "text-editor-load", CLAY_ICON_DOCUMENT_OPEN,
    NC_("text-editor-action", "Open"), NULL, { NULL },
    NC_("text-editor-action", "Load text from file"),
    text_editor_load_cmd_callback,
    NULL },

  { "text-editor-clear", CLAY_ICON_EDIT_CLEAR,
    NC_("text-editor-action", "Clear"), NULL, { NULL },
    NC_("text-editor-action", "Clear all text"),
    text_editor_clear_cmd_callback,
    NULL }
};

static const ClayRadioActionEntry text_editor_direction_actions[] =
{
  { "text-editor-direction-ltr", CLAY_ICON_FORMAT_TEXT_DIRECTION_LTR,
    NC_("text-editor-action", "LTR"), NULL, { NULL },
    NC_("text-editor-action", "From left to right"),
    CLAY_TEXT_DIRECTION_LTR,
    NULL },

  { "text-editor-direction-rtl", CLAY_ICON_FORMAT_TEXT_DIRECTION_RTL,
    NC_("text-editor-action", "RTL"), NULL, { NULL },
    NC_("text-editor-action", "From right to left"),
    CLAY_TEXT_DIRECTION_RTL,
    NULL },

  { "text-editor-direction-ttb-rtl", CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL,
    NC_("text-editor-action", "TTB-RTL"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, right to left (mixed orientation)"),
    CLAY_TEXT_DIRECTION_TTB_RTL,
    NULL },

  { "text-editor-direction-ttb-rtl-upright", CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NC_("text-editor-action", "TTB-RTL-UPRIGHT"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, right to left (upright orientation)"),
    CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT,
    NULL },

  { "text-editor-direction-ttb-ltr", CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR,
    NC_("text-editor-action", "TTB-LTR"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, left to right (mixed orientation)"),
    CLAY_TEXT_DIRECTION_TTB_LTR,
    NULL },

  { "text-editor-direction-ttb-ltr-upright", CLAY_ICON_FORMAT_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NC_("text-editor-action", "TTB-LTR-UPRIGHT"), NULL, { NULL },
    NC_("text-editor-action", "Vertical, left to right (upright orientation)"),
    CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT,
    NULL },
};


void
text_editor_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "text-editor-action",
                                 text_editor_actions,
                                 G_N_ELEMENTS (text_editor_actions));

  clay_action_group_add_radio_actions (group, "text-editor-action",
                                       text_editor_direction_actions,
                                       G_N_ELEMENTS (text_editor_direction_actions),
                                       NULL,
                                       CLAY_TEXT_DIRECTION_LTR,
                                       text_editor_direction_cmd_callback);
}

void
text_editor_actions_update (ClayActionGroup *group,
                            gpointer         data)
{
  ClayTextEditor *editor = CLAY_TEXT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        clay_action_group_set_action_active (group, action, (condition) != 0)

  switch (editor->base_dir)
    {
    case CLAY_TEXT_DIRECTION_LTR:
      SET_ACTIVE ("text-editor-direction-ltr", TRUE);
      break;

    case CLAY_TEXT_DIRECTION_RTL:
      SET_ACTIVE ("text-editor-direction-rtl", TRUE);
      break;

    case CLAY_TEXT_DIRECTION_TTB_RTL:
      SET_ACTIVE ("text-editor-direction-ttb-rtl", TRUE);
      break;

    case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
      SET_ACTIVE ("text-editor-direction-ttb-rtl-upright", TRUE);
      break;

    case CLAY_TEXT_DIRECTION_TTB_LTR:
      SET_ACTIVE ("text-editor-direction-ttb-ltr", TRUE);
      break;

    case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
      SET_ACTIVE ("text-editor-direction-ttb-ltr-upright", TRUE);
      break;
    }

#undef SET_ACTIVE
}
