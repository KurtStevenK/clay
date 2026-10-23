/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerpopup.h
 * Copyright (C) 2003-2014 Michael Natterer <mitch@gimp.org>
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

#include "claypopup.h"


#define CLAY_TYPE_CONTAINER_POPUP            (clay_container_popup_get_type ())
#define CLAY_CONTAINER_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CONTAINER_POPUP, ClayContainerPopup))
#define CLAY_CONTAINER_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CONTAINER_POPUP, ClayContainerPopupClass))
#define CLAY_IS_CONTAINER_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CONTAINER_POPUP))
#define CLAY_IS_CONTAINER_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CONTAINER_POPUP))
#define CLAY_CONTAINER_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CONTAINER_POPUP, ClayContainerPopupClass))


typedef struct _ClayContainerPopupClass  ClayContainerPopupClass;

struct _ClayContainerPopup
{
  ClayPopup            parent_instance;

  ClayContainer       *container;
  ClayContext         *orig_context;
  ClayContext         *context;

  ClayViewType         view_type;
  gint                 default_view_size;
  gint                 view_size;
  gint                 view_border_width;

  GtkWidget           *frame;
  ClayContainerEditor *editor;

  ClayDialogFactory   *dialog_factory;
  gchar               *dialog_identifier;
  gchar               *dialog_icon_name;
  gchar               *dialog_tooltip;
};

struct _ClayContainerPopupClass
{
  ClayPopupClass  parent_class;
};


GType       clay_container_popup_get_type (void);

GtkWidget * clay_container_popup_new      (ClayContainer      *container,
                                           ClayContext        *context,
                                           ClayViewType        view_type,
                                           gint                default_view_size,
                                           gint                view_size,
                                           gint                view_border_width,
                                           ClayDialogFactory  *dialog_factory,
                                           const gchar        *dialog_identifier,
                                           const gchar        *dialog_icon_name,
                                           const gchar        *dialog_tooltip);

ClayViewType clay_container_popup_get_view_type (ClayContainerPopup *popup);
void         clay_container_popup_set_view_type (ClayContainerPopup *popup,
                                                 ClayViewType        view_type);

ClayViewSize clay_container_popup_get_view_size (ClayContainerPopup *popup);
void         clay_container_popup_set_view_size (ClayContainerPopup *popup,
                                                 ClayViewSize        view_size);
