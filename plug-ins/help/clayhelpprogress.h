/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The Clay Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Henrik Brix Andersen <brix@gimp.org>
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


typedef struct
{
  void  (* start)           (const gchar *message,
                             gboolean     cancelable,
                             gpointer     user_data);
  void  (* end)             (gpointer     user_data);
  void  (* set_value)       (gdouble      percentage,
                             gpointer     user_data);

  /* Padding for future expansion. Must be initialized with NULL! */
  void  (* _clay_reserved1) (void);
  void  (* _clay_reserved2) (void);
  void  (* _clay_reserved3) (void);
  void  (* _clay_reserved4) (void);
} ClayHelpProgressVTable;


ClayHelpProgress * clay_help_progress_new    (const ClayHelpProgressVTable *vtable,
                                              gpointer                      user_data);
void               clay_help_progress_free   (ClayHelpProgress *progress);

void               clay_help_progress_cancel (ClayHelpProgress *progress);
