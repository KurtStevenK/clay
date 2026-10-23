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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"

#include "widgets-types.h"

#include "core/clay.h"

#include "clayrender.h"


static void   clay_render_setup_notify (gpointer    config,
                                        GParamSpec *param_spec,
                                        Clay       *clay);


static GeglColor *color1 = NULL;
static GeglColor *color2 = NULL;


void
clay_render_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  color1 = gegl_color_new (NULL);
  gegl_color_set_pixel (color1, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR1);
  color2 = gegl_color_new (NULL);
  gegl_color_set_pixel (color2, babl_format ("R'G'B'A double"), CLAY_CHECKS_CUSTOM_COLOR2);

  g_signal_connect (clay->config, "notify::transparency-type",
                    G_CALLBACK (clay_render_setup_notify),
                    clay);

  g_signal_connect (clay->config, "notify::transparency-custom-color1",
                    G_CALLBACK (clay_render_setup_notify),
                    clay);

  g_signal_connect (clay->config, "notify::transparency-custom-color2",
                    G_CALLBACK (clay_render_setup_notify),
                    clay);

  clay_render_setup_notify (clay->config, NULL, clay);
}

void
clay_render_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_signal_handlers_disconnect_by_func (clay->config,
                                        clay_render_setup_notify,
                                        clay);

  g_clear_object (&color1);
  g_clear_object (&color2);
}

const GeglColor *
clay_render_check_color1 (void)
{
  return color1;
}

const GeglColor *
clay_render_check_color2 (void)
{
  return color2;
}

static void
clay_render_setup_notify (gpointer    config,
                          GParamSpec *param_spec,
                          Clay       *clay)
{
  GeglColor     *color1_custom = NULL;
  GeglColor     *color2_custom = NULL;
  ClayCheckType  check_type;

  g_object_get (config,
                "transparency-type",          &check_type,
                "transparency-custom-color1", &color1_custom,
                "transparency-custom-color2", &color2_custom,
                NULL);

  g_clear_object (&color1);
  g_clear_object (&color2);
  color1 = color1_custom;
  color2 = color2_custom;
  clay_checks_get_colors (check_type, &color1, &color2);
  g_clear_object (&color1_custom);
  g_clear_object (&color2_custom);
}
