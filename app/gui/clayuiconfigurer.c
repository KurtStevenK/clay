/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayuiconfigurer.c
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
#include "libclaywidgets/claywidgets.h"

#include "gui-types.h"

#include "core/clay.h"
#include "core/claycontext.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockcolumns.h"
#include "widgets/claydockcontainer.h"
#include "widgets/claydockwindow.h"
#include "widgets/claytoolbox.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/clayimagewindow.h"

#include "clayuiconfigurer.h"


enum
{
  PROP_0,
  PROP_CLAY
};


struct _ClayUIConfigurerPrivate
{
  Clay *clay;
};


static void              clay_ui_configurer_set_property                (GObject           *object,
                                                                         guint              property_id,
                                                                         const GValue      *value,
                                                                         GParamSpec        *pspec);
static void              clay_ui_configurer_get_property                (GObject           *object,
                                                                         guint              property_id,
                                                                         GValue            *value,
                                                                         GParamSpec        *pspec);
static void              clay_ui_configurer_move_docks_to_columns       (ClayUIConfigurer  *ui_configurer,
                                                                         ClayImageWindow   *uber_image_window);
static void              clay_ui_configurer_move_shells                 (ClayUIConfigurer  *ui_configurer,
                                                                         ClayImageWindow   *source_image_window,
                                                                         ClayImageWindow   *target_image_window);
static void              clay_ui_configurer_separate_docks              (ClayUIConfigurer  *ui_configurer,
                                                                         ClayImageWindow   *source_image_window);
static void              clay_ui_configurer_move_docks_to_window        (ClayUIConfigurer  *ui_configurer,
                                                                         ClayDockColumns   *dock_columns,
                                                                         ClayAlignmentType  screen_side);
static void              clay_ui_configurer_separate_shells             (ClayUIConfigurer  *ui_configurer,
                                                                         ClayImageWindow   *source_image_window);
static void              clay_ui_configurer_configure_for_single_window (ClayUIConfigurer  *ui_configurer);
static void              clay_ui_configurer_configure_for_multi_window  (ClayUIConfigurer  *ui_configurer);
static ClayImageWindow * clay_ui_configurer_get_uber_window             (ClayUIConfigurer  *ui_configurer);


G_DEFINE_TYPE_WITH_PRIVATE (ClayUIConfigurer, clay_ui_configurer,
                            CLAY_TYPE_OBJECT)

#define parent_class clay_ui_configurer_parent_class


static void
clay_ui_configurer_class_init (ClayUIConfigurerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = clay_ui_configurer_set_property;
  object_class->get_property = clay_ui_configurer_get_property;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_ui_configurer_init (ClayUIConfigurer *ui_configurer)
{
  ui_configurer->p = clay_ui_configurer_get_instance_private (ui_configurer);
}

static void
clay_ui_configurer_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayUIConfigurer *ui_configurer = CLAY_UI_CONFIGURER (object);

  switch (property_id)
    {
    case PROP_CLAY:
      ui_configurer->p->clay = g_value_get_object (value); /* don't ref */
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_ui_configurer_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayUIConfigurer *ui_configurer = CLAY_UI_CONFIGURER (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, ui_configurer->p->clay);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
clay_ui_configurer_get_window_center_pos (GtkWindow *window,
                                          gint      *out_x,
                                          gint      *out_y)
{
  gint x, y, w, h;
  gtk_window_get_position (window, &x, &y);
  gtk_window_get_size (window, &w, &h);

  if (out_x)
    *out_x = x + w / 2;
  if (out_y)
    *out_y = y + h / 2;
}

/**
 * clay_ui_configurer_get_relative_window_pos:
 * @window_a:
 * @window_b:
 *
 * Returns: At what side @window_b is relative to @window_a. Either
 * CLAY_ALIGN_LEFT or CLAY_ALIGN_RIGHT.
 **/
static ClayAlignmentType
clay_ui_configurer_get_relative_window_pos (GtkWindow *window_a,
                                            GtkWindow *window_b)
{
  gint a_x, b_x;

  clay_ui_configurer_get_window_center_pos (window_a, &a_x, NULL);
  clay_ui_configurer_get_window_center_pos (window_b, &b_x, NULL);

  return b_x < a_x ? CLAY_ALIGN_LEFT : CLAY_ALIGN_RIGHT;
}

static void
clay_ui_configurer_move_docks_to_columns (ClayUIConfigurer *ui_configurer,
                                          ClayImageWindow  *uber_image_window)
{
  GList *dialogs     = NULL;
  GList *dialog_iter = NULL;

  dialogs =
    g_list_copy (clay_dialog_factory_get_open_dialogs (clay_dialog_factory_get_singleton ()));

  for (dialog_iter = dialogs; dialog_iter; dialog_iter = dialog_iter->next)
    {
      ClayDockWindow    *dock_window;
      ClayDockContainer *dock_container;
      ClayDockColumns   *dock_columns;
      GList             *docks;
      GList             *dock_iter;

      if (!CLAY_IS_DOCK_WINDOW (dialog_iter->data))
        continue;

      dock_window = CLAY_DOCK_WINDOW (dialog_iter->data);

      /* If the dock window is on the left side of the image window,
       * move the docks to the left side. If the dock window is on the
       * right side, move the docks to the right side of the image
       * window.
       */
      if (clay_ui_configurer_get_relative_window_pos (GTK_WINDOW (uber_image_window),
                                                      GTK_WINDOW (dock_window)) == CLAY_ALIGN_LEFT)
        dock_columns = clay_image_window_get_left_docks (uber_image_window);
      else
        dock_columns = clay_image_window_get_right_docks (uber_image_window);

      dock_container = CLAY_DOCK_CONTAINER (dock_window);

      g_object_add_weak_pointer (G_OBJECT (dock_window),
                                 (gpointer) &dock_window);

      docks = clay_dock_container_get_docks (dock_container);
      for (dock_iter = docks; dock_iter; dock_iter = dock_iter->next)
        {
          ClayDock *dock = CLAY_DOCK (dock_iter->data);

          /* Move the dock from the image window to the dock columns
           * widget. Note that we need a ref while the dock is parentless
           */
          g_object_ref (dock);
          clay_dock_window_remove_dock (dock_window, dock);
          clay_dock_columns_add_dock (dock_columns, dock, -1);
          g_object_unref (dock);
        }
      g_list_free (docks);

      if (dock_window)
        g_object_remove_weak_pointer (G_OBJECT (dock_window),
                                      (gpointer) &dock_window);

      /* Kill the window if removing the dock didn't destroy it
       * already. This will be the case for the toolbox dock window
       */
      if (GTK_IS_WIDGET (dock_window))
        {
          guint docks_len;

          docks     = clay_dock_container_get_docks (dock_container);
          docks_len = g_list_length (docks);

          if (docks_len == 0)
            {
              clay_dialog_factory_remove_dialog (clay_dialog_factory_get_singleton (),
                                                 GTK_WIDGET (dock_window));
              gtk_widget_destroy (GTK_WIDGET (dock_window));
            }

          g_list_free (docks);
        }
    }

  g_list_free (dialogs);
}

/**
 * clay_ui_configurer_move_shells:
 * @ui_configurer:
 * @source_image_window:
 * @target_image_window:
 *
 * Move all display shells from one image window to the another.
 **/
static void
clay_ui_configurer_move_shells (ClayUIConfigurer  *ui_configurer,
                                ClayImageWindow   *source_image_window,
                                ClayImageWindow   *target_image_window)
{
  while (clay_image_window_get_n_shells (source_image_window) > 0)
    {
      ClayDisplayShell *shell;

      shell = clay_image_window_get_shell (source_image_window, 0);

      g_object_ref (shell);
      clay_image_window_remove_shell (source_image_window, shell);
      clay_image_window_add_shell (target_image_window, shell);
      g_object_unref (shell);
    }
}

/**
 * clay_ui_configurer_separate_docks:
 * @ui_configurer:
 * @image_window:
 *
 * Move out the docks from the image window.
 **/
static void
clay_ui_configurer_separate_docks (ClayUIConfigurer  *ui_configurer,
                                   ClayImageWindow   *image_window)
{
  ClayDockColumns *left_docks  = NULL;
  ClayDockColumns *right_docks = NULL;

  left_docks  = clay_image_window_get_left_docks (image_window);
  right_docks = clay_image_window_get_right_docks (image_window);

  clay_ui_configurer_move_docks_to_window (ui_configurer, left_docks, CLAY_ALIGN_LEFT);
  clay_ui_configurer_move_docks_to_window (ui_configurer, right_docks, CLAY_ALIGN_RIGHT);
}

/**
 * clay_ui_configurer_move_docks_to_window:
 * @dock_columns:
 * @screen_side: At what side of the screen the dock window should be put.
 *
 * Moves docks in @dock_columns into a new #ClayDockWindow and
 * position it on the screen in a non-overlapping manner.
 */
static void
clay_ui_configurer_move_docks_to_window (ClayUIConfigurer  *ui_configurer,
                                         ClayDockColumns   *dock_columns,
                                         ClayAlignmentType  screen_side)
{
  GdkMonitor    *monitor;
  GdkRectangle   monitor_rect;
  GdkGravity     gravity;
  GList         *docks;
  GList         *iter;
  gboolean       contains_toolbox = FALSE;
  GtkWidget     *dock_window;
  GtkAllocation  original_size;
  gint           x, y;

  docks = g_list_copy (clay_dock_columns_get_docks (dock_columns));
  if (! docks)
    return;

  monitor = clay_widget_get_monitor (GTK_WIDGET (dock_columns));

  gdk_monitor_get_workarea (monitor, &monitor_rect);

  /* Remember the size so we can set the new dock window to the same
   * size
   */
  gtk_widget_get_allocation (GTK_WIDGET (dock_columns), &original_size);

  /* Do we need a toolbox window? */
  for (iter = docks; iter; iter = g_list_next (iter))
    {
      ClayDock *dock = CLAY_DOCK (iter->data);

      if (CLAY_IS_TOOLBOX (dock))
        {
          contains_toolbox = TRUE;
          break;
        }
    }

  /* Create a dock window to put the dock in. Checking for
   * CLAY_IS_TOOLBOX() is kind of ugly but not a disaster. We need
   * the dock window correctly configured if we create it for the
   * toolbox
   */
  dock_window =
    clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                    monitor,
                                    NULL /*ui_manager*/,
                                    GTK_WIDGET (dock_columns),
                                    (contains_toolbox ?
                                     "clay-toolbox-window" :
                                     "clay-dock-window"),
                                    -1 /*view_size*/,
                                    FALSE /*present*/);

  for (iter = docks; iter; iter = g_list_next (iter))
    {
      ClayDock *dock = CLAY_DOCK (iter->data);

      /* Move the dock to the window */
      g_object_ref (dock);
      clay_dock_columns_remove_dock (dock_columns, dock);
      clay_dock_window_add_dock (CLAY_DOCK_WINDOW (dock_window), dock, -1);
      g_object_unref (dock);
    }

  /* Position the window */
  if (screen_side == CLAY_ALIGN_LEFT)
    {
      gravity = GDK_GRAVITY_NORTH_WEST;

      x = monitor_rect.x;
      y = monitor_rect.y;
    }
  else if (screen_side == CLAY_ALIGN_RIGHT)
    {
      gravity = GDK_GRAVITY_NORTH_EAST;

      x = monitor_rect.x + monitor_rect.width - original_size.width;
      y = monitor_rect.y;
    }
  else
    {
      clay_assert_not_reached ();
    }

  gtk_window_set_gravity (GTK_WINDOW (dock_window), gravity);
  gtk_window_move (GTK_WINDOW (dock_window), x, y);

  /* Try to keep the same size */
  gtk_window_set_default_size (GTK_WINDOW (dock_window),
                               original_size.width,
                               original_size.height);

  /* Don't forget to show the window */
  gtk_widget_set_visible (dock_window, TRUE);

  g_list_free (docks);
}

/**
 * clay_ui_configurer_separate_shells:
 * @ui_configurer:
 * @source_image_window:
 *
 * Create one image window per display shell and move it there.
 **/
static void
clay_ui_configurer_separate_shells (ClayUIConfigurer *ui_configurer,
                                    ClayImageWindow  *source_image_window)
{
  ClayDisplayShell *active_shell  = clay_image_window_get_active_shell (source_image_window);
  ClayImageWindow  *active_window = NULL;

  /* The last display shell remains in its window */
  while (clay_image_window_get_n_shells (source_image_window) > 1)
    {
      ClayImageWindow  *new_image_window;
      ClayDisplayShell *shell;

      /* Create a new image window */
      new_image_window = clay_image_window_new (ui_configurer->p->clay,
                                                NULL,
                                                clay_dialog_factory_get_singleton (),
                                                clay_widget_get_monitor (GTK_WIDGET (source_image_window)));
      /* Move the shell there */
      shell = clay_image_window_get_shell (source_image_window, 1);

      if (shell == active_shell)
        active_window = new_image_window;

      g_object_ref (shell);
      clay_image_window_remove_shell (source_image_window, shell);
      clay_image_window_add_shell (new_image_window, shell);
      g_object_unref (shell);

      /* FIXME: If we don't set a size request here the window will be
       * too small. Get rid of this hack and fix it the proper way
       */
      gtk_widget_set_size_request (GTK_WIDGET (new_image_window), 640, 480);

      /* Show after we have added the shell */
      gtk_widget_set_visible (GTK_WIDGET (new_image_window), TRUE);
    }

  /* If none of the shells were active, I assume the first one is. */
  if (active_window == NULL)
    active_window = source_image_window;

  /* The active tab must stay at the top of the windows stack. */
  gtk_window_present (GTK_WINDOW (active_window));
}

/**
 * clay_ui_configurer_configure_for_single_window:
 * @ui_configurer:
 *
 * Move docks and display shells into a single window.
 **/
static void
clay_ui_configurer_configure_for_single_window (ClayUIConfigurer *ui_configurer)
{
  Clay             *clay              = ui_configurer->p->clay;
  GList            *windows           = clay_get_image_windows (clay);
  GList            *iter              = NULL;
  ClayImageWindow  *uber_image_window = NULL;
  ClayDisplay      *active_display    = clay_context_get_display (clay_get_user_context (clay));
  ClayDisplayShell *active_shell      = clay_display_get_shell (active_display);

  /* Get and setup the window to put everything in */
  uber_image_window = clay_ui_configurer_get_uber_window (ui_configurer);

  /* Mve docks to the left and right side of the image window */
  clay_ui_configurer_move_docks_to_columns (ui_configurer,
                                            uber_image_window);

  /* Move image shells from other windows to the uber image window */
  for (iter = windows; iter; iter = g_list_next (iter))
    {
      ClayImageWindow *image_window = CLAY_IMAGE_WINDOW (iter->data);

      /* Don't move stuff to itself */
      if (image_window == uber_image_window)
        continue;

      /* Put the displays in the rest of the image windows into
       * the uber image window
       */
      clay_ui_configurer_move_shells (ui_configurer,
                                      image_window,
                                      uber_image_window);
      /* Destroy the window */
      clay_image_window_destroy (image_window);
    }

  /* Ensure the context shell remains active after mode switch. */
  clay_image_window_set_active_shell (uber_image_window, active_shell);

  g_list_free (windows);
}

/**
 * clay_ui_configurer_configure_for_multi_window:
 * @ui_configurer:
 *
 * Moves all display shells into their own image window.
 **/
static void
clay_ui_configurer_configure_for_multi_window (ClayUIConfigurer *ui_configurer)
{
  Clay  *clay    = ui_configurer->p->clay;
  GList *windows = clay_get_image_windows (clay);
  GList *iter    = NULL;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      ClayImageWindow *image_window = CLAY_IMAGE_WINDOW (iter->data);

      clay_ui_configurer_separate_docks (ui_configurer, image_window);

      clay_ui_configurer_separate_shells (ui_configurer, image_window);
    }

  g_list_free (windows);
}

/**
 * clay_ui_configurer_get_uber_window:
 * @ui_configurer:
 *
 * Returns: The window to be used as the main window for single-window
 *          mode.
 **/
static ClayImageWindow *
clay_ui_configurer_get_uber_window (ClayUIConfigurer *ui_configurer)
{
  Clay             *clay         = ui_configurer->p->clay;
  ClayDisplay      *display      = clay_get_display_iter (clay)->data;
  ClayDisplayShell *shell        = clay_display_get_shell (display);
  ClayImageWindow  *image_window = clay_display_shell_get_window (shell);

  return image_window;
}

/**
 * clay_ui_configurer_update_appearance:
 * @ui_configurer:
 *
 * Updates the appearance of all shells in all image windows, so they
 * do whatever they deem necessary to fit the new UI mode mode.
 **/
static void
clay_ui_configurer_update_appearance (ClayUIConfigurer *ui_configurer)
{
  Clay  *clay    = ui_configurer->p->clay;
  GList *windows = clay_get_image_windows (clay);
  GList *list;

  for (list = windows; list; list = g_list_next (list))
    {
      ClayImageWindow *image_window = CLAY_IMAGE_WINDOW (list->data);
      gint             n_shells;
      gint             i;

      n_shells = clay_image_window_get_n_shells (image_window);

      for (i = 0; i < n_shells; i++)
        {
          ClayDisplayShell *shell;

          shell = clay_image_window_get_shell (image_window, i);

          clay_display_shell_appearance_update (shell);
        }
    }

  g_list_free (windows);
}

/**
 * clay_ui_configurer_configure:
 * @ui_configurer:
 * @single_window_mode:
 *
 * Configure the UI.
 **/
void
clay_ui_configurer_configure (ClayUIConfigurer *ui_configurer,
                              gboolean          single_window_mode)
{
  if (single_window_mode)
    clay_ui_configurer_configure_for_single_window (ui_configurer);
  else
    clay_ui_configurer_configure_for_multi_window (ui_configurer);

  clay_ui_configurer_update_appearance (ui_configurer);
}
