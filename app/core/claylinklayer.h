/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayLinkLayer
 * Copyright (C) 2019 Jehan
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


#define CLAY_TYPE_LINK_LAYER            (clay_link_layer_get_type ())
#define CLAY_LINK_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LINK_LAYER, ClayLinkLayer))
#define CLAY_LINK_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LINK_LAYER, ClayLinkLayerClass))
#define CLAY_IS_LINK_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LINK_LAYER))
#define CLAY_IS_LINK_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LINK_LAYER))
#define CLAY_LINK_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LINK_LAYER, ClayLinkLayerClass))


typedef struct _ClayLinkLayerClass   ClayLinkLayerClass;
typedef struct _ClayLinkLayerPrivate ClayLinkLayerPrivate;

struct _ClayLinkLayer
{
  ClayLayer             layer;

  ClayLinkLayerPrivate *p;
};

struct _ClayLinkLayerClass
{
  ClayLayerClass  parent_class;
};


GType       clay_link_layer_get_type             (void);

ClayLayer * clay_link_layer_new                  (ClayImage             *image,
                                                  ClayLink              *link);

ClayLink  * clay_link_layer_get_link             (ClayLinkLayer         *layer);
gboolean    clay_link_layer_set_link             (ClayLinkLayer         *layer,
                                                  ClayLink              *link,
                                                  gboolean               push_undo);
gboolean    clay_link_layer_set_link_with_matrix (ClayLinkLayer         *layer,
                                                  ClayLink              *link,
                                                  ClayMatrix3           *matrix,
                                                  ClayInterpolationType  interpolation_type,
                                                  gint                   offset_x,
                                                  gint                   offset_y,
                                                  gboolean               push_undo);

gboolean    clay_link_layer_is_monitored         (ClayLinkLayer         *layer);

gboolean    clay_link_layer_get_transform        (ClayLinkLayer         *layer,
                                                  ClayMatrix3           *matrix,
                                                  gint                  *offset_x,
                                                  gint                  *offset_y,
                                                  ClayInterpolationType *interpolation);
gboolean    clay_link_layer_set_transform        (ClayLinkLayer         *layer,
                                                  ClayMatrix3           *matrix,
                                                  ClayInterpolationType  interpolation_type,
                                                  gboolean               push_undo);

gboolean    clay_item_is_link_layer              (ClayItem              *item);

/* Only to be used for XCF loading/saving. */

guint32     clay_link_layer_get_xcf_flags        (ClayLinkLayer         *layer);
void        clay_link_layer_set_xcf_flags        (ClayLinkLayer         *layer,
                                                  guint32                flags);
