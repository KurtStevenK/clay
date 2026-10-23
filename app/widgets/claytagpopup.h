/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytagpopup.h
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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


#define CLAY_TYPE_TAG_POPUP            (clay_tag_popup_get_type ())
#define CLAY_TAG_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TAG_POPUP, ClayTagPopup))
#define CLAY_TAG_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TAG_POPUP, ClayTagPopupClass))
#define CLAY_IS_TAG_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TAG_POPUP))
#define CLAY_IS_TAG_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TAG_POPUP))
#define CLAY_TAG_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TAG_POPUP, ClayTagPopupClass))


typedef struct _ClayTagPopupClass  ClayTagPopupClass;

typedef struct _PopupTagData PopupTagData;

struct _ClayTagPopup
{
  GtkWindow          parent_instance;

  ClayComboTagEntry *combo_entry;

  GtkWidget         *frame;
  GtkWidget         *border_area;
  GtkWidget         *tag_area;

  PangoLayout       *layout;

  PopupTagData      *tag_data;
  gint               tag_count;

  PopupTagData      *prelight;

  gboolean           single_select_disabled;

  guint              scroll_timeout_id;
  gint               scroll_height;
  gint               scroll_y;
  gint               scroll_step;
  gint               scroll_arrow_height;
  gboolean           scroll_fast;
  gboolean           arrows_visible;
  gboolean           upper_arrow_prelight;
  gboolean           lower_arrow_prelight;
  GtkStateFlags      upper_arrow_state;
  GtkStateFlags      lower_arrow_state;

  gboolean           smooth_scrolling;
};

struct _ClayTagPopupClass
{
  GtkWindowClass  parent_class;
};


GType       clay_tag_popup_get_type (void);

GtkWidget * clay_tag_popup_new      (ClayComboTagEntry *entry);

void        clay_tag_popup_show     (ClayTagPopup      *popup,
                                     GdkEvent          *event);
