/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycompressioncombobox.h
 * Copyright (C) 2019 Ell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#define CLAY_TYPE_COMPRESSION_COMBO_BOX (clay_compression_combo_box_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayCompressionComboBox,
                          clay_compression_combo_box,
                          CLAY, COMPRESSION_COMBO_BOX,
                          ClayStringComboBox)


struct _ClayCompressionComboBoxClass
{
  ClayStringComboBoxClass  parent_class;
};


GtkWidget * clay_compression_combo_box_new             (void);

void        clay_compression_combo_box_set_compression (ClayCompressionComboBox *combo_box,
                                                        const gchar             *compression);
gchar     * clay_compression_combo_box_get_compression (ClayCompressionComboBox *combo_box);
