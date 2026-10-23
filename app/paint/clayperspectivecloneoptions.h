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

#include "claycloneoptions.h"


#define CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS            (clay_perspective_clone_options_get_type ())
#define CLAY_PERSPECTIVE_CLONE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS, ClayPerspectiveCloneOptions))
#define CLAY_PERSPECTIVE_CLONE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS, ClayPerspectiveCloneOptionsClass))
#define CLAY_IS_PERSPECTIVE_CLONE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS))
#define CLAY_IS_PERSPECTIVE_CLONE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS))
#define CLAY_PERSPECTIVE_CLONE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PERSPECTIVE_CLONE_OPTIONS, ClayPerspectiveCloneOptionsClass))


typedef struct _ClayPerspectiveCloneOptionsClass ClayPerspectiveCloneOptionsClass;

struct _ClayPerspectiveCloneOptions
{
  ClayCloneOptions          paint_instance;

  ClayPerspectiveCloneMode  clone_mode;
};

struct _ClayPerspectiveCloneOptionsClass
{
  ClayCloneOptionsClass  parent_class;
};


GType   clay_perspective_clone_options_get_type (void);
