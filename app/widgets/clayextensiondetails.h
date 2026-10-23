/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextensiondetails.h
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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


#define CLAY_TYPE_EXTENSION_DETAILS            (clay_extension_details_get_type ())
#define CLAY_EXTENSION_DETAILS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EXTENSION_DETAILS, ClayExtensionDetails))
#define CLAY_EXTENSION_DETAILS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EXTENSION_DETAILS, ClayExtensionDetailsClass))
#define CLAY_IS_EXTENSION_DETAILS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EXTENSION_DETAILS))
#define CLAY_IS_EXTENSION_DETAILS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EXTENSION_DETAILS))
#define CLAY_EXTENSION_DETAILS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_EXTENSION_DETAILS, ClayExtensionDetailsClass))


typedef struct _ClayExtensionDetailsClass    ClayExtensionDetailsClass;
typedef struct _ClayExtensionDetailsPrivate  ClayExtensionDetailsPrivate;

struct _ClayExtensionDetails
{
  GtkFrame                     parent_instance;

  ClayExtensionDetailsPrivate *p;
};

struct _ClayExtensionDetailsClass
{
  GtkFrameClass                parent_class;
};


GType        clay_extension_details_get_type     (void);

GtkWidget  * clay_extension_details_new          (void);

void         clay_extension_details_set          (ClayExtensionDetails *details,
                                                  ClayExtension        *extension);
