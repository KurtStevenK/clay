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


struct _ClayHelpLocale
{
  gchar      *locale_id;
  GHashTable *help_id_mapping;
  gchar      *help_missing;

  /* eek */
  GList      *toplevel_items;
};


ClayHelpLocale * clay_help_locale_new   (const gchar       *locale_id);
void             clay_help_locale_free  (ClayHelpLocale    *locale);

const gchar    * clay_help_locale_map   (ClayHelpLocale    *locale,
                                         const gchar       *help_id);

gboolean         clay_help_locale_parse (ClayHelpLocale    *locale,
                                         const gchar       *uri,
                                         const gchar       *help_domain,
                                         ClayHelpProgress  *progress,
                                         GError           **error);
