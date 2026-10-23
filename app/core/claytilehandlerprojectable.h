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

#pragma once

#include "gegl/claytilehandlervalidate.h"


/***
 * ClayTileHandlerProjectable is a GeglTileHandler that renders a
 * projectable, calling the projectable's begin_render() and end_render()
 * before/after the actual rendering.
 *
 * Note that the tile handler does not own a reference to the projectable.
 * It's the user's responsibility to manage the handler's and projectable's
 * lifetime.
 */

#define CLAY_TYPE_TILE_HANDLER_PROJECTABLE            (clay_tile_handler_projectable_get_type ())
#define CLAY_TILE_HANDLER_PROJECTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TILE_HANDLER_PROJECTABLE, ClayTileHandlerProjectable))
#define CLAY_TILE_HANDLER_PROJECTABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_TILE_HANDLER_PROJECTABLE, ClayTileHandlerProjectableClass))
#define CLAY_IS_TILE_HANDLER_PROJECTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TILE_HANDLER_PROJECTABLE))
#define CLAY_IS_TILE_HANDLER_PROJECTABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_TILE_HANDLER_PROJECTABLE))
#define CLAY_TILE_HANDLER_PROJECTABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_TILE_HANDLER_PROJECTABLE, ClayTileHandlerProjectableClass))


typedef struct _ClayTileHandlerProjectable      ClayTileHandlerProjectable;
typedef struct _ClayTileHandlerProjectableClass ClayTileHandlerProjectableClass;

struct _ClayTileHandlerProjectable
{
  ClayTileHandlerValidate  parent_instance;

  ClayProjectable         *projectable;
};

struct _ClayTileHandlerProjectableClass
{
  ClayTileHandlerValidateClass  parent_class;
};


GType             clay_tile_handler_projectable_get_type (void);

GeglTileHandler * clay_tile_handler_projectable_new      (ClayProjectable *projectable);
