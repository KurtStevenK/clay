/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayenumaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "clayaction.h"
#include "clayaction-history.h"
#include "clayenumaction.h"


/**
 * ClayEnumAction:
 *
 * An action storing an enum value.
 *
 * Note that several actions with different values of the same enum type are not
 * exclusive. ClayEnumAction-s are simple on-activate actions and are not
 * stateful. If you want a stateful group of actions whose state is represented
 * by an enum type, you are instead looking for ClayRadioAction.
 */


enum
{
  PROP_0,
  PROP_VALUE,
  PROP_VALUE_VARIABLE
};


static void   clay_enum_action_g_action_iface_init (GActionInterface *iface);

static void   clay_enum_action_set_property        (GObject          *object,
                                                    guint             prop_id,
                                                    const GValue     *value,
                                                    GParamSpec       *pspec);
static void   clay_enum_action_get_property        (GObject          *object,
                                                    guint             prop_id,
                                                    GValue           *value,
                                                    GParamSpec       *pspec);

static void   clay_enum_action_activate            (GAction          *action,
                                                    GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (ClayEnumAction, clay_enum_action, CLAY_TYPE_ACTION_IMPL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, clay_enum_action_g_action_iface_init))

#define parent_class clay_enum_action_parent_class


static void
clay_enum_action_class_init (ClayEnumActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_enum_action_set_property;
  object_class->get_property = clay_enum_action_get_property;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_int ("value",
                                                     NULL, NULL,
                                                     G_MININT, G_MAXINT, 0,
                                                     CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_VALUE_VARIABLE,
                                   g_param_spec_boolean ("value-variable",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_enum_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate = clay_enum_action_activate;
}

static void
clay_enum_action_init (ClayEnumAction *action)
{
}

static void
clay_enum_action_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayEnumAction *action = CLAY_ENUM_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_int (value, action->value);
      break;
    case PROP_VALUE_VARIABLE:
      g_value_set_boolean (value, action->value_variable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_enum_action_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayEnumAction *action = CLAY_ENUM_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      action->value = g_value_get_int (value);
      break;
    case PROP_VALUE_VARIABLE:
      action->value_variable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

ClayEnumAction *
clay_enum_action_new (const gchar *name,
                      const gchar *label,
                      const gchar *short_label,
                      const gchar *tooltip,
                      const gchar *icon_name,
                      const gchar *help_id,
                      gint         value,
                      gboolean     value_variable,
                      ClayContext *context)
{
  ClayEnumAction *action;

  action = g_object_new (CLAY_TYPE_ENUM_ACTION,
                         "name",           name,
                         "label",          label,
                         "short-label",    short_label,
                         "tooltip",        tooltip,
                         "icon-name",      icon_name,
                         "value",          value,
                         "value-variable", value_variable,
                         "context",        context,
                         NULL);

  clay_action_set_help_id (CLAY_ACTION (action), help_id);

  return action;
}

static void
clay_enum_action_activate (GAction  *action,
                           GVariant *parameter)
{
  ClayEnumAction *enum_action = CLAY_ENUM_ACTION (action);

  clay_action_emit_activate (CLAY_ACTION (enum_action),
                             g_variant_new_int32 (enum_action->value));

  clay_action_history_action_activated (CLAY_ACTION (action));
}
