/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextLayer
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "core/claylayer.h"


#define CLAY_TYPE_TEXT_LAYER            (clay_text_layer_get_type ())
#define CLAY_TEXT_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_LAYER, ClayTextLayer))
#define CLAY_TEXT_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_LAYER, ClayTextLayerClass))
#define CLAY_IS_TEXT_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_LAYER))
#define CLAY_IS_TEXT_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_LAYER))
#define CLAY_TEXT_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEXT_LAYER, ClayTextLayerClass))


typedef struct _ClayTextLayerClass   ClayTextLayerClass;
typedef struct _ClayTextLayerPrivate ClayTextLayerPrivate;

struct _ClayTextLayer
{
  ClayLayer     layer;

  ClayText     *text;
  const gchar  *text_parasite;        /* parasite name that this text was set from,
                                       * and that should be removed when the text
                                       * is changed.
                                       */
  gboolean      text_parasite_is_old; /* Format before XCF 19. */
  gboolean      auto_rename;

  const Babl   *convert_format;

  ClayTextLayerPrivate *private;
};

struct _ClayTextLayerClass
{
  ClayLayerClass  parent_class;
};


GType             clay_text_layer_get_type    (void);

ClayLayer       * clay_text_layer_new         (ClayImage     *image,
                                               ClayText      *text);
ClayText        * clay_text_layer_get_text    (ClayTextLayer *layer);
void              clay_text_layer_set_text    (ClayTextLayer *layer,
                                               ClayText      *text);
void              clay_text_layer_set         (ClayTextLayer *layer,
                                               const gchar   *undo_desc,
                                               const gchar   *first_property_name,
                                               ...) G_GNUC_NULL_TERMINATED;

gboolean          clay_item_is_text_layer     (ClayItem      *item);
ClayTextLayout  * clay_text_layer_get_layout  (ClayTextLayer *layer);

void        clay_text_layer_set_style_overlay_position
                                        (ClayTextLayer *layer,
                                         gboolean       positioned,
                                         gdouble        x,
                                         gdouble        y);
gboolean    clay_text_layer_get_style_overlay_position
                                        (ClayTextLayer *layer,
                                         gdouble       *x,
                                         gdouble       *y);
gboolean    clay_text_layer_is_style_overlay_positioned
                                        (ClayTextLayer *layer);
void        clay_text_layer_set_style_overlay_offset
                                        (ClayTextLayer *layer,
                                         gdouble        offset_x,
                                         gdouble        offset_y);
void        clay_text_layer_get_style_overlay_offset
                                        (ClayTextLayer *layer,
                                         gdouble       *offset_x,
                                         gdouble       *offset_y);
