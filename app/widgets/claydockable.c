/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockable.c
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claycontext.h"

#include "menus/menus.h"

#include "claydialogfactory.h"
#include "claydnd.h"
#include "claydock.h"
#include "claydockable.h"
#include "claydockbook.h"
#include "claydocked.h"
#include "claydockwindow.h"
#include "clayhelp-ids.h"
#include "claypanedbox.h"
#include "claysessioninfo-aux.h"
#include "claysessionmanaged.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_LOCKED
};


struct _ClayDockablePrivate
{
  gchar        *name;
  gchar        *blurb;
  gchar        *icon_name;
  gchar        *help_id;
  ClayTabStyle  tab_style;
  gboolean      locked;

  ClayDockbook *dockbook;

  ClayContext  *context;
};


static void       clay_dockable_session_managed_iface_init (ClaySessionManagedInterface *iface);

static void       clay_dockable_dispose       (GObject            *object);
static void       clay_dockable_set_property  (GObject            *object,
                                               guint               property_id,
                                               const GValue       *value,
                                               GParamSpec         *pspec);
static void       clay_dockable_get_property  (GObject            *object,
                                               guint               property_id,
                                               GValue             *value,
                                               GParamSpec         *pspec);

static void       clay_dockable_style_updated (GtkWidget          *widget);

static void       clay_dockable_add           (GtkContainer       *container,
                                               GtkWidget          *widget);
static GType      clay_dockable_child_type    (GtkContainer       *container);

static GList    * clay_dockable_get_aux_info  (ClaySessionManaged *managed);
static void       clay_dockable_set_aux_info  (ClaySessionManaged *managed,
                                               GList              *aux_info);


G_DEFINE_TYPE_WITH_CODE (ClayDockable, clay_dockable, GTK_TYPE_BIN,
                         G_ADD_PRIVATE (ClayDockable)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SESSION_MANAGED,
                                                clay_dockable_session_managed_iface_init))

#define parent_class clay_dockable_parent_class

static const GtkTargetEntry dialog_target_table[] = { CLAY_TARGET_NOTEBOOK_TAB };


static void
clay_dockable_class_init (ClayDockableClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->dispose       = clay_dockable_dispose;
  object_class->set_property  = clay_dockable_set_property;
  object_class->get_property  = clay_dockable_get_property;

  widget_class->style_updated = clay_dockable_style_updated;

  container_class->add        = clay_dockable_add;
  container_class->child_type = clay_dockable_child_type;

  gtk_container_class_handle_border_width (container_class);

  g_object_class_install_property (object_class, PROP_LOCKED,
                                   g_param_spec_boolean ("locked", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("content-border",
                                                             NULL, NULL,
                                                             0,
                                                             G_MAXINT,
                                                             0,
                                                             CLAY_PARAM_READABLE));
}

static void
clay_dockable_init (ClayDockable *dockable)
{
  dockable->p = clay_dockable_get_instance_private (dockable);

  dockable->p->tab_style = CLAY_TAB_STYLE_PREVIEW;

  gtk_drag_dest_set (GTK_WIDGET (dockable),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);
}

static void
clay_dockable_session_managed_iface_init (ClaySessionManagedInterface *iface)
{
  iface->get_aux_info = clay_dockable_get_aux_info;
  iface->set_aux_info = clay_dockable_set_aux_info;
}

static void
clay_dockable_dispose (GObject *object)
{
  ClayDockable *dockable = CLAY_DOCKABLE (object);

  if (dockable->p->context)
    clay_dockable_set_context (dockable, NULL);

  g_clear_pointer (&dockable->p->blurb,     g_free);
  g_clear_pointer (&dockable->p->name,      g_free);
  g_clear_pointer (&dockable->p->icon_name, g_free);
  g_clear_pointer (&dockable->p->help_id,   g_free);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dockable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClayDockable *dockable = CLAY_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      clay_dockable_set_locked (dockable, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dockable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClayDockable *dockable = CLAY_DOCKABLE (object);

  switch (property_id)
    {
    case PROP_LOCKED:
      g_value_set_boolean (value, dockable->p->locked);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dockable_style_updated (GtkWidget *widget)
{
  gint content_border;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "content-border", &content_border,
                        NULL);

  gtk_container_set_border_width (GTK_CONTAINER (widget), content_border);
}


static void
clay_dockable_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  ClayDockable *dockable;

  g_return_if_fail (gtk_bin_get_child (GTK_BIN (container)) == NULL);

  GTK_CONTAINER_CLASS (parent_class)->add (container, widget);

  /*  not all tab styles are supported by all children  */
  dockable = CLAY_DOCKABLE (container);
  clay_dockable_set_tab_style (dockable, dockable->p->tab_style);
}

static GType
clay_dockable_child_type (GtkContainer *container)
{
  if (gtk_bin_get_child (GTK_BIN (container)))
    return G_TYPE_NONE;

  return CLAY_TYPE_DOCKED;
}

static GtkWidget *
clay_dockable_new_tab_widget_internal (ClayDockable *dockable,
                                       ClayContext  *context,
                                       ClayTabStyle  tab_style,
                                       GtkIconSize   size,
                                       gboolean      dnd)
{
  GtkWidget *tab_widget = NULL;
  GtkWidget *label      = NULL;
  GtkWidget *icon       = NULL;

  switch (tab_style)
    {
    case CLAY_TAB_STYLE_NAME:
    case CLAY_TAB_STYLE_ICON_NAME:
    case CLAY_TAB_STYLE_PREVIEW_NAME:
      label = gtk_label_new (dockable->p->name);
      break;

    case CLAY_TAB_STYLE_BLURB:
    case CLAY_TAB_STYLE_ICON_BLURB:
    case CLAY_TAB_STYLE_PREVIEW_BLURB:
      label = gtk_label_new (dockable->p->blurb);
      break;

    default:
      break;
    }

  switch (tab_style)
    {
    case CLAY_TAB_STYLE_ICON:
    case CLAY_TAB_STYLE_ICON_NAME:
    case CLAY_TAB_STYLE_ICON_BLURB:
      icon = clay_dockable_get_icon (dockable, size);
      break;

    case CLAY_TAB_STYLE_PREVIEW:
    case CLAY_TAB_STYLE_PREVIEW_NAME:
    case CLAY_TAB_STYLE_PREVIEW_BLURB:
      {
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

        if (child)
          icon = clay_docked_get_preview (CLAY_DOCKED (child),
                                          context, size);

        if (! icon)
          icon = clay_dockable_get_icon (dockable, size);
      }
      break;

    default:
      break;
    }

  if (label && dnd)
    clay_label_set_attributes (GTK_LABEL (label),
                               PANGO_ATTR_WEIGHT, PANGO_WEIGHT_SEMIBOLD,
                               -1);

  switch (tab_style)
    {
    case CLAY_TAB_STYLE_ICON:
    case CLAY_TAB_STYLE_PREVIEW:
      tab_widget = icon;
      break;

    case CLAY_TAB_STYLE_NAME:
    case CLAY_TAB_STYLE_BLURB:
      tab_widget = label;
      break;

    case CLAY_TAB_STYLE_ICON_NAME:
    case CLAY_TAB_STYLE_ICON_BLURB:
    case CLAY_TAB_STYLE_PREVIEW_NAME:
    case CLAY_TAB_STYLE_PREVIEW_BLURB:
      tab_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, dnd ? 6 : 2);

      gtk_box_pack_start (GTK_BOX (tab_widget), icon, FALSE, FALSE, 0);
      gtk_widget_set_visible (icon, TRUE);

      gtk_box_pack_start (GTK_BOX (tab_widget), label, FALSE, FALSE, 0);
      gtk_widget_set_visible (label, TRUE);
      break;
    }

  return tab_widget;
}

/*  public functions  */

GtkWidget *
clay_dockable_new (const gchar *name,
                   const gchar *blurb,
                   const gchar *icon_name,
                   const gchar *help_id)
{
  ClayDockable *dockable;

  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);

  dockable = g_object_new (CLAY_TYPE_DOCKABLE, NULL);

  dockable->p->name      = g_strdup (name);
  dockable->p->icon_name = g_strdup (icon_name);
  dockable->p->help_id   = g_strdup (help_id);

  if (blurb)
    dockable->p->blurb  = g_strdup (blurb);
  else
    dockable->p->blurb  = g_strdup (dockable->p->name);

  clay_help_set_help_data (GTK_WIDGET (dockable), NULL, help_id);

  return GTK_WIDGET (dockable);
}

void
clay_dockable_set_dockbook (ClayDockable *dockable,
                            ClayDockbook *dockbook)
{
  g_return_if_fail (CLAY_IS_DOCKABLE (dockable));
  g_return_if_fail (dockbook == NULL ||
                    CLAY_IS_DOCKBOOK (dockbook));

  dockable->p->dockbook = dockbook;
}

ClayDockbook *
clay_dockable_get_dockbook (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return dockable->p->dockbook;
}

void
clay_dockable_set_tab_style (ClayDockable *dockable,
                             ClayTabStyle  tab_style)
{
  GtkWidget *child;

  g_return_if_fail (CLAY_IS_DOCKABLE (dockable));

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child && ! CLAY_DOCKED_GET_IFACE (child)->get_preview)
    tab_style = clay_preview_tab_style_to_icon (tab_style);

  dockable->p->tab_style = tab_style;
}

ClayTabStyle
clay_dockable_get_tab_style (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), -1);

  return dockable->p->tab_style;
}

void
clay_dockable_set_locked (ClayDockable *dockable,
                          gboolean      lock)
{
  g_return_if_fail (CLAY_IS_DOCKABLE (dockable));

  if (dockable->p->locked != lock)
    {
      dockable->p->locked = lock ? TRUE : FALSE;

      g_object_notify (G_OBJECT (dockable), "locked");
    }
}

gboolean
clay_dockable_get_locked (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), FALSE);

  return dockable->p->locked;
}

const gchar *
clay_dockable_get_name (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return dockable->p->name;
}

const gchar *
clay_dockable_get_blurb (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return dockable->p->blurb;
}

const gchar *
clay_dockable_get_help_id (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return dockable->p->help_id;
}

const gchar *
clay_dockable_get_icon_name (ClayDockable *dockable)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return dockable->p->icon_name;
}

GtkWidget *
clay_dockable_get_icon (ClayDockable *dockable,
                        GtkIconSize   size)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  return gtk_image_new_from_icon_name (dockable->p->icon_name, size);
}

GtkWidget *
clay_dockable_create_tab_widget (ClayDockable *dockable,
                                 ClayContext  *context,
                                 ClayTabStyle  tab_style,
                                 GtkIconSize   size)
{
  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return clay_dockable_new_tab_widget_internal (dockable, context,
                                                tab_style, size, FALSE);
}

void
clay_dockable_set_context (ClayDockable *dockable,
                           ClayContext  *context)
{
  g_return_if_fail (CLAY_IS_DOCKABLE (dockable));
  g_return_if_fail (context == NULL || CLAY_IS_CONTEXT (context));

  if (context != dockable->p->context)
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (dockable));

      if (child)
        clay_docked_set_context (CLAY_DOCKED (child), context);

      dockable->p->context = context;
    }
}

ClayUIManager *
clay_dockable_get_menu (ClayDockable  *dockable,
                        const gchar  **ui_path,
                        gpointer      *popup_data)
{
  GtkWidget *child;

  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);
  g_return_val_if_fail (popup_data != NULL, NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return clay_docked_get_menu (CLAY_DOCKED (child), ui_path, popup_data);

  return NULL;
}

void
clay_dockable_detach (ClayDockable *dockable)
{
  ClayDialogFactory *dialog_factory;
  ClayMenuFactory   *menu_factory;
  ClayDockWindow    *src_dock_window;
  ClayDock          *src_dock;
  GtkWidget         *dock;
  ClayDockWindow    *dock_window;
  GtkWidget         *dockbook;

  g_return_if_fail (CLAY_IS_DOCKABLE (dockable));
  g_return_if_fail (CLAY_IS_DOCKBOOK (dockable->p->dockbook));

  src_dock        = clay_dockbook_get_dock (dockable->p->dockbook);
  src_dock_window = clay_dock_window_from_dock (src_dock);

  dialog_factory = clay_dock_get_dialog_factory (src_dock);
  menu_factory   = menus_get_global_menu_factory (clay_dialog_factory_get_context (dialog_factory)->clay);

  dock = clay_dock_with_window_new (dialog_factory,
                                    clay_widget_get_monitor (GTK_WIDGET (dockable)),
                                    FALSE /*toolbox*/);
  dock_window = clay_dock_window_from_dock (CLAY_DOCK (dock));
  gtk_window_set_position (GTK_WINDOW (dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    clay_dock_window_setup (dock_window, src_dock_window);

  dockbook = clay_dockbook_new (menu_factory);

  clay_dock_add_book (CLAY_DOCK (dock), CLAY_DOCKBOOK (dockbook), 0);

  g_object_ref (dockable);
  gtk_container_remove (GTK_CONTAINER (dockable->p->dockbook),
                        GTK_WIDGET (dockable));

  gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                            GTK_WIDGET (dockable), NULL);
  g_object_unref (dockable);

  gtk_widget_set_visible (GTK_WIDGET (dock_window), TRUE);
  gtk_widget_set_visible (dock, TRUE);
}


/*  private functions  */

static GList *
clay_dockable_get_aux_info (ClaySessionManaged *session_managed)
{
  ClayDockable *dockable;
  GtkWidget    *child;

  g_return_val_if_fail (CLAY_IS_DOCKABLE (session_managed), NULL);

  dockable = CLAY_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    return clay_docked_get_aux_info (CLAY_DOCKED (child));

  return NULL;
}

static void
clay_dockable_set_aux_info (ClaySessionManaged *session_managed,
                            GList              *aux_info)
{
  ClayDockable *dockable;
  GtkWidget    *child;

  g_return_if_fail (CLAY_IS_DOCKABLE (session_managed));

  dockable = CLAY_DOCKABLE (session_managed);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    clay_docked_set_aux_info (CLAY_DOCKED (child), aux_info);
}
