/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytooldialog.h
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#pragma once

#include "widgets/clayviewabledialog.h"


#define CLAY_TYPE_TOOL_DIALOG (clay_tool_dialog_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayToolDialog,
                          clay_tool_dialog,
                          CLAY, TOOL_DIALOG,
                          ClayViewableDialog)


struct _ClayToolDialogClass
{
  ClayViewableDialogClass  parent_class;
};


GtkWidget * clay_tool_dialog_new       (ClayToolInfo     *tool_info,
                                        GdkMonitor       *monitor,
                                        const gchar      *title,
                                        const gchar      *description,
                                        const gchar      *icon_name,
                                        const gchar      *help_id,
                                        ...) G_GNUC_NULL_TERMINATED;

void        clay_tool_dialog_set_shell (ClayToolDialog   *tool_dialog,
                                        ClayDisplayShell *shell);
