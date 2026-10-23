/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfilter.h
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

#include "clayviewable.h"


#define CLAY_TYPE_FILTER (clay_filter_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayFilter,
                          clay_filter,
                          CLAY, FILTER,
                          ClayViewable)


struct _ClayFilterClass
{
  ClayViewableClass  parent_class;

  /*  signals  */
  void       (* active_changed) (ClayFilter *filter);

  /*  virtual functions  */
  GeglNode * (* get_node)       (ClayFilter *filter);
};


ClayFilter     * clay_filter_new              (const gchar    *name);

GeglNode       * clay_filter_get_node         (ClayFilter     *filter);
GeglNode       * clay_filter_peek_node        (ClayFilter     *filter);

void             clay_filter_set_active       (ClayFilter     *filter,
                                               gboolean        active);
gboolean         clay_filter_get_active       (ClayFilter     *filter);

void             clay_filter_set_is_last_node (ClayFilter     *filter,
                                               gboolean        is_last_node);
gboolean         clay_filter_get_is_last_node (ClayFilter     *filter);

void             clay_filter_set_applicator   (ClayFilter     *filter,
                                               ClayApplicator *applicator);
ClayApplicator * clay_filter_get_applicator   (ClayFilter     *filter);
