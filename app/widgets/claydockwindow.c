/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockwindow.c
 * Copyright (C) 2001-2005 Michael Natterer <mitch@gimp.org>
 * Copyright (C)      2009 Martin Nordholts <martinn@src.gnome.org>
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
#include <gdk/gdk.h>
#ifdef _WIN32
#include <gdk/gdkwin32.h>
#endif
#ifdef PLATFORM_OSX
#include <gdk/quartz/gdkquartz-cocoa-access.h>
#endif

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "dialogs/dialogs.h" /* FIXME, we are in the widget layer */

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claycontainer.h"
#include "core/claycontainer.h"
#include "core/claylist.h"
#include "core/clayimage.h"

#include "menus/menus.h"

#include "claycontainercombobox.h"
#include "claycontainerview.h"
#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockbook.h"
#include "claydockcolumns.h"
#include "claydockcontainer.h"
#include "claydockwindow.h"
#include "clayhelp-ids.h"
#include "claymenufactory.h"
#include "claysessioninfo-aux.h"
#include "claysessioninfo.h"
#include "claysessionmanaged.h"
#include "claytoolbox.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"
#include "claywindow.h"

#include "clay-intl.h"


#define DEFAULT_DOCK_HEIGHT          300
#define DEFAULT_MENU_VIEW_SIZE       GTK_ICON_SIZE_SMALL_TOOLBAR
#define AUX_INFO_SHOW_IMAGE_MENU     "show-image-menu"
#define AUX_INFO_FOLLOW_ACTIVE_IMAGE "follow-active-image"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_DIALOG_FACTORY,
  PROP_UI_MANAGER_NAME,
  PROP_IMAGE_CONTAINER,
  PROP_DISPLAY_CONTAINER,
  PROP_ALLOW_DOCKBOOK_ABSENCE
};


struct _ClayDockWindowPrivate
{
  ClayContext       *context;

  ClayDialogFactory *dialog_factory;

  gchar             *ui_manager_name;
  ClayUIManager     *ui_manager;
  GQuark             image_flush_handler_id;

  ClayDockColumns   *dock_columns;

  gboolean           allow_dockbook_absence;

  guint              update_title_idle_id;

  gint               ID;

  ClayContainer     *image_container;
  ClayContainer     *display_container;

  gboolean           show_image_menu;
  gboolean           auto_follow_active;

  GtkWidget         *image_combo;
  GtkWidget         *auto_button;
};


static void            clay_dock_window_dock_container_iface_init (ClayDockContainerInterface *iface);
static void            clay_dock_window_session_managed_iface_init(ClaySessionManagedInterface*iface);
static void            clay_dock_window_constructed               (GObject                    *object);
static void            clay_dock_window_dispose                   (GObject                    *object);
static void            clay_dock_window_finalize                  (GObject                    *object);
static void            clay_dock_window_set_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   const GValue               *value,
                                                                   GParamSpec                 *pspec);
static void            clay_dock_window_get_property              (GObject                    *object,
                                                                   guint                       property_id,
                                                                   GValue                     *value,
                                                                   GParamSpec                 *pspec);
static void            clay_dock_window_style_updated             (GtkWidget                  *widget);
static gboolean        clay_dock_window_delete_event              (GtkWidget                  *widget,
                                                                   GdkEventAny                *event);
static gboolean        clay_dock_window_key_press_event           (GtkWidget                  *widget,
                                                                   GdkEventKey                *event);
#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
static void            clay_dock_window_realize                   (ClayDockWindow             *dock_window,
                                                                   gpointer                    data);
static void            clay_dock_window_map                       (ClayDockWindow             *dock_window,
                                                                   gpointer                    data);
#endif
static GList         * clay_dock_window_get_docks                 (ClayDockContainer          *dock_container);
static GList         * clay_dock_window_get_docks_self            (ClayDockWindow             *self);
static ClayDialogFactory * clay_dock_window_get_dialog_factory    (ClayDockContainer          *dock_container);
static ClayUIManager * clay_dock_window_get_ui_manager            (ClayDockContainer          *dock_container);
static void            clay_dock_window_add_dock_from_session     (ClayDockContainer          *dock_container,
                                                                   ClayDock                   *dock,
                                                                   ClaySessionInfoDock        *dock_info);
static GList         * clay_dock_window_get_aux_info              (ClaySessionManaged         *session_managed);
static void            clay_dock_window_set_aux_info              (ClaySessionManaged         *session_managed,
                                                                   GList                      *aux_info);
static ClayAlignmentType
                       clay_dock_window_get_dock_side             (ClayDockContainer          *dock_container,
                                                                   ClayDock                   *dock);
static gboolean        clay_dock_window_should_add_to_recent      (ClayDockWindow             *dock_window);
static void            clay_dock_window_display_changed           (ClayDockWindow             *dock_window,
                                                                   ClayDisplay                *display,
                                                                   ClayContext                *context);
static void            clay_dock_window_image_changed             (ClayDockWindow             *dock_window,
                                                                   ClayImage                  *image,
                                                                   ClayContext                *context);
static void            clay_dock_window_image_flush               (ClayImage                  *image,
                                                                   gboolean                    invalidate_preview,
                                                                   ClayDockWindow             *dock_window);
static void            clay_dock_window_update_title              (ClayDockWindow             *dock_window);
static gboolean        clay_dock_window_update_title_idle         (ClayDockWindow             *dock_window);
#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
static gboolean        clay_dock_window_update_focus_idle         (gpointer                    data);
#endif
static gchar         * clay_dock_window_get_description           (ClayDockWindow             *dock_window,
                                                                   gboolean                    complete);
static void            clay_dock_window_dock_removed              (ClayDockWindow             *dock_window,
                                                                   ClayDock                   *dock,
                                                                   ClayDockColumns            *dock_columns);
static void            clay_dock_window_factory_display_changed   (ClayContext                *context,
                                                                   ClayDisplay                *display,
                                                                   ClayDock                   *dock);
static void            clay_dock_window_factory_image_changed     (ClayContext                *context,
                                                                   ClayImage                  *image,
                                                                   ClayDock                   *dock);
static void            clay_dock_window_auto_clicked              (GtkWidget                  *widget,
                                                                   ClayDock                   *dock);


G_DEFINE_TYPE_WITH_CODE (ClayDockWindow, clay_dock_window, CLAY_TYPE_WINDOW,
                         G_ADD_PRIVATE (ClayDockWindow)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCK_CONTAINER,
                                                clay_dock_window_dock_container_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SESSION_MANAGED,
                                                clay_dock_window_session_managed_iface_init))

#define parent_class clay_dock_window_parent_class


static void
clay_dock_window_class_init (ClayDockWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = clay_dock_window_constructed;
  object_class->dispose       = clay_dock_window_dispose;
  object_class->finalize      = clay_dock_window_finalize;
  object_class->set_property  = clay_dock_window_set_property;
  object_class->get_property  = clay_dock_window_get_property;

  widget_class->style_updated   = clay_dock_window_style_updated;
  widget_class->delete_event    = clay_dock_window_delete_event;
  widget_class->key_press_event = clay_dock_window_key_press_event;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        CLAY_TYPE_CONTEXT,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DIALOG_FACTORY,
                                   g_param_spec_object ("dialog-factory",
                                                        NULL, NULL,
                                                        CLAY_TYPE_DIALOG_FACTORY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_UI_MANAGER_NAME,
                                   g_param_spec_string ("ui-manager-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_IMAGE_CONTAINER,
                                   g_param_spec_object ("image-container",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DISPLAY_CONTAINER,
                                   g_param_spec_object ("display-container",
                                                        NULL, NULL,
                                                        CLAY_TYPE_CONTAINER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_ALLOW_DOCKBOOK_ABSENCE,
                                   g_param_spec_boolean ("allow-dockbook-absence",
                                                         NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));


  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("default-height",
                                                             NULL, NULL,
                                                             -1, G_MAXINT,
                                                             DEFAULT_DOCK_HEIGHT,
                                                             CLAY_PARAM_READABLE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_enum ("menu-preview-size",
                                                              NULL, NULL,
                                                              GTK_TYPE_ICON_SIZE,
                                                              DEFAULT_MENU_VIEW_SIZE,
                                                              CLAY_PARAM_READABLE));
}

static void
clay_dock_window_init (ClayDockWindow *dock_window)
{
  static gint  dock_window_ID = 1;
  gchar       *name           = NULL;

  dock_window->p = clay_dock_window_get_instance_private (dock_window);
  dock_window->p->ID                 = dock_window_ID++;
  dock_window->p->auto_follow_active = TRUE;

  name = g_strdup_printf ("clay-dock-%d", dock_window->p->ID);
  gtk_widget_set_name (GTK_WIDGET (dock_window), name);
  g_free (name);

  gtk_window_set_resizable (GTK_WINDOW (dock_window), TRUE);
  gtk_window_set_focus_on_map (GTK_WINDOW (dock_window), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dock_window), FALSE);

#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
  g_signal_connect (dock_window, "realize",
                    G_CALLBACK (clay_dock_window_realize),
                    NULL);
  g_signal_connect (dock_window, "map",
                    G_CALLBACK (clay_dock_window_map),
                    NULL);
#endif
}

static void
clay_dock_window_dock_container_iface_init (ClayDockContainerInterface *iface)
{
  iface->get_docks          = clay_dock_window_get_docks;
  iface->get_dialog_factory = clay_dock_window_get_dialog_factory;
  iface->get_ui_manager     = clay_dock_window_get_ui_manager;
  iface->add_dock           = clay_dock_window_add_dock_from_session;
  iface->get_dock_side      = clay_dock_window_get_dock_side;
}

static void
clay_dock_window_session_managed_iface_init (ClaySessionManagedInterface *iface)
{
  iface->get_aux_info = clay_dock_window_get_aux_info;
  iface->set_aux_info = clay_dock_window_set_aux_info;
}

static void
clay_dock_window_constructed (GObject *object)
{
  ClayDockWindow  *dock_window = CLAY_DOCK_WINDOW (object);
  ClayGuiConfig   *config;
  ClayContext     *factory_context;
  ClayMenuFactory *menu_factory;
  Clay            *clay;
  gint             menu_view_width  = -1;
  gint             menu_view_height = -1;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay   = Clay (dock_window->p->context->clay);
  config = CLAY_GUI_CONFIG (clay->config);

  /* Create a separate context per dock so that docks can be bound to
   * a specific image and does not necessarily have to follow the
   * active image in the user context
   */
  g_object_unref (dock_window->p->context);
  dock_window->p->context           = clay_context_new (clay, "Dock Context", NULL);
  dock_window->p->image_container   = clay->images;
  dock_window->p->display_container = clay->displays;

  factory_context =
    clay_dialog_factory_get_context (dock_window->p->dialog_factory);

  /* Setup hints */
  clay_window_set_hint (GTK_WINDOW (dock_window), config->dock_window_hint);

  menu_factory = menus_get_global_menu_factory (clay);

  /* Make image window related keyboard shortcuts work also when a
   * dock window is the focused window
   */
  dock_window->p->ui_manager =
    clay_menu_factory_get_manager (menu_factory,
                                   dock_window->p->ui_manager_name,
                                   dock_window);

  g_signal_connect_object (dock_window->p->context, "display-changed",
                           G_CALLBACK (clay_dock_window_display_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (dock_window->p->context, "image-changed",
                           G_CALLBACK (clay_dock_window_image_changed),
                           dock_window,
                           G_CONNECT_SWAPPED);

  dock_window->p->image_flush_handler_id =
    clay_container_add_handler (clay->images, "flush",
                                G_CALLBACK (clay_dock_window_image_flush),
                                dock_window);

  clay_context_define_properties (dock_window->p->context,
                                  CLAY_CONTEXT_PROP_MASK_ALL &
                                  ~(CLAY_CONTEXT_PROP_MASK_IMAGE |
                                    CLAY_CONTEXT_PROP_MASK_DISPLAY),
                                  FALSE);
  clay_context_set_parent (dock_window->p->context,
                           factory_context);

  /* Setup widget hierarchy */
  {
    GtkWidget *vbox = NULL;

    /* Top-level GtkVBox */
    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add (GTK_CONTAINER (dock_window), vbox);
    gtk_widget_set_visible (vbox, TRUE);

    /* Image selection menu */
    {
      GtkWidget *hbox = NULL;

      /* GtkHBox */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      if (dock_window->p->show_image_menu)
        gtk_widget_set_visible (hbox, TRUE);

      /* Image combo */
      dock_window->p->image_combo = clay_container_combo_box_new (NULL, NULL, 16, 1);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->image_combo, TRUE, TRUE, 0);
      g_signal_connect (dock_window->p->image_combo, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &dock_window->p->image_combo);
      clay_help_set_help_data (dock_window->p->image_combo,
                               NULL, CLAY_HELP_DOCK_IMAGE_MENU);
      gtk_widget_set_visible (dock_window->p->image_combo, TRUE);

      /* Auto button */
      dock_window->p->auto_button = gtk_toggle_button_new_with_label (_("Auto"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                    dock_window->p->auto_follow_active);
      gtk_box_pack_start (GTK_BOX (hbox), dock_window->p->auto_button, FALSE, FALSE, 0);
      gtk_widget_set_visible (dock_window->p->auto_button, TRUE);

      g_signal_connect (dock_window->p->auto_button, "clicked",
                        G_CALLBACK (clay_dock_window_auto_clicked),
                        dock_window);

      clay_help_set_help_data (dock_window->p->auto_button,
                               _("When enabled, the dialog automatically "
                                 "follows the image you are working on."),
                               CLAY_HELP_DOCK_AUTO_BUTTON);
    }

    /* ClayDockColumns */
    /* Let the ClayDockColumns mirror the context so that a ClayDock can
     * get it when inside a dock window. We do the same thing in the
     * ClayImageWindow so docks can get the ClayContext there as well
     */
    dock_window->p->dock_columns =
      CLAY_DOCK_COLUMNS (clay_dock_columns_new (dock_window->p->context,
                                                dock_window->p->dialog_factory,
                                                dock_window->p->ui_manager));
    gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (dock_window->p->dock_columns),
                        TRUE, TRUE, 0);
    gtk_widget_set_visible (GTK_WIDGET (dock_window->p->dock_columns), TRUE);
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (clay_dock_window_dock_removed),
                             dock_window,
                             G_CONNECT_SWAPPED);

    g_signal_connect_object (dock_window->p->dock_columns, "dock-added",
                             G_CALLBACK (clay_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (dock_window->p->dock_columns, "dock-removed",
                             G_CALLBACK (clay_dock_window_update_title),
                             dock_window,
                             G_CONNECT_SWAPPED);
  }

  if (dock_window->p->auto_follow_active)
    {
      if (clay_context_get_display (factory_context))
        clay_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    CLAY_CONTEXT_PROP_DISPLAY);
      else
        clay_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    CLAY_CONTEXT_PROP_IMAGE);
    }

  g_signal_connect_object (factory_context, "display-changed",
                           G_CALLBACK (clay_dock_window_factory_display_changed),
                           dock_window,
                           0);
  g_signal_connect_object (factory_context, "image-changed",
                           G_CALLBACK (clay_dock_window_factory_image_changed),
                           dock_window,
                           0);

  gtk_icon_size_lookup (DEFAULT_MENU_VIEW_SIZE,
                        &menu_view_width,
                        &menu_view_height);

  g_object_set (dock_window->p->image_combo,
                "container", dock_window->p->image_container,
                "context",   dock_window->p->context,
                NULL);

  clay_help_connect (GTK_WIDGET (dock_window), NULL, clay_standard_help_func,
                     CLAY_HELP_DOCK, NULL, NULL);

  if (dock_window->p->auto_follow_active)
    {
      if (clay_context_get_display (factory_context))
        clay_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    CLAY_CONTEXT_PROP_DISPLAY);
      else
        clay_context_copy_property (factory_context,
                                    dock_window->p->context,
                                    CLAY_CONTEXT_PROP_IMAGE);
    }
}

static void
clay_dock_window_dispose (GObject *object)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (object);

  if (dock_window->p->update_title_idle_id)
    {
      g_source_remove (dock_window->p->update_title_idle_id);
      dock_window->p->update_title_idle_id = 0;
    }

  if (dock_window->p->image_flush_handler_id)
    {
      clay_container_remove_handler (dock_window->p->context->clay->images,
                                     dock_window->p->image_flush_handler_id);
      dock_window->p->image_flush_handler_id = 0;
    }

  g_clear_object (&dock_window->p->dialog_factory);

  /* Null our reference, that update_title callback dereferences.
   * Gtk will dispose the widget but doesn't know of our reference.
   */
  dock_window->p->dock_columns = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_dock_window_finalize (GObject *object)
{
  ClayDockWindow  *dock_window = CLAY_DOCK_WINDOW (object);
  ClayMenuFactory *menu_factory;
  Clay            *clay;

  clay         = Clay (dock_window->p->context->clay);
  menu_factory = menus_get_global_menu_factory (clay);

  /* If the whole GUI is in destruction, global menu factory might already not
   * exist anymore.
   */
  if (menu_factory != NULL)
    clay_menu_factory_delete_manager (menu_factory,
                                      dock_window->p->ui_manager_name,
                                      dock_window);
  g_clear_pointer (&dock_window->p->ui_manager_name, g_free);
  g_clear_object (&dock_window->p->context);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_dock_window_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      dock_window->p->context = g_value_dup_object (value);
      break;

    case PROP_DIALOG_FACTORY:
      dock_window->p->dialog_factory = g_value_dup_object (value);
      break;

    case PROP_UI_MANAGER_NAME:
      dock_window->p->ui_manager_name = g_value_dup_string (value);
      break;

    case PROP_IMAGE_CONTAINER:
      dock_window->p->image_container = g_value_dup_object (value);
      break;

    case PROP_DISPLAY_CONTAINER:
      dock_window->p->display_container = g_value_dup_object (value);
      break;

    case PROP_ALLOW_DOCKBOOK_ABSENCE:
      dock_window->p->allow_dockbook_absence = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dock_window_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, dock_window->p->context);
      break;

    case PROP_DIALOG_FACTORY:
      g_value_set_object (value, dock_window->p->dialog_factory);
      break;

    case PROP_UI_MANAGER_NAME:
      g_value_set_string (value, dock_window->p->ui_manager_name);
      break;

    case PROP_IMAGE_CONTAINER:
      g_value_set_object (value, dock_window->p->image_container);
      break;

    case PROP_DISPLAY_CONTAINER:
      g_value_set_object (value, dock_window->p->display_container);
      break;

    case PROP_ALLOW_DOCKBOOK_ABSENCE:
      g_value_set_boolean (value, dock_window->p->allow_dockbook_absence);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_dock_window_style_updated (GtkWidget *widget)
{
  ClayDockWindow  *dock_window      = CLAY_DOCK_WINDOW (widget);
  GtkStyleContext *button_style;
  GtkIconSize      menu_view_size;
  gint             default_height = DEFAULT_DOCK_HEIGHT;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "default-height", &default_height,
                        "menu-preview-size", &menu_view_size,
                        NULL);

  gtk_window_set_default_size (GTK_WINDOW (widget), -1, default_height);

  if (dock_window->p->image_combo)
    {
      GtkBorder border;
      gint      menu_view_width  = 18;
      gint      menu_view_height = 18;
      gint      focus_line_width;
      gint      focus_padding;

      gtk_icon_size_lookup (menu_view_size,
                            &menu_view_width,
                            &menu_view_height);

      gtk_widget_style_get (dock_window->p->auto_button,
                            "focus-line-width", &focus_line_width,
                            "focus-padding",    &focus_padding,
                            NULL);

      button_style = gtk_widget_get_style_context (widget);
      gtk_style_context_get_border (button_style,
                                    gtk_widget_get_state_flags (widget),
                                    &border);

      clay_container_view_set_view_size (CLAY_CONTAINER_VIEW (dock_window->p->image_combo),
                                         menu_view_height, 1);

      gtk_widget_set_size_request (dock_window->p->auto_button, -1,
                                   menu_view_height +
                                   2 * (1 /* CHILD_SPACING */ +
                                        border.top            +
                                        border.bottom         +
                                        focus_padding         +
                                        focus_line_width));
    }
}

/**
 * clay_dock_window_delete_event:
 * @widget:
 * @event:
 *
 * Makes sure that when dock windows are closed they are added to the
 * list of recently closed docks so that they are easy to bring back.
 **/
static gboolean
clay_dock_window_delete_event (GtkWidget   *widget,
                               GdkEventAny *event)
{
  ClayDockWindow         *dock_window = CLAY_DOCK_WINDOW (widget);
  ClaySessionInfo        *info        = NULL;
  const gchar            *entry_name  = NULL;
  ClayDialogFactoryEntry *entry       = NULL;
  gchar                  *name        = NULL;

  /* Don't add docks with just a single dockable to the list of
   * recently closed dock since those can be brought back through the
   * normal Windows->Dockable Dialogs menu
   */
  if (! clay_dock_window_should_add_to_recent (dock_window))
    return FALSE;

  info = clay_session_info_new ();

  name = clay_dock_window_get_description (dock_window, TRUE /*complete*/);
  clay_object_set_name (CLAY_OBJECT (info), name);
  g_free (name);

  clay_session_info_get_info_with_widget (info, GTK_WIDGET (dock_window));

  entry_name = (clay_dock_window_has_toolbox (dock_window) ?
                "clay-toolbox-window" :
                "clay-dock-window");
  entry = clay_dialog_factory_find_entry (dock_window->p->dialog_factory,
                                          entry_name);
  clay_session_info_set_factory_entry (info, entry);

  clay_container_add (global_recent_docks, CLAY_OBJECT (info));
  g_object_unref (info);

  return FALSE;
}

/* Make image window related keyboard shortcuts work also when a
   dock window is the focused window. See: #9921 */
static gboolean
clay_dock_window_key_press_event (GtkWidget   *widget,
                                  GdkEventKey *event)
{
  GtkApplication *app;
  gboolean        handled = FALSE;

  /* dockable own shortcuts */
  if (GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
    return TRUE;

  /* clay main window shortcuts */
  app = gtk_window_get_application (GTK_WINDOW (widget));
  if (!app)
    {
      GApplication *default_app = g_application_get_default ();
      if (default_app && GTK_IS_APPLICATION (default_app))
        app = GTK_APPLICATION (default_app);
    }
  if (app)
    {
      GList *app_windows = gtk_application_get_windows (app);
      GList *iter;
      for (iter = app_windows; iter; iter = g_list_next (iter))
        {
          GtkWindow *app_win = GTK_WINDOW (iter->data);
          if (app_win != GTK_WINDOW (widget) &&
              !CLAY_IS_DOCK_WINDOW (app_win) &&
              !GTK_IS_DIALOG (app_win))
            {
              if (gtk_window_activate_key (app_win, event))
                {
                  handled = TRUE;
                  break;
                }
            }
        }
    }

  return handled;
}

#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
static void
clay_dock_window_realize (ClayDockWindow *dock_window,
                          gpointer        data)
{
  ClayGuiConfig *config;
  Clay          *clay;
#ifdef PLATFORM_OSX
  NSWindow      *ns_window;
  NSUInteger     minimize_mask;
  NSButton      *maximize_button;
#endif

  dock_window->p = clay_dock_window_get_instance_private (dock_window);

  if (dock_window->p->context && dock_window->p->context->clay)
    {
      clay   = Clay (dock_window->p->context->clay);
      config = CLAY_GUI_CONFIG (clay->config);

#ifdef G_OS_WIN32
      clay_window_set_title_bar_theme (dock_window->p->context->clay,
                                       GTK_WIDGET (dock_window));
#endif

      if (config->dock_window_hint == CLAY_WINDOW_HINT_UTILITY)
        {
#ifdef GDK_WINDOWING_WAYLAND
          g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                           (GSourceFunc) clay_dock_window_update_focus_idle,
                           g_object_ref (dock_window), g_object_unref);
#endif

#ifdef G_OS_WIN32
          /* keep dockable window transient (since clay_dialog_factory_dialog_new_internal is not enough)*/
          HWND main_window = GetActiveWindow();
          g_object_set_data (G_OBJECT (dock_window), "win32-parent-window", (gpointer) main_window);
          g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                           (GSourceFunc) clay_dock_window_update_focus_idle,
                           g_object_ref (dock_window), g_object_unref);
#endif

#ifdef PLATFORM_OSX
          /* since GTK can not create NSPanel, let's make the NSWindow behave like one */
          ns_window = gdk_quartz_window_get_nswindow (gtk_widget_get_window (GTK_WIDGET (dock_window)));
          if (ns_window)
            {
              /* keep dockable window transient (since clay_widget_set_auto_transient don't work well
                 here: e.g. it makes dockables in front of main window dialogs, which is wrong)*/
              NSWindow *main_window = [NSApp mainWindow];
              g_object_set_data (G_OBJECT (dock_window), "osx-parent-window", main_window);
              /* do NOT call clay_dock_window_update_focus_idle here to avoid glitches */
              [main_window addChildWindow:ns_window ordered:NSWindowAbove];

              /* keep dockable window transient when restored */
              [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidDeminiaturizeNotification
                                                    object:nil
                                                    queue:[NSOperationQueue mainQueue]
                                                    usingBlock:^(NSNotification *note) {
                g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                 clay_dock_window_update_focus_idle,
                                 g_object_ref (dock_window), g_object_unref);
              }];

              /* keep dockable window transient when app becomes active
                 (e.g. after opening clay from terminal )*/
              [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationDidBecomeActiveNotification
                                                    object:nil
                                                    queue:[NSOperationQueue mainQueue]
                                                    usingBlock:^(NSNotification *note) {
                g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                 clay_dock_window_update_focus_idle,
                                 g_object_ref (dock_window), g_object_unref);
              }];

              /* remove minimize button */
              minimize_mask = [ns_window styleMask];
              minimize_mask &= ~NSWindowStyleMaskMiniaturizable;
              [ns_window setStyleMask:minimize_mask];

              /* disable maximize button (while still preserving resizable attribute) */
              maximize_button = [ns_window standardWindowButton:NSWindowZoomButton];
              if (maximize_button)
                /* do NOT use setHidden:YES since it breaks addChildWindow */
                [maximize_button setEnabled:NO];
            }
#endif
        }
    }
}

static void
clay_dock_window_map (ClayDockWindow *dock_window,
                      gpointer        data)
{
  ClayGuiConfig *config;

  dock_window->p = clay_dock_window_get_instance_private (dock_window);

  if (dock_window->p->context && dock_window->p->context->clay)
    {
      config = CLAY_GUI_CONFIG (Clay (dock_window->p->context->clay)->config);

      if (config->dock_window_hint == CLAY_WINDOW_HINT_UTILITY)
        {
          /* re-establish transiency on later maps (e.g. after using Tab key). See #12448 */
          if (! g_object_get_data (G_OBJECT (dock_window), "clay-dock-window-mapped"))
            {
              g_object_set_data (G_OBJECT (dock_window), "clay-dock-window-mapped",
                                GINT_TO_POINTER (TRUE));
              return;
            }

#if defined (G_OS_WIN32) || defined (PLATFORM_OSX)
          /* re-mapping the dock window (e.g. with the Tab key) lets it steal
             focus, so focus back to the main window */
          g_object_set_data (G_OBJECT (dock_window), "clay-dock-window-refocus-main",
                             GINT_TO_POINTER (TRUE));
#endif
          g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                           clay_dock_window_update_focus_idle,
                           g_object_ref (dock_window), g_object_unref);
        }
    }
}
#endif

/* Returns list of docks from self.
 * Delegate to self's dock_columns, when it exists.
 *
 * Returns NULL when self has no dock_columns,
 * which can happen during destroy.
 *
 * A returned non-null list is owned by self's dock_columns
 * and need not be freed.
 */
static GList *
clay_dock_window_get_docks_self (ClayDockWindow *self)
{
  if (self->p->dock_columns)
    return clay_dock_columns_get_docks (self->p->dock_columns);
  else
    return NULL;
}

/* Implementation of ClayDockContainer interface method get_docks
 * Transfers ownership of list to caller.
 */
static GList *
clay_dock_window_get_docks (ClayDockContainer *dock_container)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock_container);

  return g_list_copy (clay_dock_window_get_docks_self (dock_window));
}

static ClayDialogFactory *
clay_dock_window_get_dialog_factory (ClayDockContainer *dock_container)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock_container);

  return dock_window->p->dialog_factory;
}

static ClayUIManager *
clay_dock_window_get_ui_manager (ClayDockContainer *dock_container)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock_container);

  return dock_window->p->ui_manager;
}

static void
clay_dock_window_add_dock_from_session (ClayDockContainer   *dock_container,
                                        ClayDock            *dock,
                                        ClaySessionInfoDock *dock_info)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock_container);

  clay_dock_window_add_dock (dock_window,
                             dock,
                             -1 /*index*/);
}

static GList *
clay_dock_window_get_aux_info (ClaySessionManaged *session_managed)
{
  ClayDockWindow     *dock_window = CLAY_DOCK_WINDOW (session_managed);
  GList              *aux_info    = NULL;
  ClaySessionInfoAux *aux;

  if (dock_window->p->allow_dockbook_absence)
    {
      /* Assume it is the toolbox; it does not have aux info */
      return NULL;
    }

  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), NULL);

  aux = clay_session_info_aux_new (AUX_INFO_SHOW_IMAGE_MENU,
                                   dock_window->p->show_image_menu ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  aux = clay_session_info_aux_new (AUX_INFO_FOLLOW_ACTIVE_IMAGE,
                                   dock_window->p->auto_follow_active ?
                                   "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  return aux_info;
}

static void
clay_dock_window_set_aux_info (ClaySessionManaged *session_managed,
                               GList              *aux_info)
{
  ClayDockWindow *dock_window;
  GList          *list;
  gboolean        menu_shown;
  gboolean        auto_follow;

  g_return_if_fail (CLAY_IS_DOCK_WINDOW (session_managed));

  dock_window = CLAY_DOCK_WINDOW (session_managed);
  menu_shown  = dock_window->p->show_image_menu;
  auto_follow = dock_window->p->auto_follow_active;

  for (list = aux_info; list; list = g_list_next (list))
    {
      ClaySessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_SHOW_IMAGE_MENU))
        {
          menu_shown = ! g_ascii_strcasecmp (aux->value, "true");
        }
      else if (! strcmp (aux->name, AUX_INFO_FOLLOW_ACTIVE_IMAGE))
        {
          auto_follow = ! g_ascii_strcasecmp (aux->value, "true");
        }
    }

  if (menu_shown != dock_window->p->show_image_menu)
    clay_dock_window_set_show_image_menu (dock_window, menu_shown);

  if (auto_follow != dock_window->p->auto_follow_active)
    clay_dock_window_set_auto_follow_active (dock_window, auto_follow);
}

static ClayAlignmentType
clay_dock_window_get_dock_side (ClayDockContainer *dock_container,
                                ClayDock          *dock)
{
  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_container), -1);
  g_return_val_if_fail (CLAY_IS_DOCK (dock), -1);

  /* A ClayDockWindow don't have docks on different sides, it's just
   * one set of columns
   */
  return -1;
}

/**
 * clay_dock_window_should_add_to_recent:
 * @dock_window:
 *
 * Returns: %FALSE if the dock window can be recreated with one
 *          Windows menu item such as Windows->Toolbox or
 *          Windows->Dockable Dialogs->Layers, %TRUE if not. It should
 *          then be added to the list of recently closed docks.
 **/
static gboolean
clay_dock_window_should_add_to_recent (ClayDockWindow *dock_window)
{
  GList    *docks;
  gboolean  should_add = TRUE;

  docks = clay_dock_window_get_docks_self (dock_window);

  if (! docks)
    {
      should_add = FALSE;
    }
  else if (g_list_length (docks) == 1)
    {
      ClayDock *dock = CLAY_DOCK (g_list_nth_data (docks, 0));

      if (CLAY_IS_TOOLBOX (dock) &&
          clay_dock_get_n_dockables (dock) == 0)
        {
          should_add = FALSE;
        }
      else if (! CLAY_IS_TOOLBOX (dock) &&
               clay_dock_get_n_dockables (dock) == 1)
        {
          should_add = FALSE;
        }
    }

  return should_add;
}

static void
clay_dock_window_image_flush (ClayImage      *image,
                              gboolean        invalidate_preview,
                              ClayDockWindow *dock_window)
{
  if (image == clay_context_get_image (dock_window->p->context))
    {
      ClayDisplay *display = clay_context_get_display (dock_window->p->context);

      if (display)
        clay_ui_manager_update (dock_window->p->ui_manager, display);
    }
}

static void
clay_dock_window_update_title (ClayDockWindow *dock_window)
{
  if (dock_window->p->update_title_idle_id)
    g_source_remove (dock_window->p->update_title_idle_id);

  dock_window->p->update_title_idle_id =
    g_idle_add ((GSourceFunc) clay_dock_window_update_title_idle,
                dock_window);
}

static gboolean
clay_dock_window_update_title_idle (ClayDockWindow *dock_window)
{
  gchar *desc = clay_dock_window_get_description (dock_window,
                                                  FALSE /*complete*/);
  if (desc)
    {
      gtk_window_set_title (GTK_WINDOW (dock_window), desc);
      g_free (desc);
    }

  dock_window->p->update_title_idle_id = 0;

  return FALSE;
}

#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
static gboolean
clay_dock_window_update_focus_idle (gpointer data)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (data);
#ifdef GDK_WINDOWING_WAYLAND
  GtkApplication *app;
#endif
#ifdef _WIN32
  HWND            hwnd_window;
#endif
#ifdef PLATFORM_OSX
  NSWindow       *ns_window;
#endif
#if defined (G_OS_WIN32) || defined (PLATFORM_OSX)
  gboolean        refocus_main;
#endif

  if (! GTK_IS_WIDGET (dock_window) || ! gtk_widget_get_realized (GTK_WIDGET (dock_window)))
    return G_SOURCE_REMOVE;

#if defined (G_OS_WIN32) || defined (PLATFORM_OSX)
  /* set by clay_dock_window_map() when the dock window is being re-mapped
   * (e.g. with the Tab key) which requires to refocus main window */
  refocus_main = (g_object_get_data (G_OBJECT (dock_window),
                                     "clay-dock-window-refocus-main") != NULL);
  g_object_set_data (G_OBJECT (dock_window), "clay-dock-window-refocus-main", NULL);
#endif

#ifdef GDK_WINDOWING_WAYLAND
  app = gtk_window_get_application (GTK_WINDOW (dock_window));
  if (!app)
    {
      GApplication *default_app = g_application_get_default ();
      if (default_app && GTK_IS_APPLICATION (default_app))
        app = GTK_APPLICATION (default_app);
    }
  if (app)
    {
      GtkWindow *app_win = gtk_application_get_active_window (app);
      gtk_window_set_transient_for (GTK_WINDOW (dock_window), app_win);
    }
#endif

#ifdef G_OS_WIN32
  hwnd_window = (HWND) gdk_win32_window_get_handle (gtk_widget_get_window (GTK_WIDGET (dock_window)));
  if (hwnd_window)
    {
      HWND new_parent = (HWND) g_object_get_data (G_OBJECT (dock_window), "win32-parent-window");
      if (new_parent)
        {
          g_object_set_data (G_OBJECT (dock_window), "win32-parent-window", NULL);
        }
      else
        {
          /* GetActiveWindow() is unreliable for docks: showing/re-mapping the dock
             (e.g. with the Tab key) has already made it the active window */
          GtkApplication *app = gtk_window_get_application (GTK_WINDOW (dock_window));
          if (! app)
            {
              GApplication *default_app = g_application_get_default ();
              if (default_app && GTK_IS_APPLICATION (default_app))
                app = GTK_APPLICATION (default_app);
            }
          if (app)
            {
              GList *app_windows = gtk_application_get_windows (app);
              GList *iter;
              for (iter = app_windows; iter; iter = g_list_next (iter))
                {
                  GtkWindow *app_win = GTK_WINDOW (iter->data);
                  if (gtk_widget_get_window (GTK_WIDGET (app_win)) &&
                      !CLAY_IS_DOCK_WINDOW (app_win) &&
                      !GTK_IS_DIALOG (app_win))
                    {
                      new_parent = (HWND) gdk_win32_window_get_handle (gtk_widget_get_window (GTK_WIDGET (app_win)));
                      break;
                    }
                }
            }
          if (! new_parent)
            new_parent = GetActiveWindow ();
        }

      if (hwnd_window && new_parent && hwnd_window != new_parent)
        {
          SetWindowLongPtrW (hwnd_window, GWLP_HWNDPARENT, (LONG_PTR)new_parent);
          SetWindowPos (hwnd_window, HWND_TOP, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

          if (refocus_main)
            SetForegroundWindow (new_parent);
        }
    }
#endif

#ifdef PLATFORM_OSX
  ns_window = gdk_quartz_window_get_nswindow (gtk_widget_get_window (GTK_WIDGET (dock_window)));
  if (ns_window)
    {
      NSWindow *new_parent = (NSWindow *) g_object_get_data (G_OBJECT (dock_window), "osx-parent-window");
      if (new_parent)
        g_object_set_data (G_OBJECT (dock_window), "osx-parent-window", NULL);
      else
        new_parent = [NSApp mainWindow];

      if (new_parent && new_parent != ns_window && [new_parent level] == NSNormalWindowLevel)
        {
          NSWindow *old_parent = [ns_window parentWindow];
          if (old_parent)
            [old_parent removeChildWindow:ns_window];

          [new_parent addChildWindow:ns_window ordered:NSWindowAbove];

          if (refocus_main)
            [new_parent makeKeyWindow];
        }
    }
#endif

  return G_SOURCE_REMOVE;
}
#endif

static gchar *
clay_dock_window_get_description (ClayDockWindow *dock_window,
                                  gboolean        complete)
{
  GString *complete_desc = g_string_new (NULL);
  GList   *docks         = NULL;
  GList   *iter          = NULL;

  docks = clay_dock_window_get_docks_self (dock_window);

  for (iter = docks;
       iter;
       iter = g_list_next (iter))
    {
      gchar *desc = clay_dock_get_description (CLAY_DOCK (iter->data), complete);
      g_string_append (complete_desc, desc);
      g_free (desc);

      if (g_list_next (iter))
        g_string_append (complete_desc, CLAY_DOCK_COLUMN_SEPARATOR);
    }

  return g_string_free (complete_desc, FALSE /*free_segment*/);
}

static void
clay_dock_window_dock_removed (ClayDockWindow  *dock_window,
                               ClayDock        *dock,
                               ClayDockColumns *dock_columns)
{
  g_return_if_fail (CLAY_IS_DOCK (dock));

  if (clay_dock_columns_get_docks (dock_columns) == NULL &&
      ! dock_window->p->allow_dockbook_absence)
    gtk_widget_destroy (GTK_WIDGET (dock_window));
}

static void
clay_dock_window_factory_display_changed (ClayContext *context,
                                          ClayDisplay *display,
                                          ClayDock    *dock)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock);

  if (display && dock_window->p->auto_follow_active)
    clay_context_set_display (dock_window->p->context, display);

#if defined (GDK_WINDOWING_WAYLAND) || defined (G_OS_WIN32) || defined (PLATFORM_OSX)
  if (gtk_widget_get_realized (GTK_WIDGET (dock_window)))
    {
      if (dock_window->p->context && dock_window->p->context->clay)
        {
          Clay          *clay   = Clay (dock_window->p->context->clay);
          ClayGuiConfig *config = CLAY_GUI_CONFIG (clay->config);

          if (config->dock_window_hint == CLAY_WINDOW_HINT_UTILITY)
            {
#ifdef G_OS_WIN32
              /* this needs to be outside clay_dock_window_update_focus_idle to prevent
                 `Gdk-WARNING **: window unexpectedly destroyed` on clay closure */
              HWND dock_hwnd = (HWND) gdk_win32_window_get_handle (gtk_widget_get_window (GTK_WIDGET (dock_window)));
              if (dock_hwnd && gtk_widget_get_mapped (GTK_WIDGET (dock_window)))
                SetWindowLongPtrW (dock_hwnd, GWLP_HWNDPARENT, (LONG_PTR)NULL);
#endif
              if (display)
                g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                 clay_dock_window_update_focus_idle,
                                 g_object_ref (dock_window), g_object_unref);
            }
        }
    }
#endif
}

static void
clay_dock_window_factory_image_changed (ClayContext *context,
                                        ClayImage   *image,
                                        ClayDock    *dock)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock);

  /*  won't do anything if we already set the display above  */
  if (image && dock_window->p->auto_follow_active)
    clay_context_set_image (dock_window->p->context, image);
}

static void
clay_dock_window_display_changed (ClayDockWindow *dock_window,
                                  ClayDisplay    *display,
                                  ClayContext    *context)
{
  /*  make sure auto-follow-active works both ways  */
  if (display && dock_window->p->auto_follow_active)
    {
      ClayContext *factory_context =
        clay_dialog_factory_get_context (dock_window->p->dialog_factory);

      clay_context_set_display (factory_context, display);
    }

  clay_ui_manager_update (dock_window->p->ui_manager,
                          display);
}

static void
clay_dock_window_image_changed (ClayDockWindow *dock_window,
                                ClayImage      *image,
                                ClayContext    *context)
{
  ClayContainer *display_container = dock_window->p->display_container;

  /*  make sure auto-follow-active works both ways  */
  if (image && dock_window->p->auto_follow_active)
    {
      ClayContext *factory_context =
        clay_dialog_factory_get_context (dock_window->p->dialog_factory);

      clay_context_set_image (factory_context, image);
    }

  if (image != NULL && ! clay_container_is_empty (display_container))
    {
      ClayDisplay *display;
      ClayImage   *display_image;
      gboolean     find_display = TRUE;

      display = clay_context_get_display (context);

      if (display)
        {
          g_object_get (display, "image", &display_image, NULL);

          if (display_image)
            {
              g_object_unref (display_image);

              if (display_image == image)
                find_display = FALSE;
            }
        }

      if (find_display)
        {
          GList *list;

          for (list = CLAY_LIST (display_container)->queue->head;
               list;
               list = g_list_next (list))
            {
              display = list->data;

              g_object_get (display, "image", &display_image, NULL);

              if (display_image)
                {
                  g_object_unref (display_image);

                  if (display_image == image)
                    {
                      /*  this invokes this function recursively but we
                       *  don't enter the if(find_display) branch the
                       *  second time
                       */
                      clay_context_set_display (context, display);

                      /*  don't stop signal emission here because the
                       *  context's image was not changed by the
                       *  recursive call
                       */
                      break;
                    }
                }
            }
        }
    }

  clay_ui_manager_update (dock_window->p->ui_manager,
                          clay_context_get_display (context));
}

static void
clay_dock_window_auto_clicked (GtkWidget *widget,
                               ClayDock  *dock)
{
  ClayDockWindow *dock_window = CLAY_DOCK_WINDOW (dock);

  clay_toggle_button_update (widget, &dock_window->p->auto_follow_active);

  if (dock_window->p->auto_follow_active)
    {
      ClayContext *context;

      context = clay_dialog_factory_get_context (dock_window->p->dialog_factory);

      clay_context_copy_properties (context,
                                    dock_window->p->context,
                                    CLAY_CONTEXT_PROP_MASK_DISPLAY |
                                    CLAY_CONTEXT_PROP_MASK_IMAGE);
    }
}


void
clay_dock_window_add_dock (ClayDockWindow *dock_window,
                           ClayDock       *dock,
                           gint            index)
{
  g_return_if_fail (CLAY_IS_DOCK_WINDOW (dock_window));
  g_return_if_fail (CLAY_IS_DOCK (dock));

  clay_dock_columns_add_dock (dock_window->p->dock_columns,
                              CLAY_DOCK (dock),
                              index);

  g_signal_connect_object (dock, "description-invalidated",
                           G_CALLBACK (clay_dock_window_update_title),
                           dock_window,
                           G_CONNECT_SWAPPED);

  /* Some docks like the toolbox dock needs to maintain special hints
   * on its container GtkWindow, allow those to do so
   */
  clay_dock_set_host_geometry_hints (dock, GTK_WINDOW (dock_window));
  g_signal_connect_object (dock, "geometry-invalidated",
                           G_CALLBACK (clay_dock_set_host_geometry_hints),
                           dock_window, 0);
}

void
clay_dock_window_remove_dock (ClayDockWindow *dock_window,
                              ClayDock       *dock)
{
  clay_dock_columns_remove_dock (dock_window->p->dock_columns,
                                 CLAY_DOCK (dock));

  g_signal_handlers_disconnect_by_func (dock,
                                        clay_dock_window_update_title,
                                        dock_window);
  g_signal_handlers_disconnect_by_func (dock,
                                        clay_dock_set_host_geometry_hints,
                                        dock_window);
}

GtkWidget *
clay_dock_window_new (const gchar       *role,
                      const gchar       *ui_manager_name,
                      gboolean           allow_dockbook_absence,
                      ClayDialogFactory *dialog_factory,
                      ClayContext       *context)
{
  g_return_val_if_fail (CLAY_IS_DIALOG_FACTORY (dialog_factory), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  return g_object_new (CLAY_TYPE_DOCK_WINDOW,
                       "role",                   role,
                       "ui-manager-name",        ui_manager_name,
                       "allow-dockbook-absence", allow_dockbook_absence,
                       "dialog-factory",         dialog_factory,
                       "context",                context,
                       NULL);
}

gint
clay_dock_window_get_id (ClayDockWindow *dock_window)
{
  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), 0);

  return dock_window->p->ID;
}

ClayContext *
clay_dock_window_get_context (ClayDockWindow *dock_window)
{
  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), NULL);

  return dock_window->p->context;
}

gboolean
clay_dock_window_get_auto_follow_active (ClayDockWindow *dock_window)
{
  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->auto_follow_active;
}

void
clay_dock_window_set_auto_follow_active (ClayDockWindow *dock_window,
                                         gboolean        auto_follow_active)
{
  g_return_if_fail (CLAY_IS_DOCK_WINDOW (dock_window));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock_window->p->auto_button),
                                auto_follow_active ? TRUE : FALSE);
}

gboolean
clay_dock_window_get_show_image_menu (ClayDockWindow *dock_window)
{
  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), FALSE);

  return dock_window->p->show_image_menu;
}

void
clay_dock_window_set_show_image_menu (ClayDockWindow *dock_window,
                                      gboolean        show)
{
  GtkWidget *parent;

  g_return_if_fail (CLAY_IS_DOCK_WINDOW (dock_window));

  parent = gtk_widget_get_parent (dock_window->p->image_combo);

  gtk_widget_set_visible (parent, show);

  dock_window->p->show_image_menu = show ? TRUE : FALSE;
}

void
clay_dock_window_setup (ClayDockWindow *dock_window,
                        ClayDockWindow *template)
{
  clay_dock_window_set_auto_follow_active (CLAY_DOCK_WINDOW (dock_window),
                                           template->p->auto_follow_active);
  clay_dock_window_set_show_image_menu (CLAY_DOCK_WINDOW (dock_window),
                                        template->p->show_image_menu);
}

/**
 * clay_dock_window_has_toolbox:
 * @dock_window:
 *
 * Returns: %TRUE if the dock window has a ClayToolbox dock, %FALSE
 * otherwise.
 **/
gboolean
clay_dock_window_has_toolbox (ClayDockWindow *dock_window)
{
  GList *iter = NULL;

  g_return_val_if_fail (CLAY_IS_DOCK_WINDOW (dock_window), FALSE);

  for (iter = clay_dock_window_get_docks_self (dock_window);
       iter;
       iter = g_list_next (iter))
    {
      if (CLAY_IS_TOOLBOX (iter->data))
        return TRUE;
    }

  return FALSE;
}


/**
 * clay_dock_window_from_dock:
 * @dock:
 *
 * For convenience.
 *
 * Returns: If the toplevel widget for the dock is a ClayDockWindow,
 * return that. Otherwise return %NULL.
 **/
ClayDockWindow *
clay_dock_window_from_dock (ClayDock *dock)
{
  GtkWidget *toplevel = NULL;

  g_return_val_if_fail (CLAY_IS_DOCK (dock), NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dock));

  if (CLAY_IS_DOCK_WINDOW (toplevel))
    return CLAY_DOCK_WINDOW (toplevel);
  else
    return NULL;
}
