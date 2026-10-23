/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycontroller.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "claywidgetstypes.h"

#include "claywidgetsmarshal.h"

#include "libclay/libclay-intl.h"

#define CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "claycontroller.h"
#include "clayicons.h"


/**
 * SECTION: claycontroller
 * @title: ClayController
 * @short_description: Pluggable Clay input controller modules.
 *
 * An abstract interface for implementing arbitrary input controllers.
 **/


enum
{
  PROP_0,
  PROP_NAME,
  PROP_STATE
};

enum
{
  EVENT,
  LAST_SIGNAL
};

typedef struct _ClayControllerPrivate
{
  gchar    *name;
  gchar    *state;
} ClayControllerPrivate;

#define GET_PRIVATE(obj) ((ClayControllerPrivate *) clay_controller_get_instance_private ((ClayController *) (obj)))


static void   clay_controller_finalize     (GObject      *object);
static void   clay_controller_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   clay_controller_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ClayController, clay_controller, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (ClayController)
                                  G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL))

#define parent_class clay_controller_parent_class

static guint controller_signals[LAST_SIGNAL] = { 0 };


static void
clay_controller_class_init (ClayControllerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_controller_finalize;
  object_class->set_property = clay_controller_set_property;
  object_class->get_property = clay_controller_get_property;

  klass->name                = "Unnamed";
  klass->help_domain         = NULL;
  klass->help_id             = NULL;
  klass->icon_name           = CLAY_ICON_CONTROLLER;

  klass->get_n_events        = NULL;
  klass->get_event_name      = NULL;
  klass->event               = NULL;

  g_object_class_install_property (object_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        _("The controller's name"),
                                                        "Unnamed Controller",
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATE,
                                   g_param_spec_string ("state",
                                                        "State",
                                                        _("The controller's state, as human-readable string"),
                                                        "Unknown",
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  controller_signals[EVENT] =
    g_signal_new ("event",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayControllerClass, event),
                  g_signal_accumulator_true_handled, NULL,
                  _clay_widgets_marshal_BOOLEAN__POINTER,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_POINTER);
}

static void
clay_controller_init (ClayController *controller)
{
}

static void
clay_controller_finalize (GObject *object)
{
  ClayController        *controller = CLAY_CONTROLLER (object);
  ClayControllerPrivate *priv       = GET_PRIVATE (controller);

  g_clear_pointer (&priv->name,  g_free);
  g_clear_pointer (&priv->state, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_controller_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayController        *controller = CLAY_CONTROLLER (object);
  ClayControllerPrivate *priv       = GET_PRIVATE (controller);

  switch (property_id)
    {
    case PROP_NAME:
      if (priv->name)
        g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;
    case PROP_STATE:
      if (priv->state)
        g_free (priv->state);
      priv->state = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_controller_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayController        *controller = CLAY_CONTROLLER (object);
  ClayControllerPrivate *priv       = GET_PRIVATE (controller);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;
    case PROP_STATE:
      g_value_set_string (value, priv->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

ClayController *
clay_controller_new (GType controller_type)
{
  ClayController *controller;

  g_return_val_if_fail (g_type_is_a (controller_type, CLAY_TYPE_CONTROLLER),
                        NULL);

  controller = g_object_new (controller_type, NULL);

  return controller;
}

gint
clay_controller_get_n_events (ClayController *controller)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER (controller), 0);

  if (CLAY_CONTROLLER_GET_CLASS (controller)->get_n_events)
    return CLAY_CONTROLLER_GET_CLASS (controller)->get_n_events (controller);

  return 0;
}

const gchar *
clay_controller_get_event_name (ClayController *controller,
                                gint            event_id)
{
  const gchar *name = NULL;

  g_return_val_if_fail (CLAY_IS_CONTROLLER (controller), NULL);

  if (CLAY_CONTROLLER_GET_CLASS (controller)->get_event_name)
    name = CLAY_CONTROLLER_GET_CLASS (controller)->get_event_name (controller,
                                                                   event_id);

  if (! name)
    name = "<invalid event id>";

  return name;
}

const gchar *
clay_controller_get_event_blurb (ClayController *controller,
                                 gint            event_id)
{
  const gchar *blurb = NULL;

  g_return_val_if_fail (CLAY_IS_CONTROLLER (controller), NULL);

  if (CLAY_CONTROLLER_GET_CLASS (controller)->get_event_blurb)
    blurb =  CLAY_CONTROLLER_GET_CLASS (controller)->get_event_blurb (controller,
                                                                      event_id);

  if (! blurb)
    blurb = "<invalid event id>";

  return blurb;
}

gboolean
clay_controller_event (ClayController            *controller,
                       const ClayControllerEvent *event)
{
  gboolean retval = FALSE;

  g_return_val_if_fail (CLAY_IS_CONTROLLER (controller), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  g_signal_emit (controller, controller_signals[EVENT], 0,
                 event, &retval);

  return retval;
}
