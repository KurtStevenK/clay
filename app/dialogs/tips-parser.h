/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * tips-parser.h - Parse the clay-tips.xml file.
 * Copyright (C) 2002, 2008  Sven Neumann <sven@gimp.org>
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


typedef struct _ClayTip ClayTip;

struct _ClayTip
{
  gchar *text;
  gchar *help_id;
};


ClayTip * clay_tip_new        (const gchar  *title,
                               const gchar  *format,
                               ...) G_GNUC_PRINTF(2, 3);
void      clay_tip_free       (ClayTip      *tip);

GList   * clay_tips_from_file (GFile        *file,
                               GError      **error);
void      clay_tips_free      (GList        *tips);
