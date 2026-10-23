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

#include "core/clayprogress.h"

#include "widgets/clayhelp.h"

#include "actions.h"
#include "help-commands.h"


void
help_help_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Clay        *clay;
  ClayDisplay *display;
  return_if_no_clay (clay, data);
  return_if_no_display (display, data);

  clay_help_show (clay, CLAY_PROGRESS (display), NULL, NULL);
}

void
help_context_help_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  clay_context_help (widget);
}

void
help_online_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  g_app_info_launch_default_for_uri (g_variant_get_string (value, NULL),
                                     NULL, NULL);
}
