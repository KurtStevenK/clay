/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysamplepointeditor.h
 * Copyright (C) 2005-2016 Michael Natterer <mitch@gimp.org>
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

#include "clayimageeditor.h"


#define CLAY_TYPE_SAMPLE_POINT_EDITOR            (clay_sample_point_editor_get_type ())
#define CLAY_SAMPLE_POINT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SAMPLE_POINT_EDITOR, ClaySamplePointEditor))
#define CLAY_SAMPLE_POINT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SAMPLE_POINT_EDITOR, ClaySamplePointEditorClass))
#define CLAY_IS_SAMPLE_POINT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SAMPLE_POINT_EDITOR))
#define CLAY_IS_SAMPLE_POINT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SAMPLE_POINT_EDITOR))
#define CLAY_SAMPLE_POINT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SAMPLE_POINT_EDITOR, ClaySamplePointEditorClass))


typedef struct _ClaySamplePointEditorClass ClaySamplePointEditorClass;

struct _ClaySamplePointEditor
{
  ClayImageEditor  parent_instance;

  GtkWidget       *empty_icon;
  GtkWidget       *empty_label;

  GtkWidget       *grid;
  GtkWidget      **color_frames;
  gint             n_color_frames;

  guint            dirty_idle_id;

  gboolean         sample_merged;
};

struct _ClaySamplePointEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_sample_point_editor_get_type          (void);

GtkWidget * clay_sample_point_editor_new               (ClayMenuFactory *menu_factory);

void        clay_sample_point_editor_set_sample_merged (ClaySamplePointEditor *editor,
                                                        gboolean               sample_merged);
gboolean    clay_sample_point_editor_get_sample_merged (ClaySamplePointEditor *editor);
