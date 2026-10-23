/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolhandlegrid.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "claytooltransformgrid.h"


#define CLAY_TYPE_TOOL_HANDLE_GRID            (clay_tool_handle_grid_get_type ())
#define CLAY_TOOL_HANDLE_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_HANDLE_GRID, ClayToolHandleGrid))
#define CLAY_TOOL_HANDLE_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_HANDLE_GRID, ClayToolHandleGridClass))
#define CLAY_IS_TOOL_HANDLE_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_HANDLE_GRID))
#define CLAY_IS_TOOL_HANDLE_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_HANDLE_GRID))
#define CLAY_TOOL_HANDLE_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_HANDLE_GRID, ClayToolHandleGridClass))


typedef struct _ClayToolHandleGrid        ClayToolHandleGrid;
typedef struct _ClayToolHandleGridPrivate ClayToolHandleGridPrivate;
typedef struct _ClayToolHandleGridClass   ClayToolHandleGridClass;

struct _ClayToolHandleGrid
{
  ClayToolTransformGrid      parent_instance;

  ClayToolHandleGridPrivate *private;
};

struct _ClayToolHandleGridClass
{
  ClayToolTransformGridClass  parent_class;
};


GType            clay_tool_handle_grid_get_type (void);

ClayToolWidget * clay_tool_handle_grid_new      (ClayDisplayShell  *shell,
                                                 gdouble            x1,
                                                 gdouble            y1,
                                                 gdouble            x2,
                                                 gdouble            y2);
