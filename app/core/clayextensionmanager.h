/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayextensionmanager.h
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

#include "clayobject.h"


#define CLAY_TYPE_EXTENSION_MANAGER            (clay_extension_manager_get_type ())
#define CLAY_EXTENSION_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EXTENSION_MANAGER, ClayExtensionManager))
#define CLAY_EXTENSION_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EXTENSION_MANAGER, ClayExtensionManagerClass))
#define CLAY_IS_EXTENSION_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EXTENSION_MANAGER))
#define CLAY_IS_EXTENSION_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EXTENSION_MANAGER))
#define CLAY_EXTENSION_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_EXTENSION_MANAGER, ClayExtensionManagerClass))


typedef struct _ClayExtensionManagerClass   ClayExtensionManagerClass;
typedef struct _ClayExtensionManagerPrivate ClayExtensionManagerPrivate;

typedef struct _ClayStoreEntry ClayStoreEntry;

struct _ClayStoreEntry
{
  gchar    *id;
  gchar    *name;
  gchar    *name_de;
  gchar    *summary;
  gchar    *summary_de;
  gchar    *description;
  gchar    *category;
  gchar    *tags;
  gchar    *version;
  gchar    *author;
  gchar    *icon_name;
  gchar    *download_url;
  gchar    *local_gex_path;
  gchar    *menu_hint;
  gdouble   rating;
  gint      downloads;
  gboolean  featured;
};

struct _ClayExtensionManager
{
  ClayObject                   parent_instance;

  ClayExtensionManagerPrivate *p;
};

struct _ClayExtensionManagerClass
{
  ClayObjectClass      parent_class;

  void              (* extension_installed) (ClayExtensionManager *manager,
                                             ClayExtension        *extension,
                                             gboolean              is_system_ext);
  void              (* extension_removed)   (ClayExtensionManager *manager,
                                             gchar                *extension_id);
  void              (* store_updated)       (ClayExtensionManager *manager);
};


GType                  clay_extension_manager_get_type              (void);

ClayExtensionManager * clay_extension_manager_new                   (Clay                 *clay);

void                   clay_extension_manager_initialize            (ClayExtensionManager *manager);
void                   clay_extension_manager_exit                  (ClayExtensionManager *manager);

const GList          * clay_extension_manager_get_system_extensions (ClayExtensionManager *manager);
const GList          * clay_extension_manager_get_user_extensions   (ClayExtensionManager *manager);

gboolean               clay_extension_manager_is_running            (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension);
gboolean               clay_extension_manager_can_run               (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension);
gboolean               clay_extension_manager_is_removed            (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension);

gboolean               clay_extension_manager_install               (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension,
                                                                     GError              **error);
gboolean               clay_extension_manager_remove                (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension,
                                                                     GError              **error);
gboolean               clay_extension_manager_undo_remove           (ClayExtensionManager *manager,
                                                                     ClayExtension        *extension,
                                                                     GError              **error);

ClayExtension        * clay_extension_manager_lookup                (ClayExtensionManager *manager,
                                                                     const gchar          *id);

const GList          * clay_extension_manager_get_store_catalog     (ClayExtensionManager *manager);
GList                * clay_extension_manager_search_store          (ClayExtensionManager *manager,
                                                                     const gchar          *query,
                                                                     const gchar          *category);
ClayStoreEntry       * clay_extension_manager_find_store_entry      (ClayExtensionManager *manager,
                                                                     const gchar          *id);
void                   clay_extension_manager_refresh_store         (ClayExtensionManager *manager);

gboolean               clay_extension_manager_install_from_directory (ClayExtensionManager *manager,
                                                                      const gchar          *path,
                                                                      GError              **error);
gboolean               clay_extension_manager_install_from_store     (ClayExtensionManager *manager,
                                                                      const gchar          *entry_id,
                                                                      GError              **error);

void                   clay_store_entry_free                        (ClayStoreEntry       *entry);
