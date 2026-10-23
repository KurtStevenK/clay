/* LIBCLAY - The Clay Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydialog.h
 * Copyright (C) 2000-2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_DIALOG_H__
#define __CLAY_DIALOG_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define CLAY_TYPE_DIALOG (clay_dialog_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDialog, clay_dialog, CLAY, DIALOG, GtkDialog)

struct _ClayDialogClass
{
  GtkDialogClass  parent_class;

  /* Padding for future expansion */
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


GtkWidget * clay_dialog_new                (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            ClayHelpFunc    help_func,
                                            const gchar    *help_id,
                                            ...) G_GNUC_NULL_TERMINATED;

GtkWidget * clay_dialog_new_valist         (const gchar    *title,
                                            const gchar    *role,
                                            GtkWidget      *parent,
                                            GtkDialogFlags  flags,
                                            ClayHelpFunc    help_func,
                                            const gchar    *help_id,
                                            va_list         args);

GtkWidget * clay_dialog_add_button         (ClayDialog     *dialog,
                                            const gchar    *button_text,
                                            gint            response_id);
void        clay_dialog_add_buttons        (ClayDialog     *dialog,
                                            ...) G_GNUC_NULL_TERMINATED;
void        clay_dialog_add_buttons_valist (ClayDialog     *dialog,
                                            va_list         args);

gint        clay_dialog_run                (ClayDialog     *dialog);

void        clay_dialog_set_alternative_button_order_from_array
                                           (ClayDialog     *dialog,
                                            gint            n_buttons,
                                            gint           *order);

GBytes    * clay_dialog_get_native_handle  (ClayDialog     *dialog);

/*  for internal use only!  */
void        clay_dialogs_show_help_button  (gboolean        show);

/* clay_dialog_set_alternative_button_order() doesn't need a dedicated
 * wrapper function because anyway it won't be introspectable.
 * GObject-Introspection bindings will have to use
 * clay_dialog_set_alternative_button_order_from_array().
 */
#define clay_dialog_set_alternative_button_order(d,f...) \
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS;                      \
  gtk_dialog_set_alternative_button_order(d,f);          \
  G_GNUC_END_IGNORE_DEPRECATIONS;


G_END_DECLS

#endif /* __CLAY_DIALOG_H__ */
