/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysizebox.h
 * Copyright (C) 2004 Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_SIZE_BOX            (clay_size_box_get_type ())
#define CLAY_SIZE_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SIZE_BOX, ClaySizeBox))
#define CLAY_SIZE_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SIZE_BOX, ClaySizeBoxClass))
#define CLAY_IS_SIZE_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SIZE_BOX))
#define CLAY_IS_SIZE_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SIZE_BOX))
#define CLAY_SIZE_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SIZE_BOX, ClaySizeBoxClass))


typedef struct _ClaySizeBoxClass  ClaySizeBoxClass;

struct _ClaySizeBox
{
  GtkBox        parent_instance;

  GtkSizeGroup *size_group;

  gint          width;
  gint          height;
  ClayUnit     *unit;
  gdouble       xresolution;
  gdouble       yresolution;
  ClayUnit     *resolution_unit;

  gboolean      edit_resolution;
};

struct _ClaySizeBoxClass
{
  GtkBoxClass  parent_class;
};


GType           clay_size_box_get_type       (void);

ClaySizeEntry * clay_size_box_get_size_entry (ClaySizeBox *size_box);
