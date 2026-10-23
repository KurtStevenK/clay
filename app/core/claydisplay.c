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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"

#include "claydisplay.h"

#include "clay-intl.h"

enum
{
  PROP_0,
  PROP_ID,
  PROP_CLAY
};


struct _ClayDisplayPrivate
{
  gint id; /* unique identifier for this display */
};


/*  local function prototypes  */

static void     clay_display_set_property           (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     clay_display_get_property           (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDisplay, clay_display, CLAY_TYPE_OBJECT)

#define parent_class clay_display_parent_class


static void
clay_display_class_init (ClayDisplayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_display_set_property;
  object_class->get_property = clay_display_get_property;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id",
                                                     NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     CLAY_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_display_init (ClayDisplay *display)
{
  display->priv = clay_display_get_instance_private (display);
}

static void
clay_display_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  ClayDisplay        *display = CLAY_DISPLAY (object);
  ClayDisplayPrivate *private = display->priv;

  switch (property_id)
    {
    case PROP_CLAY:
      {
        gint id;

        display->clay = g_value_get_object (value); /* don't ref the clay */
        display->config = CLAY_DISPLAY_CONFIG (display->clay->config);

        do
          {
            id = display->clay->next_display_id++;

            if (display->clay->next_display_id == G_MAXINT)
              display->clay->next_display_id = 1;
          }
        while (clay_display_get_by_id (display->clay, id));

        private->id = id;
      }
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
      g_value_set_int (value, display->priv->id);
      break;

    case PROP_CLAY:
      g_value_set_object (value, display->clay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

gint
clay_display_get_id (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), -1);

  return display->priv->id;
}

ClayDisplay *
clay_display_get_by_id (Clay *clay,
                        gint  id)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  for (list = clay_get_display_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplay *display = list->data;

      if (clay_display_get_id (display) == id)
        return display;
    }

  return NULL;
}

gboolean
clay_display_present (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  if (CLAY_DISPLAY_GET_CLASS (display)->present)
    return CLAY_DISPLAY_GET_CLASS (display)->present (display);

  return FALSE;
}

gboolean
clay_display_grab_focus (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  if (CLAY_DISPLAY_GET_CLASS (display)->grab_focus)
    return CLAY_DISPLAY_GET_CLASS (display)->grab_focus (display);

  return FALSE;
}

Clay *
clay_display_get_clay (ClayDisplay *display)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), NULL);

  return display->clay;
}
