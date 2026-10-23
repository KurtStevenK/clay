/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayblobeditor.h
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

#ifndef  __CLAY_BLOB_EDITOR_H__
#define  __CLAY_BLOB_EDITOR_H__


#define CLAY_TYPE_BLOB_EDITOR            (clay_blob_editor_get_type ())
#define CLAY_BLOB_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BLOB_EDITOR, ClayBlobEditor))
#define CLAY_BLOB_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BLOB_EDITOR, ClayBlobEditorClass))
#define CLAY_IS_BLOB_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BLOB_EDITOR))
#define CLAY_IS_BLOB_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BLOB_EDITOR))
#define CLAY_BLOB_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BLOB_EDITOR, ClayBlobEditorClass))


typedef struct _ClayBlobEditorClass ClayBlobEditorClass;

struct _ClayBlobEditor
{
  GtkDrawingArea       parent_instance;

  ClayInkBlobType      type;
  gdouble              aspect;
  gdouble              angle;

  /*<  private  >*/
  gboolean             in_handle;
  gboolean             active;
};

struct _ClayBlobEditorClass
{
  GtkDrawingAreaClass  parent_class;
};


GType       clay_blob_editor_get_type (void);

GtkWidget * clay_blob_editor_new      (ClayInkBlobType  type,
                                       gdouble          aspect,
                                       gdouble          angle);


#endif  /*  __CLAY_BLOB_EDITOR_H__  */
