/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewablebutton.h
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_VIEWABLE_BUTTON            (clay_viewable_button_get_type ())
#define CLAY_VIEWABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEWABLE_BUTTON, ClayViewableButton))
#define CLAY_VIEWABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEWABLE_BUTTON, ClayViewableButtonClass))
#define CLAY_IS_VIEWABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_VIEWABLE_BUTTON))
#define CLAY_IS_VIEWABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEWABLE_BUTTON))
#define CLAY_VIEWABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEWABLE_BUTTON, ClayViewableButtonClass))


typedef struct _ClayViewableButtonClass ClayViewableButtonClass;

struct _ClayViewableButton
{
  ClayButton         parent_instance;

  ClayContainer     *container;
  ClayContext       *context;

  ClayViewType       popup_view_type;
  gint               popup_view_size;

  gint               button_view_size;
  gint               view_border_width;

  ClayDialogFactory *dialog_factory;
  gchar             *dialog_identifier;
  gchar             *dialog_icon_name;
  gchar             *dialog_tooltip;

  GtkWidget         *view;
};

struct _ClayViewableButtonClass
{
  ClayButtonClass  parent_class;
};


GType       clay_viewable_button_get_type (void);

GtkWidget * clay_viewable_button_new      (ClayContainer      *container,
                                           ClayContext        *context,
                                           ClayViewType        view_type,
                                           gint                button_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           ClayDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

ClayViewType clay_viewable_button_get_view_type (ClayViewableButton *button);
void         clay_viewable_button_set_view_type (ClayViewableButton *button,
                                                 ClayViewType        view_type);

gint         clay_viewable_button_get_view_size (ClayViewableButton *button);
void         clay_viewable_button_set_view_size (ClayViewableButton *button,
                                                 ClayViewSize        view_size);
