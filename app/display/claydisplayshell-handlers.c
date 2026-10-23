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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "config/claydisplayoptions.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-cairo.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-grid.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-quick-mask.h"
#include "core/clayimage-sample-points.h"
#include "core/clayitem.h"
#include "core/clayitemstack.h"
#include "core/claysamplepoint.h"
#include "core/claytreehandler.h"

#include "path/claypath.h"

#include "widgets/claywidgets-utils.h"

#include "claycanvascanvasboundary.h"
#include "claycanvasguide.h"
#include "claycanvaslayerboundary.h"
#include "claycanvaspath.h"
#include "claycanvasproxygroup.h"
#include "claycanvassamplepoint.h"
#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-appearance.h"
#include "claydisplayshell-callbacks.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-handlers.h"
#include "claydisplayshell-profile.h"
#include "claydisplayshell-render.h"
#include "claydisplayshell-rulers.h"
#include "claydisplayshell-scale.h"
#include "claydisplayshell-scroll.h"
#include "claydisplayshell-selection.h"
#include "claydisplayshell-title.h"
#include "clayimagewindow.h"
#include "claystatusbar.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_display_shell_clean_dirty_handler        (ClayImage        *image,
                                                             ClayDirtyMask     dirty_mask,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_undo_event_handler         (ClayImage        *image,
                                                             ClayUndoEvent     event,
                                                             ClayUndo         *undo,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_grid_notify_handler        (ClayGrid         *grid,
                                                             GParamSpec       *pspec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_name_changed_handler       (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_selection_invalidate_handler
                                                            (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_component_visibility_changed_handler
                                                            (ClayImage        *image,
                                                             ClayChannelType   channel,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_size_changed_detailed_handler
                                                            (ClayImage        *image,
                                                             gint              previous_origin_x,
                                                             gint              previous_origin_y,
                                                             gint              previous_width,
                                                             gint              previous_height,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_resolution_changed_handler (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_quick_mask_changed_handler (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_guide_add_handler          (ClayImage        *image,
                                                             ClayGuide        *guide,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_guide_remove_handler       (ClayImage        *image,
                                                             ClayGuide        *guide,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_guide_move_handler         (ClayImage        *image,
                                                             ClayGuide        *guide,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_sample_point_add_handler   (ClayImage        *image,
                                                             ClaySamplePoint  *sample_point,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_sample_point_remove_handler(ClayImage        *image,
                                                             ClaySamplePoint  *sample_point,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_sample_point_move_handler  (ClayImage        *image,
                                                             ClaySamplePoint  *sample_point,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_mode_changed_handler       (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_precision_changed_handler  (ClayImage        *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_profile_changed_handler    (ClayColorManaged *image,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_saved_handler              (ClayImage        *image,
                                                             GFile            *file,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_exported_handler           (ClayImage        *image,
                                                             GFile            *file,
                                                             ClayDisplayShell *shell);

static void   clay_display_shell_active_paths_handler       (ClayImage        *image,
                                                             ClayDisplayShell *shell);

static void   clay_display_shell_path_freeze_handler        (ClayPath         *path,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_path_thaw_handler          (ClayPath         *path,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_path_visible_handler       (ClayPath         *path,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_path_add_handler           (ClayContainer    *container,
                                                             ClayPath         *path,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_path_remove_handler        (ClayContainer    *container,
                                                             ClayPath         *path,
                                                             ClayDisplayShell *shell);

static void   clay_display_shell_check_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_title_notify_handler       (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_nav_size_notify_handler    (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_monitor_res_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_padding_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_ants_speed_notify_handler  (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void   clay_display_shell_quality_notify_handler     (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void  clay_display_shell_color_config_notify_handler (GObject          *config,
                                                             GParamSpec       *param_spec,
                                                             ClayDisplayShell *shell);
static void  clay_display_shell_display_changed_handler     (ClayContext      *context,
                                                             ClayDisplay      *display,
                                                             ClayDisplayShell *shell);


/*  public functions  */

void
clay_display_shell_connect (ClayDisplayShell *shell)
{
  ClayImage         *image;
  ClayContainer     *paths;
  ClayDisplayConfig *config;
  ClayColorConfig   *color_config;
  ClayContext       *user_context;
  GList             *list;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_DISPLAY (shell->display));

  image = clay_display_get_image (shell->display);

  g_return_if_fail (CLAY_IS_IMAGE (image));

  paths = clay_image_get_paths (image);

  config       = shell->display->config;
  color_config = CLAY_CORE_CONFIG (config)->color_management;

  user_context = clay_get_user_context (shell->display->clay);

  g_signal_connect (image, "clean",
                    G_CALLBACK (clay_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "dirty",
                    G_CALLBACK (clay_display_shell_clean_dirty_handler),
                    shell);
  g_signal_connect (image, "undo-event",
                    G_CALLBACK (clay_display_shell_undo_event_handler),
                    shell);

  g_signal_connect (clay_image_get_grid (image), "notify",
                    G_CALLBACK (clay_display_shell_grid_notify_handler),
                    shell);
  g_object_set (shell->grid, "grid", clay_image_get_grid (image), NULL);

  g_signal_connect (image, "name-changed",
                    G_CALLBACK (clay_display_shell_name_changed_handler),
                    shell);
  g_signal_connect (image, "selection-invalidate",
                    G_CALLBACK (clay_display_shell_selection_invalidate_handler),
                    shell);
  g_signal_connect (image, "component-visibility-changed",
                    G_CALLBACK (clay_display_shell_component_visibility_changed_handler),
                    shell);
  g_signal_connect (image, "size-changed-detailed",
                    G_CALLBACK (clay_display_shell_size_changed_detailed_handler),
                    shell);
  g_signal_connect (image, "resolution-changed",
                    G_CALLBACK (clay_display_shell_resolution_changed_handler),
                    shell);
  g_signal_connect (image, "quick-mask-changed",
                    G_CALLBACK (clay_display_shell_quick_mask_changed_handler),
                    shell);

  g_signal_connect (image, "guide-added",
                    G_CALLBACK (clay_display_shell_guide_add_handler),
                    shell);
  g_signal_connect (image, "guide-removed",
                    G_CALLBACK (clay_display_shell_guide_remove_handler),
                    shell);
  g_signal_connect (image, "guide-moved",
                    G_CALLBACK (clay_display_shell_guide_move_handler),
                    shell);
  for (list = clay_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      clay_display_shell_guide_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "sample-point-added",
                    G_CALLBACK (clay_display_shell_sample_point_add_handler),
                    shell);
  g_signal_connect (image, "sample-point-removed",
                    G_CALLBACK (clay_display_shell_sample_point_remove_handler),
                    shell);
  g_signal_connect (image, "sample-point-moved",
                    G_CALLBACK (clay_display_shell_sample_point_move_handler),
                    shell);
  for (list = clay_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      clay_display_shell_sample_point_add_handler (image, list->data, shell);
    }

  g_signal_connect (image, "mode-changed",
                    G_CALLBACK (clay_display_shell_mode_changed_handler),
                    shell);
  g_signal_connect (image, "precision-changed",
                    G_CALLBACK (clay_display_shell_precision_changed_handler),
                    shell);
  g_signal_connect (image, "profile-changed",
                    G_CALLBACK (clay_display_shell_profile_changed_handler),
                    shell);
  g_signal_connect_swapped (image, "simulation-profile-changed",
                            G_CALLBACK (clay_display_shell_profile_update),
                            shell);
  g_signal_connect_swapped (image, "simulation-intent-changed",
                            G_CALLBACK (clay_display_shell_profile_update),
                            shell);
  g_signal_connect_swapped (image, "simulation-bpc-changed",
                            G_CALLBACK (clay_display_shell_profile_update),
                            shell);

  g_signal_connect (image, "saved",
                    G_CALLBACK (clay_display_shell_saved_handler),
                    shell);
  g_signal_connect (image, "exported",
                    G_CALLBACK (clay_display_shell_exported_handler),
                    shell);

  g_signal_connect (image, "selected-paths-changed",
                    G_CALLBACK (clay_display_shell_active_paths_handler),
                    shell);

  shell->path_freeze_handler =
    clay_tree_handler_connect (paths, "freeze",
                               G_CALLBACK (clay_display_shell_path_freeze_handler),
                               shell);
  shell->path_thaw_handler =
    clay_tree_handler_connect (paths, "thaw",
                               G_CALLBACK (clay_display_shell_path_thaw_handler),
                               shell);
  shell->path_visible_handler =
    clay_tree_handler_connect (paths, "visibility-changed",
                               G_CALLBACK (clay_display_shell_path_visible_handler),
                               shell);

  g_signal_connect (paths, "add",
                    G_CALLBACK (clay_display_shell_path_add_handler),
                    shell);
  g_signal_connect (paths, "remove",
                    G_CALLBACK (clay_display_shell_path_remove_handler),
                    shell);
  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (paths));
       list;
       list = g_list_next (list))
    {
      clay_display_shell_path_add_handler (paths, list->data, shell);
    }

  g_signal_connect (config,
                    "notify::transparency-size",
                    G_CALLBACK (clay_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-type",
                    G_CALLBACK (clay_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-custom-color1",
                    G_CALLBACK (clay_display_shell_check_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::transparency-custom-color2",
                    G_CALLBACK (clay_display_shell_check_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::image-title-format",
                    G_CALLBACK (clay_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::image-status-format",
                    G_CALLBACK (clay_display_shell_title_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::navigation-preview-size",
                    G_CALLBACK (clay_display_shell_nav_size_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-resolution-from-windowing-system",
                    G_CALLBACK (clay_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-xresolution",
                    G_CALLBACK (clay_display_shell_monitor_res_notify_handler),
                    shell);
  g_signal_connect (config,
                    "notify::monitor-yresolution",
                    G_CALLBACK (clay_display_shell_monitor_res_notify_handler),
                    shell);

  g_signal_connect (config->default_view,
                    "notify::padding-mode",
                    G_CALLBACK (clay_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_view,
                    "notify::padding-color",
                    G_CALLBACK (clay_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-mode",
                    G_CALLBACK (clay_display_shell_padding_notify_handler),
                    shell);
  g_signal_connect (config->default_fullscreen_view,
                    "notify::padding-color",
                    G_CALLBACK (clay_display_shell_padding_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::marching-ants-speed",
                    G_CALLBACK (clay_display_shell_ants_speed_notify_handler),
                    shell);

  g_signal_connect (config,
                    "notify::zoom-quality",
                    G_CALLBACK (clay_display_shell_quality_notify_handler),
                    shell);

  g_signal_connect (color_config, "notify",
                    G_CALLBACK (clay_display_shell_color_config_notify_handler),
                    shell);

  g_signal_connect (user_context, "display-changed",
                    G_CALLBACK (clay_display_shell_display_changed_handler),
                    shell);

  clay_display_shell_active_paths_handler     (image, shell);
  clay_display_shell_quick_mask_changed_handler (image, shell);
  clay_display_shell_profile_changed_handler    (CLAY_COLOR_MANAGED (image),
                                                 shell);
  clay_display_shell_color_config_notify_handler (G_OBJECT (color_config),
                                                  NULL, /* sync all */
                                                  shell);

  clay_canvas_layer_boundary_set_layers (CLAY_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         clay_image_get_selected_layers (image));

  clay_canvas_canvas_boundary_set_image (CLAY_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         image);

  if (shell->show_all)
    {
      clay_image_inc_show_all_count (image);

      clay_image_flush (image);
    }
}

void
clay_display_shell_disconnect (ClayDisplayShell *shell)
{
  ClayImage         *image;
  ClayContainer     *paths;
  ClayDisplayConfig *config;
  ClayColorConfig   *color_config;
  ClayContext       *user_context;
  GList             *list;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (CLAY_IS_DISPLAY (shell->display));

  image = clay_display_get_image (shell->display);

  g_return_if_fail (CLAY_IS_IMAGE (image));

  paths = clay_image_get_paths (image);

  config       = shell->display->config;
  color_config = CLAY_CORE_CONFIG (config)->color_management;

  user_context = clay_get_user_context (shell->display->clay);

  clay_canvas_layer_boundary_set_layers (CLAY_CANVAS_LAYER_BOUNDARY (shell->layer_boundary),
                                         NULL);

  clay_canvas_canvas_boundary_set_image (CLAY_CANVAS_CANVAS_BOUNDARY (shell->canvas_boundary),
                                         NULL);

  g_signal_handlers_disconnect_by_func (user_context,
                                        clay_display_shell_display_changed_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (color_config,
                                        clay_display_shell_color_config_notify_handler,
                                        shell);
  shell->color_config_set = FALSE;

  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_quality_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_ants_speed_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_fullscreen_view,
                                        clay_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config->default_view,
                                        clay_display_shell_padding_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_monitor_res_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_nav_size_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_title_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (config,
                                        clay_display_shell_check_notify_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (paths,
                                        clay_display_shell_path_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (paths,
                                        clay_display_shell_path_add_handler,
                                        shell);

  clay_tree_handler_disconnect (shell->path_visible_handler);
  shell->path_visible_handler = NULL;

  clay_tree_handler_disconnect (shell->path_thaw_handler);
  shell->path_thaw_handler = NULL;

  clay_tree_handler_disconnect (shell->path_freeze_handler);
  shell->path_freeze_handler = NULL;

  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_active_paths_handler,
                                        shell);

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (paths));
       list;
       list = g_list_next (list))
    {
      clay_canvas_proxy_group_remove_item (CLAY_CANVAS_PROXY_GROUP (shell->paths),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_exported_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_saved_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_profile_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_profile_update,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_precision_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_mode_changed_handler,
                                        shell);

  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_guide_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_guide_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_guide_move_handler,
                                        shell);
  for (list = clay_image_get_guides (image);
       list;
       list = g_list_next (list))
    {
      clay_canvas_proxy_group_remove_item (CLAY_CANVAS_PROXY_GROUP (shell->guides),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_sample_point_add_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_sample_point_remove_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_sample_point_move_handler,
                                        shell);
  for (list = clay_image_get_sample_points (image);
       list;
       list = g_list_next (list))
    {
      clay_canvas_proxy_group_remove_item (CLAY_CANVAS_PROXY_GROUP (shell->sample_points),
                                           list->data);
    }

  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_quick_mask_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_resolution_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_component_visibility_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_size_changed_detailed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_selection_invalidate_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_name_changed_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (clay_image_get_grid (image),
                                        clay_display_shell_grid_notify_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_undo_event_handler,
                                        shell);
  g_signal_handlers_disconnect_by_func (image,
                                        clay_display_shell_clean_dirty_handler,
                                        shell);

  if (shell->show_all)
    {
      clay_image_dec_show_all_count (image);

      clay_image_flush (image);
    }
}


/*  private functions  */

static void
clay_display_shell_clean_dirty_handler (ClayImage        *image,
                                        ClayDirtyMask     dirty_mask,
                                        ClayDisplayShell *shell)
{
  clay_display_shell_title_update (shell);
}

static void
clay_display_shell_undo_event_handler (ClayImage        *image,
                                       ClayUndoEvent     event,
                                       ClayUndo         *undo,
                                       ClayDisplayShell *shell)
{
  clay_display_shell_title_update (shell);
}

static void
clay_display_shell_grid_notify_handler (ClayGrid         *grid,
                                        GParamSpec       *pspec,
                                        ClayDisplayShell *shell)
{
  g_object_set (shell->grid, "grid", grid, NULL);
}

static void
clay_display_shell_name_changed_handler (ClayImage        *image,
                                         ClayDisplayShell *shell)
{
  clay_display_shell_title_update (shell);
}

static void
clay_display_shell_selection_invalidate_handler (ClayImage        *image,
                                                 ClayDisplayShell *shell)
{
  clay_display_shell_selection_undraw (shell);
}

static void
clay_display_shell_resolution_changed_handler (ClayImage        *image,
                                               ClayDisplayShell *shell)
{
  clay_display_shell_scale_update (shell);

  if (shell->dot_for_dot)
    {
      if (shell->unit != clay_unit_pixel ())
        {
          clay_display_shell_rulers_update (shell);
        }

      clay_display_shell_scaled (shell);
    }
  else
    {
      /* A resolution change has the same effect as a size change from
       * a display shell point of view. Force a redraw of the display
       * so that we don't get any display garbage.
       */

      ClayDisplayConfig *config = shell->display->config;
      gboolean           resize_window;

      /* Resize windows only in multi-window mode */
      resize_window = (config->resize_windows_on_resize &&
                       ! CLAY_GUI_CONFIG (config)->single_window_mode);

      clay_display_shell_scale_resize (shell, resize_window, FALSE);
    }
}

static void
clay_display_shell_quick_mask_changed_handler (ClayImage        *image,
                                               ClayDisplayShell *shell)
{
  GtkImage *gtk_image;
  gboolean  quick_mask_state;

  gtk_image = GTK_IMAGE (gtk_bin_get_child (GTK_BIN (shell->quick_mask_button)));

  g_signal_handlers_block_by_func (shell->quick_mask_button,
                                   clay_display_shell_quick_mask_toggled,
                                   shell);

  quick_mask_state = clay_image_get_quick_mask_state (image);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (shell->quick_mask_button),
                                quick_mask_state);

  if (quick_mask_state)
    gtk_image_set_from_icon_name (gtk_image, CLAY_ICON_QUICK_MASK_ON,
                                  GTK_ICON_SIZE_MENU);
  else
    gtk_image_set_from_icon_name (gtk_image, CLAY_ICON_QUICK_MASK_OFF,
                                  GTK_ICON_SIZE_MENU);

  g_signal_handlers_unblock_by_func (shell->quick_mask_button,
                                     clay_display_shell_quick_mask_toggled,
                                     shell);
}

static void
clay_display_shell_guide_add_handler (ClayImage        *image,
                                      ClayGuide        *guide,
                                      ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->guides);
  ClayCanvasItem       *item;
  ClayGuideStyle        style;

  style = clay_guide_get_style (guide);
  item = clay_canvas_guide_new (shell,
                                clay_guide_get_orientation (guide),
                                clay_guide_get_position (guide),
                                style);

  clay_canvas_proxy_group_add_item (group, guide, item);
  g_object_unref (item);
}

static void
clay_display_shell_guide_remove_handler (ClayImage        *image,
                                         ClayGuide        *guide,
                                         ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->guides);

  clay_canvas_proxy_group_remove_item (group, guide);
}

static void
clay_display_shell_guide_move_handler (ClayImage        *image,
                                       ClayGuide        *guide,
                                       ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->guides);
  ClayCanvasItem       *item;

  item = clay_canvas_proxy_group_get_item (group, guide);

  clay_canvas_guide_set (item,
                         clay_guide_get_orientation (guide),
                         clay_guide_get_position (guide));
}

static void
clay_display_shell_sample_point_add_handler (ClayImage        *image,
                                             ClaySamplePoint  *sample_point,
                                             ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->sample_points);
  ClayCanvasItem       *item;
  GList                *list;
  gint                  x;
  gint                  y;
  gint                  i;

  clay_sample_point_get_position (sample_point, &x, &y);

  item = clay_canvas_sample_point_new (shell, x, y, 0, TRUE);

  clay_canvas_proxy_group_add_item (group, sample_point, item);
  g_object_unref (item);

  for (list = clay_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      ClaySamplePoint *sample_point = list->data;

      item = clay_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
clay_display_shell_sample_point_remove_handler (ClayImage        *image,
                                                ClaySamplePoint  *sample_point,
                                                ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->sample_points);
  GList                *list;
  gint                  i;

  clay_canvas_proxy_group_remove_item (group, sample_point);

  for (list = clay_image_get_sample_points (image), i = 1;
       list;
       list = g_list_next (list), i++)
    {
      ClaySamplePoint *sample_point = list->data;
      ClayCanvasItem  *item;

      item = clay_canvas_proxy_group_get_item (group, sample_point);

      if (item)
        g_object_set (item,
                      "index", i,
                      NULL);
    }
}

static void
clay_display_shell_sample_point_move_handler (ClayImage        *image,
                                              ClaySamplePoint  *sample_point,
                                              ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->sample_points);
  ClayCanvasItem       *item;
  gint                  x;
  gint                  y;

  item = clay_canvas_proxy_group_get_item (group, sample_point);

  clay_sample_point_get_position (sample_point, &x, &y);

  clay_canvas_sample_point_set (item, x, y);
}

static void
clay_display_shell_component_visibility_changed_handler (ClayImage        *image,
                                                         ClayChannelType   channel,
                                                         ClayDisplayShell *shell)
{
  if (channel == CLAY_CHANNEL_ALPHA && shell->show_all)
    clay_display_shell_expose_full (shell);
}

static void
clay_display_shell_size_changed_detailed_handler (ClayImage        *image,
                                                  gint              previous_origin_x,
                                                  gint              previous_origin_y,
                                                  gint              previous_width,
                                                  gint              previous_height,
                                                  ClayDisplayShell *shell)
{
  ClayDisplayConfig *config = shell->display->config;
  gboolean           resize_window;

  /* Resize windows only in multi-window mode */
  resize_window = (config->resize_windows_on_resize &&
                   ! CLAY_GUI_CONFIG (config)->single_window_mode);

  if (resize_window)
    {
      ClayImageWindow *window = clay_display_shell_get_window (shell);

      if (window && clay_image_window_get_active_shell (window) == shell)
        {
          /* If the window is resized just center the image in it when it
           * has change size
           */
          clay_image_window_shrink_wrap (window, FALSE);
        }
    }
  else
    {
      ClayImage *image      = clay_display_get_image (shell->display);
      gint       new_width  = clay_image_get_width  (image);
      gint       new_height = clay_image_get_height (image);
      gint       scaled_previous_origin_x;
      gint       scaled_previous_origin_y;
      gboolean   horizontally;
      gboolean   vertically;

      scaled_previous_origin_x = SCALEX (shell, previous_origin_x);
      scaled_previous_origin_y = SCALEY (shell, previous_origin_y);

      horizontally = (SCALEX (shell, previous_width)  >  shell->disp_width  &&
                      SCALEX (shell, new_width)       <= shell->disp_width);
      vertically   = (SCALEY (shell, previous_height) >  shell->disp_height &&
                      SCALEY (shell, new_height)      <= shell->disp_height);

      clay_display_shell_scroll_set_offset (shell,
                                            shell->offset_x + scaled_previous_origin_x,
                                            shell->offset_y + scaled_previous_origin_y);

      if (! clay_display_shell_get_infinite_canvas (shell))
        {
          clay_display_shell_scroll_center_image (shell,
                                                  horizontally, vertically);
        }

      /* The above calls might not lead to a call to
       * clay_display_shell_scroll_clamp_and_update() and
       * clay_display_shell_expose_full() in all cases because when
       * scaling the old and new scroll offset might be the same.
       *
       * We need them to be called in all cases, so simply call them
       * explicitly here at the end
       */
      clay_display_shell_scroll_clamp_and_update (shell);

      clay_display_shell_expose_full (shell);
      clay_display_shell_render_invalidate_full (shell);
    }
}

static void
clay_display_shell_mode_changed_handler (ClayImage        *image,
                                         ClayDisplayShell *shell)
{
  clay_display_shell_profile_update (shell);
}

static void
clay_display_shell_precision_changed_handler (ClayImage        *image,
                                              ClayDisplayShell *shell)
{
  clay_display_shell_profile_update (shell);
}

static void
clay_display_shell_profile_changed_handler (ClayColorManaged *image,
                                            ClayDisplayShell *shell)
{
  clay_color_managed_profile_changed (CLAY_COLOR_MANAGED (shell));
}

static void
clay_display_shell_saved_handler (ClayImage        *image,
                                  GFile            *file,
                                  ClayDisplayShell *shell)
{
  ClayStatusbar *statusbar = clay_display_shell_get_statusbar (shell);

  clay_statusbar_push_temp (statusbar, CLAY_MESSAGE_INFO,
                            CLAY_ICON_DOCUMENT_SAVE,
                            _("Image saved to '%s'"),
                            clay_file_get_utf8_name (file));
}

static void
clay_display_shell_exported_handler (ClayImage        *image,
                                     GFile            *file,
                                     ClayDisplayShell *shell)
{
  ClayStatusbar *statusbar = clay_display_shell_get_statusbar (shell);

  clay_statusbar_push_temp (statusbar, CLAY_MESSAGE_INFO,
                            CLAY_ICON_DOCUMENT_SAVE,
                            _("Image exported to '%s'"),
                            clay_file_get_utf8_name (file));
}

static void
clay_display_shell_active_paths_handler (ClayImage        *image,
                                         ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group    = CLAY_CANVAS_PROXY_GROUP (shell->paths);
  GList                *selected = clay_image_get_selected_paths (image);
  GList                *list;

  for (list = clay_image_get_path_iter (image);
       list;
       list = g_list_next (list))
    {
      ClayPath       *path = list->data;
      ClayCanvasItem *item;

      item = clay_canvas_proxy_group_get_item (group, path);

      clay_canvas_item_set_highlight (item,
                                      g_list_find (selected, path) != NULL);
    }
}

static void
clay_display_shell_path_freeze_handler (ClayPath         *path,
                                        ClayDisplayShell *shell)
{
  /* do nothing */
}

static void
clay_display_shell_path_thaw_handler (ClayPath         *path,
                                      ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->paths);
  ClayCanvasItem       *item;

  item = clay_canvas_proxy_group_get_item (group, path);

  clay_canvas_path_set (item, clay_path_get_bezier (path));
}

static void
clay_display_shell_path_visible_handler (ClayPath         *path,
                                         ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->paths);
  ClayCanvasItem       *item;

  item = clay_canvas_proxy_group_get_item (group, path);

  clay_canvas_item_set_visible (item,
                                clay_item_get_visible (CLAY_ITEM (path)));
}

static void
clay_display_shell_path_add_handler (ClayContainer    *container,
                                     ClayPath         *path,
                                     ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->paths);
  ClayCanvasItem       *item;

  item = clay_canvas_path_new (shell,
                               clay_path_get_bezier (path),
                               0, 0,
                               FALSE,
                               CLAY_PATH_STYLE_PATH);
  clay_canvas_item_set_visible (item,
                                clay_item_get_visible (CLAY_ITEM (path)));

  clay_canvas_proxy_group_add_item (group, path, item);
  g_object_unref (item);
}

static void
clay_display_shell_path_remove_handler (ClayContainer    *container,
                                        ClayPath         *path,
                                        ClayDisplayShell *shell)
{
  ClayCanvasProxyGroup *group = CLAY_CANVAS_PROXY_GROUP (shell->paths);

  clay_canvas_proxy_group_remove_item (group, path);
}

static void
clay_display_shell_check_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         ClayDisplayShell *shell)
{
  ClayCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;

  g_clear_pointer (&shell->checkerboard, cairo_pattern_destroy);

  clay_display_shell_get_padding (shell, &padding_mode, &padding_color);

  switch (padding_mode)
    {
    case CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case CLAY_CANVAS_PADDING_MODE_DARK_CHECK:
      clay_display_shell_set_padding (shell, padding_mode, padding_color);
      break;

    default:
      break;
    }

  clay_display_shell_expose_full (shell);

  g_object_unref (padding_color);
}

static void
clay_display_shell_title_notify_handler (GObject          *config,
                                         GParamSpec       *param_spec,
                                         ClayDisplayShell *shell)
{
  clay_display_shell_title_update (shell);
}

static void
clay_display_shell_nav_size_notify_handler (GObject          *config,
                                            GParamSpec       *param_spec,
                                            ClayDisplayShell *shell)
{
  g_clear_pointer (&shell->nav_popup, gtk_widget_destroy);
}

static void
clay_display_shell_monitor_res_notify_handler (GObject          *config,
                                               GParamSpec       *param_spec,
                                               ClayDisplayShell *shell)
{
  if (CLAY_DISPLAY_CONFIG (config)->monitor_res_from_gdk)
    {
      clay_get_monitor_resolution (clay_widget_get_monitor (GTK_WIDGET (shell)),
                                   &shell->monitor_xres,
                                   &shell->monitor_yres);
    }
  else
    {
      shell->monitor_xres = CLAY_DISPLAY_CONFIG (config)->monitor_xres;
      shell->monitor_yres = CLAY_DISPLAY_CONFIG (config)->monitor_yres;
    }

  clay_display_shell_scale_update (shell);

  if (! shell->dot_for_dot)
    {
      clay_display_shell_scroll_clamp_and_update (shell);

      clay_display_shell_scaled (shell);

      clay_display_shell_expose_full (shell);
      clay_display_shell_render_invalidate_full (shell);
    }
}

static void
clay_display_shell_padding_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           ClayDisplayShell *shell)
{
  ClayDisplayConfig     *display_config;
  ClayImageWindow       *window;
  gboolean               fullscreen;
  ClayCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;

  display_config = shell->display->config;

  window = clay_display_shell_get_window (shell);

  if (window)
    fullscreen = clay_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  /*  if the user did not set the padding mode for this display explicitly  */
  if (! shell->fullscreen_options->padding_mode_set)
    {
      padding_mode  = display_config->default_fullscreen_view->padding_mode;
      padding_color = display_config->default_fullscreen_view->padding_color;

      if (fullscreen)
        {
          clay_display_shell_set_padding (shell, padding_mode, padding_color);
        }
      else
        {
          shell->fullscreen_options->padding_mode  = padding_mode;
          g_clear_object (&shell->fullscreen_options->padding_color);
          shell->fullscreen_options->padding_color = gegl_color_duplicate (padding_color);
        }
    }

  /*  if the user did not set the padding mode for this display explicitly  */
  if (! shell->options->padding_mode_set)
    {
      padding_mode  = display_config->default_view->padding_mode;
      padding_color = display_config->default_view->padding_color;

      if (fullscreen)
        {
          shell->options->padding_mode  = padding_mode;
          g_clear_object (&shell->options->padding_color);
          shell->options->padding_color = gegl_color_duplicate (padding_color);
        }
      else
        {
          clay_display_shell_set_padding (shell, padding_mode, padding_color);
        }
    }
}

static void
clay_display_shell_ants_speed_notify_handler (GObject          *config,
                                              GParamSpec       *param_spec,
                                              ClayDisplayShell *shell)
{
  clay_display_shell_selection_pause (shell);
  clay_display_shell_selection_resume (shell);
}

static void
clay_display_shell_quality_notify_handler (GObject          *config,
                                           GParamSpec       *param_spec,
                                           ClayDisplayShell *shell)
{
  clay_display_shell_expose_full (shell);
  clay_display_shell_render_invalidate_full (shell);
}

static void
clay_display_shell_color_config_notify_handler (GObject          *config,
                                                GParamSpec       *param_spec,
                                                ClayDisplayShell *shell)
{
  if (param_spec)
    {
      gboolean copy = TRUE;

      if (! strcmp (param_spec->name, "mode")                                 ||
          ! strcmp (param_spec->name, "display-rendering-intent")             ||
          ! strcmp (param_spec->name, "display-use-black-point-compensation") ||
          ! strcmp (param_spec->name, "simulation-gamut-check"))
        {
          if (shell->color_config_set)
            copy = FALSE;
        }

      if (copy)
        {
          GValue value = G_VALUE_INIT;

          g_value_init (&value, param_spec->value_type);

          g_object_get_property (config,
                                 param_spec->name, &value);
          g_object_set_property (G_OBJECT (shell->color_config),
                                 param_spec->name, &value);

          g_value_unset (&value);
        }
    }
  else
    {
      clay_config_copy (CLAY_CONFIG (config),
                        CLAY_CONFIG (shell->color_config),
                        0);
      shell->color_config_set = FALSE;
    }
}

static void
clay_display_shell_display_changed_handler (ClayContext      *context,
                                            ClayDisplay      *display,
                                            ClayDisplayShell *shell)
{
  if (shell->display == display)
    clay_display_shell_update_priority_rect (shell);
}
