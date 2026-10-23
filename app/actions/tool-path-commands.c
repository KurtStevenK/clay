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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claytoolinfo.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolpath.h"

#include "dialogs/dialogs.h"

#include "tool-path-commands.h"


/*  public functions  */

void
tool_path_delete_anchor_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (data);

  clay_tool_path_delete_anchor (tool_path);
}

void
tool_path_shift_start_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (data);

  clay_tool_path_shift_start (tool_path);
}

void
tool_path_insert_anchor_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (data);

  clay_tool_path_insert_anchor (tool_path);
}

void
tool_path_delete_segment_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (data);

  clay_tool_path_delete_segment (tool_path);
}

void
tool_path_reverse_stroke_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayToolPath *tool_path = CLAY_TOOL_PATH (data);

  clay_tool_path_reverse_stroke (tool_path);
}

