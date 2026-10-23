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
 * ClayTileHandlerIscissors is a GeglTileHandler that renders the
 * Iscissors tool's gradmap.
 */

#define CLAY_TYPE_TILE_HANDLER_ISCISSORS            (clay_tile_handler_iscissors_get_type ())
#define CLAY_TILE_HANDLER_ISCISSORS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TILE_HANDLER_ISCISSORS, ClayTileHandlerIscissors))
#define CLAY_TILE_HANDLER_ISCISSORS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_TILE_HANDLER_ISCISSORS, ClayTileHandlerIscissorsClass))
#define CLAY_IS_TILE_HANDLER_ISCISSORS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TILE_HANDLER_ISCISSORS))
#define CLAY_IS_TILE_HANDLER_ISCISSORS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_TILE_HANDLER_ISCISSORS))
#define CLAY_TILE_HANDLER_ISCISSORS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_TILE_HANDLER_ISCISSORS, ClayTileHandlerIscissorsClass))


typedef struct _ClayTileHandlerIscissors      ClayTileHandlerIscissors;
typedef struct _ClayTileHandlerIscissorsClass ClayTileHandlerIscissorsClass;

struct _ClayTileHandlerIscissors
{
  ClayTileHandlerValidate  parent_instance;

  ClayPickable            *pickable;
};

struct _ClayTileHandlerIscissorsClass
{
  ClayTileHandlerValidateClass  parent_class;
};


GType             clay_tile_handler_iscissors_get_type (void);

GeglTileHandler * clay_tile_handler_iscissors_new      (ClayPickable *pickable);
