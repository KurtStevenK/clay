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

#include "claychannel.h"


#define CLAY_TYPE_SELECTION            (clay_selection_get_type ())
#define CLAY_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SELECTION, ClaySelection))
#define CLAY_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SELECTION, ClaySelectionClass))
#define CLAY_IS_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SELECTION))
#define CLAY_IS_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SELECTION))
#define CLAY_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SELECTION, ClaySelectionClass))


typedef struct _ClaySelectionClass ClaySelectionClass;

struct _ClaySelection
{
  ClayChannel parent_instance;

  gint        suspend_count;
};

struct _ClaySelectionClass
{
  ClayChannelClass parent_class;
};


GType         clay_selection_get_type (void);

ClayChannel * clay_selection_new      (ClayImage     *image,
                                       gint           width,
                                       gint           height);

gint          clay_selection_suspend  (ClaySelection *selection);
gint          clay_selection_resume   (ClaySelection *selection);

GeglBuffer  * clay_selection_extract  (ClaySelection *selection,
                                       GList         *pickables,
                                       ClayContext   *context,
                                       gboolean       cut_image,
                                       gboolean       keep_indexed,
                                       gboolean       add_alpha,
                                       gint          *offset_x,
                                       gint          *offset_y,
                                       GError       **error);

ClayLayer   * clay_selection_float    (ClaySelection *selection,
                                       GList         *drawables,
                                       ClayContext   *context,
                                       gboolean       cut_image,
                                       gint           off_x,
                                       gint           off_y,
                                       GError       **error);
