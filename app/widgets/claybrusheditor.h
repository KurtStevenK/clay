/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrusheditor.h
 * Copyright 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef  __CLAY_BRUSH_EDITOR_H__
#define  __CLAY_BRUSH_EDITOR_H__


#include "claydataeditor.h"


#define CLAY_TYPE_BRUSH_EDITOR            (clay_brush_editor_get_type ())
#define CLAY_BRUSH_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_EDITOR, ClayBrushEditor))
#define CLAY_BRUSH_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_EDITOR, ClayBrushEditorClass))
#define CLAY_IS_BRUSH_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_EDITOR))
#define CLAY_IS_BRUSH_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_EDITOR))
#define CLAY_BRUSH_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_EDITOR, ClayBrushEditorClass))


typedef struct _ClayBrushEditorClass ClayBrushEditorClass;

struct _ClayBrushEditor
{
  ClayDataEditor  parent_instance;

  GtkWidget      *shape_group;
  GtkWidget      *options_box;
  GtkAdjustment  *radius_data;
  GtkAdjustment  *spikes_data;
  GtkAdjustment  *hardness_data;
  GtkAdjustment  *angle_data;
  GtkAdjustment  *aspect_ratio_data;
  GtkAdjustment  *spacing_data;
};

struct _ClayBrushEditorClass
{
  ClayDataEditorClass  parent_class;
};


GType       clay_brush_editor_get_type (void);

GtkWidget * clay_brush_editor_new      (ClayContext     *context,
                                        ClayMenuFactory *menu_factory);


#endif  /*  __CLAY_BRUSH_EDITOR_H__  */
