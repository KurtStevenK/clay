/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockcontainer.c
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

#include "claydockcontainer.h"


G_DEFINE_INTERFACE (ClayDockContainer, clay_dock_container, GTK_TYPE_WIDGET)


/*  private functions  */


static void
clay_dock_container_default_init (ClayDockContainerInterface *iface)
{
}


/*  public functions  */


/**
 * clay_dock_container_get_docks:
 * @container: A #ClayDockContainer
 *
 * Returns: A list of #ClayDock:s in the dock container. Free with
 *          g_list_free() when done.
 **/
GList *
clay_dock_container_get_docks (ClayDockContainer *container)
{
  ClayDockContainerInterface *iface;

  g_return_val_if_fail (CLAY_IS_DOCK_CONTAINER (container), NULL);

  iface = CLAY_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_docks)
    return iface->get_docks (container);

  return NULL;
}

/**
 * clay_dock_container_get_dialog_factory:
 * @container: A #ClayDockContainer
 *
 * Returns: The #ClayDialogFactory of the #ClayDockContainer
 **/
ClayDialogFactory *
clay_dock_container_get_dialog_factory (ClayDockContainer *container)
{
  ClayDockContainerInterface *iface;

  g_return_val_if_fail (CLAY_IS_DOCK_CONTAINER (container), NULL);

  iface = CLAY_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dialog_factory)
    return iface->get_dialog_factory (container);

  return NULL;
}

/**
 * clay_dock_container_get_ui_manager:
 * @container: A #ClayDockContainer
 *
 * Returns: The #ClayUIManager of the #ClayDockContainer
 **/
ClayUIManager *
clay_dock_container_get_ui_manager (ClayDockContainer *container)
{
  ClayDockContainerInterface *iface;

  g_return_val_if_fail (CLAY_IS_DOCK_CONTAINER (container), NULL);

  iface = CLAY_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_ui_manager)
    return iface->get_ui_manager (container);

  return NULL;
}

/**
 * clay_dock_container_add_dock:
 * @container: A #ClayDockContainer
 * @dock:      The newly created #ClayDock to add to the container.
 * @dock_info: The #ClaySessionInfoDock the @dock was created from.
 *
 * Add @dock that was created from @dock_info to @container.
 **/
void
clay_dock_container_add_dock (ClayDockContainer   *container,
                              ClayDock            *dock,
                              ClaySessionInfoDock *dock_info)
{
  ClayDockContainerInterface *iface;

  g_return_if_fail (CLAY_IS_DOCK_CONTAINER (container));

  iface = CLAY_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->add_dock)
    iface->add_dock (container,
                     dock,
                     dock_info);
}

/**
 * clay_dock_container_get_dock_side:
 * @container: A #ClayDockContainer
 * @dock:      A #ClayDock
 *
 * Returns: What side @dock is in in @container, either
 *          CLAY_ALIGN_LEFT or CLAY_ALIGN_RIGHT, or -1 if the side
 *          concept is not applicable.
 **/
ClayAlignmentType
clay_dock_container_get_dock_side (ClayDockContainer   *container,
                                   ClayDock            *dock)
{
  ClayDockContainerInterface *iface;

  g_return_val_if_fail (CLAY_IS_DOCK_CONTAINER (container), -1);

  iface = CLAY_DOCK_CONTAINER_GET_IFACE (container);

  if (iface->get_dock_side)
    return iface->get_dock_side (container, dock);

  return -1;
}
