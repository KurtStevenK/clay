/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerentry.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayviewable.h"

#include "claycellrendererviewable.h"
#include "claycontainerentry.h"
#include "claycontainertreestore.h"
#include "claycontainerview.h"
#include "clayviewrenderer.h"


static void     clay_container_entry_view_iface_init (ClayContainerViewInterface *iface);

static void     clay_container_entry_finalize     (GObject                *object);

static void     clay_container_entry_set_context  (ClayContainerView      *view,
                                                   ClayContext            *context);
static void     clay_container_entry_set_view_size(ClayContainerView      *view);
static gboolean clay_container_entry_set_selected (ClayContainerView      *view,
                                                   GList                  *items);
static gint     clay_container_entry_get_selected (ClayContainerView      *view,
                                                   GList                 **items);

static gpointer clay_container_entry_insert_item  (ClayContainerView      *view,
                                                   ClayViewable           *viewable,
                                                   gpointer                parent_insert_data,
                                                   gint                    index);
static void     clay_container_entry_remove_item  (ClayContainerView      *view,
                                                   ClayViewable           *viewable,
                                                   gpointer                insert_data);
static void     clay_container_entry_reorder_item (ClayContainerView      *view,
                                                   ClayViewable           *viewable,
                                                   gint                    new_index,
                                                   gpointer                insert_data);
static void     clay_container_entry_rename_item  (ClayContainerView      *view,
                                                   ClayViewable           *viewable,
                                                   gpointer                insert_data);
static void     clay_container_entry_clear_items  (ClayContainerView      *view);

static void     clay_container_entry_changed      (GtkEntry               *entry,
                                                   ClayContainerView      *view);
static gboolean clay_container_entry_match_selected (GtkEntryCompletion     *widget,
                                                     GtkTreeModel           *model,
                                                     GtkTreeIter            *iter,
                                                     ClayContainerView      *view);


G_DEFINE_TYPE_WITH_CODE (ClayContainerEntry, clay_container_entry,
                         GTK_TYPE_ENTRY,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_container_entry_view_iface_init))

#define parent_class clay_container_entry_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_container_entry_class_init (ClayContainerEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = _clay_container_view_set_property;
  object_class->get_property = _clay_container_view_get_property;
  object_class->finalize     = clay_container_entry_finalize;

  _clay_container_view_install_properties (object_class);
}

static void
clay_container_entry_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  if (! parent_view_iface)
    parent_view_iface = g_type_default_interface_peek (CLAY_TYPE_CONTAINER_VIEW);

  iface->set_context   = clay_container_entry_set_context;
  iface->set_view_size = clay_container_entry_set_view_size;
  iface->set_selected  = clay_container_entry_set_selected;
  iface->get_selected  = clay_container_entry_get_selected;

  iface->insert_item   = clay_container_entry_insert_item;
  iface->remove_item   = clay_container_entry_remove_item;
  iface->reorder_item  = clay_container_entry_reorder_item;
  iface->rename_item   = clay_container_entry_rename_item;
  iface->clear_items   = clay_container_entry_clear_items;

  iface->insert_data_free = (GDestroyNotify) gtk_tree_iter_free;
}

static void
clay_container_entry_init (ClayContainerEntry *entry)
{
  GtkEntryCompletion *completion;
  GtkTreeModel       *model;
  GtkCellRenderer    *cell;
  GType               types[CLAY_CONTAINER_TREE_STORE_N_COLUMNS];
  gint                n_types = 0;

  entry->viewable = NULL;

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "inline-completion",  FALSE,
                             "popup-single-match", TRUE,
                             "popup-set-width",    FALSE,
                             NULL);

  clay_container_tree_store_columns_init (types, &n_types);

  model = clay_container_tree_store_new (CLAY_CONTAINER_VIEW (entry),
                                         n_types, types);
  clay_container_tree_store_set_use_name (CLAY_CONTAINER_TREE_STORE (model),
                                          TRUE);

  gtk_entry_completion_set_model (completion, model);
  g_object_unref (model);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);

  g_signal_connect (completion, "match-selected",
                    G_CALLBACK (clay_container_entry_match_selected),
                    entry);

  g_object_unref (completion);

  /*  FIXME: This can be done better with GTK+ 2.6.  */

  cell = clay_cell_renderer_viewable_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "renderer",
                                  CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER,
                                  NULL);

  clay_container_tree_store_add_renderer_cell (CLAY_CONTAINER_TREE_STORE (model),
                                               cell, -1);

  gtk_entry_completion_set_text_column (completion,
                                        CLAY_CONTAINER_TREE_STORE_COLUMN_NAME);

  g_signal_connect (entry, "changed",
                    G_CALLBACK (clay_container_entry_changed),
                    entry);
}

GtkWidget *
clay_container_entry_new (ClayContainer *container,
                          ClayContext   *context,
                          gint           view_size,
                          gint           view_border_width)
{
  g_return_val_if_fail (container == NULL || CLAY_IS_CONTAINER (container),
                        NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_CONTAINER_ENTRY,
                       "view-size",         view_size,
                       "view-border-width", view_border_width,
                       "context",           context,
                       "container",         container,
                       NULL);
}


/*  ClayContainerView methods  */

static GtkTreeModel *
clay_container_entry_get_model (ClayContainerView *view)
{
  GtkEntryCompletion *completion;

  completion = gtk_entry_get_completion (GTK_ENTRY (view));

  if (completion)
    return gtk_entry_completion_get_model (completion);

  return NULL;
}

static void
clay_container_entry_finalize (GObject *object)
{
  ClayContainerEntry *entry = CLAY_CONTAINER_ENTRY (object);

  g_clear_weak_pointer (&entry->viewable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_container_entry_set_context (ClayContainerView *view,
                                  ClayContext       *context)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  parent_view_iface->set_context (view, context);

  if (model)
    clay_container_tree_store_set_context (CLAY_CONTAINER_TREE_STORE (model),
                                           context);
}

static void
clay_container_entry_set_view_size (ClayContainerView *view)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  clay_container_tree_store_set_view_size (CLAY_CONTAINER_TREE_STORE (model));
}

static gboolean
clay_container_entry_set_selected (ClayContainerView   *view,
                                   GList               *viewables)
{
  ClayContainerEntry *container_entry = CLAY_CONTAINER_ENTRY (view);
  GtkEntry           *entry           = GTK_ENTRY (view);
  ClayViewable       *viewable        = NULL;

  /* XXX Only support 1 selected viewable for now. */
  if (viewables)
    viewable = viewables->data;

  g_set_weak_pointer (&container_entry->viewable, viewable);

  if (viewable)
    {
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         NULL);
    }
  else
    {
      /* The selected item does not exist. */
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         CLAY_ICON_WILBER_EEK);
    }

  gtk_entry_set_text (entry, viewable ? clay_object_get_name (viewable) : "");

  return TRUE;
}

static gint
clay_container_entry_get_selected (ClayContainerView  *view,
                                   GList             **items)
{
  ClayContainerEntry *container_entry = CLAY_CONTAINER_ENTRY (view);

  if (items)
    {
      if (container_entry->viewable)
        *items = g_list_prepend (NULL, container_entry->viewable);
      else
        *items = NULL;
    }

  return container_entry->viewable ? 1 : 0;
}

static gpointer
clay_container_entry_insert_item (ClayContainerView *view,
                                  ClayViewable      *viewable,
                                  gpointer           parent_insert_data,
                                  gint               index)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  return clay_container_tree_store_insert_item (CLAY_CONTAINER_TREE_STORE (model),
                                                viewable,
                                                parent_insert_data,
                                                index);
}

static void
clay_container_entry_remove_item (ClayContainerView *view,
                                  ClayViewable      *viewable,
                                  gpointer           insert_data)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  clay_container_tree_store_remove_item (CLAY_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static void
clay_container_entry_reorder_item (ClayContainerView *view,
                                   ClayViewable      *viewable,
                                   gint               new_index,
                                   gpointer           insert_data)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  clay_container_tree_store_reorder_item (CLAY_CONTAINER_TREE_STORE (model),
                                          viewable,
                                          new_index,
                                          insert_data);
}

static void
clay_container_entry_rename_item (ClayContainerView *view,
                                  ClayViewable      *viewable,
                                  gpointer           insert_data)
{
  ClayContainerEntry *container_entry = CLAY_CONTAINER_ENTRY (view);
  GtkEntry           *entry           = GTK_ENTRY (view);
  GtkTreeModel       *model           = clay_container_entry_get_model (view);

  if (viewable == container_entry->viewable)
    {
      g_signal_handlers_block_by_func (entry,
                                       clay_container_entry_changed,
                                       view);

      gtk_entry_set_text (entry, clay_object_get_name (viewable));

      g_signal_handlers_unblock_by_func (entry,
                                         clay_container_entry_changed,
                                         view);
    }

  clay_container_tree_store_rename_item (CLAY_CONTAINER_TREE_STORE (model),
                                         viewable,
                                         insert_data);
}

static void
clay_container_entry_clear_items (ClayContainerView *view)
{
  GtkTreeModel *model = clay_container_entry_get_model (view);

  /* model is NULL in dispose() */
  if (model)
    clay_container_tree_store_clear_items (CLAY_CONTAINER_TREE_STORE (model));

  parent_view_iface->clear_items (view);
}

static void
clay_container_entry_changed (GtkEntry          *entry,
                              ClayContainerView *view)
{
  ClayContainerEntry *container_entry = CLAY_CONTAINER_ENTRY (entry);
  ClayContainer      *container       = clay_container_view_get_container (view);
  ClayObject         *object;
  const gchar        *text;

  if (! container)
    return;

  text = gtk_entry_get_text (entry);

  /* don't change a valid selection. If we arrive here after selecting an entry from the list, don't do anything.
   * If we arrive here after typing the full resource name, look for it in the container and set it.
   * There is a difference, because some resources share the same display name e.g. 2 different fonts named "Holiday",
   * it's not possible to get the correct one from the container based on display name, as it's not unique.
  */
  if (container_entry->viewable == NULL || g_strcmp0 (clay_object_get_name (container_entry->viewable), text))
    {
      object = clay_container_get_child_by_name (container, text);

      g_set_weak_pointer (&container_entry->viewable, CLAY_VIEWABLE (object));

      _clay_container_view_selection_changed (view);
    }

  if (container_entry->viewable)
    {
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         NULL);
    }
  else
    {
      /* While editing the entry, show EEK for non-existent item. */
      gtk_entry_set_icon_from_icon_name (entry,
                                         GTK_ENTRY_ICON_SECONDARY,
                                         CLAY_ICON_WILBER_EEK);
    }
}

static gboolean
clay_container_entry_match_selected (GtkEntryCompletion *widget,
                                     GtkTreeModel       *model,
                                     GtkTreeIter        *iter,
                                     ClayContainerView  *view)
{
  ClayContainerEntry *container_entry = CLAY_CONTAINER_ENTRY (GTK_ENTRY (view));
  ClayViewRenderer   *renderer;

  gtk_tree_model_get (model, iter,
                      CLAY_CONTAINER_TREE_STORE_COLUMN_RENDERER, &renderer,
                      -1);
  g_set_weak_pointer (&container_entry->viewable, renderer->viewable);
  g_object_unref (renderer);

  _clay_container_view_selection_changed (view);

  return FALSE;
}
