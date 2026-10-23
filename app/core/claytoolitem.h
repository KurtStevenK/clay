/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolitem.h
 * Copyright (C) 2020 Ell
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

#include "clayviewable.h"


#define CLAY_TYPE_TOOL_ITEM            (clay_tool_item_get_type ())
#define CLAY_TOOL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_ITEM, ClayToolItem))
#define CLAY_TOOL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_ITEM, ClayToolItemClass))
#define CLAY_IS_TOOL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_ITEM))
#define CLAY_IS_TOOL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_ITEM))
#define CLAY_TOOL_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_ITEM, ClayToolItemClass))


typedef struct _ClayToolItemPrivate ClayToolItemPrivate;
typedef struct _ClayToolItemClass   ClayToolItemClass;

struct _ClayToolItem
{
  ClayViewable         parent_instance;

  ClayToolItemPrivate *priv;
};

struct _ClayToolItemClass
{
  ClayViewableClass  parent_class;

  /*  signals  */
  void (* visible_changed) (ClayToolItem *tool_item);
  void (* shown_changed)   (ClayToolItem *tool_item);
};


GType      clay_tool_item_get_type      (void);

void       clay_tool_item_set_visible   (ClayToolItem *tool_item,
                                         gboolean      visible);
gboolean   clay_tool_item_get_visible   (ClayToolItem *tool_item);

gboolean   clay_tool_item_get_shown     (ClayToolItem *tool_item);


/*  protected  */

void       clay_tool_item_shown_changed (ClayToolItem *tool_item);
