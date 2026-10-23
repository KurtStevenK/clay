/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayitemchooser.h
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


#define CLAY_TYPE_ITEM_CHOOSER            (clay_item_chooser_get_type ())
#define CLAY_ITEM_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ITEM_CHOOSER, ClayItemChooser))
#define CLAY_ITEM_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ITEM_CHOOSER, ClayItemChooserClass))
#define CLAY_IS_ITEM_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ITEM_CHOOSER))
#define CLAY_IS_ITEM_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ITEM_CHOOSER))
#define CLAY_ITEM_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ITEM_CHOOSER, ClayItemChooserClass))


typedef struct _ClayItemChooserPrivate ClayItemChooserPrivate;
typedef struct _ClayItemChooserClass   ClayItemChooserClass;

struct _ClayItemChooser
{
  GtkFrame                    parent_instance;

  ClayItemChooserPrivate *priv;
};

struct _ClayItemChooserClass
{
  GtkFrameClass  parent_class;

  /* Signals. */

  void (* activate) (ClayItemChooser *view,
                     ClayItem        *item);
};


GType          clay_item_chooser_get_type     (void);

GtkWidget    * clay_item_chooser_new          (ClayContext     *context,
                                               GType            item_type,
                                               gint             view_size,
                                               gint             view_border_width);

ClayItem *     clay_item_chooser_get_item     (ClayItemChooser *chooser);
void           clay_item_chooser_set_item     (ClayItemChooser *chooser,
                                               ClayItem        *item);
