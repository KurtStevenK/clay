/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayImageCommentEditor
 * Copyright (C) 2007  Sven Neumann <sven@gimp.org>
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

#include "clayimageparasiteview.h"


#define CLAY_TYPE_IMAGE_COMMENT_EDITOR            (clay_image_comment_editor_get_type ())
#define CLAY_IMAGE_COMMENT_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_COMMENT_EDITOR, ClayImageCommentEditor))
#define CLAY_IMAGE_COMMENT_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_COMMENT_EDITOR, ClayImageCommentEditorClass))
#define CLAY_IS_IMAGE_COMMENT_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_COMMENT_EDITOR))
#define CLAY_IS_IMAGE_COMMENT_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_COMMENT_EDITOR))
#define CLAY_IMAGE_COMMENT_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_COMMENT_EDITOR, ClayImageCommentEditorClass))


typedef struct _ClayImageCommentEditorClass ClayImageCommentEditorClass;

struct _ClayImageCommentEditor
{
  ClayImageParasiteView  parent_instance;

  GtkTextBuffer         *buffer;
  gboolean               recoursing;
};

struct _ClayImageCommentEditorClass
{
  ClayImageParasiteViewClass  parent_class;
};


GType       clay_image_comment_editor_get_type (void);

GtkWidget * clay_image_comment_editor_new      (ClayImage *image);
