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

#include "claytoolitem.h"


#define CLAY_TYPE_TOOL_INFO            (clay_tool_info_get_type ())
#define CLAY_TOOL_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_INFO, ClayToolInfo))
#define CLAY_TOOL_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_INFO, ClayToolInfoClass))
#define CLAY_IS_TOOL_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_INFO))
#define CLAY_IS_TOOL_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_INFO))
#define CLAY_TOOL_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_INFO, ClayToolInfoClass))


typedef struct _ClayToolInfoClass ClayToolInfoClass;

struct _ClayToolInfo
{
  ClayToolItem         parent_instance;

  Clay                *clay;

  GType                tool_type;
  GType                tool_options_type;
  ClayContextPropMask  context_props;

  gchar               *label;
  gchar               *tooltip;

  gchar               *menu_label;
  gchar               *menu_accel;

  gchar               *help_domain;
  gchar               *help_id;

  gboolean             hidden;
  gboolean             experimental;

  ClayToolOptions     *tool_options;
  ClayPaintInfo       *paint_info;

  ClayContainer       *presets;
};

struct _ClayToolInfoClass
{
  ClayToolItemClass  parent_class;
};


GType          clay_tool_info_get_type         (void);

ClayToolInfo * clay_tool_info_new              (Clay                *clay,
                                                GType                tool_type,
                                                GType                tool_options_type,
                                                ClayContextPropMask  context_props,
                                                const gchar         *identifier,
                                                const gchar         *label,
                                                const gchar         *tooltip,
                                                const gchar         *menu_label,
                                                const gchar         *menu_accel,
                                                const gchar         *help_domain,
                                                const gchar         *help_id,
                                                const gchar         *paint_core_name,
                                                const gchar         *icon_name);

void           clay_tool_info_set_standard     (Clay                *clay,
                                                ClayToolInfo        *tool_info);
ClayToolInfo * clay_tool_info_get_standard     (Clay                *clay);

gchar        * clay_tool_info_get_action_name (ClayToolInfo         *tool_info);

GFile        * clay_tool_info_get_options_file (ClayToolInfo        *tool_info,
                                                const gchar         *suffix);
