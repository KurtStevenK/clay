/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydisplayconfig.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/claylist.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockwindow.h"
#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "dialogs/dialogs.h"

#include "windows-actions.h"
#include "windows-commands.h"

#include "clay-intl.h"


static void  windows_actions_display_add               (ClayContainer     *container,
                                                        ClayDisplay       *display,
                                                        ClayActionGroup   *group);
static void  windows_actions_display_remove            (ClayContainer     *container,
                                                        ClayDisplay       *display,
                                                        ClayActionGroup   *group);
static void  windows_actions_display_reorder           (ClayContainer     *container,
                                                        ClayDisplay       *display,
                                                        gint               old_index,
                                                        gint               new_index,
                                                        ClayActionGroup   *group);
static void  windows_actions_image_notify              (ClayDisplay       *display,
                                                        const GParamSpec  *unused,
                                                        ClayActionGroup   *group);
static void  windows_actions_title_notify              (ClayDisplayShell  *shell,
                                                        const GParamSpec  *unused,
                                                        ClayActionGroup   *group);
static void  windows_actions_update_display_accels     (ClayActionGroup   *group);

static void  windows_actions_dock_window_added         (ClayDialogFactory *factory,
                                                        ClayDockWindow    *dock_window,
                                                        ClayActionGroup   *group);
static void  windows_actions_dock_window_removed       (ClayDialogFactory *factory,
                                                        ClayDockWindow    *dock_window,
                                                        ClayActionGroup   *group);
static void  windows_actions_dock_window_notify        (ClayDockWindow    *dock,
                                                        const GParamSpec  *pspec,
                                                        ClayActionGroup   *group);
static void  windows_actions_recent_add                (ClayContainer     *container,
                                                        ClaySessionInfo   *info,
                                                        ClayActionGroup   *group);
static void  windows_actions_recent_remove             (ClayContainer     *container,
                                                        ClaySessionInfo   *info,
                                                        ClayActionGroup   *group);
static void  windows_actions_single_window_mode_notify (ClayDisplayConfig *config,
                                                        GParamSpec        *pspec,
                                                        ClayActionGroup   *group);


/* The only reason we have "Tab" in the action entries below is to
 * give away the hardcoded keyboard shortcut. If the user changes the
 * shortcut to something else, both that shortcut and Tab will
 * work. The reason we have the shortcut hardcoded is because
 * gtk_accelerator_valid() returns FALSE for GDK_tab.
 */

#ifndef PLATFORM_OSX
#define WINDOWS_SHOW_DISPLAY_NEXT_ACCEL     "<alt>Tab"
#define WINDOWS_SHOW_DISPLAY_PREVIOUS_ACCEL "<alt><shift>Tab"
#else
/* on macOS, Cmd+` is used. See: #428 and app/display/claydisplayshell-tool-events.c */
#define WINDOWS_SHOW_DISPLAY_NEXT_ACCEL     "<primary>grave"
#define WINDOWS_SHOW_DISPLAY_PREVIOUS_ACCEL "<primary><shift>grave"
#endif

static const ClayActionEntry windows_actions[] =
{
  { "windows-show-display-next", NULL,
    NC_("windows-action", "Next Image"), NULL, { WINDOWS_SHOW_DISPLAY_NEXT_ACCEL, "Forward", NULL },
    NC_("windows-action", "Switch to the next image"),
    windows_show_display_next_cmd_callback,
    NULL },

  { "windows-show-display-previous", NULL,
    NC_("windows-action", "Previous Image"), NULL, { WINDOWS_SHOW_DISPLAY_PREVIOUS_ACCEL, "Back", NULL },
    NC_("windows-action", "Switch to the previous image"),
    windows_show_display_previous_cmd_callback,
    NULL },

  { "windows-tab-position",        NULL, NC_("windows-action",
                                             "_Tabs Position")   },
};

static const ClayToggleActionEntry windows_toggle_actions[] =
{
  { "windows-hide-docks", NULL,
    NC_("windows-action", "_Hide Docks"), NULL, { NULL },
    NC_("windows-action", "When enabled, docks and other dialogs are hidden, leaving only image windows."),
    windows_hide_docks_cmd_callback,
    FALSE,
    CLAY_HELP_WINDOWS_HIDE_DOCKS },

  { "windows-show-tabs", NULL,
    NC_("windows-action", "_Show Tabs"), NULL, { NULL },
    NC_("windows-action", "When enabled, the image tabs bar is shown."),
    windows_show_tabs_cmd_callback,
    TRUE,
    CLAY_HELP_WINDOWS_SHOW_TABS },

  { "windows-use-single-window-mode", NULL,
    NC_("windows-action", "Single-Window _Mode"), NULL, { NULL },
    NC_("windows-action", "When enabled, Clay is in a single-window mode."),
    windows_use_single_window_mode_cmd_callback,
    TRUE,
    CLAY_HELP_WINDOWS_USE_SINGLE_WINDOW_MODE }
};

static const ClayRadioActionEntry windows_tabs_position_actions[] =
{
  { "windows-tabs-position-top", CLAY_ICON_GO_TOP,
    NC_("windows-tabs-position-action", "_Top"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the top"),
    CLAY_POSITION_TOP, CLAY_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-bottom", CLAY_ICON_GO_BOTTOM,
    NC_("windows-tabs-position-action", "_Bottom"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the bottom"),
    CLAY_POSITION_BOTTOM, CLAY_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-left", CLAY_ICON_GO_FIRST,
    NC_("windows-tabs-position-action", "_Left"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the left"),
    CLAY_POSITION_LEFT, CLAY_HELP_WINDOWS_TABS_POSITION },

  { "windows-tabs-position-right", CLAY_ICON_GO_LAST,
    NC_("windows-tabs-position-action", "_Right"), NULL, { NULL },
    NC_("windows-tabs-position-action", "Position the tabs on the right"),
    CLAY_POSITION_RIGHT, CLAY_HELP_WINDOWS_TABS_POSITION },
};

void
windows_actions_setup (ClayActionGroup *group)
{
  GList *list;

  clay_action_group_add_actions (group, "windows-action",
                                 windows_actions,
                                 G_N_ELEMENTS (windows_actions));

  clay_action_group_add_toggle_actions (group, "windows-action",
                                        windows_toggle_actions,
                                        G_N_ELEMENTS (windows_toggle_actions));

  clay_action_group_add_radio_actions (group, "windows-tabs-position-action",
                                       windows_tabs_position_actions,
                                       G_N_ELEMENTS (windows_tabs_position_actions),
                                       NULL, 0,
                                       windows_set_tabs_position_cmd_callback);

  g_signal_connect_object (group->clay->displays, "add",
                           G_CALLBACK (windows_actions_display_add),
                           group, 0);
  g_signal_connect_object (group->clay->displays, "remove",
                           G_CALLBACK (windows_actions_display_remove),
                           group, 0);
  g_signal_connect_object (group->clay->displays, "reorder",
                           G_CALLBACK (windows_actions_display_reorder),
                           group, 0);

  for (list = clay_get_display_iter (group->clay);
       list;
       list = g_list_next (list))
    {
      ClayDisplay *display = list->data;

      windows_actions_display_add (group->clay->displays, display, group);
    }

  g_signal_connect_object (clay_dialog_factory_get_singleton (), "dock-window-added",
                           G_CALLBACK (windows_actions_dock_window_added),
                           group, 0);
  g_signal_connect_object (clay_dialog_factory_get_singleton (), "dock-window-removed",
                           G_CALLBACK (windows_actions_dock_window_removed),
                           group, 0);

  for (list = clay_dialog_factory_get_open_dialogs (clay_dialog_factory_get_singleton ());
       list;
       list = g_list_next (list))
    {
      ClayDockWindow *dock_window = list->data;

      if (CLAY_IS_DOCK_WINDOW (dock_window))
        windows_actions_dock_window_added (clay_dialog_factory_get_singleton (),
                                           dock_window,
                                           group);
    }

  g_signal_connect_object (global_recent_docks, "add",
                           G_CALLBACK (windows_actions_recent_add),
                           group, 0);
  g_signal_connect_object (global_recent_docks, "remove",
                           G_CALLBACK (windows_actions_recent_remove),
                           group, 0);

  for (list = CLAY_LIST (global_recent_docks)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClaySessionInfo *info = list->data;

      windows_actions_recent_add (global_recent_docks, info, group);
    }

  g_signal_connect_object (group->clay->config, "notify::single-window-mode",
                           G_CALLBACK (windows_actions_single_window_mode_notify),
                           group, 0);
}

void
windows_actions_update (ClayActionGroup *group,
                        gpointer         data)
{
  ClayGuiConfig *config = CLAY_GUI_CONFIG (group->clay->config);
  const gchar   *action = NULL;

#define SET_ACTIVE(action,condition) \
        clay_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("windows-use-single-window-mode", config->single_window_mode);
  SET_ACTIVE ("windows-hide-docks", config->hide_docks);
  SET_ACTIVE ("windows-show-tabs", config->show_tabs);

  switch (config->tabs_position)
    {
    case CLAY_POSITION_TOP:
      action = "windows-tabs-position-top";
      break;
    case CLAY_POSITION_BOTTOM:
      action = "windows-tabs-position-bottom";
      break;
    case CLAY_POSITION_LEFT:
      action = "windows-tabs-position-left";
      break;
    case CLAY_POSITION_RIGHT:
      action = "windows-tabs-position-right";
      break;
    default:
      action = "windows-tabs-position-top";
      break;
    }

  clay_action_group_set_action_active (group, action, TRUE);
  clay_action_group_set_action_sensitive (group, "windows-tab-position", config->single_window_mode,
                                          _("Single-window mode disabled"));
  clay_action_group_set_action_sensitive (group, "windows-show-tabs", config->single_window_mode,
                                          _("Single-window mode disabled"));

#undef SET_ACTIVE
}

gchar *
windows_actions_dock_window_to_action_name (ClayDockWindow *dock_window)
{
  return g_strdup_printf ("windows-dock-%04d",
                          clay_dock_window_get_id (dock_window));
}


/*  private functions  */

static void
windows_actions_display_add (ClayContainer   *container,
                             ClayDisplay     *display,
                             ClayActionGroup *group)
{
  ClayDisplayShell *shell = clay_display_get_shell (display);

  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_actions_image_notify),
                           group, 0);

  g_signal_connect_object (shell, "notify::title",
                           G_CALLBACK (windows_actions_title_notify),
                           group, 0);

  windows_actions_image_notify (display, NULL, group);
}

static void
windows_actions_display_remove (ClayContainer   *container,
                                ClayDisplay     *display,
                                ClayActionGroup *group)
{
  ClayDisplayShell *shell = clay_display_get_shell (display);
  ClayAction       *action;
  gchar            *action_name;

  if (shell)
    g_signal_handlers_disconnect_by_func (shell,
                                          windows_actions_title_notify,
                                          group);

  action_name = clay_display_get_action_name (display);
  action = clay_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    clay_action_group_remove_action_and_accel (group, action);

  windows_actions_update_display_accels (group);
}

static void
windows_actions_display_reorder (ClayContainer   *container,
                                 ClayDisplay     *display,
                                 gint             old_index,
                                 gint             new_index,
                                 ClayActionGroup *group)
{
  windows_actions_update_display_accels (group);
}

static void
windows_actions_image_notify (ClayDisplay      *display,
                              const GParamSpec *unused,
                              ClayActionGroup  *group)
{
  ClayImage  *image = clay_display_get_image (display);
  ClayAction *action;
  gchar      *action_name;

  action_name = clay_display_get_action_name (display);

  action = clay_action_group_get_action (group, action_name);

  if (! action)
    {
      ClayActionEntry entry = { 0 };

      entry.name        = action_name;
      entry.icon_name   = CLAY_ICON_IMAGE;
      entry.label       = "";
      entry.tooltip     = NULL;
      entry.callback    = windows_show_display_cmd_callback;
      entry.help_id     = NULL;

      clay_action_group_add_actions (group, NULL, &entry, 1);

      action = clay_action_group_get_action (group, action_name);

      g_object_set_data (G_OBJECT (action), "display", display);
    }

  g_free (action_name);

  if (image)
    {
      const gchar *display_name;
      gchar       *escaped;
      gchar       *title;

      display_name = clay_image_get_display_name (image);
      escaped = clay_escape_uline (display_name);

      /* TRANSLATORS: label for an action allowing to show (i.e. raise the image
       * tab or window above others) specific images or views of image. The part
       * between quotes is the image name and other view identifiers.
       */
      title = g_strdup_printf (_("Show \"%s-%d.%d\""), escaped,
                               clay_image_get_id (image),
                               clay_display_get_instance (display));

      g_object_set (action,
                    "visible",     TRUE,
                    "label",       title,
                    "short-label", escaped,
                    "tooltip",     clay_image_get_display_path (image),
                    "viewable",    image,
                    NULL);

      g_free (title);
      g_free (escaped);

      windows_actions_update_display_accels (group);
    }
  else
    {
      g_object_set (action,
                    "visible",  FALSE,
                    "viewable", NULL,
                    NULL);
    }
}

static void
windows_actions_title_notify (ClayDisplayShell *shell,
                              const GParamSpec *unused,
                              ClayActionGroup  *group)
{
  windows_actions_image_notify (shell->display, NULL, group);
}

static void
windows_actions_update_display_accels (ClayActionGroup *group)
{
  GList *list;
  gint   i;

  for (list = clay_get_display_iter (group->clay), i = 0;
       list && i < 10;
       list = g_list_next (list), i++)
    {
      ClayDisplay *display = list->data;
      ClayImage   *image   = clay_display_get_image (display);
      ClayAction  *action;
      gchar       *action_name;

      if (image == NULL)
        break;

      action_name = clay_display_get_action_name (display);

      action = clay_action_group_get_action (group, action_name);
      g_free (action_name);

      if (action)
        {
          const gchar *ntooltip;
          gchar       *tooltip;
          gchar       *accel;

          g_object_set (action,
                        "ellipsize",       PANGO_ELLIPSIZE_MIDDLE,
                        "max-width-chars", 40,
                        NULL);

          if (i < 9)
            accel = gtk_accelerator_name (GDK_KEY_1 + i, GDK_MOD1_MASK);
          else
            accel = gtk_accelerator_name (GDK_KEY_0, GDK_MOD1_MASK);

          clay_action_set_accels (action, (const gchar*[]) { accel, NULL });
          g_free (accel);

          /* TRANSLATORS: the first argument (%1$s) is the image name, the
           * second (%2$d) is its tab order in the graphical interface.
           */
          ntooltip = ngettext ("Switch to the first image view: %1$s",
                               "Switch to image view %2$d: %1$s",
                               i + 1);
          tooltip = g_strdup_printf (ntooltip, clay_image_get_display_path (image), i + 1);
          clay_action_set_tooltip (action, tooltip);
          g_free (tooltip);
        }
    }
}

static void
windows_actions_dock_window_added (ClayDialogFactory *factory,
                                   ClayDockWindow    *dock_window,
                                   ClayActionGroup   *group)
{
  ClayAction      *action;
  ClayActionEntry  entry       = { 0 };
  gchar           *action_name = windows_actions_dock_window_to_action_name (dock_window);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = "";
  entry.tooltip     = NULL;
  entry.callback    = windows_show_dock_cmd_callback;
  entry.help_id     = CLAY_HELP_WINDOWS_SHOW_DOCK;

  clay_action_group_add_actions (group, NULL, &entry, 1);

  action = clay_action_group_get_action (group, action_name);

  g_object_set (action,
                "ellipsize", PANGO_ELLIPSIZE_END,
                NULL);

  g_object_set_data (G_OBJECT (action), "dock-window", dock_window);

  g_free (action_name);

  g_signal_connect_object (dock_window, "notify::title",
                           G_CALLBACK (windows_actions_dock_window_notify),
                           group, 0);

  if (gtk_window_get_title (GTK_WINDOW (dock_window)))
    windows_actions_dock_window_notify (dock_window, NULL, group);
}

static void
windows_actions_dock_window_removed (ClayDialogFactory *factory,
                                     ClayDockWindow    *dock_window,
                                     ClayActionGroup   *group)
{
  ClayAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = clay_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    clay_action_group_remove_action_and_accel (group, action);
}

static void
windows_actions_dock_window_notify (ClayDockWindow   *dock_window,
                                    const GParamSpec *pspec,
                                    ClayActionGroup  *group)
{
  ClayAction *action;
  gchar      *action_name;

  action_name = windows_actions_dock_window_to_action_name (dock_window);
  action = clay_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    g_object_set (action,
                  "label",   gtk_window_get_title (GTK_WINDOW (dock_window)),
                  "tooltip", gtk_window_get_title (GTK_WINDOW (dock_window)),
                  NULL);
}

static void
windows_actions_recent_add (ClayContainer   *container,
                            ClaySessionInfo *info,
                            ClayActionGroup *group)
{
  ClayAction      *action;
  ClayActionEntry  entry = { 0 };
  gint             info_id;
  static gint      info_id_counter = 1;
  gchar           *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  if (! info_id)
    {
      info_id = info_id_counter++;

      g_object_set_data (G_OBJECT (info), "recent-action-id",
                         GINT_TO_POINTER (info_id));
    }

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);

  entry.name        = action_name;
  entry.icon_name   = NULL;
  entry.label       = clay_object_get_name (info);
  entry.tooltip     = clay_object_get_name (info);
  entry.callback    = windows_open_recent_cmd_callback;
  entry.help_id     = CLAY_HELP_WINDOWS_OPEN_RECENT_DOCK;

  clay_action_group_add_actions (group, NULL, &entry, 1);

  action = clay_action_group_get_action (group, action_name);

  g_object_set (action,
                "ellipsize",       PANGO_ELLIPSIZE_END,
                "max-width-chars", 30,
                NULL);

  g_object_set_data (G_OBJECT (action), "info", info);

  g_free (action_name);
}

static void
windows_actions_recent_remove (ClayContainer   *container,
                               ClaySessionInfo *info,
                               ClayActionGroup *group)
{
  ClayAction *action;
  gint        info_id;
  gchar      *action_name;

  info_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (info),
                                                "recent-action-id"));

  action_name = g_strdup_printf ("windows-recent-%04d", info_id);
  action = clay_action_group_get_action (group, action_name);
  g_free (action_name);

  if (action)
    clay_action_group_remove_action_and_accel (group, action);
}

static void
windows_actions_single_window_mode_notify (ClayDisplayConfig *config,
                                           GParamSpec        *pspec,
                                           ClayActionGroup   *group)
{
  clay_action_group_set_action_active (group,
                                       "windows-use-single-window-mode",
                                       CLAY_GUI_CONFIG (config)->single_window_mode);
}
