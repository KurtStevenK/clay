/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoverlaychild.h
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


typedef struct _ClayOverlayChild ClayOverlayChild;

struct _ClayOverlayChild
{
  GtkWidget      *widget;
  GdkWindow      *window;

  gboolean        relative_to_shell;
  gboolean        has_position;
  gdouble         xalign;
  gdouble         yalign;
  gdouble         x;
  gdouble         y;

  gdouble         angle;
  gdouble         opacity;

  /* updated in size_allocate */
  cairo_matrix_t  matrix;
};


ClayOverlayChild * clay_overlay_child_new                  (ClayOverlayBox  *box,
                                                            GtkWidget       *widget,
                                                            gdouble          xalign,
                                                            gdouble          yalign,
                                                            gdouble          angle,
                                                            gdouble          opacity);
void               clay_overlay_child_free                 (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child);

ClayOverlayChild * clay_overlay_child_find                 (ClayOverlayBox   *box,
                                                            GtkWidget        *widget);

void               clay_overlay_child_realize              (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child);
void               clay_overlay_child_unrealize            (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child);
void               clay_overlay_child_get_preferred_width  (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child,
                                                            gint             *minimum,
                                                            gint             *natural);
void               clay_overlay_child_get_preferred_height (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child,
                                                            gint             *minimum,
                                                            gint             *natural);
void               clay_overlay_child_size_allocate        (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child);
gboolean           clay_overlay_child_draw                 (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child,
                                                            cairo_t          *cr);
gboolean           clay_overlay_child_damage               (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child,
                                                            GdkEventExpose   *event);

void               clay_overlay_child_invalidate           (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child);
gboolean           clay_overlay_child_pick                 (ClayOverlayBox   *box,
                                                            ClayOverlayChild *child,
                                                            gdouble           box_x,
                                                            gdouble           box_y);
void              clay_overlay_child_set_relative_to_shell (ClayOverlayChild *child,
                                                            gboolean          relative_to_shell);
