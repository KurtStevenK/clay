/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claystringaction.c
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
#include "claystringaction.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void   clay_string_action_g_action_iface_init (GActionInterface *iface);

static void   clay_string_action_finalize            (GObject          *object);
static void   clay_string_action_set_property        (GObject          *object,
                                                      guint             prop_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void   clay_string_action_get_property        (GObject          *object,
                                                      guint             prop_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void   clay_string_action_activate            (GAction          *action,
                                                      GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (ClayStringAction, clay_string_action, CLAY_TYPE_ACTION_IMPL,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, clay_string_action_g_action_iface_init))

#define parent_class clay_string_action_parent_class


static void
clay_string_action_class_init (ClayStringActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_string_action_finalize;
  object_class->set_property = clay_string_action_set_property;
  object_class->get_property = clay_string_action_get_property;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_string_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate = clay_string_action_activate;
}

static void
clay_string_action_init (ClayStringAction *action)
{
}

static void
clay_string_action_finalize (GObject *object)
{
  ClayStringAction *action = CLAY_STRING_ACTION (object);

  g_clear_pointer (&action->value, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_string_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayStringAction *action = CLAY_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, action->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_string_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayStringAction *action = CLAY_STRING_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_set_str (&action->value, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

ClayStringAction *
clay_string_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *short_label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        const gchar *value,
                        ClayContext *context)
{
  ClayStringAction *action;

  action = g_object_new (CLAY_TYPE_STRING_ACTION,
                         "name",        name,
                         "label",       label,
                         "short-label", short_label,
                         "tooltip",     tooltip,
                         "icon-name",   icon_name,
                         "value",       value,
                         "context",     context,
                         NULL);

  clay_action_set_help_id (CLAY_ACTION (action), help_id);

  return action;
}

static void
clay_string_action_activate (GAction  *action,
                             GVariant *parameter)
{
  ClayStringAction *string_action = CLAY_STRING_ACTION (action);

  clay_action_emit_activate (CLAY_ACTION (action),
                             g_variant_new_string (string_action->value));

  clay_action_history_action_activated (CLAY_ACTION (action));
}
