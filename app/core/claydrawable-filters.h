/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawable-filters.h
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


/*  internal functions  */

void            _clay_drawable_filters_init         (ClayDrawable *drawable);
void            _clay_drawable_filters_finalize     (ClayDrawable *drawable);


/*  public functions  */

ClayContainer * clay_drawable_get_filters           (ClayDrawable *drawable);

gboolean        clay_drawable_has_visible_filters   (ClayDrawable *drawable);

gboolean        clay_drawable_n_editable_filters    (ClayDrawable *drawable,
                                                     gint         *n_editable,
                                                     gint         *first,
                                                     gint         *last);

void            clay_drawable_add_filter            (ClayDrawable *drawable,
                                                     ClayFilter   *filter);
void            clay_drawable_remove_filter         (ClayDrawable *drawable,
                                                     ClayFilter   *filter);
void            clay_drawable_clear_filters         (ClayDrawable *drawable);

gboolean        clay_drawable_has_filter            (ClayDrawable *drawable,
                                                     ClayFilter   *filter);

gboolean        clay_drawable_raise_filter          (ClayDrawable *drawable,
                                                     ClayFilter   *filter);
gboolean        clay_drawable_lower_filter          (ClayDrawable *drawable,
                                                     ClayFilter   *filter);

void            clay_drawable_merge_filters         (ClayDrawable *drawable);
gboolean        clay_drawable_merge_filter          (ClayDrawable *drawable,
                                                     ClayFilter   *filter,
                                                     ClayProgress *progress,
                                                     const gchar  *undo_desc,
                                                     const Babl   *format,
                                                     gboolean      clip,
                                                     gboolean      cancellable,
                                                     gboolean      update);
