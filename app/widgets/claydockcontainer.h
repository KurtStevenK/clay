/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockcontainer.h
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

#pragma once


#define CLAY_TYPE_DOCK_CONTAINER (clay_dock_container_get_type ())
G_DECLARE_INTERFACE (ClayDockContainer,
                     clay_dock_container,
                     CLAY, DOCK_CONTAINER,
                     GtkWidget)


struct _ClayDockContainerInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  GList             * (* get_docks)          (ClayDockContainer   *container);
  ClayDialogFactory * (* get_dialog_factory) (ClayDockContainer   *container);
  ClayUIManager     * (* get_ui_manager)     (ClayDockContainer   *container);
  void                (* add_dock)           (ClayDockContainer   *container,
                                              ClayDock            *dock,
                                              ClaySessionInfoDock *dock_info);
  ClayAlignmentType   (* get_dock_side)      (ClayDockContainer   *container,
                                              ClayDock            *dock);
};


GList             * clay_dock_container_get_docks          (ClayDockContainer   *container);
ClayDialogFactory * clay_dock_container_get_dialog_factory (ClayDockContainer   *container);
ClayUIManager     * clay_dock_container_get_ui_manager     (ClayDockContainer   *container);
void                clay_dock_container_add_dock           (ClayDockContainer   *container,
                                                            ClayDock            *dock,
                                                            ClaySessionInfoDock *dock_info);
ClayAlignmentType   clay_dock_container_get_dock_side      (ClayDockContainer   *container,
                                                            ClayDock            *dock);
