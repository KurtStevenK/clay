/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylanguagecombobox.h
 * Copyright (C) 2009  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_LANGUAGE_COMBO_BOX            (clay_language_combo_box_get_type ())
#define CLAY_LANGUAGE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LANGUAGE_COMBO_BOX, ClayLanguageComboBox))
#define CLAY_LANGUAGE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LANGUAGE_COMBO_BOX, ClayLanguageComboBoxClass))
#define CLAY_IS_LANGUAGE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LANGUAGE_COMBO_BOX))
#define CLAY_IS_LANGUAGE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LANGUAGE_COMBO_BOX))
#define CLAY_LANGUAGE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LANGUAGE_COMBO_BOX, ClayLanguageComboBoxClass))


typedef struct _ClayLanguageComboBoxClass  ClayLanguageComboBoxClass;

struct _ClayLanguageComboBoxClass
{
  GtkComboBoxClass  parent_class;
};


GType       clay_language_combo_box_get_type (void);

GtkWidget * clay_language_combo_box_new      (gboolean              manual_l18n,
                                              const gchar          *empty_label);

gchar     * clay_language_combo_box_get_code (ClayLanguageComboBox *combo);
gboolean    clay_language_combo_box_set_code (ClayLanguageComboBox *combo,
                                              const gchar          *code);
