/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/clayguiconfig.h"

#include "widgets/claydockcontainer.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "claydialogfactory.h"
#include "claydock.h"
#include "claydockwindow.h"
#include "claysessioninfo.h"
#include "claysessioninfo-aux.h"
#include "claysessioninfo-book.h"
#include "claysessioninfo-dock.h"
#include "claysessioninfo-private.h"
#include "claysessionmanaged.h"

#include "clay-log.h"


enum
{
  SESSION_INFO_FACTORY_ENTRY,
  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_MONITOR,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,
  SESSION_INFO_DOCK,
  SESSION_INFO_CLAY_DOCK,
  SESSION_INFO_CLAY_TOOLBOX
};

#define DEFAULT_MONITOR NULL


typedef struct
{
  ClaySessionInfo   *info;
  ClayDialogFactory *factory;
  GdkMonitor        *monitor;
  GtkWidget         *dialog;
} ClayRestoreDocksData;


static void      clay_session_info_config_iface_init  (ClayConfigInterface  *iface);
static void      clay_session_info_finalize           (GObject              *object);
static gint64    clay_session_info_get_memsize        (ClayObject           *object,
                                                       gint64               *gui_size);
static gboolean  clay_session_info_serialize          (ClayConfig           *config,
                                                       ClayConfigWriter     *writer,
                                                       gpointer              data);
static gboolean  clay_session_info_deserialize        (ClayConfig           *config,
                                                       GScanner             *scanner,
                                                       gint                  nest_level,
                                                       gpointer              data);
static gboolean  clay_session_info_is_for_dock_window (ClaySessionInfo      *info);
static void      clay_session_info_dialog_show        (GtkWidget            *widget,
                                                       ClaySessionInfo      *info);
static gboolean  clay_session_info_restore_docks      (ClayRestoreDocksData *data);

static void      clay_session_info_monitor_invalidate (GdkMonitor           *monitor,
                                                       ClaySessionInfo      *info);


G_DEFINE_TYPE_WITH_CODE (ClaySessionInfo, clay_session_info, CLAY_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClaySessionInfo)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_session_info_config_iface_init))

#define parent_class clay_session_info_parent_class


static void
clay_session_info_class_init (ClaySessionInfoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_session_info_finalize;

  clay_object_class->get_memsize = clay_session_info_get_memsize;
}

static void
clay_session_info_init (ClaySessionInfo *info)
{
  info->p = clay_session_info_get_instance_private (info);

  info->p->monitor = DEFAULT_MONITOR;
}

static void
clay_session_info_config_iface_init (ClayConfigInterface *iface)
{
  iface->serialize   = clay_session_info_serialize;
  iface->deserialize = clay_session_info_deserialize;
}

static void
clay_session_info_finalize (GObject *object)
{
  ClaySessionInfo *info = CLAY_SESSION_INFO (object);

  clay_session_info_clear_info (info);

  clay_session_info_set_widget (info, NULL);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_session_info_get_memsize (ClayObject *object,
                               gint64     *gui_size)
{
#if 0
  ClaySessionInfo *info    = CLAY_SESSION_INFO (object);
#endif
  gint64           memsize = 0;

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gint
monitor_number (GdkMonitor *monitor)
{
  GdkDisplay *display    = gdk_monitor_get_display (monitor);
  gint        n_monitors = gdk_display_get_n_monitors (display);
  gint        i;

  for (i = 0; i < n_monitors; i++)
    if (gdk_display_get_monitor (display, i) == monitor)
      return i;

  return 0;
}

static gboolean
clay_session_info_serialize (ClayConfig       *config,
                             ClayConfigWriter *writer,
                             gpointer          data)
{
  ClaySessionInfo *info = CLAY_SESSION_INFO (config);
  GList           *iter = NULL;
  gint             x;
  gint             y;
  gint             width;
  gint             height;

  if (info->p->factory_entry && info->p->factory_entry->identifier)
    {
      clay_config_writer_open (writer, "factory-entry");
      clay_config_writer_string (writer, info->p->factory_entry->identifier);
      clay_config_writer_close (writer);
    }

  x      = clay_session_info_apply_position_accuracy (info->p->x);
  y      = clay_session_info_apply_position_accuracy (info->p->y);
  width  = clay_session_info_apply_position_accuracy (info->p->width);
  height = clay_session_info_apply_position_accuracy (info->p->height);

  clay_config_writer_open (writer, "position");
  clay_config_writer_printf (writer, "%d %d", x, y);
  clay_config_writer_close (writer);

  if (info->p->width > 0 && info->p->height > 0)
    {
      clay_config_writer_open (writer, "size");
      clay_config_writer_printf (writer, "%d %d", width, height);
      clay_config_writer_close (writer);
    }

  if (info->p->monitor != DEFAULT_MONITOR)
    {
      clay_config_writer_open (writer, "monitor");
      clay_config_writer_printf (writer, "%d", monitor_number (info->p->monitor));
      clay_config_writer_close (writer);
    }

  if (info->p->open)
    {
      clay_config_writer_open (writer, "open-on-exit");
      clay_config_writer_close (writer);
    }

  if (info->p->aux_info)
    clay_session_info_aux_serialize (writer, info->p->aux_info);

  for (iter = info->p->docks; iter; iter = g_list_next (iter))
    clay_session_info_dock_serialize (writer, iter->data);

  return TRUE;
}

/*
 * This function is just like clay_scanner_parse_int(), but it is allows
 * to detect the special value '-0'. This is used as in X geometry strings.
 */
static gboolean
clay_session_info_parse_offset (GScanner *scanner,
                                gint     *dest,
                                gboolean *negative)
{
  if (g_scanner_peek_next_token (scanner) == '-')
    {
      *negative = TRUE;
      g_scanner_get_next_token (scanner);
    }
  else
    {
      *negative = FALSE;
    }

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*negative)
    *dest = -scanner->value.v_int64;
  else
    *dest = scanner->value.v_int64;

  return TRUE;
}

static gboolean
clay_session_info_deserialize (ClayConfig *config,
                               GScanner   *scanner,
                               gint        nest_level,
                               gpointer    data)
{
  ClaySessionInfo *info = CLAY_SESSION_INFO (config);
  GTokenType       token;
  guint            scope_id;
  guint            old_scope_id;

  scope_id = g_type_qname (G_TYPE_FROM_INSTANCE (config));
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id, "factory-entry",
                              GINT_TO_POINTER (SESSION_INFO_FACTORY_ENTRY));
  g_scanner_scope_add_symbol (scanner, scope_id, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, scope_id, "size",
                              GINT_TO_POINTER (SESSION_INFO_SIZE));
  g_scanner_scope_add_symbol (scanner, scope_id, "monitor",
                              GINT_TO_POINTER (SESSION_INFO_MONITOR));
  g_scanner_scope_add_symbol (scanner, scope_id, "open-on-exit",
                              GINT_TO_POINTER (SESSION_INFO_OPEN));
  g_scanner_scope_add_symbol (scanner, scope_id, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_AUX));
  g_scanner_scope_add_symbol (scanner, scope_id, "clay-dock",
                              GINT_TO_POINTER (SESSION_INFO_CLAY_DOCK));
  g_scanner_scope_add_symbol (scanner, scope_id, "clay-toolbox",
                              GINT_TO_POINTER (SESSION_INFO_CLAY_TOOLBOX));

  /* For sessionrc files from version <= Clay 2.6 */
  g_scanner_scope_add_symbol (scanner, scope_id, "dock",
                              GINT_TO_POINTER (SESSION_INFO_DOCK));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
          gint dummy;

        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case SESSION_INFO_FACTORY_ENTRY:
              {
                gchar                  *identifier = NULL;
                ClayDialogFactoryEntry *entry      = NULL;

                token = G_TOKEN_STRING;
                if (! clay_scanner_parse_string (scanner, &identifier))
                  goto error;

                entry = clay_dialog_factory_find_entry (clay_dialog_factory_get_singleton (),
                                                        identifier);
                if (! entry)
                  goto error;

                clay_session_info_set_factory_entry (info, entry);

                g_free (identifier);
              }
              break;

            case SESSION_INFO_POSITION:
              token = G_TOKEN_INT;
              if (! clay_session_info_parse_offset (scanner,
                                                    &info->p->x,
                                                    &info->p->right_align))
                goto error;
              if (! clay_session_info_parse_offset (scanner,
                                                    &info->p->y,
                                                    &info->p->bottom_align))
                goto error;
              break;

            case SESSION_INFO_SIZE:
              token = G_TOKEN_INT;
              if (! clay_scanner_parse_int (scanner, &info->p->width))
                goto error;
              if (! clay_scanner_parse_int (scanner, &info->p->height))
                goto error;
              break;

            case SESSION_INFO_MONITOR:
              token = G_TOKEN_INT;
              if (clay_scanner_parse_int (scanner, &dummy))
                {
                  if (info->p->monitor != NULL)
                    g_signal_handlers_disconnect_by_func (info->p->monitor,
                                                          G_CALLBACK (clay_session_info_monitor_invalidate),
                                                          info);

                  info->p->monitor =
                    gdk_display_get_monitor (gdk_display_get_default (), dummy);

                  if (info->p->monitor != NULL)
                    g_signal_connect_object (info->p->monitor, "invalidate",
                                             G_CALLBACK (clay_session_info_monitor_invalidate),
                                             info, 0);
                }
              else
                goto error;
              break;

            case SESSION_INFO_OPEN:
              info->p->open = TRUE;

              /*  the screen number is optional, and obsolete  */
              if (g_scanner_peek_next_token (scanner) == G_TOKEN_RIGHT_PAREN)
                break;

              token = G_TOKEN_INT;
              if (! clay_scanner_parse_int (scanner, &dummy))
                goto error;
              break;

            case SESSION_INFO_AUX:
              token = clay_session_info_aux_deserialize (scanner,
                                                         &info->p->aux_info);
              if (token != G_TOKEN_LEFT_PAREN)
                goto error;
              break;

            case SESSION_INFO_CLAY_TOOLBOX:
            case SESSION_INFO_CLAY_DOCK:
            case SESSION_INFO_DOCK:
              {
                ClaySessionInfoDock *dock_info  = NULL;
                const gchar         *dock_type = NULL;

                /* Handle old sessionrc:s from versions <= Clay 2.6 */
                if (GPOINTER_TO_INT (scanner->value.v_symbol) == SESSION_INFO_DOCK &&
                    info->p->factory_entry &&
                    info->p->factory_entry->identifier &&
                    strcmp ("clay-toolbox-window",
                            info->p->factory_entry->identifier) == 0)
                  {
                    dock_type = "clay-toolbox";
                  }
                else
                  {
                    dock_type = ((GPOINTER_TO_INT (scanner->value.v_symbol) ==
                                  SESSION_INFO_CLAY_TOOLBOX) ?
                                 "clay-toolbox" :
                                 "clay-dock");
                  }

                g_scanner_set_scope (scanner, scope_id + 1);
                token = clay_session_info_dock_deserialize (scanner, scope_id + 1,
                                                            &dock_info,
                                                            dock_type);

                if (token == G_TOKEN_LEFT_PAREN)
                  {
                    g_scanner_set_scope (scanner, scope_id);
                    info->p->docks = g_list_append (info->p->docks, dock_info);
                  }
                else
                  goto error;
              }
              break;

            default:
              break;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

 error:

  /* If we don't have docks, assume it is a toolbox dock window from a
   * sessionrc file from Clay <= 2.6 and add a toolbox dock manually
   */
  if (! info->p->docks &&
      info->p->factory_entry &&
      strcmp ("clay-toolbox-window",
              info->p->factory_entry->identifier) == 0)
    {
      info->p->docks =
        g_list_append (info->p->docks,
                       clay_session_info_dock_new ("clay-toolbox"));
    }

  g_scanner_scope_remove_symbol (scanner, scope_id, "factory-entry");
  g_scanner_scope_remove_symbol (scanner, scope_id, "position");
  g_scanner_scope_remove_symbol (scanner, scope_id, "size");
  g_scanner_scope_remove_symbol (scanner, scope_id, "open-on-exit");
  g_scanner_scope_remove_symbol (scanner, scope_id, "aux-info");
  g_scanner_scope_remove_symbol (scanner, scope_id, "clay-dock");
  g_scanner_scope_remove_symbol (scanner, scope_id, "clay-toolbox");
  g_scanner_scope_remove_symbol (scanner, scope_id, "dock");

  g_scanner_set_scope (scanner, old_scope_id);

  return clay_config_deserialize_return (scanner, token, nest_level);
}

/**
 * clay_session_info_is_for_dock_window:
 * @info:
 *
 * Helper function to determine if the session info is for a dock. It
 * uses the dialog factory entry state and the associated widget state
 * if any to determine that.
 *
 * Returns: %TRUE if session info is for a dock, %FALSE otherwise.
 **/
static gboolean
clay_session_info_is_for_dock_window (ClaySessionInfo *info)
{
  gboolean entry_state_for_dock  =  info->p->factory_entry == NULL;
  gboolean widget_state_for_dock = (info->p->widget == NULL ||
                                    CLAY_IS_DOCK_WINDOW (info->p->widget));

  return entry_state_for_dock && widget_state_for_dock;
}

static void
clay_session_info_dialog_show (GtkWidget       *widget,
                               ClaySessionInfo *info)
{
  gtk_window_move (GTK_WINDOW (widget),
                   info->p->x, info->p->y);

  if (clay_session_info_get_remember_size (info) &&
      info->p->width  > 0 &&
      info->p->height > 0)
    {
      gtk_window_resize (GTK_WINDOW (info->p->widget),
                         info->p->width, info->p->height);
    }
}

static gboolean
clay_session_info_restore_docks (ClayRestoreDocksData *data)
{
  ClaySessionInfo     *info    = data->info;
  ClayDialogFactory   *factory = data->factory;
  GdkMonitor          *monitor = data->monitor;
  GtkWidget           *dialog  = data->dialog;
  GList               *iter;

  if (CLAY_IS_DOCK_CONTAINER (dialog))
    {
      /* We expect expect there to always be docks. In sessionrc files
       * from <= 2.6 not all dock window entries had dock entries, but we
       * take care of that during sessionrc parsing
       */
      for (iter = info->p->docks; iter; iter = g_list_next (iter))
        {
          ClaySessionInfoDock *dock_info = (ClaySessionInfoDock *) iter->data;
          GtkWidget           *dock;

          dock =
            GTK_WIDGET (clay_session_info_dock_restore (dock_info,
                                                        factory,
                                                        monitor,
                                                        CLAY_DOCK_CONTAINER (dialog)));

          if (dock && dock_info->position != 0)
            {
              GtkWidget *parent = gtk_widget_get_parent (dock);

              if (GTK_IS_PANED (parent))
                {
                  GtkPaned *paned = GTK_PANED (parent);

                  if (dock == gtk_paned_get_child2 (paned))
                    gtk_paned_set_position (paned, dock_info->position);
                }
            }
        }
    }

  clay_session_info_clear_info (info);

  g_object_unref (dialog);
  g_object_unref (monitor);
  g_object_unref (factory);
  g_object_unref (info);

  g_slice_free (ClayRestoreDocksData, data);

  return FALSE;
}

static void
clay_session_info_monitor_invalidate (GdkMonitor      *monitor,
                                      ClaySessionInfo *info)
{
  /* When a monitor is disconnected, even if it is connected again, it
   * will now be a different object. We need to get rid of the dangling
   * pointer.
   *
   * TODO: maybe storing also the manufacturer/model (and in GTK4, the
   * description) may be a good way to retrieve a monitor upon
   * reconnection so that a window stay associated to the same monitor,
   * even when disconnecting/reconnecting (e.g. a laptop plugged to a
   * desktop display).
   */
  g_signal_handlers_disconnect_by_func (monitor,
                                        G_CALLBACK (clay_session_info_monitor_invalidate),
                                        info);
  info->p->monitor = DEFAULT_MONITOR;
}


/*  public functions  */

ClaySessionInfo *
clay_session_info_new (void)
{
  return g_object_new (CLAY_TYPE_SESSION_INFO, NULL);
}

void
clay_session_info_restore (ClaySessionInfo   *info,
                           ClayDialogFactory *factory,
                           GdkMonitor        *monitor)
{
  GtkWidget            *dialog = NULL;
  ClayRestoreDocksData *data;

  g_return_if_fail (CLAY_IS_SESSION_INFO (info));
  g_return_if_fail (CLAY_IS_DIALOG_FACTORY (factory));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  g_object_ref (info);

  info->p->open = FALSE;

  if (info->p->factory_entry &&
      info->p->factory_entry->restore_func)
    {
      dialog = info->p->factory_entry->restore_func (factory,
                                                     monitor,
                                                     info);
    }
  else
    g_printerr ("EEEEK\n");

  if (CLAY_IS_SESSION_MANAGED (dialog) && info->p->aux_info)
    clay_session_managed_set_aux_info (CLAY_SESSION_MANAGED (dialog),
                                       info->p->aux_info);

  /* In single-window mode, clay_session_managed_set_aux_info()
   * will set the size of the dock areas at the sides. If we don't
   * wait for those areas to get their size-allocation, we can't
   * properly restore the docks inside them, so do that in an idle
   * callback.
   */

  /* Objects are unreffed again in the callback */
  data = g_slice_new0 (ClayRestoreDocksData);
  data->info    = g_object_ref (info);
  data->factory = g_object_ref (factory);
  data->monitor = g_object_ref (monitor);
  data->dialog  = dialog ? g_object_ref (dialog) : NULL;

  g_idle_add ((GSourceFunc) clay_session_info_restore_docks, data);

  g_object_unref (info);
}

/**
 * clay_session_info_apply_geometry:
 * @info:
 * @monitor:
 * @current_monitor:
 *
 * Apply the geometry stored in the session info object to the
 * associated widget.
 **/
void
clay_session_info_apply_geometry (ClaySessionInfo *info,
                                  GdkMonitor      *current_monitor,
                                  gboolean         apply_stored_monitor)
{
  GdkMonitor     *monitor;
  GdkRectangle    rect;
  GdkRectangle    work_rect;
  GdkGravity      gravity;
  GdkWindowHints  hints;
  gint            width;
  gint            height;

  g_return_if_fail (CLAY_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->p->widget));
  g_return_if_fail (GDK_IS_MONITOR (current_monitor));

  monitor = current_monitor;

  if (apply_stored_monitor)
    {
      GdkDisplay *display = gdk_monitor_get_display (current_monitor);
      gint        n_monitors;

      n_monitors = gdk_display_get_n_monitors (display);

      if (info->p->monitor                  != DEFAULT_MONITOR &&
          monitor_number (info->p->monitor) <  n_monitors)
        {
          monitor = info->p->monitor;
        }
      else
        {
          monitor = gdk_display_get_primary_monitor (display);
        }
    }

  gdk_monitor_get_geometry (monitor, &rect);
  gdk_monitor_get_workarea (monitor, &work_rect);

  info->p->x += rect.x;
  info->p->y += rect.y;

  if (clay_session_info_get_remember_size (info) &&
      info->p->width  > 0 &&
      info->p->height > 0)
    {
      width  = info->p->width;
      height = info->p->height;
    }
  else
    {
      GtkRequisition requisition;

      gtk_widget_get_preferred_size (info->p->widget, NULL, &requisition);

      width  = requisition.width;
      height = requisition.height;
    }

  info->p->x = CLAMP (info->p->x,
                      work_rect.x,
                      work_rect.x + work_rect.width  - width);
  info->p->y = CLAMP (info->p->y,
                      work_rect.y,
                      work_rect.y + work_rect.height - height);

  if (clay_session_info_get_remember_size (info) &&
      info->p->width  > 0 &&
      info->p->height > 0)
    {
      /*  This used to call gtk_window_set_default_size() which worked
       *  fine in gtk2 and should continue to work, but doesn't for
       *  dock windows. gtk_window_resize() seems to work fine for all
       *  windows. Leave this comment here until we figured what's
       *  going on...
       *
       *  XXX If we end up updating this code, also do the same to the
       *  gtk_window_resize() call in clay_session_info_dialog_show()
       *  signal handler.
       */
#if 1
      gtk_window_resize (GTK_WINDOW (info->p->widget),
                         info->p->width, info->p->height);
#else
      gtk_window_set_default_size (GTK_WINDOW (info->p->widget),
                                   info->p->width, info->p->height);
#endif
   }

  gtk_window_get_size (GTK_WINDOW (info->p->widget), &width, &height);

  gravity = GDK_GRAVITY_NORTH_WEST;

  if (info->p->right_align && info->p->bottom_align)
    {
      gravity = GDK_GRAVITY_SOUTH_EAST;
    }
  else if (info->p->right_align)
    {
      gravity = GDK_GRAVITY_NORTH_EAST;
    }
  else if (info->p->bottom_align)
    {
      gravity = GDK_GRAVITY_SOUTH_WEST;
    }

  if (gravity == GDK_GRAVITY_SOUTH_EAST ||
      gravity == GDK_GRAVITY_NORTH_EAST)
    info->p->x = work_rect.x + work_rect.width - width;

  if (gravity == GDK_GRAVITY_SOUTH_WEST ||
      gravity == GDK_GRAVITY_SOUTH_EAST)
    info->p->y = work_rect.y + work_rect.height - height;

  gtk_window_set_gravity (GTK_WINDOW (info->p->widget), gravity);
  gtk_window_move (GTK_WINDOW (info->p->widget),
                   info->p->x, info->p->y);

  hints = GDK_HINT_USER_POS;
  if (clay_session_info_get_remember_size (info))
    hints |= GDK_HINT_USER_SIZE;

  gtk_window_set_geometry_hints (GTK_WINDOW (info->p->widget),
                                 NULL, NULL, hints);

  /*  Window managers and windowing systems suck. They have their own
   *  ideas about WM standards and when it's appropriate to honor
   *  user/application-set window positions and when not. Therefore,
   *  use brute force and "manually" position dialogs whenever they
   *  are shown. This is important especially for transient dialogs,
   *  because window managers behave even "smarter" then...
   */
  if (GTK_IS_WINDOW (info->p->widget))
    g_signal_connect (info->p->widget, "show",
                      G_CALLBACK (clay_session_info_dialog_show),
                      info);
}

/**
 * clay_session_info_read_geometry:
 * @info:  A #ClaySessionInfo
 * @cevent A #GdkEventConfigure. If set, use the size from here
 *         instead of from the window allocation.
 *
 * Read geometry related information from the associated widget.
 **/
void
clay_session_info_read_geometry (ClaySessionInfo   *info,
                                 GdkEventConfigure *cevent)
{
  GdkWindow  *window;
  GdkDisplay *display;

  g_return_if_fail (CLAY_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WINDOW (info->p->widget));

  window  = gtk_widget_get_window (info->p->widget);
  display = gtk_widget_get_display (info->p->widget);

  if (window)
    {
      gint          x, y;
      GdkMonitor   *monitor;
      GdkRectangle  geometry;

      gtk_window_get_position (GTK_WINDOW (info->p->widget), &x, &y);

      /* Don't write negative values to the sessionrc, they are
       * interpreted as relative to the right, respective bottom edge
       * of the display.
       */
      info->p->x = MAX (0, x);
      info->p->y = MAX (0, y);

      monitor = gdk_display_get_monitor_at_point (display,
                                                  info->p->x, info->p->y);
      gdk_monitor_get_geometry (monitor, &geometry);

      /* Always store window coordinates relative to the monitor */
      info->p->x -= geometry.x;
      info->p->y -= geometry.y;

      if (clay_session_info_get_remember_size (info))
        {
          gtk_window_get_size (GTK_WINDOW (info->p->widget),
                               &info->p->width, &info->p->height);
        }
      else
        {
          info->p->width  = 0;
          info->p->height = 0;
        }

      if (info->p->monitor != NULL)
        g_signal_handlers_disconnect_by_func (info->p->monitor,
                                              G_CALLBACK (clay_session_info_monitor_invalidate),
                                              info);

      info->p->monitor = DEFAULT_MONITOR;

      if (monitor != gdk_display_get_primary_monitor (display))
        info->p->monitor = monitor;

      if (info->p->monitor != NULL)
        g_signal_connect_object (info->p->monitor, "invalidate",
                                 G_CALLBACK (clay_session_info_monitor_invalidate),
                                 info, 0);
    }

  info->p->open = FALSE;

  if (clay_session_info_get_remember_if_open (info))
    {
      ClayDialogVisibilityState visibility;

      visibility =
        GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info->p->widget),
                                            CLAY_DIALOG_VISIBILITY_KEY));

      switch (visibility)
        {
        case CLAY_DIALOG_VISIBILITY_UNKNOWN:
          info->p->open = gtk_widget_get_visible (info->p->widget);
          break;

        case CLAY_DIALOG_VISIBILITY_INVISIBLE:
          info->p->open = FALSE;
          break;

        case CLAY_DIALOG_VISIBILITY_HIDDEN:
        case CLAY_DIALOG_VISIBILITY_VISIBLE:
          /* Even if a dialog is hidden (with Windows->Hide docks) it
           * is still considered open. It will be restored the next
           * time Clay starts
           */
          info->p->open = TRUE;
          break;
        }
    }
}

void
clay_session_info_get_info (ClaySessionInfo *info)
{
  g_return_if_fail (CLAY_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WIDGET (info->p->widget));

  clay_session_info_read_geometry (info, NULL /*cevent*/);

  if (CLAY_IS_SESSION_MANAGED (info->p->widget))
    info->p->aux_info =
      clay_session_managed_get_aux_info (CLAY_SESSION_MANAGED (info->p->widget));

  if (CLAY_IS_DOCK_CONTAINER (info->p->widget))
    {
      ClayDockContainer *dock_container = CLAY_DOCK_CONTAINER (info->p->widget);
      GList             *iter           = NULL;
      GList             *docks;

      docks = clay_dock_container_get_docks (dock_container);

      for (iter = docks;
           iter;
           iter = g_list_next (iter))
        {
          ClayDock *dock = CLAY_DOCK (iter->data);

          info->p->docks =
            g_list_append (info->p->docks,
                           clay_session_info_dock_from_widget (dock));
        }

      g_list_free (docks);
    }
}

/**
 * clay_session_info_get_info_with_widget:
 * @info:
 * @widget: #GtkWidget to use
 *
 * Temporarily sets @widget on @info and calls
 * clay_session_info_get_info(), then restores the old widget that was
 * set.
 **/
void
clay_session_info_get_info_with_widget (ClaySessionInfo *info,
                                        GtkWidget       *widget)
{
  GtkWidget *old_widget;

  g_return_if_fail (CLAY_IS_SESSION_INFO (info));
  g_return_if_fail (GTK_IS_WIDGET (widget));

  old_widget = clay_session_info_get_widget (info);

  clay_session_info_set_widget (info, widget);
  clay_session_info_get_info (info);
  clay_session_info_set_widget (info, old_widget);
}

void
clay_session_info_clear_info (ClaySessionInfo *info)
{
  g_return_if_fail (CLAY_IS_SESSION_INFO (info));

  if (info->p->aux_info)
    {
      g_list_free_full (info->p->aux_info,
                        (GDestroyNotify) clay_session_info_aux_free);
      info->p->aux_info = NULL;
    }

  if (info->p->docks)
    {
      g_list_free_full (info->p->docks,
                        (GDestroyNotify) clay_session_info_dock_free);
      info->p->docks = NULL;
    }
}

gboolean
clay_session_info_is_singleton (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return (! clay_session_info_is_for_dock_window (info) &&
          info->p->factory_entry &&
          info->p->factory_entry->singleton);
}

gboolean
clay_session_info_is_session_managed (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return (clay_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->session_managed));
}


gboolean
clay_session_info_get_remember_size (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return (clay_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->remember_size));
}

gboolean
clay_session_info_get_remember_if_open (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return (clay_session_info_is_for_dock_window (info) ||
          (info->p->factory_entry &&
           info->p->factory_entry->remember_if_open));
}

GtkWidget *
clay_session_info_get_widget (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return info->p->widget;
}

void
clay_session_info_set_widget (ClaySessionInfo *info,
                              GtkWidget       *widget)
{
  g_return_if_fail (CLAY_IS_SESSION_INFO (info));

  if (GTK_IS_WINDOW (info->p->widget))
    g_signal_handlers_disconnect_by_func (info->p->widget,
                                          clay_session_info_dialog_show,
                                          info);

  info->p->widget = widget;
}

ClayDialogFactoryEntry *
clay_session_info_get_factory_entry (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return info->p->factory_entry;
}

void
clay_session_info_set_factory_entry (ClaySessionInfo        *info,
                                     ClayDialogFactoryEntry *entry)
{
  g_return_if_fail (CLAY_IS_SESSION_INFO (info));

  info->p->factory_entry = entry;
}

gboolean
clay_session_info_get_open (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), FALSE);

  return info->p->open;
}

gint
clay_session_info_get_x (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), 0);

  return info->p->x;
}

gint
clay_session_info_get_y (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), 0);

  return info->p->y;
}

gint
clay_session_info_get_width (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), 0);

  return info->p->width;
}

gint
clay_session_info_get_height (ClaySessionInfo *info)
{
  g_return_val_if_fail (CLAY_IS_SESSION_INFO (info), 0);

  return info->p->height;
}

static gint position_accuracy = 0;

/**
 * clay_session_info_set_position_accuracy:
 * @accuracy:
 *
 * When writing sessionrc, make positions and sizes a multiple of
 * @accuracy. Meant to be used by test cases that does regression
 * testing on session managed window positions and sizes, to allow for
 * some deviations from the original setup, that the window manager
 * might impose.
 **/
void
clay_session_info_set_position_accuracy (gint accuracy)
{
  position_accuracy = accuracy;
}

/**
 * clay_session_info_apply_position_accuracy:
 * @position:
 *
 * Rounds @position to the nearest multiple of what was set with
 * clay_session_info_set_position_accuracy().
 *
 * Returns: Result.
 **/
gint
clay_session_info_apply_position_accuracy (gint position)
{
  if (position_accuracy > 0)
    {
      gint to_floor = position + position_accuracy / 2;

      return to_floor - to_floor % position_accuracy;
    }

  return position;
}
