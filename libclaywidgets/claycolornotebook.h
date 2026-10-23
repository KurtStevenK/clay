/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolornotebook.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on color_notebook module
 * Copyright (C) 1998 Austin Donnelly <austin@greenend.org.uk>
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

#ifndef __CLAY_COLOR_NOTEBOOK_H__
#define __CLAY_COLOR_NOTEBOOK_H__

#include <libclaywidgets/claycolorselector.h>

G_BEGIN_DECLS


#define CLAY_TYPE_COLOR_NOTEBOOK (clay_color_notebook_get_type ())
G_DECLARE_FINAL_TYPE (ClayColorNotebook, clay_color_notebook, CLAY, COLOR_NOTEBOOK, ClayColorSelector)


GtkWidget         * clay_color_notebook_set_has_page         (ClayColorNotebook *notebook,
                                                              GType              page_type,
                                                              gboolean           has_page);

GtkWidget         * clay_color_notebook_get_notebook         (ClayColorNotebook *notebook);
GList             * clay_color_notebook_get_selectors        (ClayColorNotebook *notebook);
ClayColorSelector * clay_color_notebook_get_current_selector (ClayColorNotebook *notebook);
void                clay_color_notebook_set_format           (ClayColorNotebook *notebook,
                                                              const Babl        *format);
void                clay_color_notebook_set_simulation       (ClayColorNotebook *notebook,
                                                              ClayColorProfile  *profile,
                                                              ClayColorRenderingIntent intent,
                                                              gboolean           bpc);
void                clay_color_notebook_enable_simulation    (ClayColorNotebook *notebook,
                                                              gboolean           enabled);


G_END_DECLS

#endif /* __CLAY_COLOR_NOTEBOOK_H__ */
