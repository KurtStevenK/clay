/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball
 *
 * claylayermodebox.h
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_LAYER_MODE_BOX            (clay_layer_mode_box_get_type ())
#define CLAY_LAYER_MODE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER_MODE_BOX, ClayLayerModeBox))
#define CLAY_LAYER_MODE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LAYER_MODE_BOX, ClayLayerModeBoxClass))
#define CLAY_IS_LAYER_MODE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER_MODE_BOX))
#define CLAY_IS_LAYER_MODE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LAYER_MODE_BOX))
#define CLAY_LAYER_MODE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LAYER_MODE_BOX, ClayLayerModeBoxClass))


typedef struct _ClayLayerModeBoxPrivate ClayLayerModeBoxPrivate;
typedef struct _ClayLayerModeBoxClass   ClayLayerModeBoxClass;

struct _ClayLayerModeBox
{
  GtkBox                   parent_instance;

  ClayLayerModeBoxPrivate *priv;
};

struct _ClayLayerModeBoxClass
{
  GtkBoxClass  parent_class;
};


GType                  clay_layer_mode_box_get_type      (void);

GtkWidget            * clay_layer_mode_box_new           (ClayLayerModeContext  context);

void                   clay_layer_mode_box_set_context   (ClayLayerModeBox     *box,
                                                          ClayLayerModeContext  context);
ClayLayerModeContext   clay_layer_mode_box_get_context   (ClayLayerModeBox     *box);

void                   clay_layer_mode_box_set_mode      (ClayLayerModeBox     *box,
                                                          ClayLayerMode         mode);
ClayLayerMode          clay_layer_mode_box_get_mode      (ClayLayerModeBox     *box);

void                   clay_layer_mode_box_set_label     (ClayLayerModeBox     *box,
                                                          const gchar          *label);
void                   clay_layer_mode_box_set_ellipsize (ClayLayerModeBox     *box,
                                                          PangoEllipsizeMode    mode);
