/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypickablepopup.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PICKABLE_POPUP            (clay_pickable_popup_get_type ())
#define CLAY_PICKABLE_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PICKABLE_POPUP, ClayPickablePopup))
#define CLAY_PICKABLE_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PICKABLE_POPUP, ClayPickablePopupClass))
#define CLAY_IS_PICKABLE_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PICKABLE_POPUP))
#define CLAY_IS_PICKABLE_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PICKABLE_POPUP))
#define CLAY_PICKABLE_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PICKABLE_POPUP, ClayPickablePopupClass))


typedef struct _ClayPickablePopupPrivate ClayPickablePopupPrivate;
typedef struct _ClayPickablePopupClass   ClayPickablePopupClass;

struct _ClayPickablePopup
{
  ClayPopup                 parent_instance;

  ClayPickablePopupPrivate *priv;
};

struct _ClayPickablePopupClass
{
  ClayPopupClass  parent_class;
};


GType          clay_pickable_popup_get_type     (void);

GtkWidget    * clay_pickable_popup_new          (ClayContext       *context,
                                                 gint               view_size,
                                                 gint               view_border_width);

ClayPickable * clay_pickable_popup_get_pickable (ClayPickablePopup *popup);
