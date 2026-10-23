/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


#define CLAY_TYPE_ITEM_LIST            (clay_item_list_get_type ())
#define CLAY_ITEM_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ITEM_LIST, ClayItemList))
#define CLAY_ITEM_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ITEM_LIST, ClayItemListClass))
#define CLAY_IS_ITEM_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ITEM_LIST))
#define CLAY_IS_ITEM_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ITEM_LIST))
#define CLAY_ITEM_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ITEM_LIST, ClayItemListClass))


typedef struct _ClayItemListClass   ClayItemListClass;
typedef struct _ClayItemListPrivate ClayItemListPrivate;

struct _ClayItemList
{
  ClayObject          parent_instance;

  ClayItemListPrivate *p;
};

struct _ClayItemListClass
{
  ClayObjectClass  parent_class;

  /*  signals  */
  void  (* empty) (ClayItemList *set);
};


GType          clay_item_list_get_type  (void);

ClayItemList  * clay_item_list_named_new    (ClayImage        *image,
                                             GType             item_type,
                                             const gchar      *name,
                                             GList            *items);

ClayItemList  * clay_item_list_pattern_new  (ClayImage        *image,
                                             GType             item_type,
                                             ClaySelectMethod  pattern_syntax,
                                             const gchar      *pattern);

GType          clay_item_list_get_item_type (ClayItemList     *set);
GList        * clay_item_list_get_items     (ClayItemList     *set,
                                             GError          **error);
gboolean       clay_item_list_is_pattern    (ClayItemList     *set,
                                             ClaySelectMethod *pattern_syntax);

void           clay_item_list_add           (ClayItemList     *set,
                                             ClayItem         *item);
