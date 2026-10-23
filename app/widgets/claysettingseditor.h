/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysettingseditor.h
 * Copyright (C) 2008-2011 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_SETTINGS_EDITOR            (clay_settings_editor_get_type ())
#define CLAY_SETTINGS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SETTINGS_EDITOR, ClaySettingsEditor))
#define CLAY_SETTINGS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SETTINGS_EDITOR, ClaySettingsEditorClass))
#define CLAY_IS_SETTINGS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SETTINGS_EDITOR))
#define CLAY_IS_SETTINGS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SETTINGS_EDITOR))
#define CLAY_SETTINGS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SETTINGS_EDITOR, ClaySettingsEditorClass))


typedef struct _ClaySettingsEditorClass ClaySettingsEditorClass;

struct _ClaySettingsEditor
{
  GtkBox  parent_instance;
};

struct _ClaySettingsEditorClass
{
  GtkBoxClass  parent_class;
};


GType       clay_settings_editor_get_type (void);

GtkWidget * clay_settings_editor_new      (Clay          *clay,
                                           GObject       *config,
                                           ClayContainer *container);
