/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydock.c
 * Copyright (C) 2001-2018 Michael Natterer <mitch@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "menus/menus.h"

#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockable.h"
#include "claydockbook.h"
#include "claydockcolumns.h"
#include "claydockcontainer.h"
#include "claydockwindow.h"
#include "claypanedbox.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  BOOK_ADDED,
  BOOK_REMOVED,
  DESCRIPTION_INVALIDATED,
  GEOMETRY_INVALIDATED,
  LAST_SIGNAL
};


struct _ClayDockPrivate
{
  GtkWidget *main_vbox;
  GtkWidget *paned_vbox;

  GList     *dockbooks;

  gint       ID;
};


static void       clay_dock_dispose                (GObject      *object);

static gchar    * clay_dock_real_get_description   (ClayDock     *dock,
                                                    gboolean      complete);
static void       clay_dock_real_book_added        (ClayDock     *dock,
                                                    ClayDockbook *dockbook);
static void       clay_dock_real_book_removed      (ClayDock     *dock,
                                                    ClayDockbook *dockbook);
static void       clay_dock_invalidate_description (ClayDock     *dock);
static gboolean   clay_dock_dropped_cb             (GtkWidget    *notebook,
                                                    GtkWidget    *child,
                                                    gint          insert_index,
                                                    gpointer      data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDock, clay_dock, GTK_TYPE_BOX)

#define parent_class clay_dock_parent_class

static guint dock_signals[LAST_SIGNAL] = { 0 };


static void
clay_dock_class_init (ClayDockClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  dock_signals[BOOK_ADDED] =
    g_signal_new ("book-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockClass, book_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCKBOOK);

  dock_signals[BOOK_REMOVED] =
    g_signal_new ("book-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockClass, book_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCKBOOK);

  dock_signals[DESCRIPTION_INVALIDATED] =
    g_signal_new ("description-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockClass, description_invalidated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  dock_signals[GEOMETRY_INVALIDATED] =
    g_signal_new ("geometry-invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockClass, geometry_invalidated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose          = clay_dock_dispose;

  klass->get_description         = clay_dock_real_get_description;
  klass->set_host_geometry_hints = NULL;
  klass->book_added              = clay_dock_real_book_added;
  klass->book_removed            = clay_dock_real_book_removed;
  klass->description_invalidated = NULL;
  klass->geometry_invalidated    = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tool-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              GTK_ICON_SIZE_SMALL_TOOLBAR,
                                                              CLAY_PARAM_READABLE));

  gtk_widget_class_set_css_name (widget_class, "ClayDock");
}

static void
clay_dock_init (ClayDock *dock)
{
  static gint  dock_ID = 1;
  gchar       *name    = NULL;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (dock),
                                  GTK_ORIENTATION_VERTICAL);

  dock->p = clay_dock_get_instance_private (dock);
  dock->p->ID = dock_ID++;

  name = g_strdup_printf ("clay-internal-dock-%d", dock->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock), name);
  g_free (name);

  dock->p->main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (dock), dock->p->main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (dock->p->main_vbox, TRUE);

  dock->p->paned_vbox = clay_paned_box_new (FALSE, 0, GTK_ORIENTATION_VERTICAL);
  clay_paned_box_set_dropped_cb (CLAY_PANED_BOX (dock->p->paned_vbox),
                                 clay_dock_dropped_cb,
                                 dock);
  gtk_box_pack_start (GTK_BOX (dock->p->main_vbox), dock->p->paned_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (dock->p->paned_vbox, TRUE);
}

static void
clay_dock_dispose (GObject *object)
{
  ClayDock *dock = CLAY_DOCK (object);

  while (dock->p->dockbooks)
    {
      ClayDockbook *dockbook = dock->p->dockbooks->data;

      g_object_ref (dockbook);
      clay_dock_remove_book (dock, dockbook);
      gtk_widget_destroy (GTK_WIDGET (dockbook));
      g_object_unref (dockbook);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gchar *
clay_dock_real_get_description (ClayDock *dock,
                                gboolean  complete)
{
  GString *desc;
  GList   *list;

  desc = g_string_new (NULL);

  for (list = clay_dock_get_dockbooks (dock);
       list;
       list = g_list_next (list))
    {
      ClayDockbook *dockbook = list->data;
      GList        *children;
      GList        *child;

      if (complete)
        {
          /* Include all dockables */
          children = gtk_container_get_children (GTK_CONTAINER (dockbook));
        }
      else
        {
          GtkWidget *dockable = NULL;
          gint       page_num = 0;

          page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
          dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

          /* Only include active dockables */
          children = g_list_append (NULL, dockable);
        }

      for (child = children; child; child = g_list_next (child))
        {
          ClayDockable *dockable = child->data;

          g_string_append (desc, clay_dockable_get_name (dockable));

          if (g_list_next (child))
            g_string_append (desc, CLAY_DOCK_DOCKABLE_SEPARATOR);
        }

      g_list_free (children);

      if (g_list_next (list))
        g_string_append (desc, CLAY_DOCK_BOOK_SEPARATOR);
    }

  return g_string_free (desc, FALSE);
}

static void
clay_dock_real_book_added (ClayDock     *dock,
                           ClayDockbook *dockbook)
{
  g_signal_connect_object (dockbook, "switch-page",
                           G_CALLBACK (clay_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
}

static void
clay_dock_real_book_removed (ClayDock     *dock,
                             ClayDockbook *dockbook)
{
  g_signal_handlers_disconnect_by_func (dockbook,
                                        clay_dock_invalidate_description,
                                        dock);
}

static void
clay_dock_invalidate_description (ClayDock *dock)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[DESCRIPTION_INVALIDATED], 0);
}

static gboolean
clay_dock_dropped_cb (GtkWidget *notebook,
                      GtkWidget *child,
                      gint       insert_index,
                      gpointer   data)
{
  ClayDock          *dock     = CLAY_DOCK (data);
  ClayDockbook      *dockbook = CLAY_DOCKBOOK (notebook);
  ClayDockable      *dockable = CLAY_DOCKABLE (child);
  ClayDialogFactory *factory;
  GtkWidget         *new_dockbook;

  /*  if dropping to the same dock, take care that we don't try
   *  to reorder the *only* dockable in the dock
   */
  if (clay_dockbook_get_dock (dockbook) == dock)
    {
      gint n_dockables = gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook));
      gint n_books     = g_list_length (clay_dock_get_dockbooks (dock));

      if (n_books == 1 && n_dockables == 1)
        return TRUE; /* successfully do nothing */
    }

  /* Detach the dockable from the old dockbook */
  g_object_ref (dockable);
  gtk_notebook_detach_tab (GTK_NOTEBOOK (notebook), child);

  /* Create a new dockbook */
  factory = clay_dock_get_dialog_factory (dock);
  new_dockbook = clay_dockbook_new (menus_get_global_menu_factory (clay_dialog_factory_get_context (factory)->clay));
  clay_dock_add_book (dock, CLAY_DOCKBOOK (new_dockbook), insert_index);

  /* Add the dockable to new new dockbook */
  gtk_notebook_append_page (GTK_NOTEBOOK (new_dockbook), child, NULL);
  g_object_unref (dockable);

  return TRUE;
}


/*  public functions  */

/**
 * clay_dock_get_description:
 * @dock:
 * @complete: If %TRUE, only includes the active dockables, i.e. not the
 *            dockables in a non-active GtkNotebook tab
 *
 * Returns: A string describing the contents of the dock.
 **/
gchar *
clay_dock_get_description (ClayDock *dock,
                           gboolean  complete)
{
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  if (CLAY_DOCK_GET_CLASS (dock)->get_description)
    return CLAY_DOCK_GET_CLASS (dock)->get_description (dock, complete);

  return NULL;
}

/**
 * clay_dock_set_host_geometry_hints:
 * @dock:   The dock
 * @window: The #GtkWindow to adapt to hosting the dock
 *
 * Some docks have some specific needs on the #GtkWindow they are
 * in. This function allows such docks to perform any such setup on
 * the #GtkWindow they are in/will be put in.
 **/
void
clay_dock_set_host_geometry_hints (ClayDock  *dock,
                                   GtkWindow *window)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));
  g_return_if_fail (GTK_IS_WINDOW (window));

  if (CLAY_DOCK_GET_CLASS (dock)->set_host_geometry_hints)
    CLAY_DOCK_GET_CLASS (dock)->set_host_geometry_hints (dock, window);
}

/**
 * clay_dock_invalidate_geometry:
 * @dock:
 *
 * Call when the dock needs to setup its host #GtkWindow with
 * GtkDock::set_host_geometry_hints().
 **/
void
clay_dock_invalidate_geometry (ClayDock *dock)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));

  g_signal_emit (dock, dock_signals[GEOMETRY_INVALIDATED], 0);
}

/**
 * clay_dock_update_with_context:
 * @dock:
 * @context:
 *
 * Set the @context on all dockables in the @dock.
 **/
void
clay_dock_update_with_context (ClayDock    *dock,
                               ClayContext *context)
{
  GList *iter = NULL;

  for (iter = clay_dock_get_dockbooks (dock);
       iter;
       iter = g_list_next (iter))
    {
      ClayDockbook *dockbook = CLAY_DOCKBOOK (iter->data);

      clay_dockbook_update_with_context (dockbook, context);
    }
}

/**
 * clay_dock_get_context:
 * @dock:
 *
 * Returns: The #ClayContext for the #ClayDockWindow the @dock is in.
 **/
ClayContext *
clay_dock_get_context (ClayDock *dock)
{
  ClayContext *context = NULL;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  /* First try ClayDockColumns */
  if (! context)
    {
      ClayDockColumns *dock_columns;

      dock_columns =
        CLAY_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    CLAY_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        context = clay_dock_columns_get_context (dock_columns);
    }

  /* Then ClayDockWindow */
  if (! context)
    {
      ClayDockWindow *dock_window = clay_dock_window_from_dock (dock);

      if (dock_window)
        context = clay_dock_window_get_context (dock_window);
    }

  return context;
}

/**
 * clay_dock_get_dialog_factory:
 * @dock:
 *
 * Returns: The #ClayDialogFactory for the #ClayDockWindow the @dock
 *          is in.
 **/
ClayDialogFactory *
clay_dock_get_dialog_factory (ClayDock *dock)
{
  ClayDialogFactory *dialog_factory = NULL;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  /* First try ClayDockColumns */
  if (! dialog_factory)
    {
      ClayDockColumns *dock_columns;

      dock_columns =
        CLAY_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    CLAY_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        dialog_factory = clay_dock_columns_get_dialog_factory (dock_columns);
    }

  /* Then ClayDockWindow */
  if (! dialog_factory)
    {
      ClayDockWindow *dock_window = clay_dock_window_from_dock (dock);

      if (dock_window)
        dialog_factory = clay_dock_container_get_dialog_factory (CLAY_DOCK_CONTAINER (dock_window));
    }

  return dialog_factory;
}

/**
 * clay_dock_get_ui_manager:
 * @dock:
 *
 * Returns: The #ClayUIManager for the #ClayDockWindow the @dock is
 *          in.
 **/
ClayUIManager *
clay_dock_get_ui_manager (ClayDock *dock)
{
  ClayUIManager *ui_manager = NULL;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  /* First try ClayDockColumns */
  if (! ui_manager)
    {
      ClayDockColumns *dock_columns;

      dock_columns =
        CLAY_DOCK_COLUMNS (gtk_widget_get_ancestor (GTK_WIDGET (dock),
                                                    CLAY_TYPE_DOCK_COLUMNS));

      if (dock_columns)
        ui_manager = clay_dock_columns_get_ui_manager (dock_columns);
    }

  /* Then ClayDockContainer */
  if (! ui_manager)
    {
      ClayDockWindow *dock_window = clay_dock_window_from_dock (dock);

      if (dock_window)
        {
          ClayDockContainer *dock_container = CLAY_DOCK_CONTAINER (dock_window);

          ui_manager = clay_dock_container_get_ui_manager (dock_container);
        }
    }

  return ui_manager;
}

GList *
clay_dock_get_dockbooks (ClayDock *dock)
{
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  return dock->p->dockbooks;
}

gint
clay_dock_get_n_dockables (ClayDock *dock)
{
  GList *list = NULL;
  gint   n    = 0;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), 0);

  for (list = dock->p->dockbooks; list; list = list->next)
    n += gtk_notebook_get_n_pages (GTK_NOTEBOOK (list->data));

  return n;
}

GtkWidget *
clay_dock_get_main_vbox (ClayDock *dock)
{
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  return dock->p->main_vbox;
}

GtkWidget *
clay_dock_get_vbox (ClayDock *dock)
{
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  return dock->p->paned_vbox;
}

void
clay_dock_set_id (ClayDock *dock,
                  gint      ID)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));

  dock->p->ID = ID;
}

gint
clay_dock_get_id (ClayDock *dock)
{
  g_return_val_if_fail (CLAY_IS_DOCK (dock), 0);

  return dock->p->ID;
}

void
clay_dock_add_book (ClayDock     *dock,
                    ClayDockbook *dockbook,
                    gint          index)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));
  g_return_if_fail (CLAY_IS_DOCKBOOK (dockbook));
  g_return_if_fail (clay_dockbook_get_dock (dockbook) == NULL);

  clay_dockbook_set_dock (dockbook, dock);

  g_signal_connect_object (dockbook, "dockable-added",
                           G_CALLBACK (clay_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-removed",
                           G_CALLBACK (clay_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);
  g_signal_connect_object (dockbook, "dockable-reordered",
                           G_CALLBACK (clay_dock_invalidate_description),
                           dock, G_CONNECT_SWAPPED);

  dock->p->dockbooks = g_list_insert (dock->p->dockbooks, dockbook, index);
  clay_paned_box_add_widget (CLAY_PANED_BOX (dock->p->paned_vbox),
                             GTK_WIDGET (dockbook),
                             index);
  gtk_widget_set_visible (GTK_WIDGET (dockbook), TRUE);

  clay_dock_invalidate_description (dock);

  g_signal_emit (dock, dock_signals[BOOK_ADDED], 0, dockbook);
}

void
clay_dock_remove_book (ClayDock     *dock,
                       ClayDockbook *dockbook)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));
  g_return_if_fail (CLAY_IS_DOCKBOOK (dockbook));
  g_return_if_fail (clay_dockbook_get_dock (dockbook) == dock);

  clay_dockbook_set_dock (dockbook, NULL);

  g_signal_handlers_disconnect_by_func (dockbook,
                                        clay_dock_invalidate_description,
                                        dock);

  /* Ref the dockbook so we can emit the "book-removed" signal and
   * pass it as a parameter before it's destroyed
   */
  g_object_ref (dockbook);

  dock->p->dockbooks = g_list_remove (dock->p->dockbooks, dockbook);
  clay_paned_box_remove_widget (CLAY_PANED_BOX (dock->p->paned_vbox),
                                GTK_WIDGET (dockbook));

  clay_dock_invalidate_description (dock);

  g_signal_emit (dock, dock_signals[BOOK_REMOVED], 0, dockbook);

  g_object_unref (dockbook);
}
