/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include "claylayer.h"


#define CLAY_TYPE_GROUP_LAYER (clay_group_layer_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayGroupLayer,
                          clay_group_layer,
                          CLAY, GROUP_LAYER,
                          ClayLayer)


struct _ClayGroupLayerClass
{
  ClayLayerClass  parent_class;
};


ClayLayer      * clay_group_layer_new                 (ClayImage           *image);

ClayProjection * clay_group_layer_get_projection      (ClayGroupLayer      *group);

void             clay_group_layer_suspend_resize      (ClayGroupLayer      *group,
                                                       gboolean             push_undo);
void             clay_group_layer_resume_resize       (ClayGroupLayer      *group,
                                                       gboolean             push_undo);

void             clay_group_layer_suspend_mask        (ClayGroupLayer      *group,
                                                       gboolean             push_undo);
void             clay_group_layer_resume_mask         (ClayGroupLayer      *group,
                                                       gboolean             push_undo);


void             _clay_group_layer_set_suspended_mask (ClayGroupLayer      *group,
                                                       GeglBuffer          *buffer,
                                                       const GeglRectangle *bounds);
GeglBuffer     * _clay_group_layer_get_suspended_mask (ClayGroupLayer      *group,
                                                       GeglRectangle       *bounds);

void             _clay_group_layer_start_transform    (ClayGroupLayer      *group,
                                                       gboolean             push_undo);
void             _clay_group_layer_end_transform      (ClayGroupLayer      *group,
                                                       gboolean             push_undo);
