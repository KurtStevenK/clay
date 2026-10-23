/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymultiwindowstrategy.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "core/clay.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claywindowstrategy.h"

#include "claymultiwindowstrategy.h"


static void        clay_multi_window_strategy_window_strategy_iface_init (ClayWindowStrategyInterface *iface);
static GtkWidget * clay_multi_window_strategy_show_dockable_dialog       (ClayWindowStrategy          *strategy,
                                                                          Clay                        *clay,
                                                                          ClayDialogFactory           *factory,
                                                                          GdkMonitor                  *monitor,
                                                                          const gchar                 *identifiers);


G_DEFINE_TYPE_WITH_CODE (ClayMultiWindowStrategy, clay_multi_window_strategy, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_WINDOW_STRATEGY,
                                                clay_multi_window_strategy_window_strategy_iface_init))

#define parent_class clay_multi_window_strategy_parent_class


static void
clay_multi_window_strategy_class_init (ClayMultiWindowStrategyClass *klass)
{
}

static void
clay_multi_window_strategy_init (ClayMultiWindowStrategy *strategy)
{
}

static void
clay_multi_window_strategy_window_strategy_iface_init (ClayWindowStrategyInterface *iface)
{
  iface->show_dockable_dialog = clay_multi_window_strategy_show_dockable_dialog;
}

static GtkWidget *
clay_multi_window_strategy_show_dockable_dialog (ClayWindowStrategy *strategy,
                                                 Clay               *clay,
                                                 ClayDialogFactory  *factory,
                                                 GdkMonitor         *monitor,
                                                 const gchar        *identifiers)
{
  return clay_dialog_factory_dialog_raise (factory, monitor, NULL,
                                           identifiers, -1);
}

ClayObject *
clay_multi_window_strategy_get_singleton (void)
{
  static ClayObject *singleton = NULL;

  if (! singleton)
    singleton = g_object_new (CLAY_TYPE_MULTI_WINDOW_STRATEGY, NULL);

  return singleton;
}
