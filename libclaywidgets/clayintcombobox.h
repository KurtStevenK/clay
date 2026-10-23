/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayintcombobox.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_INT_COMBO_BOX_H__
#define __CLAY_INT_COMBO_BOX_H__

G_BEGIN_DECLS


#define CLAY_TYPE_INT_COMBO_BOX (clay_int_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayIntComboBox, clay_int_combo_box, CLAY, INT_COMBO_BOX, GtkComboBox)


struct _ClayIntComboBoxClass
{
  GtkComboBoxClass  parent_class;

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


/**
 * ClayIntSensitivityFunc:
 * @value:
 * @data: (closure):
 */
typedef  gboolean (* ClayIntSensitivityFunc) (gint      value,
                                              gpointer  data);


GtkWidget   * clay_int_combo_box_new             (const gchar     *first_label,
                                                  gint             first_value,
                                                  ...) G_GNUC_NULL_TERMINATED;
GtkWidget   * clay_int_combo_box_new_valist      (const gchar     *first_label,
                                                  gint             first_value,
                                                  va_list          values);

GtkWidget   * clay_int_combo_box_new_array       (gint             n_values,
                                                  const gchar     *labels[]);

void          clay_int_combo_box_prepend         (ClayIntComboBox *combo_box,
                                                  ...);
void          clay_int_combo_box_append          (ClayIntComboBox *combo_box,
                                                  ...);

gboolean      clay_int_combo_box_set_active      (ClayIntComboBox *combo_box,
                                                  gint             value);
gboolean      clay_int_combo_box_get_active      (ClayIntComboBox *combo_box,
                                                  gint            *value);

gboolean
      clay_int_combo_box_set_active_by_user_data (ClayIntComboBox *combo_box,
                                                  gpointer         user_data);
gboolean
      clay_int_combo_box_get_active_user_data    (ClayIntComboBox *combo_box,
                                                  gpointer        *user_data);

gulong        clay_int_combo_box_connect         (ClayIntComboBox *combo_box,
                                                  gint             value,
                                                  GCallback        callback,
                                                  gpointer         data,
                                                  GDestroyNotify   data_destroy);

void          clay_int_combo_box_set_label       (ClayIntComboBox *combo_box,
                                                  const gchar     *label);
const gchar * clay_int_combo_box_get_label       (ClayIntComboBox *combo_box);

void          clay_int_combo_box_set_layout      (ClayIntComboBox       *combo_box,
                                                  ClayIntComboBoxLayout  layout);
ClayIntComboBoxLayout
              clay_int_combo_box_get_layout      (ClayIntComboBox       *combo_box);

void          clay_int_combo_box_set_sensitivity (ClayIntComboBox        *combo_box,
                                                  ClayIntSensitivityFunc  func,
                                                  gpointer                data,
                                                  GDestroyNotify          destroy);


G_END_DECLS

#endif  /* __CLAY_INT_COMBO_BOX_H__ */
