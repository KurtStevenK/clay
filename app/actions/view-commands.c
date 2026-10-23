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

#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydisplayoptions.h"
#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claygrouplayer.h"

#include "widgets/clayactiongroup.h"
#include "widgets/claycolordialog.h"
#include "widgets/claydock.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claytoggleaction.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "display/claydisplay.h"
#include "display/claydisplay-foreach.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/claydisplayshell-filter-dialog.h"
#include "display/claydisplayshell-rotate.h"
#include "display/claydisplayshell-rotate-dialog.h"
#include "display/claydisplayshell-scale.h"
#include "display/claydisplayshell-scale-dialog.h"
#include "display/claydisplayshell-scroll.h"
#include "display/claydisplayshell-close.h"
#include "display/clayimagewindow.h"

#include "dialogs/color-profile-dialog.h"
#include "dialogs/dialogs.h"

#include "menus/menus.h"

#include "actions.h"
#include "view-commands.h"

#include "clay-intl.h"


#define SET_ACTIVE(manager,action_name,active) \
  { ClayActionGroup *group = \
      clay_ui_manager_get_action_group (manager, "view"); \
    clay_action_group_set_action_active (group, action_name, active); }

#define IS_ACTIVE_DISPLAY(display) \
  ((display) == \
   clay_context_get_display (clay_get_user_context ((display)->clay)))


/*  local function prototypes  */

static void   view_padding_color_dialog_update (ClayColorDialog          *dialog,
                                                GeglColor                *color,
                                                ClayColorDialogState      state,
                                                ClayDisplayShell         *shell);


/*  public functions  */

void
view_new_cmd_callback (ClayAction *action,
                       GVariant   *value,
                       gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  clay_create_display (display->clay,
                       clay_display_get_image (display),
                       shell->unit, clay_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (clay_widget_get_monitor (GTK_WIDGET (shell))));
}

void
view_close_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  ClayImage        *image;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);
  image = clay_display_get_image (display);

  /* Check for the image so we don't close the last display. */
  if (image)
    clay_display_shell_close (shell, FALSE);
}

void
view_scroll_center_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDisplay *display;
  return_if_no_display (display, data);

  clay_display_shell_scroll_center_image (clay_display_get_shell (display),
                                          TRUE, TRUE);
}

void
view_zoom_fit_in_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplay *display;
  return_if_no_display (display, data);

  clay_display_shell_scale_fit_in (clay_display_get_shell (display));
}

void
view_zoom_fill_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayDisplay *display;
  return_if_no_display (display, data);

  clay_display_shell_scale_fill (clay_display_get_shell (display));
}

void
view_zoom_selection_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  gint         x, y, width, height;
  return_if_no_display (display, data);
  return_if_no_image (image, data);

  clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                    &x, &y, &width, &height);

  clay_display_shell_scale_to_rectangle (clay_display_get_shell (display),
                                         CLAY_ZOOM_IN,
                                         x, y, width, height,
                                         FALSE);
}

void
view_zoom_revert_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplay *display;
  return_if_no_display (display, data);

  clay_display_shell_scale_revert (clay_display_get_shell (display));
}

void
view_zoom_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayDisplayShell     *shell;
  ClayActionSelectType  select_type;
  return_if_no_shell (shell, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  switch (select_type)
    {
    case CLAY_ACTION_SELECT_FIRST:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_OUT_MAX,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    case CLAY_ACTION_SELECT_LAST:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_IN_MAX,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    case CLAY_ACTION_SELECT_PREVIOUS:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_OUT,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    case CLAY_ACTION_SELECT_NEXT:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_IN,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    case CLAY_ACTION_SELECT_SKIP_PREVIOUS:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_OUT_MORE,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    case CLAY_ACTION_SELECT_SKIP_NEXT:
      clay_display_shell_scale (shell,
                                CLAY_ZOOM_IN_MORE,
                                0.0,
                                CLAY_ZOOM_FOCUS_BEST_GUESS);
      break;

    default:
      {
        gdouble scale = clay_zoom_model_get_factor (shell->zoom);

        scale = action_select_value (select_type,
                                     scale,
                                     0.0, 512.0, 1.0,
                                     1.0 / 8.0, 1.0, 16.0, 0.0,
                                     FALSE);

        /* min = 1.0 / 256,  max = 256.0                */
        /* scale = min *  (max / min)**(i/n), i = 0..n  */
        scale = pow (65536.0, scale / 512.0) / 256.0;

        clay_display_shell_scale (shell,
                                  CLAY_ZOOM_TO,
                                  scale,
                                  CLAY_ZOOM_FOCUS_BEST_GUESS);
        break;
      }
    }
}

void
view_zoom_explicit_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDisplayShell *shell;
  gint              factor;
  return_if_no_shell (shell, data);

  factor = g_variant_get_int32 (value);

  if (factor != 0 /* not Other... */)
    {
      if (fabs (factor - clay_zoom_model_get_factor (shell->zoom)) > 0.0001)
        clay_display_shell_scale (shell,
                                  CLAY_ZOOM_TO,
                                  (gdouble) factor / 10000,
                                  CLAY_ZOOM_FOCUS_RETAIN_CENTERING_ELSE_BEST_GUESS);
    }
  else
    {
      clay_display_shell_scale_dialog (shell);
    }
}

void
view_show_all_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->show_all)
    {
      ClayImageWindow *window = clay_display_shell_get_window (shell);

      clay_display_shell_set_show_all (shell, active);

      if (window)
        SET_ACTIVE (menus_get_image_manager_singleton (display->clay),
                    "view-show-all", shell->show_all);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-show-all",
                    shell->show_all);
    }
}

void
view_dot_for_dot_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->dot_for_dot)
    {
      ClayImageWindow *window = clay_display_shell_get_window (shell);

      clay_display_shell_scale_set_dot_for_dot (shell, active);

      if (window)
        SET_ACTIVE (menus_get_image_manager_singleton (display->clay),
                    "view-dot-for-dot", shell->dot_for_dot);

      if (IS_ACTIVE_DISPLAY (display))
        SET_ACTIVE (shell->popup_manager, "view-dot-for-dot",
                    shell->dot_for_dot);
    }
}

void
view_flip_horizontally_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_horizontally)
    {
      clay_display_shell_flip (shell, active, shell->flip_vertically);
    }
}

void
view_flip_vertically_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  active = g_variant_get_boolean (value);

  if (active != shell->flip_vertically)
    {
      clay_display_shell_flip (shell, shell->flip_horizontally, active);
    }
}

void
view_flip_reset_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;

  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);
  clay_display_shell_flip (shell, FALSE, FALSE);
}

void
view_rotate_absolute_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay          *display;
  ClayDisplayShell     *shell;
  ClayActionSelectType  select_type;
  gdouble               angle = 0.0;
  return_if_no_display (display, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  shell = clay_display_get_shell (display);

  angle = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  clay_display_shell_rotate_to (shell, angle);
}

void
view_rotate_relative_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay          *display;
  ClayDisplayShell     *shell;
  ClayActionSelectType  select_type;
  gdouble               delta = 0.0;
  return_if_no_display (display, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  shell = clay_display_get_shell (display);

  delta = action_select_value (select_type,
                               0.0,
                               -180.0, 180.0, 0.0,
                               1.0, 15.0, 90.0, 0.0,
                               TRUE);

  clay_display_shell_rotate (shell, delta);
}

void
view_rotate_other_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);

  clay_display_shell_rotate_dialog (shell);
}

void
view_reset_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;

  return_if_no_display (display, data);

  shell = clay_display_get_shell (display);
  clay_display_shell_rotate_to (shell, 0.0);
  clay_display_shell_flip (shell, FALSE, FALSE);
}

void
view_scroll_horizontal_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayDisplayShell     *shell;
  GtkAdjustment        *adj;
  ClayActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  adj = shell->hsbdata;

  offset = action_select_value (select_type,
                                gtk_adjustment_get_value (adj),
                                gtk_adjustment_get_lower (adj),
                                gtk_adjustment_get_upper (adj) -
                                gtk_adjustment_get_page_size (adj),
                                gtk_adjustment_get_lower (adj),
                                1,
                                gtk_adjustment_get_step_increment (adj),
                                gtk_adjustment_get_page_increment (adj),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->hsbdata, offset);
}

void
view_scroll_vertical_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplayShell     *shell;
  GtkAdjustment        *adj;
  ClayActionSelectType  select_type;
  gdouble               offset;
  return_if_no_shell (shell, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  adj = shell->vsbdata;

  offset = action_select_value (select_type,
                                gtk_adjustment_get_value (adj),
                                gtk_adjustment_get_lower (adj),
                                gtk_adjustment_get_upper (adj) -
                                gtk_adjustment_get_page_size (adj),
                                gtk_adjustment_get_lower (adj),
                                1,
                                gtk_adjustment_get_step_increment (adj),
                                gtk_adjustment_get_page_increment (adj),
                                0,
                                FALSE);

  gtk_adjustment_set_value (shell->vsbdata, offset);
}

void
view_navigation_window_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  Clay             *clay;
  ClayDisplayShell *shell;
  return_if_no_clay (clay, data);
  return_if_no_shell (shell, data);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                             clay,
                                             clay_dialog_factory_get_singleton (),
                                             clay_widget_get_monitor (GTK_WIDGET (shell)),
                                             "clay-navigation-view");
}

void
view_display_filters_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_shell (shell, data);

#define FILTERS_DIALOG_KEY "clay-display-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = clay_display_shell_filter_dialog_new (shell);

      dialogs_attach_dialog (G_OBJECT (shell), FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
view_color_management_reset_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayDisplayShell *shell;
  ClayColorConfig  *global_config;
  ClayColorConfig  *shell_config;
  return_if_no_shell (shell, data);

  global_config = CLAY_CORE_CONFIG (shell->display->config)->color_management;
  shell_config  = clay_display_shell_get_color_config (shell);

  clay_config_copy (CLAY_CONFIG (global_config),
                    CLAY_CONFIG (shell_config),
                    0);
  shell->color_config_set = FALSE;
}

void
view_color_management_enable_cmd_callback (ClayAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  ClayDisplayShell        *shell;
  ClayColorConfig         *color_config;
  ClayColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = clay_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = clay_color_config_get_mode (color_config);

  if (active)
    {
      if (mode != CLAY_COLOR_MANAGEMENT_SOFTPROOF)
        mode = CLAY_COLOR_MANAGEMENT_DISPLAY;
    }
  else
    {
      mode = CLAY_COLOR_MANAGEMENT_OFF;
    }

  if (mode != clay_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_color_management_softproof_cmd_callback (ClayAction *action,
                                              GVariant   *value,
                                              gpointer    data)
{
  ClayDisplayShell        *shell;
  ClayColorConfig         *color_config;
  ClayColorManagementMode  mode;
  gboolean                 active;
  return_if_no_shell (shell, data);

  color_config = clay_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  mode = clay_color_config_get_mode (color_config);

  if (active)
    {
      mode = CLAY_COLOR_MANAGEMENT_SOFTPROOF;
    }
  else
    {
      if (mode != CLAY_COLOR_MANAGEMENT_OFF)
        mode = CLAY_COLOR_MANAGEMENT_DISPLAY;
    }

  if (mode != clay_color_config_get_mode (color_config))
    {
      g_object_set (color_config,
                    "mode", mode,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_intent_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDisplayShell          *shell;
  ClayColorConfig           *color_config;
  ClayColorRenderingIntent   intent;
  return_if_no_shell (shell, data);

  intent = (ClayColorRenderingIntent) g_variant_get_int32 (value);

  color_config = clay_display_shell_get_color_config (shell);

  if (intent != clay_color_config_get_display_intent (color_config))
    {
      g_object_set (color_config,
                    "display-rendering-intent", intent,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_display_bpc_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplayShell *shell;
  ClayColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = clay_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != clay_color_config_get_display_bpc (color_config))
    {
      g_object_set (color_config,
                    "display-use-black-point-compensation", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_softproof_gamut_check_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayDisplayShell *shell;
  ClayColorConfig  *color_config;
  gboolean          active;
  return_if_no_shell (shell, data);

  color_config = clay_display_shell_get_color_config (shell);

  active = g_variant_get_boolean (value);

  if (active != clay_color_config_get_simulation_gamut_check (color_config))
    {
      g_object_set (color_config,
                    "simulation-gamut-check", active,
                    NULL);
      shell->color_config_set = TRUE;
    }
}

void
view_toggle_selection_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_selection (shell))
    {
      clay_display_shell_set_show_selection (shell, active);
    }
}

void
view_toggle_layer_boundary_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_layer (shell))
    {
      clay_display_shell_set_show_layer (shell, active);
    }
}

void
view_toggle_canvas_boundary_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_canvas (shell))
    {
      clay_display_shell_set_show_canvas (shell, active);
    }
}

void
view_toggle_menubar_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_menubar (shell))
    {
      clay_display_shell_set_show_menubar (shell, active);
    }
}

void
view_toggle_rulers_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_rulers (shell))
    {
      clay_display_shell_set_show_rulers (shell, active);
    }
}

void
view_toggle_scrollbars_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_scrollbars (shell))
    {
      clay_display_shell_set_show_scrollbars (shell, active);
    }
}

void
view_toggle_statusbar_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_statusbar (shell))
    {
      clay_display_shell_set_show_statusbar (shell, active);
    }
}

void
view_toggle_guides_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_guides (shell))
    {
      clay_display_shell_set_show_guides (shell, active);
    }
}

void
view_toggle_grid_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_grid (shell))
    {
      clay_display_shell_set_show_grid (shell, active);
    }
}

void
view_toggle_sample_points_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_show_sample_points (shell))
    {
      clay_display_shell_set_show_sample_points (shell, active);
    }
}

void
view_snap_to_guides_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_guides (shell))
    {
      clay_display_shell_set_snap_to_guides (shell, active);
    }
}

void
view_snap_to_grid_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_grid (shell))
    {
      clay_display_shell_set_snap_to_grid (shell, active);
    }
}

void
view_snap_to_canvas_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_canvas (shell))
    {
      clay_display_shell_set_snap_to_canvas (shell, active);
    }
}

void
view_snap_to_path_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_path (shell))
    {
      clay_display_shell_set_snap_to_path (shell, active);
    }
}

void
view_snap_to_bbox_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_bbox (shell))
    {
      clay_display_shell_set_snap_to_bbox (shell, active);
    }
}

void
view_snap_to_equidistance_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_snap_to_equidistance (shell))
    clay_display_shell_set_snap_to_equidistance (shell, active);
}

void
view_padding_color_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayDisplay           *display;
  ClayImageWindow       *window;
  ClayDisplayShell      *shell;
  ClayDisplayOptions    *options;
  ClayCanvasPaddingMode  padding_mode;
  gboolean               fullscreen;
  return_if_no_display (display, data);

  padding_mode = (ClayCanvasPaddingMode) g_variant_get_int32 (value);

  shell  = clay_display_get_shell (display);
  window = clay_display_shell_get_window (shell);

  if (window)
    fullscreen = clay_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

#define PADDING_COLOR_DIALOG_KEY "clay-padding-color-dialog"

  switch (padding_mode)
    {
    case CLAY_CANVAS_PADDING_MODE_DEFAULT:
    case CLAY_CANVAS_PADDING_MODE_LIGHT_CHECK:
    case CLAY_CANVAS_PADDING_MODE_DARK_CHECK:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      options->padding_mode_set = TRUE;

      clay_display_shell_set_padding (shell, padding_mode,
                                      options->padding_color);
      break;

    case CLAY_CANVAS_PADDING_MODE_CUSTOM:
      {
        GtkWidget             *dialog;
        GeglColor             *old_color;
        ClayCanvasPaddingMode  old_padding_mode;

        dialog = dialogs_get_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

        if (! dialog)
          {
            ClayImage        *image = clay_display_get_image (display);
            ClayDisplayShell *shell = clay_display_get_shell (display);

            dialog =
              clay_color_dialog_new (CLAY_VIEWABLE (image),
                                     action_data_get_context (data),
                                     FALSE,
                                     _("Set Canvas Padding Color"),
                                     CLAY_ICON_FONT,
                                     _("Set Custom Canvas Padding Color"),
                                     GTK_WIDGET (shell),
                                     NULL, NULL,
                                     options->padding_color,
                                     TRUE, FALSE);

            g_signal_connect (dialog, "update",
                              G_CALLBACK (view_padding_color_dialog_update),
                              shell);

            dialogs_attach_dialog (G_OBJECT (shell),
                                   PADDING_COLOR_DIALOG_KEY, dialog);
          }
        old_color        = gegl_color_duplicate (options->padding_color);
        old_padding_mode = options->padding_mode;
        g_object_set_data_full (G_OBJECT (dialog), "old-color",
                                old_color, g_object_unref);
        g_object_set_data (G_OBJECT (dialog), "old-padding-mode",
                           GINT_TO_POINTER (old_padding_mode));

        gtk_window_present (GTK_WINDOW (dialog));
      }
      break;

    case CLAY_CANVAS_PADDING_MODE_RESET:
      dialogs_destroy_dialog (G_OBJECT (shell), PADDING_COLOR_DIALOG_KEY);

      {
        ClayDisplayOptions *default_options;

        options->padding_mode_set = FALSE;

        if (fullscreen)
          default_options = display->config->default_fullscreen_view;
        else
          default_options = display->config->default_view;

        clay_display_shell_set_padding (shell,
                                        default_options->padding_mode,
                                        default_options->padding_color);
        clay_display_shell_set_padding_in_show_all (shell,
                                                    default_options->padding_in_show_all);
      }
      break;
    }
}

void
view_padding_color_in_show_all_cmd_callback (ClayAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  ClayDisplayShell *shell;
  gboolean          active;
  return_if_no_shell (shell, data);

  active = g_variant_get_boolean (value);

  if (active != clay_display_shell_get_padding_in_show_all (shell))
    {
      clay_display_shell_set_padding_in_show_all (shell, active);
    }
}

void
view_shrink_wrap_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplayShell *shell;
  return_if_no_shell (shell, data);

  clay_display_shell_scale_shrink_wrap (shell, FALSE);
}

void
view_fullscreen_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayDisplay      *display;
  ClayDisplayShell *shell;
  ClayImageWindow  *window;
  return_if_no_display (display, data);

  shell  = clay_display_get_shell (display);
  window = clay_display_shell_get_window (shell);

  if (window)
    {
      gboolean active = g_variant_get_boolean (value);

      clay_image_window_set_fullscreen (window, active);
    }
}


/*  private functions  */

static void
view_padding_color_dialog_update (ClayColorDialog      *dialog,
                                  GeglColor            *color,
                                  ClayColorDialogState  state,
                                  ClayDisplayShell     *shell)
{
  ClayImageWindow       *window;
  ClayDisplayOptions    *options;
  GeglColor             *old_color;
  ClayCanvasPaddingMode  old_padding_mode;
  gboolean               fullscreen;

  window           = clay_display_shell_get_window (shell);
  old_color        = g_object_get_data (G_OBJECT (dialog), "old-color");
  old_padding_mode = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "old-padding-mode"));

  g_return_if_fail (old_color);

  if (window)
    fullscreen = clay_image_window_get_fullscreen (window);
  else
    fullscreen = FALSE;

  if (fullscreen)
    options = shell->fullscreen_options;
  else
    options = shell->options;

  switch (state)
    {
    case CLAY_COLOR_DIALOG_OK:
      options->padding_mode_set = TRUE;

      clay_display_shell_set_padding (shell, CLAY_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case CLAY_COLOR_DIALOG_CANCEL:
      clay_display_shell_set_padding (shell,
                                      old_padding_mode,
                                      old_color);
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    case CLAY_COLOR_DIALOG_UPDATE:
      clay_display_shell_set_padding (shell, CLAY_CANVAS_PADDING_MODE_CUSTOM,
                                      color);
      break;

    default:
      break;
    }
}
