/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayprogressbar.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_PROGRESS_BAR_H__
#define __CLAY_PROGRESS_BAR_H__

G_BEGIN_DECLS


#define CLAY_TYPE_PROGRESS_BAR (clay_progress_bar_get_type ())
G_DECLARE_FINAL_TYPE (ClayProgressBar, clay_progress_bar, CLAY, PROGRESS_BAR, GtkProgressBar)


GtkWidget * clay_progress_bar_new  (void);


G_END_DECLS

#endif /* __CLAY_PROGRESS_BAR_H__ */
