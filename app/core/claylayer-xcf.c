/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   claylayer-xcf.c
 *
 *   Copyright 2003 Sven Neumann <sven@gimp.org>
 *   Copyright 2025 Jehan
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "clayimage.h"
#include "claylayer.h"
#include "claylayer-xcf.h"

/**
 * clay_layer_from_layer:
 * @layer:          a pointer to #ClayLayer object which will be
 *                  replaced by a new layer of type @new_layer_type.
 * @new_layer_type: a #GType for the new layer type
 *
 * Converts a standard #ClayLayer into a more specific type of
 * #ClayLayer. The new layer takes ownership of properties of @layer.
 * The @layer object is rendered unusable by this function. Don't even
 * try to use it afterwards!
 *
 * This is a gross hack that is needed in order to load text or vector
 * layers from XCF files in a backwards-compatible way, or as a
 * secondary step (after more data has been loaded). Please don't use it
 * for anything else!
 *
 * The variable list of arguments will be the properties which will be
 * used to create the new layer of type @new_layer_type. Note that the
 * "image" property needs to be set at the minimum.
 **/
void
clay_layer_from_layer (ClayLayer **layer,
                       GType       new_layer_type,
                       ...)
{
  ClayLayer    *new_layer;
  ClayImage    *image;
  gboolean      attached;
  ClayLayer    *parent   = NULL;
  gint          position = 0;
  va_list       args;
  const gchar  *first_prop;

  g_return_if_fail (layer && CLAY_IS_LAYER (*layer));
  g_return_if_fail (g_type_is_a (new_layer_type, CLAY_TYPE_LAYER));

  image = clay_item_get_image (CLAY_ITEM (*layer));

  if ((attached = clay_item_is_attached (CLAY_ITEM (*layer))))
    {
      parent   = clay_layer_get_parent (*layer);
      position = clay_item_get_index (CLAY_ITEM (*layer));

      g_object_ref (*layer);
      clay_image_remove_layer (image, *layer, FALSE, NULL);
    }

  va_start (args, new_layer_type);
  first_prop = va_arg (args, gchar *);
  new_layer = CLAY_LAYER (g_object_new_valist (new_layer_type, first_prop, args));
  va_end (args);

  clay_item_replace_item (CLAY_ITEM (new_layer), CLAY_ITEM (*layer));

  clay_layer_set_opacity         (CLAY_LAYER (new_layer),
                                  clay_layer_get_opacity (*layer), FALSE);
  clay_layer_set_mode            (CLAY_LAYER (new_layer),
                                  clay_layer_get_mode (*layer), FALSE);
  clay_layer_set_blend_space     (CLAY_LAYER (new_layer),
                                  clay_layer_get_blend_space (*layer), FALSE);
  clay_layer_set_composite_space (CLAY_LAYER (new_layer),
                                  clay_layer_get_composite_space (*layer), FALSE);
  clay_layer_set_composite_mode  (CLAY_LAYER (new_layer),
                                  clay_layer_get_composite_mode (*layer), FALSE);
  clay_layer_set_lock_alpha      (CLAY_LAYER (new_layer),
                                  clay_layer_get_lock_alpha (*layer), FALSE);

  clay_drawable_steal_buffer (CLAY_DRAWABLE (new_layer), (ClayDrawable **) layer);

  if (attached)
    clay_image_add_layer (image, new_layer, parent, position, FALSE);

  *layer = new_layer;
}
