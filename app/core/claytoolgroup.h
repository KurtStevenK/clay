/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolgroup.h
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

#include "claytoolitem.h"


#define CLAY_TYPE_TOOL_GROUP            (clay_tool_group_get_type ())
#define CLAY_TOOL_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_GROUP, ClayToolGroup))
#define CLAY_TOOL_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_GROUP, ClayToolGroupClass))
#define CLAY_IS_TOOL_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_GROUP))
#define CLAY_IS_TOOL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_GROUP))
#define CLAY_TOOL_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_GROUP, ClayToolGroupClass))


typedef struct _ClayToolGroupPrivate ClayToolGroupPrivate;
typedef struct _ClayToolGroupClass   ClayToolGroupClass;

struct _ClayToolGroup
{
  ClayToolItem          parent_instance;

  ClayToolGroupPrivate *priv;
};

struct _ClayToolGroupClass
{
  ClayToolItemClass  parent_class;

  /*  signals  */
  void (* active_tool_changed) (ClayToolGroup *tool_group);
};


GType           clay_tool_group_get_type             (void);

ClayToolGroup * clay_tool_group_new                  (void);

void            clay_tool_group_set_active_tool      (ClayToolGroup *tool_group,
                                                      const gchar   *tool_name);
const gchar   * clay_tool_group_get_active_tool      (ClayToolGroup *tool_group);

void            clay_tool_group_set_active_tool_info (ClayToolGroup *tool_group,
                                                      ClayToolInfo  *tool_info);
ClayToolInfo  * clay_tool_group_get_active_tool_info (ClayToolGroup *tool_group);
