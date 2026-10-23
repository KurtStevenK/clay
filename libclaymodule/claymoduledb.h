/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_MODULE_H_INSIDE__) && !defined (CLAY_MODULE_COMPILATION)
#error "Only <libclaymodule/claymodule.h> can be included directly."
#endif

#ifndef __CLAY_MODULE_DB_H__
#define __CLAY_MODULE_DB_H__

G_BEGIN_DECLS

#define CLAY_TYPE_MODULE_DB (clay_module_db_get_type ())
G_DECLARE_FINAL_TYPE (ClayModuleDB,
                      clay_module_db,
                      CLAY, MODULE_DB,
                      GObject)


ClayModuleDB * clay_module_db_new              (gboolean      verbose);

void           clay_module_db_set_verbose      (ClayModuleDB *db,
                                                gboolean      verbose);
gboolean       clay_module_db_get_verbose      (ClayModuleDB *db);

void           clay_module_db_set_load_inhibit (ClayModuleDB *db,
                                                const gchar  *load_inhibit);
const gchar  * clay_module_db_get_load_inhibit (ClayModuleDB *db);

void           clay_module_db_load             (ClayModuleDB *db,
                                                const gchar  *module_path);
void           clay_module_db_refresh          (ClayModuleDB *db,
                                                const gchar  *module_path);


G_END_DECLS

#endif  /* __CLAY_MODULE_DB_H__ */
