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

#include "display-types.h"

#include "config/claydisplayoptions.h"

#include "core/clayimage.h"

#include "widgets/claydockcolumns.h"
#include "widgets/claymenumodel.h"
#include "widgets/clayrender.h"
#include "widgets/claywidgets-utils.h"

#include "claycanvas.h"
#include "claycanvasitem.h"
#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-actions.h"
#include "claydisplayshell-appearance.h"
#include "claydisplayshell-expose.h"
#include "claydisplayshell-selection.h"
#include "claydisplayshell-scroll.h"
#include "claydisplayshell-scrollbars.h"
#include "clayimagewindow.h"
#include "claystatusbar.h"


/*  local function prototypes  */

static ClayDisplayOptions * appearance_get_options (ClayDisplayShell *shell);


/*  public functions  */

void
clay_display_shell_appearance_update (ClayDisplayShell *shell)
{
  ClayDisplayOptions *options;
  ClayImageWindow    *window;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = clay_display_shell_get_window (shell);

  if (window)
    {
      gboolean fullscreen = clay_image_window_get_fullscreen (window);

      clay_display_shell_set_action_active (shell, "view-fullscreen",
                                            fullscreen);
    }

  clay_display_shell_set_show_menubar        (shell,
                                              options->show_menubar);
  clay_display_shell_set_show_statusbar      (shell,
                                              options->show_statusbar);

  clay_display_shell_set_show_rulers         (shell,
                                              options->show_rulers);
  clay_display_shell_set_show_scrollbars     (shell,
                                              options->show_scrollbars);
  clay_display_shell_set_show_selection      (shell,
                                              options->show_selection);
  clay_display_shell_set_show_layer          (shell,
                                              options->show_layer_boundary);
  clay_display_shell_set_show_canvas         (shell,
                                              options->show_canvas_boundary);
  clay_display_shell_set_show_guides         (shell,
                                              options->show_guides);
  clay_display_shell_set_show_grid           (shell,
                                              options->show_grid);
  clay_display_shell_set_show_sample_points  (shell,
                                              options->show_sample_points);
  clay_display_shell_set_padding             (shell,
                                              options->padding_mode,
                                              options->padding_color);
  clay_display_shell_set_padding_in_show_all (shell,
                                              options->padding_in_show_all);
}

void
clay_display_shell_set_show_menubar (ClayDisplayShell *shell,
                                     gboolean          show)
{
  ClayDisplayOptions *options;
  ClayImageWindow    *window;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);
  window  = clay_display_shell_get_window (shell);

  g_object_set (options, "show-menubar", show, NULL);

  if (window && clay_image_window_get_active_shell (window) == shell)
    {
      clay_image_window_keep_canvas_pos (clay_display_shell_get_window (shell));
      clay_image_window_set_show_menubar (window, show);
    }

  clay_display_shell_set_action_active (shell, "view-show-menubar", show);
}

gboolean
clay_display_shell_get_show_menubar (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_menubar;
}

void
clay_display_shell_set_show_statusbar (ClayDisplayShell *shell,
                                       gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-statusbar", show, NULL);

  clay_image_window_keep_canvas_pos (clay_display_shell_get_window (shell));
  clay_statusbar_set_visible (CLAY_STATUSBAR (shell->statusbar), show);

  clay_display_shell_set_action_active (shell, "view-show-statusbar", show);
}

gboolean
clay_display_shell_get_show_statusbar (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_statusbar;
}

void
clay_display_shell_set_show_rulers (ClayDisplayShell *shell,
                                    gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-rulers", show, NULL);

  clay_image_window_keep_canvas_pos (clay_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->origin, show);
  gtk_widget_set_visible (shell->hrule, show);
  gtk_widget_set_visible (shell->vrule, show);
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  clay_display_shell_set_action_active (shell, "view-show-rulers", show);
}

gboolean
clay_display_shell_get_show_rulers (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_rulers;
}

void
clay_display_shell_set_show_scrollbars (ClayDisplayShell *shell,
                                        gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-scrollbars", show, NULL);

  clay_image_window_keep_canvas_pos (clay_display_shell_get_window (shell));
  gtk_widget_set_visible (shell->nav_ebox, show);
  gtk_widget_set_visible (shell->hsb, show);
  gtk_widget_set_visible (shell->vsb, show);
  gtk_widget_set_visible (shell->quick_mask_button, show);
  gtk_widget_set_visible (shell->zoom_button, show);

  clay_display_shell_set_action_active (shell, "view-show-scrollbars", show);
}

gboolean
clay_display_shell_get_show_scrollbars (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_scrollbars;
}

void
clay_display_shell_set_show_selection (ClayDisplayShell *shell,
                                       gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-selection", show, NULL);

  clay_display_shell_selection_set_show (shell, show);

  clay_display_shell_set_action_active (shell, "view-show-selection", show);
}

gboolean
clay_display_shell_get_show_selection (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_selection;
}

void
clay_display_shell_set_show_layer (ClayDisplayShell *shell,
                                   gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-layer-boundary", show, NULL);

  clay_canvas_item_set_visible (shell->layer_boundary, show);

  clay_display_shell_set_action_active (shell, "view-show-layer-boundary", show);
}

gboolean
clay_display_shell_get_show_layer (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_layer_boundary;
}

void
clay_display_shell_set_show_canvas (ClayDisplayShell *shell,
                                    gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-canvas-boundary", show, NULL);

  clay_canvas_item_set_visible (shell->canvas_boundary,
                                show && shell->show_all);

  clay_display_shell_set_action_active (shell, "view-show-canvas-boundary", show);
}

gboolean
clay_display_shell_get_show_canvas (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_canvas_boundary;
}

void
clay_display_shell_update_show_canvas (ClayDisplayShell *shell)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  clay_canvas_item_set_visible (shell->canvas_boundary,
                                options->show_canvas_boundary &&
                                shell->show_all);
}

void
clay_display_shell_set_show_guides (ClayDisplayShell *shell,
                                    gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-guides", show, NULL);

  clay_canvas_item_set_visible (shell->guides, show);

  clay_display_shell_set_action_active (shell, "view-show-guides", show);
}

gboolean
clay_display_shell_get_show_guides (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_guides;
}

void
clay_display_shell_set_show_grid (ClayDisplayShell *shell,
                                  gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-grid", show, NULL);

  clay_canvas_item_set_visible (shell->grid, show);

  clay_display_shell_set_action_active (shell, "view-show-grid", show);
}

gboolean
clay_display_shell_get_show_grid (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_grid;
}

void
clay_display_shell_set_show_sample_points (ClayDisplayShell *shell,
                                           gboolean          show)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "show-sample-points", show, NULL);

  clay_canvas_item_set_visible (shell->sample_points, show);

  clay_display_shell_set_action_active (shell, "view-show-sample-points", show);
}

gboolean
clay_display_shell_get_show_sample_points (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->show_sample_points;
}

void
clay_display_shell_set_snap_to_grid (ClayDisplayShell *shell,
                                     gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-grid", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_grid (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_grid;
}

void
clay_display_shell_set_snap_to_guides (ClayDisplayShell *shell,
                                       gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-guides", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_guides (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_guides;
}

void
clay_display_shell_set_snap_to_canvas (ClayDisplayShell *shell,
                                       gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-canvas", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_canvas (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_canvas;
}

void
clay_display_shell_set_snap_to_path (ClayDisplayShell *shell,
                                     gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-path", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_path (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_path;
}

void
clay_display_shell_set_snap_to_bbox (ClayDisplayShell *shell,
                                     gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-bbox", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_bbox (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_bbox;
}

void
clay_display_shell_set_snap_to_equidistance (ClayDisplayShell *shell,
                                             gboolean          snap)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  g_object_set (options, "snap-to-equidistance", snap, NULL);
}

gboolean
clay_display_shell_get_snap_to_equidistance (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->snap_to_equidistance;
}

void
clay_display_shell_set_padding (ClayDisplayShell      *shell,
                                ClayCanvasPaddingMode  padding_mode,
                                GeglColor             *padding_color)
{
  ClayImageWindow    *window;
  ClayMenuModel      *model;
  ClayDisplayOptions *options;
  GeglColor          *color;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (GEGL_IS_COLOR (padding_color));

  options = appearance_get_options (shell);
  color   = padding_color;

  switch (padding_mode)
    {
    case CLAY_CANVAS_PADDING_MODE_DEFAULT:
      break;

    case CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK:
      color = GEGL_COLOR (clay_render_check_color1 ());
      break;

    case CLAY_CANVAS_PADDING_MODE_DARK_CHECK:
      color = GEGL_COLOR (clay_render_check_color2 ());
      break;

    case CLAY_CANVAS_PADDING_MODE_CUSTOM:
    case CLAY_CANVAS_PADDING_MODE_RESET:
      break;
    }

  color = gegl_color_duplicate (color);
  g_object_set (options,
                "padding-mode",  padding_mode,
                "padding-color", color,
                NULL);

  clay_canvas_set_padding (CLAY_CANVAS (shell->canvas),
                           padding_mode, color);

  window = clay_display_shell_get_window (shell);
  model  = clay_image_window_get_menubar_model (window);

  if (padding_mode != CLAY_CANVAS_PADDING_MODE_DEFAULT)
    clay_menu_model_set_color (model, "/View/Padding color", options->padding_color);
  else
    clay_menu_model_set_color (model, "/View/Padding color", NULL);

  g_object_unref (color);
}

void
clay_display_shell_get_padding (ClayDisplayShell       *shell,
                                ClayCanvasPaddingMode  *padding_mode,
                                GeglColor             **padding_color)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (padding_mode)
    *padding_mode = options->padding_mode;

  if (padding_color)
    *padding_color = gegl_color_duplicate (options->padding_color);
}

void
clay_display_shell_set_padding_in_show_all (ClayDisplayShell *shell,
                                            gboolean          keep)
{
  ClayDisplayOptions *options;

  g_return_if_fail (CLAY_IS_DISPLAY_SHELL (shell));

  options = appearance_get_options (shell);

  if (options->padding_in_show_all != keep)
    {
      g_object_set (options, "padding-in-show-all", keep, NULL);

      if (shell->display)
        {
          clay_display_shell_scroll_clamp_and_update (shell);
          clay_display_shell_scrollbars_update (shell);

          clay_display_shell_expose_full (shell);
        }

      clay_display_shell_set_action_active (shell,
                                            "view-padding-color-in-show-all",
                                            keep);

      g_object_notify (G_OBJECT (shell), "infinite-canvas");
    }
}

gboolean
clay_display_shell_get_padding_in_show_all (ClayDisplayShell *shell)
{
  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), FALSE);

  return appearance_get_options (shell)->padding_in_show_all;
}


/*  private functions  */

static ClayDisplayOptions *
appearance_get_options (ClayDisplayShell *shell)
{
  if (clay_display_get_image (shell->display))
    {
      ClayImageWindow *window = clay_display_shell_get_window (shell);

      if (window && clay_image_window_get_fullscreen (window))
        return shell->fullscreen_options;
      else
        return shell->options;
    }

  return shell->no_image_options;
}
