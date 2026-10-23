/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo-dockable.h
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
 * ClaySessionInfoDockable:
 *
 * Contains information about a dockable in the interface.
 */
struct _ClaySessionInfoDockable
{
  gchar        *identifier;
  gboolean      locked;
  ClayTabStyle  tab_style;
  gint          view_size;

  /*  dialog specific list of ClaySessionInfoAux  */
  GList        *aux_info;
};


ClaySessionInfoDockable *
               clay_session_info_dockable_new         (void);
void           clay_session_info_dockable_free        (ClaySessionInfoDockable  *info);

void           clay_session_info_dockable_serialize   (ClayConfigWriter         *writer,
                                                       ClaySessionInfoDockable  *dockable);
GTokenType     clay_session_info_dockable_deserialize (GScanner                 *scanner,
                                                       gint                      scope,
                                                       ClaySessionInfoDockable **dockable);

ClaySessionInfoDockable *
               clay_session_info_dockable_from_widget (ClayDockable             *dockable);

ClayDockable * clay_session_info_dockable_restore     (ClaySessionInfoDockable  *info,
                                                       ClayDock                 *dock);
