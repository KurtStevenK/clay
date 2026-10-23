/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "claycoloroptions.h"


#define CLAY_TYPE_FILTER_OPTIONS            (clay_filter_options_get_type ())
#define CLAY_FILTER_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILTER_OPTIONS, ClayFilterOptions))
#define CLAY_FILTER_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILTER_OPTIONS, ClayFilterOptionsClass))
#define CLAY_IS_FILTER_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILTER_OPTIONS))
#define CLAY_IS_FILTER_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FILTER_OPTIONS))
#define CLAY_FILTER_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FILTER_OPTIONS, ClayFilterOptionsClass))


typedef struct _ClayFilterOptionsClass ClayFilterOptionsClass;

struct _ClayFilterOptions
{
  ClayColorOptions   parent_instance;

  gboolean           preview;
  gboolean           preview_split;
  ClayAlignmentType  preview_split_alignment;
  gint               preview_split_position;
  gboolean           controller;

  gboolean           blending_options_expanded;

  gboolean           merge_filter;
};

struct _ClayFilterOptionsClass
{
  ClayColorOptionsClass  parent_class;
};


GType   clay_filter_options_get_type                   (void);

void    clay_filter_options_switch_preview_side        (ClayFilterOptions *options);
void    clay_filter_options_switch_preview_orientation (ClayFilterOptions *options,
                                                        gint               position_x,
                                                        gint               position_y);
