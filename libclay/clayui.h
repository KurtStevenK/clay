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

#ifndef __CLAY_UI_H__
#define __CLAY_UI_H__

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libclay/clay.h>

#include <libclaywidgets/claywidgets.h>

#define __CLAY_UI_H_INSIDE__

#include <libclay/clayuitypes.h>

#include <libclay/clayaspectpreview.h>
#include <libclay/claybrushchooser.h>
#include <libclay/claydrawablechooser.h>
#include <libclay/claydrawablepreview.h>
#include <libclay/clayfontchooser.h>
#include <libclay/claygradientchooser.h>
#include <libclay/clayimagechooser.h>
#include <libclay/clayimagecombobox.h>
#include <libclay/clayitemchooser.h>
#include <libclay/clayitemcombobox.h>
#include <libclay/claypalettechooser.h>
#include <libclay/claypatternchooser.h>
#include <libclay/clayprocbrowserdialog.h>
#include <libclay/clayproceduredialog.h>
#include <libclay/clayexportproceduredialog.h>
#include <libclay/clayvectorloadproceduredialog.h>
#include <libclay/clayprocview.h>
#include <libclay/clayprogressbar.h>
#include <libclay/claypropwidgets.h>
#include <libclay/clayresourcechooser.h>
#include <libclay/clayzoompreview.h>

#undef __CLAY_UI_H_INSIDE__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


void        clay_ui_init                          (const gchar *prog_name);

void        clay_window_set_transient             (GtkWindow   *window);
void        clay_window_set_transient_for         (GtkWindow   *window,
                                                   GBytes      *handle);
void        clay_window_set_transient_for_display (GtkWindow   *window,
                                                   ClayDisplay *display);


G_END_DECLS

#endif /* __CLAY_UI_H__ */
