/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypluginview.c
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

#include "config.h"

#include <gtk/gtk.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "widgets-types.h"

#include "plug-in/claypluginprocedure.h"

#include "claypluginview.h"

#include "clay-intl.h"


enum
{
  COLUMN_FILE,
  COLUMN_PATH,
  N_COLUMNS
};

enum
{
  CHANGED,
  LAST_SIGNAL
};


static void  clay_plug_in_view_finalize          (GObject          *object);

static void  clay_plug_in_view_selection_changed (GtkTreeSelection *selection,
                                                  ClayPlugInView   *view);


G_DEFINE_TYPE (ClayPlugInView, clay_plug_in_view, GTK_TYPE_TREE_VIEW)

#define parent_class clay_plug_in_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
clay_plug_in_view_class_init (ClayPlugInViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = clay_plug_in_view_finalize;

  klass->changed         = NULL;

  view_signals[CHANGED] = g_signal_new ("changed",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (ClayPlugInViewClass,
                                                         changed),
                                        NULL, NULL, NULL,
                                        G_TYPE_NONE, 0);
}

static void
clay_plug_in_view_init (ClayPlugInView *view)
{
  view->plug_in_hash = g_hash_table_new_full (g_file_hash,
                                              (GEqualFunc) g_file_equal,
                                              (GDestroyNotify) g_object_unref,
                                              (GDestroyNotify) g_free);
}

static void
clay_plug_in_view_finalize (GObject *object)
{
  ClayPlugInView *view = CLAY_PLUG_IN_VIEW (object);

  g_clear_pointer (&view->plug_in_hash, g_hash_table_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
clay_plug_in_view_new (GSList *procedures)
{
  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkListStore      *store;
  GSList            *list;

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_FILE,    /* COLUMN_FILE */
                              G_TYPE_STRING); /* COLUMN_PATH */

  view = g_object_new (CLAY_TYPE_PLUG_IN_VIEW,
                       "model", store,
                       NULL);

  g_object_unref (store);

  for (list = procedures; list; list = g_slist_next (list))
    {
      ClayPlugInProcedure *proc = list->data;
      GFile               *file = clay_plug_in_procedure_get_file (proc);

      if (! g_hash_table_lookup (CLAY_PLUG_IN_VIEW (view)->plug_in_hash, file))
        {
          GtkTreeIter  iter;
          gchar       *path = clay_file_get_config_path (file, NULL);

          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              COLUMN_FILE, file,
                              COLUMN_PATH, path,
                              -1);

          g_free (path);

          g_hash_table_insert (CLAY_PLUG_IN_VIEW (view)->plug_in_hash,
                               g_object_ref (file),
                               g_memdup2 (&iter, sizeof (GtkTreeIter)));
        }
    }

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Plug-In"));
  gtk_tree_view_column_set_expand (column, TRUE);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", COLUMN_PATH,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  g_signal_connect (gtk_tree_view_get_selection (view), "changed",
                    G_CALLBACK (clay_plug_in_view_selection_changed),
                    view);

  return GTK_WIDGET (view);
}

gchar *
clay_plug_in_view_get_plug_in (ClayPlugInView *view)
{
  GtkTreeModel     *model;
  GtkTreeSelection *selection;
  GtkTreeIter       iter;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_VIEW (view), NULL);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *path;

      gtk_tree_model_get (model, &iter,
                          COLUMN_PATH,  &path,
                          -1);

      return path;
    }

  return NULL;
}

gboolean
clay_plug_in_view_set_plug_in (ClayPlugInView *view,
                               const gchar    *path)
{
  GFile            *file;
  GtkTreeIter      *iter;
  GtkTreeSelection *selection;

  g_return_val_if_fail (CLAY_IS_PLUG_IN_VIEW (view), FALSE);

  file = clay_file_new_for_config_path (path, NULL);

  iter = g_hash_table_lookup (view->plug_in_hash, file);

  g_object_unref (file);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (iter)
    {
      gtk_tree_selection_select_iter (selection, iter);

      return TRUE;
    }

  gtk_tree_selection_unselect_all (selection);

  return FALSE;
}

static void
clay_plug_in_view_selection_changed (GtkTreeSelection *selection,
                                     ClayPlugInView   *view)
{
  g_signal_emit (view, view_signals[CHANGED], 0);
}
