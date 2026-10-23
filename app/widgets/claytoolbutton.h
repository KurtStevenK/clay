/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolbutton.h
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


#define CLAY_TYPE_TOOL_BUTTON            (clay_tool_button_get_type ())
#define CLAY_TOOL_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_BUTTON, ClayToolButton))
#define CLAY_TOOL_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_BUTTON, ClayToolButtonClass))
#define CLAY_IS_TOOL_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_TOOL_BUTTON))
#define CLAY_IS_TOOL_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_BUTTON))
#define CLAY_TOOL_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_BUTTON, ClayToolButtonClass))


typedef struct _ClayToolButtonPrivate ClayToolButtonPrivate;
typedef struct _ClayToolButtonClass   ClayToolButtonClass;

struct _ClayToolButton
{
  GtkToggleToolButton    parent_instance;

  ClayToolButtonPrivate *priv;
};

struct _ClayToolButtonClass
{
  GtkToggleToolButtonClass  parent_class;
};


GType          clay_tool_button_get_type      (void);

GtkToolItem  * clay_tool_button_new           (ClayToolbox    *toolbox,
                                               ClayToolItem   *tool_item);

ClayToolbox  * clay_tool_button_get_toolbox   (ClayToolButton *tool_button);

void           clay_tool_button_set_tool_item (ClayToolButton *tool_button,
                                               ClayToolItem   *tool_item);
ClayToolItem * clay_tool_button_get_tool_item (ClayToolButton *tool_button);

ClayToolInfo * clay_tool_button_get_tool_info (ClayToolButton *tool_button);
