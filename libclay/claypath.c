/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claypath.c
 * Copyright (C) Jehan
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


struct _ClayPath
{
  ClayItem parent_instance;
};

G_DEFINE_TYPE (ClayPath, clay_path, CLAY_TYPE_ITEM)

#define parent_class clay_path_parent_class


static void
clay_path_class_init (ClayPathClass *klass)
{
}

static void
clay_path_init (ClayPath *path)
{
}

/**
 * clay_path_get_by_id:
 * @path_id: The path id.
 *
 * Returns a #ClayPath representing @path_id. This function
 * calls clay_item_get_by_id() and returns the item if it is a path
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #ClayPath for @path_id
 *          or %NULL if @path_id does not represent a valid
 *          path. The object belongs to libclay and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
ClayPath *
clay_path_get_by_id (gint32 path_id)
{
  ClayItem *item = clay_item_get_by_id (path_id);

  if (CLAY_IS_PATH (item))
    return (ClayPath *) item;

  return NULL;
}
