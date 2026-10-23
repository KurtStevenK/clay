/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypanedbox.h
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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


#define CLAY_TYPE_PANED_BOX            (clay_paned_box_get_type ())
#define CLAY_PANED_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PANED_BOX, ClayPanedBox))
#define CLAY_PANED_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PANED_BOX, ClayPanedBoxClass))
#define CLAY_IS_PANED_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PANED_BOX))
#define CLAY_IS_PANED_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PANED_BOX))
#define CLAY_PANED_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PANED_BOX, ClayPanedBoxClass))


typedef struct _ClayPanedBoxClass    ClayPanedBoxClass;
typedef struct _ClayPanedBoxPrivate  ClayPanedBoxPrivate;

/**
 * ClayPanedBox:
 *
 * A #GtkBox with the children separated by #GtkPaned:s and basic
 * docking mechanisms.
 */
struct _ClayPanedBox
{
  GtkBox parent_instance;

  ClayPanedBoxPrivate *p;
};

struct _ClayPanedBoxClass
{
  GtkBoxClass parent_class;
};


GType       clay_paned_box_get_type         (void);

GtkWidget * clay_paned_box_new              (gboolean                 homogeneous,
                                             gint                     spacing,
                                             GtkOrientation           orientation);
void        clay_paned_box_set_dropped_cb   (ClayPanedBox            *paned_box,
                                             ClayPanedBoxDroppedFunc  dropped_cb,
                                             gpointer                 dropped_cb_data);
void        clay_paned_box_add_widget       (ClayPanedBox            *paned_box,
                                             GtkWidget               *widget,
                                             gint                     index);
void        clay_paned_box_remove_widget    (ClayPanedBox            *paned_box,
                                             GtkWidget               *widget);
gboolean    clay_paned_box_will_handle_drag (ClayPanedBox            *paned_box,
                                             GtkWidget               *widget,
                                             GdkDragContext          *context,
                                             gint                     x,
                                             gint                     y,
                                             gint                     time);
void        clay_paned_box_set_drag_handler (ClayPanedBox            *paned_box,
                                             ClayPanedBox            *drag_handler);
