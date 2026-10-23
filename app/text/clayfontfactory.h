/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfontfactory.h
 * Copyright (C) 2018 Michael Natterer <mitch@gimp.org>
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

#include <pango/pango.h>
#include "core/claydatafactory.h"


#define CLAY_TYPE_FONT_FACTORY            (clay_font_factory_get_type ())
#define CLAY_FONT_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FONT_FACTORY, ClayFontFactory))
#define CLAY_FONT_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FONT_FACTORY, ClayFontFactoryClass))
#define CLAY_IS_FONT_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FONT_FACTORY))
#define CLAY_IS_FONT_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FONT_FACTORY))
#define CLAY_FONT_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FONT_FACTORY, ClayFontFactoryClass))


typedef struct _ClayFontFactoryPrivate ClayFontFactoryPrivate;
typedef struct _ClayFontFactoryClass   ClayFontFactoryClass;

struct _ClayFontFactory
{
  ClayDataFactory         parent_instance;

  ClayFontFactoryPrivate *priv;
};

struct _ClayFontFactoryClass
{
  ClayDataFactoryClass  parent_class;
};


GType             clay_font_factory_get_type (void);

ClayDataFactory * clay_font_factory_new                        (Clay             *clay,
                                                                const gchar      *path_property_name);
GList           * clay_font_factory_get_custom_fonts_dirs      (ClayFontFactory  *factory);
void              clay_font_factory_get_custom_config_path     (ClayFontFactory  *factory,
                                                                gchar           **conf,
                                                                gchar           **sysconf);
GSList          * clay_font_factory_get_fonts_renaming_config  (ClayFontFactory  *factory);

PangoContext    * clay_font_factory_get_pango_context          (ClayFontFactory  *factory);
