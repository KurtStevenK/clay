/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayiconpicker.h
 * Copyright (C) 2011 Michael Natterer <mitch@gimp.org>
 *               2012 Daniel Sabo
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


#define CLAY_TYPE_ICON_PICKER            (clay_icon_picker_get_type ())
#define CLAY_ICON_PICKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ICON_PICKER, ClayIconPicker))
#define CLAY_ICON_PICKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ICON_PICKER, ClayIconPickerClass))
#define CLAY_IS_ICON_PICKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ICON_PICKER))
#define CLAY_IS_ICON_PICKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ICON_PICKER))
#define CLAY_ICON_PICKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ICON_PICKER, ClayIconPickerClass))


typedef struct _ClayIconPickerClass ClayIconPickerClass;

struct _ClayIconPicker
{
  GtkBox  parent_instance;
};

struct _ClayIconPickerClass
{
  GtkBoxClass   parent_class;
};


GType          clay_icon_picker_get_type       (void);

GtkWidget   * clay_icon_picker_new             (Clay           *clay);

const gchar * clay_icon_picker_get_icon_name   (ClayIconPicker *picker);
void          clay_icon_picker_set_icon_name   (ClayIconPicker *picker,
                                                const gchar    *icon_name);

GdkPixbuf   * clay_icon_picker_get_icon_pixbuf (ClayIconPicker *picker);
void          clay_icon_picker_set_icon_pixbuf (ClayIconPicker *picker,
                                                GdkPixbuf      *value);
