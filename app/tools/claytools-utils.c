/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claylayer.h"

#include "path/claypath.h"

#include "widgets/claycontainerview.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayitemtreeview.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "claytools-utils.h"


static ClayItemTreeView * clay_tools_get_tree_view_for (Clay     *clay,
                                                        ClayItem *item);


/*  public functions  */

void
clay_tools_blink_lock_box (Clay     *clay,
                           ClayItem *item)
{
  ClayItemTreeView *view;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_ITEM (item));

  view = clay_tools_get_tree_view_for (clay, item);
  clay_item_tree_view_blink_lock (view, item);
}

void
clay_tools_blink_item (Clay     *clay,
                       ClayItem *item)
{
  ClayItemTreeView *view;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_ITEM (item));

  view = clay_tools_get_tree_view_for (clay, item);
  clay_item_tree_view_blink_item (view, item);
}

void
clay_tools_show_tool_options (Clay *clay)
{
  GdkMonitor *monitor;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  monitor = clay_get_monitor_at_pointer ();

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                             clay,
                                             clay_dialog_factory_get_singleton (),
                                             monitor, "clay-tool-options");
}


/*  Private functions  */

static ClayItemTreeView *
clay_tools_get_tree_view_for (Clay     *clay,
                              ClayItem *item)
{
  GtkWidget        *dockable;
  ClayItemTreeView *view;
  GdkMonitor       *monitor;
  const gchar      *identifier;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  if (CLAY_IS_LAYER (item))
    identifier = "clay-layer-list";
  else if (CLAY_IS_CHANNEL (item))
    identifier = "clay-channel-list";
  else if (CLAY_IS_PATH (item))
    identifier = "clay-path-list";
  else
    g_return_val_if_reached (NULL);

  monitor  = clay_get_monitor_at_pointer ();
  dockable = clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                                        clay,
                                                        clay_dialog_factory_get_singleton (),
                                                        monitor,
                                                        identifier);

  if (! dockable)
    return NULL;

  view = CLAY_ITEM_TREE_VIEW (gtk_bin_get_child (GTK_BIN (dockable)));

  return view;
}
