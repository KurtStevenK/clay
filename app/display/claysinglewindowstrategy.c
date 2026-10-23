/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysinglewindowstrategy.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include "display-types.h"

#include "core/clay.h"

#include "menus/menus.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/claydockbook.h"
#include "widgets/claydockcolumns.h"
#include "widgets/claywindowstrategy.h"

#include "clayimagewindow.h"
#include "claysinglewindowstrategy.h"


static void        clay_single_window_strategy_window_strategy_iface_init (ClayWindowStrategyInterface *iface);
static GtkWidget * clay_single_window_strategy_show_dockable_dialog       (ClayWindowStrategy          *strategy,
                                                                           Clay                        *clay,
                                                                           ClayDialogFactory           *factory,
                                                                           GdkMonitor                  *monitor,
                                                                           const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (ClaySingleWindowStrategy, clay_single_window_strategy, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_WINDOW_STRATEGY,
                                                clay_single_window_strategy_window_strategy_iface_init))

#define parent_class clay_single_window_strategy_parent_class


static void
clay_single_window_strategy_class_init (ClaySingleWindowStrategyClass *klass)
{
}

static void
clay_single_window_strategy_init (ClaySingleWindowStrategy *strategy)
{
}

static void
clay_single_window_strategy_window_strategy_iface_init (ClayWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = clay_single_window_strategy_show_dockable_dialog;
}

static GtkWidget *
clay_single_window_strategy_show_dockable_dialog (ClayWindowStrategy *strategy,
                                                  Clay               *clay,
                                                  ClayDialogFactory  *factory,
                                                  GdkMonitor         *monitor,
                                                  const gchar        *identifiers)
{
  GList           *windows = clay_get_image_windows (clay);
  GtkWidget       *widget  = NULL;
  ClayImageWindow *window;

  g_return_val_if_fail (windows != NULL, NULL);

  /* In single-window mode, there should only be one window... */
  window = CLAY_IMAGE_WINDOW (windows->data);

  if (strcmp ("clay-toolbox", identifiers) == 0)
    {
      /* Only allow one toolbox... */
      if (! clay_image_window_has_toolbox (window))
        {
          ClayDockColumns *columns;
          ClayUIManager   *ui_manager = menus_get_image_manager_singleton (clay);

          widget = clay_dialog_factory_dialog_new (factory, monitor,
                                                   ui_manager,
                                                   GTK_WIDGET (window),
                                                   "clay-toolbox",
                                                   -1 /*view_size*/,
                                                   FALSE /*present*/);
          gtk_widget_set_visible (widget, TRUE);

          columns = clay_image_window_get_left_docks (window);
          clay_dock_columns_add_dock (columns,
                                      CLAY_DOCK (widget),
                                      -1 /*index*/);
        }
      else
        {
          widget = clay_dialog_factory_find_widget (factory, "clay-toolbox");
        }
    }
  else if (clay_dialog_factory_find_widget (factory, identifiers))
    {
      /* if the dialog is already open, simply raise it */
      return clay_dialog_factory_dialog_raise (factory, monitor,
                                               GTK_WIDGET (window),
                                               identifiers, -1);
   }
  else
    {
      GtkWidget *dockbook;

      dockbook = clay_image_window_get_default_dockbook (window);

      if (! dockbook)
        {
          ClayDockColumns *dock_columns;

          /* No dock, need to add one */
          dock_columns = clay_image_window_get_right_docks (window);
          clay_dock_columns_prepare_dockbook (dock_columns,
                                              -1 /*index*/,
                                              &dockbook);
        }

      widget = clay_dockbook_add_from_dialog_factory (CLAY_DOCKBOOK (dockbook),
                                                      identifiers);
    }


  g_list_free (windows);

  return widget;
}

ClayObject *
clay_single_window_strategy_get_singleton (void)
{
  static ClayObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (CLAY_TYPE_SINGLE_WINDOW_STRATEGY, NULL);

  return singleton;
}
