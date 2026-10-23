/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claynavigationeditor.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@gimp.org>
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

#include "widgets/clayeditor.h"


#define CLAY_TYPE_NAVIGATION_EDITOR            (clay_navigation_editor_get_type ())
#define CLAY_NAVIGATION_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_NAVIGATION_EDITOR, ClayNavigationEditor))
#define CLAY_NAVIGATION_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_NAVIGATION_EDITOR, ClayNavigationEditorClass))
#define CLAY_IS_NAVIGATION_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_NAVIGATION_EDITOR))
#define CLAY_IS_NAVIGATION_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_NAVIGATION_EDITOR))
#define CLAY_NAVIGATION_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_NAVIGATION_EDITOR, ClayNavigationEditorClass))


typedef struct _ClayNavigationEditorClass  ClayNavigationEditorClass;

struct _ClayNavigationEditor
{
  ClayEditor        parent_instance;

  ClayContext      *context;
  ClayDisplayShell *shell;

  ClayImageProxy   *image_proxy;

  GtkWidget        *view;
  GtkWidget        *zoom_label;
  GtkAdjustment    *zoom_adjustment;

  GtkWidget        *zoom_out_button;
  GtkWidget        *zoom_in_button;
  GtkWidget        *zoom_100_button;
  GtkWidget        *zoom_fit_in_button;
  GtkWidget        *zoom_fill_button;
  GtkWidget        *shrink_wrap_button;

  guint             scale_timeout;
};

struct _ClayNavigationEditorClass
{
  ClayEditorClass  parent_class;
};


GType       clay_navigation_editor_get_type  (void);

GtkWidget * clay_navigation_editor_new       (ClayMenuFactory  *menu_factory);
void        clay_navigation_editor_popup     (ClayDisplayShell *shell,
                                              GtkWidget        *widget,
                                              GdkEvent         *event,
                                              gint              click_x,
                                              gint              click_y);
