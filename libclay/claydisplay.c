/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claydisplay.c
 * Copyright (C) Jehan
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

#include "clay.h"

#include "libclaybase/claywire-private.h" /* FIXME kill this include */

#include "clayplugin-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};


struct _ClayDisplay
{
  GObject parent_instance;
  gint    id;
};


static void   clay_display_set_property  (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void   clay_display_get_property  (GObject      *object,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);


G_DEFINE_TYPE (ClayDisplay, clay_display, G_TYPE_OBJECT)

#define parent_class clay_display_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
clay_display_class_init (ClayDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_display_set_property;
  object_class->get_property = clay_display_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The display id",
                      "The display id for internal use",
                      0, G_MAXINT32, 0,
                      CLAY_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
clay_display_init (ClayDisplay *display)
{
}

static void
clay_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClayDisplay *display = CLAY_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      display->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_display_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  ClayDisplay *display = CLAY_DISPLAY (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, display->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * clay_display_get_id:
 * @display: The display.
 *
 * Note: in most use cases, you should not need a display's ID which is
 * mostly internal data and not reusable across sessions.
 *
 * Returns: the display ID.
 *
 * Since: 3.0
 **/
gint32
clay_display_get_id (ClayDisplay *display)
{
  return display ? display->id : -1;
}

/**
 * clay_display_get_by_id:
 * @display_id: The display id.
 *
 * Returns a #ClayDisplay representing @display_id.
 *
 * Note: in most use cases, you should not need to retrieve a
 * #ClayDisplay by its ID, which is mostly internal data and not
 * reusable across sessions. Use the appropriate functions for your use
 * case instead.
 *
 * Returns: (nullable) (transfer none): a #ClayDisplay for @display_id or
 *          %NULL if @display_id does not represent a valid display.
 *          The object belongs to libclay and you must not modify or
 *          unref it.
 *
 * Since: 3.0
 **/
ClayDisplay *
clay_display_get_by_id (gint32 display_id)
{
  if (display_id > 0)
    {
      ClayPlugIn *plug_in = clay_get_plug_in ();

      return _clay_plug_in_get_display (plug_in, display_id);
    }

  return NULL;
}

/**
 * clay_display_is_valid:
 * @display: The display to check.
 *
 * Returns TRUE if the display is valid.
 *
 * This procedure checks if the given display is valid and refers to
 * an existing display.
 *
 * Returns: Whether the display is valid.
 *
 * Since: 2.4
 **/
gboolean
clay_display_is_valid (ClayDisplay *display)
{
  return clay_display_id_is_valid (clay_display_get_id (display));
}
