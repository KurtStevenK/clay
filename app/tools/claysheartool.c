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
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay-transform-utils.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolgui.h"
#include "display/claytoolsheargrid.h"

#include "claysheartool.h"
#include "claytoolcontrol.h"
#include "claytransformgridoptions.h"

#include "clay-intl.h"


/*  index into trans_info array  */
enum
{
  ORIENTATION,
  SHEAR_X,
  SHEAR_Y
};


#define SB_WIDTH 10


/*  local function prototypes  */
static gboolean         clay_shear_tool_key_press      (ClayTool              *tool,
                                                        GdkEventKey           *kevent,
                                                        ClayDisplay           *display);
static gboolean         clay_shear_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                                        ClayMatrix3           *transform);
static gchar          * clay_shear_tool_get_undo_desc  (ClayTransformGridTool *tg_tool);
static void             clay_shear_tool_dialog         (ClayTransformGridTool *tg_tool);
static void             clay_shear_tool_dialog_update  (ClayTransformGridTool *tg_tool);
static void             clay_shear_tool_prepare        (ClayTransformGridTool *tg_tool);
static ClayToolWidget * clay_shear_tool_get_widget     (ClayTransformGridTool *tg_tool);
static void             clay_shear_tool_update_widget  (ClayTransformGridTool *tg_tool);
static void             clay_shear_tool_widget_changed (ClayTransformGridTool *tg_tool);

static void             shear_x_mag_changed            (GtkAdjustment         *adj,
                                                        ClayTransformGridTool *tg_tool);
static void             shear_y_mag_changed            (GtkAdjustment         *adj,
                                                        ClayTransformGridTool *tg_tool);


G_DEFINE_TYPE (ClayShearTool, clay_shear_tool, CLAY_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class clay_shear_tool_parent_class


void
clay_shear_tool_register (ClayToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (CLAY_TYPE_SHEAR_TOOL,
                CLAY_TYPE_TRANSFORM_GRID_OPTIONS,
                clay_transform_grid_options_gui,
                0,
                "clay-shear-tool",
                _("Shear"),
                _("Shear Tool: Shear the layer, selection or path"),
                N_("S_hear"), "<shift>H",
                NULL, CLAY_HELP_TOOL_SHEAR,
                CLAY_ICON_TOOL_SHEAR,
                data);
}

static void
clay_shear_tool_class_init (ClayShearToolClass *klass)
{
  ClayToolClass              *tool_class = CLAY_TOOL_CLASS (klass);
  ClayTransformToolClass     *tr_class   = CLAY_TRANSFORM_TOOL_CLASS (klass);
  ClayTransformGridToolClass *tg_class   = CLAY_TRANSFORM_GRID_TOOL_CLASS (klass);

  tool_class->key_press     = clay_shear_tool_key_press;

  tg_class->info_to_matrix  = clay_shear_tool_info_to_matrix;
  tg_class->get_undo_desc   = clay_shear_tool_get_undo_desc;
  tg_class->dialog          = clay_shear_tool_dialog;
  tg_class->dialog_update   = clay_shear_tool_dialog_update;
  tg_class->prepare         = clay_shear_tool_prepare;
  tg_class->get_widget      = clay_shear_tool_get_widget;
  tg_class->update_widget   = clay_shear_tool_update_widget;
  tg_class->widget_changed  = clay_shear_tool_widget_changed;

  tr_class->progress_text   = C_("undo-type", "Shear");
  tr_class->progress_text   = _("Shearing");
  tg_class->ok_button_label = _("_Shear");
}

static void
clay_shear_tool_init (ClayShearTool *shear_tool)
{
  ClayTool *tool = CLAY_TOOL (shear_tool);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_SHEAR);
}

static gboolean
clay_shear_tool_key_press (ClayTool    *tool,
                           GdkEventKey *kevent,
                           ClayDisplay *display)
{
  ClayShearTool *shear = CLAY_SHEAR_TOOL (tool);
  gdouble        x     = gtk_adjustment_get_value (shear->x_adj);
  gdouble        y     = gtk_adjustment_get_value (shear->y_adj);
  gdouble        step  = gtk_adjustment_get_step_increment (shear->x_adj);

  if (kevent->state == clay_get_extend_selection_mask ())
    step = gtk_adjustment_get_page_increment (shear->x_adj);

  /* Adapt to the zoom factor when holding <shift> */
  step /= clay_zoom_model_get_factor (clay_display_get_shell (display)->zoom);
  step = MAX (1, step);

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
      gtk_adjustment_set_value (shear->y_adj, (y + step));
      return TRUE;

    case GDK_KEY_Down:
      gtk_adjustment_set_value (shear->y_adj, (y - step));
      return TRUE;

    case GDK_KEY_Right:
      gtk_adjustment_set_value (shear->x_adj, (x + step));
      return TRUE;

    case GDK_KEY_Left:
      gtk_adjustment_set_value (shear->x_adj, (x - step));
      return TRUE;

    default:
      break;
    }

  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static gboolean
clay_shear_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                ClayMatrix3           *transform)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  gdouble            amount;

  if (tg_tool->trans_info[SHEAR_X] == 0.0 &&
      tg_tool->trans_info[SHEAR_Y] == 0.0)
    {
      tg_tool->trans_info[ORIENTATION] = CLAY_ORIENTATION_UNKNOWN;
    }

  if ((ClayOrientationType) tg_tool->trans_info[ORIENTATION] == CLAY_ORIENTATION_HORIZONTAL)
    amount = tg_tool->trans_info[SHEAR_X];
  else
    amount = tg_tool->trans_info[SHEAR_Y];

  clay_matrix3_identity (transform);
  clay_transform_matrix_shear (transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tg_tool->trans_info[ORIENTATION],
                               amount);

  return TRUE;
}

static gchar *
clay_shear_tool_get_undo_desc (ClayTransformGridTool *tg_tool)
{
  gdouble x = tg_tool->trans_info[SHEAR_X];
  gdouble y = tg_tool->trans_info[SHEAR_Y];

  switch ((gint) tg_tool->trans_info[ORIENTATION])
    {
    case CLAY_ORIENTATION_HORIZONTAL:
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g"),
                              x);

    case CLAY_ORIENTATION_VERTICAL:
      return g_strdup_printf (C_("undo-type", "Shear vertically by %-3.3g"),
                              y);

    default:
      /* e.g. user entered numbers but no notification callback */
      return g_strdup_printf (C_("undo-type", "Shear horizontally by %-3.3g, vertically by %-3.3g"),
                              x, y);
    }
}

static void
clay_shear_tool_dialog (ClayTransformGridTool *tg_tool)
{
  ClayShearTool *shear = CLAY_SHEAR_TOOL (tg_tool);
  GtkWidget     *vbox;
  GtkWidget     *scale;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (tg_tool->gui)), vbox,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  shear->x_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = clay_spin_scale_new (shear->x_adj, _("Shear magnitude _X"), 0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (shear->x_adj, "value-changed",
                    G_CALLBACK (shear_x_mag_changed),
                    tg_tool);

  shear->y_adj = gtk_adjustment_new (0, -65536, 65536, 1, 10, 0);
  scale = clay_spin_scale_new (shear->y_adj, _("Shear magnitude _Y"), 0);
  clay_spin_scale_set_scale_limits (CLAY_SPIN_SCALE (scale), -1000, 1000);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale, TRUE);

  g_signal_connect (shear->y_adj, "value-changed",
                    G_CALLBACK (shear_y_mag_changed),
                    tg_tool);
}

static void
clay_shear_tool_dialog_update (ClayTransformGridTool *tg_tool)
{
  ClayShearTool *shear = CLAY_SHEAR_TOOL (tg_tool);

  gtk_adjustment_set_value (shear->x_adj, tg_tool->trans_info[SHEAR_X]);
  gtk_adjustment_set_value (shear->y_adj, tg_tool->trans_info[SHEAR_Y]);
}

static void
clay_shear_tool_prepare (ClayTransformGridTool *tg_tool)
{
  tg_tool->trans_info[ORIENTATION] = CLAY_ORIENTATION_UNKNOWN;
  tg_tool->trans_info[SHEAR_X]     = 0.0;
  tg_tool->trans_info[SHEAR_Y]     = 0.0;
}

static ClayToolWidget *
clay_shear_tool_get_widget (ClayTransformGridTool *tg_tool)
{
  ClayTool          *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayDisplayShell  *shell   = clay_display_get_shell (tool->display);
  ClayToolWidget    *widget;

  widget = clay_tool_shear_grid_new (shell,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2,
                                     tr_tool->y2,
                                     tg_tool->trans_info[ORIENTATION],
                                     tg_tool->trans_info[SHEAR_X],
                                     tg_tool->trans_info[SHEAR_Y]);

  g_object_set (widget,
                "inside-function",  CLAY_TRANSFORM_FUNCTION_SHEAR,
                "outside-function", CLAY_TRANSFORM_FUNCTION_SHEAR,
                "frompivot-shear",  TRUE,
                NULL);

  return widget;
}

static void
clay_shear_tool_update_widget (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "x1",          (gdouble) tr_tool->x1,
                "y1",          (gdouble) tr_tool->y1,
                "x2",          (gdouble) tr_tool->x2,
                "y2",          (gdouble) tr_tool->y2,
                "orientation", (gint) tg_tool->trans_info[ORIENTATION],
                "shear-x",     tg_tool->trans_info[SHEAR_X],
                "shear-y",     tg_tool->trans_info[SHEAR_Y],
                NULL);
}

static void
clay_shear_tool_widget_changed (ClayTransformGridTool *tg_tool)
{
  ClayOrientationType orientation;

  g_object_get (tg_tool->widget,
                "orientation", &orientation,
                "shear-x",     &tg_tool->trans_info[SHEAR_X],
                "shear-y",     &tg_tool->trans_info[SHEAR_Y],
                NULL);

  tg_tool->trans_info[ORIENTATION] = orientation;

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
shear_x_mag_changed (GtkAdjustment         *adj,
                     ClayTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_X])
    {
      ClayTool          *tool    = CLAY_TOOL (tg_tool);
      ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = CLAY_ORIENTATION_HORIZONTAL;

      tg_tool->trans_info[SHEAR_X] = value;
      tg_tool->trans_info[SHEAR_Y] = 0.0;  /* can only shear in one axis */

      clay_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      clay_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}

static void
shear_y_mag_changed (GtkAdjustment         *adj,
                     ClayTransformGridTool *tg_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tg_tool->trans_info[SHEAR_Y])
    {
      ClayTool          *tool    = CLAY_TOOL (tg_tool);
      ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

      tg_tool->trans_info[ORIENTATION] = CLAY_ORIENTATION_VERTICAL;

      tg_tool->trans_info[SHEAR_Y] = value;
      tg_tool->trans_info[SHEAR_X] = 0.0;  /* can only shear in one axis */

      clay_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

      clay_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
}
