/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claywindow.h
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


#define CLAY_TYPE_WINDOW            (clay_window_get_type ())
#define CLAY_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_WINDOW, ClayWindow))
#define CLAY_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_WINDOW, ClayWindowClass))
#define CLAY_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_WINDOW))
#define CLAY_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_WINDOW))
#define CLAY_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_WINDOW, ClayWindowClass))


typedef struct _ClayWindowClass   ClayWindowClass;
typedef struct _ClayWindowPrivate ClayWindowPrivate;

struct _ClayWindow
{
  GtkApplicationWindow  parent_instance;

  ClayWindowPrivate    *private;
};

struct _ClayWindowClass
{
  GtkApplicationWindowClass  parent_class;

  void (* monitor_changed) (ClayWindow *window,
                            GdkMonitor *monitor);
};


GType       clay_window_get_type                 (void);

void        clay_window_set_primary_focus_widget (ClayWindow *window,
                                                  GtkWidget  *primary_focus);
GtkWidget * clay_window_get_primary_focus_widget (ClayWindow *window);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayWindow, g_object_unref);
