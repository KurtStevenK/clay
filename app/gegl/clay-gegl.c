/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-gegl.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
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

#include "config.h"

#include <gio/gio.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "clay-gegl-types.h"

#include "config/claygeglconfig.h"

#include "operations/clay-operations.h"

#include "core/clay.h"
#include "core/clay-parallel.h"

#include "clay-babl.h"
#include "clay-gegl.h"

#include <operation/gegl-operation.h>


static void  clay_gegl_notify_temp_path        (ClayGeglConfig *config);
static void  clay_gegl_notify_swap_path        (ClayGeglConfig *config);
static void  clay_gegl_notify_swap_compression (ClayGeglConfig *config);
static void  clay_gegl_notify_tile_cache_size  (ClayGeglConfig *config);
static void  clay_gegl_notify_num_processors   (ClayGeglConfig *config);
static void  clay_gegl_notify_use_opencl       (ClayGeglConfig *config);


/*  public functions  */

void
clay_gegl_init (Clay *clay)
{
  ClayGeglConfig *config;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  config = CLAY_GEGL_CONFIG (clay->config);

  /* make sure temp and swap directories exist */
  clay_gegl_notify_temp_path (config);
  clay_gegl_notify_swap_path (config);

  g_object_set (gegl_config (),
                "swap-compression", config->swap_compression,
                "tile-cache-size",  (guint64) config->tile_cache_size,
                "threads",          config->num_processors,
                "use-opencl",       config->use_opencl,
                NULL);

  clay_parallel_init (clay);

  g_signal_connect (config, "notify::temp-path",
                    G_CALLBACK (clay_gegl_notify_temp_path),
                    NULL);
  g_signal_connect (config, "notify::swap-path",
                    G_CALLBACK (clay_gegl_notify_swap_path),
                    NULL);
  g_signal_connect (config, "notify::swap-compression",
                    G_CALLBACK (clay_gegl_notify_swap_compression),
                    NULL);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (clay_gegl_notify_num_processors),
                    NULL);
  g_signal_connect (config, "notify::tile-cache-size",
                    G_CALLBACK (clay_gegl_notify_tile_cache_size),
                    NULL);
  g_signal_connect (config, "notify::num-processors",
                    G_CALLBACK (clay_gegl_notify_num_processors),
                    NULL);
  g_signal_connect (config, "notify::use-opencl",
                    G_CALLBACK (clay_gegl_notify_use_opencl),
                    NULL);

  clay_babl_init ();

  clay_operations_init (clay);
}

void
clay_gegl_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_operations_exit (clay);
  clay_parallel_exit (clay);
}


/*  private functions  */

static void
clay_gegl_notify_temp_path (ClayGeglConfig *config)
{
  GFile *file = clay_file_new_for_config_path (config->temp_path, NULL);

  if (! g_file_query_exists (file, NULL))
    g_file_make_directory_with_parents (file, NULL, NULL);

  g_object_unref (file);
}

static void
clay_gegl_notify_swap_path (ClayGeglConfig *config)
{
  GFile *file = clay_file_new_for_config_path (config->swap_path, NULL);
  gchar *path = g_file_get_path (file);

  if (! g_file_query_exists (file, NULL))
    g_file_make_directory_with_parents (file, NULL, NULL);

  g_object_set (gegl_config (),
                "swap", path,
                NULL);

  g_free (path);
  g_object_unref (file);
}

static void
clay_gegl_notify_swap_compression (ClayGeglConfig *config)
{
  g_object_set (gegl_config (),
                "swap-compression", config->swap_compression,
                NULL);
}

static void
clay_gegl_notify_tile_cache_size (ClayGeglConfig *config)
{
  g_object_set (gegl_config (),
                "tile-cache-size", (guint64) config->tile_cache_size,
                NULL);
}

static void
clay_gegl_notify_num_processors (ClayGeglConfig *config)
{
  g_object_set (gegl_config (),
                "threads", config->num_processors,
                NULL);
}

static void
clay_gegl_notify_use_opencl (ClayGeglConfig *config)
{
  g_object_set (gegl_config (),
                "use-opencl", config->use_opencl,
                NULL);
}
