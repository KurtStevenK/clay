/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoolcompass.h
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


#define CLAY_TYPE_TOOL_COMPASS            (clay_tool_compass_get_type ())
#define CLAY_TOOL_COMPASS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOOL_COMPASS, ClayToolCompass))
#define CLAY_TOOL_COMPASS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOOL_COMPASS, ClayToolCompassClass))
#define CLAY_IS_TOOL_COMPASS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOOL_COMPASS))
#define CLAY_IS_TOOL_COMPASS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TOOL_COMPASS))
#define CLAY_TOOL_COMPASS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TOOL_COMPASS, ClayToolCompassClass))


typedef struct _ClayToolCompass        ClayToolCompass;
typedef struct _ClayToolCompassPrivate ClayToolCompassPrivate;
typedef struct _ClayToolCompassClass   ClayToolCompassClass;

struct _ClayToolCompass
{
  ClayToolWidget          parent_instance;

  ClayToolCompassPrivate *private;
};

struct _ClayToolCompassClass
{
  ClayToolWidgetClass  parent_class;

  void (* create_guides) (ClayToolCompass *compass,
                          gint             x,
                          gint             y,
                          gboolean         horizontal,
                          gboolean         vertical);
};


GType            clay_tool_compass_get_type (void);

ClayToolWidget * clay_tool_compass_new      (ClayDisplayShell       *shell,
                                             ClayCompassOrientation  orinetation,
                                             gint                    n_points,
                                             gint                    x1,
                                             gint                    y1,
                                             gint                    x2,
                                             gint                    y2,
                                             gint                    y3,
                                             gint                    x3);
