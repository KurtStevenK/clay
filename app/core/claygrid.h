/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGrid
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "clayobject.h"


#define CLAY_TYPE_GRID            (clay_grid_get_type ())
#define CLAY_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRID, ClayGrid))
#define CLAY_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRID, ClayGridClass))
#define CLAY_IS_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRID))
#define CLAY_IS_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRID))
#define CLAY_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRID, ClayGridClass))


typedef struct _ClayGridClass  ClayGridClass;

struct _ClayGrid
{
  ClayObject     parent_instance;

  ClayGridStyle  style;
  GeglColor     *fgcolor;
  GeglColor     *bgcolor;
  gdouble        xspacing;
  gdouble        yspacing;
  ClayUnit      *spacing_unit;
  gdouble        xoffset;
  gdouble        yoffset;
  ClayUnit      *offset_unit;
};


struct _ClayGridClass
{
  ClayObjectClass  parent_class;
};


GType           clay_grid_get_type               (void);

ClayGridStyle   clay_grid_get_style              (ClayGrid           *grid);

GeglColor     * clay_grid_get_fgcolor            (ClayGrid           *grid);
GeglColor     * clay_grid_get_bgcolor            (ClayGrid           *grid);

void            clay_grid_get_spacing            (ClayGrid           *grid,
                                                  gdouble            *xspacing,
                                                  gdouble            *yspacing);
void            clay_grid_get_offset             (ClayGrid           *grid,
                                                  gdouble            *xoffset,
                                                  gdouble            *yoffset);

const gchar   * clay_grid_parasite_name          (void) G_GNUC_CONST;
ClayParasite  * clay_grid_to_parasite            (ClayGrid           *grid);
ClayGrid      * clay_grid_from_parasite          (const ClayParasite *parasite);
