/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_UI_H_INSIDE__) && !defined (CLAY_COMPILATION)
#error "Only <libclay/clayui.h> can be included directly."
#endif

#ifndef __CLAY_RESOURCE_CHOOSER_H__
#define __CLAY_RESOURCE_CHOOSER_H__

G_BEGIN_DECLS

#define CLAY_TYPE_RESOURCE_CHOOSER (clay_resource_chooser_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayResourceChooser, clay_resource_chooser, CLAY, RESOURCE_CHOOSER, GtkBox)

struct _ClayResourceChooserClass
{
  GtkBoxClass parent_class;

  /* Signals */
  void (* resource_set) (ClayResourceChooser *chooser,
                         ClayResource        *resource,
                         gboolean             dialog_closing);

  /* Abstract methods and class variables */
  void (*draw_interior) (ClayResourceChooser *chooser);

  GType resource_type;

  /* Padding for future expansion */
  gpointer padding[8];
};

ClayResource * clay_resource_chooser_get_resource     (ClayResourceChooser  *chooser);
void           clay_resource_chooser_set_resource     (ClayResourceChooser  *chooser,
                                                       ClayResource         *resource);
GtkWidget    * clay_resource_chooser_get_label        (ClayResourceChooser  *widget);


/* API from below, used by subclasses e.g. ClayBrushChooser */

G_GNUC_INTERNAL void  _clay_resource_chooser_set_drag_target (ClayResourceChooser  *chooser,
                                                              GtkWidget            *drag_region_widget,
                                                              const GtkTargetEntry *drag_target);
G_GNUC_INTERNAL void  _clay_resource_chooser_set_clickable   (ClayResourceChooser  *chooser,
                                                              GtkWidget            *widget);

G_END_DECLS

#endif /* __CLAY_RESOURCE_CHOOSER_H__ */
