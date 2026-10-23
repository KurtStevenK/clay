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


#define CLAY_TYPE_AUX_ITEM            (clay_aux_item_get_type ())
#define CLAY_AUX_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_AUX_ITEM, ClayAuxItem))
#define CLAY_AUX_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_AUX_ITEM, ClayAuxItemClass))
#define CLAY_IS_AUX_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_AUX_ITEM))
#define CLAY_IS_AUX_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_AUX_ITEM))
#define CLAY_AUX_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_AUX_ITEM, ClayAuxItemClass))


typedef struct _ClayAuxItemPrivate ClayAuxItemPrivate;
typedef struct _ClayAuxItemClass   ClayAuxItemClass;

struct _ClayAuxItem
{
  GObject             parent_instance;

  ClayAuxItemPrivate *priv;
};

struct _ClayAuxItemClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void (* removed) (ClayAuxItem *aux_item);
};


GType     clay_aux_item_get_type (void);

guint32   clay_aux_item_get_id   (ClayAuxItem *aux_item);

void      clay_aux_item_removed  (ClayAuxItem *aux_item);
