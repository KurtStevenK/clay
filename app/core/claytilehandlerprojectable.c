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

#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "clayprojectable.h"

#include "claytilehandlerprojectable.h"


static void   clay_tile_handler_projectable_begin_validate (ClayTileHandlerValidate *validate);
static void   clay_tile_handler_projectable_end_validate   (ClayTileHandlerValidate *validate);


G_DEFINE_TYPE (ClayTileHandlerProjectable, clay_tile_handler_projectable,
               CLAY_TYPE_TILE_HANDLER_VALIDATE)

#define parent_class clay_tile_handler_projectable_parent_class


static void
clay_tile_handler_projectable_class_init (ClayTileHandlerProjectableClass *klass)
{
  ClayTileHandlerValidateClass *validate_class;

  validate_class = CLAY_TILE_HANDLER_VALIDATE_CLASS (klass);

  validate_class->begin_validate = clay_tile_handler_projectable_begin_validate;
  validate_class->end_validate   = clay_tile_handler_projectable_end_validate;
}

static void
clay_tile_handler_projectable_init (ClayTileHandlerProjectable *projectable)
{
}

static void
clay_tile_handler_projectable_begin_validate (ClayTileHandlerValidate *validate)
{
  ClayTileHandlerProjectable *handler = CLAY_TILE_HANDLER_PROJECTABLE (validate);

  CLAY_TILE_HANDLER_VALIDATE_CLASS (parent_class)->begin_validate (validate);

  clay_projectable_begin_render (handler->projectable);
}

static void
clay_tile_handler_projectable_end_validate (ClayTileHandlerValidate *validate)
{
  ClayTileHandlerProjectable *handler = CLAY_TILE_HANDLER_PROJECTABLE (validate);

  clay_projectable_end_render (handler->projectable);

  CLAY_TILE_HANDLER_VALIDATE_CLASS (parent_class)->end_validate (validate);
}

GeglTileHandler *
clay_tile_handler_projectable_new (ClayProjectable *projectable)
{
  ClayTileHandlerProjectable *handler;

  g_return_val_if_fail (CLAY_IS_PROJECTABLE (projectable), NULL);

  handler = g_object_new (CLAY_TYPE_TILE_HANDLER_PROJECTABLE, NULL);

  CLAY_TILE_HANDLER_VALIDATE (handler)->graph =
    g_object_ref (clay_projectable_get_graph (projectable));

  handler->projectable = projectable;

  return GEGL_TILE_HANDLER (handler);
}
