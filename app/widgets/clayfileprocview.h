/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfileprocview.h
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


#define CLAY_TYPE_FILE_PROC_VIEW            (clay_file_proc_view_get_type ())
#define CLAY_FILE_PROC_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILE_PROC_VIEW, ClayFileProcView))
#define CLAY_FILE_PROC_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILE_PROC_VIEW, ClayFileProcViewClass))
#define CLAY_IS_FILE_PROC_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILE_PROC_VIEW))
#define CLAY_IS_FILE_PROC_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FILE_PROC_VIEW))
#define CLAY_FILE_PROC_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FILE_PROC_VIEW, ClayFileProcViewClass))


typedef struct _ClayFileProcViewClass ClayFileProcViewClass;

struct _ClayFileProcView
{
  GtkTreeView        parent_instance;

  GList             *meta_extensions;
};

struct _ClayFileProcViewClass
{
  GtkTreeViewClass   parent_class;

  void (* changed) (ClayFileProcView *view);
};


GType                 clay_file_proc_view_get_type    (void);

GtkWidget           * clay_file_proc_view_new         (Clay                    *clay,
                                                       GSList                  *procedures,
                                                       ClayFileProcedureGroup   file_proc_group,
                                                       const gchar             *automatic,
                                                       const gchar             *automatic_help_id);

ClayPlugInProcedure * clay_file_proc_view_get_proc    (ClayFileProcView        *view,
                                                       gchar                  **label,
                                                       GtkFileFilter          **filter);
gboolean              clay_file_proc_view_set_proc    (ClayFileProcView        *view,
                                                       ClayPlugInProcedure     *proc);

gchar               * clay_file_proc_view_get_help_id (ClayFileProcView        *view);
