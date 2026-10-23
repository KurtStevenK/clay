/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydataloaderfactory.h
 * Copyright (C) 2001-2018 Michael Natterer <mitch@gimp.org>
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

#include "claydatafactory.h"


typedef GList * (* ClayDataLoadFunc) (ClayContext   *context,
                                      GFile         *file,
                                      GInputStream  *input,
                                      GError       **error);


#define CLAY_TYPE_DATA_LOADER_FACTORY            (clay_data_loader_factory_get_type ())
#define CLAY_DATA_LOADER_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DATA_LOADER_FACTORY, ClayDataLoaderFactory))
#define CLAY_DATA_LOADER_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DATA_LOADER_FACTORY, ClayDataLoaderFactoryClass))
#define CLAY_IS_DATA_LOADER_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DATA_LOADER_FACTORY))
#define CLAY_IS_DATA_LOADER_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DATA_LOADER_FACTORY))
#define CLAY_DATA_LOADER_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DATA_LOADER_FACTORY, ClayDataLoaderFactoryClass))


typedef struct _ClayDataLoaderFactoryPrivate ClayDataLoaderFactoryPrivate;
typedef struct _ClayDataLoaderFactoryClass   ClayDataLoaderFactoryClass;

struct _ClayDataLoaderFactory
{
  ClayDataFactory               parent_instance;

  ClayDataLoaderFactoryPrivate *priv;
};

struct _ClayDataLoaderFactoryClass
{
  ClayDataFactoryClass  parent_class;
};


GType             clay_data_loader_factory_get_type     (void);

ClayDataFactory * clay_data_loader_factory_new          (Clay                    *clay,
                                                         GType                    data_type,
                                                         const gchar             *path_property_name,
                                                         const gchar             *writable_property_name,
                                                         const gchar             *ext_property_name,
                                                         ClayDataNewFunc          new_func,
                                                         ClayDataGetStandardFunc  get_standard_func);

void              clay_data_loader_factory_add_loader   (ClayDataFactory         *factory,
                                                         const gchar             *name,
                                                         ClayDataLoadFunc         load_func,
                                                         const gchar             *extension,
                                                         gboolean                 writable);
void              clay_data_loader_factory_add_fallback (ClayDataFactory         *factory,
                                                         const gchar             *name,
                                                         ClayDataLoadFunc         load_func);
