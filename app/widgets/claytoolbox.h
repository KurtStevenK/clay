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

#include "claydock.h"


#define CLAY_TYPE_TOOLBOX            (clay_toolbox_get_type ())
#define CLAY_TOOLBOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOLBOX, ClayToolbox))
#define CLAY_TOOLBOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOLBOX, ClayToolboxClass))
#define CLAY_IS_TOOLBOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOLBOX))
#define CLAY_IS_TOOLBOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOLBOX))
#define CLAY_TOOLBOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOLBOX, ClayToolboxClass))


typedef struct _ClayToolboxClass   ClayToolboxClass;
typedef struct _ClayToolboxPrivate ClayToolboxPrivate;

struct _ClayToolbox
{
  ClayDock parent_instance;

  ClayToolboxPrivate *p;
};

struct _ClayToolboxClass
{
  ClayDockClass parent_class;
};


GType               clay_toolbox_get_type           (void);
GtkWidget         * clay_toolbox_new                (ClayDialogFactory *factory,
                                                     ClayContext       *context,
                                                     ClayUIManager     *ui_manager);
ClayContext       * clay_toolbox_get_context        (ClayToolbox       *toolbox);
void                clay_toolbox_set_drag_handler   (ClayToolbox       *toolbox,
                                                     ClayPanedBox      *drag_handler);
