/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayaction.c
 * Copyright (C) 2004-2019 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "clayaction.h"
#include "clayactionimpl.h"
#include "clayaction-history.h"


enum
{
  PROP_0,
  PROP_ENABLED = CLAY_ACTION_PROP_LAST + 1,
  PROP_PARAMETER_TYPE,
  PROP_STATE_TYPE,
  PROP_STATE
};

enum
{
  CHANGE_STATE,
  LAST_SIGNAL
};

struct _ClayActionImplPrivate
{
  GVariantType *parameter_type;
  GVariant     *state;
  GVariant     *state_hint;
  gboolean      state_set_already;
};

static void   clay_action_g_action_iface_init     (GActionInterface    *iface);

static void   clay_action_impl_finalize           (GObject             *object);
static void   clay_action_impl_set_property       (GObject             *object,
                                                   guint                prop_id,
                                                   const GValue        *value,
                                                   GParamSpec          *pspec);
static void   clay_action_impl_get_property       (GObject             *object,
                                                   guint                prop_id,
                                                   GValue              *value,
                                                   GParamSpec          *pspec);

/* XXX Implementations for our ClayAction are widely inspired by GSimpleAction
 * implementations.
 */
static void   clay_action_impl_activate           (GAction             *action,
                                                   GVariant            *parameter);
static void   clay_action_impl_change_state       (GAction             *action,
                                                   GVariant            *value);
static gboolean clay_action_impl_get_enabled      (GAction             *action);
static const GVariantType *
              clay_action_impl_get_parameter_type (GAction             *action);
static const GVariantType *
              clay_action_impl_get_state_type     (GAction             *action);
static GVariant *
              clay_action_impl_get_state          (GAction             *action);
static GVariant *
              clay_action_impl_get_state_hint     (GAction             *action);

static void   clay_action_impl_set_state          (ClayAction          *clay_action,
                                                   GVariant            *value);


G_DEFINE_TYPE_WITH_CODE (ClayActionImpl, clay_action_impl, CLAY_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayActionImpl)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ACTION, clay_action_g_action_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_ACTION, NULL))

#define parent_class clay_action_impl_parent_class

static guint clay_action_impl_signals[LAST_SIGNAL] = { 0 };

static void
clay_action_impl_class_init (ClayActionImplClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  clay_action_impl_signals[CHANGE_STATE] =
    g_signal_new ("change-state",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_MUST_COLLECT,
                  G_STRUCT_OFFSET (ClayActionImplClass, change_state),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_VARIANT);

  object_class->finalize      = clay_action_impl_finalize;
  object_class->set_property  = clay_action_impl_set_property;
  object_class->get_property  = clay_action_impl_get_property;

  clay_action_install_properties (object_class);

  /**
   * ClayAction:enabled:
   *
   * If @action is currently enabled.
   *
   * If the action is disabled then calls to g_action_activate() and
   * g_action_change_state() have no effect.
   **/
  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "If the action can be activated",
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE));
  /**
   * ClayAction:parameter-type:
   *
   * The type of the parameter that must be given when activating the
   * action.
   **/
  g_object_class_install_property (object_class, PROP_PARAMETER_TYPE,
                                   g_param_spec_boxed ("parameter-type",
                                                       "Parameter Type",
                                                       "The type of GVariant passed to activate()",
                                                       G_TYPE_VARIANT_TYPE,
                                                       CLAY_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY));
  /**
   * ClayAction:state-type:
   *
   * The #GVariantType of the state that the action has, or %NULL if the
   * action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE_TYPE,
                                   g_param_spec_boxed ("state-type",
                                                       "State Type",
                                                       "The type of the state kept by the action",
                                                       G_TYPE_VARIANT_TYPE,
                                                       CLAY_PARAM_READABLE));

  /**
   * ClayAction:state:
   *
   * The state of the action, or %NULL if the action is stateless.
   **/
  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_variant ("state",
                                                         "State",
                                                         "The state the action is in",
                                                         G_VARIANT_TYPE_ANY,
                                                         NULL,
                                                         CLAY_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
clay_action_g_action_iface_init (GActionInterface *iface)
{
  iface->activate           = clay_action_impl_activate;
  iface->change_state       = clay_action_impl_change_state;
  iface->get_enabled        = clay_action_impl_get_enabled;
  iface->get_name           = (const gchar* (*) (GAction*)) clay_action_get_name;
  iface->get_parameter_type = clay_action_impl_get_parameter_type;
  iface->get_state_type     = clay_action_impl_get_state_type;
  iface->get_state          = clay_action_impl_get_state;
  iface->get_state_hint     = clay_action_impl_get_state_hint;
}

static void
clay_action_impl_init (ClayActionImpl *impl)
{
  impl->priv                    = clay_action_impl_get_instance_private (impl);
  impl->priv->state_set_already = FALSE;

  clay_action_init (CLAY_ACTION (impl));
}

static void
clay_action_impl_finalize (GObject *object)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (object);

  if (impl->priv->parameter_type)
    g_variant_type_free (impl->priv->parameter_type);
  if (impl->priv->state)
    g_variant_unref (impl->priv->state);
  if (impl->priv->state_hint)
    g_variant_unref (impl->priv->state_hint);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_action_impl_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (object);

  switch (prop_id)
    {
    case PROP_ENABLED:
      g_value_set_boolean (value, clay_action_impl_get_enabled (G_ACTION (impl)));
      break;
    case PROP_PARAMETER_TYPE:
      g_value_set_boxed (value, clay_action_impl_get_parameter_type (G_ACTION (impl)));
      break;
    case PROP_STATE_TYPE:
      g_value_set_boxed (value, clay_action_impl_get_state_type (G_ACTION (impl)));
      break;
    case PROP_STATE:
      g_value_take_variant (value, clay_action_impl_get_state (G_ACTION (impl)));
      break;

    default:
      clay_action_get_property (object, prop_id, value, pspec);
      break;
    }
}

static void
clay_action_impl_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (object);

  switch (prop_id)
    {
    case PROP_ENABLED:
      clay_action_set_sensitive (CLAY_ACTION (impl),
                                 g_value_get_boolean (value), NULL);
      break;
    case PROP_PARAMETER_TYPE:
      impl->priv->parameter_type = g_value_dup_boxed (value);
      break;
    case PROP_STATE:
      /* The first time we see this (during construct) we should just
       * take the state as it was handed to us.
       *
       * After that, we should make sure we go through the same checks
       * as the C API.
       */
      if (!impl->priv->state_set_already)
        {
          impl->priv->state             = g_value_dup_variant (value);
          impl->priv->state_set_already = TRUE;
        }
      else
        {
          clay_action_impl_set_state (CLAY_ACTION (impl), g_value_get_variant (value));
        }

      break;

    default:
      clay_action_set_property (object, prop_id, value, pspec);
      break;
    }
}

static void
clay_action_impl_activate (GAction  *action,
                           GVariant *parameter)
{
  g_object_add_weak_pointer (G_OBJECT (action), (gpointer) &action);

  clay_action_emit_activate (CLAY_ACTION (action), parameter);

  if (action != NULL)
    {
      /* Some actions are self-destructive, such as the "windows-recent-*"
       * actions which don't exist after being run. Don't log these.
       */
      g_object_remove_weak_pointer (G_OBJECT (action), (gpointer) &action);

      clay_action_history_action_activated (CLAY_ACTION (action));
    }
}

static void
clay_action_impl_change_state (GAction  *action,
                               GVariant *value)
{
  /* If the user connected a signal handler then they are responsible
   * for handling state changes.
   */
  if (g_signal_has_handler_pending (action, clay_action_impl_signals[CHANGE_STATE], 0, TRUE))
    g_signal_emit (action, clay_action_impl_signals[CHANGE_STATE], 0, value);
  else
    /* If not, then the default behavior is to just set the state. */
    clay_action_impl_set_state (CLAY_ACTION (action), value);
}

static gboolean
clay_action_impl_get_enabled (GAction *action)
{
  return clay_action_is_sensitive (CLAY_ACTION (action), NULL);
}

static const GVariantType *
clay_action_impl_get_parameter_type (GAction *action)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (action);

  return impl->priv->parameter_type;
}

static const GVariantType *
clay_action_impl_get_state_type (GAction *action)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (action);

  if (impl->priv->state != NULL)
    return g_variant_get_type (impl->priv->state);
  else
    return NULL;
}

static GVariant *
clay_action_impl_get_state (GAction *action)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (action);

  return impl->priv->state ? g_variant_ref (impl->priv->state) : NULL;
}

static GVariant *
clay_action_impl_get_state_hint (GAction *action)
{
  ClayActionImpl *impl = CLAY_ACTION_IMPL (action);

  if (impl->priv->state_hint != NULL)
    return g_variant_ref (impl->priv->state_hint);
  else
    return NULL;
}


/*  public functions  */

ClayAction *
clay_action_impl_new (const gchar *name,
                      const gchar *label,
                      const gchar *short_label,
                      const gchar *tooltip,
                      const gchar *icon_name,
                      const gchar *help_id,
                      ClayContext *context)
{
  ClayAction *action;

  action = g_object_new (CLAY_TYPE_ACTION_IMPL,
                         "name",        name,
                         "label",       label,
                         "short-label", short_label,
                         "tooltip",     tooltip,
                         "icon-name",   icon_name,
                         "context",     context,
                         NULL);

  clay_action_set_help_id (action, help_id);

  return action;
}


/*  private functions  */

static void
clay_action_impl_set_state (ClayAction *clay_action,
                            GVariant   *value)
{
  ClayActionImpl     *impl;
  const GVariantType *state_type;

  g_return_if_fail (CLAY_IS_ACTION (clay_action));
  g_return_if_fail (value != NULL);

  impl = CLAY_ACTION_IMPL (clay_action);

  state_type = impl->priv->state ? g_variant_get_type (impl->priv->state) : NULL;

  g_return_if_fail (state_type != NULL);
  g_return_if_fail (g_variant_is_of_type (value, state_type));

  g_variant_ref_sink (value);

  if (! impl->priv->state || ! g_variant_equal (impl->priv->state, value))
    {
      if (impl->priv->state)
        g_variant_unref (impl->priv->state);

      impl->priv->state = g_variant_ref (value);

      g_object_notify (G_OBJECT (impl), "state");
    }

  g_variant_unref (value);
}
