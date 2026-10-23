/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydatafactory.h
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

#include "clayobject.h"


typedef ClayData * (* ClayDataNewFunc)         (ClayContext     *context,
                                                const gchar     *name);
typedef ClayData * (* ClayDataGetStandardFunc) (ClayContext     *context);

typedef void       (* ClayDataForeachFunc)     (ClayDataFactory *factory,
                                                ClayData        *data,
                                                gpointer         user_data);


#define CLAY_TYPE_DATA_FACTORY (clay_data_factory_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayDataFactory,
                          clay_data_factory,
                          CLAY, DATA_FACTORY,
                          ClayObject)


struct _ClayDataFactoryClass
{
  ClayObjectClass  parent_class;

  void           (* data_init)      (ClayDataFactory *factory,
                                     ClayContext     *context);
  void           (* data_refresh)   (ClayDataFactory *factory,
                                     ClayContext     *context);
  void           (* data_save)      (ClayDataFactory *factory);

  void           (* data_cancel)    (ClayDataFactory *factory);

  ClayData     * (* data_duplicate) (ClayDataFactory  *factory,
                                     ClayData         *data);
  gboolean       (* data_delete)    (ClayDataFactory  *factory,
                                     ClayData         *data,
                                     gboolean          delete_from_disk,
                                     GError          **error);
};


void            clay_data_factory_data_init         (ClayDataFactory  *factory,
                                                     ClayContext      *context,
                                                     gboolean          no_data);
void            clay_data_factory_data_clean        (ClayDataFactory  *factory);
void            clay_data_factory_data_refresh      (ClayDataFactory  *factory,
                                                     ClayContext      *context);
void            clay_data_factory_data_save         (ClayDataFactory  *factory);
void            clay_data_factory_data_free         (ClayDataFactory  *factory);

ClayAsyncSet  * clay_data_factory_get_async_set     (ClayDataFactory  *factory);
gboolean        clay_data_factory_data_wait         (ClayDataFactory  *factory);
void            clay_data_factory_data_cancel       (ClayDataFactory  *factory);

ClayData      * clay_data_factory_get_data          (ClayDataFactory  *factory,
                                                     const gchar      *name,
                                                     const gchar      *collection,
                                                     gboolean          is_internal);

gboolean        clay_data_factory_has_data_new_func (ClayDataFactory  *factory);
ClayData      * clay_data_factory_data_new          (ClayDataFactory  *factory,
                                                     ClayContext      *context,
                                                     const gchar      *name);
ClayData      * clay_data_factory_data_get_standard (ClayDataFactory  *factory,
                                                     ClayContext      *context);

ClayData      * clay_data_factory_data_duplicate    (ClayDataFactory  *factory,
                                                     ClayData         *data);
gboolean        clay_data_factory_data_delete       (ClayDataFactory  *factory,
                                                     ClayData         *data,
                                                     gboolean          delete_from_disk,
                                                     GError          **error);

gboolean        clay_data_factory_data_save_single  (ClayDataFactory  *factory,
                                                     ClayData         *data,
                                                     GError          **error);

void            clay_data_factory_data_foreach      (ClayDataFactory  *factory,
                                                     gboolean          skip_internal,
                                                     ClayDataForeachFunc  callback,
                                                     gpointer          user_data);

Clay          * clay_data_factory_get_clay          (ClayDataFactory  *factory);
GType           clay_data_factory_get_data_type     (ClayDataFactory  *factory);
ClayContainer * clay_data_factory_get_container     (ClayDataFactory  *factory);
ClayContainer * clay_data_factory_get_container_obsolete
                                                    (ClayDataFactory  *factory);
GList         * clay_data_factory_get_data_path     (ClayDataFactory  *factory);
GList         * clay_data_factory_get_data_path_writable
                                                    (ClayDataFactory  *factory);
const GList   * clay_data_factory_get_data_path_ext (ClayDataFactory  *factory);
