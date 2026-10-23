/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo-dock.h
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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


/**
 * ClaySessionInfoDock:
 *
 * Contains information about a dock in the interface.
 */
struct _ClaySessionInfoDock
{
  /* Type of dock, written to/read from sessionrc. E.g. 'clay-dock' or
   * 'clay-toolbox'
   */
  gchar             *dock_type;

  /* What side this dock is in in single-window mode. Either
   * CLAY_ALIGN_LEFT, CLAY_ALIGN_RIGHT or -1.
   */
  ClayAlignmentType  side;

  /* GtkPaned position of this dock */
  gint               position;

  /*  list of ClaySessionInfoBook  */
  GList             *books;
};

ClaySessionInfoDock * clay_session_info_dock_new         (const gchar          *dock_type);
void                  clay_session_info_dock_free        (ClaySessionInfoDock  *dock_info);
void                  clay_session_info_dock_serialize   (ClayConfigWriter     *writer,
                                                          ClaySessionInfoDock  *dock);
GTokenType            clay_session_info_dock_deserialize (GScanner             *scanner,
                                                          gint                  scope,
                                                          ClaySessionInfoDock **info,
                                                          const gchar          *dock_type);
ClaySessionInfoDock * clay_session_info_dock_from_widget (ClayDock             *dock);
ClayDock            * clay_session_info_dock_restore     (ClaySessionInfoDock  *dock_info,
                                                          ClayDialogFactory    *factory,
                                                          GdkMonitor           *monitor,
                                                          ClayDockContainer    *dock_container);
