/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo-book.h
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
 * ClaySessionInfoBook:
 *
 * Contains information about a book (a GtkNotebook of dockables) in
 * the interface.
 */
struct _ClaySessionInfoBook
{
  gint   position;
  gint   current_page;

  /*  list of ClaySessionInfoDockable  */
  GList *dockables;
};


ClaySessionInfoBook *
             clay_session_info_book_new         (void);
void         clay_session_info_book_free        (ClaySessionInfoBook  *info);

void         clay_session_info_book_serialize   (ClayConfigWriter     *writer,
                                                 ClaySessionInfoBook  *book);
GTokenType   clay_session_info_book_deserialize (GScanner             *scanner,
                                                 gint                  scope,
                                                 ClaySessionInfoBook **book);

ClaySessionInfoBook *
             clay_session_info_book_from_widget (ClayDockbook         *dockbook);

ClayDockbook * clay_session_info_book_restore   (ClaySessionInfoBook  *info,
                                                 ClayDock             *dock);
