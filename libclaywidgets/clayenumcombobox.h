/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayenumcombobox.h
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

#ifndef __CLAY_ENUM_COMBO_BOX_H__
#define __CLAY_ENUM_COMBO_BOX_H__

#include <libclaywidgets/clayintcombobox.h>

G_BEGIN_DECLS

#define CLAY_TYPE_ENUM_COMBO_BOX (clay_enum_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayEnumComboBox, clay_enum_combo_box, CLAY, ENUM_COMBO_BOX, ClayIntComboBox)

struct _ClayEnumComboBoxClass
{
  ClayIntComboBoxClass  parent_class;

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


GtkWidget * clay_enum_combo_box_new              (GType             enum_type);
GtkWidget * clay_enum_combo_box_new_with_model   (ClayEnumStore    *enum_store);

void        clay_enum_combo_box_set_icon_prefix  (ClayEnumComboBox *combo_box,
                                                  const gchar      *icon_prefix);

G_END_DECLS

#endif  /* __CLAY_ENUM_COMBO_BOX_H__ */
