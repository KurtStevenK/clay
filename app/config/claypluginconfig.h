/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayPluginConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "config/claydialogconfig.h"


#define CLAY_TYPE_PLUGIN_CONFIG            (clay_plugin_config_get_type ())
#define CLAY_PLUGIN_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUGIN_CONFIG, ClayPluginConfig))
#define CLAY_PLUGIN_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUGIN_CONFIG, ClayPluginConfigClass))
#define CLAY_IS_PLUGIN_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUGIN_CONFIG))
#define CLAY_IS_PLUGIN_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUGIN_CONFIG))


typedef struct _ClayPluginConfigClass ClayPluginConfigClass;

struct _ClayPluginConfig
{
  ClayDialogConfig    parent_instance;

  gchar              *fractalexplorer_path;
  gchar              *gfig_path;
  gchar              *gflare_path;
  gchar              *clayressionist_path;
  gchar              *script_fu_path;
};

struct _ClayPluginConfigClass
{
  ClayGuiConfigClass  parent_class;
};


GType  clay_plugin_config_get_type (void);
