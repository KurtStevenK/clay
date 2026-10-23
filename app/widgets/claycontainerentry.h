/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerentry.h
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_CONTAINER_ENTRY            (clay_container_entry_get_type ())
#define CLAY_CONTAINER_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_ENTRY, ClayContainerEntry))
#define CLAY_CONTAINER_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_ENTRY, ClayContainerEntryClass))
#define CLAY_IS_CONTAINER_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_ENTRY))
#define CLAY_IS_CONTAINER_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_ENTRY))
#define CLAY_CONTAINER_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_ENTRY, ClayContainerEntryClass))


typedef struct _ClayContainerEntryClass  ClayContainerEntryClass;

struct _ClayContainerEntry
{
  GtkEntry        parent_instance;

  ClayViewable   *viewable;
};

struct _ClayContainerEntryClass
{
  GtkEntryClass   parent_class;
};


GType       clay_container_entry_get_type (void);

GtkWidget * clay_container_entry_new      (ClayContainer *container,
                                           ClayContext   *context,
                                           gint           view_size,
                                           gint           view_border_width);
