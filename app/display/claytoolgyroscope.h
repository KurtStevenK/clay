/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolgyroscope.h
 * Copyright (C) 2018 Ell
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


#define CLAY_TYPE_TOOL_GYROSCOPE            (clay_tool_gyroscope_get_type ())
#define CLAY_TOOL_GYROSCOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_GYROSCOPE, ClayToolGyroscope))
#define CLAY_TOOL_GYROSCOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_GYROSCOPE, ClayToolGyroscopeClass))
#define CLAY_IS_TOOL_GYROSCOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_GYROSCOPE))
#define CLAY_IS_TOOL_GYROSCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_GYROSCOPE))
#define CLAY_TOOL_GYROSCOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_GYROSCOPE, ClayToolGyroscopeClass))


typedef struct _ClayToolGyroscope        ClayToolGyroscope;
typedef struct _ClayToolGyroscopePrivate ClayToolGyroscopePrivate;
typedef struct _ClayToolGyroscopeClass   ClayToolGyroscopeClass;

struct _ClayToolGyroscope
{
  ClayToolWidget            parent_instance;

  ClayToolGyroscopePrivate *private;
};

struct _ClayToolGyroscopeClass
{
  ClayToolWidgetClass  parent_class;
};


GType            clay_tool_gyroscope_get_type (void);

ClayToolWidget * clay_tool_gyroscope_new      (ClayDisplayShell *shell);
