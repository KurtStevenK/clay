/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycolorhistory.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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


#define CLAY_TYPE_COLOR_HISTORY            (clay_color_history_get_type ())
#define CLAY_COLOR_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_HISTORY, ClayColorHistory))
#define CLAY_COLOR_HISTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_HISTORY, ClayColorHistoryClass))
#define CLAY_IS_COLOR_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_HISTORY))
#define CLAY_IS_COLOR_HISTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_HISTORY))
#define CLAY_COLOR_HISTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_HISTORY, ClayColorHistoryClass))


typedef struct _ClayColorHistoryClass ClayColorHistoryClass;

struct _ClayColorHistory
{
  GtkGrid       parent_instance;

  ClayContext  *context;
  ClayImage    *active_image;

  GtkWidget   **color_areas;
  GtkWidget   **buttons;
  gint          history_size;
  gint          n_rows;
};

struct _ClayColorHistoryClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void   (* color_selected) (ClayColorHistory *history,
                             GeglColor        *color);
};


GType       clay_color_history_get_type (void);

GtkWidget * clay_color_history_new      (ClayContext     *context,
                                         gint             history_size);
