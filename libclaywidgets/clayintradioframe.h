/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayintradioframe.h
 * Copyright (C) 2022 Jehan
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

#include <libclaywidgets/clayframe.h>

#ifndef __CLAY_INT_RADIO_FRAME_H__
#define __CLAY_INT_RADIO_FRAME_H__

G_BEGIN_DECLS


#define CLAY_TYPE_INT_RADIO_FRAME (clay_int_radio_frame_get_type ())
G_DECLARE_FINAL_TYPE (ClayIntRadioFrame, clay_int_radio_frame, CLAY, INT_RADIO_FRAME, ClayFrame)


/**
 * ClayIntRadioFrameSensitivityFunc:
 * @value: the value associated with a radio button.
 * @user_data: the data associated with a radio button.
 * @new_value: the value to check instead if the function returns %FALSE.
 * @data: (closure): the data set in clay_int_radio_frame_set_sensitivity()
 *
 * Signature for a function called on each radio button value and data,
 * each time the %ClayIntRadioFrame is drawn, to make some radio button
 * insensitive.
 * If the function returns %FALSE, it usually means that the value is
 * not a valid choice in current situation. In this case, you might want
 * to toggle instead another value automatically. Set @new_value to the
 * value to toggle. If you leave this untouched, the radio button will
 * stay toggled despite being insensitive. This is up to you to decide
 * whether this is meaningful.
 *
 * Returns: %TRUE if the button stays sensitive, %FALSE otherwise.
 */
typedef  gboolean (* ClayIntRadioFrameSensitivityFunc) (gint      value,
                                                        gpointer  user_data,
                                                        gint     *new_value,
                                                        gpointer  data);



GtkWidget   * clay_int_radio_frame_new_from_store  (const gchar       *title,
                                                    ClayIntStore      *store);
GtkWidget   * clay_int_radio_frame_new             (const gchar       *first_label,
                                                    gint               first_value,
                                                    ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * clay_int_radio_frame_new_valist      (const gchar       *first_label,
                                                    gint               first_value,
                                                    va_list            values);

GtkWidget   * clay_int_radio_frame_new_array       (const gchar       *labels[]);

void          clay_int_radio_frame_prepend         (ClayIntRadioFrame *radio_frame,
                                                    ...);
void          clay_int_radio_frame_append          (ClayIntRadioFrame *radio_frame,
                                                    ...);

void          clay_int_radio_frame_set_title       (ClayIntRadioFrame *frame,
                                                    const gchar       *title,
                                                    gboolean           with_mnemonic);

gboolean      clay_int_radio_frame_set_active      (ClayIntRadioFrame *radio_frame,
                                                    gint               value);
gint          clay_int_radio_frame_get_active      (ClayIntRadioFrame *radio_frame);

gboolean
      clay_int_radio_frame_set_active_by_user_data (ClayIntRadioFrame *radio_frame,
                                                    gpointer           user_data);
gboolean
      clay_int_radio_frame_get_active_user_data    (ClayIntRadioFrame *radio_frame,
                                                    gpointer          *user_data);

void          clay_int_radio_frame_set_sensitivity (ClayIntRadioFrame *radio_frame,
                                                    ClayIntRadioFrameSensitivityFunc  func,
                                                    gpointer           data,
                                                    GDestroyNotify     destroy);


G_END_DECLS

#endif  /* __CLAY_INT_RADIO_FRAME_H__ */
