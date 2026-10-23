/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablebutton.h
 * Copyright (C) 2013 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PICKABLE_BUTTON            (clay_pickable_button_get_type ())
#define CLAY_PICKABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PICKABLE_BUTTON, ClayPickableButton))
#define CLAY_PICKABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PICKABLE_BUTTON, ClayPickableButtonClass))
#define CLAY_IS_PICKABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PICKABLE_BUTTON))
#define CLAY_IS_PICKABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PICKABLE_BUTTON))
#define CLAY_PICKABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PICKABLE_BUTTON, ClayPickableButtonClass))


typedef struct _ClayPickableButtonPrivate ClayPickableButtonPrivate;
typedef struct _ClayPickableButtonClass   ClayPickableButtonClass;

struct _ClayPickableButton
{
  ClayButton                 parent_instance;

  ClayPickableButtonPrivate *private;
};

struct _ClayPickableButtonClass
{
  ClayButtonClass  parent_class;
};


GType          clay_pickable_button_get_type     (void);

GtkWidget    * clay_pickable_button_new          (ClayContext        *context,
                                                  gint                view_size,
                                                  gint                view_border_width);

ClayPickable * clay_pickable_button_get_pickable (ClayPickableButton *button);
void           clay_pickable_button_set_pickable (ClayPickableButton *button,
                                                  ClayPickable       *pickable);
