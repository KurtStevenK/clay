/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextensionlist.h
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


#define CLAY_TYPE_EXTENSION_LIST            (clay_extension_list_get_type ())
#define CLAY_EXTENSION_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EXTENSION_LIST, ClayExtensionList))
#define CLAY_EXTENSION_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EXTENSION_LIST, ClayExtensionListClass))
#define CLAY_IS_EXTENSION_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EXTENSION_LIST))
#define CLAY_IS_EXTENSION_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EXTENSION_LIST))
#define CLAY_EXTENSION_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_EXTENSION_LIST, ClayExtensionListClass))


typedef struct _ClayExtensionListClass    ClayExtensionListClass;
typedef struct _ClayExtensionListPrivate  ClayExtensionListPrivate;

struct _ClayExtensionList
{
  GtkListBox                parent_instance;

  ClayExtensionListPrivate *p;
};

struct _ClayExtensionListClass
{
  GtkListBoxClass           parent_class;

  void (* extension_activated)  (ClayExtensionList *list,
                                 ClayExtension     *extension);
  void (* store_entry_activated)(ClayExtensionList *list,
                                 ClayStoreEntry    *entry);
  void (* install_requested)    (ClayExtensionList *list,
                                 const gchar       *entry_id);
};


GType        clay_extension_list_get_type     (void);

GtkWidget  * clay_extension_list_new          (ClayExtensionManager *manager);

void         clay_extension_list_show_system  (ClayExtensionList *list);
void         clay_extension_list_show_user    (ClayExtensionList *list);
void         clay_extension_list_show_search  (ClayExtensionList *list,
                                               const gchar       *search_terms);
void         clay_extension_list_show_store   (ClayExtensionList *list,
                                               const gchar       *search_terms,
                                               const gchar       *category);
