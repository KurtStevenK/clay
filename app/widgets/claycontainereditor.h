/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainereditor.h
 * Copyright (C) 2001-2011 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_CONTAINER_EDITOR            (clay_container_editor_get_type ())
#define CLAY_CONTAINER_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_EDITOR, ClayContainerEditor))
#define CLAY_CONTAINER_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_EDITOR, ClayContainerEditorClass))
#define CLAY_IS_CONTAINER_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_EDITOR))
#define CLAY_IS_CONTAINER_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_EDITOR))
#define CLAY_CONTAINER_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_EDITOR, ClayContainerEditorClass))


typedef struct _ClayContainerEditorPrivate ClayContainerEditorPrivate;
typedef struct _ClayContainerEditorClass   ClayContainerEditorClass;

struct _ClayContainerEditor
{
  GtkBox             parent_instance;

  ClayContainerView *view;

  ClayContainerEditorPrivate *priv;
};

struct _ClayContainerEditorClass
{
  GtkBoxClass  parent_class;

  void (* select_item)   (ClayContainerEditor *editor,
                          ClayViewable        *object);
  void (* activate_item) (ClayContainerEditor *editor,
                          ClayViewable        *object);
};


GType            clay_container_editor_get_type           (void);

GtkSelectionMode clay_container_editor_get_selection_mode (ClayContainerEditor *editor);
void             clay_container_editor_set_selection_mode (ClayContainerEditor *editor,
                                                           GtkSelectionMode     mode);

void             clay_container_editor_bind_to_async_set  (ClayContainerEditor *editor,
                                                           ClayAsyncSet        *async_set,
                                                           const gchar         *message);
