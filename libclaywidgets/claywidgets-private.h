/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claywidgets-private.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_WIDGETS_PRIVATE_H__
#define __CLAY_WIDGETS_PRIVATE_H__


typedef GeglColor * (* ClayGetColorFunc)      (void);
typedef void        (* ClayEnsureModulesFunc) (void);


extern ClayHelpFunc          _clay_standard_help_func;
extern ClayGetColorFunc      _clay_get_foreground_func;
extern ClayGetColorFunc      _clay_get_background_func;
extern ClayEnsureModulesFunc _clay_ensure_modules_func;


G_BEGIN_DECLS


void  clay_widgets_init               (ClayHelpFunc           standard_help_func,
                                       ClayGetColorFunc       get_foreground_func,
                                       ClayGetColorFunc       get_background_func,
                                       ClayEnsureModulesFunc  ensure_modules_func,
                                       const gchar           *test_base_dir);

void  clay_widget_set_identifier      (GtkWidget             *widget,
                                       const gchar           *identifier);
void  clay_widget_set_bound_property  (GtkWidget             *widget,
                                       GObject               *config,
                                       const gchar           *property_name);

void  clay_widget_set_title_bar_theme (GtkWidget             *dialog);

void  clay_widget_set_auto_transient  (GtkWidget             *dialog);


G_END_DECLS

#endif /* __CLAY_WIDGETS_PRIVATE_H__ */
