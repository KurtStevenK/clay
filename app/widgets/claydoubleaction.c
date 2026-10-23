/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydoubleaction.c
 * Copyright (C) 2022 Jehan
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
#include "claydoubleaction.h"


enum
{
  PROP_0,
  PROP_VALUE
};

#define GET_PRIVATE(obj) (clay_double_action_get_instance_private ((ClayDoubleAction *) (obj)))

typedef struct _ClayDoubleActionPrivate ClayDoubleActionPrivate;

struct _ClayDoubleActionPrivate
{
  gdouble stateful_value;
};

static void   clay_double_action_g_action_iface_init (GActionInterface *iface);

static void   clay_double_action_set_property        (GObject          *object,
                                                      guint             prop_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void   clay_double_action_get_property        (GObject          *object,
                                                      guint             prop_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void   clay_double_action_activate            (GAction          *action,
                                                      GVariant         *parameter);


G_DEFINE_TYPE_WITH_CODE (ClayDoubleAction, clay_double_action, CLAY_TYPE_ACTION_IMPL,
                         G_ADD_PRIVATE (ClayDoubleAction)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, clay_double_action_g_action_iface_init))

#define parent_class clay_double_action_parent_class


static void
clay_double_action_class_init (ClayDoubleActionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_double_action_set_property;
  object_class->get_property = clay_double_action_get_property;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE, G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_double_action_init (ClayDoubleAction *action)
{
  ClayDoubleActionPrivate *private = GET_PRIVATE (action);

  private->stateful_value = -1;
}

static void
clay_double_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate = clay_double_action_activate;
}

static void
clay_double_action_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayDoubleAction *action = CLAY_DOUBLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, action->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
clay_double_action_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayDoubleAction *action = CLAY_DOUBLE_ACTION (object);

  switch (prop_id)
    {
    case PROP_VALUE:
      action->value = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

ClayDoubleAction *
clay_double_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *short_label,
                        const gchar *tooltip,
                        const gchar *icon_name,
                        const gchar *help_id,
                        gdouble      value,
                        ClayContext *context)
{
  ClayDoubleAction *action;

  action = g_object_new (CLAY_TYPE_DOUBLE_ACTION,
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
clay_double_action_activate (GAction  *action,
                             GVariant *parameter)
{
  ClayDoubleAction        *double_action = CLAY_DOUBLE_ACTION (action);
  ClayDoubleActionPrivate *private       = GET_PRIVATE (double_action);

  /* Handle double parameters in a way friendly to GtkPadController */
  if (g_variant_is_of_type (parameter, G_VARIANT_TYPE_DOUBLE))
    {
      double value      = g_variant_get_double (parameter);
      double delta;
      double new_value;

      if (private->stateful_value < 0)
        {
          /* Initial event */
          delta = 0;
          new_value = value;
        }
      else if (private->stateful_value == value)
        {
          /* Last (stop) event */
          delta = 0;
          new_value = -1;
        }
      else
        {
          /* Intermediate event */
          delta = (private->stateful_value < value) ? 1 : -1;
          new_value = value;
        }

      double_action->value += delta;
      private->stateful_value = new_value;
    }

  clay_action_emit_activate (CLAY_ACTION (action),
                             g_variant_new_double (double_action->value));

  clay_action_history_action_activated (CLAY_ACTION (action));
}
