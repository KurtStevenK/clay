/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_RULER_H__
#define __CLAY_RULER_H__

G_BEGIN_DECLS

#define CLAY_TYPE_RULER (clay_ruler_get_type ())
G_DECLARE_FINAL_TYPE (ClayRuler, clay_ruler, CLAY, RULER, GtkWidget)


GtkWidget * clay_ruler_new                 (GtkOrientation  orientation);

void        clay_ruler_add_track_widget    (ClayRuler      *ruler,
                                            GtkWidget      *widget);
void        clay_ruler_remove_track_widget (ClayRuler      *ruler,
                                            GtkWidget      *widget);

void        clay_ruler_set_unit            (ClayRuler      *ruler,
                                            ClayUnit       *unit);
ClayUnit  * clay_ruler_get_unit            (ClayRuler      *ruler);
void        clay_ruler_set_position        (ClayRuler      *ruler,
                                            gdouble         position);
gdouble     clay_ruler_get_position        (ClayRuler      *ruler);
void        clay_ruler_set_range           (ClayRuler      *ruler,
                                            gdouble         lower,
                                            gdouble         upper,
                                            gdouble         max_size);
void        clay_ruler_get_range           (ClayRuler      *ruler,
                                            gdouble        *lower,
                                            gdouble        *upper,
                                            gdouble        *max_size);

G_END_DECLS

#endif /* __CLAY_RULER_H__ */
