/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydeviceinfoeditor.h
 * Copyright (C) 2010 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_DEVICE_INFO_EDITOR (clay_device_info_editor_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDeviceInfoEditor,
                          clay_device_info_editor,
                          CLAY, DEVICE_INFO_EDITOR,
                          GtkBox)


struct _ClayDeviceInfoEditorClass
{
  GtkBoxClass  parent_class;
};


GtkWidget * clay_device_info_editor_new (ClayDeviceInfo *info);
