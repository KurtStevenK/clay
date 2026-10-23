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

#include "clayobject.h"


#define CLAY_TYPE_DISPLAY            (clay_display_get_type ())
#define CLAY_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DISPLAY, ClayDisplay))
#define CLAY_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DISPLAY, ClayDisplayClass))
#define CLAY_IS_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DISPLAY))
#define CLAY_IS_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DISPLAY))
#define CLAY_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DISPLAY, ClayDisplayClass))


typedef struct _ClayDisplayClass   ClayDisplayClass;
typedef struct _ClayDisplayPrivate ClayDisplayPrivate;

struct _ClayDisplay
{
  ClayObject          parent_instance;

  Clay               *clay;
  ClayDisplayConfig  *config;

  ClayDisplayPrivate *priv;
};

struct _ClayDisplayClass
{
  ClayObjectClass     parent_class;

  gboolean         (* present)    (ClayDisplay *display);
  gboolean         (* grab_focus) (ClayDisplay *display);
};


GType         clay_display_get_type  (void);

gint          clay_display_get_id    (ClayDisplay *display);
ClayDisplay * clay_display_get_by_id (Clay        *clay,
                                      gint         id);

gboolean      clay_display_present    (ClayDisplay *display);
gboolean      clay_display_grab_focus (ClayDisplay *display);

Clay        * clay_display_get_clay  (ClayDisplay *display);
