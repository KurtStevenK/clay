/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claybrowser.h
 * Copyright (C) 2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_BROWSER_H__
#define __CLAY_BROWSER_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_BROWSER (clay_browser_get_type ())
G_DECLARE_FINAL_TYPE (ClayBrowser, clay_browser, CLAY, BROWSER, GtkPaned)


GtkWidget * clay_browser_new                (void);

void        clay_browser_add_search_types   (ClayBrowser *browser,
                                             const gchar *first_type_label,
                                             gint         first_type_id,
                                             ...) G_GNUC_NULL_TERMINATED;

GtkWidget * clay_browser_get_left_vbox      (ClayBrowser *browser);
GtkWidget * clay_browser_get_right_vbox     (ClayBrowser *browser);

void        clay_browser_set_search_summary (ClayBrowser *browser,
                                             const gchar *summary);
void        clay_browser_set_widget         (ClayBrowser *browser,
                                             GtkWidget   *widget);
void        clay_browser_show_message       (ClayBrowser *browser,
                                             const gchar *message);


G_END_DECLS

#endif  /*  __CLAY_BROWSER_H__  */
