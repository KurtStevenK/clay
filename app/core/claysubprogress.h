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


#define CLAY_TYPE_SUB_PROGRESS            (clay_sub_progress_get_type ())
#define CLAY_SUB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SUB_PROGRESS, ClaySubProgress))
#define CLAY_SUB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SUB_PROGRESS, ClaySubProgressClass))
#define CLAY_IS_SUB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SUB_PROGRESS))
#define CLAY_IS_SUB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SUB_PROGRESS))
#define CLAY_SUB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SUB_PROGRESS, ClaySubProgressClass))


typedef struct _ClaySubProgressClass ClaySubProgressClass;

struct _ClaySubProgress
{
  GObject       parent_instance;

  ClayProgress *progress;
  gdouble       start;
  gdouble       end;
};

struct _ClaySubProgressClass
{
  GObjectClass  parent_class;
};


GType          clay_sub_progress_get_type (void);

ClayProgress * clay_sub_progress_new       (ClayProgress    *progress);
void           clay_sub_progress_set_range (ClaySubProgress *progress,
                                            gdouble          start,
                                            gdouble          end);
void           clay_sub_progress_set_step  (ClaySubProgress *progress,
                                            gint             index,
                                            gint             num_steps);
