/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockbook.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@gimp.org>
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
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "menus/menus.h"

#include "clayactiongroup.h"
#include "claydialogfactory.h"
#include "claydnd.h"
#include "claydock.h"
#include "claydockable.h"
#include "claydockbook.h"
#include "claydocked.h"
#include "claydockcontainer.h"
#include "claydockwindow.h"
#include "clayhelp-ids.h"
#include "claymenufactory.h"
#include "claypanedbox.h"
#include "claystringaction.h"
#include "clayuimanager.h"
#include "clayview.h"
#include "claywidgets-utils.h"

#include "clay-log.h"
#include "clay-intl.h"


#define DEFAULT_TAB_ICON_SIZE        GTK_ICON_SIZE_BUTTON
#define DND_WIDGET_ICON_SIZE         GTK_ICON_SIZE_BUTTON
#define MENU_WIDGET_SPACING          4
#define TAB_HOVER_TIMEOUT            500
#define CLAY_DOCKABLE_DETACH_REF_KEY "clay-dockable-detach-ref"


enum
{
  DOCKABLE_ADDED,
  DOCKABLE_REMOVED,
  DOCKABLE_REORDERED,
  LAST_SIGNAL
};


typedef struct
{
  ClayDockbookDragCallback callback;
  gpointer                 data;
} ClayDockbookDragCallbackData;

struct _ClayDockbookPrivate
{
  ClayDock       *dock;
  ClayUIManager  *ui_manager;

  guint           tab_hover_timeout;
  ClayDockable   *tab_hover_dockable;

  ClayPanedBox   *drag_handler;

  GtkWidget      *menu_button;
};


static void         clay_dockbook_style_updated     (GtkWidget       *widget);
static void         clay_dockbook_drag_begin        (GtkWidget      *widget,
                                                     GdkDragContext *context);
static void         clay_dockbook_drag_end          (GtkWidget      *widget,
                                                     GdkDragContext *context);
static gboolean     clay_dockbook_drag_motion       (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     clay_dockbook_drag_drop         (GtkWidget      *widget,
                                                     GdkDragContext *context,
                                                     gint            x,
                                                     gint            y,
                                                     guint           time);
static gboolean     clay_dockbook_popup_menu        (GtkWidget      *widget);

static GtkNotebook *clay_dockbook_create_window     (GtkNotebook    *notebook,
                                                     GtkWidget      *page,
                                                     gint            x,
                                                     gint            y);
static void         clay_dockbook_page_added        (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         clay_dockbook_page_removed      (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);
static void         clay_dockbook_page_reordered    (GtkNotebook    *notebook,
                                                     GtkWidget      *child,
                                                     guint           page_num);

static gboolean     clay_dockbook_tab_scroll_cb     (GtkWidget      *widget,
                                                     GdkEventScroll *event);

static gboolean     clay_dockbook_menu_button_press (ClayDockbook   *dockbook,
                                                     GdkEventButton *bevent,
                                                     GtkWidget      *button);
static gboolean     clay_dockbook_show_menu         (ClayDockbook   *dockbook);
static void         clay_dockbook_menu_end          (ClayDockable   *dockable);
static void         clay_dockbook_tab_locked_notify (ClayDockable   *dockable,
                                                     GParamSpec     *pspec,
                                                     ClayDockbook   *dockbook);

static void         clay_dockbook_help_func         (const gchar    *help_id,
                                                     gpointer        help_data);


G_DEFINE_TYPE_WITH_PRIVATE (ClayDockbook, clay_dockbook, GTK_TYPE_NOTEBOOK)

#define parent_class clay_dockbook_parent_class

static guint dockbook_signals[LAST_SIGNAL] = { 0 };

static const GtkTargetEntry dialog_target_table[] = { CLAY_TARGET_NOTEBOOK_TAB };

static GList *drag_callbacks = NULL;


static void
clay_dockbook_class_init (ClayDockbookClass *klass)
{
  GtkWidgetClass   *widget_class   = GTK_WIDGET_CLASS (klass);
  GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

  dockbook_signals[DOCKABLE_ADDED] =
    g_signal_new ("dockable-added",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockbookClass, dockable_added),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REMOVED] =
    g_signal_new ("dockable-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockbookClass, dockable_removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCKABLE);

  dockbook_signals[DOCKABLE_REORDERED] =
    g_signal_new ("dockable-reordered",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDockbookClass, dockable_reordered),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  CLAY_TYPE_DOCKABLE);

  widget_class->style_updated    = clay_dockbook_style_updated;
  widget_class->drag_begin       = clay_dockbook_drag_begin;
  widget_class->drag_end         = clay_dockbook_drag_end;
  widget_class->drag_motion      = clay_dockbook_drag_motion;
  widget_class->drag_drop        = clay_dockbook_drag_drop;
  widget_class->popup_menu       = clay_dockbook_popup_menu;

  notebook_class->create_window  = clay_dockbook_create_window;
  notebook_class->page_added     = clay_dockbook_page_added;
  notebook_class->page_removed   = clay_dockbook_page_removed;
  notebook_class->page_reordered = clay_dockbook_page_reordered;

  klass->dockable_added          = NULL;
  klass->dockable_removed        = NULL;
  klass->dockable_reordered      = NULL;

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("tab-icon-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_TAB_ICON_SIZE,
                                                              CLAY_PARAM_READABLE));
}

static void
clay_dockbook_init (ClayDockbook *dockbook)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (dockbook);
  GtkWidget   *image;

  dockbook->p = clay_dockbook_get_instance_private (dockbook);

  /* Various init */
  gtk_notebook_popup_enable (notebook);
  gtk_notebook_set_scrollable (notebook, TRUE);
  gtk_notebook_set_show_border (notebook, FALSE);
  gtk_notebook_set_show_tabs (notebook, TRUE);
  gtk_notebook_set_group_name (notebook, "clay-dockbook");

  gtk_widget_add_events (GTK_WIDGET (notebook),
                         GDK_SCROLL_MASK);
  g_signal_connect (GTK_WIDGET (notebook), "scroll-event",
                    G_CALLBACK (clay_dockbook_tab_scroll_cb),
                    NULL);

  gtk_drag_dest_set (GTK_WIDGET (dockbook),
                     0,
                     dialog_target_table, G_N_ELEMENTS (dialog_target_table),
                     GDK_ACTION_MOVE);

  /* Menu button */
  dockbook->p->menu_button = gtk_button_new ();
  gtk_widget_set_can_focus (dockbook->p->menu_button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (dockbook->p->menu_button),
                         GTK_RELIEF_NONE);
  gtk_notebook_set_action_widget (notebook,
                                  dockbook->p->menu_button,
                                  GTK_PACK_END);
  gtk_widget_set_visible (dockbook->p->menu_button, TRUE);

  image = gtk_image_new_from_icon_name (CLAY_ICON_MENU_LEFT,
                                        GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
  gtk_image_set_from_icon_name (GTK_IMAGE (image), CLAY_ICON_MENU_LEFT,
                                GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (dockbook->p->menu_button), image);
  gtk_widget_set_visible (image, TRUE);

  clay_help_set_help_data (dockbook->p->menu_button, _("Configure this tab"),
                           CLAY_HELP_DOCK_TAB_MENU);

  g_signal_connect_swapped (dockbook->p->menu_button, "button-press-event",
                            G_CALLBACK (clay_dockbook_menu_button_press),
                            dockbook);
}

static void
clay_dockbook_style_updated (GtkWidget *widget)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (widget);
  ClayContext  *context;
  GtkWidget    *tab_widget;
  GList        *children;
  GList        *iter;
  GtkIconSize   tab_size = DEFAULT_TAB_ICON_SIZE;
  gint          icon_size = 12;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (! dockbook->p->dock ||
      ! (context = clay_dock_get_context (dockbook->p->dock)))
    return;

  /* Update size of 'Configure this tab' icon */
  children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  for (iter = children; iter; iter = g_list_next (iter))
    {
      ClayDockable *dockable = CLAY_DOCKABLE (iter->data);

      tab_widget = clay_dockbook_create_tab_widget (dockbook, dockable);
      gtk_notebook_set_tab_label (GTK_NOTEBOOK (dockbook),
                                  GTK_WIDGET (dockable),
                                  tab_widget);
    }
  g_list_free (children);

  children = gtk_container_get_children (GTK_CONTAINER (dockbook->p->menu_button));
  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);
  gtk_icon_size_lookup (tab_size, &icon_size, NULL);
  gtk_image_set_pixel_size (GTK_IMAGE (children->data), icon_size * 0.75f);
  g_list_free (children);

  clay_dock_invalidate_geometry (CLAY_DOCK (dockbook->p->dock));
}

static void
clay_dockbook_drag_begin (GtkWidget      *widget,
                          GdkDragContext *context)
{
  GList *iter;

  if (GTK_WIDGET_CLASS (parent_class)->drag_begin)
    GTK_WIDGET_CLASS (parent_class)->drag_begin (widget, context);

  iter = drag_callbacks;

  while (iter)
    {
      ClayDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, TRUE, callback_data->data);
    }
}

static void
clay_dockbook_drag_end (GtkWidget      *widget,
                        GdkDragContext *context)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      ClayDockbookDragCallbackData *callback_data = iter->data;

      iter = g_list_next (iter);

      callback_data->callback (context, FALSE, callback_data->data);
    }

  if (GTK_WIDGET_CLASS (parent_class)->drag_end)
    GTK_WIDGET_CLASS (parent_class)->drag_end (widget, context);
}

static gboolean
clay_dockbook_drag_motion (GtkWidget      *widget,
                           GdkDragContext *context,
                           gint            x,
                           gint            y,
                           guint           time)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (widget);

  if (clay_paned_box_will_handle_drag (dockbook->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      gdk_drag_status (context, 0, time);

      return FALSE;
    }

  return GTK_WIDGET_CLASS (parent_class)->drag_motion (widget, context,
                                                       x, y, time);
}

static gboolean
clay_dockbook_drag_drop (GtkWidget      *widget,
                         GdkDragContext *context,
                         gint            x,
                         gint            y,
                         guint           time)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (widget);

  if (clay_paned_box_will_handle_drag (dockbook->p->drag_handler,
                                       widget,
                                       context,
                                       x, y,
                                       time))
    {
      return FALSE;
    }

  if (widget == gtk_drag_get_source_widget (context))
    {
      GList *children   = gtk_container_get_children (GTK_CONTAINER (widget));
      gint   n_children = g_list_length (children);

      g_list_free (children);

      /* we dragged the only tab, and want to drop it back on the same
       * notebook, this would remove and add the page, causing the
       * dockbook to be destroyed in the process because it becomes
       * empty
       */
      if (n_children == 1)
        return FALSE;
    }

  return GTK_WIDGET_CLASS (parent_class)->drag_drop (widget, context,
                                                     x, y, time);
}

static gboolean
clay_dockbook_popup_menu (GtkWidget *widget)
{
  return clay_dockbook_show_menu (CLAY_DOCKBOOK (widget));
}

static GtkNotebook *
clay_dockbook_create_window (GtkNotebook *notebook,
                             GtkWidget   *page,
                             gint         x,
                             gint         y)
{
  ClayDockbook      *dockbook = CLAY_DOCKBOOK (notebook);
  ClayDialogFactory *dialog_factory;
  ClayMenuFactory   *menu_factory;
  ClayDockWindow    *src_dock_window;
  ClayDock          *src_dock;
  GtkWidget         *new_dock;
  ClayDockWindow    *new_dock_window;
  GtkWidget         *new_dockbook;

  src_dock        = clay_dockbook_get_dock (dockbook);
  src_dock_window = clay_dock_window_from_dock (src_dock);

  dialog_factory = clay_dock_get_dialog_factory (src_dock);
  menu_factory   = menus_get_global_menu_factory (clay_dialog_factory_get_context (dialog_factory)->clay);

  new_dock = clay_dock_with_window_new (dialog_factory,
                                        clay_widget_get_monitor (page),
                                        FALSE);

  new_dock_window = clay_dock_window_from_dock (CLAY_DOCK (new_dock));
  gtk_window_set_position (GTK_WINDOW (new_dock_window), GTK_WIN_POS_MOUSE);
  if (src_dock_window)
    clay_dock_window_setup (new_dock_window, src_dock_window);

  new_dockbook = clay_dockbook_new (menu_factory);

  clay_dock_add_book (CLAY_DOCK (new_dock), CLAY_DOCKBOOK (new_dockbook), 0);

  gtk_widget_set_visible (GTK_WIDGET (new_dock_window), TRUE);
  gtk_widget_set_visible (new_dock, TRUE);

  return GTK_NOTEBOOK (new_dockbook);
}

static void
clay_dockbook_page_added (GtkNotebook *notebook,
                          GtkWidget   *child,
                          guint        page_num)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (notebook);
  ClayDockable *dockable = CLAY_DOCKABLE (child);
  GtkWidget    *tab_widget;
  GtkWidget    *menu_widget;

  CLAY_LOG (DND, "ClayDockable %p added to ClayDockbook %p",
            dockable, dockbook);

  tab_widget = clay_dockbook_create_tab_widget (dockbook, dockable);

  /* For the notebook right-click menu, always use the icon style */
  menu_widget =
    clay_dockable_create_tab_widget (dockable,
                                     clay_dock_get_context (dockbook->p->dock),
                                     CLAY_TAB_STYLE_ICON_BLURB,
                                     GTK_ICON_SIZE_MENU);

  gtk_notebook_set_tab_label (notebook, child, tab_widget);
  gtk_notebook_set_menu_label (notebook, child, menu_widget);

  if (! clay_dockable_get_locked (dockable))
    {
      gtk_notebook_set_tab_reorderable (notebook, child, TRUE);
      gtk_notebook_set_tab_detachable (notebook, child, TRUE);
    }

  clay_dockable_set_dockbook (dockable, dockbook);

  clay_dockable_set_context (dockable,
                             clay_dock_get_context (dockbook->p->dock));

  g_signal_connect (dockable, "notify::locked",
                    G_CALLBACK (clay_dockbook_tab_locked_notify),
                    dockbook);

  gtk_widget_set_visible (child, TRUE);
  gtk_notebook_set_current_page (notebook, page_num);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_ADDED], 0, dockable);
}

static void
clay_dockbook_page_removed (GtkNotebook *notebook,
                            GtkWidget   *child,
                            guint        page_num)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (notebook);
  ClayDockable *dockable = CLAY_DOCKABLE (child);

  CLAY_LOG (DND, "ClayDockable removed %p from ClayDockbook %p",
            dockable, dockbook);

  g_signal_handlers_disconnect_by_func (dockable,
                                        G_CALLBACK (clay_dockbook_tab_locked_notify),
                                        dockbook);

  clay_dockable_set_dockbook (dockable, NULL);

  clay_dockable_set_context (dockable, NULL);

  g_signal_emit (dockbook, dockbook_signals[DOCKABLE_REMOVED], 0, dockable);

  if (dockbook->p->dock)
    {
      GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));

      if (! children)
        clay_dock_remove_book (dockbook->p->dock, dockbook);

      g_list_free (children);
    }
}

/* Restore GTK2 behavior of mouse-scrolling to switch between
 * notebook tabs. References Geany's notebook_tab_bar_click_cb ()
 * at https://github.com/geany/geany/blob/master/src/notebook.c
 */
static gboolean
clay_dockbook_tab_scroll_cb (GtkWidget      *widget,
                             GdkEventScroll *event)
{
  GtkNotebook *notebook = GTK_NOTEBOOK (widget);
  GtkWidget   *page     = NULL;

  page = gtk_notebook_get_nth_page (notebook,
                                    gtk_notebook_get_current_page (notebook));
  if (! page)
    return FALSE;

  switch (event->direction)
    {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page (notebook);
      break;

    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page (notebook);
      break;

    default:
      break;
    }

  return TRUE;
}

static void
clay_dockbook_page_reordered (GtkNotebook *notebook,
                              GtkWidget   *child,
                              guint        page_num)
{
}

static gboolean
clay_dockbook_menu_button_press (ClayDockbook   *dockbook,
                                 GdkEventButton *bevent,
                                 GtkWidget      *button)
{
  gboolean handled = FALSE;

  if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS && bevent->button == 1)
    handled = clay_dockbook_show_menu (dockbook);

  return handled;
}

static gboolean
clay_dockbook_show_menu (ClayDockbook *dockbook)
{
  ClayUIManager *dockbook_ui_manager;
  ClayUIManager *dialog_ui_manager;
  const gchar   *dialog_ui_path;
  gpointer       dialog_popup_data;
  ClayDockable  *dockable;
  gint           page_num;

  dockbook_ui_manager = dockbook->p->ui_manager;

  if (! dockbook_ui_manager)
    return FALSE;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));
  dockable = CLAY_DOCKABLE (gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook),
                                                       page_num));

  if (! dockable)
    return FALSE;

  dialog_ui_manager = clay_dockable_get_menu (dockable,
                                              &dialog_ui_path,
                                              &dialog_popup_data);

  /*  an action callback may destroy both dockable and dockbook, so
   *  reference them for clay_dockbook_menu_end()
   */
  g_object_ref (dockable);
  g_object_set_data_full (G_OBJECT (dockable), CLAY_DOCKABLE_DETACH_REF_KEY,
                          g_object_ref (dockbook),
                          g_object_unref);

  if (dialog_ui_manager)
    clay_ui_manager_update (dialog_ui_manager, dialog_popup_data);

  clay_ui_manager_update (dockbook_ui_manager, dockable);

  clay_ui_manager_ui_popup_at_widget (dockbook_ui_manager,
                                      "/dockable-popup",
                                      dialog_ui_manager,
                                      dialog_ui_path,
                                      dockbook->p->menu_button,
                                      GDK_GRAVITY_WEST,
                                      GDK_GRAVITY_NORTH_EAST,
                                      NULL,
                                      (GDestroyNotify) clay_dockbook_menu_end,
                                      dockable);

  return TRUE;
}

static void
clay_dockbook_menu_end (ClayDockable *dockable)
{
  /*  release clay_dockbook_show_menu()'s references  */
  g_object_set_data (G_OBJECT (dockable), CLAY_DOCKABLE_DETACH_REF_KEY, NULL);
  g_object_unref (dockable);
}


/*  public functions  */

GtkWidget *
clay_dockbook_new (ClayMenuFactory *menu_factory)
{
  ClayDockbook *dockbook;

  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  dockbook = g_object_new (CLAY_TYPE_DOCKBOOK, NULL);

  dockbook->p->ui_manager = clay_menu_factory_get_manager (menu_factory,
                                                           "<Dockable>",
                                                           dockbook);

  clay_help_connect (GTK_WIDGET (dockbook), NULL, clay_dockbook_help_func,
                     CLAY_HELP_DOCK, dockbook, NULL);

  return GTK_WIDGET (dockbook);
}

ClayDock *
clay_dockbook_get_dock (ClayDockbook *dockbook)
{
  g_return_val_if_fail (CLAY_IS_DOCKBOOK (dockbook), NULL);

  return dockbook->p->dock;
}

void
clay_dockbook_set_dock (ClayDockbook *dockbook,
                        ClayDock     *dock)
{
  ClayContext *context;

  g_return_if_fail (CLAY_IS_DOCKBOOK (dockbook));
  g_return_if_fail (dock == NULL || CLAY_IS_DOCK (dock));

  if (dockbook->p->dock &&
      (context = clay_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_handlers_disconnect_by_func (CLAY_GUI_CONFIG (context->clay->config),
                                            G_CALLBACK (clay_dockbook_style_updated),
                                            dockbook);
    }

  dockbook->p->dock = dock;

  if (dockbook->p->dock &&
      (context = clay_dock_get_context (dockbook->p->dock)) != NULL)
    {
      g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                               "notify::theme",
                               G_CALLBACK (clay_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                               "notify::override-theme-icon-size",
                               G_CALLBACK (clay_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
      g_signal_connect_object (CLAY_GUI_CONFIG (context->clay->config),
                               "notify::custom-icon-size",
                               G_CALLBACK (clay_dockbook_style_updated),
                               dockbook, G_CONNECT_AFTER | G_CONNECT_SWAPPED);
    }
}

/**
 * clay_dockbook_add_from_dialog_factory:
 * @dockbook:    The #DockBook
 * @identifiers: The dockable identifier(s)
 *
 * Add a dockable from the dialog factory associated with the dockbook.
 **/
GtkWidget *
clay_dockbook_add_from_dialog_factory (ClayDockbook *dockbook,
                                       const gchar  *identifiers)
{
  GtkWidget *dockable;
  ClayDock  *dock;
  gchar     *identifier;
  gchar     *p;

  g_return_val_if_fail (CLAY_IS_DOCKBOOK (dockbook), NULL);
  g_return_val_if_fail (identifiers != NULL, NULL);

  identifier = g_strdup (identifiers);

  p = strchr (identifier, '|');

  if (p)
    *p = '\0';

  dock     = clay_dockbook_get_dock (dockbook);
  dockable = clay_dialog_factory_dockable_new (clay_dock_get_dialog_factory (dock),
                                               dock,
                                               identifier, -1);

  g_free (identifier);

  /*  Maybe clay_dialog_factory_dockable_new() returned an already
   *  existing singleton dockable, so check if it already is
   *  attached to a dockbook.
   */
  if (dockable && ! clay_dockable_get_dockbook (CLAY_DOCKABLE (dockable)))
    gtk_notebook_append_page (GTK_NOTEBOOK (dockbook),
                              dockable, NULL);

  return dockable;
}

/**
 * clay_dockbook_update_with_context:
 * @dockbook:
 * @context:
 *
 * Set @context on all dockables in @dockbook.
 **/
void
clay_dockbook_update_with_context (ClayDockbook *dockbook,
                                   ClayContext  *context)
{
  GList *children = gtk_container_get_children (GTK_CONTAINER (dockbook));
  GList *iter;

  for (iter = children; iter; iter = g_list_next (iter))
    {
      ClayDockable *dockable = CLAY_DOCKABLE (iter->data);

      clay_dockable_set_context (dockable, context);
    }

  g_list_free (children);
}

GtkWidget *
clay_dockbook_create_tab_widget (ClayDockbook *dockbook,
                                 ClayDockable *dockable)
{
  GtkWidget      *tab_widget;
  ClayDockWindow *dock_window;
  ClayAction     *action = NULL;
  GtkIconSize     tab_size = DEFAULT_TAB_ICON_SIZE;

  gtk_widget_style_get (GTK_WIDGET (dockbook),
                        "tab-icon-size", &tab_size,
                        NULL);
  tab_widget =
    clay_dockable_create_tab_widget (dockable,
                                     clay_dock_get_context (dockbook->p->dock),
                                     clay_dockable_get_tab_style (dockable),
                                     tab_size);

  if (CLAY_IS_VIEW (tab_widget))
    {
      GtkWidget *event_box;

      event_box = gtk_event_box_new ();
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);
      gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), TRUE);
      gtk_container_add (GTK_CONTAINER (event_box), tab_widget);
      gtk_widget_set_visible (tab_widget, TRUE);

      tab_widget = event_box;
    }

  /* EEK */
  dock_window = clay_dock_window_from_dock (dockbook->p->dock);
  if (dock_window &&
      clay_dock_container_get_ui_manager (CLAY_DOCK_CONTAINER (dock_window)))
    {
      const gchar *dialog_id;

      dialog_id = g_object_get_data (G_OBJECT (dockable),
                                     "clay-dialog-identifier");

      if (dialog_id)
        {
          ClayDockContainer *dock_container;
          ClayActionGroup   *group;

          dock_container = CLAY_DOCK_CONTAINER (dock_window);

          group = clay_ui_manager_get_action_group
            (clay_dock_container_get_ui_manager (dock_container), "dialogs");

          if (group)
            {
              GList *actions;
              GList *list;

              actions = clay_action_group_list_actions (group);

              for (list = actions; list; list = g_list_next (list))
                {
                  if (CLAY_IS_STRING_ACTION (list->data) &&
                      strstr (CLAY_STRING_ACTION (list->data)->value,
                              dialog_id))
                    {
                      action = list->data;
                      break;
                    }
                }

              g_list_free (actions);
            }
        }
    }

  if (action)
    clay_widget_set_accel_help (tab_widget, action);
  else
    clay_help_set_help_data (tab_widget,
                             clay_dockable_get_blurb (dockable),
                             clay_dockable_get_help_id (dockable));

  return tab_widget;
}

/**
 * clay_dockable_set_drag_handler:
 * @dockable:
 * @handler:
 *
 * Set a drag handler that will be asked if it will handle drag events
 * before the dockbook handles the event itself.
 **/
void
clay_dockbook_set_drag_handler (ClayDockbook *dockbook,
                                ClayPanedBox *drag_handler)
{
  g_return_if_fail (CLAY_IS_DOCKBOOK (dockbook));

  dockbook->p->drag_handler = drag_handler;
}

void
clay_dockbook_add_drag_callback (ClayDockbookDragCallback callback,
                                 gpointer                 data)
{
  ClayDockbookDragCallbackData *callback_data;

  callback_data = g_slice_new (ClayDockbookDragCallbackData);

  callback_data->callback = callback;
  callback_data->data     = data;

  drag_callbacks = g_list_prepend (drag_callbacks, callback_data);
}

void
clay_dockbook_remove_drag_callback (ClayDockbookDragCallback callback,
                                    gpointer                 data)
{
  GList *iter;

  iter = drag_callbacks;

  while (iter)
    {
      ClayDockbookDragCallbackData *callback_data = iter->data;
      GList                        *next          = g_list_next (iter);

      if (callback_data->callback == callback &&
          callback_data->data     == data)
        {
          g_slice_free (ClayDockbookDragCallbackData, callback_data);

          drag_callbacks = g_list_delete_link (drag_callbacks, iter);
        }

      iter = next;
    }
}


/*  private functions  */

static void
clay_dockbook_tab_locked_notify (ClayDockable *dockable,
                                 GParamSpec   *pspec,
                                 ClayDockbook *dockbook)
{
  gboolean locked = clay_dockable_get_locked (dockable);

  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (dockbook),
                                    GTK_WIDGET (dockable), ! locked);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (dockbook),
                                   GTK_WIDGET (dockable), ! locked);
}

static void
clay_dockbook_help_func (const gchar *help_id,
                         gpointer     help_data)
{
  ClayDockbook *dockbook = CLAY_DOCKBOOK (help_data);
  GtkWidget    *dockable;
  gint          page_num;

  page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

  dockable = gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);

  if (CLAY_IS_DOCKABLE (dockable))
    clay_standard_help_func (clay_dockable_get_help_id (CLAY_DOCKABLE (dockable)),
                             NULL);
  else
    clay_standard_help_func (CLAY_HELP_DOCK, NULL);
}
