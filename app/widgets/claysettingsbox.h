/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysettingsbox.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_SETTINGS_BOX            (clay_settings_box_get_type ())
#define CLAY_SETTINGS_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SETTINGS_BOX, ClaySettingsBox))
#define CLAY_SETTINGS_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SETTINGS_BOX, ClaySettingsBoxClass))
#define CLAY_IS_SETTINGS_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SETTINGS_BOX))
#define CLAY_IS_SETTINGS_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SETTINGS_BOX))
#define CLAY_SETTINGS_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SETTINGS_BOX, ClaySettingsBoxClass))


typedef struct _ClaySettingsBoxClass ClaySettingsBoxClass;

struct _ClaySettingsBox
{
  GtkBox  parent_instance;
};

struct _ClaySettingsBoxClass
{
  GtkBoxClass  parent_class;

  /*  signals  */
  void (* import)            (ClaySettingsBox      *box,
                              GFile                *file);
  void (* export)            (ClaySettingsBox      *box,
                              GFile                *file);
  void (* selected)          (ClaySettingsBox      *box,
                              GObject              *config);
};


GType       clay_settings_box_get_type    (void);

GtkWidget * clay_settings_box_new         (Clay            *clay,
                                           GObject         *config,
                                           ClayContainer   *container,
                                           const gchar     *import_dialog_title,
                                           const gchar     *export_dialog_title,
                                           const gchar     *file_dialog_help_id,
                                           GFile           *default_folder,
                                           GFile           *last_file);

GtkWidget * clay_settings_box_get_combo   (ClaySettingsBox *box);

void        clay_settings_box_add_current (ClaySettingsBox *box,
                                           gint             max_recent);

gboolean    clay_settings_box_get_legacy  (ClaySettingsBox *box);
void        clay_settings_box_show_legacy (ClaySettingsBox *box);

void        clay_settings_box_unset       (ClaySettingsBox *box);
