/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydialogfactory.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "core/clayobject.h"


#define CLAY_DIALOG_VISIBILITY_KEY "clay-dialog-visibility"

typedef enum
{
  CLAY_DIALOG_VISIBILITY_UNKNOWN = 0,
  CLAY_DIALOG_VISIBILITY_INVISIBLE,
  CLAY_DIALOG_VISIBILITY_VISIBLE,
  CLAY_DIALOG_VISIBILITY_HIDDEN
} ClayDialogVisibilityState;


/* In order to support constructors of various types, these functions
 * takes the union of the set of arguments required for each type of
 * widget constructor. If this set becomes too big we can consider
 * passing a struct or use varargs.
 */
typedef GtkWidget * (* ClayDialogNewFunc)     (ClayDialogFactory      *factory,
                                               ClayContext            *context,
                                               ClayUIManager          *ui_manager,
                                               gint                    view_size);


struct _ClayDialogFactoryEntry
{
  gchar                *identifier;
  gchar                *name;
  gchar                *blurb;
  gchar                *icon_name;
  gchar                *help_id;

  ClayDialogNewFunc     new_func;
  ClayDialogRestoreFunc restore_func;
  gint                  view_size;

  gboolean              singleton;
  gboolean              session_managed;
  gboolean              remember_size;
  gboolean              remember_if_open;

  /* If TRUE the visibility of the dialog is toggleable */
  gboolean              hideable;

  /* If TRUE the entry is for a ClayImageWindow, FALSE otherwise */
  gboolean              image_window;

  /* If TRUE the entry is for a dockable, FALSE otherwise */
  gboolean              dockable;
};


#define CLAY_TYPE_DIALOG_FACTORY            (clay_dialog_factory_get_type ())
#define CLAY_DIALOG_FACTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DIALOG_FACTORY, ClayDialogFactory))
#define CLAY_DIALOG_FACTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DIALOG_FACTORY, ClayDialogFactoryClass))
#define CLAY_IS_DIALOG_FACTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DIALOG_FACTORY))
#define CLAY_IS_DIALOG_FACTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DIALOG_FACTORY))
#define CLAY_DIALOG_FACTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DIALOG_FACTORY, ClayDialogFactoryClass))


typedef struct _ClayDialogFactoryPrivate  ClayDialogFactoryPrivate;
typedef struct _ClayDialogFactoryClass    ClayDialogFactoryClass;

/**
 * ClayDialogFactory:
 *
 * A factory with the main purpose of creating toplevel windows and
 * position them according to the session information kept within the
 * factory. Over time it has accumulated more functionality than this.
 */
struct _ClayDialogFactory
{
  ClayObject                parent_instance;

  ClayDialogFactoryPrivate *p;
};

struct _ClayDialogFactoryClass
{
  ClayObjectClass  parent_class;

  void (* dock_window_added)   (ClayDialogFactory *factory,
                                ClayDockWindow    *dock_window);
  void (* dock_window_removed) (ClayDialogFactory *factory,
                                ClayDockWindow    *dock_window);
};


GType               clay_dialog_factory_get_type             (void);
ClayDialogFactory * clay_dialog_factory_new                  (const gchar             *name,
                                                              ClayContext             *context);

void                clay_dialog_factory_register_entry       (ClayDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              const gchar             *name,
                                                              const gchar             *blurb,
                                                              const gchar             *icon_name,
                                                              const gchar             *help_id,
                                                              ClayDialogNewFunc        new_func,
                                                              ClayDialogRestoreFunc    restore_func,
                                                              gint                     view_size,
                                                              gboolean                 singleton,
                                                              gboolean                 session_managed,
                                                              gboolean                 remember_size,
                                                              gboolean                 remember_if_open,
                                                              gboolean                 hideable,
                                                              gboolean                 image_window,
                                                              gboolean                 dockable);

ClayDialogFactoryEntry *
                    clay_dialog_factory_find_entry           (ClayDialogFactory       *factory,
                                                              const gchar             *identifier);
ClaySessionInfo *   clay_dialog_factory_find_session_info    (ClayDialogFactory       *factory,
                                                              const gchar             *identifier);
GtkWidget *         clay_dialog_factory_find_widget          (ClayDialogFactory       *factory,
                                                              const gchar             *identifiers);

GtkWidget *         clay_dialog_factory_dialog_new           (ClayDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              ClayUIManager           *ui_manager,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifier,
                                                              gint                     view_size,
                                                              gboolean                 present);

ClayContext *       clay_dialog_factory_get_context          (ClayDialogFactory       *factory);
GList *             clay_dialog_factory_get_open_dialogs     (ClayDialogFactory       *factory);

GList *             clay_dialog_factory_get_session_infos    (ClayDialogFactory       *factory);
void                clay_dialog_factory_add_session_info     (ClayDialogFactory       *factory,
                                                              ClaySessionInfo         *info);

GtkWidget *         clay_dialog_factory_dialog_raise         (ClayDialogFactory       *factory,
                                                              GdkMonitor              *monitor,
                                                              GtkWidget               *parent,
                                                              const gchar             *identifiers,
                                                              gint                     view_size);

GtkWidget *         clay_dialog_factory_dockable_new         (ClayDialogFactory       *factory,
                                                              ClayDock                *dock,
                                                              const gchar             *identifier,
                                                              gint                     view_size);

void                clay_dialog_factory_add_dialog           (ClayDialogFactory       *factory,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                clay_dialog_factory_add_foreign          (ClayDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                clay_dialog_factory_position_dialog      (ClayDialogFactory       *factory,
                                                              const gchar             *identifier,
                                                              GtkWidget               *dialog,
                                                              GdkMonitor              *monitor);
void                clay_dialog_factory_remove_dialog        (ClayDialogFactory       *factory,
                                                              GtkWidget               *dialog);

void                clay_dialog_factory_hide_dialog          (GtkWidget               *dialog);

void                clay_dialog_factory_save                 (ClayDialogFactory       *factory,
                                                              ClayConfigWriter        *writer);
void                clay_dialog_factory_restore              (ClayDialogFactory       *factory,
                                                              GdkMonitor              *monitor);

void                clay_dialog_factory_set_state            (ClayDialogFactory       *factory,
                                                              ClayDialogsState         state);
ClayDialogsState    clay_dialog_factory_get_state            (ClayDialogFactory       *factory);

void                clay_dialog_factory_show_with_display    (ClayDialogFactory       *factory);
void                clay_dialog_factory_hide_with_display    (ClayDialogFactory       *factory);

void                clay_dialog_factory_set_busy             (ClayDialogFactory       *factory);
void                clay_dialog_factory_unset_busy           (ClayDialogFactory       *factory);

ClayDialogFactory * clay_dialog_factory_from_widget          (GtkWidget               *dialog,
                                                              ClayDialogFactoryEntry **entry);

void                clay_dialog_factory_set_has_min_size     (GtkWindow               *window,
                                                              gboolean                 has_min_size);
gboolean            clay_dialog_factory_get_has_min_size     (GtkWindow               *window);

ClayDialogFactory * clay_dialog_factory_get_singleton        (void);
void                clay_dialog_factory_set_singleton        (ClayDialogFactory       *factory);
