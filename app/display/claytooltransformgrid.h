/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooltransformgrid.h
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

#include "claycanvashandle.h"
#include "claytoolwidget.h"


#define CLAY_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE \
  (1.5 * CLAY_CANVAS_HANDLE_SIZE_LARGE)


typedef enum
{
  CLAY_TRANSFORM_HANDLE_NONE,
  CLAY_TRANSFORM_HANDLE_NW_P,     /* north west perspective         */
  CLAY_TRANSFORM_HANDLE_NE_P,     /* north east perspective         */
  CLAY_TRANSFORM_HANDLE_SW_P,     /* south west perspective         */
  CLAY_TRANSFORM_HANDLE_SE_P,     /* south east perspective         */
  CLAY_TRANSFORM_HANDLE_NW,       /* north west                     */
  CLAY_TRANSFORM_HANDLE_NE,       /* north east                     */
  CLAY_TRANSFORM_HANDLE_SW,       /* south west                     */
  CLAY_TRANSFORM_HANDLE_SE,       /* south east                     */
  CLAY_TRANSFORM_HANDLE_N,        /* north                          */
  CLAY_TRANSFORM_HANDLE_S,        /* south                          */
  CLAY_TRANSFORM_HANDLE_E,        /* east                           */
  CLAY_TRANSFORM_HANDLE_W,        /* west                           */
  CLAY_TRANSFORM_HANDLE_CENTER,   /* center for moving              */
  CLAY_TRANSFORM_HANDLE_PIVOT,    /* pivot for rotation and scaling */
  CLAY_TRANSFORM_HANDLE_N_S,      /* north shearing                 */
  CLAY_TRANSFORM_HANDLE_S_S,      /* south shearing                 */
  CLAY_TRANSFORM_HANDLE_E_S,      /* east shearing                  */
  CLAY_TRANSFORM_HANDLE_W_S,      /* west shearing                  */
  CLAY_TRANSFORM_HANDLE_ROTATION, /* rotation                       */

  CLAY_N_TRANSFORM_HANDLES /* keep this last so *handles[] is the right size */
} ClayTransformHandle;


#define CLAY_TYPE_TOOL_TRANSFORM_GRID            (clay_tool_transform_grid_get_type ())
#define CLAY_TOOL_TRANSFORM_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_TRANSFORM_GRID, ClayToolTransformGrid))
#define CLAY_TOOL_TRANSFORM_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_TRANSFORM_GRID, ClayToolTransformGridClass))
#define CLAY_IS_TOOL_TRANSFORM_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_TRANSFORM_GRID))
#define CLAY_IS_TOOL_TRANSFORM_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_TRANSFORM_GRID))
#define CLAY_TOOL_TRANSFORM_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_TRANSFORM_GRID, ClayToolTransformGridClass))


typedef struct _ClayToolTransformGrid        ClayToolTransformGrid;
typedef struct _ClayToolTransformGridPrivate ClayToolTransformGridPrivate;
typedef struct _ClayToolTransformGridClass   ClayToolTransformGridClass;

struct _ClayToolTransformGrid
{
  ClayToolWidget                parent_instance;

  ClayToolTransformGridPrivate *private;
};

struct _ClayToolTransformGridClass
{
  ClayToolWidgetClass  parent_class;
};


GType                 clay_tool_transform_grid_get_type   (void);

ClayToolWidget      * clay_tool_transform_grid_new        (ClayDisplayShell      *shell,
                                                           const ClayMatrix3     *transform,
                                                           gdouble                x1,
                                                           gdouble                y1,
                                                           gdouble                x2,
                                                           gdouble                y2);

/*  protected functions  */

ClayTransformHandle   clay_tool_transform_grid_get_handle (ClayToolTransformGrid *grid);
