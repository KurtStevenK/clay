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

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "buffers-actions.h"
#include "buffers-commands.h"

#include "clay-intl.h"


static const ClayActionEntry buffers_actions[] =
{
  { "buffers-paste-as-new-image", CLAY_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as _New Image"), NULL, { NULL },
    NC_("buffers-action", "Paste the selected buffer as a new image"),
    buffers_paste_as_new_image_cmd_callback,
    CLAY_HELP_BUFFER_PASTE_AS_NEW_IMAGE },

  { "buffers-delete", CLAY_ICON_EDIT_DELETE,
    NC_("buffers-action", "_Delete Buffer"), NULL, { NULL },
    NC_("buffers-action", "Delete the selected buffer"),
    buffers_delete_cmd_callback,
    CLAY_HELP_BUFFER_DELETE }
};

static const ClayEnumActionEntry buffers_paste_actions[] =
{
  { "buffers-paste", CLAY_ICON_EDIT_PASTE,
    NC_("buffers-action", "_Paste Buffer"), NULL, { NULL },
    NC_("buffers-action", "Paste the selected buffer"),
    CLAY_PASTE_TYPE_FLOATING, FALSE,
    CLAY_HELP_BUFFER_PASTE },

  { "buffers-paste-in-place", CLAY_ICON_EDIT_PASTE,
    NC_("buffers-action", "Paste Buffer In Pl_ace"), NULL, { NULL },
    NC_("buffers-action", "Paste the selected buffer at its original position"),
    CLAY_PASTE_TYPE_FLOATING_IN_PLACE, FALSE,
    CLAY_HELP_BUFFER_PASTE_IN_PLACE },

  { "buffers-paste-into", CLAY_ICON_EDIT_PASTE_INTO,
    NC_("buffers-action", "Paste Buffer _Into The Selection"), NULL, { NULL },
    NC_("buffers-action", "Paste the selected buffer into the selection"),
    CLAY_PASTE_TYPE_FLOATING_INTO, FALSE,
    CLAY_HELP_BUFFER_PASTE_INTO },

  { "buffers-paste-into-in-place", CLAY_ICON_EDIT_PASTE_INTO,
    NC_("buffers-action", "Paste Buffer Into The Selection In Place"), NULL, { NULL },
    NC_("buffers-action",
        "Paste the selected buffer into the selection at its original position"),
    CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE, FALSE,
    CLAY_HELP_BUFFER_PASTE_INTO_IN_PLACE },

  { "buffers-paste-as-new-layer", CLAY_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as New _Layer"), NULL, { NULL },
    NC_("buffers-action", "Paste the selected buffer as a new layer"),
    CLAY_PASTE_TYPE_NEW_LAYER, FALSE,
    CLAY_HELP_BUFFER_PASTE_AS_NEW_LAYER },

  { "buffers-paste-as-new-layer-in-place", CLAY_ICON_EDIT_PASTE_AS_NEW,
    NC_("buffers-action", "Paste Buffer as New Layer in Place"), NULL, { NULL },
    NC_("buffers-action",
        "Paste the selected buffer as a new layer at its original position"),
    CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE, FALSE,
    CLAY_HELP_BUFFER_PASTE_AS_NEW_LAYER_IN_PLACE },
};


void
buffers_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "buffers-action",
                                 buffers_actions,
                                 G_N_ELEMENTS (buffers_actions));

  clay_action_group_add_enum_actions (group, "buffers-action",
                                      buffers_paste_actions,
                                      G_N_ELEMENTS (buffers_paste_actions),
                                      buffers_paste_cmd_callback);
}

void
buffers_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
  ClayContext *context = action_data_get_context (data);
  ClayBuffer  *buffer  = NULL;

  if (context)
    buffer = clay_context_get_buffer (context);

#define SET_SENSITIVE(action,condition,reason) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, reason)

  SET_SENSITIVE ("buffers-paste",                       buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-in-place",              buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-into",                  buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-into-in-place",         buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-layer",          buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-layer-in-place", buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-paste-as-new-image",          buffer, _("No selected buffer"));
  SET_SENSITIVE ("buffers-delete",                      buffer, _("No selected buffer"));

#undef SET_SENSITIVE
}
