/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayactionview.h
 * Copyright (C) 2004-2005  Michael Natterer <mitch@gimp.org>
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


enum
{
  CLAY_ACTION_VIEW_COLUMN_VISIBLE,
  CLAY_ACTION_VIEW_COLUMN_ACTION,
  CLAY_ACTION_VIEW_COLUMN_ICON_NAME,
  CLAY_ACTION_VIEW_COLUMN_LABEL,
  CLAY_ACTION_VIEW_COLUMN_LABEL_CASEFOLD,
  CLAY_ACTION_VIEW_COLUMN_NAME,
  CLAY_ACTION_VIEW_COLUMN_ACCEL_KEY,
  CLAY_ACTION_VIEW_COLUMN_ACCEL_MASK,
  CLAY_ACTION_VIEW_N_COLUMNS
};


#define CLAY_TYPE_ACTION_VIEW            (clay_action_view_get_type ())
#define CLAY_ACTION_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ACTION_VIEW, ClayActionView))
#define CLAY_ACTION_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ACTION_VIEW, ClayActionViewClass))
#define CLAY_IS_ACTION_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ACTION_VIEW))
#define CLAY_IS_ACTION_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ACTION_VIEW))
#define CLAY_ACTION_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ACTION_VIEW, ClayActionViewClass))


typedef struct _ClayActionViewClass ClayActionViewClass;

struct _ClayActionView
{
  GtkTreeView    parent_instance;

  Clay          *clay;
  gboolean       show_shortcuts;

  gchar         *filter;
};

struct _ClayActionViewClass
{
  GtkTreeViewClass  parent_class;
};


GType       clay_action_view_get_type   (void);

GtkWidget * clay_action_view_new        (Clay           *clay,
                                         const gchar    *select_action,
                                         gboolean        show_shortcuts);

void        clay_action_view_set_filter (ClayActionView *view,
                                         const gchar    *filter);
