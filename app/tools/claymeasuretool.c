/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Measure tool
 * Copyright (C) 1999-2003 Sven Neumann <sven@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayprogress.h"
#include "core/clay-transform-utils.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"
#include "display/claytoolcompass.h"
#include "display/claytoolgui.h"

#include "claymeasureoptions.h"
#include "claymeasuretool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     clay_measure_tool_control         (ClayTool              *tool,
                                                   ClayToolAction         action,
                                                   ClayDisplay           *display);
static void     clay_measure_tool_modifier_key    (ClayTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   ClayDisplay           *display);
static void     clay_measure_tool_button_press    (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayButtonPressType    press_type,
                                                   ClayDisplay           *display);
static void     clay_measure_tool_button_release  (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayButtonReleaseType  release_type,
                                                   ClayDisplay           *display);
static void     clay_measure_tool_motion          (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayDisplay           *display);

static void     clay_measure_tool_recalc_matrix   (ClayTransformTool     *tr_tool);
static gchar  * clay_measure_tool_get_undo_desc   (ClayTransformTool     *tr_tool);

static void     clay_measure_tool_compass_changed (ClayToolWidget        *widget,
                                                   ClayMeasureTool       *measure);
static void     clay_measure_tool_compass_response(ClayToolWidget        *widget,
                                                   gint                   response_id,
                                                   ClayMeasureTool       *measure);
static void     clay_measure_tool_compass_status  (ClayToolWidget        *widget,
                                                   const gchar           *status,
                                                   ClayMeasureTool       *measure);
static void     clay_measure_tool_compass_create_guides
                                                  (ClayToolWidget        *widget,
                                                   gint                   x,
                                                   gint                   y,
                                                   gboolean               horizontal,
                                                   gboolean               vertical,
                                                   ClayMeasureTool       *measure);

static void     clay_measure_tool_start           (ClayMeasureTool       *measure,
                                                   ClayDisplay           *display,
                                                   const ClayCoords      *coords);
static void     clay_measure_tool_halt            (ClayMeasureTool       *measure);

static ClayToolGui * clay_measure_tool_dialog_new (ClayMeasureTool       *measure);
static void     clay_measure_tool_dialog_update   (ClayMeasureTool       *measure,
                                                   ClayDisplay           *display);

static void     clay_measure_tool_straighten_button_clicked
                                                  (GtkWidget             *button,
                                                   ClayMeasureTool       *measure);

G_DEFINE_TYPE (ClayMeasureTool, clay_measure_tool, CLAY_TYPE_TRANSFORM_TOOL)

#define parent_class clay_measure_tool_parent_class


void
clay_measure_tool_register (ClayToolRegisterCallback  callback,
                            gpointer                  data)
{
  (* callback) (CLAY_TYPE_MEASURE_TOOL,
                CLAY_TYPE_MEASURE_OPTIONS,
                clay_measure_options_gui,
                0,
                "clay-measure-tool",
                _("Measure"),
                _("Measure Tool: Measure distances and angles"),
                N_("_Measure"), "<shift>M",
                NULL, CLAY_HELP_TOOL_MEASURE,
                CLAY_ICON_TOOL_MEASURE,
                data);
}

static void
clay_measure_tool_class_init (ClayMeasureToolClass *klass)
{
  ClayToolClass          *tool_class = CLAY_TOOL_CLASS (klass);
  ClayTransformToolClass *tr_class   = CLAY_TRANSFORM_TOOL_CLASS (klass);

  tool_class->control        = clay_measure_tool_control;
  tool_class->modifier_key   = clay_measure_tool_modifier_key;
  tool_class->button_press   = clay_measure_tool_button_press;
  tool_class->button_release = clay_measure_tool_button_release;
  tool_class->motion         = clay_measure_tool_motion;
  tool_class->is_destructive = FALSE;

  tr_class->recalc_matrix    = clay_measure_tool_recalc_matrix;
  tr_class->get_undo_desc    = clay_measure_tool_get_undo_desc;

  tr_class->undo_desc        = C_("undo-type", "Straighten");
  tr_class->progress_text    = _("Straightening");
}

static void
clay_measure_tool_init (ClayMeasureTool *measure)
{
  ClayTool *tool = CLAY_TOOL (measure);

  clay_tool_control_set_handle_empty_image (tool->control, TRUE);
  clay_tool_control_set_active_modifiers   (tool->control,
                                            CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_PIXEL_BORDER);
  clay_tool_control_set_cursor             (tool->control,
                                            CLAY_CURSOR_CROSSHAIR_SMALL);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_MEASURE);

  clay_draw_tool_set_default_status (CLAY_DRAW_TOOL (tool),
                                     _("Click-Drag to create a line"));
}

static void
clay_measure_tool_control (ClayTool       *tool,
                           ClayToolAction  action,
                           ClayDisplay    *display)
{
  ClayMeasureTool *measure = CLAY_MEASURE_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_measure_tool_halt (measure);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_measure_tool_modifier_key (ClayTool        *tool,
                                GdkModifierType  key,
                                gboolean         press,
                                GdkModifierType  state,
                                ClayDisplay     *display)
{
  ClayMeasureOptions *options = CLAY_MEASURE_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_toggle_behavior_mask ())
    {
      switch (options->orientation)
        {
        case CLAY_COMPASS_ORIENTATION_HORIZONTAL:
          g_object_set (options,
                        "orientation", CLAY_COMPASS_ORIENTATION_VERTICAL,
                        NULL);
          break;

        case CLAY_COMPASS_ORIENTATION_VERTICAL:
          g_object_set (options,
                        "orientation", CLAY_COMPASS_ORIENTATION_HORIZONTAL,
                        NULL);
          break;

        default:
          break;
        }
    }
}

static void
clay_measure_tool_button_press (ClayTool            *tool,
                                const ClayCoords    *coords,
                                guint32              time,
                                GdkModifierType      state,
                                ClayButtonPressType  press_type,
                                ClayDisplay         *display)
{
  ClayMeasureTool    *measure = CLAY_MEASURE_TOOL (tool);
  ClayMeasureOptions *options = CLAY_MEASURE_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell   *shell   = clay_display_get_shell (display);
  ClayImage          *image   = clay_display_get_image (display);

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! measure->widget)
    {
      measure->supress_guides = TRUE;

      clay_measure_tool_start (measure, display, coords);

      clay_tool_widget_hover (measure->widget, coords, state, TRUE);
    }

  if (clay_tool_widget_button_press (measure->widget, coords, time, state,
                                     press_type))
    {
      measure->grab_widget = measure->widget;
    }

  /*  create the info window if necessary  */
  if (! measure->gui)
    {
      if (options->use_info_window ||
          ! clay_display_shell_get_show_statusbar (shell))
        {
          g_set_weak_pointer (&measure->gui,
                              clay_measure_tool_dialog_new (measure));
        }
    }

  if (measure->gui)
    {
      clay_tool_gui_set_shell (measure->gui, shell);
      clay_tool_gui_set_viewable (measure->gui, CLAY_VIEWABLE (image));

      clay_measure_tool_dialog_update (measure, display);
    }

  clay_tool_control_activate (tool->control);
}

static void
clay_measure_tool_button_release (ClayTool              *tool,
                                  const ClayCoords      *coords,
                                  guint32                time,
                                  GdkModifierType        state,
                                  ClayButtonReleaseType  release_type,
                                  ClayDisplay           *display)
{
  ClayMeasureTool *measure = CLAY_MEASURE_TOOL (tool);

  clay_tool_control_halt (tool->control);

  if (measure->grab_widget)
    {
      clay_tool_widget_button_release (measure->grab_widget,
                                       coords, time, state, release_type);
      measure->grab_widget = NULL;
    }

  measure->supress_guides = FALSE;
}

static void
clay_measure_tool_motion (ClayTool         *tool,
                          const ClayCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          ClayDisplay      *display)
{
  ClayMeasureTool *measure = CLAY_MEASURE_TOOL (tool);

  if (measure->grab_widget)
    {
      clay_tool_widget_motion (measure->grab_widget, coords, time, state);
    }
}

static void
clay_measure_tool_recalc_matrix (ClayTransformTool *tr_tool)
{
  ClayMeasureTool *measure = CLAY_MEASURE_TOOL (tr_tool);
  gdouble          angle;

  if (measure->n_points < 2)
    {
      tr_tool->transform_valid = FALSE;

      return;
    }

  g_object_get (measure->widget,
                "pixel-angle", &angle,
                NULL);

  clay_matrix3_identity (&tr_tool->transform);
  clay_transform_matrix_rotate_center (&tr_tool->transform,
                                       measure->x[0], measure->y[0],
                                       angle);

  tr_tool->transform_valid = TRUE;
}

static gchar *
clay_measure_tool_get_undo_desc (ClayTransformTool *tr_tool)
{
  ClayMeasureTool        *measure = CLAY_MEASURE_TOOL (tr_tool);
  ClayCompassOrientation  orientation;
  gdouble                 angle;

  g_object_get (measure->widget,
                "effective-orientation", &orientation,
                "pixel-angle",           &angle,
                NULL);

  angle = clay_rad_to_deg (fabs (angle));

  switch (orientation)
    {
    case CLAY_COMPASS_ORIENTATION_AUTO:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten by %-3.3g°"),
                              angle);

    case CLAY_COMPASS_ORIENTATION_HORIZONTAL:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten Horizontally by %-3.3g°"),
                              angle);

    case CLAY_COMPASS_ORIENTATION_VERTICAL:
      return g_strdup_printf (C_("undo-type",
                                 "Straighten Vertically by %-3.3g°"),
                              angle);
    }

  g_return_val_if_reached (NULL);
}

static void
clay_measure_tool_compass_changed (ClayToolWidget  *widget,
                                   ClayMeasureTool *measure)
{
  ClayMeasureOptions *options = CLAY_MEASURE_TOOL_GET_OPTIONS (measure);

  g_object_get (widget,
                "n-points", &measure->n_points,
                "x1",       &measure->x[0],
                "y1",       &measure->y[0],
                "x2",       &measure->x[1],
                "y2",       &measure->y[1],
                "x3",       &measure->x[2],
                "y3",       &measure->y[2],
                NULL);

  gtk_widget_set_sensitive (options->straighten_button, measure->n_points >= 2);
  clay_measure_tool_dialog_update (measure, CLAY_TOOL (measure)->display);
}

static void
clay_measure_tool_compass_response (ClayToolWidget  *widget,
                                    gint             response_id,
                                    ClayMeasureTool *measure)
{
  ClayTool *tool = CLAY_TOOL (measure);

  if (response_id == CLAY_TOOL_WIDGET_RESPONSE_CANCEL)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
}

static void
clay_measure_tool_compass_status (ClayToolWidget  *widget,
                                  const gchar     *status,
                                  ClayMeasureTool *measure)
{
  ClayTool *tool = CLAY_TOOL (measure);

  if (! status)
    {
      /* replace status bar hint by distance and angle */
      clay_measure_tool_dialog_update (measure, tool->display);
    }
}

static void
clay_measure_tool_compass_create_guides (ClayToolWidget  *widget,
                                         gint             x,
                                         gint             y,
                                         gboolean         horizontal,
                                         gboolean         vertical,
                                         ClayMeasureTool *measure)
{
  ClayDisplay *display = CLAY_TOOL (measure)->display;
  ClayImage   *image   = clay_display_get_image (display);

  if (measure->supress_guides)
    return;

  if (horizontal || vertical)
    {
      if (horizontal && vertical)
        clay_image_undo_group_start (image,
                                     CLAY_UNDO_GROUP_GUIDE,
                                     _("Add Guides"));

      if (horizontal)
        clay_image_add_hguide (image, y, TRUE);

      if (vertical)
        clay_image_add_vguide (image, x, TRUE);

      if (horizontal && vertical)
        clay_image_undo_group_end (image);

      clay_image_flush (image);
    }
}

static void
clay_measure_tool_start (ClayMeasureTool  *measure,
                         ClayDisplay      *display,
                         const ClayCoords *coords)
{
  ClayTool           *tool    = CLAY_TOOL (measure);
  ClayDisplayShell   *shell   = clay_display_get_shell (display);
  ClayMeasureOptions *options = CLAY_MEASURE_TOOL_GET_OPTIONS (tool);

  measure->n_points = 1;
  measure->x[0]     = coords->x;
  measure->y[0]     = coords->y;
  measure->x[1]     = 0;
  measure->y[1]     = 0;
  measure->x[2]     = 0;
  measure->y[2]     = 0;

  measure->widget = clay_tool_compass_new (shell,
                                           options->orientation,
                                           measure->n_points,
                                           measure->x[0],
                                           measure->y[0],
                                           measure->x[1],
                                           measure->y[1],
                                           measure->x[2],
                                           measure->y[2]);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), measure->widget);

  g_object_bind_property (options,         "orientation",
                          measure->widget, "orientation",
                          G_BINDING_DEFAULT);

  g_signal_connect (measure->widget, "changed",
                    G_CALLBACK (clay_measure_tool_compass_changed),
                    measure);
  g_signal_connect (measure->widget, "response",
                    G_CALLBACK (clay_measure_tool_compass_response),
                    measure);
  g_signal_connect (measure->widget, "status",
                    G_CALLBACK (clay_measure_tool_compass_status),
                    measure);
  g_signal_connect (measure->widget, "create-guides",
                    G_CALLBACK (clay_measure_tool_compass_create_guides),
                    measure);
  g_signal_connect (options->straighten_button, "clicked",
                    G_CALLBACK (clay_measure_tool_straighten_button_clicked),
                    measure);

  tool->display = display;

  clay_draw_tool_start (CLAY_DRAW_TOOL (measure), display);
}

static void
clay_measure_tool_halt (ClayMeasureTool *measure)
{
  ClayMeasureOptions *options = CLAY_MEASURE_TOOL_GET_OPTIONS (measure);
  ClayTool           *tool    = CLAY_TOOL (measure);

  if (options->straighten_button)
    {
      gtk_widget_set_sensitive (options->straighten_button, FALSE);

      g_signal_handlers_disconnect_by_func (
        options->straighten_button,
        G_CALLBACK (clay_measure_tool_straighten_button_clicked),
        measure);
    }

  if (tool->display)
    clay_tool_pop_status (tool, tool->display);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (measure)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (measure));

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&measure->widget);

  g_clear_object (&measure->gui);

  tool->display = NULL;
}

static void
clay_measure_tool_dialog_update (ClayMeasureTool *measure,
                                 ClayDisplay     *display)
{
  ClayDisplayShell *shell = clay_display_get_shell (display);
  ClayImage        *image = clay_display_get_image (display);
  gint              ax, ay;
  gint              bx, by;
  gint              pixel_width;
  gint              pixel_height;
  gdouble           unit_width;
  gdouble           unit_height;
  gdouble           pixel_distance;
  gdouble           unit_distance;
  gdouble           inch_distance;
  gdouble           pixel_angle;
  gdouble           unit_angle;
  gdouble           xres;
  gdouble           yres;
  gchar             format[128];
  gint              unit_distance_digits = 0;
  gint              unit_width_digits;
  gint              unit_height_digits;

  /*  calculate distance and angle  */
  ax = measure->x[1] - measure->x[0];
  ay = measure->y[1] - measure->y[0];

  if (measure->n_points == 3)
    {
      bx = measure->x[2] - measure->x[0];
      by = measure->y[2] - measure->y[0];
    }
  else
    {
      bx = 0;
      by = 0;
    }

  pixel_width  = ABS (ax - bx);
  pixel_height = ABS (ay - by);

  clay_image_get_resolution (image, &xres, &yres);

  unit_width  = clay_pixels_to_units (pixel_width,  shell->unit, xres);
  unit_height = clay_pixels_to_units (pixel_height, shell->unit, yres);

  pixel_distance = sqrt (SQR (ax - bx) + SQR (ay - by));
  inch_distance = sqrt (SQR ((gdouble) (ax - bx) / xres) +
                        SQR ((gdouble) (ay - by) / yres));
  unit_distance = clay_unit_get_factor (shell->unit) * inch_distance;

  g_object_get (measure->widget,
                "pixel-angle", &pixel_angle,
                "unit-angle",  &unit_angle,
                NULL);

  pixel_angle = fabs (pixel_angle * 180.0 / G_PI);
  unit_angle  = fabs (unit_angle  * 180.0 / G_PI);

  /* Compute minimum digits to display accurate values, so that
   * every pixel shows a different value in unit.
   */
  if (inch_distance)
    unit_distance_digits = clay_unit_get_scaled_digits (shell->unit,
                                                        pixel_distance /
                                                        inch_distance);
  unit_width_digits  = clay_unit_get_scaled_digits (shell->unit, xres);
  unit_height_digits = clay_unit_get_scaled_digits (shell->unit, yres);

  if (shell->unit == clay_unit_pixel ())
    {
      clay_tool_replace_status (CLAY_TOOL (measure), display,
                                "%.1f %s, %.2f\302\260 (%d × %d)",
                                pixel_distance, _("pixels"), pixel_angle,
                                pixel_width, pixel_height);
    }
  else
    {
      g_snprintf (format, sizeof (format),
                  "%%.%df %s, %%.2f\302\260 (%%.%df × %%.%df)",
                  unit_distance_digits,
                  clay_unit_get_name (shell->unit),
                  unit_width_digits,
                  unit_height_digits);

      clay_tool_replace_status (CLAY_TOOL (measure), display, format,
                                unit_distance, unit_angle,
                                unit_width, unit_height);
    }

  if (measure->gui)
    {
      gchar buf[128];

      /* Distance */
      g_snprintf (buf, sizeof (buf), "%.1f", pixel_distance);
      gtk_label_set_text (GTK_LABEL (measure->distance_label[0]), buf);

      if (shell->unit != clay_unit_pixel ())
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_distance_digits);
          g_snprintf (buf, sizeof (buf), format, unit_distance);
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]),
                              clay_unit_get_name (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->distance_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[0]), NULL);
        }

      /* Angle */
      g_snprintf (buf, sizeof (buf), "%.2f", pixel_angle);
      gtk_label_set_text (GTK_LABEL (measure->angle_label[0]), buf);

      if (fabs (unit_angle - pixel_angle) > 0.01)
        {
          g_snprintf (buf, sizeof (buf), "%.2f", unit_angle);
          gtk_label_set_text (GTK_LABEL (measure->angle_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[1]), "\302\260");
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->angle_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[1]), NULL);
        }

      /* Width */
      g_snprintf (buf, sizeof (buf), "%d", pixel_width);
      gtk_label_set_text (GTK_LABEL (measure->width_label[0]), buf);

      if (shell->unit != clay_unit_pixel ())
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_width_digits);
          g_snprintf (buf, sizeof (buf), format, unit_width);
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]),
                              clay_unit_get_name (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->width_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[2]), NULL);
        }

      g_snprintf (buf, sizeof (buf), "%d", pixel_height);
      gtk_label_set_text (GTK_LABEL (measure->height_label[0]), buf);

      /* Height */
      if (shell->unit != clay_unit_pixel ())
        {
          g_snprintf (format, sizeof (format), "%%.%df",
                      unit_height_digits);
          g_snprintf (buf, sizeof (buf), format, unit_height);
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), buf);

          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]),
                              clay_unit_get_name (shell->unit));
        }
      else
        {
          gtk_label_set_text (GTK_LABEL (measure->height_label[1]), NULL);
          gtk_label_set_text (GTK_LABEL (measure->unit_label[3]), NULL);
        }

      clay_tool_gui_show (measure->gui);
    }
}

static ClayToolGui *
clay_measure_tool_dialog_new (ClayMeasureTool *measure)
{
  ClayTool         *tool = CLAY_TOOL (measure);
  ClayDisplayShell *shell;
  ClayToolGui      *gui;
  GtkWidget        *grid;
  GtkWidget        *label;

  g_return_val_if_fail (tool->display != NULL, NULL);

  shell = clay_display_get_shell (tool->display);

  gui = clay_tool_gui_new (tool->tool_info,
                           NULL,
                           _("Measure Distances and Angles"),
                           NULL, NULL,
                           clay_widget_get_monitor (GTK_WIDGET (shell)),
                           TRUE,

                           _("_Close"), GTK_RESPONSE_CLOSE,

                           NULL);

  clay_tool_gui_set_auto_overlay (gui, TRUE);
  clay_tool_gui_set_focus_on_map (gui, FALSE);

  g_signal_connect (gui, "response",
                    G_CALLBACK (g_object_unref),
                    NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (gui)), grid,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (grid, TRUE);


  label = gtk_label_new (_("Distance:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->distance_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->distance_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->unit_label[0] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 0, 1, 1);
  gtk_widget_set_visible (label, TRUE);


  label = gtk_label_new (_("Angle:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->angle_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new ("\302\260");
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->angle_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->unit_label[1] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);


  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->width_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->width_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->unit_label[2] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 2, 1, 1);
  gtk_widget_set_visible (label, TRUE);


  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->height_label[0] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new (_("pixels"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->height_label[1] = label = gtk_label_new ("0.0");
  gtk_label_set_selectable (GTK_LABEL (label), TRUE);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  measure->unit_label[3] = label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 4, 3, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  return gui;
}

static void
clay_measure_tool_straighten_button_clicked (GtkWidget       *button,
                                             ClayMeasureTool *measure)
{
  ClayTool          *tool    = CLAY_TOOL (measure);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (measure);

  if (clay_transform_tool_transform (tr_tool, tool->display))
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
}
