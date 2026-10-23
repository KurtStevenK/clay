/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockcolumns.c
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "menus/menus.h"

#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockable.h"
#include "claydockbook.h"
#include "claydockcolumns.h"
#include "claymenudock.h"
#include "claypanedbox.h"
#include "claytoolbox.h"
#include "clayuimanager.h"

#include "clay-log.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER
};

enum
{
  DOCK_ADDED,
  DOCK_REMOVED,
  LAST_SIGNAL
};


struct _ClayDockColumnsPrivate
{
  ClayContext       *context;
  ClayDialogFactory *dialog_factory;
  ClayUIManager     *ui_manager;

  GList             *docks;

  GtkWidget         *paned_hbox;
};


static void      clay_dock_columns_dispose           (GObject         *object);
static void      clay_dock_columns_set_property      (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void      clay_dock_columns_get_property      (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);
static gboolean  clay_dock_columns_dropped_cb        (GtkWidget       *notebook,
                                                      GtkWidget       *child,
                                                      gint             insert_index,
                                                      gpointer         data);
static void      clay_dock_columns_real_dock_added   (ClayDockColumns *dock_columns,
                                                      ClayDock        *dock);
static void      clay_dock_columns_real_dock_removed (ClayDockColumns *dock_columns,
                                                      ClayDock        *dock);
static void      clay_dock_columns_dock_book_removed (ClayDockColumns *dock_columns,
                                                      ClayDockbook    *dockbook,
                                                      ClayDock        *dock);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDockColumns, clay_dock_columns, GTK_TYPE_BOX)

#define parent_class clay_dock_columns_parent_class

static guint dock_columns_signals[LAST_SIGNAL] = { 0 };


static void
clay_dock_columns_class_init (ClayDockColumnsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = clay_dock_columns_dispose;
  object_class->set_property = clay_dock_columns_set_property;
  object_class->get_property = clay_dock_columns_get_property;

  klass->dock_added   = clay_dock_columns_real_dock_added;
  klass->dock_removed = clay_dock_columns_real_dock_removed;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DIALOG_FACTORY,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER,
                                   g_param_spec_object ("ui-manager",
                                                        NULL, NULL,
                                                        CLAY_TYPE_UI_MANAGER,
                                                        CLAY_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  dock_columns_signals[DOCK_ADDED] =
    g_signal_new ("dock-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockColumnsClass, dock_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCK);

  dock_columns_signals[DOCK_REMOVED] =
    g_signal_new ("dock-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockColumnsClass, dock_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCK);
}

static void
clay_dock_columns_init (ClayDockColumns *dock_columns)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock_columns),
                                  GTK_ORIENTATION_HORIZONTAL);

  dock_columns->p = clay_dock_columns_get_instance_private (dock_columns);

  dock_columns->p->paned_hbox = clay_paned_box_new (FALSE, 0,
                                                    GTK_ORIENTATION_HORIZONTAL);
  clay_paned_box_set_dropped_cb (CLAY_PANED_BOX (dock_columns->p->paned_hbox),
                                 clay_dock_columns_dropped_cb,
                                 dock_columns);
  gtk_box_pack_start (GTK_BOX (dock_columns), dock_columns->p->paned_hbox,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (dock_columns->p->paned_hbox, TRUE);
}

static void
clay_dock_columns_dispose (GObject *object)
{
  ClayDockColumns *dock_columns = CLAY_DOCK_COLUMNS (object);

  while (dock_columns->p->docks)
    {
      ClayDock *dock = dock_columns->p->docks->data;

      g_object_ref (dock);
      clay_dock_columns_remove_dock (dock_columns, dock);
      gtk_widget_destroy (GTK_WIDGET (dock));
      g_object_unref (dock);
    }

  g_clear_weak_pointer (&dock_columns->p->context);
  g_clear_weak_pointer (&dock_columns->p->dialog_factory);
  g_clear_weak_pointer (&dock_columns->p->ui_manager);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dock_columns_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayDockColumns *dock_columns = CLAY_DOCK_COLUMNS (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_set_weak_pointer (&dock_columns->p->context,
                          g_value_get_object (value));
      break;

    case PROP_DIALOG_FACTORY:
      g_set_weak_pointer (&dock_columns->p->dialog_factory,
                          g_value_get_object (value));
      break;

    case PROP_UI_MANAGER:
      g_set_weak_pointer (&dock_columns->p->ui_manager,
                          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dock_columns_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayDockColumns *dock_columns = CLAY_DOCK_COLUMNS (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock_columns->p->context);
      break;
    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock_columns->p->dialog_factory);
      break;
    case PROP_UI_MANAGER:
      g_value_set_object (value, dock_columns->p->ui_manager);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_dock_columns_dropped_cb (GtkWidget *notebook,
                              GtkWidget *child,
                              gint       insert_index,
                              gpointer   data)
{
  ClayDockColumns *dock_columns = CLAY_DOCK_COLUMNS (data);
  ClayDockable    *dockable     = CLAY_DOCKABLE (child);
  GtkWidget       *new_dockbook = NULL;

  /* Create a new dock (including a new dockbook) */
  clay_dock_columns_prepare_dockbook (dock_columns,
                                      insert_index,
                                      &new_dockbook);

  /* Move the dockable to the new dockbook */
  g_object_ref (new_dockbook);
  g_object_ref (dockable);

  gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), child);
  gtk_notebook_append_page (GTK_NOTEBOOK (new_dockbook), child, NULL);

  g_object_unref (dockable);
  g_object_unref (new_dockbook);

  return TRUE;
}

static void
clay_dock_columns_real_dock_added (ClayDockColumns *dock_columns,
                                   ClayDock        *dock)
{
}

static void
clay_dock_columns_real_dock_removed (ClayDockColumns *dock_columns,
                                     ClayDock        *dock)
{
}

static void
clay_dock_columns_dock_book_removed (ClayDockColumns *dock_columns,
                                     ClayDockbook    *dockbook,
                                     ClayDock        *dock)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));

  if (clay_dock_get_dockbooks (dock) == NULL &&
      ! CLAY_IS_TOOLBOX (dock) &&
      gtk_widget_get_parent (GTK_WIDGET (dock)) != NULL)
    clay_dock_columns_remove_dock (dock_columns, dock);
}


/**
 * clay_dock_columns_new:
 * @context:
 *
 * Returns: A new #ClayDockColumns.
 **/
GtkWidget *
clay_dock_columns_new (ClayContext       *context,
                       ClayDialogFactory *dialog_factory,
                       ClayUIManager     *ui_manager)
{
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (CLAY_IS_UI_MANAGER (ui_manager), NULL);

  return g_object_new (CLAY_TYPE_DOCK_COLUMNS,
                       "context",        context,
                       "dialog-factory", dialog_factory,
                       "ui-manager",     ui_manager,
                       NULL);
}

/**
 * clay_dock_columns_add_dock:
 * @dock_columns:
 * @dock:
 *
 * Add a dock, added to a horizontal ClayPanedBox.
 **/
void
clay_dock_columns_add_dock (ClayDockColumns *dock_columns,
                            ClayDock        *dock,
                            gint             index)
{
  g_return_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (CLAY_IS_DOCK (dock));

  CLAY_LOG (DND, "Adding ClayDock %p to ClayDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_insert (dock_columns->p->docks, dock, index);

  clay_dock_update_with_context (dock, dock_columns->p->context);

  clay_paned_box_add_widget (CLAY_PANED_BOX (dock_columns->p->paned_hbox),
                             GTK_WIDGET (dock),
                             index);

  g_signal_connect_object (dock, "book-removed",
                           G_CALLBACK (clay_dock_columns_dock_book_removed),
                           dock_columns,
                           G_CONNECT_SWAPPED);

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_ADDED], 0, dock);
}

/**
 * clay_dock_columns_prepare_dockbook:
 * @dock_columns:
 * @dock_index:
 * @dockbook_p:
 *
 * Create a new dock and add it to the dock columns with the given
 * dock_index insert index, then create and add a dockbook and put it
 * in the dock.
 **/
void
clay_dock_columns_prepare_dockbook (ClayDockColumns  *dock_columns,
                                    gint              dock_index,
                                    GtkWidget       **dockbook_p)
{
  ClayDialogFactory *dialog_factory;
  ClayMenuFactory   *menu_factory;
  GtkWidget         *dock;
  GtkWidget         *dockbook;

  dock = clay_menu_dock_new ();
  clay_dock_columns_add_dock (dock_columns, CLAY_DOCK (dock), dock_index);

  dialog_factory = dock_columns->p->dialog_factory;
  menu_factory   = menus_get_global_menu_factory (clay_dialog_factory_get_context (dialog_factory)->clay);
  dockbook = clay_dockbook_new (menu_factory);
  clay_dock_add_book (CLAY_DOCK (dock), CLAY_DOCKBOOK (dockbook), -1);

  gtk_widget_set_visible (GTK_WIDGET (dock), TRUE);

  if (dockbook_p)
    *dockbook_p = dockbook;
}


void
clay_dock_columns_remove_dock (ClayDockColumns *dock_columns,
                               ClayDock        *dock)
{
  g_return_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns));
  g_return_if_fail (CLAY_IS_DOCK (dock));

  CLAY_LOG (DND, "Removing ClayDock %p from ClayDockColumns %p", dock, dock_columns);

  dock_columns->p->docks = g_list_remove (dock_columns->p->docks, dock);

  clay_dock_update_with_context (dock, NULL);

  g_signal_handlers_disconnect_by_func (dock,
                                        clay_dock_columns_dock_book_removed,
                                        dock_columns);

  g_object_ref (dock);
  clay_paned_box_remove_widget (CLAY_PANED_BOX (dock_columns->p->paned_hbox),
                                GTK_WIDGET (dock));

  g_signal_emit (dock_columns, dock_columns_signals[DOCK_REMOVED], 0, dock);
  g_object_unref (dock);
}

GList *
clay_dock_columns_get_docks (ClayDockColumns *dock_columns)
{
  g_return_val_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->docks;
}

ClayContext *
clay_dock_columns_get_context (ClayDockColumns *dock_columns)
{
  g_return_val_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->context;
}

void
clay_dock_columns_set_context (ClayDockColumns *dock_columns,
                               ClayContext     *context)
{
  g_return_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns));

  dock_columns->p->context = context;
}

ClayDialogFactory *
clay_dock_columns_get_dialog_factory (ClayDockColumns *dock_columns)
{
  g_return_val_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->dialog_factory;
}

ClayUIManager *
clay_dock_columns_get_ui_manager (ClayDockColumns *dock_columns)
{
  g_return_val_if_fail (CLAY_IS_DOCK_COLUMNS (dock_columns), NULL);

  return dock_columns->p->ui_manager;
}
