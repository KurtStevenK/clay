/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ClayColorManaged interface
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "claycolortypes.h"

#include "claycolormanaged.h"
#include "claycolorprofile.h"


/**
 * SECTION: claycolormanaged
 * @title: ClayColorManaged
 * @short_description: An interface dealing with color profiles.
 *
 * An interface dealing with color profiles.
 **/


enum
{
  PROFILE_CHANGED,
  SIMULATION_PROFILE_CHANGED,
  SIMULATION_INTENT_CHANGED,
  SIMULATION_BPC_CHANGED,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (ClayColorManaged, clay_color_managed, G_TYPE_OBJECT)


static guint clay_color_managed_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
clay_color_managed_default_init (ClayColorManagedInterface *iface)
{
  clay_color_managed_signals[PROFILE_CHANGED] =
    g_signal_new ("profile-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorManagedInterface,
                                   profile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_color_managed_signals[SIMULATION_PROFILE_CHANGED] =
    g_signal_new ("simulation-profile-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorManagedInterface,
                                   simulation_profile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_color_managed_signals[SIMULATION_INTENT_CHANGED] =
    g_signal_new ("simulation-intent-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorManagedInterface,
                                   simulation_intent_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_color_managed_signals[SIMULATION_BPC_CHANGED] =
    g_signal_new ("simulation-bpc-changed",
                  G_TYPE_FROM_INTERFACE (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayColorManagedInterface,
                                   simulation_bpc_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}


/*  public functions  */


/**
 * clay_color_managed_get_icc_profile:
 * @managed: an object the implements the #ClayColorManaged interface
 * @len: (out): return location for the number of bytes in the profile data
 *
 * Returns: (array length=len): A blob of data that represents an ICC color
 *                              profile.
 *
 * Since: 2.4
 */
const guint8 *
clay_color_managed_get_icc_profile (ClayColorManaged *managed,
                                    gsize            *len)
{
  ClayColorManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_COLOR_MANAGED (managed), NULL);
  g_return_val_if_fail (len != NULL, NULL);

  *len = 0;

  iface = CLAY_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_icc_profile)
    return iface->get_icc_profile (managed, len);

  return NULL;
}

/**
 * clay_color_managed_get_color_profile:
 * @managed: an object the implements the #ClayColorManaged interface
 *
 * This function always returns a #ClayColorProfile and falls back to
 * clay_color_profile_new_rgb_srgb() if the method is not implemented.
 *
 * Returns: (transfer full): The @managed's #ClayColorProfile.
 *
 * Since: 2.10
 **/
ClayColorProfile *
clay_color_managed_get_color_profile (ClayColorManaged *managed)
{
  ClayColorManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_COLOR_MANAGED (managed), NULL);

  iface = CLAY_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_color_profile)
    return iface->get_color_profile (managed);

  return NULL;
}

/**
 * clay_color_managed_get_simulation_profile:
 * @managed: an object the implements the #ClayColorManaged interface
 *
 * This function always returns a #ClayColorProfile
 *
 * Returns: (transfer full): The @managed's simulation #ClayColorProfile.
 *
 * Since: 3.0
 **/
ClayColorProfile *
clay_color_managed_get_simulation_profile (ClayColorManaged *managed)
{
  ClayColorManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_COLOR_MANAGED (managed), NULL);

  iface = CLAY_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_profile)
    return iface->get_simulation_profile (managed);

  return NULL;
}

/**
 * clay_color_managed_get_simulation_intent:
 * @managed: an object the implements the #ClayColorManaged interface
 *
 * This function always returns a #ClayColorRenderingIntent
 *
 * Returns: The @managed's simulation #ClayColorRenderingIntent.
 *
 * Since: 3.0
 **/
ClayColorRenderingIntent
clay_color_managed_get_simulation_intent (ClayColorManaged *managed)
{
  ClayColorManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_COLOR_MANAGED (managed),
                        CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  iface = CLAY_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_intent)
    return iface->get_simulation_intent (managed);

  return CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
}

/**
 * clay_color_managed_get_simulation_bpc:
 * @managed: an object the implements the #ClayColorManaged interface
 *
 * This function always returns a gboolean representing whether
 * Black Point Compensation is enabled
 *
 * Returns: The @managed's simulation Black Point Compensation value.
 *
 * Since: 3.0
 **/
gboolean
clay_color_managed_get_simulation_bpc (ClayColorManaged *managed)
{
  ClayColorManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_COLOR_MANAGED (managed), FALSE);

  iface = CLAY_COLOR_MANAGED_GET_IFACE (managed);

  if (iface->get_simulation_bpc)
    return iface->get_simulation_bpc (managed);

  return FALSE;
}


/**
 * clay_color_managed_profile_changed:
 * @managed: an object that implements the #ClayColorManaged interface
 *
 * Emits the "profile-changed" signal.
 *
 * Since: 2.4
 **/
void
clay_color_managed_profile_changed (ClayColorManaged *managed)
{
  g_return_if_fail (CLAY_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, clay_color_managed_signals[PROFILE_CHANGED], 0);
}

/**
 * clay_color_managed_simulation_profile_changed:
 * @managed: an object that implements the #ClayColorManaged interface
 *
 * Emits the "simulation-profile-changed" signal.
 *
 * Since: 3.0
 **/
void
clay_color_managed_simulation_profile_changed (ClayColorManaged *managed)
{
  g_return_if_fail (CLAY_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, clay_color_managed_signals[SIMULATION_PROFILE_CHANGED], 0);
}

/**
 * clay_color_managed_simulation_intent_changed:
 * @managed: an object that implements the #ClayColorManaged interface
 *
 * Emits the "simulation-intent-changed" signal.
 *
 * Since: 3.0
 **/
void
clay_color_managed_simulation_intent_changed (ClayColorManaged *managed)
{
  g_return_if_fail (CLAY_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, clay_color_managed_signals[SIMULATION_INTENT_CHANGED], 0);
}

/**
 * clay_color_managed_simulation_bpc_changed:
 * @managed: an object that implements the #ClayColorManaged interface
 *
 * Emits the "simulation-bpc-changed" signal.
 *
 * Since: 3.0
 **/
void
clay_color_managed_simulation_bpc_changed (ClayColorManaged *managed)
{
  g_return_if_fail (CLAY_IS_COLOR_MANAGED (managed));

  g_signal_emit (managed, clay_color_managed_signals[SIMULATION_BPC_CHANGED], 0);
}
