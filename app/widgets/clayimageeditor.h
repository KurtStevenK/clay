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

#include "clayeditor.h"


#define CLAY_TYPE_IMAGE_EDITOR            (clay_image_editor_get_type ())
#define CLAY_IMAGE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_EDITOR, ClayImageEditor))
#define CLAY_IMAGE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_EDITOR, ClayImageEditorClass))
#define CLAY_IS_IMAGE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_EDITOR))
#define CLAY_IS_IMAGE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_EDITOR))
#define CLAY_IMAGE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_EDITOR, ClayImageEditorClass))


typedef struct _ClayImageEditorClass ClayImageEditorClass;

struct _ClayImageEditor
{
  ClayEditor   parent_instance;

  ClayContext *context;
  ClayImage   *image;
};

struct _ClayImageEditorClass
{
  ClayEditorClass  parent_class;

  /*  virtual function  */
  void (* set_image) (ClayImageEditor *editor,
                      ClayImage       *image);
};


GType       clay_image_editor_get_type  (void);

void        clay_image_editor_set_image (ClayImageEditor *editor,
                                         ClayImage       *image);
ClayImage * clay_image_editor_get_image (ClayImageEditor *editor);
