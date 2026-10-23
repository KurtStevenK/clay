/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/clayrc.h"

#include "clay.h"
#include "clay-data-factories.h"
#include "clay-gradients.h"
#include "clay-memsize.h"
#include "clay-palettes.h"
#include "clay-utils.h"
#include "claycontainer.h"
#include "claybrush-load.h"
#include "claybrush.h"
#include "claybrushclipboard.h"
#include "claybrushgenerated-load.h"
#include "claybrushpipe-load.h"
#include "claycurve.h"
#include "claydataloaderfactory.h"
#include "claydynamics.h"
#include "claydynamics-load.h"
#include "claygradient-load.h"
#include "claygradient.h"
#include "claymybrush-load.h"
#include "claymybrush.h"
#include "claypalette-load.h"
#include "claypalette.h"
#include "claypattern-load.h"
#include "claypattern.h"
#include "claypatternclipboard.h"
#include "claytagcache.h"
#include "claytoolpreset.h"
#include "claytoolpreset-load.h"

#include "text/clayfont.h"
#include "text/clayfontfactory.h"

#include "clay-intl.h"


void
clay_data_factories_init (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay->brush_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_BRUSH,
                                  "brush-path",
                                  "brush-path-writable",
                                  "brush-paths",
                                  clay_brush_new,
                                  clay_brush_get_standard);
  clay_object_set_static_name (CLAY_OBJECT (clay->brush_factory),
                               "brush factory");
  clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Clay Brush",
                                       clay_brush_load,
                                       CLAY_BRUSH_FILE_EXTENSION,
                                       TRUE);
  clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Clay Brush Pixmap",
                                       clay_brush_load,
                                       CLAY_BRUSH_PIXMAP_FILE_EXTENSION,
                                       FALSE);
  clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Photoshop ABR Brush",
                                       clay_brush_load_abr,
                                       CLAY_BRUSH_PS_FILE_EXTENSION,
                                       FALSE);
  clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Paint Shop Pro JBR Brush",
                                       clay_brush_load_abr,
                                       CLAY_BRUSH_PSP_FILE_EXTENSION,
                                       FALSE);
 clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Clay Generated Brush",
                                       clay_brush_generated_load,
                                       CLAY_BRUSH_GENERATED_FILE_EXTENSION,
                                       TRUE);
  clay_data_loader_factory_add_loader (clay->brush_factory,
                                       "Clay Brush Pipe",
                                       clay_brush_pipe_load,
                                       CLAY_BRUSH_PIPE_FILE_EXTENSION,
                                       TRUE);

  clay->dynamics_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_DYNAMICS,
                                  "dynamics-path",
                                  "dynamics-path-writable",
                                  "dynamics-paths",
                                  clay_dynamics_new,
                                  clay_dynamics_get_standard);
  clay_object_set_static_name (CLAY_OBJECT (clay->dynamics_factory),
                               "dynamics factory");
  clay_data_loader_factory_add_loader (clay->dynamics_factory,
                                       "Clay Paint Dynamics",
                                       clay_dynamics_load,
                                       CLAY_DYNAMICS_FILE_EXTENSION,
                                       TRUE);

  clay->mybrush_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_MYBRUSH,
                                  "mypaint-brush-path",
                                  "mypaint-brush-path-writable",
                                  "mypaint-brush-paths",
                                  NULL,
                                  NULL);
  clay_object_set_static_name (CLAY_OBJECT (clay->mybrush_factory),
                               "mypaint brush factory");
  clay_data_loader_factory_add_loader (clay->mybrush_factory,
                                       "MyPaint Brush",
                                       clay_mybrush_load,
                                       CLAY_MYBRUSH_FILE_EXTENSION,
                                       FALSE);

  clay->pattern_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_PATTERN,
                                  "pattern-path",
                                  "pattern-path-writable",
                                  "pattern-paths",
                                  NULL,
                                  clay_pattern_get_standard);
  clay_object_set_static_name (CLAY_OBJECT (clay->pattern_factory),
                               "pattern factory");
  clay_data_loader_factory_add_loader (clay->pattern_factory,
                                       "Clay Pattern",
                                       clay_pattern_load,
                                       CLAY_PATTERN_FILE_EXTENSION,
                                       TRUE);
  clay_data_loader_factory_add_fallback (clay->pattern_factory,
                                         "Pattern from GdkPixbuf",
                                         clay_pattern_load_pixbuf);

  clay->gradient_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_GRADIENT,
                                  "gradient-path",
                                  "gradient-path-writable",
                                  "gradient-paths",
                                  clay_gradient_new,
                                  clay_gradient_get_standard);
  clay_object_set_static_name (CLAY_OBJECT (clay->gradient_factory),
                               "gradient factory");
  clay_data_loader_factory_add_loader (clay->gradient_factory,
                                       "Clay Gradient",
                                       clay_gradient_load,
                                       CLAY_GRADIENT_FILE_EXTENSION,
                                       TRUE);
  clay_data_loader_factory_add_loader (clay->gradient_factory,
                                       "SVG Gradient",
                                       clay_gradient_load_svg,
                                       CLAY_GRADIENT_SVG_FILE_EXTENSION,
                                       FALSE);

  clay->palette_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_PALETTE,
                                  "palette-path",
                                  "palette-path-writable",
                                  "palette-paths",
                                  clay_palette_new,
                                  clay_palette_get_standard);
  clay_object_set_static_name (CLAY_OBJECT (clay->palette_factory),
                               "palette factory");
  clay_data_loader_factory_add_loader (clay->palette_factory,
                                       "Clay Palette",
                                       clay_palette_load,
                                       CLAY_PALETTE_FILE_EXTENSION,
                                       TRUE);

  clay->font_factory =
    clay_font_factory_new (clay,
                           "font-path");
  clay_object_set_static_name (CLAY_OBJECT (clay->font_factory),
                               "font factory");
  clay_font_class_set_font_factory (CLAY_FONT_FACTORY (clay->font_factory));

  clay->tool_preset_factory =
    clay_data_loader_factory_new (clay,
                                  CLAY_TYPE_TOOL_PRESET,
                                  "tool-preset-path",
                                  "tool-preset-path-writable",
                                  "tool-preset-paths",
                                  clay_tool_preset_new,
                                  NULL);
  clay_object_set_static_name (CLAY_OBJECT (clay->tool_preset_factory),
                               "tool preset factory");
  clay_data_loader_factory_add_loader (clay->tool_preset_factory,
                                       "Clay Tool Preset",
                                       clay_tool_preset_load,
                                       CLAY_TOOL_PRESET_FILE_EXTENSION,
                                       TRUE);

  clay->tag_cache = clay_tag_cache_new ();
}

void
clay_data_factories_add_builtin (Clay *clay)
{
  ClayData *data;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /*  add the builtin FG -> BG etc. gradients  */
  clay_gradients_init (clay);

  /*  add the color history palette  */
  clay_palettes_init (clay);

  /*  add the clipboard brushes  */
  data = clay_brush_clipboard_new (clay, FALSE);
  clay_data_make_internal (data, "clay-brush-clipboard-image");
  clay_container_add (clay_data_factory_get_container (clay->brush_factory),
                      CLAY_OBJECT (data));
  g_object_unref (data);

  data = clay_brush_clipboard_new (clay, TRUE);
  clay_data_make_internal (data, "clay-brush-clipboard-mask");
  clay_container_add (clay_data_factory_get_container (clay->brush_factory),
                      CLAY_OBJECT (data));
  g_object_unref (data);

  /*  add the clipboard pattern  */
  data = clay_pattern_clipboard_new (clay);
  clay_data_make_internal (data, "clay-pattern-clipboard-image");
  clay_container_add (clay_data_factory_get_container (clay->pattern_factory),
                      CLAY_OBJECT (data));
  g_object_unref (data);
}

void
clay_data_factories_clear (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (clay->brush_factory)
    clay_data_factory_data_free (clay->brush_factory);

  if (clay->dynamics_factory)
    clay_data_factory_data_free (clay->dynamics_factory);

  if (clay->mybrush_factory)
    clay_data_factory_data_free (clay->mybrush_factory);

  if (clay->pattern_factory)
    clay_data_factory_data_free (clay->pattern_factory);

  if (clay->gradient_factory)
    clay_data_factory_data_free (clay->gradient_factory);

  if (clay->palette_factory)
    clay_data_factory_data_free (clay->palette_factory);

  if (clay->font_factory)
    clay_data_factory_data_free (clay->font_factory);

  if (clay->tool_preset_factory)
    clay_data_factory_data_free (clay->tool_preset_factory);
}

void
clay_data_factories_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_clear_object (&clay->brush_factory);
  g_clear_object (&clay->dynamics_factory);
  g_clear_object (&clay->mybrush_factory);
  g_clear_object (&clay->pattern_factory);
  g_clear_object (&clay->gradient_factory);
  g_clear_object (&clay->palette_factory);
  g_clear_object (&clay->font_factory);
  g_clear_object (&clay->tool_preset_factory);
  g_clear_object (&clay->tag_cache);
}

gboolean
clay_data_factories_wait (Clay *clay)
{
  GList    *data_types;
  GList    *excluded;
  gboolean  loaded = TRUE;

  /* Curves are the only data type without a factory. */
  excluded = g_list_prepend (NULL, CLAYTYPE_TO_POINTER (CLAY_TYPE_CURVE));

  data_types = clay_get_type_children (CLAY_TYPE_DATA, NULL, excluded);
  g_list_free (excluded);

  for (GList *iter = data_types; iter; iter = iter->next)
    {
      ClayDataFactory *factory;

      factory = clay_get_data_factory (clay, CLAYPOINTER_TO_TYPE (iter->data));

      if (factory)
        {
          ClayAsyncSet *set;

          set = clay_data_factory_get_async_set (factory);
          g_object_get (set, "empty", &loaded, NULL);
          if (! loaded)
            {
              clay_data_factory_data_wait (factory);
              loaded = TRUE;
            }
        }
    }

  g_list_free (data_types);

  return loaded;
}

gint64
clay_data_factories_get_memsize (Clay   *clay,
                                 gint64 *gui_size)
{
  gint64 memsize = 0;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), 0);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->named_buffers),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->brush_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->dynamics_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->mybrush_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->pattern_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->gradient_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->palette_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->font_factory),
                                      gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->tool_preset_factory),
                                      gui_size);

  memsize += clay_object_get_memsize (CLAY_OBJECT (clay->tag_cache),
                                      gui_size);

  return memsize;
}

void
clay_data_factories_data_clean (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_data_factory_data_clean (clay->brush_factory);
  clay_data_factory_data_clean (clay->dynamics_factory);
  clay_data_factory_data_clean (clay->mybrush_factory);
  clay_data_factory_data_clean (clay->pattern_factory);
  clay_data_factory_data_clean (clay->gradient_factory);
  clay_data_factory_data_clean (clay->palette_factory);
  clay_data_factory_data_clean (clay->font_factory);
  clay_data_factory_data_clean (clay->tool_preset_factory);
}

void
clay_data_factories_load (Clay               *clay,
                          ClayInitStatusFunc  status_callback)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  /*  initialize the list of clay brushes    */
  status_callback (NULL, _("Brushes"), 0.1);
  clay_data_factory_data_init (clay->brush_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the list of clay dynamics   */
  status_callback (NULL, _("Dynamics"), 0.15);
  clay_data_factory_data_init (clay->dynamics_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the list of mypaint brushes    */
  status_callback (NULL, _("MyPaint Brushes"), 0.2);
  clay_data_factory_data_init (clay->mybrush_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the list of clay patterns   */
  status_callback (NULL, _("Patterns"), 0.3);
  clay_data_factory_data_init (clay->pattern_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the list of clay palettes   */
  status_callback (NULL, _("Palettes"), 0.4);
  clay_data_factory_data_init (clay->palette_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the list of clay gradients  */
  status_callback (NULL, _("Gradients"), 0.5);
  clay_data_factory_data_init (clay->gradient_factory, clay->user_context,
                               clay->no_data);

  /*  initialize the color history   */
  status_callback (NULL, _("Color History"), 0.55);
  clay_palettes_load (clay);

  /*  initialize the list of clay fonts   */
  status_callback (NULL, _("Fonts"), 0.6);
  clay_data_factory_data_init (clay->font_factory, clay->user_context,
                               clay->no_fonts);

  /*  initialize the list of clay tool presets if we have a GUI  */
  if (! clay->no_interface)
    {
      status_callback (NULL, _("Tool Presets"), 0.7);
      clay_data_factory_data_init (clay->tool_preset_factory, clay->user_context,
                                   clay->no_data);
    }

  /* update tag cache */
  status_callback (NULL, _("Updating tag cache"), 0.75);
  clay_tag_cache_load (clay->tag_cache);
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->brush_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->dynamics_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->mybrush_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->pattern_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->gradient_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->palette_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->font_factory));
  clay_tag_cache_add_container (clay->tag_cache,
                                clay_data_factory_get_container (clay->tool_preset_factory));
}

void
clay_data_factories_save (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_tag_cache_save (clay->tag_cache);

  clay_data_factory_data_save (clay->brush_factory);
  clay_data_factory_data_save (clay->dynamics_factory);
  clay_data_factory_data_save (clay->mybrush_factory);
  clay_data_factory_data_save (clay->pattern_factory);
  clay_data_factory_data_save (clay->gradient_factory);
  clay_data_factory_data_save (clay->palette_factory);
  clay_data_factory_data_save (clay->font_factory);
  clay_data_factory_data_save (clay->tool_preset_factory);

  clay_palettes_save (clay);
}
