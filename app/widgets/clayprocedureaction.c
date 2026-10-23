/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprocedureaction.c
 * Copyright (C) 2004-2016 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>
#include <gegl.h>

#include "widgets-types.h"

#include "libclaybase/claybase.h"

#include "pdb/clayprocedure.h"

#include "clayaction.h"
#include "clayaction-history.h"
#include "clayprocedureaction.h"
#include "claywidgets-utils.h"


enum
{
  PROP_0,
  PROP_PROCEDURE
};


static void   clay_procedure_action_g_action_iface_init (GActionInterface *iface);

static void   clay_procedure_action_finalize            (GObject          *object);
static void   clay_procedure_action_set_property        (GObject          *object,
                                                         guint             prop_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void   clay_procedure_action_get_property        (GObject          *object,
                                                         guint             prop_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);

static void   clay_procedure_action_activate            (GAction          *action,
                                                         GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (ClayProcedureAction, clay_procedure_action, CLAY_TYPE_ACTION_IMPL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, clay_procedure_action_g_action_iface_init))

#define parent_class clay_procedure_action_parent_class


static void
clay_procedure_action_class_init (ClayProcedureActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize      = clay_procedure_action_finalize;
  object_class->set_property  = clay_procedure_action_set_property;
  object_class->get_property  = clay_procedure_action_get_property;

  g_object_class_install_property (object_class, PROP_PROCEDURE,
                                   g_param_spec_object ("procedure",
                                                        NULL, NULL,
                                                        CLAY_TYPE_PROCEDURE,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_procedure_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate = clay_procedure_action_activate;
}

static void
clay_procedure_action_init (ClayProcedureAction *action)
{
}

static void
clay_procedure_action_finalize (GObject *object)
{
  ClayProcedureAction *action = CLAY_PROCEDURE_ACTION (object);

  g_clear_object (&action->procedure);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_procedure_action_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  ClayProcedureAction *action = CLAY_PROCEDURE_ACTION (object);

  switch (prop_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, action->procedure);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_procedure_action_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  ClayProcedureAction *action = CLAY_PROCEDURE_ACTION (object);

  switch (prop_id)
    {
    case PROP_PROCEDURE:
      g_set_object (&action->procedure, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_procedure_action_activate (GAction  *action,
                                GVariant *parameter)
{
  ClayProcedureAction *procedure_action = CLAY_PROCEDURE_ACTION (action);

  /* Not all actions have procedures associated with them, for example
   * unused "filters-recent-[N]" actions, so check for NULL before we
   * invoke the action
   */
  if (procedure_action->procedure)
    {
      gsize hack = GPOINTER_TO_SIZE (procedure_action->procedure);

      g_object_add_weak_pointer (G_OBJECT (action), (gpointer) &action);
      clay_action_emit_activate (CLAY_ACTION (action),
                                 g_variant_new_uint64 (hack));

      if (action != NULL)
        {
          g_object_remove_weak_pointer (G_OBJECT (action), (gpointer) &action);
          /* An action might disappear between the moment it's run and the
           * moment we log it. I experienced this when script-fu crashed, though
           * we could imagine that it may be soon possible in normal process too
           * for procedures to get un-registered (e.g. in extensions).
           */
          clay_action_history_action_activated (CLAY_ACTION (action));
        }
    }
}


/*  public functions  */

ClayProcedureAction *
clay_procedure_action_new (const gchar   *name,
                           const gchar   *label,
                           const gchar   *tooltip,
                           const gchar   *icon_name,
                           const gchar   *help_id,
                           ClayProcedure *procedure,
                           ClayContext   *context)
{
  ClayProcedureAction *action;

  action = g_object_new (CLAY_TYPE_PROCEDURE_ACTION,
                         "name",       name,
                         "label",      label,
                         "tooltip",    tooltip,
                         "icon-name",  icon_name,
                         "procedure",  procedure,
                         "context",    context,
                         NULL);

  clay_action_set_help_id (CLAY_ACTION (action), help_id);

  return action;
}
