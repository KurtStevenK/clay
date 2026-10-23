/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayscalecombobox.h
 * Copyright (C) 2004, 2008  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_SCALE_COMBO_BOX            (clay_scale_combo_box_get_type ())
#define CLAY_SCALE_COMBO_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SCALE_COMBO_BOX, ClayScaleComboBox))
#define CLAY_SCALE_COMBO_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SCALE_COMBO_BOX, ClayScaleComboBoxClass))
#define CLAY_IS_SCALE_COMBO_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SCALE_COMBO_BOX))
#define CLAY_IS_SCALE_COMBO_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SCALE_COMBO_BOX))
#define CLAY_SCALE_COMBO_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SCALE_COMBO_BOX, ClayScaleComboBoxClass))


typedef struct _ClayScaleComboBoxClass  ClayScaleComboBoxClass;

struct _ClayScaleComboBox
{
  GtkComboBox  parent_instance;

  gdouble      scale;
  GtkTreePath *last_path;
  GList       *mru;
};

struct _ClayScaleComboBoxClass
{
  GtkComboBoxClass  parent_class;

  void (* entry_activated) (ClayScaleComboBox *combo_box);
};


GType       clay_scale_combo_box_get_type   (void);

GtkWidget * clay_scale_combo_box_new        (void);
void        clay_scale_combo_box_set_scale  (ClayScaleComboBox *combo_box,
                                             gdouble            scale);
gdouble     clay_scale_combo_box_get_scale  (ClayScaleComboBox *combo_box);
