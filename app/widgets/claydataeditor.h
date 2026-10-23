/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydataeditor.h
 * Copyright (C) 2002-2004 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_DATA_EDITOR            (clay_data_editor_get_type ())
#define CLAY_DATA_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DATA_EDITOR, ClayDataEditor))
#define CLAY_DATA_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DATA_EDITOR, ClayDataEditorClass))
#define CLAY_IS_DATA_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DATA_EDITOR))
#define CLAY_IS_DATA_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DATA_EDITOR))
#define CLAY_DATA_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DATA_EDITOR, ClayDataEditorClass))


typedef struct _ClayDataEditorClass ClayDataEditorClass;

struct _ClayDataEditor
{
  ClayEditor       parent_instance;

  ClayDataFactory *data_factory;
  ClayContext     *context;
  gboolean         edit_active;

  ClayData        *data;
  gboolean         data_editable;

  GtkWidget       *name_entry;

  GtkWidget       *view; /* filled by subclasses */
};

struct _ClayDataEditorClass
{
  ClayEditorClass  parent_class;

  /*  virtual functions  */
  void (* set_data) (ClayDataEditor *editor,
                     ClayData       *data);

  const gchar *title;
};


GType       clay_data_editor_get_type        (void);

void        clay_data_editor_set_data        (ClayDataEditor *editor,
                                              ClayData       *data);
ClayData  * clay_data_editor_get_data        (ClayDataEditor *editor);

void        clay_data_editor_set_edit_active (ClayDataEditor *editor,
                                              gboolean        edit_active);
gboolean    clay_data_editor_get_edit_active (ClayDataEditor *editor);
