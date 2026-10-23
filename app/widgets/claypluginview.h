/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginview.h
 * Copyright (C) 2017  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PLUG_IN_VIEW            (clay_plug_in_view_get_type ())
#define CLAY_PLUG_IN_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUG_IN_VIEW, ClayPlugInView))
#define CLAY_PLUG_IN_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUG_IN_VIEW, ClayPlugInViewClass))
#define CLAY_IS_PLUG_IN_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUG_IN_VIEW))
#define CLAY_IS_PLUG_IN_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUG_IN_VIEW))
#define CLAY_PLUG_IN_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PLUG_IN_VIEW, ClayPlugInViewClass))


typedef struct _ClayPlugInViewClass ClayPlugInViewClass;

struct _ClayPlugInView
{
  GtkTreeView  parent_instance;

  GHashTable  *plug_in_hash;
};

struct _ClayPlugInViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (ClayPlugInView *view);
};


GType       clay_plug_in_view_get_type    (void);

GtkWidget * clay_plug_in_view_new         (GSList         *procedures);

gchar     * clay_plug_in_view_get_plug_in (ClayPlugInView *view);
gboolean    clay_plug_in_view_set_plug_in (ClayPlugInView *view,
                                           const gchar    *path);
