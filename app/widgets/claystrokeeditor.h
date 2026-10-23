/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claystrokeeditor.h
 * Copyright (C) 2003 Sven Neumann <sven@gimp.org>
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

#include "clayfilleditor.h"


#define CLAY_TYPE_STROKE_EDITOR            (clay_stroke_editor_get_type ())
#define CLAY_STROKE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_STROKE_EDITOR, ClayStrokeEditor))
#define CLAY_STROKE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_STROKE_EDITOR, ClayStrokeEditorClass))
#define CLAY_IS_STROKE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_STROKE_EDITOR))
#define CLAY_IS_STROKE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_STROKE_EDITOR))
#define CLAY_STROKE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_STROKE_EDITOR, ClayStrokeEditorClass))


typedef struct _ClayStrokeEditorClass ClayStrokeEditorClass;

struct _ClayStrokeEditor
{
  ClayFillEditor  parent_instance;

  gdouble         resolution;
};

struct _ClayStrokeEditorClass
{
  ClayFillEditorClass  parent_class;
};


GType       clay_stroke_editor_get_type (void);

GtkWidget * clay_stroke_editor_new      (ClayStrokeOptions *options,
                                         gdouble            resolution,
                                         gboolean           edit_context,
                                         gboolean           use_custom_style);
