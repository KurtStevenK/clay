/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolordisplay.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "claywidgetstypes.h"

#include "claycolordisplay.h"
#include "clayicons.h"


/**
 * SECTION: claycolordisplay
 * @title: ClayColorDisplay
 * @short_description: Pluggable Clay display color correction modules.
 * @see_also: #GModule, #GTypeModule, #ClayModule
 *
 * Functions and definitions for creating pluggable Clay
 * display color correction modules.
 **/


enum
{
  PROP_0,
  PROP_ENABLED,
  PROP_COLOR_CONFIG,
  PROP_COLOR_MANAGED
};

enum
{
  CHANGED,
  LAST_SIGNAL
};


typedef struct _ClayColorDisplayPrivate
{
  gboolean          enabled;
  ClayColorConfig  *config;
  ClayColorManaged *managed;
} ClayColorDisplayPrivate;

#define GET_PRIVATE(obj) ((ClayColorDisplayPrivate *) clay_color_display_get_instance_private ((ClayColorDisplay *) (obj)))



static void       clay_color_display_constructed  (GObject      *object);
static void       clay_color_display_dispose      (GObject      *object);
static void       clay_color_display_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void       clay_color_display_get_property (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);

static void  clay_color_display_set_color_config  (ClayColorDisplay *display,
                                                   ClayColorConfig  *config);
static void  clay_color_display_set_color_managed (ClayColorDisplay *display,
                                                   ClayColorManaged *managed);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ClayColorDisplay, clay_color_display, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (ClayColorDisplay)
                                  G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL))

#define parent_class clay_color_display_parent_class

static guint display_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_display_class_init (ClayColorDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_color_display_constructed;
  object_class->dispose      = clay_color_display_dispose;
  object_class->set_property = clay_color_display_set_property;
  object_class->get_property = clay_color_display_get_property;

  g_object_class_install_property (object_class, PROP_ENABLED,
                                   g_param_spec_boolean ("enabled",
                                                         "Enabled",
                                                         "Whether this display filter is enabled",
                                                         TRUE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_COLOR_CONFIG,
                                   g_param_spec_object ("color-config",
                                                        "Color Config",
                                                        "The color config used for this filter",
                                                        CLAY_TYPE_COLOR_CONFIG,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_COLOR_MANAGED,
                                   g_param_spec_object ("color-managed",
                                                        "Color Managed",
                                                        "The color managed pixel source that is filtered",
                                                        CLAY_TYPE_COLOR_MANAGED,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  display_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorDisplayClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->name            = "Unnamed";
  klass->help_id         = NULL;
  klass->icon_name       = CLAY_ICON_DISPLAY_FILTER;

  klass->convert_buffer  = NULL;
  klass->configure       = NULL;

  klass->changed         = NULL;
}

static void
clay_color_display_init (ClayColorDisplay *display)
{
}

static void
clay_color_display_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* emit an initial "changed" signal after all construct properties are set */
  clay_color_display_changed (CLAY_COLOR_DISPLAY (object));
}

static void
clay_color_display_dispose (GObject *object)
{
  ClayColorDisplayPrivate *private = GET_PRIVATE (object);

  if (private->config)
    {
      g_signal_handlers_disconnect_by_func (private->config,
                                            clay_color_display_changed,
                                            object);
      g_object_unref (private->config);
      private->config = NULL;
    }

  if (private->managed)
    {
      g_signal_handlers_disconnect_by_func (private->managed,
                                            clay_color_display_changed,
                                            object);
      g_object_unref (private->managed);
      private->managed = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_color_display_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayColorDisplay        *display = CLAY_COLOR_DISPLAY (object);
  ClayColorDisplayPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      private->enabled = g_value_get_boolean (value);
      break;

    case PROP_COLOR_CONFIG:
      clay_color_display_set_color_config (display,
                                           g_value_get_object (value));
      break;

    case PROP_COLOR_MANAGED:
      clay_color_display_set_color_managed (display,
                                            g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_display_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayColorDisplayPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ENABLED:
      g_value_set_boolean (value, private->enabled);
      break;

    case PROP_COLOR_CONFIG:
      g_value_set_object (value, private->config);
      break;

    case PROP_COLOR_MANAGED:
      g_value_set_object (value, private->managed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_color_display_set_color_config (ClayColorDisplay *display,
                                     ClayColorConfig  *config)
{
  ClayColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->config == NULL);

  if (config)
    {
      private->config = g_object_ref (config);

      g_signal_connect_swapped (private->config, "notify",
                                G_CALLBACK (clay_color_display_changed),
                                display);
    }
}

static void
clay_color_display_set_color_managed (ClayColorDisplay *display,
                                      ClayColorManaged *managed)
{
  ClayColorDisplayPrivate *private = GET_PRIVATE (display);

  g_return_if_fail (private->managed == NULL);

  if (managed)
    {
      private->managed = g_object_ref (managed);

      g_signal_connect_swapped (private->managed, "profile-changed",
                                G_CALLBACK (clay_color_display_changed),
                                display);
    }
}

/**
 * clay_color_display_clone:
 * @display: a #ClayColorDisplay
 *
 * Creates a copy of @display.
 *
 * Returns: (transfer full): a duplicate of @display.
 *
 * Since: 2.0
 **/
ClayColorDisplay *
clay_color_display_clone (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), NULL);

  return CLAY_COLOR_DISPLAY (clay_config_duplicate (CLAY_CONFIG (display)));
}

/**
 * clay_color_display_convert_buffer:
 * @display: a #ClayColorDisplay
 * @buffer:  a #GeglBuffer
 * @area:    area in @buffer to convert
 *
 * Converts all pixels in @area of @buffer.
 *
 * Since: 2.10
 **/
void
clay_color_display_convert_buffer (ClayColorDisplay *display,
                                   GeglBuffer       *buffer,
                                   GeglRectangle    *area)
{
  ClayColorDisplayPrivate *private;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  private = GET_PRIVATE (display);

  if (private->enabled &&
      CLAY_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer)
    {
      CLAY_COLOR_DISPLAY_GET_CLASS (display)->convert_buffer (display, buffer,
                                                              area);
    }
}

/**
 * clay_color_display_load_state:
 * @display: a #ClayColorDisplay
 * @state:   a #ClayParasite
 *
 * Configures @display from the contents of the parasite @state.
 * @state must be a properly serialized configuration for a
 * #ClayColorDisplay, such as saved by clay_color_display_save_state().
 *
 * Since: 2.0
 **/
void
clay_color_display_load_state (ClayColorDisplay *display,
                               ClayParasite     *state)
{
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));
  g_return_if_fail (state != NULL);

  clay_config_deserialize_parasite (CLAY_CONFIG (display),
                                    state,
                                    NULL, NULL);
}

/**
 * clay_color_display_save_state:
 * @display: a #ClayColorDisplay
 *
 * Saves the configuration state of @display as a new parasite.
 *
 * Returns: (transfer full): a #ClayParasite
 *
 * Since: 2.0
 **/
ClayParasite *
clay_color_display_save_state (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), NULL);

  return clay_config_serialize_to_parasite (CLAY_CONFIG (display),
                                            "Display/Proof",
                                            CLAY_PARASITE_PERSISTENT,
                                            NULL);
}

/**
 * clay_color_display_configure:
 * @display: a #ClayColorDisplay
 *
 * Creates a configuration widget for @display which can be added to a
 * container widget.
 *
 * Returns: (transfer full): a new configuration widget for @display, or
 *          %NULL if no specific widget exists.
 *
 * Since: 2.0
 **/
GtkWidget *
clay_color_display_configure (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), NULL);

  if (CLAY_COLOR_DISPLAY_GET_CLASS (display)->configure)
    return CLAY_COLOR_DISPLAY_GET_CLASS (display)->configure (display);

  return NULL;
}

void
clay_color_display_configure_reset (ClayColorDisplay *display)
{
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  clay_config_reset (CLAY_CONFIG (display));
}

void
clay_color_display_changed (ClayColorDisplay *display)
{
  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  g_signal_emit (display, display_signals[CHANGED], 0);
}

void
clay_color_display_set_enabled (ClayColorDisplay *display,
                                gboolean          enabled)
{
  ClayColorDisplayPrivate *private;

  g_return_if_fail (CLAY_IS_COLOR_DISPLAY (display));

  private = GET_PRIVATE (display);

  if (enabled != private->enabled)
    {
      g_object_set (display,
                    "enabled", enabled,
                    NULL);
    }
}

gboolean
clay_color_display_get_enabled (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), FALSE);

  return GET_PRIVATE (display)->enabled;
}

/**
 * clay_color_display_get_config:
 * @display:
 *
 * Returns: (transfer none): a pointer to the #ClayColorConfig
 *               object or %NULL.
 *
 * Since: 2.4
 **/
ClayColorConfig *
clay_color_display_get_config (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->config;
}

/**
 * clay_color_display_get_managed:
 * @display:
 *
 * Returns: (transfer none): a pointer to the #ClayColorManaged
 *               object or %NULL.
 *
 * Since: 2.4
 **/
ClayColorManaged *
clay_color_display_get_managed (ClayColorDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_COLOR_DISPLAY (display), NULL);

  return GET_PRIVATE (display)->managed;
}
