/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayprogress.h
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

#if !defined (__CLAY_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clay.h> can be included directly."
#endif

#ifndef __CLAY_PROGRESS_H__
#define __CLAY_PROGRESS_H__

G_BEGIN_DECLS

/**
 * ClayProgressVtableStartFunc:
 * @message: The message to show
 * @cancelable: Whether the procedure is cancelable
 * @user_data: (closure): User data
 *
 * Starts the progress
 */
typedef void (* ClayProgressVtableStartFunc) (const gchar *message,
                                              gboolean     cancelable,
                                              gpointer     user_data);

/**
 * ClayProgressVtableEndFunc:
 * @user_data: (closure): User data
 *
 * Ends the progress
 */
typedef void (* ClayProgressVtableEndFunc) (gpointer user_data);

/**
 * ClayProgressVtableSetTextFunc:
 * @message: The new text
 * @user_data: (closure): User data
 *
 * Sets a new text on the progress.
 */
typedef void (* ClayProgressVtableSetTextFunc) (const gchar *message,
                                                gpointer     user_data);

/**
 * ClayProgressVtableSetValueFunc:
 * @percentage: The progress in percent
 * @user_data: (closure): User data
 *
 * Sets a new percentage on the progress.
 */
typedef void (* ClayProgressVtableSetValueFunc) (gdouble  percentage,
                                                 gpointer user_data);

/**
 * ClayProgressVtablePulseFunc:
 * @user_data: (closure): User data
 *
 * Makes the progress pulse
 */
typedef void (* ClayProgressVtablePulseFunc) (gpointer user_data);

/**
 * ClayProgressVtableGetWindowFunc:
 * @user_data: (closure): User data
 *
 * Returns: the handle of the window where the progress is displayed.
 */
typedef GBytes * (* ClayProgressVtableGetWindowFunc) (gpointer user_data);


typedef struct _ClayProgressVtable ClayProgressVtable;

/**
 * ClayProgressVtable:
 * @start:      starts the progress.
 * @end:        ends the progress.
 * @set_text:   sets a new text on the progress.
 * @set_value:  sets a new percentage on the progress.
 * @pulse:      makes the progress pulse.
 * @get_window_handle: returns the handle of the window where the progress is displayed.
 **/
struct _ClayProgressVtable
{
  ClayProgressVtableStartFunc     start;
  ClayProgressVtableEndFunc       end;
  ClayProgressVtableSetTextFunc   set_text;
  ClayProgressVtableSetValueFunc  set_value;
  ClayProgressVtablePulseFunc     pulse;
  ClayProgressVtableGetWindowFunc get_window_handle;

  /* Padding for future expansion. Must be initialized with NULL! */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


const gchar * clay_progress_install_vtable  (const ClayProgressVtable *vtable,
                                             gpointer                  user_data,
                                             GDestroyNotify            user_data_destroy);
void          clay_progress_uninstall       (const gchar              *progress_callback);

gboolean      clay_progress_init            (const gchar              *message);
gboolean      clay_progress_init_printf     (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      clay_progress_set_text_printf (const gchar              *format,
                                             ...) G_GNUC_PRINTF (1, 2);

gboolean      clay_progress_update          (gdouble                   percentage);


G_END_DECLS

#endif /* __CLAY_PROGRESS_H__ */
