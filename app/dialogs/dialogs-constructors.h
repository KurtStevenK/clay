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


/*  toplevel dialogs  */

GtkWidget * dialogs_image_new_new               (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_file_open_new               (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_file_open_location_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_file_save_new               (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_file_export_new             (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_preferences_get             (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_extensions_get              (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_input_devices_get           (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_keyboard_shortcuts_get      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_module_get                  (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_palette_import_get          (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_tips_get                    (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_welcome_get                 (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_about_get                   (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_action_search_get           (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_error_get                   (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_critical_get                (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_close_all_get               (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_quit_get                    (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);


/*  docks  */

GtkWidget * dialogs_toolbox_new                 (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_toolbox_dock_window_new     (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dock_new                    (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dock_window_new             (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);


/*  dockables  */

GtkWidget * dialogs_tool_options_new            (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_device_status_new           (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_error_console_new           (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_cursor_view_new             (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dashboard_new               (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_image_list_view_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_brush_list_view_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dynamics_list_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_mypaint_brush_list_view_new (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_pattern_list_view_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_gradient_list_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_palette_list_view_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_font_list_view_new          (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_buffer_list_view_new        (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_tool_preset_list_view_new   (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_document_list_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_template_list_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_image_grid_view_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_brush_grid_view_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dynamics_grid_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_mypaint_brush_grid_view_new (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_pattern_grid_view_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_gradient_grid_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_palette_grid_view_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_font_grid_view_new          (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_buffer_grid_view_new        (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_tool_preset_grid_view_new   (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_document_grid_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_template_grid_view_new      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_layer_list_view_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_channel_list_view_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_path_list_view_new          (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_colormap_editor_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_histogram_editor_new        (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_selection_editor_new        (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_symmetry_editor_new         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_undo_editor_new             (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_sample_point_editor_new     (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_navigation_editor_new       (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_color_editor_new            (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);

GtkWidget * dialogs_brush_editor_get            (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_dynamics_editor_get         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_gradient_editor_get         (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_palette_editor_get          (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
GtkWidget * dialogs_tool_preset_editor_get      (ClayDialogFactory *factory,
                                                 ClayContext       *context,
                                                 ClayUIManager     *ui_manager,
                                                 gint               view_size);
