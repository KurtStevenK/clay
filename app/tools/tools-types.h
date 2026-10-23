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

#pragma once

#include "paint/paint-types.h"
#include "display/display-types.h"

#include "tools/tools-enums.h"


typedef struct _ClayTool                     ClayTool;
typedef struct _ClayToolControl              ClayToolControl;

typedef struct _ClayBrushTool                ClayBrushTool;
typedef struct _ClayColorTool                ClayColorTool;
typedef struct _ClayDrawTool                 ClayDrawTool;
typedef struct _ClayFilterTool               ClayFilterTool;
typedef struct _ClayGenericTransformTool     ClayGenericTransformTool;
typedef struct _ClayPaintTool                ClayPaintTool;
typedef struct _ClayTransformGridTool        ClayTransformGridTool;
typedef struct _ClayTransformTool            ClayTransformTool;

typedef struct _ClayColorOptions             ClayColorOptions;
typedef struct _ClayFilterOptions            ClayFilterOptions;


/*  functions  */

typedef void (* ClayToolRegisterCallback) (GType                     tool_type,
                                           GType                     tool_option_type,
                                           ClayToolOptionsGUIFunc    options_gui_func,
                                           ClayContextPropMask       context_props,
                                           const gchar              *identifier,
                                           const gchar              *label,
                                           const gchar              *tooltip,
                                           const gchar              *menu_path,
                                           const gchar              *menu_accel,
                                           const gchar              *help_domain,
                                           const gchar              *help_data,
                                           const gchar              *icon_name,
                                           gpointer                  register_data);

typedef void (* ClayToolRegisterFunc)     (ClayToolRegisterCallback  callback,
                                           gpointer                  register_data);
