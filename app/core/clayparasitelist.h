/* parasitelist.h: Copyright 1998 Jay Cox <jaycox@gimp.org>
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


#define CLAY_TYPE_PARASITE_LIST            (clay_parasite_list_get_type ())
#define CLAY_PARASITE_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PARASITE_LIST, ClayParasiteList))
#define CLAY_PARASITE_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PARASITE_LIST, ClayParasiteListClass))
#define CLAY_IS_PARASITE_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PARASITE_LIST))
#define CLAY_IS_PARASITE_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PARASITE_LIST))
#define CLAY_PARASITE_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PARASITE_LIST, ClayParasiteListClass))


typedef struct _ClayParasiteListClass ClayParasiteListClass;

struct _ClayParasiteList
{
  ClayObject  object;

  GHashTable *table;
};

struct _ClayParasiteListClass
{
  ClayObjectClass parent_class;

  void (* add)    (ClayParasiteList *list,
                   ClayParasite     *parasite);
  void (* remove) (ClayParasiteList *list,
                   ClayParasite     *parasite);
};


GType                clay_parasite_list_get_type (void);

ClayParasiteList   * clay_parasite_list_new      (void);
ClayParasiteList   * clay_parasite_list_copy     (ClayParasiteList       *list);
void                 clay_parasite_list_add      (ClayParasiteList       *list,
                                                  const ClayParasite     *parasite);
void                 clay_parasite_list_remove   (ClayParasiteList       *list,
                                                  const gchar            *name);
gint                 clay_parasite_list_length   (ClayParasiteList       *list);
gint                 clay_parasite_list_persistent_length (ClayParasiteList *list);
void                 clay_parasite_list_foreach  (ClayParasiteList       *list,
                                                  GHFunc                  function,
                                                  gpointer                user_data);
const ClayParasite * clay_parasite_list_find     (ClayParasiteList       *list,
                                                  const gchar            *name);
