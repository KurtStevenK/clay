/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydialogfactory.c
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "menus/menus.h"

#include "display/display-types.h"
#include "display/clayimagewindow.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "claycursor.h"
#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockbook.h"
#include "claydockable.h"
#include "claydockcontainer.h"
#include "claydockwindow.h"
#include "claymenufactory.h"
#include "claysessioninfo.h"
#include "claywidgets-utils.h"

#include "clay-log.h"


enum
{
  DOCK_WINDOW_ADDED,
  DOCK_WINDOW_REMOVED,
  LAST_SIGNAL
};


struct _ClayDialogFactoryPrivate
{
  ClayContext      *context;

  GList            *open_dialogs;
  GList            *session_infos;

  GList            *registered_dialogs;

  ClayDialogsState  dialog_state;
};


static void        clay_dialog_factory_dispose              (GObject                *object);
static void        clay_dialog_factory_finalize             (GObject                *object);
static GtkWidget * clay_dialog_factory_constructor          (ClayDialogFactory      *factory,
                                                             ClayDialogFactoryEntry *entry,
                                                             ClayContext            *context,
                                                             ClayUIManager          *ui_manager,
                                                             gint                    view_size);
static void        clay_dialog_factory_config_notify        (ClayDialogFactory      *factory,
                                                             GParamSpec             *pspec,
                                                             ClayGuiConfig          *config);
static void        clay_dialog_factory_set_widget_data      (GtkWidget              *dialog,
                                                             ClayDialogFactory      *factory,
                                                             ClayDialogFactoryEntry *entry);
static void        clay_dialog_factory_unset_widget_data    (GtkWidget              *dialog);
static gboolean    clay_dialog_factory_set_user_pos         (GtkWidget              *dialog,
                                                             GdkEventConfigure      *cevent,
                                                             gpointer                data);
static gboolean    clay_dialog_factory_dialog_configure     (GtkWidget              *dialog,
                                                             GdkEventConfigure      *cevent,
                                                             ClayDialogFactory      *factory);
static void        clay_dialog_factory_hide                 (ClayDialogFactory      *factory);
static void        clay_dialog_factory_show                 (ClayDialogFactory      *factory);

static void        clay_dialog_factory_display_changed      (ClayContext            *context,
                                                             ClayDisplay            *display,
                                                             GtkWidget              *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDialogFactory, clay_dialog_factory,
                            CLAY_TYPE_OBJECT)

#define parent_class clay_dialog_factory_parent_class

static guint factory_signals[LAST_SIGNAL] = { 0 };


/* Is set by dialogs.c to a dialog factory initialized there.
 *
 * FIXME: The layer above should not do this kind of initialization of
 * layers below.
 */
static ClayDialogFactory *clay_toplevel_factory = NULL;


static void
clay_dialog_factory_class_init (ClayDialogFactoryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = clay_dialog_factory_dispose;
  object_class->finalize = clay_dialog_factory_finalize;

  factory_signals[DOCK_WINDOW_ADDED] =
    g_signal_new ("dock-window-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayDialogFactoryClass, dock_window_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCK_WINDOW);

  factory_signals[DOCK_WINDOW_REMOVED] =
    g_signal_new ("dock-window-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (ClayDialogFactoryClass, dock_window_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCK_WINDOW);
}

static void
clay_dialog_factory_init (ClayDialogFactory *factory)
{
  factory->p = clay_dialog_factory_get_instance_private (factory);
  factory->p->dialog_state = CLAY_DIALOGS_SHOWN;
}

static void
clay_dialog_factory_dispose (GObject *object)
{
  ClayDialogFactory *factory = CLAY_DIALOG_FACTORY (object);
  GList             *list;

  /*  start iterating from the beginning each time we destroyed a
   *  toplevel because destroying a dock may cause lots of items
   *  to be removed from factory->p->open_dialogs
   */
  while (factory->p->open_dialogs)
    {
      for (list = factory->p->open_dialogs; list; list = g_list_next (list))
        {
          if (gtk_widget_is_toplevel (list->data))
            {
              gtk_widget_destroy (GTK_WIDGET (list->data));
              break;
            }
        }

      /*  the list being non-empty without any toplevel is an error,
       *  so eek and chain up
       */
      if (! list)
        {
          g_warning ("%s: %d stale non-toplevel entries in factory->p->open_dialogs",
                     G_STRFUNC, g_list_length (factory->p->open_dialogs));
          break;
        }
    }

  g_clear_pointer (&factory->p->open_dialogs, g_list_free);

  if (factory->p->session_infos)
    {
      g_list_free_full (factory->p->session_infos,
                        (GDestroyNotify) g_object_unref);
      factory->p->session_infos = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dialog_factory_finalize (GObject *object)
{
  ClayDialogFactory *factory = CLAY_DIALOG_FACTORY (object);
  GList             *list;

  for (list = factory->p->registered_dialogs; list; list = g_list_next (list))
    {
      ClayDialogFactoryEntry *entry = list->data;

      g_free (entry->identifier);
      g_free (entry->name);
      g_free (entry->blurb);
      g_free (entry->icon_name);
      g_free (entry->help_id);

      g_slice_free (ClayDialogFactoryEntry, entry);
    }

  g_clear_pointer (&factory->p->registered_dialogs, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

ClayDialogFactory *
clay_dialog_factory_new (const gchar *name,
                         ClayContext *context)
{
  ClayDialogFactory *factory;
  ClayGuiConfig     *config;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  factory = g_object_new (CLAY_TYPE_DIALOG_FACTORY, NULL);

  clay_object_set_name (CLAY_OBJECT (factory), name);

  config = CLAY_GUI_CONFIG (context->clay->config);

  factory->p->context      = context;
  factory->p->dialog_state = (config->hide_docks ?
                              CLAY_DIALOGS_HIDDEN_EXPLICITLY :
                              CLAY_DIALOGS_SHOWN);

  g_signal_connect_object (config, "notify::hide-docks",
                           G_CALLBACK (clay_dialog_factory_config_notify),
                           factory, G_CONNECT_SWAPPED);

  return factory;
}

void
clay_dialog_factory_register_entry (ClayDialogFactory    *factory,
                                    const gchar          *identifier,
                                    const gchar          *name,
                                    const gchar          *blurb,
                                    const gchar          *icon_name,
                                    const gchar          *help_id,
                                    ClayDialogNewFunc     new_func,
                                    ClayDialogRestoreFunc restore_func,
                                    gint                  view_size,
                                    gboolean              singleton,
                                    gboolean              session_managed,
                                    gboolean              remember_size,
                                    gboolean              remember_if_open,
                                    gboolean              hideable,
                                    gboolean              image_window,
                                    gboolean              dockable)
{
  ClayDialogFactoryEntry *entry;

  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);

  entry = g_slice_new0 (ClayDialogFactoryEntry);

  entry->identifier       = g_strdup (identifier);
  entry->name             = g_strdup (name);
  entry->blurb            = g_strdup (blurb);
  entry->icon_name        = g_strdup (icon_name);
  entry->help_id          = g_strdup (help_id);
  entry->new_func         = new_func;
  entry->restore_func     = restore_func;
  entry->view_size        = view_size;
  entry->singleton        = singleton ? TRUE : FALSE;
  entry->session_managed  = session_managed ? TRUE : FALSE;
  entry->remember_size    = remember_size ? TRUE : FALSE;
  entry->remember_if_open = remember_if_open ? TRUE : FALSE;
  entry->hideable         = hideable ? TRUE : FALSE;
  entry->image_window     = image_window ? TRUE : FALSE;
  entry->dockable         = dockable ? TRUE : FALSE;

  factory->p->registered_dialogs = g_list_prepend (factory->p->registered_dialogs,
                                                   entry);
}

ClayDialogFactoryEntry *
clay_dialog_factory_find_entry (ClayDialogFactory *factory,
                                const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->p->registered_dialogs; list; list = g_list_next (list))
    {
      ClayDialogFactoryEntry *entry = list->data;

      if (! strcmp (identifier, entry->identifier))
        return entry;
    }

  return NULL;
}

ClaySessionInfo *
clay_dialog_factory_find_session_info (ClayDialogFactory *factory,
                                       const gchar       *identifier)
{
  GList *list;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      ClaySessionInfo *info = list->data;

      if (clay_session_info_get_factory_entry (info) &&
          g_str_equal (identifier,
                       clay_session_info_get_factory_entry (info)->identifier))
        {
          return info;
        }
    }

  return NULL;
}

GtkWidget *
clay_dialog_factory_find_widget (ClayDialogFactory *factory,
                                 const gchar       *identifiers)
{
  GtkWidget  *widget = NULL;
  gchar     **ids;
  gint        i;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  ids = g_strsplit (identifiers, "|", 0);

  for (i = 0; ids[i]; i++)
    {
      ClaySessionInfo *info;

      info = clay_dialog_factory_find_session_info (factory, ids[i]);

      if (info)
        {
          widget =  clay_session_info_get_widget (info);

          if (widget)
            break;
        }
    }

  g_strfreev (ids);

  return widget;
}

/**
 * clay_dialog_factory_dialog_sane:
 * @factory:
 * @widget_factory:
 * @widget_entry:
 * @widget:
 *
 * Makes sure that the @widget with the given @widget_entry that was
 * created by the given @widget_factory belongs to @efactory.
 *
 * Returns: %TRUE if that is the case, %FALSE otherwise.
 **/
static gboolean
clay_dialog_factory_dialog_sane (ClayDialogFactory      *factory,
                                 ClayDialogFactory      *widget_factory,
                                 ClayDialogFactoryEntry *widget_entry,
                                 GtkWidget              *widget)
{
  if (! widget_factory || ! widget_entry)
    {
      g_warning ("%s: dialog was not created by a ClayDialogFactory",
                 G_STRFUNC);
      return FALSE;
    }

  if (widget_factory != factory)
    {
      g_warning ("%s: dialog was created by a different ClayDialogFactory",
                 G_STRFUNC);
      return FALSE;
    }

  return TRUE;
}

/**
 * clay_dialog_factory_dialog_new_internal:
 * @factory:
 * @monitor:
 * @context:
 * @ui_manager:
 * @parent:
 * @identifier:
 * @view_size:
 * @return_existing:   If %TRUE, (or if the dialog is a singleton),
 *                     don't create a new dialog if it exists, instead
 *                     return the existing one
 * @present:           If %TRUE, the toplevel that contains the dialog (if any)
 *                     will be gtk_window_present():ed
 * @create_containers: If %TRUE, then containers for the
 *                     dialog/dockable will be created as well. If you
 *                     want to manage your own containers, pass %FALSE
 *
 * This is the lowest level dialog factory creation function.
 *
 * Returns: A created or existing #GtkWidget.
 **/
static GtkWidget *
clay_dialog_factory_dialog_new_internal (ClayDialogFactory *factory,
                                         GdkMonitor        *monitor,
                                         ClayContext       *context,
                                         ClayUIManager     *ui_manager,
                                         GtkWidget         *parent,
                                         const gchar       *identifier,
                                         gint               view_size,
                                         gboolean           return_existing,
                                         gboolean           present,
                                         gboolean           create_containers)
{
  ClayDialogFactoryEntry *entry    = NULL;
  GtkWidget              *dialog   = NULL;
  GtkWidget              *toplevel = NULL;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  entry = clay_dialog_factory_find_entry (factory, identifier);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"",
                 G_STRFUNC, identifier);
      return NULL;
    }

  if (! entry->new_func)
    {
      g_warning ("%s: entry for \"%s\" has no constructor",
                 G_STRFUNC, identifier);
      return NULL;
    }

  /*  a singleton dialog is always returned if it already exists  */
  if (return_existing || entry->singleton)
    {
      dialog = clay_dialog_factory_find_widget (factory, identifier);
    }

  /*  create the dialog if it was not found  */
  if (! dialog)
    {
      GtkWidget *dock        = NULL;
      GtkWidget *dockbook    = NULL;
      GtkWidget *dock_window = NULL;

      /* What follows is special-case code for some entries. At some
       * point we might want to abstract this block of code away.
       */
      if (create_containers)
        {
          if (entry->dockable)
            {
              /*  It doesn't make sense to have a dockable without a dock
               *  so create one. Create a new dock _before_ creating the
               *  dialog. We do this because the new dockable needs to be
               *  created in its dock's context.
               */
              dock = clay_dock_with_window_new (factory, monitor, FALSE);
              dockbook = clay_dockbook_new (menus_get_global_menu_factory (factory->p->context->clay));

              clay_dock_add_book (CLAY_DOCK (dock),
                                  CLAY_DOCKBOOK (dockbook),
                                  0);
            }
          else if (strcmp ("clay-toolbox", entry->identifier) == 0)
            {
              ClayDockContainer *dock_container;

              dock_window = clay_dialog_factory_dialog_new (factory,
                                                            monitor,
                                                            NULL /*ui_manager*/,
                                                            parent,
                                                            "clay-toolbox-window",
                                                            -1 /*view_size*/,
                                                            FALSE /*present*/);

              /* When we get a dock window, we also get a UI
               * manager
               */
              dock_container = CLAY_DOCK_CONTAINER (dock_window);
              ui_manager     = clay_dock_container_get_ui_manager (dock_container);
            }
        }

      /*  Create the new dialog in the appropriate context which is
       *  - the passed context if not NULL
       *  - the newly created dock's context if we just created it
       *  - the factory's context, which happens when raising a toplevel
       *    dialog was the original request.
       */
      if (view_size < CLAY_VIEW_SIZE_TINY)
        view_size = entry->view_size;

      if (context)
        dialog = clay_dialog_factory_constructor (factory, entry,
                                                  context,
                                                  ui_manager,
                                                  view_size);
      else if (dock)
        dialog = clay_dialog_factory_constructor (factory, entry,
                                                  clay_dock_get_context (CLAY_DOCK (dock)),
                                                  clay_dock_get_ui_manager (CLAY_DOCK (dock)),
                                                  view_size);
      else
        dialog = clay_dialog_factory_constructor (factory, entry,
                                                  factory->p->context,
                                                  ui_manager,
                                                  view_size);

      if (dialog)
        {
          clay_dialog_factory_set_widget_data (dialog, factory, entry);

          /*  If we created a dock before, the newly created dialog is
           *  supposed to be a ClayDockable.
           */
          if (dock)
            {
              if (CLAY_IS_DOCKABLE (dialog))
                {
                  gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                                            dialog, NULL);
                  gtk_widget_set_visible (dock, TRUE);
                }
              else
                {
                  g_warning ("%s: ClayDialogFactory is a dockable factory "
                             "but constructor for \"%s\" did not return a "
                             "ClayDockable",
                             G_STRFUNC, identifier);

                  gtk_widget_destroy (dialog);
                  gtk_widget_destroy (dock);

                  dialog = NULL;
                  dock   = NULL;
                }
            }
          else if (dock_window)
            {
              if (CLAY_IS_DOCK (dialog))
                {
                  clay_dock_window_add_dock (CLAY_DOCK_WINDOW (dock_window),
                                             CLAY_DOCK (dialog),
                                             -1 /*index*/);

                  gtk_widget_set_visible (dialog, present);
                  gtk_widget_set_visible (dock_window, present);
                }
              else
                {
                  g_warning ("%s: ClayDialogFactory is a dock factory entry "
                             "but constructor for \"%s\" did not return a "
                             "ClayDock",
                             G_STRFUNC, identifier);

                  gtk_widget_destroy (dialog);
                  gtk_widget_destroy (dock_window);

                  dialog      = NULL;
                  dock_window = NULL;
                }
            }
        }
      else if (dock)
        {
          g_warning ("%s: constructor for \"%s\" returned NULL",
                     G_STRFUNC, identifier);

          gtk_widget_destroy (dock);

          dock = NULL;
        }

      if (dialog)
        clay_dialog_factory_add_dialog (factory, dialog, monitor);
    }

  /*  Finally, if we found an existing dialog or created a new one, raise it.
   */
  if (! dialog)
    return NULL;

  if (gtk_widget_is_toplevel (dialog))
    {
      gtk_window_set_screen (GTK_WINDOW (dialog),
                             gdk_display_get_default_screen (gdk_monitor_get_display (monitor)));

      if (parent)
        {
          GtkWidget *parent_toplevel = gtk_widget_get_toplevel (parent);

          if (GTK_IS_WINDOW (parent_toplevel))
            {
              ClayGuiConfig *config          = NULL;
#ifndef G_OS_WIN32
              /* On Linux, this is not needed since window compositors are smart.
                 On macOS, this breaks animation and positioning of the dialog
                 so we use clay_widget_set_auto_transient which cycles all dialogs */
              gboolean       force_transient = FALSE;
#else
              /* On Windows, this is needed and works well in the same process,
                 otherwise dialogs like "Open Location" and "About" get lost */
              gboolean       force_transient = TRUE;
#endif

              if (context != NULL)
                config = CLAY_GUI_CONFIG (context->clay->config);

              if (force_transient || ! config ||
                  config->dock_window_hint == CLAY_WINDOW_HINT_KEEP_ABOVE)
                {
                  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                                GTK_WINDOW (parent_toplevel));

                  if (context != NULL && G_TYPE_FROM_INSTANCE (parent_toplevel) == CLAY_TYPE_IMAGE_WINDOW)
                    {
                      g_signal_connect_object (context, "display-changed",
                                               G_CALLBACK (clay_dialog_factory_display_changed),
                                               dialog, 0);
                    }
                }
            }
        }

      toplevel = dialog;
    }
  else if (CLAY_IS_DOCK (dialog))
    {
      toplevel = gtk_widget_get_toplevel (dialog);
    }
  else if (CLAY_IS_DOCKABLE (dialog))
    {
      ClayDockable *dockable = CLAY_DOCKABLE (dialog);

      if (clay_dockable_get_dockbook (dockable) &&
          clay_dockbook_get_dock (clay_dockable_get_dockbook (dockable)))
        {
          GtkNotebook *notebook = GTK_NOTEBOOK (clay_dockable_get_dockbook (dockable));
          gint         num      = gtk_notebook_page_num (notebook, dialog);

          if (num != -1)
            {
              gtk_notebook_set_current_page (notebook, num);

              clay_widget_blink (dialog);
            }
        }

      toplevel = gtk_widget_get_toplevel (dialog);
    }

  if (present && GTK_IS_WINDOW (toplevel))
    {
      /*  Work around focus-stealing protection, or whatever makes the
       *  dock appear below the one where we clicked a button to open
       *  it. See bug #630173.
       */
      gtk_widget_show_now (toplevel);
      gdk_window_raise (gtk_widget_get_window (toplevel));
    }

  return dialog;
}

/**
 * clay_dialog_factory_dialog_new:
 * @factory:      a #ClayDialogFactory
 * @monitor       the #GdkMonitor the dialog should appear on
 * @ui_manager:   A #ClayUIManager, if applicable.
 * @parent:       The parent widget, from which the call originated, if
 *                applicable, NULL otherwise.
 * @identifier:   the identifier of the dialog as registered with
 *                clay_dialog_factory_register_entry()
 * @view_size:    the initial preview size
 * @present:      whether gtk_window_present() should be called
 *
 * Creates a new toplevel dialog or a #ClayDockable, depending on whether
 * %factory is a toplevel of dockable factory.
 *
 * Returns: the newly created dialog or an already existing singleton
 *               dialog.
 **/
GtkWidget *
clay_dialog_factory_dialog_new (ClayDialogFactory *factory,
                                GdkMonitor        *monitor,
                                ClayUIManager     *ui_manager,
                                GtkWidget         *parent,
                                const gchar       *identifier,
                                gint               view_size,
                                gboolean           present)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  return clay_dialog_factory_dialog_new_internal (factory,
                                                  monitor,
                                                  factory->p->context,
                                                  ui_manager,
                                                  parent,
                                                  identifier,
                                                  view_size,
                                                  FALSE /*return_existing*/,
                                                  present,
                                                  FALSE /*create_containers*/);
}

ClayContext *
clay_dialog_factory_get_context (ClayDialogFactory *factory)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->context;
}

GList *
clay_dialog_factory_get_open_dialogs (ClayDialogFactory *factory)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->open_dialogs;
}

GList *
clay_dialog_factory_get_session_infos (ClayDialogFactory *factory)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);

  return factory->p->session_infos;
}

void
clay_dialog_factory_add_session_info (ClayDialogFactory *factory,
                                      ClaySessionInfo   *info)
{
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (CLAY_IS_SESSION_INFO (info));

  /* We want to append rather than prepend so that the serialized
   * order in sessionrc remains the same
   */
  factory->p->session_infos = g_list_append (factory->p->session_infos,
                                             g_object_ref (info));
}

/**
 * clay_dialog_factory_dialog_raise:
 * @factory:      a #ClayDialogFactory
 * @monitor:      the #GdkMonitor the dialog should appear on
 * @parent:       the #GtkWidget from which the raised dialog
 *                originated, if applicable, %NULL otherwise.
 * @identifiers:  a '|' separated list of identifiers of dialogs as
 *                registered with clay_dialog_factory_register_entry()
 * @view_size:    the initial preview size if a dialog needs to be created
 *
 * Raises any of a list of already existing toplevel dialog or
 * #ClayDockable if it was already created by this %facory.
 *
 * Implicitly creates the first dialog in the list if none of the dialogs
 * were found.
 *
 * Returns: the raised or newly created dialog.
 **/
GtkWidget *
clay_dialog_factory_dialog_raise (ClayDialogFactory *factory,
                                  GdkMonitor        *monitor,
                                  GtkWidget         *parent,
                                  const gchar       *identifiers,
                                  gint               view_size)
{
  GtkWidget *dialog;
  gchar    **ids;
  gint       i;

  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  /*  If the identifier is a list, try to find a matching dialog and
   *  raise it. If there's no match, use the first list item.
   *
   *  (we split the identifier list manually here because we must pass
   *  a single identifier, not a list, to new_internal() below)
   */
  ids = g_strsplit (identifiers, "|", 0);
  for (i = 0; ids[i]; i++)
    {
      if (clay_dialog_factory_find_widget (factory, ids[i]))
        break;
    }

  dialog = clay_dialog_factory_dialog_new_internal (factory,
                                                    monitor,
                                                    NULL,
                                                    NULL,
                                                    parent,
                                                    ids[i] ? ids[i] : ids[0],
                                                    view_size,
                                                    TRUE /*return_existing*/,
                                                    TRUE /*present*/,
                                                    TRUE /*create_containers*/);
  g_strfreev (ids);

  return dialog;
}

/**
 * clay_dialog_factory_dockable_new:
 * @factory:      a #ClayDialogFactory
 * @dock:         a #ClayDock created by this %factory.
 * @identifier:   the identifier of the dialog as registered with
 *                clay_dialog_factory_register_entry()
 * @view_size:
 *
 * Creates a new #ClayDockable in the context of the #ClayDock it will be
 * added to.
 *
 * Implicitly raises & returns an already existing singleton dockable,
 * so callers should check that clay_dockable_get_dockbook (dockable)
 * is NULL before trying to add it to it's #ClayDockbook.
 *
 * Returns: the newly created #ClayDockable or an already existing
 *               singleton dockable.
 **/
GtkWidget *
clay_dialog_factory_dockable_new (ClayDialogFactory *factory,
                                  ClayDock          *dock,
                                  const gchar       *identifier,
                                  gint               view_size)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);

  return clay_dialog_factory_dialog_new_internal (factory,
                                                  clay_widget_get_monitor (GTK_WIDGET (dock)),
                                                  clay_dock_get_context (dock),
                                                  clay_dock_get_ui_manager (dock),
                                                  NULL,
                                                  identifier,
                                                  view_size,
                                                  FALSE /*return_existing*/,
                                                  FALSE /*present*/,
                                                  FALSE /*create_containers*/);
}

void
clay_dialog_factory_add_dialog (ClayDialogFactory *factory,
                                GtkWidget         *dialog,
                                GdkMonitor        *monitor)
{
  ClayDialogFactory      *dialog_factory = NULL;
  ClayDialogFactoryEntry *entry          = NULL;
  ClaySessionInfo        *info           = NULL;
  GList                  *list           = NULL;
  gboolean                toplevel       = FALSE;

  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  if (g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog already registered", G_STRFUNC);
      return;
    }

  dialog_factory = clay_dialog_factory_from_widget (dialog, &entry);

  if (! clay_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return;

  toplevel = gtk_widget_is_toplevel (dialog);

  if (entry)
    {
      /* dialog is a toplevel (but not a ClayDockWindow) or a ClayDockable */

      CLAY_LOG (DIALOG_FACTORY, "adding %s \"%s\"",
                toplevel ? "toplevel" : "dockable",
                entry->identifier);

      for (list = factory->p->session_infos; list; list = g_list_next (list))
        {
          ClaySessionInfo *current_info = list->data;

          if (clay_session_info_get_factory_entry (current_info) == entry)
            {
              if (clay_session_info_get_widget (current_info))
                {
                  if (clay_session_info_is_singleton (current_info))
                    {
                      g_warning ("%s: singleton dialog \"%s\" created twice",
                                 G_STRFUNC, entry->identifier);

                      CLAY_LOG (DIALOG_FACTORY,
                                "corrupt session info: %p (widget %p)",
                                current_info,
                                clay_session_info_get_widget (current_info));

                      return;
                    }

                  continue;
                }

              clay_session_info_set_widget (current_info, dialog);

              CLAY_LOG (DIALOG_FACTORY,
                        "updating session info %p (widget %p) for %s \"%s\"",
                        current_info,
                        clay_session_info_get_widget (current_info),
                        toplevel ? "toplevel" : "dockable",
                        entry->identifier);

              if (toplevel &&
                  clay_session_info_is_session_managed (current_info) &&
                  ! gtk_widget_get_visible (dialog))
                {
                  /* Clay opens every window on the startup screen, not on
                   * the monitor under the pointer or a previously stored one.
                   */
                  clay_session_info_apply_geometry (current_info,
                                                    monitor,
                                                    FALSE);
                }

              info = current_info;
              break;
            }
        }

      if (! info)
        {
          info = clay_session_info_new ();

          clay_session_info_set_widget (info, dialog);

          CLAY_LOG (DIALOG_FACTORY,
                    "creating session info %p (widget %p) for %s \"%s\"",
                    info,
                    clay_session_info_get_widget (info),
                    toplevel ? "toplevel" : "dockable",
                    entry->identifier);

          clay_session_info_set_factory_entry (info, entry);

          if (clay_session_info_is_session_managed (info))
            {
              /* Make the dialog show up at the user position the
               * first time it is shown. After it has been shown the
               * first time we don't want it to show at the mouse the
               * next time. Think of the use cases "hide and show with
               * tab" and "change virtual desktops"
               */
              CLAY_LOG (WM, "setting GTK_WIN_POS_MOUSE for %p (\"%s\")\n",
                        dialog, entry->identifier);

              gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_MOUSE);
              g_signal_connect (dialog, "configure-event",
                                G_CALLBACK (clay_dialog_factory_set_user_pos),
                                NULL);
            }

          clay_dialog_factory_add_session_info (factory, info);
          g_object_unref (info);
        }
    }

  /* Some special logic for dock windows */
  if (CLAY_IS_DOCK_WINDOW (dialog))
    {
      g_signal_emit (factory, factory_signals[DOCK_WINDOW_ADDED], 0, dialog);
    }

  factory->p->open_dialogs = g_list_prepend (factory->p->open_dialogs, dialog);

  g_signal_connect_object (dialog, "destroy",
                           G_CALLBACK (clay_dialog_factory_remove_dialog),
                           factory,
                           G_CONNECT_SWAPPED);

  if (clay_session_info_is_session_managed (info))
    g_signal_connect_object (dialog, "configure-event",
                             G_CALLBACK (clay_dialog_factory_dialog_configure),
                             factory,
                             0);
}

void
clay_dialog_factory_add_foreign (ClayDialogFactory *factory,
                                 const gchar       *identifier,
                                 GtkWidget         *dialog,
                                 GdkMonitor        *monitor)
{
  ClayDialogFactory      *dialog_factory;
  ClayDialogFactoryEntry *entry;

  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  dialog_factory = clay_dialog_factory_from_widget (dialog, &entry);

  if (dialog_factory || entry)
    {
      g_warning ("%s: dialog was created by a ClayDialogFactory",
                 G_STRFUNC);
      return;
    }

  entry = clay_dialog_factory_find_entry (factory, identifier);

  if (! entry)
    {
      g_warning ("%s: no entry registered for \"%s\"",
                 G_STRFUNC, identifier);
      return;
    }

  if (entry->new_func)
    {
      g_warning ("%s: entry for \"%s\" has a constructor (is not foreign)",
                 G_STRFUNC, identifier);
      return;
    }

  clay_dialog_factory_set_widget_data (dialog, factory, entry);

  clay_dialog_factory_add_dialog (factory, dialog, monitor);
}

/**
 * clay_dialog_factory_position_dialog:
 * @factory:
 * @identifier:
 * @dialog:
 * @monitor:
 *
 * We correctly position all newly created dialog via
 * clay_dialog_factory_add_dialog(), but some dialogs (like various
 * color dialogs) are never destroyed but created only once per
 * session. On re-showing, whatever window managing magic kicks in and
 * the dialog sometimes goes where it shouldn't.
 *
 * This function correctly positions a dialog on re-showing so it
 * appears where it was before it was hidden.
 *
 * See https://github.com/KurtStevenK/clay/issues/1093
 **/
void
clay_dialog_factory_position_dialog (ClayDialogFactory *factory,
                                     const gchar       *identifier,
                                     GtkWidget         *dialog,
                                     GdkMonitor        *monitor)
{
  ClaySessionInfo *info;

  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (identifier != NULL);
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  info = clay_dialog_factory_find_session_info (factory, identifier);

  if (! info)
    {
      g_warning ("%s: no session info found for \"%s\"",
                 G_STRFUNC, identifier);
      return;
    }

  if (clay_session_info_get_widget (info) != dialog)
    {
      g_warning ("%s: session info for \"%s\" is for a different widget",
                 G_STRFUNC, identifier);
      return;
    }

  clay_session_info_apply_geometry (info,
                                    monitor,
                                    FALSE);
}

void
clay_dialog_factory_remove_dialog (ClayDialogFactory *factory,
                                   GtkWidget         *dialog)
{
  ClayDialogFactory      *dialog_factory;
  ClayDialogFactoryEntry *entry;
  GList                  *list;

  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  if (! g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog not registered", G_STRFUNC);
      return;
    }

  factory->p->open_dialogs = g_list_remove (factory->p->open_dialogs, dialog);

  dialog_factory = clay_dialog_factory_from_widget (dialog, &entry);

  if (! clay_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return;

  CLAY_LOG (DIALOG_FACTORY, "removing \"%s\" (dialog = %p)",
            entry->identifier,
            dialog);

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      ClaySessionInfo *session_info = list->data;

      if (clay_session_info_get_widget (session_info) == dialog)
        {
          CLAY_LOG (DIALOG_FACTORY,
                    "clearing session info %p (widget %p) for \"%s\"",
                    session_info, clay_session_info_get_widget (session_info),
                    entry->identifier);

          clay_session_info_set_widget (session_info, NULL);

          clay_dialog_factory_unset_widget_data (dialog);

          g_signal_handlers_disconnect_by_func (dialog,
                                                clay_dialog_factory_set_user_pos,
                                                NULL);
          g_signal_handlers_disconnect_by_func (dialog,
                                                clay_dialog_factory_remove_dialog,
                                                factory);

          if (clay_session_info_is_session_managed (session_info))
            g_signal_handlers_disconnect_by_func (dialog,
                                                  clay_dialog_factory_dialog_configure,
                                                  factory);

          if (CLAY_IS_DOCK_WINDOW (dialog))
            {
              /*  don't save session info for empty docks  */
              factory->p->session_infos = g_list_remove (factory->p->session_infos,
                                                         session_info);
              g_object_unref (session_info);

              g_signal_emit (factory, factory_signals[DOCK_WINDOW_REMOVED], 0,
                             dialog);
            }

          break;
        }
    }
}

void
clay_dialog_factory_hide_dialog (GtkWidget *dialog)
{
  ClayDialogFactory *factory = NULL;

  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (gtk_widget_is_toplevel (dialog));

  if (! (factory = clay_dialog_factory_from_widget (dialog, NULL)))
    {
      g_warning ("%s: dialog was not created by a ClayDialogFactory",
                 G_STRFUNC);
      return;
    }

  gtk_widget_set_visible (dialog, FALSE);

  if (factory->p->dialog_state != CLAY_DIALOGS_SHOWN)
    g_object_set_data (G_OBJECT (dialog), CLAY_DIALOG_VISIBILITY_KEY,
                       GINT_TO_POINTER (CLAY_DIALOG_VISIBILITY_INVISIBLE));
}

void
clay_dialog_factory_set_state (ClayDialogFactory *factory,
                               ClayDialogsState   state)
{
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));

  factory->p->dialog_state = state;

  if (state == CLAY_DIALOGS_SHOWN)
    {
      clay_dialog_factory_show (factory);
    }
  else
    {
      clay_dialog_factory_hide (factory);
    }
}

ClayDialogsState
clay_dialog_factory_get_state (ClayDialogFactory *factory)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (factory), 0);

  return factory->p->dialog_state;
}

void
clay_dialog_factory_show_with_display (ClayDialogFactory *factory)
{
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));

  if (factory->p->dialog_state == CLAY_DIALOGS_HIDDEN_WITH_DISPLAY)
    {
      clay_dialog_factory_set_state (factory, CLAY_DIALOGS_SHOWN);
    }
}

void
clay_dialog_factory_hide_with_display (ClayDialogFactory *factory)
{
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));

  if (factory->p->dialog_state == CLAY_DIALOGS_SHOWN)
    {
      clay_dialog_factory_set_state (factory, CLAY_DIALOGS_HIDDEN_WITH_DISPLAY);
    }
}

static GQuark clay_dialog_factory_key       = 0;
static GQuark clay_dialog_factory_entry_key = 0;

ClayDialogFactory *
clay_dialog_factory_from_widget (GtkWidget               *dialog,
                                 ClayDialogFactoryEntry **entry)
{
  g_return_val_if_fail (GTK_IS_WIDGET (dialog), NULL);

  if (! clay_dialog_factory_key)
    {
      clay_dialog_factory_key =
        g_quark_from_static_string ("clay-dialog-factory");

      clay_dialog_factory_entry_key =
        g_quark_from_static_string ("clay-dialog-factory-entry");
    }

  if (entry)
    *entry = g_object_get_qdata (G_OBJECT (dialog),
                                 clay_dialog_factory_entry_key);

  return g_object_get_qdata (G_OBJECT (dialog), clay_dialog_factory_key);
}

#define CLAY_DIALOG_FACTORY_MIN_SIZE_KEY "clay-dialog-factory-min-size"

void
clay_dialog_factory_set_has_min_size (GtkWindow *window,
                                      gboolean   has_min_size)
{
  g_return_if_fail (GTK_IS_WINDOW (window));

  g_object_set_data (G_OBJECT (window), CLAY_DIALOG_FACTORY_MIN_SIZE_KEY,
                     GINT_TO_POINTER (has_min_size ? TRUE : FALSE));
}

gboolean
clay_dialog_factory_get_has_min_size (GtkWindow *window)
{
  g_return_val_if_fail (GTK_IS_WINDOW (window), FALSE);

  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window),
                                             CLAY_DIALOG_FACTORY_MIN_SIZE_KEY));
}


/*  private functions  */

static GtkWidget *
clay_dialog_factory_constructor (ClayDialogFactory      *factory,
                                 ClayDialogFactoryEntry *entry,
                                 ClayContext            *context,
                                 ClayUIManager          *ui_manager,
                                 gint                    view_size)
{
  GtkWidget *widget;

  widget = entry->new_func (factory, context, ui_manager, view_size);

  /* The entry is for a dockable, so we simply need to put the created
   * widget in a dockable
   */
  if (widget && entry->dockable)
    {
      GtkWidget *dockable = NULL;

      dockable = clay_dockable_new (entry->name, entry->blurb,
                                    entry->icon_name, entry->help_id);
      gtk_container_add (GTK_CONTAINER (dockable), widget);
      gtk_widget_set_visible (widget, TRUE);

      /* EEK */
      g_object_set_data (G_OBJECT (dockable), "clay-dialog-identifier",
                         entry->identifier);

      /* Return the dockable instead */
      widget = dockable;
    }

  return widget;
}

static void
clay_dialog_factory_config_notify (ClayDialogFactory *factory,
                                   GParamSpec        *pspec,
                                   ClayGuiConfig     *config)
{
  ClayDialogsState state     = clay_dialog_factory_get_state (factory);
  ClayDialogsState new_state = state;

  /* Make sure the state and config are in sync */
  if (config->hide_docks && state == CLAY_DIALOGS_SHOWN)
    new_state = CLAY_DIALOGS_HIDDEN_EXPLICITLY;
  else if (! config->hide_docks)
    new_state = CLAY_DIALOGS_SHOWN;

  if (state != new_state)
    clay_dialog_factory_set_state (factory, new_state);
}

static void
clay_dialog_factory_set_widget_data (GtkWidget              *dialog,
                                     ClayDialogFactory      *factory,
                                     ClayDialogFactoryEntry *entry)
{
  g_return_if_fail (GTK_IS_WIDGET (dialog));
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));

  if (! clay_dialog_factory_key)
    {
      clay_dialog_factory_key =
        g_quark_from_static_string ("clay-dialog-factory");

      clay_dialog_factory_entry_key =
        g_quark_from_static_string ("clay-dialog-factory-entry");
    }

  g_object_set_qdata (G_OBJECT (dialog), clay_dialog_factory_key, factory);

  if (entry)
    g_object_set_qdata (G_OBJECT (dialog), clay_dialog_factory_entry_key,
                        entry);
}

static void
clay_dialog_factory_unset_widget_data (GtkWidget *dialog)
{
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  if (! clay_dialog_factory_key)
    return;

  g_object_set_qdata (G_OBJECT (dialog), clay_dialog_factory_key,       NULL);
  g_object_set_qdata (G_OBJECT (dialog), clay_dialog_factory_entry_key, NULL);
}

static gboolean
clay_dialog_factory_set_user_pos (GtkWidget         *dialog,
                                  GdkEventConfigure *cevent,
                                  gpointer           data)
{
  GdkWindowHints geometry_mask;

  /* Not only set geometry hints, also reset window position */
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);
  g_signal_handlers_disconnect_by_func (dialog,
                                        clay_dialog_factory_set_user_pos,
                                        data);

  CLAY_LOG (WM, "setting GDK_HINT_USER_POS for %p\n", dialog);
  geometry_mask = GDK_HINT_USER_POS;

  if (clay_dialog_factory_get_has_min_size (GTK_WINDOW (dialog)))
    geometry_mask |= GDK_HINT_MIN_SIZE;

  gtk_window_set_geometry_hints (GTK_WINDOW (dialog), NULL, NULL,
                                 geometry_mask);

  return FALSE;
}

static gboolean
clay_dialog_factory_dialog_configure (GtkWidget         *dialog,
                                      GdkEventConfigure *cevent,
                                      ClayDialogFactory *factory)
{
  ClayDialogFactory      *dialog_factory;
  ClayDialogFactoryEntry *entry;
  GList                  *list;

  if (! g_list_find (factory->p->open_dialogs, dialog))
    {
      g_warning ("%s: dialog not registered", G_STRFUNC);
      return FALSE;
    }

  dialog_factory = clay_dialog_factory_from_widget (dialog, &entry);

  if (! clay_dialog_factory_dialog_sane (factory,
                                         dialog_factory,
                                         entry,
                                         dialog))
    return FALSE;

  for (list = factory->p->session_infos; list; list = g_list_next (list))
    {
      ClaySessionInfo *session_info = list->data;

      if (clay_session_info_get_widget (session_info) == dialog)
        {
          clay_session_info_read_geometry (session_info, cevent);

          CLAY_LOG (DIALOG_FACTORY,
                    "updated session info for \"%s\" from window geometry "
                    "(x=%d y=%d  %dx%d)",
                    entry->identifier,
                    clay_session_info_get_x (session_info),
                    clay_session_info_get_y (session_info),
                    clay_session_info_get_width (session_info),
                    clay_session_info_get_height (session_info));

          break;
        }
    }

  return FALSE;
}

void
clay_dialog_factory_save (ClayDialogFactory *factory,
                          ClayConfigWriter  *writer)
{
  GList *infos;

  for (infos = factory->p->session_infos; infos; infos = g_list_next (infos))
    {
      ClaySessionInfo *info = infos->data;

      /*  we keep session info entries for all toplevel dialogs created
       *  by the factory but don't save them if they don't want to be
       *  managed
       */
      if (! clay_session_info_is_session_managed (info) ||
          clay_session_info_get_factory_entry (info) == NULL)
        continue;

      if (clay_session_info_get_widget (info))
        clay_session_info_get_info (info);

      clay_config_writer_open (writer, "session-info");
      clay_config_writer_string (writer,
                                 clay_object_get_name (factory));

      CLAY_CONFIG_GET_IFACE (info)->serialize (CLAY_CONFIG (info), writer, NULL);

      clay_config_writer_close (writer);

      if (clay_session_info_get_widget (info))
        clay_session_info_clear_info (info);
    }
}

void
clay_dialog_factory_restore (ClayDialogFactory *factory,
                             GdkMonitor        *monitor)
{
  GList *infos;

  for (infos = factory->p->session_infos; infos; infos = g_list_next (infos))
    {
      ClaySessionInfo *info = infos->data;

      if (clay_session_info_get_open (info))
        {
          clay_session_info_restore (info, factory, monitor);
        }
      else
        {
          CLAY_LOG (DIALOG_FACTORY,
                    "skipping to restore session info %p, not open",
                    info);
        }
    }
}

static void
clay_dialog_factory_hide (ClayDialogFactory *factory)
{
  GList *list;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          ClayDialogFactoryEntry    *entry      = NULL;
          ClayDialogVisibilityState  visibility = CLAY_DIALOG_VISIBILITY_UNKNOWN;

          clay_dialog_factory_from_widget (widget, &entry);
          if (! entry->hideable)
            continue;

          if (gtk_widget_get_visible (widget))
            {
              gtk_widget_set_visible (widget, FALSE);
              visibility = CLAY_DIALOG_VISIBILITY_HIDDEN;

              CLAY_LOG (WM, "Hiding '%s' [%p]",
                        gtk_window_get_title (GTK_WINDOW (widget)),
                        widget);
            }
          else
            {
              visibility = CLAY_DIALOG_VISIBILITY_INVISIBLE;
            }

          g_object_set_data (G_OBJECT (widget),
                             CLAY_DIALOG_VISIBILITY_KEY,
                             GINT_TO_POINTER (visibility));
        }
    }
}

static void
clay_dialog_factory_show (ClayDialogFactory *factory)
{
  GList *list;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          ClayDialogVisibilityState visibility;

          visibility =
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                CLAY_DIALOG_VISIBILITY_KEY));

          if (! gtk_widget_get_visible (widget) &&
              visibility == CLAY_DIALOG_VISIBILITY_HIDDEN)
            {
              CLAY_LOG (WM, "Showing '%s' [%p]",
                        gtk_window_get_title (GTK_WINDOW (widget)),
                        widget);

              /* Don't use gtk_window_present() here, we don't want the
               * keyboard focus to move.
               */
              gtk_widget_set_visible (widget, TRUE);
              g_object_set_data (G_OBJECT (widget),
                                 CLAY_DIALOG_VISIBILITY_KEY,
                                 GINT_TO_POINTER (CLAY_DIALOG_VISIBILITY_VISIBLE));

              if (gtk_widget_get_visible (widget))
                gdk_window_raise (gtk_widget_get_window (widget));
            }
        }
    }
}

void
clay_dialog_factory_set_busy (ClayDialogFactory *factory)
{
  GList *list;

  if (! factory)
    return;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          GdkWindow *window = gtk_widget_get_window (widget);

          if (window)
            {
              GdkCursor *cursor = clay_cursor_new (window,
                                                   CLAY_HANDEDNESS_RIGHT,
                                                   (ClayCursorType) GDK_WATCH,
                                                   CLAY_TOOL_CURSOR_NONE,
                                                   CLAY_CURSOR_MODIFIER_NONE);
              gdk_window_set_cursor (window, cursor);
              g_object_unref (cursor);
            }
        }
    }
}

void
clay_dialog_factory_unset_busy (ClayDialogFactory *factory)
{
  GList *list;

  if (! factory)
    return;

  for (list = factory->p->open_dialogs; list; list = g_list_next (list))
    {
      GtkWidget *widget = list->data;

      if (GTK_IS_WIDGET (widget) && gtk_widget_is_toplevel (widget))
        {
          GdkWindow *window = gtk_widget_get_window (widget);

          if (window)
            gdk_window_set_cursor (window, NULL);
        }
    }
}

/**
 * clay_dialog_factory_get_singleton:
 *
 * Returns: The toplevel ClayDialogFactory instance.
 **/
ClayDialogFactory *
clay_dialog_factory_get_singleton (void)
{
  g_return_val_if_fail (clay_toplevel_factory != NULL, NULL);

  return clay_toplevel_factory;
}

/**
 * clay_dialog_factory_set_singleton:
 * @:
 *
 * Set the toplevel ClayDialogFactory instance. Must only be called by
 * dialogs_init()!.
 **/
void
clay_dialog_factory_set_singleton (ClayDialogFactory *factory)
{
  g_return_if_fail (clay_toplevel_factory == NULL ||
                    factory               == NULL);

  clay_toplevel_factory = factory;
}


/* Private Functions */

static void
clay_dialog_factory_display_changed (ClayContext *context,
                                     ClayDisplay *display,
                                     GtkWidget   *dialog)
{
  GtkWindow *parent;

  if (display == NULL)
    /* No need to remove the parent. It will happen by itself if the
     * window was removed.
     */
    return;

  parent = GTK_WINDOW (clay_display_shell_get_window (clay_display_get_shell (display)));
  gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
}
