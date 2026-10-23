/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytilebackendplugin.h
 * Copyright (C) 2011-2019 Øyvind Kolås <pippin@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_TILE_BACKEND_PLUGIN_H__
#define __CLAY_TILE_BACKEND_PLUGIN_H__

#include <gegl-buffer-backend.h>

G_BEGIN_DECLS

#define CLAY_TYPE_TILE_BACKEND_PLUGIN            (_clay_tile_backend_plugin_get_type ())
#define CLAY_TILE_BACKEND_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TILE_BACKEND_PLUGIN, ClayTileBackendPlugin))
#define CLAY_TILE_BACKEND_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_TILE_BACKEND_PLUGIN, ClayTileBackendPluginClass))
#define CLAY_IS_TILE_BACKEND_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TILE_BACKEND_PLUGIN))
#define CLAY_IS_TILE_BACKEND_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_TILE_BACKEND_PLUGIN))
#define CLAY_TILE_BACKEND_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_TILE_BACKEND_PLUGIN, ClayTileBackendPluginClass))


typedef struct _ClayTileBackendPlugin        ClayTileBackendPlugin;
typedef struct _ClayTileBackendPluginClass   ClayTileBackendPluginClass;
typedef struct _ClayTileBackendPluginPrivate ClayTileBackendPluginPrivate;

struct _ClayTileBackendPlugin
{
  GeglTileBackend               parent_instance;

  ClayTileBackendPluginPrivate *priv;
};

struct _ClayTileBackendPluginClass
{
  GeglTileBackendClass parent_class;
};

GType             _clay_tile_backend_plugin_get_type (void);

GeglTileBackend * _clay_tile_backend_plugin_new      (ClayDrawable *drawable,
                                                      gint          shadow);

G_END_DECLS

#endif /* __CLAY_TILE_BACKEND_PLUGIN_H__ */
