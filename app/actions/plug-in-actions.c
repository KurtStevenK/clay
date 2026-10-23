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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"

#include "plug-in/claypluginmanager.h"
#include "plug-in/claypluginmanager-help-domain.h"
#include "plug-in/claypluginprocedure.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayactionimpl.h"
#include "widgets/clayhelp-ids.h"

#include "actions.h"
#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     plug_in_actions_register_procedure   (ClayPDB             *pdb,
                                                      ClayProcedure       *procedure,
                                                      ClayActionGroup     *group);
static void     plug_in_actions_unregister_procedure (ClayPDB             *pdb,
                                                      ClayProcedure       *procedure,
                                                      ClayActionGroup     *group);
static void     plug_in_actions_add_proc             (ClayActionGroup     *group,
                                                      ClayPlugInProcedure *proc);


/*  private variables  */

static const ClayActionEntry plug_in_actions[] =
{
  { "plug-in-reset-all", CLAY_ICON_RESET,
    NC_("plug-in-action", "Reset all _Filters"), NULL, { NULL },
    NC_("plug-in-action", "Reset all plug-ins to their default settings"),
    plug_in_reset_all_cmd_callback,
    CLAY_HELP_FILTER_RESET_ALL }
};


/*  public functions  */

void
plug_in_actions_setup (ClayActionGroup *group)
{
  ClayPlugInManager *manager = group->clay->plug_in_manager;
  GSList            *list;

  clay_action_group_add_actions (group, "plug-in-action",
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions));

  for (list = manager->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      ClayPlugInProcedure *plug_in_proc = list->data;

      if (plug_in_proc->file)
        plug_in_actions_register_procedure (group->clay->pdb,
                                            CLAY_PROCEDURE (plug_in_proc),
                                            group);
    }

  g_signal_connect_object (group->clay->pdb, "register-procedure",
                           G_CALLBACK (plug_in_actions_register_procedure),
                           group, 0);
  g_signal_connect_object (group->clay->pdb, "unregister-procedure",
                           G_CALLBACK (plug_in_actions_unregister_procedure),
                           group, 0);
}

void
plug_in_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
  ClayImage         *image    = action_data_get_image (data);
  ClayPlugInManager *manager  = group->clay->plug_in_manager;
  GSList            *list;

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      ClayPlugInProcedure *proc = list->data;

      if (proc->menu_label && ! proc->file_proc)
        {
          ClayProcedure *procedure = CLAY_PROCEDURE (proc);
          gboolean       sensitive;
          const gchar   *tooltip;
          const gchar   *reason = NULL;

          sensitive = clay_procedure_get_sensitive (procedure,
                                                    CLAY_OBJECT (image),
                                                    &reason);

          clay_action_group_set_action_sensitive (group,
                                                  clay_object_get_name (proc),
                                                  sensitive, reason);

          tooltip = clay_procedure_get_blurb (procedure);
          if (tooltip)
            clay_action_group_set_action_tooltip (group,
                                                  clay_object_get_name (proc),
                                                  tooltip);
        }
    }
}


/*  private functions  */

static void
plug_in_actions_register_procedure (ClayPDB         *pdb,
                                    ClayProcedure   *procedure,
                                    ClayActionGroup *group)
{
  if (CLAY_IS_PLUG_IN_PROCEDURE (procedure))
    {
      ClayPlugInProcedure *plug_in_proc = CLAY_PLUG_IN_PROCEDURE (procedure);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   clay_object_get_name (procedure));
#endif

          plug_in_actions_add_proc (group, plug_in_proc);
        }
    }
}

static void
plug_in_actions_unregister_procedure (ClayPDB         *pdb,
                                      ClayProcedure   *procedure,
                                      ClayActionGroup *group)
{
  if (CLAY_IS_PLUG_IN_PROCEDURE (procedure))
    {
      ClayPlugInProcedure *plug_in_proc = CLAY_PLUG_IN_PROCEDURE (procedure);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
          ClayAction *action;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   clay_object_get_name (procedure));
#endif

          action = clay_action_group_get_action (group,
                                                 clay_object_get_name (procedure));

          if (action)
            clay_action_group_remove_action (group, action);
        }
    }
}

static void
plug_in_actions_add_proc (ClayActionGroup     *group,
                          ClayPlugInProcedure *proc)
{
  ClayProcedureActionEntry  entry;

  entry.name        = clay_object_get_name (proc);
  entry.icon_name   = clay_viewable_get_icon_name (CLAY_VIEWABLE (proc));
  entry.label       = clay_procedure_get_menu_label (CLAY_PROCEDURE (proc));
  entry.accelerator = NULL;
  entry.tooltip     = clay_procedure_get_blurb (CLAY_PROCEDURE (proc));
  entry.procedure   = CLAY_PROCEDURE (proc);
  entry.help_id     = clay_procedure_get_help_id (CLAY_PROCEDURE (proc));

  clay_action_group_add_procedure_actions (group, &entry, 1,
                                           plug_in_run_cmd_callback);

  if (proc->image_types_val)
    {
      ClayContext  *context  = clay_get_user_context (group->clay);
      ClayImage    *image    = clay_context_get_image (context);
      gboolean      sensitive;
      const gchar  *tooltip;
      const gchar  *reason;

      sensitive = clay_procedure_get_sensitive (CLAY_PROCEDURE (proc),
                                                CLAY_OBJECT (image),
                                                &reason);

      clay_action_group_set_action_sensitive (group,
                                              clay_object_get_name (proc),
                                              sensitive, reason);

      tooltip = clay_procedure_get_blurb (CLAY_PROCEDURE (proc));
      if (tooltip)
        clay_action_group_set_action_tooltip (group,
                                              clay_object_get_name (proc),
                                              tooltip);
    }
}
