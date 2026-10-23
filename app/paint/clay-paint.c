/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "paint-types.h"

#include "core/clay.h"
#include "core/claylist.h"
#include "core/claypaintinfo.h"

#include "clay-paint.h"
#include "clayairbrush.h"
#include "clayclone.h"
#include "clayconvolve.h"
#include "claydodgeburn.h"
#include "clayeraser.h"
#include "clayheal.h"
#include "clayink.h"
#include "claymybrushcore.h"
#include "claypaintoptions.h"
#include "claypaintbrush.h"
#include "claypencil.h"
#include "clayperspectiveclone.h"
#include "claysmudge.h"


/*  local function prototypes  */

static void   clay_paint_register (Clay        *clay,
                                   GType        paint_type,
                                   GType        paint_options_type,
                                   const gchar *identifier,
                                   const gchar *blurb,
                                   const gchar *icon_name);


/*  public functions  */

void
clay_paint_init (Clay *clay)
{
  ClayPaintRegisterFunc register_funcs[] =
  {
    clay_dodge_burn_register,
    clay_smudge_register,
    clay_convolve_register,
    clay_perspective_clone_register,
    clay_heal_register,
    clay_clone_register,
    clay_mybrush_core_register,
    clay_ink_register,
    clay_airbrush_register,
    clay_eraser_register,
    clay_paintbrush_register,
    clay_pencil_register
  };

  gint i;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay->paint_info_list = clay_list_new (CLAY_TYPE_PAINT_INFO, FALSE);
  clay_object_set_static_name (CLAY_OBJECT (clay->paint_info_list),
                               "paint infos");

  clay_container_freeze (clay->paint_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (clay, clay_paint_register);
    }

  clay_container_thaw (clay->paint_info_list);
}

void
clay_paint_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_paint_info_set_standard (clay, NULL);

  if (clay->paint_info_list)
    {
      clay_container_foreach (clay->paint_info_list,
                              (GFunc) g_object_run_dispose, NULL);
      g_clear_object (&clay->paint_info_list);
    }
}


/*  private functions  */

static void
clay_paint_register (Clay        *clay,
                     GType        paint_type,
                     GType        paint_options_type,
                     const gchar *identifier,
                     const gchar *blurb,
                     const gchar *icon_name)
{
  ClayPaintInfo *paint_info;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (g_type_is_a (paint_type, CLAY_TYPE_PAINT_CORE));
  g_return_if_fail (g_type_is_a (paint_options_type, CLAY_TYPE_PAINT_OPTIONS));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (blurb != NULL);

  paint_info = clay_paint_info_new (clay,
                                    paint_type,
                                    paint_options_type,
                                    identifier,
                                    blurb,
                                    icon_name);

  clay_container_add (clay->paint_info_list, CLAY_OBJECT (paint_info));
  g_object_unref (paint_info);

  if (paint_type == CLAY_TYPE_PAINTBRUSH)
    clay_paint_info_set_standard (clay, paint_info);
}
