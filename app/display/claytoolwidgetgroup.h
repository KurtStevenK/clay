/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolwidgetgroup.h
 * Copyright (C) 2018 Ell
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

#include "claytoolwidget.h"


#define CLAY_TYPE_TOOL_WIDGET_GROUP            (clay_tool_widget_group_get_type ())
#define CLAY_TOOL_WIDGET_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_WIDGET_GROUP, ClayToolWidgetGroup))
#define CLAY_TOOL_WIDGET_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_WIDGET_GROUP, ClayToolWidgetGroupClass))
#define CLAY_IS_TOOL_WIDGET_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_WIDGET_GROUP))
#define CLAY_IS_TOOL_WIDGET_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_WIDGET_GROUP))
#define CLAY_TOOL_WIDGET_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_WIDGET_GROUP, ClayToolWidgetGroupClass))


typedef struct _ClayToolWidgetGroupPrivate ClayToolWidgetGroupPrivate;
typedef struct _ClayToolWidgetGroupClass   ClayToolWidgetGroupClass;

struct _ClayToolWidgetGroup
{
  ClayToolWidget              parent_instance;

  ClayToolWidgetGroupPrivate *priv;
};

struct _ClayToolWidgetGroupClass
{
  ClayToolWidgetClass  parent_class;
};


GType            clay_tool_widget_group_get_type         (void);

ClayToolWidget * clay_tool_widget_group_new              (ClayDisplayShell    *shell);

ClayContainer  * clay_tool_widget_group_get_children     (ClayToolWidgetGroup *group);

ClayToolWidget * clay_tool_widget_group_get_focus_widget (ClayToolWidgetGroup *group);

void             clay_tool_widget_group_set_auto_raise   (ClayToolWidgetGroup *group,
                                                          gboolean             auto_raise);
gboolean         clay_tool_widget_group_get_auto_raise   (ClayToolWidgetGroup *group);
