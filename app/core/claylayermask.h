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

#include "claychannel.h"


#define CLAY_TYPE_LAYER_MASK            (clay_layer_mask_get_type ())
#define CLAY_LAYER_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER_MASK, ClayLayerMask))
#define CLAY_LAYER_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LAYER_MASK, ClayLayerMaskClass))
#define CLAY_IS_LAYER_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER_MASK))
#define CLAY_IS_LAYER_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LAYER_MASK))
#define CLAY_LAYER_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LAYER_MASK, ClayLayerMaskClass))


typedef struct _ClayLayerMaskClass  ClayLayerMaskClass;

struct _ClayLayerMask
{
  ClayChannel  parent_instance;

  ClayLayer   *layer;
};

struct _ClayLayerMaskClass
{
  ClayChannelClass  parent_class;
};


GType           clay_layer_mask_get_type  (void);

ClayLayerMask * clay_layer_mask_new       (ClayImage     *image,
                                           gint           width,
                                           gint           height,
                                           const gchar   *name,
                                           GeglColor     *color);

void            clay_layer_mask_set_layer (ClayLayerMask *layer_mask,
                                           ClayLayer     *layer);
ClayLayer     * clay_layer_mask_get_layer (ClayLayerMask *layer_mask);
