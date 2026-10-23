/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "claycontext.h"


#define CLAY_TYPE_TOOL_OPTIONS            (clay_tool_options_get_type ())
#define CLAY_TOOL_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_OPTIONS, ClayToolOptions))
#define CLAY_TOOL_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_OPTIONS, ClayToolOptionsClass))
#define CLAY_IS_TOOL_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_OPTIONS))
#define CLAY_IS_TOOL_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_OPTIONS))
#define CLAY_TOOL_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_OPTIONS, ClayToolOptionsClass))


typedef struct _ClayToolOptionsClass ClayToolOptionsClass;

struct _ClayToolOptions
{
  ClayContext   parent_instance;

  ClayToolInfo *tool_info;

  /*  if TRUE this instance is the main tool options object used for
   *  the GUI, this is not exactly clean, but there are some things
   *  (like linking brush properties to the active brush, or properly
   *  maintaining global brush, pattern etc.) that can only be done
   *  right in the object, and not by signal connections from the GUI,
   *  or upon switching tools, all of which was much more horrible.
   */
  gboolean      gui_mode;
};

struct _ClayToolOptionsClass
{
  ClayContextClass parent_class;
};


GType      clay_tool_options_get_type      (void);

void       clay_tool_options_set_gui_mode  (ClayToolOptions   *tool_options,
                                            gboolean           gui_mode);
gboolean   clay_tool_options_get_gui_mode  (ClayToolOptions   *tool_options);

gboolean   clay_tool_options_serialize     (ClayToolOptions   *tool_options,
                                            GError           **error);
gboolean   clay_tool_options_deserialize   (ClayToolOptions   *tool_options,
                                            GError           **error);

gboolean   clay_tool_options_delete        (ClayToolOptions   *tool_options,
                                            GError           **error);
void       clay_tool_options_create_folder (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayToolOptions, g_object_unref);
