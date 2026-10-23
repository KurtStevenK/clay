/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * clayrasterizable.h
 * Copyright (C) 2025 Jehan
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

#include "claydrawable.h"


#define CLAY_TYPE_RASTERIZABLE (clay_rasterizable_get_type ())
G_DECLARE_INTERFACE (ClayRasterizable,
                     clay_rasterizable,
                     CLAY, RASTERIZABLE,
                     ClayDrawable)


struct _ClayRasterizableInterface
{
  GTypeInterface base_iface;

  /*  signals            */
  void   (* set_rasterized)                      (ClayRasterizable  *rasterizable,
                                                  gboolean           rasterized);
};


void       clay_rasterizable_rasterize           (ClayRasterizable  *rasterizable,
                                                  gboolean           push_undo);
void       clay_rasterizable_restore             (ClayRasterizable  *rasterizable);

gboolean   clay_rasterizable_is_rasterized       (ClayRasterizable  *rasterizable);

void       clay_rasterizable_set_auto_rename     (ClayRasterizable  *rasterizable,
                                                  gboolean           auto_rename);
gboolean   clay_rasterizable_get_auto_rename     (ClayRasterizable  *rasterizable);


/* Overwrite rename() of ClayItemClass of all children. */
gboolean   clay_rasterizable_rename              (ClayItem          *item,
                                                  const gchar       *new_name,
                                                  const gchar       *undo_desc,
                                                  GError           **error);
/* Shared code to rename the rasterizable. */
void       clay_rasterizable_auto_rename         (ClayRasterizable  *rasterizable,
                                                  ClayObject        *rename_data,
                                                  const gchar       *rename_string);

/* To be used for undo steps only */
void       clay_rasterizable_set_undo_rasterized (ClayRasterizable  *rasterizable,
                                                  gboolean           rasterize);
