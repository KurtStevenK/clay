/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "widgets/claywindow.h"


#define CLAY_TYPE_IMAGE_WINDOW (clay_image_window_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayImageWindow,
                          clay_image_window,
                          CLAY, IMAGE_WINDOW,
                          ClayWindow)


struct _ClayImageWindowClass
{
  ClayWindowClass  parent_class;
};


ClayImageWindow  * clay_image_window_new                  (Clay              *clay,
                                                           ClayImage         *image,
                                                           ClayDialogFactory *dialog_factory,
                                                           GdkMonitor        *monitor);
void               clay_image_window_destroy              (ClayImageWindow   *window);

ClayDockColumns  * clay_image_window_get_left_docks       (ClayImageWindow  *window);
ClayDockColumns  * clay_image_window_get_right_docks      (ClayImageWindow  *window);

void               clay_image_window_add_shell            (ClayImageWindow  *window,
                                                           ClayDisplayShell *shell);
ClayDisplayShell * clay_image_window_get_shell            (ClayImageWindow  *window,
                                                           gint              index);
void               clay_image_window_remove_shell         (ClayImageWindow  *window,
                                                           ClayDisplayShell *shell);

gint               clay_image_window_get_n_shells         (ClayImageWindow  *window);

void               clay_image_window_set_active_shell     (ClayImageWindow  *window,
                                                           ClayDisplayShell *shell);
ClayDisplayShell * clay_image_window_get_active_shell     (ClayImageWindow  *window);

ClayMenuModel    * clay_image_window_get_menubar_model    (ClayImageWindow  *window);

void               clay_image_window_set_fullscreen       (ClayImageWindow  *window,
                                                           gboolean          fullscreen);
gboolean           clay_image_window_get_fullscreen       (ClayImageWindow  *window);

void               clay_image_window_set_show_menubar     (ClayImageWindow  *window,
                                                           gboolean          show);
gboolean           clay_image_window_get_show_menubar     (ClayImageWindow  *window);

void               clay_image_window_set_show_statusbar   (ClayImageWindow  *window,
                                                           gboolean          show);
gboolean           clay_image_window_get_show_statusbar   (ClayImageWindow  *window);

gboolean           clay_image_window_is_iconified         (ClayImageWindow  *window);
gboolean           clay_image_window_is_maximized         (ClayImageWindow  *window);

gboolean           clay_image_window_has_toolbox          (ClayImageWindow  *window);

void               clay_image_window_shrink_wrap          (ClayImageWindow  *window,
                                                           gboolean          grow_only);

GtkWidget        * clay_image_window_get_default_dockbook (ClayImageWindow  *window);

void               clay_image_window_keep_canvas_pos      (ClayImageWindow  *window);
void               clay_image_window_suspend_keep_pos     (ClayImageWindow  *window);
void               clay_image_window_resume_keep_pos      (ClayImageWindow  *window);

void               clay_image_window_update_tabs          (ClayImageWindow  *window);
