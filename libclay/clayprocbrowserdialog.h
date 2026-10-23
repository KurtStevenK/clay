/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayprocbrowserdialog.c
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

#ifndef __CLAY_PROC_BROWSER_DIALOG_H__
#define __CLAY_PROC_BROWSER_DIALOG_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_PROC_BROWSER_DIALOG (clay_proc_browser_dialog_get_type ())
G_DECLARE_FINAL_TYPE (ClayProcBrowserDialog, clay_proc_browser_dialog, CLAY, PROC_BROWSER_DIALOG, ClayDialog)


GtkWidget * clay_proc_browser_dialog_new          (const gchar  *title,
                                                   const gchar  *role,
                                                   ClayHelpFunc  help_func,
                                                   const gchar  *help_id,
                                                   ...) G_GNUC_NULL_TERMINATED;

gchar     * clay_proc_browser_dialog_get_selected (ClayProcBrowserDialog *dialog);


G_END_DECLS

#endif  /* __CLAY_PROC_BROWSER_DIALOG_H__ */
