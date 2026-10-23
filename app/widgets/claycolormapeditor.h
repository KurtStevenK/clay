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

#include "clayimageeditor.h"


#define CLAY_TYPE_COLORMAP_EDITOR            (clay_colormap_editor_get_type ())
#define CLAY_COLORMAP_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLORMAP_EDITOR, ClayColormapEditor))
#define CLAY_COLORMAP_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLORMAP_EDITOR, ClayColormapEditorClass))
#define CLAY_IS_COLORMAP_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLORMAP_EDITOR))
#define CLAY_IS_COLORMAP_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLORMAP_EDITOR))
#define CLAY_COLORMAP_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLORMAP_EDITOR, ClayColormapEditorClass))


typedef struct _ClayColormapEditorClass ClayColormapEditorClass;

struct _ClayColormapEditor
{
  ClayImageEditor  parent_instance;

  GtkWidget       *selection;
  GtkWidget       *color_dialog;
};

struct _ClayColormapEditorClass
{
  ClayImageEditorClass  parent_class;
};


GType       clay_colormap_editor_get_type           (void);

GtkWidget * clay_colormap_editor_new                (ClayMenuFactory    *menu_factory);

void        clay_colormap_editor_edit_color         (ClayColormapEditor *editor);
void        clay_colormap_editor_delete_color       (ClayColormapEditor *editor);
gboolean    clay_colormap_editor_is_color_deletable (ClayColormapEditor *editor);

gint        clay_colormap_editor_get_index          (ClayColormapEditor *editor,
                                                     GeglColor          *search);
gboolean    clay_colormap_editor_set_index          (ClayColormapEditor *editor,
                                                     gint                index,
                                                     GeglColor          *color);

gint        clay_colormap_editor_max_index          (ClayColormapEditor *editor);
