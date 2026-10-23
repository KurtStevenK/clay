/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayratioentry.h
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_NUMBER_PAIR_ENTRY_H__
#define __CLAY_NUMBER_PAIR_ENTRY_H__

G_BEGIN_DECLS


#define CLAY_TYPE_NUMBER_PAIR_ENTRY (clay_number_pair_entry_get_type ())
G_DECLARE_FINAL_TYPE (ClayNumberPairEntry, clay_number_pair_entry, CLAY, NUMBER_PAIR_ENTRY, GtkEntry)


GtkWidget *    clay_number_pair_entry_new                (const gchar         *separators,
                                                          gboolean             allow_simplification,
                                                          gdouble              min_valid_value,
                                                          gdouble              max_valid_value);
void           clay_number_pair_entry_set_default_values (ClayNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           clay_number_pair_entry_get_default_values (ClayNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);
void           clay_number_pair_entry_set_values         (ClayNumberPairEntry *entry,
                                                          gdouble              left,
                                                          gdouble              right);
void           clay_number_pair_entry_get_values         (ClayNumberPairEntry *entry,
                                                          gdouble             *left,
                                                          gdouble             *right);

void           clay_number_pair_entry_set_default_text   (ClayNumberPairEntry *entry,
                                                          const gchar         *string);
const gchar *  clay_number_pair_entry_get_default_text   (ClayNumberPairEntry *entry);

void           clay_number_pair_entry_set_ratio          (ClayNumberPairEntry *entry,
                                                          gdouble              ratio);
gdouble        clay_number_pair_entry_get_ratio          (ClayNumberPairEntry *entry);

void           clay_number_pair_entry_set_aspect         (ClayNumberPairEntry *entry,
                                                          ClayAspectType       aspect);
ClayAspectType clay_number_pair_entry_get_aspect         (ClayNumberPairEntry *entry);

void           clay_number_pair_entry_set_user_override  (ClayNumberPairEntry *entry,
                                                          gboolean             user_override);
gboolean       clay_number_pair_entry_get_user_override  (ClayNumberPairEntry *entry);


G_END_DECLS

#endif /* __CLAY_NUMBER_PAIR_ENTRY_H__ */
