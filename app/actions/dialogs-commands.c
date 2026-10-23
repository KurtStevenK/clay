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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "actions.h"
#include "dialogs-commands.h"


/*  public functions  */

void
dialogs_create_toplevel_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GtkWidget   *widget;
  const gchar *identifier;
  return_if_no_widget (widget, data);

  identifier = g_variant_get_string (value, NULL);

  if (identifier)
    clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                    clay_widget_get_monitor (widget),
                                    NULL /*ui_manager*/,
                                    widget,
                                    identifier, -1, TRUE);
}

void
dialogs_create_dockable_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  const gchar *identifier;
  return_if_no_clay   (clay, data);
  return_if_no_widget (widget, data);

  identifier = g_variant_get_string (value, NULL);

  if (identifier)
    clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                               clay,
                                               clay_dialog_factory_get_singleton (),
                                               clay_widget_get_monitor (widget),
                                               identifier);
}
