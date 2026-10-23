/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablechooser.h
 * Copyright (C) 2023 Jehan
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


#define CLAY_TYPE_PICKABLE_CHOOSER            (clay_pickable_chooser_get_type ())
#define CLAY_PICKABLE_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PICKABLE_CHOOSER, ClayPickableChooser))
#define CLAY_PICKABLE_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PICKABLE_CHOOSER, ClayPickableChooserClass))
#define CLAY_IS_PICKABLE_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PICKABLE_CHOOSER))
#define CLAY_IS_PICKABLE_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PICKABLE_CHOOSER))
#define CLAY_PICKABLE_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PICKABLE_CHOOSER, ClayPickableChooserClass))


typedef struct _ClayPickableChooserPrivate ClayPickableChooserPrivate;
typedef struct _ClayPickableChooserClass   ClayPickableChooserClass;

struct _ClayPickableChooser
{
  GtkFrame                    parent_instance;

  ClayPickableChooserPrivate *priv;
};

struct _ClayPickableChooserClass
{
  GtkFrameClass  parent_class;

  /* Signals. */

  void (* activate) (ClayPickableChooser *view,
                     ClayPickable        *pickable);
};


GType          clay_pickable_chooser_get_type     (void);

GtkWidget    * clay_pickable_chooser_new          (ClayContext         *context,
                                                   GType                pickable_type,
                                                   gint                 view_size,
                                                   gint                 view_border_width);

ClayPickable * clay_pickable_chooser_get_pickable (ClayPickableChooser *chooser);
void           clay_pickable_chooser_set_pickable (ClayPickableChooser *chooser,
                                                   ClayPickable        *pickable);
