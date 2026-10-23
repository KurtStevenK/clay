/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claystringcombobox.h
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#ifndef __CLAY_STRING_COMBO_BOX_H__
#define __CLAY_STRING_COMBO_BOX_H__

G_BEGIN_DECLS


/**
 * ClayStringSensitivityFunc:
 * @id: the string value from the column @id_column as passed to [ctor@StringComboBox.new].
 * @data: (closure): the data passed in [method@StringComboBox.set_sensitivity].
 */
typedef  gboolean (* ClayStringSensitivityFunc) (const gchar *id,
                                                 gpointer     data);


#define CLAY_TYPE_STRING_COMBO_BOX (clay_string_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayStringComboBox, clay_string_combo_box, CLAY, STRING_COMBO_BOX, GtkComboBox)

struct _ClayStringComboBoxClass
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


GtkWidget * clay_string_combo_box_new             (GtkTreeModel              *model,
                                                   gint                       id_column,
                                                   gint                       label_column);
gboolean    clay_string_combo_box_set_active      (ClayStringComboBox        *combo_box,
                                                   const gchar               *id);
gchar     * clay_string_combo_box_get_active      (ClayStringComboBox        *combo_box);

void        clay_string_combo_box_set_sensitivity (ClayStringComboBox        *combo_box,
                                                   ClayStringSensitivityFunc  func,
                                                   gpointer                   data,
                                                   GDestroyNotify             destroy);

G_END_DECLS

#endif  /* __CLAY_STRING_COMBO_BOX_H__ */
