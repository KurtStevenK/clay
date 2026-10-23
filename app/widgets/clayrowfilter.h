/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowfilter.h
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "clayrow.h"


#define CLAY_TYPE_ROW_FILTER (clay_row_filter_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayRowFilter,
                          clay_row_filter,
                          CLAY, ROW_FILTER,
                          ClayRow)


struct _ClayRowFilterClass
{
  ClayRowClass  parent_class;

  const gchar  *active_property;
  const gchar  *active_changed_signal;

  void (* active_toggled) (ClayRowFilter *row,
                           gboolean       active);
};
