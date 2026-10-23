/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolpath.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "claytoolwidget.h"


#define CLAY_TYPE_TOOL_PATH            (clay_tool_path_get_type ())
#define CLAY_TOOL_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_PATH, ClayToolPath))
#define CLAY_TOOL_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_PATH, ClayToolPathClass))
#define CLAY_IS_TOOL_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_PATH))
#define CLAY_IS_TOOL_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_PATH))
#define CLAY_TOOL_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_PATH, ClayToolPathClass))


typedef struct _ClayToolPath        ClayToolPath;
typedef struct _ClayToolPathPrivate ClayToolPathPrivate;
typedef struct _ClayToolPathClass   ClayToolPathClass;

struct _ClayToolPath
{
  ClayToolWidget       parent_instance;

  ClayToolPathPrivate *private;
};

struct _ClayToolPathClass
{
  ClayToolWidgetClass  parent_class;

  void (* begin_change) (ClayToolPath    *path,
                         const gchar     *desc);
  void (* end_change)   (ClayToolPath    *path,
                         gboolean         success);
  void (* activate)     (ClayToolPath    *path,
                         GdkModifierType  state);
};


GType            clay_tool_path_get_type        (void);

ClayToolWidget * clay_tool_path_new             (ClayDisplayShell *shell);

void             clay_tool_path_set_path        (ClayToolPath     *tool_path,
                                                 ClayPath         *path);

void             clay_tool_path_get_popup_state (ClayToolPath     *tool_path,
                                                 gboolean         *on_handle,
                                                 gboolean         *on_curve);

void             clay_tool_path_delete_anchor   (ClayToolPath     *tool_path);
void             clay_tool_path_shift_start     (ClayToolPath     *tool_path);
void             clay_tool_path_insert_anchor   (ClayToolPath     *tool_path);
void             clay_tool_path_delete_segment  (ClayToolPath     *tool_path);
void             clay_tool_path_reverse_stroke  (ClayToolPath     *tool_path);
