/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/claytoolinfo.h"

#include "widgets/claycolorframe.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolgui.h"

#include "claycolorpickeroptions.h"
#include "claycolorpickertool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_color_picker_tool_constructed   (GObject             *object);
static void   clay_color_picker_tool_dispose       (GObject             *object);

static void   clay_color_picker_tool_control       (ClayTool            *tool,
                                                    ClayToolAction       action,
                                                    ClayDisplay         *display);
static void   clay_color_picker_tool_modifier_key  (ClayTool            *tool,
                                                    GdkModifierType      key,
                                                    gboolean             press,
                                                    GdkModifierType      state,
                                                    ClayDisplay         *display);
static void   clay_color_picker_tool_oper_update   (ClayTool            *tool,
                                                    const ClayCoords    *coords,
                                                    GdkModifierType      state,
                                                    gboolean             proximity,
                                                    ClayDisplay         *display);

static void   clay_color_picker_tool_picked        (ClayColorTool       *color_tool,
                                                    const ClayCoords    *coords,
                                                    ClayDisplay         *display,
                                                    ClayColorPickState   pick_state,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    GeglColor           *color);

static void   clay_color_picker_tool_info_create   (ClayColorPickerTool *picker_tool,
                                                    ClayDisplay         *display);
static void   clay_color_picker_tool_info_response (ClayToolGui         *gui,
                                                    gint                 response_id,
                                                    ClayColorPickerTool *picker_tool);
static void   clay_color_picker_tool_info_update   (ClayColorPickerTool *picker_tool,
                                                    ClayDisplay         *display,
                                                    gboolean             sample_average,
                                                    const Babl          *sample_format,
                                                    gpointer             pixel,
                                                    GeglColor           *color,
                                                    gint                 x,
                                                    gint                 y);


G_DEFINE_TYPE (ClayColorPickerTool, clay_color_picker_tool,
               CLAY_TYPE_COLOR_TOOL)

#define parent_class clay_color_picker_tool_parent_class


void
clay_color_picker_tool_register (ClayToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (CLAY_TYPE_COLOR_PICKER_TOOL,
                CLAY_TYPE_COLOR_PICKER_OPTIONS,
                clay_color_picker_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-color-picker-tool",
                _("Color Picker"),
                _("Color Picker Tool: Set colors from image pixels"),
                N_("C_olor Picker"), "O",
                NULL, CLAY_HELP_TOOL_COLOR_PICKER,
                CLAY_ICON_TOOL_COLOR_PICKER,
                data);
}

static void
clay_color_picker_tool_class_init (ClayColorPickerToolClass *klass)
{
  GObjectClass       *object_class     = G_OBJECT_CLASS (klass);
  ClayToolClass      *tool_class       = CLAY_TOOL_CLASS (klass);
  ClayColorToolClass *color_tool_class = CLAY_COLOR_TOOL_CLASS (klass);

  object_class->constructed  = clay_color_picker_tool_constructed;
  object_class->dispose      = clay_color_picker_tool_dispose;

  tool_class->control        = clay_color_picker_tool_control;
  tool_class->modifier_key   = clay_color_picker_tool_modifier_key;
  tool_class->oper_update    = clay_color_picker_tool_oper_update;
  tool_class->is_destructive = FALSE;

  color_tool_class->picked   = clay_color_picker_tool_picked;
}

static void
clay_color_picker_tool_init (ClayColorPickerTool *picker_tool)
{
  ClayColorTool *color_tool = CLAY_COLOR_TOOL (picker_tool);

  color_tool->pick_target = CLAY_COLOR_PICK_TARGET_FOREGROUND;
}

static void
clay_color_picker_tool_constructed (GObject *object)
{
  ClayTool *tool = CLAY_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_color_tool_enable (CLAY_COLOR_TOOL (object),
                          CLAY_COLOR_TOOL_GET_OPTIONS (tool));
}

static void
clay_color_picker_tool_dispose (GObject *object)
{
  ClayColorPickerTool *picker_tool = CLAY_COLOR_PICKER_TOOL (object);

  g_clear_object (&picker_tool->gui);
  picker_tool->color_area   = NULL;
  picker_tool->color_frame1 = NULL;
  picker_tool->color_frame2 = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_color_picker_tool_control (ClayTool       *tool,
                                ClayToolAction  action,
                                ClayDisplay    *display)
{
  ClayColorPickerTool *picker_tool = CLAY_COLOR_PICKER_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      if (picker_tool->gui)
        clay_tool_gui_hide (picker_tool->gui);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_color_picker_tool_modifier_key (ClayTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     ClayDisplay     *display)
{
  ClayColorPickerOptions *options = CLAY_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_extend_selection_mask ())
    {
      g_object_set (options, "use-info-window", ! options->use_info_window,
                    NULL);
    }
  else if (key == clay_get_toggle_behavior_mask ())
    {
      switch (options->pick_target)
        {
        case CLAY_COLOR_PICK_TARGET_FOREGROUND:
          g_object_set (options,
                        "pick-target", CLAY_COLOR_PICK_TARGET_BACKGROUND,
                        NULL);
          break;

        case CLAY_COLOR_PICK_TARGET_BACKGROUND:
          g_object_set (options,
                        "pick-target", CLAY_COLOR_PICK_TARGET_FOREGROUND,
                        NULL);
          break;

        default:
          break;
        }

    }
}

static void
clay_color_picker_tool_oper_update (ClayTool         *tool,
                                    const ClayCoords *coords,
                                    GdkModifierType   state,
                                    gboolean          proximity,
                                    ClayDisplay      *display)
{
  ClayColorPickerTool    *picker_tool = CLAY_COLOR_PICKER_TOOL (tool);
  ClayColorPickerOptions *options = CLAY_COLOR_PICKER_TOOL_GET_OPTIONS (tool);

  CLAY_COLOR_TOOL (tool)->pick_target = options->pick_target;

  clay_tool_pop_status (tool, display);

  if (proximity)
    {
      gchar           *status_help = NULL;
      GdkModifierType  extend_mask = 0;
      GdkModifierType  toggle_mask;

      if (! picker_tool->gui)
        extend_mask = clay_get_extend_selection_mask ();

      toggle_mask = clay_get_toggle_behavior_mask ();

      switch (options->pick_target)
        {
        case CLAY_COLOR_PICK_TARGET_NONE:
          status_help = clay_suggest_modifiers (_("Click in any image to view"
                                                  " its color"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;

        case CLAY_COLOR_PICK_TARGET_FOREGROUND:
          status_help = clay_suggest_modifiers (_("Click in any image to pick"
                                                  " the foreground color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case CLAY_COLOR_PICK_TARGET_BACKGROUND:
          status_help = clay_suggest_modifiers (_("Click in any image to pick"
                                                  " the background color"),
                                                (extend_mask | toggle_mask) &
                                                ~state,
                                                NULL, NULL, NULL);
          break;

        case CLAY_COLOR_PICK_TARGET_PALETTE:
          status_help = clay_suggest_modifiers (_("Click in any image to add"
                                                  " the color to the palette"),
                                                extend_mask & ~state,
                                                NULL, NULL, NULL);
          break;
        }

      if (status_help != NULL)
        {
          clay_tool_push_status (tool, display, "%s", status_help);
          g_free (status_help);
        }
    }

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
clay_color_picker_tool_picked (ClayColorTool      *color_tool,
                               const ClayCoords   *coords,
                               ClayDisplay        *display,
                               ClayColorPickState  pick_state,
                               const Babl         *sample_format,
                               gpointer            pixel,
                               GeglColor          *color)
{
  ClayColorPickerTool    *picker_tool = CLAY_COLOR_PICKER_TOOL (color_tool);
  ClayColorPickerOptions *options;

  options = CLAY_COLOR_PICKER_TOOL_GET_OPTIONS (color_tool);

  if (options->use_info_window && ! picker_tool->gui)
    clay_color_picker_tool_info_create (picker_tool, display);

  if (picker_tool->gui &&
      (options->use_info_window ||
       clay_tool_gui_get_visible (picker_tool->gui)))
    {
      clay_color_picker_tool_info_update (picker_tool, display,
                                          CLAY_COLOR_OPTIONS (options)->sample_average,
                                          sample_format, pixel, color,
                                          (gint) floor (coords->x),
                                          (gint) floor (coords->y));
    }

  CLAY_COLOR_TOOL_CLASS (parent_class)->picked (color_tool,
                                                coords, display, pick_state,
                                                sample_format, pixel, color);
}

static void
clay_color_picker_tool_info_create (ClayColorPickerTool *picker_tool,
                                    ClayDisplay         *display)
{
  Clay             *clay      = clay_display_get_clay (display);
  ClayTool         *tool      = CLAY_TOOL (picker_tool);
  ClayToolOptions  *options   = CLAY_TOOL_GET_OPTIONS (tool);
  ClayContext      *context   = CLAY_CONTEXT (tool->tool_info->tool_options);
  ClayDisplayShell *shell     = clay_display_get_shell (display);
  ClayImage        *image     = clay_display_get_image (display);
  GList            *drawables = clay_image_get_selected_drawables (image);
  GtkWidget        *hbox;
  GtkWidget        *frame;
  GeglColor        *color;

  picker_tool->gui = clay_tool_gui_new (tool->tool_info,
                                        NULL,
                                        _("Color Picker Information"),
                                        NULL, NULL,
                                        clay_widget_get_monitor (GTK_WIDGET (shell)),
                                        TRUE,

                                        _("_Close"), GTK_RESPONSE_CLOSE,

                                        NULL);

  clay_tool_gui_set_auto_overlay (picker_tool->gui, TRUE);
  clay_tool_gui_set_focus_on_map (picker_tool->gui, FALSE);
  clay_tool_gui_set_viewables (picker_tool->gui, drawables);

  g_signal_connect (picker_tool->gui, "response",
                    G_CALLBACK (clay_color_picker_tool_info_response),
                    picker_tool);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (picker_tool->gui)),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (hbox, TRUE);

  picker_tool->color_frame1 = clay_color_frame_new (clay);
  clay_color_frame_set_color_config (CLAY_COLOR_FRAME (picker_tool->color_frame1),
                                     context->clay->config->color_management);
  clay_color_frame_set_has_coords (CLAY_COLOR_FRAME (picker_tool->color_frame1),
                                   TRUE);
  g_object_bind_property (options,                   "frame1-mode",
                          picker_tool->color_frame1, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame1,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (picker_tool->color_frame1, TRUE);

  picker_tool->color_frame2 = clay_color_frame_new (clay);
  clay_color_frame_set_color_config (CLAY_COLOR_FRAME (picker_tool->color_frame2),
                                     context->clay->config->color_management);
  g_object_bind_property (options,                   "frame2-mode",
                          picker_tool->color_frame2, "mode",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (hbox), picker_tool->color_frame2,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (picker_tool->color_frame2, TRUE);

  frame = gtk_frame_new (NULL);
  clay_widget_set_fully_opaque (frame, TRUE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  color = gegl_color_new ("transparent");
  picker_tool->color_area =
    clay_color_area_new (color,
                         drawables && clay_drawable_has_alpha (drawables->data) ?
                         CLAY_COLOR_AREA_LARGE_CHECKS :
                         CLAY_COLOR_AREA_FLAT,
                         GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
  clay_color_area_set_color_config (CLAY_COLOR_AREA (picker_tool->color_area),
                                    context->clay->config->color_management);
  gtk_widget_set_size_request (picker_tool->color_area, 48, -1);
  gtk_drag_dest_unset (picker_tool->color_area);
  gtk_container_add (GTK_CONTAINER (frame), picker_tool->color_area);
  gtk_widget_set_visible (picker_tool->color_area, TRUE);

  g_list_free (drawables);
  g_object_unref (color);
}

static void
clay_color_picker_tool_info_response (ClayToolGui         *gui,
                                      gint                 response_id,
                                      ClayColorPickerTool *picker_tool)
{
  ClayTool *tool = CLAY_TOOL (picker_tool);

  clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, NULL);
}

static void
clay_color_picker_tool_info_update (ClayColorPickerTool *picker_tool,
                                    ClayDisplay         *display,
                                    gboolean             sample_average,
                                    const Babl          *sample_format,
                                    gpointer             pixel,
                                    GeglColor           *color,
                                    gint                 x,
                                    gint                 y)
{
  ClayTool  *tool      = CLAY_TOOL (picker_tool);
  ClayImage *image     = clay_display_get_image (display);
  GList     *drawables = clay_image_get_selected_drawables (image);

  tool->display = display;

  clay_tool_gui_set_shell (picker_tool->gui,
                           clay_display_get_shell (display));
  clay_tool_gui_set_viewables (picker_tool->gui, drawables);
  g_list_free (drawables);

  clay_color_area_set_color (CLAY_COLOR_AREA (picker_tool->color_area), color);
  clay_color_frame_set_color (CLAY_COLOR_FRAME (picker_tool->color_frame1),
                              sample_average, color, x, y);
  clay_color_frame_set_color (CLAY_COLOR_FRAME (picker_tool->color_frame2),
                              sample_average, color, x, y);

  clay_tool_gui_show (picker_tool->gui);
}
