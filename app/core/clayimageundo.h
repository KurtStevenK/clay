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

#include "clayundo.h"


#define CLAY_TYPE_IMAGE_UNDO            (clay_image_undo_get_type ())
#define CLAY_IMAGE_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_UNDO, ClayImageUndo))
#define CLAY_IMAGE_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_UNDO, ClayImageUndoClass))
#define CLAY_IS_IMAGE_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_UNDO))
#define CLAY_IS_IMAGE_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_UNDO))
#define CLAY_IMAGE_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_UNDO, ClayImageUndoClass))


typedef struct _ClayImageUndo      ClayImageUndo;
typedef struct _ClayImageUndoClass ClayImageUndoClass;

struct _ClayImageUndo
{
  ClayUndo           parent_instance;

  ClayImageBaseType  base_type;
  ClayPrecision      precision;
  gint               width;
  gint               height;
  gint               previous_origin_x;
  gint               previous_origin_y;
  gint               previous_width;
  gint               previous_height;
  gdouble            xresolution;
  gdouble            yresolution;
  ClayUnit          *resolution_unit;
  ClayGrid          *grid;
  gint               num_colors;
  ClayPalette       *colormap;
  ClayColorProfile  *hidden_profile;
  ClayMetadata      *metadata;
  gchar             *parasite_name;
  ClayParasite      *parasite;
};

struct _ClayImageUndoClass
{
  ClayUndoClass  parent_class;
};


GType   clay_image_undo_get_type (void);
