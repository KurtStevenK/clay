/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claylanguagestore.h
 * Copyright (C) 2008, 2009  Sven Neumann <sven@gimp.org>
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


enum
{
  CLAY_LANGUAGE_STORE_LABEL,
  CLAY_LANGUAGE_STORE_CODE
};


#define CLAY_TYPE_LANGUAGE_STORE            (clay_language_store_get_type ())
#define CLAY_LANGUAGE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LANGUAGE_STORE, ClayLanguageStore))
#define CLAY_LANGUAGE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LANGUAGE_STORE, ClayLanguageStoreClass))
#define CLAY_IS_LANGUAGE_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LANGUAGE_STORE))
#define CLAY_IS_LANGUAGE_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LANGUAGE_STORE))
#define CLAY_LANGUAGE_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LANGUAGE_STORE, ClayLanguageStoreClass))


typedef struct _ClayLanguageStoreClass  ClayLanguageStoreClass;

struct _ClayLanguageStoreClass
{
  GtkListStoreClass  parent_class;

  void (* add) (ClayLanguageStore *store,
                const gchar       *label,
                const gchar       *code);
};

struct _ClayLanguageStore
{
  GtkListStore     parent_instance;
};


GType          clay_language_store_get_type (void);

GtkListStore * clay_language_store_new      (void);

gboolean       clay_language_store_lookup   (ClayLanguageStore *store,
                                             const gchar       *code,
                                             GtkTreeIter       *iter);
