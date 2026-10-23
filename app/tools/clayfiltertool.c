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

/* This file contains a base class for tools that implement on canvas
 * preview for non destructive editing. The processing of the pixels can
 * be done either by a gegl op or by a C function (apply_func).
 *
 * For the core side of this, please see app/core/claydrawablefilter.c.
 */

#include "config.h"

#include <gegl.h>
#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "operations/clay-operation-config.h"
#include "operations/clayoperationsettings.h"

#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claydrawable.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/claygrouplayer.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-pick-color.h"
#include "core/clayimage-undo-push.h"
#include "core/claylayer.h"
#include "core/claylist.h"
#include "core/claypickable.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"
#include "core/claysettings.h"
#include "core/claytoolinfo.h"

#include "widgets/claylayermodebox.h"
#include "widgets/claypropwidgets.h"
#include "widgets/claysettingsbox.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/claydisplayshell-transform.h"
#include "display/claytoolgui.h"
#include "display/claytoolwidget.h"

#include "clayfilteroptions.h"
#include "clayfiltertool.h"
#include "clayfiltertool-settings.h"
#include "clayfiltertool-widgets.h"
#include "clayguidetool.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"
#include "tool_manager.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void      clay_filter_tool_finalize       (GObject             *object);

static gboolean  clay_filter_tool_initialize     (ClayTool            *tool,
                                                  ClayDisplay         *display,
                                                  GError             **error);
static void      clay_filter_tool_control        (ClayTool            *tool,
                                                  ClayToolAction       action,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_button_press   (ClayTool            *tool,
                                                  const ClayCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  ClayButtonPressType  press_type,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_button_release (ClayTool            *tool,
                                                  const ClayCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  ClayButtonReleaseType release_type,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_motion         (ClayTool            *tool,
                                                  const ClayCoords    *coords,
                                                  guint32              time,
                                                  GdkModifierType      state,
                                                  ClayDisplay         *display);
static gboolean  clay_filter_tool_key_press      (ClayTool            *tool,
                                                  GdkEventKey         *kevent,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_oper_update    (ClayTool            *tool,
                                                  const ClayCoords    *coords,
                                                  GdkModifierType      state,
                                                  gboolean             proximity,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_cursor_update  (ClayTool            *tool,
                                                  const ClayCoords    *coords,
                                                  GdkModifierType      state,
                                                  ClayDisplay         *display);
static void      clay_filter_tool_options_notify (ClayTool            *tool,
                                                  ClayToolOptions     *options,
                                                  const GParamSpec    *pspec);

static gboolean  clay_filter_tool_can_pick_color (ClayColorTool       *color_tool,
                                                  const ClayCoords    *coords,
                                                  ClayDisplay         *display);
static gboolean  clay_filter_tool_pick_color     (ClayColorTool       *color_tool,
                                                  const ClayCoords    *coords,
                                                  ClayDisplay         *display,
                                                  const Babl         **sample_format,
                                                  gpointer             pixel,
                                                  GeglColor          **color);
static void      clay_filter_tool_color_picked   (ClayColorTool       *color_tool,
                                                  const ClayCoords    *coords,
                                                  ClayDisplay         *display,
                                                  ClayColorPickState   pick_state,
                                                  const Babl          *sample_format,
                                                  gpointer             pixel,
                                                  GeglColor           *color);

static void      clay_filter_tool_real_reset     (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_real_set_config(ClayFilterTool      *filter_tool,
                                                  ClayConfig          *config);
static void      clay_filter_tool_real_config_notify
                                                 (ClayFilterTool      *filter_tool,
                                                  ClayConfig          *config,
                                                  const GParamSpec    *pspec);

static void      clay_filter_tool_halt           (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_commit         (ClayFilterTool      *filter_tool,
                                                  gboolean             non_destructive);

static void      clay_filter_tool_dialog         (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_reset          (ClayFilterTool      *filter_tool);

static void      clay_filter_tool_create_filter  (ClayFilterTool      *filter_tool);

static void      clay_filter_tool_update_dialog  (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_update_dialog_operation_settings
                                                 (ClayFilterTool      *filter_tool);


static void      clay_filter_tool_region_changed (ClayFilterTool      *filter_tool);

static void      clay_filter_tool_flush          (ClayDrawableFilter  *filter,
                                                  ClayFilterTool      *filter_tool);
static void      clay_filter_tool_config_notify  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  ClayFilterTool      *filter_tool);
static void      clay_filter_tool_unset_setting  (GObject             *object,
                                                  const GParamSpec    *pspec,
                                                  ClayFilterTool      *filter_tool);
static void      clay_filter_tool_lock_position_changed
                                                 (ClayDrawable        *drawable,
                                                  ClayFilterTool      *filter_tool);
static void      clay_filter_tool_mask_changed   (ClayImage           *image,
                                                  ClayFilterTool      *filter_tool);

static void      clay_filter_tool_add_guide      (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_remove_guide   (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_move_guide     (ClayFilterTool      *filter_tool);
static void      clay_filter_tool_guide_removed  (ClayGuide           *guide,
                                                  ClayFilterTool      *filter_tool);
static void      clay_filter_tool_guide_moved    (ClayGuide           *guide,
                                                  const GParamSpec    *pspec,
                                                  ClayFilterTool      *filter_tool);

static void      clay_filter_tool_response       (ClayToolGui         *gui,
                                                  gint                 response_id,
                                                  ClayFilterTool      *filter_tool);

static void      clay_filter_tool_update_filter  (ClayFilterTool      *filter_tool);

static void    clay_filter_tool_set_has_settings (ClayFilterTool      *filter_tool,
                                                  gboolean             has_settings);


G_DEFINE_TYPE (ClayFilterTool, clay_filter_tool, CLAY_TYPE_COLOR_TOOL)

#define parent_class clay_filter_tool_parent_class


static void
clay_filter_tool_class_init (ClayFilterToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayColorToolClass *color_tool_class = CLAY_COLOR_TOOL_CLASS (klass);

  object_class->finalize     = clay_filter_tool_finalize;

  tool_class->initialize     = clay_filter_tool_initialize;
  tool_class->control        = clay_filter_tool_control;
  tool_class->button_press   = clay_filter_tool_button_press;
  tool_class->button_release = clay_filter_tool_button_release;
  tool_class->motion         = clay_filter_tool_motion;
  tool_class->key_press      = clay_filter_tool_key_press;
  tool_class->oper_update    = clay_filter_tool_oper_update;
  tool_class->cursor_update  = clay_filter_tool_cursor_update;
  tool_class->options_notify = clay_filter_tool_options_notify;
  tool_class->is_destructive = FALSE;

  color_tool_class->can_pick = clay_filter_tool_can_pick_color;
  color_tool_class->pick     = clay_filter_tool_pick_color;
  color_tool_class->picked   = clay_filter_tool_color_picked;

  klass->get_operation       = NULL;
  klass->dialog              = NULL;
  klass->reset               = clay_filter_tool_real_reset;
  klass->set_config          = clay_filter_tool_real_set_config;
  klass->config_notify       = clay_filter_tool_real_config_notify;
  klass->settings_import     = clay_filter_tool_real_settings_import;
  klass->settings_export     = clay_filter_tool_real_settings_export;
}

static void
clay_filter_tool_init (ClayFilterTool *filter_tool)
{
  ClayTool *tool = CLAY_TOOL (filter_tool);

  clay_tool_control_set_scroll_lock      (tool->control, TRUE);
  clay_tool_control_set_preserve         (tool->control, FALSE);
  clay_tool_control_set_dirty_mask       (tool->control,
                                          CLAY_DIRTY_IMAGE           |
                                          CLAY_DIRTY_IMAGE_STRUCTURE |
                                          CLAY_DIRTY_DRAWABLE        |
                                          CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_active_modifiers (tool->control,
                                          CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
}

static void
clay_filter_tool_finalize (GObject *object)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (object);

  g_clear_object (&filter_tool->operation);
  g_clear_object (&filter_tool->config);
  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);
  g_clear_pointer (&filter_tool->description, g_free);
  g_clear_object (&filter_tool->gui);
  filter_tool->settings_box      = NULL;
  filter_tool->controller_toggle = NULL;
  filter_tool->clip_combo        = NULL;
  filter_tool->region_combo      = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define RESPONSE_RESET 1

static gboolean
clay_filter_tool_initialize (ClayTool     *tool,
                             ClayDisplay  *display,
                             GError      **error)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayToolInfo     *tool_info   = tool->tool_info;
  ClayGuiConfig    *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage        *image       = clay_display_get_image (display);
  ClayDisplayShell *shell       = clay_display_get_shell (display);
  ClayItem         *locked_item = NULL;
  GList            *drawables   = clay_image_get_selected_drawables (image);
  ClayDrawable     *drawable    = NULL;

  if (filter_tool->existing_filter)
    {
      drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
    }
  else
    {
      if (g_list_length (drawables) != 1)
        {
          if (g_list_length (drawables) > 1)
            g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                 _("Cannot modify multiple drawables. "
                                   "Select only one."));
          else
            g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                                 _("No selected drawables."));

          g_list_free (drawables);
          return FALSE;
        }
      drawable = drawables->data;
    }

  if (clay_item_is_content_locked (CLAY_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("A selected item's pixels are locked."));
      if (error)
        clay_tools_blink_lock_box (display->clay, locked_item);

      g_list_free (drawables);
      return FALSE;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("A selected layer is not visible."));

      g_list_free (drawables);
      return FALSE;
    }

  clay_filter_tool_get_operation (filter_tool, NULL);

  clay_filter_tool_disable_color_picking (filter_tool);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  if (filter_tool->config && ! filter_tool->existing_filter)
    clay_config_reset (CLAY_CONFIG (filter_tool->config));

  if (! filter_tool->gui)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *toggle;

      /*  disabled for at least Clay 2.8  */
      filter_tool->overlay = FALSE;

      filter_tool->gui =
        clay_tool_gui_new (tool_info,
                           clay_tool_get_label (tool),
                           filter_tool->description,
                           clay_tool_get_icon_name (tool),
                           clay_tool_get_help_id (tool),
                           clay_widget_get_monitor (GTK_WIDGET (shell)),
                           filter_tool->overlay,

                           _("_Reset"),  RESPONSE_RESET,
                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                           _("_OK"),     GTK_RESPONSE_OK,

                           NULL);

      clay_tool_gui_set_default_response (filter_tool->gui, GTK_RESPONSE_OK);

      clay_tool_gui_set_alternative_button_order (filter_tool->gui,
                                                  RESPONSE_RESET,
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

      vbox = clay_tool_gui_get_vbox (filter_tool->gui);

      g_signal_connect_object (filter_tool->gui, "response",
                               G_CALLBACK (clay_filter_tool_response),
                               G_OBJECT (filter_tool), 0);

      if (filter_tool->config)
        {
          filter_tool->settings_box =
            clay_filter_tool_get_settings_box (filter_tool);
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->settings_box,
                              FALSE, FALSE, 0);

          if (filter_tool->has_settings)
            gtk_widget_set_visible (filter_tool->settings_box, TRUE);
        }

      /*  The preview and split view toggles  */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_set_visible (hbox, TRUE);

      toggle = clay_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview", NULL);

      /* Only show merge filter option if we're not editing an NDE filter or
       * applying to a layer group or layer mask */
      if ((CLAY_IS_LAYER (drawable) || CLAY_IS_CHANNEL (drawable)) &&
          ! CLAY_IS_GROUP_LAYER (drawable)                          &&
          ! filter_tool->existing_filter)
        {
          gchar *operation_name = NULL;

          gegl_node_get (filter_tool->operation,
                         "operation", &operation_name,
                         NULL);

          gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

          /* TODO: Once we can serialize ClayDrawable, remove so that filters with
           * aux nodes can be non-destructive */
          if (gegl_node_has_pad (filter_tool->operation, "aux")         ||
              (g_strcmp0 (operation_name, "gegl:gegl") == 0 &&
               g_getenv ("CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT") == NULL) ||
              (clay_item_is_rasterizable (CLAY_ITEM (drawable)) &&
               ! clay_item_is_rasterized (CLAY_ITEM (drawable))))
            {
              GParamSpec  *param_spec;
              GObject     *obj = G_OBJECT (tool_info->tool_options);
              gchar       *tooltip;
              gchar       *disabled_reason;
              const gchar *default_name;
              gboolean     show_merge;

              param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (obj),
                                                         "merge-filter");

              toggle =
                gtk_check_button_new_with_mnemonic (g_param_spec_get_nick (param_spec));

              show_merge = clay_item_is_rasterized (CLAY_ITEM (drawable));
              gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                            show_merge);
              gtk_widget_set_sensitive (toggle, FALSE);

              default_name =
                CLAY_VIEWABLE_GET_CLASS (CLAY_VIEWABLE (drawable))->default_name;

              if (gegl_node_has_pad (filter_tool->operation, "aux"))
                disabled_reason = g_strdup (_("Disabled because this filter "
                                              "depends on another image."));
              else if (clay_item_is_rasterizable (CLAY_ITEM (drawable)) &&
                       ! clay_item_is_rasterized (CLAY_ITEM (drawable)))
                /* TRANSLATORS: The %s will be replaced by the name of the
                 * layer type. */
                disabled_reason = g_strdup_printf (_("Disabled because filters "
                                                     "cannot be merged on: %s."),
                                                   default_name);
              else
                disabled_reason =
                  g_strdup (_("Disabled because GEGL Graph is unsafe.\nFor "
                              "development purpose, set environment variable "
                              "CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT."));

              tooltip = g_strdup_printf ("%s\n<i>%s</i>",
                                         g_param_spec_get_blurb (param_spec),
                                         disabled_reason);
              clay_help_set_help_data_with_markup (toggle, tooltip, NULL);
              g_free (tooltip);
              g_free (disabled_reason);

              gtk_widget_set_visible (toggle, TRUE);
            }
          else
            {
              toggle = clay_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                                   "merge-filter", NULL);
            }

          gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);

          g_free (operation_name);
        }
      else
        {
          gtk_box_pack_start (GTK_BOX (hbox), toggle, TRUE, TRUE, 0);
        }

      toggle = clay_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                           "preview-split", NULL);
      gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);

      g_object_bind_property (G_OBJECT (tool_info->tool_options), "preview",
                              toggle,                             "sensitive",
                              G_BINDING_SYNC_CREATE);

      /*  The show-controller toggle  */
      filter_tool->controller_toggle =
        clay_prop_check_button_new (G_OBJECT (tool_info->tool_options),
                                    "controller", NULL);
      gtk_box_pack_end (GTK_BOX (vbox), filter_tool->controller_toggle,
                        FALSE, FALSE, 0);
      if (! filter_tool->widget)
        gtk_widget_set_visible (filter_tool->controller_toggle, FALSE);

      /*  Fill in subclass widgets  */
      clay_filter_tool_dialog (filter_tool);

      /*  The operation-settings box  */
      filter_tool->operation_settings_box = gtk_box_new (
        GTK_ORIENTATION_VERTICAL, 2);
      gtk_box_pack_start (GTK_BOX (vbox), filter_tool->operation_settings_box,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (filter_tool->operation_settings_box, TRUE);

      clay_filter_tool_update_dialog_operation_settings (filter_tool);
    }
  else
    {
      clay_tool_gui_set_title       (filter_tool->gui,
                                     clay_tool_get_label (tool));
      clay_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      clay_tool_gui_set_icon_name   (filter_tool->gui,
                                     clay_tool_get_icon_name (tool));
      clay_tool_gui_set_help_id     (filter_tool->gui,
                                     clay_tool_get_help_id (tool));
    }

  clay_tool_gui_set_shell (filter_tool->gui, shell);
  clay_tool_gui_set_viewable (filter_tool->gui, CLAY_VIEWABLE (drawable));

  clay_tool_gui_show (filter_tool->gui);

  g_signal_connect_object (drawable, "lock-position-changed",
                           G_CALLBACK (clay_filter_tool_lock_position_changed),
                           filter_tool, 0);

  g_signal_connect_object (image, "mask-changed",
                           G_CALLBACK (clay_filter_tool_mask_changed),
                           filter_tool, 0);

  clay_filter_tool_mask_changed (image, filter_tool);

  clay_filter_tool_create_filter (filter_tool);

  return TRUE;
}

static void
clay_filter_tool_control (ClayTool       *tool,
                          ClayToolAction  action,
                          ClayDisplay    *display)
{
  ClayFilterTool    *filter_tool     = CLAY_FILTER_TOOL (tool);
  ClayFilterOptions *options         = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayDrawable      *drawable        = NULL;
  gboolean           non_destructive = TRUE;
  gchar             *operation_name  = NULL;

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_filter_tool_halt (filter_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      if (filter_tool->filter)
        {
          drawable = clay_drawable_filter_get_drawable (filter_tool->filter);

          if ((! CLAY_IS_LAYER (drawable) && ! CLAY_IS_CHANNEL (drawable)) ||
              (! filter_tool->existing_filter && options->merge_filter))
            non_destructive = FALSE;

          if (filter_tool->operation)
            {
              gegl_node_get (filter_tool->operation,
                             "operation", &operation_name,
                             NULL);

              if (! g_strcmp0 (operation_name, "gegl:nop"))
                non_destructive = FALSE;

              /* TODO: Once we can serialize ClayDrawable, remove so that
               * filters with aux nodes can be non-destructive */
              if (gegl_node_has_pad (filter_tool->operation, "aux") ||
                  /* GEGL graph is dangerous even without using third-party
                   * effects, because it may run any effect. E.g.  it can
                   * run sink effects overwriting any local files with user
                   * rights. We leave a way in through an environment
                   * variable because it is a useful tool for GEGL ops
                   * developers but it should only be set while knowing what
                   * you are doing.
                   */
                  (g_strcmp0 (operation_name, "gegl:gegl") == 0 &&
                   g_getenv ("CLAY_ALLOW_GEGL_GRAPH_LAYER_EFFECT") == NULL))
                non_destructive = FALSE;

              g_free (operation_name);
            }

          /* Ensure that filters applied to group or unrasterized layers are
           * non-destructive */
          if (CLAY_IS_GROUP_LAYER (drawable)                    ||
              (clay_item_is_rasterizable (CLAY_ITEM (drawable)) &&
               ! clay_item_is_rasterized (CLAY_ITEM (drawable))))
            non_destructive = TRUE;
        }
      clay_filter_tool_commit (filter_tool, non_destructive);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_filter_tool_button_press (ClayTool            *tool,
                               const ClayCoords    *coords,
                               guint32              time,
                               GdkModifierType      state,
                               ClayButtonPressType  press_type,
                               ClayDisplay         *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);
  ClayDrawable   *drawable    = NULL;

  if (filter_tool->existing_filter)
    drawable =
      clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (clay_filter_tool_on_guide (filter_tool, coords, display))
    {
      ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (tool);

      if (state & clay_get_extend_selection_mask ())
        {
          clay_filter_options_switch_preview_side (options);
        }
      else if (state & clay_get_toggle_behavior_mask ())
        {
          ClayItem *item = CLAY_ITEM (drawable);
          gint      pos_x;
          gint      pos_y;

          pos_x = CLAMP (RINT (coords->x) - clay_item_get_offset_x (item),
                         0, clay_item_get_width (item));
          pos_y = CLAMP (RINT (coords->y) - clay_item_get_offset_y (item),
                         0, clay_item_get_height (item));

          clay_filter_options_switch_preview_orientation (options,
                                                          pos_x, pos_y);
        }
      else
        {
          clay_guide_tool_start_edit (tool, display,
                                      filter_tool->preview_guide);
        }
    }
  else if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (tool)))
    {
      CLAY_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                    press_type, display);
    }
  else if (filter_tool->widget)
    {
      if (clay_tool_widget_button_press (filter_tool->widget, coords, time,
                                         state, press_type))
        {
          filter_tool->grab_widget = filter_tool->widget;

          clay_tool_control_activate (tool->control);
        }
    }
}

static void
clay_filter_tool_button_release (ClayTool              *tool,
                                 const ClayCoords      *coords,
                                 guint32                time,
                                 GdkModifierType        state,
                                 ClayButtonReleaseType  release_type,
                                 ClayDisplay           *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      clay_tool_control_halt (tool->control);

      clay_tool_widget_button_release (filter_tool->grab_widget,
                                       coords, time, state, release_type);
      filter_tool->grab_widget = NULL;
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->button_release (tool, coords, time, state,
                                                      release_type, display);
    }
}

static void
clay_filter_tool_motion (ClayTool         *tool,
                         const ClayCoords *coords,
                         guint32           time,
                         GdkModifierType   state,
                         ClayDisplay      *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);

  if (filter_tool->grab_widget)
    {
      clay_tool_widget_motion (filter_tool->grab_widget, coords, time, state);
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->motion (tool, coords, time, state,
                                              display);
    }
}

static gboolean
clay_filter_tool_key_press (ClayTool    *tool,
                            GdkEventKey *kevent,
                            ClayDisplay *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);

  if (filter_tool->gui && display == tool->display)
    {
      switch (kevent->keyval)
        {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_ISO_Enter:
          clay_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_OK,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_BackSpace:
          clay_filter_tool_response (filter_tool->gui,
                                     RESPONSE_RESET,
                                     filter_tool);
          return TRUE;

        case GDK_KEY_Escape:
          clay_filter_tool_response (filter_tool->gui,
                                     GTK_RESPONSE_CANCEL,
                                     filter_tool);
          return TRUE;
        }
    }

  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
clay_filter_tool_oper_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              gboolean          proximity,
                              ClayDisplay      *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);

  clay_tool_pop_status (tool, display);

  if (clay_filter_tool_on_guide (filter_tool, coords, display))
    {
      GdkModifierType  extend_mask = clay_get_extend_selection_mask ();
      GdkModifierType  toggle_mask = clay_get_toggle_behavior_mask ();
      gchar           *status      = NULL;

      if (state & extend_mask)
        {
          status = g_strdup (_("Click to switch the original and filtered sides"));
        }
      else if (state & toggle_mask)
        {
          status = g_strdup (_("Click to switch between vertical and horizontal"));
        }
      else
        {
          status = clay_suggest_modifiers (_("Click to move the split guide"),
                                           (extend_mask | toggle_mask) & ~state,
                                           _("%s: switch original and filtered"),
                                           _("%s: switch horizontal and vertical"),
                                           NULL);
        }

      if (proximity)
        clay_tool_push_status (tool, display, "%s", status);

      g_free (status);
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state,
                                                   proximity, display);
    }
}

static void
clay_filter_tool_cursor_update (ClayTool         *tool,
                                const ClayCoords *coords,
                                GdkModifierType   state,
                                ClayDisplay      *display)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (tool);

  if (clay_filter_tool_on_guide (filter_tool, coords, display))
    {
      clay_tool_set_cursor (tool, display,
                            CLAY_CURSOR_MOUSE,
                            CLAY_TOOL_CURSOR_HAND,
                            CLAY_CURSOR_MODIFIER_MOVE);
    }
  else
    {
      CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state,
                                                     display);
    }
}

static void
clay_filter_tool_options_notify (ClayTool         *tool,
                                 ClayToolOptions  *options,
                                 const GParamSpec *pspec)
{
  ClayFilterTool    *filter_tool    = CLAY_FILTER_TOOL (tool);
  ClayFilterOptions *filter_options = CLAY_FILTER_OPTIONS (options);

  if (! strcmp (pspec->name, "preview") &&
      filter_tool->filter)
    {
      clay_filter_tool_update_filter (filter_tool);

      if (filter_options->preview)
        {
          clay_drawable_filter_apply (filter_tool->filter, NULL);

          if (filter_options->preview_split)
            clay_filter_tool_add_guide (filter_tool);
        }
      else
        {
          if (filter_options->preview_split)
            clay_filter_tool_remove_guide (filter_tool);
        }
    }
  else if (! strcmp (pspec->name, "preview-split") &&
           filter_tool->filter)
    {
      if (filter_options->preview_split)
        {
          ClayDrawable     *drawable = NULL;
          ClayDisplayShell *shell    = clay_display_get_shell (tool->display);
          ClayItem         *item     = NULL;
          gint              x, y, width, height;
          gint              position;

          if (filter_tool->existing_filter)
            {
              drawable =
                clay_drawable_filter_get_drawable (filter_tool->existing_filter);
              item = CLAY_ITEM (drawable);
            }
          else
            {
              item = CLAY_ITEM (tool->drawables->data);
            }

          clay_display_shell_untransform_viewport (shell, TRUE,
                                                   &x, &y, &width, &height);

          if (! clay_rectangle_intersect (clay_item_get_offset_x (item),
                                          clay_item_get_offset_y (item),
                                          clay_item_get_width  (item),
                                          clay_item_get_height (item),
                                          x, y, width, height,
                                          &x, &y, &width, &height))
            {
              x      = clay_item_get_offset_x (item);
              y      = clay_item_get_offset_y (item);
              width  = clay_item_get_width    (item);
              height = clay_item_get_height   (item);
            }

          if (filter_options->preview_split_alignment == CLAY_ALIGN_LEFT ||
              filter_options->preview_split_alignment == CLAY_ALIGN_RIGHT)
            {
              position = (x + width  / 2) - clay_item_get_offset_x (item);
            }
          else
            {
              position = (y + height / 2) - clay_item_get_offset_y (item);
            }

          g_object_set (options,
                        "preview-split-position", position,
                        NULL);
        }

      clay_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        clay_filter_tool_add_guide (filter_tool);
      else
        clay_filter_tool_remove_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "preview-split-alignment") ||
           ! strcmp (pspec->name, "preview-split-position"))
    {
      clay_filter_tool_update_filter (filter_tool);

      if (filter_options->preview_split)
        clay_filter_tool_move_guide (filter_tool);
    }
  else if (! strcmp (pspec->name, "merge-filter") &&
           filter_tool->filter)
    {
      ClayDrawable  *drawable = NULL;
      ClayContainer *filters  = NULL;
      gint           count;

      if (filter_tool->existing_filter)
        drawable =
          clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      else
        drawable = tool->drawables->data;

      filters = clay_drawable_get_filters (drawable);
      count   = clay_container_get_n_children (filters);

      if (count > 1)
        {
          if (filter_options->merge_filter)
            clay_container_reorder (filters, CLAY_OBJECT (filter_tool->filter),
                                    count - 1);
          else
            clay_container_reorder (filters, CLAY_OBJECT (filter_tool->filter),
                                    0);

          clay_drawable_update (drawable, 0, 0, -1, -1);
          clay_image_flush (clay_item_get_image (CLAY_ITEM (drawable)));
        }

      g_object_set (filter_tool->filter,
                    "to-be-merged", filter_options->merge_filter,
                    NULL);
    }
  else if (! strcmp (pspec->name, "controller") &&
           filter_tool->widget)
    {
      clay_tool_widget_set_visible (filter_tool->widget,
                                    filter_options->controller);
    }
}

static gboolean
clay_filter_tool_can_pick_color (ClayColorTool    *color_tool,
                                 const ClayCoords *coords,
                                 ClayDisplay      *display)
{
  ClayTool       *tool        = CLAY_TOOL (color_tool);
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (color_tool);
  ClayImage      *image       = clay_display_get_image (display);

  if (! clay_image_equal_selected_drawables (image, tool->drawables))
    return FALSE;

  return filter_tool->pick_abyss ||
         CLAY_COLOR_TOOL_CLASS (parent_class)->can_pick (color_tool,
                                                         coords, display);
}

static gboolean
clay_filter_tool_pick_color (ClayColorTool     *color_tool,
                             const ClayCoords  *coords,
                             ClayDisplay       *display,
                             const Babl       **sample_format,
                             gpointer           pixel,
                             GeglColor        **color)
{
  ClayTool       *tool        = CLAY_TOOL (color_tool);
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (color_tool);
  gboolean        picked;

  if (! filter_tool->existing_filter)
    g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  g_return_val_if_fail (color != NULL && GEGL_IS_COLOR (*color), FALSE);
  g_return_val_if_fail (sample_format != NULL, FALSE);

  picked = CLAY_COLOR_TOOL_CLASS (parent_class)->pick (color_tool, coords,
                                                       display, sample_format,
                                                       pixel, color);

  if (! picked && filter_tool->pick_abyss)
    {
      gegl_color_set_rgba_with_space (*color, 0.0, 0.0, 0.0, 0.0, *sample_format);

      picked = TRUE;
    }

  return picked;
}

static void
clay_filter_tool_color_picked (ClayColorTool      *color_tool,
                               const ClayCoords   *coords,
                               ClayDisplay        *display,
                               ClayColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               GeglColor          *color)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (color_tool);

  if (filter_tool->active_picker)
    {
      ClayPickerCallback callback;
      gpointer           callback_data;

      callback      = g_object_get_data (G_OBJECT (filter_tool->active_picker),
                                         "picker-callback");
      callback_data = g_object_get_data (G_OBJECT (filter_tool->active_picker),
                                         "picker-callback-data");

      if (callback)
        {
          callback (callback_data,
                    filter_tool->pick_identifier,
                    coords->x,
                    coords->y,
                    sample_format, color);

          return;
        }
    }

  CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->color_picked (filter_tool,
                                                          filter_tool->pick_identifier,
                                                          coords->x,
                                                          coords->y,
                                                          sample_format, color);
}

static void
clay_filter_tool_real_reset (ClayFilterTool *filter_tool)
{
  if (filter_tool->config)
    {
      if (filter_tool->default_config)
        {
          clay_config_copy (CLAY_CONFIG (filter_tool->default_config),
                            CLAY_CONFIG (filter_tool->config),
                            0);
        }
      else
        {
          clay_config_reset (CLAY_CONFIG (filter_tool->config));
        }
    }
}

static void
clay_filter_tool_real_set_config (ClayFilterTool *filter_tool,
                                  ClayConfig     *config)
{
  ClayFilterRegion region;

  /* copy the "clay-region" property first, to avoid incorrectly adjusting the
   * copied operation properties in clay_operation_tool_region_changed().
   */
  g_object_get (config,
                "clay-region", &region,
                NULL);
  g_object_set (filter_tool->config,
                "clay-region", region,
                NULL);

  clay_config_copy (CLAY_CONFIG (config),
                    CLAY_CONFIG (filter_tool->config), 0);

  /*  reset the "time" property, otherwise explicitly storing the
   *  config as setting will also copy the time, and the stored object
   *  will be considered to be among the automatically stored recently
   *  used settings
   */
  g_object_set (filter_tool->config,
                "time", (gint64) 0,
                NULL);
}

static void
clay_filter_tool_real_config_notify (ClayFilterTool   *filter_tool,
                                     ClayConfig       *config,
                                     const GParamSpec *pspec)
{
  ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->filter)
    {
      /* note that we may be called with a NULL pspec.  see
       * clay_operation_tool_aux_input_notify().
       */
      if (pspec)
        {
          if (! strcmp (pspec->name, "clay-clip")    ||
              ! strcmp (pspec->name, "clay-mode")    ||
              ! strcmp (pspec->name, "clay-opacity"))
            {
              clay_filter_tool_update_filter (filter_tool);
            }
          else if (! strcmp (pspec->name, "clay-region"))
            {
              clay_filter_tool_update_filter (filter_tool);

              clay_filter_tool_region_changed (filter_tool);
            }
        }

      if (options->preview)
        clay_drawable_filter_apply (filter_tool->filter, NULL);
    }
}

static void
clay_filter_tool_halt (ClayFilterTool *filter_tool)
{
  ClayTool *tool = CLAY_TOOL (filter_tool);

  clay_filter_tool_disable_color_picking (filter_tool);

  if (tool->display)
    {
      ClayImage *image = clay_display_get_image (tool->display);
      GList     *iter;

      for (iter = tool->drawables; iter; iter = iter->next)
        g_signal_handlers_disconnect_by_func (iter->data,
                                              clay_filter_tool_lock_position_changed,
                                              filter_tool);

      g_signal_handlers_disconnect_by_func (image,
                                            clay_filter_tool_mask_changed,
                                            filter_tool);
    }

  if (filter_tool->gui)
    {
      /* explicitly clear the dialog contents first, since we might be called
       * during the dialog's delete event, in which case the dialog will be
       * externally reffed, and will only die *after* clay_filter_tool_halt()
       * returns, and, in particular, after filter_tool->config has been
       * cleared.  we want to make sure the gui is destroyed while
       * filter_tool->config is still alive, since the gui's destruction may
       * fire signals whose handlers rely on it.
       */
      clay_gtk_container_clear (
        GTK_CONTAINER (clay_filter_tool_dialog_get_vbox (filter_tool)));

      g_clear_object (&filter_tool->gui);
      filter_tool->settings_box      = NULL;
      filter_tool->controller_toggle = NULL;
      filter_tool->clip_combo        = NULL;
      filter_tool->region_combo      = NULL;
    }

  if (filter_tool->filter)
    {
      clay_drawable_filter_abort (filter_tool->filter);
      g_signal_handlers_disconnect_by_func (filter_tool->filter,
                                            clay_filter_tool_flush,
                                            filter_tool);
      g_clear_object (&filter_tool->filter);
      clay_filter_tool_remove_guide (filter_tool);
    }

  if (filter_tool->operation)
    clay_gegl_progress_disconnect (filter_tool->operation,
                                   CLAY_PROGRESS (filter_tool));

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            clay_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            clay_filter_tool_unset_setting,
                                            filter_tool);
      g_clear_object (&filter_tool->config);
    }

  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  clay_filter_tool_set_widget (filter_tool, NULL);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (filter_tool->existing_filter)
    {
      ClayDrawable *drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);

      clay_filter_set_active (CLAY_FILTER (filter_tool->existing_filter), TRUE);

      clay_drawable_update (drawable, 0, 0, -1, -1);
      clay_image_flush (clay_item_get_image (CLAY_ITEM (drawable)));

      /* Restore buttons in layer tree view */
      clay_drawable_filters_changed (clay_drawable_filter_get_drawable (filter_tool->existing_filter));
    }

  filter_tool->existing_filter = NULL;

  if (tool_manager_get_active (tool->tool_info->clay) == tool)
    tool_manager_swap_tools (tool->tool_info->clay, TRUE);
}

static void
clay_filter_tool_commit (ClayFilterTool *filter_tool,
                         gboolean        non_destructive)
{
  ClayTool *tool = CLAY_TOOL (filter_tool);

  if (filter_tool->gui)
    clay_tool_gui_hide (filter_tool->gui);

  /* Copy over filter info back to existing filter */
  if (filter_tool->existing_filter)
    {
      ClayImage               *image;
      ClayDrawable            *drawable;
      GeglNode                *node;
      GeglNode                *existing_node;
      gdouble                  opacity;
      gboolean                 clip;
      ClayLayerMode            paint_mode;
      ClayLayerColorSpace      blend_space;
      ClayLayerColorSpace      composite_space;
      ClayLayerCompositeMode   composite_mode;
      ClayFilterRegion         region;
      GParamSpec             **pspecs;
      guint                    n_pspecs;
      gchar                   *operation_name = NULL;
      gchar                   *name           = NULL;

      opacity         = clay_drawable_filter_get_opacity (filter_tool->filter);
      paint_mode      = clay_drawable_filter_get_paint_mode (filter_tool->filter);
      blend_space     = clay_drawable_filter_get_blend_space (filter_tool->filter);
      composite_space = clay_drawable_filter_get_composite_space (filter_tool->filter);
      composite_mode  = clay_drawable_filter_get_composite_mode (filter_tool->filter);
      region          = clay_drawable_filter_get_region (filter_tool->filter);
      clip            = clay_drawable_filter_get_clip (filter_tool->filter);

      node          = clay_drawable_filter_get_operation (filter_tool->filter);
      existing_node = clay_drawable_filter_get_operation (filter_tool->existing_filter);

      gegl_node_get (node, "operation", &operation_name, NULL);
      gegl_node_get (existing_node, "operation", &name, NULL);

      drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      image = clay_item_get_image (CLAY_ITEM (drawable));

      clay_image_undo_push_filter_modified (image, _("Edit filter"),
                                            drawable,
                                            filter_tool->existing_filter);
      /* If the filter was changed, we need to update the original filter's
       * operation */
      if (g_strcmp0 (operation_name, name) != 0)
        {
          gchar *filter_name = NULL;
          gchar *filter_icon = NULL;

          gegl_node_set (existing_node, "operation", operation_name, NULL);

          g_object_get (filter_tool->filter,
                        "name",      &filter_name,
                        "icon-name", &filter_icon,
                        NULL);
          g_object_set (filter_tool->existing_filter,
                        "name",      filter_name,
                        "icon-name", filter_icon,
                        NULL);

          g_free (filter_name);
          g_free (filter_icon);
        }

      g_free (name);

      pspecs = gegl_operation_list_properties (operation_name, &n_pspecs);
      g_free (operation_name);

      for (gint i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec = pspecs[i];
          GValue      value = G_VALUE_INIT;

          g_value_init (&value, pspec->value_type);
          gegl_node_get_property (filter_tool->operation, pspec->name,
                                  &value);

          gegl_node_set_property (existing_node, pspec->name,
                                  &value);
        }

      clay_drawable_filter_set_opacity (filter_tool->existing_filter, opacity);
      clay_drawable_filter_set_mode (filter_tool->existing_filter,
                                     paint_mode, blend_space, composite_space,
                                     composite_mode);
      clay_drawable_filter_set_region (filter_tool->existing_filter, region);
      clay_drawable_filter_set_clip (filter_tool->existing_filter, clip);

      /* Restore buttons in layer tree view */
      clay_drawable_filters_changed (clay_drawable_filter_get_drawable (filter_tool->existing_filter));
    }

  if (filter_tool->filter)
    {
      ClayDrawable      *drawable;
      ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (tool);

      if (! options->preview)
        clay_drawable_filter_apply (filter_tool->filter, NULL);

      clay_drawable_filter_layer_mask_freeze (filter_tool->filter);

      if (non_destructive)
        {
          clay_drawable_filter_abort (filter_tool->filter);

          if (! filter_tool->existing_filter)
            clay_drawable_filter_apply (filter_tool->filter, NULL);
        }

      clay_tool_control_push_preserve (tool->control, TRUE);

      if (! filter_tool->existing_filter)
        clay_drawable_filter_commit (filter_tool->filter, non_destructive,
                                     CLAY_PROGRESS (tool), non_destructive);

      g_signal_handlers_disconnect_by_func (filter_tool->filter,
                                            clay_filter_tool_flush,
                                            filter_tool);

      if (non_destructive && ! filter_tool->existing_filter)
        {
          ClayDrawable *drawable;
          const gchar  *filter_name;

          drawable = clay_drawable_filter_get_drawable (filter_tool->filter);

          filter_name = clay_object_get_name (filter_tool->filter);

          clay_image_undo_push_filter_add (clay_display_get_image (tool->display),
                                           filter_name, drawable,
                                           filter_tool->filter);
        }

      drawable = clay_drawable_filter_get_drawable (filter_tool->filter);

      g_clear_object (&filter_tool->filter);

      clay_tool_control_pop_preserve (tool->control);

      clay_filter_tool_remove_guide (filter_tool);

      if (CLAY_IS_LAYER (drawable) || CLAY_IS_CHANNEL (drawable))
        clay_drawable_update (drawable, 0, 0, -1, -1);

      clay_image_flush (clay_display_get_image (tool->display));

      if (filter_tool->config && filter_tool->has_settings)
        {
          ClayGuiConfig *config = CLAY_GUI_CONFIG (tool->tool_info->clay->config);

          clay_settings_box_add_current (CLAY_SETTINGS_BOX (filter_tool->settings_box),
                                         config->filter_tool_max_recent);
        }
    }

  if (filter_tool->existing_filter)
    {
      ClayDrawable *drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);

      clay_filter_set_active (CLAY_FILTER (filter_tool->existing_filter), TRUE);

      clay_drawable_update (drawable, 0, 0, -1, -1);
      clay_image_flush (clay_display_get_image (tool->display));
    }

  filter_tool->existing_filter = NULL;
}

static void
clay_filter_tool_dialog (ClayFilterTool *filter_tool)
{
  CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->dialog (filter_tool);
}

static void
clay_filter_tool_update_dialog_operation_settings (ClayFilterTool *filter_tool)
{
  ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);

  if (filter_tool->operation_settings_box)
    {
      clay_gtk_container_clear (
        GTK_CONTAINER (filter_tool->operation_settings_box));

      if (filter_tool->config)
        {
          GtkWidget *vbox;
          GtkWidget *expander;
          GtkWidget *frame;
          GtkWidget *vbox2;
          GtkWidget *mode_box;
          GtkWidget *scale;

          vbox = filter_tool->operation_settings_box;

          /*  The clipping combo  */
          filter_tool->clip_combo =
            clay_prop_enum_combo_box_new (filter_tool->config,
                                          "clay-clip",
                                          CLAY_TRANSFORM_RESIZE_ADJUST,
                                          CLAY_TRANSFORM_RESIZE_CLIP);
          clay_int_combo_box_set_label (
            CLAY_INT_COMBO_BOX (filter_tool->clip_combo), _("Clipping"));
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->clip_combo,
                              FALSE, FALSE, 0);

          /*  The region combo  */
          filter_tool->region_combo =
            clay_prop_enum_combo_box_new (filter_tool->config,
                                          "clay-region",
                                          0, 0);
          gtk_box_pack_start (GTK_BOX (vbox), filter_tool->region_combo,
                              FALSE, FALSE, 0);

          /*  The blending-options expander  */
          expander = gtk_expander_new (_("Blending Options"));
          gtk_box_pack_start (GTK_BOX (vbox), expander,
                              FALSE, FALSE, 0);
          gtk_expander_set_resize_toplevel (GTK_EXPANDER (expander), TRUE);
          gtk_widget_set_visible (expander, TRUE);

          g_object_bind_property (options,  "blending-options-expanded",
                                  expander, "expanded",
                                  G_BINDING_SYNC_CREATE |
                                  G_BINDING_BIDIRECTIONAL);

          frame = clay_frame_new (NULL);
          gtk_container_add (GTK_CONTAINER (expander), frame);
          gtk_widget_set_visible (frame, TRUE);

          vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
          gtk_container_add (GTK_CONTAINER (frame), vbox2);
          gtk_widget_set_visible (vbox2, TRUE);

          /*  The mode box  */
          mode_box = clay_prop_layer_mode_box_new (
            filter_tool->config, "clay-mode",
            CLAY_LAYER_MODE_CONTEXT_FILTER);
          clay_layer_mode_box_set_label (CLAY_LAYER_MODE_BOX (mode_box),
                                         _("Mode"));
          gtk_box_pack_start (GTK_BOX (vbox2), mode_box,
                              FALSE, FALSE, 0);
          gtk_widget_set_visible (mode_box, TRUE);

          /*  The opacity scale  */
          scale = clay_prop_spin_scale_new (filter_tool->config,
                                            "clay-opacity",
                                            1.0, 10.0, 1);
          clay_prop_widget_set_factor (scale, 100.0, 1.0, 10.0, 1);
          gtk_box_pack_start (GTK_BOX (vbox2), scale,
                              FALSE, FALSE, 0);
          gtk_widget_set_visible (scale, TRUE);
        }
    }
}

static void
clay_filter_tool_reset (ClayFilterTool *filter_tool)
{
  if (filter_tool->config)
    g_object_freeze_notify (filter_tool->config);

  CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->reset (filter_tool);

  if (filter_tool->config)
    g_object_thaw_notify (filter_tool->config);

  if (filter_tool->widget)
    clay_filter_tool_reset_widget (filter_tool, filter_tool->widget);
}

static void
clay_filter_tool_create_filter (ClayFilterTool *filter_tool)
{
  ClayTool          *tool     = CLAY_TOOL (filter_tool);
  ClayFilterOptions *options  = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayDrawable      *drawable = NULL;
  ClayContainer     *filters;
  gint               count;
  gboolean           merge_filter;

  if (filter_tool->filter)
    {
      clay_drawable_filter_abort (filter_tool->filter);
      g_object_unref (filter_tool->filter);
    }

  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  clay_assert (filter_tool->operation);

  if (! filter_tool->existing_filter)
    g_return_if_fail (g_list_length (tool->drawables) == 1);

  filter_tool->filter = clay_drawable_filter_new (drawable,
                                                  clay_tool_get_undo_desc (tool),
                                                  filter_tool->operation,
                                                  clay_tool_get_icon_name (tool));

  clay_filter_tool_update_filter (filter_tool);

  g_signal_connect (filter_tool->filter, "flush",
                    G_CALLBACK (clay_filter_tool_flush),
                    filter_tool);

  clay_gegl_progress_connect (filter_tool->operation,
                              CLAY_PROGRESS (filter_tool),
                              clay_tool_get_undo_desc (tool));

  /* We need to add the filter first to ensure it is ordered correctly,
   * then toggle its visibility based on user settings */
  if (filter_tool->existing_filter)
    {
      ClayChannel *mask = NULL;

      mask = CLAY_CHANNEL (clay_drawable_filter_get_mask (filter_tool->existing_filter));
      clay_drawable_filter_apply_with_mask (filter_tool->filter, mask,
                                            NULL);
    }
  else
    {
      clay_drawable_filter_apply (filter_tool->filter, NULL);
    }
  clay_drawable_filter_set_preview (filter_tool->filter, options->preview);

  /* TODO: Once we can serialize ClayDrawable, remove so that filters with
   * aux nodes can be non-destructive */
  merge_filter = options->merge_filter;
  if (gegl_node_has_pad (filter_tool->operation, "aux"))
    merge_filter = TRUE;
  else if (clay_item_is_rasterizable (CLAY_ITEM (drawable)) &&
           ! clay_item_is_rasterized (CLAY_ITEM (drawable)))
    merge_filter = FALSE;

  g_object_set (filter_tool->filter,
                "to-be-merged", merge_filter,
                NULL);

  if (merge_filter)
    {
      filters = clay_drawable_get_filters (drawable);
      count   = clay_container_get_n_children (filters);

      if (clay_container_have (filters, CLAY_OBJECT (filter_tool->filter)))
        {
          clay_container_reorder (filters, CLAY_OBJECT (filter_tool->filter),
                                  count - 1);

          clay_drawable_update (drawable, 0, 0, -1, -1);
          clay_image_flush (clay_display_get_image (tool->display));
        }
    }
}

static void
clay_filter_tool_update_dialog (ClayFilterTool *filter_tool)
{
  ClayTool *tool = CLAY_TOOL (filter_tool);

  if (filter_tool->gui)
    {
      ClayImage    *image    = clay_display_get_image (tool->display);
      ClayDrawable *drawable = NULL;
      ClayChannel  *mask     = clay_image_get_mask (image);
      const Babl   *format;

      if (! filter_tool->existing_filter)
        g_return_if_fail (g_list_length (tool->drawables) == 1);

      if (filter_tool->filter)
        {
          drawable = clay_drawable_filter_get_drawable (filter_tool->filter);
          format   = clay_drawable_filter_get_format (filter_tool->filter);
        }
      else
        {
          drawable = tool->drawables->data;
          format   = clay_drawable_get_format (tool->drawables->data);
        }

      if (clay_channel_is_empty (mask))
        {
          gtk_widget_set_visible (
            filter_tool->clip_combo,
            clay_item_get_clip (CLAY_ITEM (drawable), FALSE) == FALSE &&
            ! clay_gegl_node_is_point_operation (filter_tool->operation)    &&
            babl_format_has_alpha (format));

          gtk_widget_set_visible (filter_tool->region_combo, FALSE);
        }
      else
        {
          gtk_widget_set_visible (filter_tool->clip_combo, FALSE);

          gtk_widget_set_visible (
            filter_tool->region_combo,
            ! clay_gegl_node_is_point_operation (filter_tool->operation) ||
            clay_gegl_node_has_key (filter_tool->operation,
                                    "position-dependent"));
        }
    }
}

static void
clay_filter_tool_region_changed (ClayFilterTool *filter_tool)
{
  if (filter_tool->filter &&
      CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed)
    {
      CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->region_changed (filter_tool);
    }
}

static void
clay_filter_tool_flush (ClayDrawableFilter *filter,
                        ClayFilterTool     *filter_tool)
{
  ClayTool  *tool  = CLAY_TOOL (filter_tool);
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

static void
clay_filter_tool_config_notify (GObject          *object,
                                const GParamSpec *pspec,
                                ClayFilterTool   *filter_tool)
{
  CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->config_notify (filter_tool,
                                                           CLAY_CONFIG (object),
                                                           pspec);
}

static void
clay_filter_tool_unset_setting (GObject          *object,
                                const GParamSpec *pspec,
                                ClayFilterTool   *filter_tool)
{
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        clay_filter_tool_unset_setting,
                                        filter_tool);

  clay_settings_box_unset (CLAY_SETTINGS_BOX (filter_tool->settings_box));
}

static void
clay_filter_tool_lock_position_changed (ClayDrawable   *drawable,
                                        ClayFilterTool *filter_tool)
{
  clay_filter_tool_update_dialog (filter_tool);
}

static void
clay_filter_tool_mask_changed (ClayImage      *image,
                               ClayFilterTool *filter_tool)
{
  ClayOperationSettings *settings;

  settings = CLAY_OPERATION_SETTINGS (filter_tool->config);

  clay_filter_tool_update_dialog (filter_tool);

  if (settings && settings->region == CLAY_FILTER_REGION_SELECTION)
    clay_filter_tool_region_changed (filter_tool);
}

static void
clay_filter_tool_add_guide (ClayFilterTool *filter_tool)
{
  ClayTool            *tool     = CLAY_TOOL (filter_tool);
  ClayFilterOptions   *options  = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayDrawable        *drawable = NULL;
  ClayItem            *item;
  ClayImage           *image;
  ClayOrientationType  orientation;
  gint                 position;

  if (! filter_tool->existing_filter)
    g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (filter_tool->preview_guide)
    return;

  if (filter_tool->existing_filter)
    {
      drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      item = CLAY_ITEM (drawable);
    }
  else
    {
      item = CLAY_ITEM (tool->drawables->data);
    }
  image = clay_item_get_image (item);

  if (options->preview_split_alignment == CLAY_ALIGN_LEFT ||
      options->preview_split_alignment == CLAY_ALIGN_RIGHT)
    {
      orientation = CLAY_ORIENTATION_VERTICAL;
      position    = clay_item_get_offset_x (item) +
                    options->preview_split_position;
    }
  else
    {
      orientation = CLAY_ORIENTATION_HORIZONTAL;
      position    = clay_item_get_offset_y (item) +
                    options->preview_split_position;
    }

  filter_tool->preview_guide =
    clay_guide_custom_new (orientation,
                           image->clay->next_guide_id++,
                           CLAY_GUIDE_STYLE_SPLIT_VIEW);

  clay_image_add_guide (image, filter_tool->preview_guide, position);

  g_signal_connect (filter_tool->preview_guide, "removed",
                    G_CALLBACK (clay_filter_tool_guide_removed),
                    filter_tool);
  g_signal_connect (filter_tool->preview_guide, "notify::position",
                    G_CALLBACK (clay_filter_tool_guide_moved),
                    filter_tool);
}

static void
clay_filter_tool_remove_guide (ClayFilterTool *filter_tool)
{
  ClayTool  *tool = CLAY_TOOL (filter_tool);
  ClayImage *image;

  if (! filter_tool->preview_guide)
    return;

  image = clay_item_get_image (CLAY_ITEM (tool->drawables->data));

  clay_image_remove_guide (image, filter_tool->preview_guide, FALSE);
}

static void
clay_filter_tool_move_guide (ClayFilterTool *filter_tool)
{
  ClayTool            *tool     = CLAY_TOOL (filter_tool);
  ClayFilterOptions   *options  = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayDrawable        *drawable = NULL;
  ClayItem            *item;
  ClayOrientationType  orientation;
  gint                 position;

  if (! filter_tool->existing_filter)
    g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! filter_tool->preview_guide)
    return;

  if (filter_tool->existing_filter)
    {
      drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      item = CLAY_ITEM (drawable);
    }
  else
    {
      item = CLAY_ITEM (tool->drawables->data);
    }

  if (options->preview_split_alignment == CLAY_ALIGN_LEFT ||
      options->preview_split_alignment == CLAY_ALIGN_RIGHT)
    {
      orientation = CLAY_ORIENTATION_VERTICAL;
      position    = clay_item_get_offset_x (item) +
                    options->preview_split_position;
    }
  else
    {
      orientation = CLAY_ORIENTATION_HORIZONTAL;
      position    = clay_item_get_offset_x (item) +
                    options->preview_split_position;
    }

  if (orientation != clay_guide_get_orientation (filter_tool->preview_guide) ||
      position    != clay_guide_get_position (filter_tool->preview_guide))
    {
      clay_guide_set_orientation (filter_tool->preview_guide, orientation);
      clay_image_move_guide (clay_item_get_image (item),
                             filter_tool->preview_guide, position, FALSE);
    }
}

static void
clay_filter_tool_guide_removed (ClayGuide      *guide,
                                ClayFilterTool *filter_tool)
{
  ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);

  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        clay_filter_tool_guide_removed,
                                        filter_tool);
  g_signal_handlers_disconnect_by_func (G_OBJECT (filter_tool->preview_guide),
                                        clay_filter_tool_guide_moved,
                                        filter_tool);

  g_clear_object (&filter_tool->preview_guide);

  g_object_set (options,
                "preview-split", FALSE,
                NULL);
}

static void
clay_filter_tool_guide_moved (ClayGuide        *guide,
                              const GParamSpec *pspec,
                              ClayFilterTool   *filter_tool)
{
  ClayTool          *tool     = CLAY_TOOL (filter_tool);
  ClayFilterOptions *options  = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayDrawable      *drawable = NULL;
  ClayItem          *item;
  gint               position;

  if (! filter_tool->existing_filter)
    g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (filter_tool->existing_filter)
    {
      drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      item = CLAY_ITEM (drawable);
    }
  else
    {
      item = CLAY_ITEM (tool->drawables->data);
    }

  if (options->preview_split_alignment == CLAY_ALIGN_LEFT ||
      options->preview_split_alignment == CLAY_ALIGN_RIGHT)
    {
      position = CLAMP (clay_guide_get_position (guide) -
                        clay_item_get_offset_x (item),
                        0, clay_item_get_width (item));
    }
  else
    {
      position = CLAMP (clay_guide_get_position (guide) -
                        clay_item_get_offset_y (item),
                        0, clay_item_get_height (item));
    }

  g_object_set (options,
                "preview-split-position", position,
                NULL);
}

static void
clay_filter_tool_response (ClayToolGui    *gui,
                           gint            response_id,
                           ClayFilterTool *filter_tool)
{
  ClayTool *tool = CLAY_TOOL (filter_tool);

  switch (response_id)
    {
    case RESPONSE_RESET:
      clay_filter_tool_reset (filter_tool);
      break;

    case GTK_RESPONSE_OK:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);
      break;

    default:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_filter_tool_update_filter (ClayFilterTool *filter_tool)
{
  ClayFilterOptions     *options  = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);
  ClayOperationSettings *settings;

  if (! filter_tool->filter)
    return;

  settings = CLAY_OPERATION_SETTINGS (filter_tool->config);

  clay_drawable_filter_set_preview       (filter_tool->filter,
                                          options->preview);
  clay_drawable_filter_set_preview_split (filter_tool->filter,
                                          options->preview_split,
                                          options->preview_split_alignment,
                                          options->preview_split_position);
  clay_drawable_filter_set_add_alpha     (filter_tool->filter,
                                          clay_gegl_node_has_key (
                                            filter_tool->operation,
                                            "needs-alpha"));

  clay_operation_settings_sync_drawable_filter (settings, filter_tool->filter);
}

static void
clay_filter_tool_set_has_settings (ClayFilterTool *filter_tool,
                                   gboolean        has_settings)
{
  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));

  filter_tool->has_settings = has_settings;

  if (! filter_tool->settings_box)
    return;

  if (filter_tool->has_settings)
    {
      ClayTool *tool  = CLAY_TOOL (filter_tool);
      GQuark    quark = g_quark_from_static_string ("settings-folder");
      GType     type  = G_TYPE_FROM_INSTANCE (filter_tool->config);
      GFile    *settings_folder;
      gchar    *import_title;
      gchar    *export_title;

      settings_folder = g_type_get_qdata (type, quark);

      import_title = g_strdup_printf (_("Import '%s' Settings"),
                                      clay_tool_get_label (tool));
      export_title = g_strdup_printf (_("Export '%s' Settings"),
                                      clay_tool_get_label (tool));

      g_object_set (filter_tool->settings_box,
                    "visible",        TRUE,
                    "config",         filter_tool->config,
                    "container",      filter_tool->settings,
                    "help-id",        clay_tool_get_help_id (tool),
                    "import-title",   import_title,
                    "export-title",   export_title,
                    "default-folder", settings_folder,
                    "last-file",      NULL,
                    NULL);

      g_free (import_title);
      g_free (export_title);
    }
  else
    {
      g_object_set (filter_tool->settings_box,
                    "visible",        FALSE,
                    "config",         NULL,
                    "container",      NULL,
                    "help-id",        NULL,
                    "import-title",   NULL,
                    "export-title",   NULL,
                    "default-folder", NULL,
                    "last-file",      NULL,
                    NULL);
    }
}


/*  public functions  */

void
clay_filter_tool_get_operation (ClayFilterTool     *filter_tool,
                                ClayDrawableFilter *existing_filter)
{
  ClayTool             *tool;
  ClayFilterToolClass  *klass;
  gchar                *operation_name;
  GParamSpec          **pspecs;

  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));

  tool  = CLAY_TOOL (filter_tool);
  klass = CLAY_FILTER_TOOL_GET_CLASS (filter_tool);

  if (filter_tool->filter)
    {
      clay_drawable_filter_abort (filter_tool->filter);
      g_clear_object (&filter_tool->filter);

      clay_filter_tool_remove_guide (filter_tool);
    }

  g_clear_object (&filter_tool->operation);

  if (filter_tool->config)
    {
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            clay_filter_tool_config_notify,
                                            filter_tool);
      g_signal_handlers_disconnect_by_func (filter_tool->config,
                                            clay_filter_tool_unset_setting,
                                            filter_tool);
      g_clear_object (&filter_tool->config);
    }

  g_clear_object (&filter_tool->default_config);
  g_clear_object (&filter_tool->settings);
  g_clear_pointer (&filter_tool->description, g_free);

  operation_name = klass->get_operation (filter_tool,
                                         &filter_tool->description);

  if (! operation_name)
    operation_name = g_strdup ("gegl:nop");

  if (! filter_tool->description)
    filter_tool->description = g_strdup (clay_tool_get_label (tool));

  filter_tool->operation = gegl_node_new_child (NULL,
                                                "operation", operation_name,
                                                NULL);
  if (existing_filter)
    {
      filter_tool->existing_filter = existing_filter;
      clay_filter_set_active (CLAY_FILTER (filter_tool->existing_filter), FALSE);
    }

  filter_tool->config =
    g_object_new (clay_operation_config_get_type (tool->tool_info->clay,
                                                  operation_name,
                                                  clay_tool_get_icon_name (tool),
                                                  CLAY_TYPE_OPERATION_SETTINGS),
                  NULL);

  /* Update layer effect if we're editing it */
  if (filter_tool->existing_filter)
    {
      GeglNode          *existing_node;
      gdouble            opacity;
      ClayLayerMode      paint_mode;
      ClayFilterRegion   region;
      const gchar       *name;

      opacity    = clay_drawable_filter_get_opacity (filter_tool->existing_filter);
      paint_mode = clay_drawable_filter_get_paint_mode (filter_tool->existing_filter);
      region     = clay_drawable_filter_get_region (filter_tool->existing_filter);

      existing_node = clay_drawable_filter_get_operation (filter_tool->existing_filter);
      gegl_node_get (existing_node,
                     "operation", &name,
                     NULL);

      if (! strcmp (clay_object_get_name (tool->tool_info), "clay-operation-tool") &&
          ! clay_operation_config_is_custom (tool->tool_info->clay, operation_name))
        {
          GParamSpec **pspecs;
          guint        n_pspecs;

          pspecs = gegl_operation_list_properties (operation_name, &n_pspecs);

          for (gint i = 0; i < n_pspecs; i++)
            {
              GValue      value      = G_VALUE_INIT;
              GParamSpec *pspec      = pspecs[i];
              GParamSpec *clay_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (filter_tool->config),
                                                                     pspec->name);

              g_value_init (&value, pspec->value_type);
              gegl_node_get_property (existing_node, pspec->name,
                                      &value);

              if (clay_pspec)
                g_object_set_property (G_OBJECT (filter_tool->config), clay_pspec->name,
                                       &value);
              else
                g_critical ("%s: property '%s' of operation '%s' doesn't exist in config %s",
                            G_STRFUNC, pspec->name, operation_name,
                            g_type_name (G_TYPE_FROM_INSTANCE (filter_tool->config)));
              g_value_unset (&value);
            }
          g_free (pspecs);
        }

      g_object_set (filter_tool->config,
                    "clay-opacity", opacity,
                    "clay-mode",    paint_mode,
                    "clay-region",  region,
                    NULL);
    }

  clay_operation_config_sync_node (filter_tool->config,
                                   filter_tool->operation);
  clay_operation_config_connect_node (filter_tool->config,
                                      filter_tool->operation);

  filter_tool->settings =
    clay_operation_config_get_container (tool->tool_info->clay,
                                         G_TYPE_FROM_INSTANCE (filter_tool->config),
                                         (GCompareFunc) clay_settings_compare);
  g_object_ref (filter_tool->settings);

  pspecs =
    clay_operation_config_list_properties (filter_tool->config,
                                           G_TYPE_FROM_INSTANCE (filter_tool->config),
                                           0, NULL);

  clay_filter_tool_set_has_settings (filter_tool, (pspecs != NULL));

  g_free (pspecs);

  if (filter_tool->gui)
    {
      clay_tool_gui_set_title       (filter_tool->gui,
                                     clay_tool_get_label (tool));
      clay_tool_gui_set_description (filter_tool->gui, filter_tool->description);
      clay_tool_gui_set_icon_name   (filter_tool->gui,
                                     clay_tool_get_icon_name (tool));
      clay_tool_gui_set_help_id     (filter_tool->gui,
                                     clay_tool_get_help_id (tool));

      clay_filter_tool_update_dialog_operation_settings (filter_tool);
    }

  clay_filter_tool_update_dialog (filter_tool);

  g_free (operation_name);

  g_object_set (CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool),
                "preview-split", FALSE,
                NULL);

  g_signal_connect_object (filter_tool->config, "notify",
                           G_CALLBACK (clay_filter_tool_config_notify),
                           G_OBJECT (filter_tool), 0);

  if (tool->drawables)
    clay_filter_tool_create_filter (filter_tool);
}

void
clay_filter_tool_set_config (ClayFilterTool *filter_tool,
                             ClayConfig     *config)
{
  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (CLAY_IS_OPERATION_SETTINGS (config));

  /* if the user didn't change a setting since the last set_config(),
   * this handler is still connected
   */
  g_signal_handlers_disconnect_by_func (filter_tool->config,
                                        clay_filter_tool_unset_setting,
                                        filter_tool);

  CLAY_FILTER_TOOL_GET_CLASS (filter_tool)->set_config (filter_tool, config);

  if (filter_tool->widget)
    clay_filter_tool_reset_widget (filter_tool, filter_tool->widget);

  if (filter_tool->settings_box)
    g_signal_connect_object (filter_tool->config, "notify",
                             G_CALLBACK (clay_filter_tool_unset_setting),
                             G_OBJECT (filter_tool), 0);

  /* If editing a filter, move it to the right location below the existing
   * filter and set its properties. This should only run when the filter
   * is initially edited. */
  if (filter_tool->existing_filter &&
      clay_drawable_filter_get_mask (filter_tool->filter) == NULL)
    {
      ClayContainer          *filters;
      ClayDrawableFilterMask *mask;
      ClayDrawable           *existing_drawable;
      gint                    index;
      gchar                  *name;

      /* Get drawable from existing filter, as we might have a different
       * drawable selected in the layer tree */
      existing_drawable =
        clay_drawable_filter_get_drawable (filter_tool->existing_filter);
      filters = clay_drawable_get_filters (existing_drawable);

      mask = clay_drawable_filter_get_mask (filter_tool->existing_filter);
      if (filters)
        {
          gint existing_index;

          index = clay_container_get_child_index (filters,
                                                  CLAY_OBJECT (filter_tool->existing_filter));

          existing_index =
            clay_container_get_child_index (filters,
                                            CLAY_OBJECT (filter_tool->filter));

          if (existing_index > -1)
            clay_container_reorder (filters, CLAY_OBJECT (filter_tool->filter),
                                    index);
        }

      clay_drawable_filter_set_temporary (filter_tool->filter, TRUE);

      name = g_strdup_printf (_("Editing '%s'..."),
                              clay_object_get_name (filter_tool->existing_filter));
      g_object_set (filter_tool->filter,
                    "name", name,
                    "mask", mask,
                    NULL);

      g_free (name);
    }
}

void
clay_filter_tool_edit_as (ClayFilterTool *filter_tool,
                          const gchar    *new_tool_id,
                          ClayConfig     *config)
{
  ClayDisplay        *display;
  ClayContext        *user_context;
  ClayToolInfo       *tool_info;
  ClayTool           *new_tool;
  ClayDrawableFilter *existing_filter = NULL;

  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (new_tool_id != NULL);
  g_return_if_fail (CLAY_IS_CONFIG (config));

  if (filter_tool->existing_filter)
    existing_filter = filter_tool->existing_filter;

  display = CLAY_TOOL (filter_tool)->display;

  user_context = clay_get_user_context (display->clay);

  tool_info = (ClayToolInfo *)
    clay_container_get_child_by_name (display->clay->tool_info_list,
                                      new_tool_id);

  clay_tool_control (CLAY_TOOL (filter_tool), CLAY_TOOL_ACTION_HALT, display);
  clay_context_set_tool (user_context, tool_info);
  tool_manager_initialize_active (display->clay, display);

  new_tool = tool_manager_get_active (display->clay);

  CLAY_FILTER_TOOL (new_tool)->default_config = g_object_ref (G_OBJECT (config));

  clay_filter_tool_reset (CLAY_FILTER_TOOL (new_tool));

  /* If we're converting an existing filter, set it to inactive and
   * move the new filter to the right spot as usual */
  if (filter_tool->existing_filter)
    {
      ClayDrawable  *drawable = NULL;
      ClayContainer *filters  = NULL;

      CLAY_FILTER_TOOL (new_tool)->existing_filter = existing_filter;

      clay_filter_set_active (CLAY_FILTER (existing_filter), FALSE);

      drawable = clay_drawable_filter_get_drawable (existing_filter);
      filters  = clay_drawable_get_filters (drawable);

      if (filters)
        {
          gint index;
          gint existing_index;

          index =
            clay_container_get_child_index (filters,
                                            CLAY_OBJECT (existing_filter));

          existing_index =
            clay_container_get_child_index (filters,
                                            CLAY_OBJECT (CLAY_FILTER_TOOL (new_tool)->filter));

          if (existing_index > -1)
            clay_container_reorder (filters,
                                    CLAY_OBJECT (CLAY_FILTER_TOOL (new_tool)->filter),
                                    index);
        }
    }
}

gboolean
clay_filter_tool_on_guide (ClayFilterTool   *filter_tool,
                           const ClayCoords *coords,
                           ClayDisplay      *display)
{
  ClayDisplayShell *shell;

  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  shell = clay_display_get_shell (display);

  if (filter_tool->filter        &&
      filter_tool->preview_guide &&
      clay_display_shell_get_show_guides (shell))
    {
      const gint          snap_distance = display->config->snap_distance;
      ClayOrientationType orientation;
      gint                position;

      orientation = clay_guide_get_orientation (filter_tool->preview_guide);
      position    = clay_guide_get_position (filter_tool->preview_guide);

      if (orientation == CLAY_ORIENTATION_HORIZONTAL)
        {
          if (fabs (coords->y - position) <= FUNSCALEY (shell, snap_distance))
            return TRUE;
        }
      else
        {
          if (fabs (coords->x - position) <= FUNSCALEX (shell, snap_distance))
            return TRUE;
        }
    }

  return FALSE;
}

GtkWidget *
clay_filter_tool_dialog_get_vbox (ClayFilterTool *filter_tool)
{
  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), NULL);

  return clay_tool_gui_get_vbox (filter_tool->gui);
}

void
clay_filter_tool_enable_color_picking (ClayFilterTool *filter_tool,
                                       gpointer        identifier,
                                       gboolean        pick_abyss)
{
  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));

  clay_filter_tool_disable_color_picking (filter_tool);

  /* note that ownership over 'identifier' is not transferred, and its
   * lifetime should be managed by the caller.
   */
  filter_tool->pick_identifier = identifier;
  filter_tool->pick_abyss      = pick_abyss;

  clay_color_tool_enable (CLAY_COLOR_TOOL (filter_tool),
                          CLAY_COLOR_TOOL_GET_OPTIONS (filter_tool));
}

void
clay_filter_tool_disable_color_picking (ClayFilterTool *filter_tool)
{
  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));

  if (filter_tool->active_picker)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (filter_tool->active_picker);

      filter_tool->active_picker = NULL;

      gtk_toggle_button_set_active (toggle, FALSE);
    }

  if (clay_color_tool_is_enabled (CLAY_COLOR_TOOL (filter_tool)))
    clay_color_tool_disable (CLAY_COLOR_TOOL (filter_tool));
}

static void
clay_filter_tool_color_picker_toggled (GtkWidget      *widget,
                                       ClayFilterTool *filter_tool)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gpointer identifier;
      gboolean pick_abyss;

      if (filter_tool->active_picker == widget)
        return;

      identifier =                  g_object_get_data (G_OBJECT (widget),
                                                       "picker-identifier");
      pick_abyss = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                       "picker-pick-abyss"));

      clay_filter_tool_enable_color_picking (filter_tool,
                                             identifier, pick_abyss);

      filter_tool->active_picker = widget;
    }
  else if (filter_tool->active_picker == widget)
    {
      clay_filter_tool_disable_color_picking (filter_tool);
    }
}

GtkWidget *
clay_filter_tool_add_color_picker (ClayFilterTool     *filter_tool,
                                   gpointer            identifier,
                                   const gchar        *icon_name,
                                   const gchar        *tooltip,
                                   gboolean            pick_abyss,
                                   ClayPickerCallback  callback,
                                   gpointer            callback_data)
{
  GtkWidget *button;
  GtkWidget *image;

  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);

  button = g_object_new (GTK_TYPE_TOGGLE_BUTTON,
                         "draw-indicator", FALSE,
                         NULL);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  g_object_set (image,
                "margin-start",  2,
                "margin-end",    2,
                "margin-top",    2,
                "margin-bottom", 2,
                NULL);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_visible (image, TRUE);

  if (tooltip)
    clay_help_set_help_data (button, tooltip, NULL);

  g_object_set_data (G_OBJECT (button),
                     "picker-identifier", identifier);
  g_object_set_data (G_OBJECT (button),
                     "picker-pick-abyss", GINT_TO_POINTER (pick_abyss));
  g_object_set_data (G_OBJECT (button),
                     "picker-callback", callback);
  g_object_set_data (G_OBJECT (button),
                     "picker-callback-data", callback_data);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (clay_filter_tool_color_picker_toggled),
                    filter_tool);

  return button;
}

GCallback
clay_filter_tool_add_controller (ClayFilterTool     *filter_tool,
                                 ClayControllerType  controller_type,
                                 const gchar        *status_title,
                                 GCallback           callback,
                                 gpointer            callback_data,
                                 gpointer           *set_func_data)
{
  ClayToolWidget *widget;
  GCallback       set_func;

  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), NULL);
  g_return_val_if_fail (callback != NULL, NULL);
  g_return_val_if_fail (callback_data != NULL, NULL);
  g_return_val_if_fail (set_func_data != NULL, NULL);

  widget = clay_filter_tool_create_widget (filter_tool,
                                           controller_type,
                                           status_title,
                                           callback,
                                           callback_data,
                                           &set_func,
                                           set_func_data);
  clay_filter_tool_set_widget (filter_tool, widget);
  g_object_unref (widget);

  return set_func;
}

void
clay_filter_tool_set_widget (ClayFilterTool *filter_tool,
                             ClayToolWidget *widget)
{
  g_return_if_fail (CLAY_IS_FILTER_TOOL (filter_tool));
  g_return_if_fail (widget == NULL || CLAY_IS_TOOL_WIDGET (widget));

  if (widget == filter_tool->widget)
    return;

  if (filter_tool->widget)
    {
      if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (filter_tool)))
        clay_draw_tool_stop (CLAY_DRAW_TOOL (filter_tool));

      g_object_unref (filter_tool->widget);
    }

  filter_tool->widget = widget;
  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (filter_tool), widget);

  if (filter_tool->widget)
    {
      ClayFilterOptions *options = CLAY_FILTER_TOOL_GET_OPTIONS (filter_tool);

      g_object_ref (filter_tool->widget);

      clay_tool_widget_set_visible (filter_tool->widget,
                                    options->controller);

      if (CLAY_TOOL (filter_tool)->display)
        clay_draw_tool_start (CLAY_DRAW_TOOL (filter_tool),
                              CLAY_TOOL (filter_tool)->display);
    }

  if (filter_tool->controller_toggle)
    {
      gtk_widget_set_visible (filter_tool->controller_toggle,
                              filter_tool->widget != NULL);
    }
}

gboolean
clay_filter_tool_get_drawable_area (ClayFilterTool *filter_tool,
                                    gint           *drawable_offset_x,
                                    gint           *drawable_offset_y,
                                    GeglRectangle  *drawable_area)
{
  ClayTool              *tool;
  ClayOperationSettings *settings;
  ClayDrawable          *drawable;

  g_return_val_if_fail (CLAY_IS_FILTER_TOOL (filter_tool), FALSE);
  g_return_val_if_fail (drawable_offset_x != NULL, FALSE);
  g_return_val_if_fail (drawable_offset_y != NULL, FALSE);
  g_return_val_if_fail (drawable_area != NULL, FALSE);

  tool     = CLAY_TOOL (filter_tool);
  settings = CLAY_OPERATION_SETTINGS (filter_tool->config);

  if (! filter_tool->existing_filter)
    g_return_val_if_fail (g_list_length (tool->drawables) == 1, FALSE);

  *drawable_offset_x = 0;
  *drawable_offset_y = 0;

  drawable_area->x      = 0;
  drawable_area->y      = 0;
  drawable_area->width  = 1;
  drawable_area->height = 1;

  if (filter_tool->existing_filter)
    drawable = clay_drawable_filter_get_drawable (filter_tool->existing_filter);
  else
    drawable = tool->drawables->data;

  if (drawable && settings)
    {
      clay_item_get_offset (CLAY_ITEM (drawable),
                            drawable_offset_x, drawable_offset_y);

      if (CLAY_IS_GROUP_LAYER (drawable) &&
          clay_layer_get_mode (CLAY_LAYER (drawable)) == CLAY_LAYER_MODE_PASS_THROUGH)
        {
          GeglRectangle rect = clay_drawable_get_bounding_box (drawable);

          drawable_area->width  = rect.width;
          drawable_area->height = rect.height;
        }
      else
        {
          switch (settings->region)
            {
            case CLAY_FILTER_REGION_SELECTION:
              if (! clay_item_mask_intersect (CLAY_ITEM (drawable),
                                              &drawable_area->x,
                                              &drawable_area->y,
                                              &drawable_area->width,
                                              &drawable_area->height))
                {
                  drawable_area->x      = 0;
                  drawable_area->y      = 0;
                  drawable_area->width  = 1;
                  drawable_area->height = 1;
                }
              break;

            case CLAY_FILTER_REGION_DRAWABLE:
              drawable_area->width  = clay_item_get_width  (CLAY_ITEM (drawable));
              drawable_area->height = clay_item_get_height (CLAY_ITEM (drawable));
              break;
            }
        }

      return TRUE;
    }

  return FALSE;
}
