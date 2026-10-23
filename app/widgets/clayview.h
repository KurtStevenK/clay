/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayview.h
 * Copyright (C) 2001-2006 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_VIEW            (clay_view_get_type ())
#define CLAY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW, ClayView))
#define CLAY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW, ClayViewClass))
#define CLAY_IS_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW))
#define CLAY_IS_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW))
#define CLAY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW, ClayViewClass))


typedef struct _ClayViewClass  ClayViewClass;

struct _ClayView
{
  GtkWidget         parent_instance;

  GdkWindow        *event_window;

  ClayViewable     *viewable;
  ClayViewRenderer *renderer;

  guint             clickable : 1;
  guint             eat_button_events : 1;
  guint             show_popup : 1;
  guint             expand : 1;

  /*< private >*/
  guint             in_button : 1;
  guint             has_grab : 1;
  GdkModifierType   press_state;
};

struct _ClayViewClass
{
  GtkWidgetClass  parent_class;

  /*  signals  */
  void        (* set_viewable)   (ClayView        *view,
                                  ClayViewable    *old_viewable,
                                  ClayViewable    *new_viewable);
  void        (* clicked)        (ClayView        *view,
                                  GdkModifierType  modifier_state);
  void        (* double_clicked) (ClayView        *view);
  void        (* context)        (ClayView        *view);
};


GType          clay_view_get_type          (void);

GtkWidget    * clay_view_new               (ClayContext   *context,
                                            ClayViewable  *viewable,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * clay_view_new_full          (ClayContext   *context,
                                            ClayViewable  *viewable,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);
GtkWidget    * clay_view_new_by_types      (ClayContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           size,
                                            gint           border_width,
                                            gboolean       is_popup);
GtkWidget    * clay_view_new_full_by_types (ClayContext   *context,
                                            GType          view_type,
                                            GType          viewable_type,
                                            gint           width,
                                            gint           height,
                                            gint           border_width,
                                            gboolean       is_popup,
                                            gboolean       clickable,
                                            gboolean       show_popup);

ClayViewable * clay_view_get_viewable      (ClayView      *view);
void           clay_view_set_viewable      (ClayView      *view,
                                            ClayViewable  *viewable);
void           clay_view_set_expand        (ClayView      *view,
                                            gboolean       expand);
