/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessionmanaged.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "widgets-types.h"

#include "claysessionmanaged.h"


G_DEFINE_INTERFACE (ClaySessionManaged, clay_session_managed, GTK_TYPE_WIDGET)


/*  private functions  */


static void
clay_session_managed_default_init (ClaySessionManagedInterface *iface)
{
}


/*  public functions  */


/**
 * clay_session_managed_get_aux_info:
 * @session_managed: A #ClaySessionManaged
 *
 * Returns: A list of #ClaySessionInfoAux created with
 *          clay_session_info_aux_new().
 **/
GList *
clay_session_managed_get_aux_info (ClaySessionManaged *session_managed)
{
  ClaySessionManagedInterface *iface;

  g_return_val_if_fail (CLAY_IS_SESSION_MANAGED (session_managed), NULL);

  iface = CLAY_SESSION_MANAGED_GET_IFACE (session_managed);

  if (iface->get_aux_info)
    return iface->get_aux_info (session_managed);

  return NULL;
}

/**
 * clay_session_managed_get_ui_manager:
 * @session_managed: A #ClaySessionManaged
 * @aux_info         A list of #ClaySessionInfoAux
 *
 * Sets aux data previously returned from
 * clay_session_managed_get_aux_info().
 **/
void
clay_session_managed_set_aux_info (ClaySessionManaged *session_managed,
                                   GList              *aux_info)
{
  ClaySessionManagedInterface *iface;

  g_return_if_fail (CLAY_IS_SESSION_MANAGED (session_managed));

  iface = CLAY_SESSION_MANAGED_GET_IFACE (session_managed);

  if (iface->set_aux_info)
    iface->set_aux_info (session_managed, aux_info);
}
