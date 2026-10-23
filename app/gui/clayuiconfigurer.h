/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayuiconfigurer.h
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include "core/clayobject.h"


#define CLAY_TYPE_UI_CONFIGURER              (clay_ui_configurer_get_type ())
#define CLAY_UI_CONFIGURER(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_UI_CONFIGURER, ClayUIConfigurer))
#define CLAY_UI_CONFIGURER_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CLAY_TYPE_UI_CONFIGURER, ClayUIConfigurerClass))
#define CLAY_IS_UI_CONFIGURER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_UI_CONFIGURER))
#define CLAY_IS_UI_CONFIGURER_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CLAY_TYPE_UI_CONFIGURER))
#define CLAY_UI_CONFIGURER_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), CLAY_TYPE_UI_CONFIGURER, ClayUIConfigurerClass))


typedef struct _ClayUIConfigurerClass   ClayUIConfigurerClass;
typedef struct _ClayUIConfigurerPrivate ClayUIConfigurerPrivate;

struct _ClayUIConfigurer
{
  ClayObject parent_instance;

  ClayUIConfigurerPrivate *p;
};

struct _ClayUIConfigurerClass
{
  ClayObjectClass parent_class;
};


GType   clay_ui_configurer_get_type  (void);

void    clay_ui_configurer_configure (ClayUIConfigurer *ui_configurer,
                                      gboolean          single_window_mode);
