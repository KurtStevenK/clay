/* gimpparasite.h: Copyright 1998 Jay Cox <jaycox@gimp.org>
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


/* some wrappers to access clay->parasites, mainly for the PDB */

gboolean              clay_parasite_validate     (Clay               *clay,
                                                  const ClayParasite *parasite,
                                                  GError            **error);
void                  clay_parasite_attach       (Clay               *clay,
                                                  const ClayParasite *parasite);
void                  clay_parasite_detach       (Clay               *clay,
                                                  const gchar        *name);
const ClayParasite  * clay_parasite_find         (Clay               *clay,
                                                  const gchar        *name);
gchar              ** clay_parasite_list         (Clay               *clay);

void                  clay_parasite_shift_parent (ClayParasite       *parasite);

void                  clay_parasiterc_load       (Clay               *clay);
void                  clay_parasiterc_save       (Clay               *clay);
