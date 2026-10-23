/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysearchpopup.h
 * Copyright (C) 2015 Jehan <jehan at girinstud.io>
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


#define CLAY_TYPE_SEARCH_POPUP            (clay_search_popup_get_type ())
#define CLAY_SEARCH_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SEARCH_POPUP, ClaySearchPopup))
#define CLAY_SEARCH_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SEARCH_POPUP, ClaySearchPopupClass))
#define CLAY_IS_SEARCH_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SEARCH_POPUP))
#define CLAY_IS_SEARCH_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SEARCH_POPUP))
#define CLAY_SEARCH_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SEARCH_POPUP, ClaySearchPopupClass))

/**
 * ClaySearchPopupCallback:
 * @popup:  the #ClaySearchPopup to operate on.
 * @search: the text searched.
 *
 * Callback used by @popup to fill in its result list.
 * It should make use of clay_search_popup_add_result() to fill in
 * results.
 */
typedef struct _ClaySearchPopup           ClaySearchPopup;
typedef struct _ClaySearchPopupClass      ClaySearchPopupClass;
typedef struct _ClaySearchPopupPrivate    ClaySearchPopupPrivate;

typedef void   (*ClaySearchPopupCallback) (ClaySearchPopup  *popup,
                                           const gchar      *search,
                                           gpointer          data);

struct _ClaySearchPopup
{
  ClayPopup               parent_instance;

  ClaySearchPopupPrivate *priv;
};

struct _ClaySearchPopupClass
{
  ClayPopupClass          parent_class;
};

GType       clay_search_popup_get_type   (void);

GtkWidget * clay_search_popup_new        (Clay                    *clay,
                                          const gchar             *role,
                                          const gchar             *title,
                                          ClaySearchPopupCallback  callback,
                                          gpointer                 callback_data);

void        clay_search_popup_add_result (ClaySearchPopup *popup,
                                          ClayAction      *action,
                                          gint             section);
