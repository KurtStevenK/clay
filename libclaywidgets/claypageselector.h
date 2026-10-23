/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypageselector.h
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

#ifndef __CLAY_PAGE_SELECTOR_H__
#define __CLAY_PAGE_SELECTOR_H__

G_BEGIN_DECLS

#define CLAY_TYPE_PAGE_SELECTOR (clay_page_selector_get_type ())
G_DECLARE_FINAL_TYPE (ClayPageSelector, clay_page_selector, CLAY, PAGE_SELECTOR, GtkBox)


GtkWidget * clay_page_selector_new                (void);

void        clay_page_selector_set_n_pages        (ClayPageSelector *selector,
                                                   gint              n_pages);
gint        clay_page_selector_get_n_pages        (ClayPageSelector *selector);

void        clay_page_selector_set_target   (ClayPageSelector       *selector,
                                             ClayPageSelectorTarget  target);
ClayPageSelectorTarget
            clay_page_selector_get_target   (ClayPageSelector       *selector);

void        clay_page_selector_set_page_thumbnail (ClayPageSelector *selector,
                                                   gint              page_no,
                                                   GdkPixbuf        *thumbnail);
GdkPixbuf * clay_page_selector_get_page_thumbnail (ClayPageSelector *selector,
                                                   gint              page_no);

void        clay_page_selector_set_page_label     (ClayPageSelector *selector,
                                                   gint              page_no,
                                                   const gchar      *label);
gchar     * clay_page_selector_get_page_label     (ClayPageSelector *selector,
                                                   gint              page_no);

void        clay_page_selector_select_all         (ClayPageSelector *selector);
void        clay_page_selector_unselect_all       (ClayPageSelector *selector);
void        clay_page_selector_select_page        (ClayPageSelector *selector,
                                                   gint              page_no);
void        clay_page_selector_unselect_page      (ClayPageSelector *selector,
                                                   gint              page_no);
gboolean    clay_page_selector_page_is_selected   (ClayPageSelector *selector,
                                                   gint              page_no);
gint      * clay_page_selector_get_selected_pages (ClayPageSelector *selector,
                                                   gint             *n_selected_pages);

void        clay_page_selector_select_range       (ClayPageSelector *selector,
                                                   const gchar      *range);
gchar     * clay_page_selector_get_selected_range (ClayPageSelector *selector);

G_END_DECLS

#endif /* __CLAY_PAGE_SELECTOR_H__ */
