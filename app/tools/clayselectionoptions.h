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

#include "core/claytooloptions.h"


#define CLAY_TYPE_SELECTION_OPTIONS            (clay_selection_options_get_type ())
#define CLAY_SELECTION_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SELECTION_OPTIONS, ClaySelectionOptions))
#define CLAY_SELECTION_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SELECTION_OPTIONS, ClaySelectionOptionsClass))
#define CLAY_IS_SELECTION_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SELECTION_OPTIONS))
#define CLAY_IS_SELECTION_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SELECTION_OPTIONS))
#define CLAY_SELECTION_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SELECTION_OPTIONS, ClaySelectionOptionsClass))


typedef struct _ClaySelectionOptions ClaySelectionOptions;
typedef struct _ClayToolOptionsClass ClaySelectionOptionsClass;

struct _ClaySelectionOptions
{
  ClayToolOptions  parent_instance;

  ClayChannelOps   operation;
  gboolean         antialias;
  gboolean         feather;
  gdouble          feather_radius;

  /*  options gui  */
  GtkWidget       *mode_box;
  GtkWidget       *antialias_toggle;
};


GType        clay_selection_options_get_type    (void);

GtkWidget  * clay_selection_options_gui         (ClayToolOptions *tool_options);

/* For usage in claypaintselectoptions.c */

GtkWidget * clay_selection_options_get_mode_box (ClayToolOptions *tool_options,
                                                 ClayChannelOps   min_op,
                                                 ClayChannelOps   max_op);
