/* The Clay -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayfilloptions.h
 * Copyright (C) 2003 Simon Budig
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

#include "claycontext.h"


#define CLAY_TYPE_FILL_OPTIONS (clay_fill_options_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayFillOptions,
                          clay_fill_options,
                          CLAY, FILL_OPTIONS,
                          ClayContext)


struct _ClayFillOptionsClass
{
  ClayContextClass  parent_class;
};


ClayFillOptions * clay_fill_options_new              (Clay                *clay,
                                                      ClayContext         *context,
                                                      gboolean             use_context_color);

ClayFillStyle     clay_fill_options_get_style        (ClayFillOptions     *options);
void              clay_fill_options_set_style        (ClayFillOptions     *options,
                                                      ClayFillStyle        style);

ClayCustomStyle   clay_fill_options_get_custom_style (ClayFillOptions     *options);
void              clay_fill_options_set_custom_style (ClayFillOptions     *options,
                                                      ClayCustomStyle      custom_style);

gboolean          clay_fill_options_get_antialias    (ClayFillOptions     *options);
void              clay_fill_options_set_antialias    (ClayFillOptions     *options,
                                                      gboolean             antialias);

gboolean          clay_fill_options_get_feather      (ClayFillOptions     *options,
                                                      gdouble             *radius);
void              clay_fill_options_set_feather      (ClayFillOptions     *options,
                                                      gboolean             feather,
                                                      gdouble              radius);

gboolean          clay_fill_options_set_by_fill_type (ClayFillOptions     *options,
                                                      ClayContext         *context,
                                                      ClayFillType         fill_type,
                                                      GError             **error);
gboolean          clay_fill_options_set_by_fill_mode (ClayFillOptions     *options,
                                                      ClayContext         *context,
                                                      ClayBucketFillMode   fill_mode,
                                                      GError             **error);

const gchar     * clay_fill_options_get_undo_desc    (ClayFillOptions     *options);

const Babl      * clay_fill_options_get_format       (ClayFillOptions     *options,
                                                      ClayDrawable        *drawable);

GeglBuffer      * clay_fill_options_create_buffer    (ClayFillOptions     *options,
                                                      ClayDrawable        *drawable,
                                                      const GeglRectangle *rect,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);
void              clay_fill_options_fill_buffer      (ClayFillOptions     *options,
                                                      ClayDrawable        *drawable,
                                                      GeglBuffer          *buffer,
                                                      gint                 pattern_offset_x,
                                                      gint                 pattern_offset_y);

void              clay_fill_options_enable_color_history
                                                     (ClayFillOptions     *options,
                                                      gboolean             enable);
