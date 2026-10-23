/* xyz.kainzmayer.ClayDesign
 *
 * claycageoptions.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "core/claytooloptions.h"


#define CLAY_TYPE_CAGE_OPTIONS            (clay_cage_options_get_type ())
#define CLAY_CAGE_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CAGE_OPTIONS, ClayCageOptions))
#define CLAY_CAGE_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CAGE_OPTIONS, ClayCageOptionsClass))
#define CLAY_IS_CAGE_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CAGE_OPTIONS))
#define CLAY_IS_CAGE_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CAGE_OPTIONS))
#define CLAY_CAGE_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CAGE_OPTIONS, ClayCageOptionsClass))


typedef struct _ClayCageOptions      ClayCageOptions;
typedef struct _ClayCageOptionsClass ClayCageOptionsClass;

struct _ClayCageOptions
{
  ClayToolOptions  parent_instance;

  ClayCageMode     cage_mode;
  gboolean         fill_plain_color;
};

struct _ClayCageOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType       clay_cage_options_get_type (void);

GtkWidget * clay_cage_options_gui      (ClayToolOptions *tool_options);
