/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycolordialog.h
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

#include "clayviewabledialog.h"


#define CLAY_COLOR_DIALOG_HISTORY_SIZE 12


#define CLAY_TYPE_COLOR_DIALOG            (clay_color_dialog_get_type ())
#define CLAY_COLOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_DIALOG, ClayColorDialog))
#define CLAY_COLOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_DIALOG, ClayColorDialogClass))
#define CLAY_IS_COLOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_DIALOG))
#define CLAY_IS_COLOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_DIALOG))
#define CLAY_COLOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_DIALOG, ClayColorDialogClass))


typedef struct _ClayColorDialogClass ClayColorDialogClass;

struct _ClayColorDialog
{
  ClayViewableDialog   parent_instance;

  gboolean             wants_updates;
  gboolean             user_context_aware;

  GtkWidget           *stack;
  GtkWidget           *selection;
  GtkWidget           *colormap_selection;

  ClayImage           *active_image;
  gboolean             colormap_editing;
};

struct _ClayColorDialogClass
{
  ClayViewableDialogClass  parent_class;

  void (* update) (ClayColorDialog      *dialog,
                   GeglColor            *color,
                   ClayColorDialogState  state);
};


GType       clay_color_dialog_get_type  (void);

GtkWidget * clay_color_dialog_new       (ClayViewable      *viewable,
                                         ClayContext       *context,
                                         gboolean           context_aware,
                                         const gchar       *title,
                                         const gchar       *icon_name,
                                         const gchar       *desc,
                                         GtkWidget         *parent,
                                         ClayDialogFactory *dialog_factory,
                                         const gchar       *dialog_identifier,
                                         GeglColor         *color,
                                         gboolean           wants_update,
                                         gboolean           show_alpha);

void        clay_color_dialog_set_color (ClayColorDialog   *dialog,
                                         GeglColor         *color);
GeglColor * clay_color_dialog_get_color (ClayColorDialog   *dialog);
