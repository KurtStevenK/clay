/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * clayselection.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"


struct _ClaySelection
{
  ClayChannel parent_instance;
};


G_DEFINE_TYPE (ClaySelection, clay_selection, CLAY_TYPE_CHANNEL)

#define parent_class clay_selection_parent_class


static void
clay_selection_class_init (ClaySelectionClass *klass)
{
}

static void
clay_selection_init (ClaySelection *selection)
{
}

/**
 * clay_selection_get_by_id:
 * @selection_id: The selection id.
 *
 * Returns a #ClaySelection representing @selection_id. This function
 * calls clay_item_get_by_id() and returns the item if it is selection
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClaySelection for
 *          @selection_id or %NULL if @selection_id does not represent
 *          a valid selection. The object belongs to libclay and you
 *          must not modify or unref it.
 *
 * Since: 3.0
 **/
ClaySelection *
clay_selection_get_by_id (gint32 selection_id)
{
  ClayItem *item = clay_item_get_by_id (selection_id);

  if (CLAY_IS_SELECTION (item))
    return (ClaySelection *) item;

  return NULL;
}

/**
 * clay_selection_float:
 * @image:       ignored
 * @drawables:   (array zero-terminated=1): The drawables from which to
 *               float selection.
 * @offx:        x offset for translation.
 * @offy:        y offset for translation.
 *
 * Float the selection from the specified drawable with initial offsets
 * as specified.
 *
 * This procedure determines the region of the specified drawable that
 * lies beneath the current selection. The region is then cut from the
 * drawable and the resulting data is made into a new layer which is
 * instantiated as a floating selection. The offsets allow initial
 * positioning of the new floating selection.
 *
 * Returns: (transfer none): The floated layer.
 */
ClayLayer *
clay_selection_float (ClayImage     *image,
                      ClayDrawable **drawables,
                      gint           offx,
                      gint           offy)
{
  return _clay_selection_float ((const ClayDrawable **) drawables, offx, offy);
}
