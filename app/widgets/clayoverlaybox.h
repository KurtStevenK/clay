/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayOverlayBox
 * Copyright (C) 2009 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_OVERLAY_BOX            (clay_overlay_box_get_type ())
#define CLAY_OVERLAY_BOX(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OVERLAY_BOX, ClayOverlayBox))
#define CLAY_OVERLAY_BOX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_OVERLAY_BOX, ClayOverlayBoxClass))
#define CLAY_IS_OVERLAY_BOX(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OVERLAY_BOX))
#define CLAY_IS_OVERLAY_BOX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_OVERLAY_BOX))
#define CLAY_OVERLAY_BOX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_OVERLAY_BOX, ClayOverlayBoxClass))


typedef struct _ClayOverlayBoxClass ClayOverlayBoxClass;

struct _ClayOverlayBox
{
  GtkContainer  parent_instance;

  GList        *children;
};

struct _ClayOverlayBoxClass
{
  GtkContainerClass  parent_class;
};


GType       clay_overlay_box_get_type            (void);

GtkWidget * clay_overlay_box_new                 (void);

void        clay_overlay_box_add_child           (ClayOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        clay_overlay_box_set_child_alignment (ClayOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         xalign,
                                                  gdouble         yalign);
void        clay_overlay_box_set_child_position  (ClayOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         x,
                                                  gdouble         y);
void        clay_overlay_box_set_child_angle     (ClayOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         angle);
void        clay_overlay_box_set_child_opacity   (ClayOverlayBox *box,
                                                  GtkWidget      *child,
                                                  gdouble         opacity);

void        clay_overlay_box_scroll              (ClayOverlayBox *box,
                                                  gint            offset_x,
                                                  gint            offset_y);
