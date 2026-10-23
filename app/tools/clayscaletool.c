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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay-transform-utils.h"
#include "core/clayimage.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claysizebox.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-transform.h"
#include "display/claytoolgui.h"
#include "display/claytooltransformgrid.h"

#include "clayscaletool.h"
#include "claytoolcontrol.h"
#include "claytransformgridoptions.h"

#include "clay-intl.h"


#define EPSILON 1e-6


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1
};


/*  local function prototypes  */

static gboolean         clay_scale_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                                        ClayMatrix3           *transform);
static void             clay_scale_tool_matrix_to_info (ClayTransformGridTool *tg_tool,
                                                        const ClayMatrix3     *transform);
static gchar          * clay_scale_tool_get_undo_desc  (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_dialog         (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_dialog_update  (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_prepare        (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_readjust       (ClayTransformGridTool *tg_tool);
static ClayToolWidget * clay_scale_tool_get_widget     (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_update_widget  (ClayTransformGridTool *tg_tool);
static void             clay_scale_tool_widget_changed (ClayTransformGridTool *tg_tool);

static void             clay_scale_tool_size_notify    (GtkWidget             *box,
                                                        GParamSpec            *pspec,
                                                        ClayTransformGridTool *tg_tool);


G_DEFINE_TYPE (ClayScaleTool, clay_scale_tool, CLAY_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class clay_scale_tool_parent_class


void
clay_scale_tool_register (ClayToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (CLAY_TYPE_SCALE_TOOL,
                CLAY_TYPE_TRANSFORM_GRID_OPTIONS,
                clay_transform_grid_options_gui,
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-scale-tool",
                _("Scale"),
                _("Scale Tool: Scale the layer, selection or path"),
                N_("_Scale"), "<shift>S",
                NULL, CLAY_HELP_TOOL_SCALE,
                CLAY_ICON_TOOL_SCALE,
                data);
}

static void
clay_scale_tool_class_init (ClayScaleToolClass *klass)
{
  ClayTransformToolClass     *tr_class = CLAY_TRANSFORM_TOOL_CLASS (klass);
  ClayTransformGridToolClass *tg_class = CLAY_TRANSFORM_GRID_TOOL_CLASS (klass);

  tg_class->info_to_matrix  = clay_scale_tool_info_to_matrix;
  tg_class->matrix_to_info  = clay_scale_tool_matrix_to_info;
  tg_class->get_undo_desc   = clay_scale_tool_get_undo_desc;
  tg_class->dialog          = clay_scale_tool_dialog;
  tg_class->dialog_update   = clay_scale_tool_dialog_update;
  tg_class->prepare         = clay_scale_tool_prepare;
  tg_class->readjust        = clay_scale_tool_readjust;
  tg_class->get_widget      = clay_scale_tool_get_widget;
  tg_class->update_widget   = clay_scale_tool_update_widget;
  tg_class->widget_changed  = clay_scale_tool_widget_changed;

  tr_class->undo_desc       = C_("undo-type", "Scale");
  tr_class->progress_text   = _("Scaling");
  tg_class->ok_button_label = _("_Scale");
}

static void
clay_scale_tool_init (ClayScaleTool *scale_tool)
{
  ClayTool *tool  = CLAY_TOOL (scale_tool);

  clay_tool_control_set_tool_cursor (tool->control, CLAY_TOOL_CURSOR_RESIZE);
}

static gboolean
clay_scale_tool_info_to_matrix  (ClayTransformGridTool *tg_tool,
                                 ClayMatrix3           *transform)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  clay_matrix3_identity (transform);
  clay_transform_matrix_scale (transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tg_tool->trans_info[X0],
                               tg_tool->trans_info[Y0],
                               tg_tool->trans_info[X1] - tg_tool->trans_info[X0],
                               tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  return TRUE;
}

static void
clay_scale_tool_matrix_to_info  (ClayTransformGridTool *tg_tool,
                                 const ClayMatrix3     *transform)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  gdouble            x;
  gdouble            y;
  gdouble            w;
  gdouble            h;

  x = transform->coeff[0][2];
  y = transform->coeff[1][2];
  w = transform->coeff[0][0];
  h = transform->coeff[1][1];

  tg_tool->trans_info[X0] = x + w * tr_tool->x1;
  tg_tool->trans_info[Y0] = y + h * tr_tool->y1;
  tg_tool->trans_info[X1] = tg_tool->trans_info[X0] +
                            w * (tr_tool->x2 - tr_tool->x1);
  tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] +
                            h * (tr_tool->y2 - tr_tool->y1);
}

static gchar *
clay_scale_tool_get_undo_desc (ClayTransformGridTool *tg_tool)
{
  gint width;
  gint height;

  width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
  height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  return g_strdup_printf (C_("undo-type", "Scale to %d x %d"),
                          width, height);
}

static void
clay_scale_tool_dialog (ClayTransformGridTool *tg_tool)
{
}

static void
clay_scale_tool_dialog_update (ClayTransformGridTool *tg_tool)
{
  ClayTransformGridOptions *options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  gint                      width;
  gint                      height;

  width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
  height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

  g_object_set (CLAY_SCALE_TOOL (tg_tool)->box,
                "width",       width,
                "height",      height,
                "keep-aspect", options->constrain_scale,
                NULL);
}

static void
clay_scale_tool_prepare (ClayTransformGridTool *tg_tool)
{
  ClayScaleTool            *scale   = CLAY_SCALE_TOOL (tg_tool);
  ClayTransformTool        *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayTransformGridOptions *options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  ClayDisplay              *display = CLAY_TOOL (tg_tool)->display;
  gdouble                   xres;
  gdouble                   yres;

  tg_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1] = (gdouble) tr_tool->y2;

  clay_image_get_resolution (clay_display_get_image (display),
                             &xres, &yres);

  if (scale->box)
    {
      g_signal_handlers_disconnect_by_func (scale->box,
                                            clay_scale_tool_size_notify,
                                            tg_tool);
      gtk_widget_destroy (scale->box);
    }

  /*  Need to create a new ClaySizeBox widget because the initial
   *  width and height is what counts as 100%.
   */
  scale->box =
    g_object_new (CLAY_TYPE_SIZE_BOX,
                  "width",       tr_tool->x2 - tr_tool->x1,
                  "height",      tr_tool->y2 - tr_tool->y1,
                  "keep-aspect", options->constrain_scale,
                  "unit",        clay_display_get_shell (display)->unit,
                  "xresolution", xres,
                  "yresolution", yres,
                  NULL);

  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (tg_tool->gui)),
                      scale->box, FALSE, FALSE, 0);
  gtk_widget_set_visible (scale->box, TRUE);

  g_signal_connect (scale->box, "notify",
                    G_CALLBACK (clay_scale_tool_size_notify),
                    tg_tool);
}

static void
clay_scale_tool_readjust (ClayTransformGridTool *tg_tool)
{
  ClayTool         *tool  = CLAY_TOOL (tg_tool);
  ClayDisplayShell *shell = clay_display_get_shell (tool->display);
  gdouble           x;
  gdouble           y;
  gdouble           r;

  x = shell->disp_width  / 2.0;
  y = shell->disp_height / 2.0;
  r = MAX (MIN (x, y) / G_SQRT2 -
           CLAY_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0,
           CLAY_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0);

  clay_display_shell_untransform_xy_f (shell,
                                       x, y,
                                       &x, &y);

  tg_tool->trans_info[X0] = RINT (x - FUNSCALEX (shell, r));
  tg_tool->trans_info[Y0] = RINT (y - FUNSCALEY (shell, r));
  tg_tool->trans_info[X1] = RINT (x + FUNSCALEX (shell, r));
  tg_tool->trans_info[Y1] = RINT (y + FUNSCALEY (shell, r));
}

static ClayToolWidget *
clay_scale_tool_get_widget (ClayTransformGridTool *tg_tool)
{
  ClayTool          *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayDisplayShell  *shell   = clay_display_get_shell (tool->display);
  ClayToolWidget    *widget;

  widget = clay_tool_transform_grid_new (shell,
                                         &tr_tool->transform,
                                         tr_tool->x1,
                                         tr_tool->y1,
                                         tr_tool->x2,
                                         tr_tool->y2);

  g_object_set (widget,
                "inside-function",    CLAY_TRANSFORM_FUNCTION_SCALE,
                "outside-function",   CLAY_TRANSFORM_FUNCTION_SCALE,
                "use-corner-handles", TRUE,
                "use-side-handles",   TRUE,
                "use-center-handle",  TRUE,
                NULL);

  return widget;
}

static void
clay_scale_tool_update_widget (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (
    tg_tool->widget,
    "x1", (gdouble) tr_tool->x1,
    "y1", (gdouble) tr_tool->y1,
    "x2", (gdouble) tr_tool->x2,
    "y2", (gdouble) tr_tool->y2,
    NULL);
}

static void
clay_scale_tool_widget_changed (ClayTransformGridTool *tg_tool)
{
  ClayTool          *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayMatrix3       *transform;
  gdouble            x0, y0;
  gdouble            x1, y1;
  gint               width, height;

  g_object_get (tg_tool->widget,
                "transform", &transform,
                NULL);

  clay_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &x0,         &y0);
  clay_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &x1,         &y1);

  g_free (transform);

  width  = ROUND (x1 - x0);
  height = ROUND (y1 - y0);

  if (width > 0)
    {
      tg_tool->trans_info[X0] = x0;
      tg_tool->trans_info[X1] = x1;
    }
  else if (fabs (x0 - tg_tool->trans_info[X0]) < EPSILON)
    {
      tg_tool->trans_info[X1] = tg_tool->trans_info[X0] + 1.0;
    }
  else if (fabs (x1 - tg_tool->trans_info[X1]) < EPSILON)
    {
      tg_tool->trans_info[X0] = tg_tool->trans_info[X1] - 1.0;
    }
  else
    {
      tg_tool->trans_info[X0] = (x0 + x1) / 2.0 - 0.5;
      tg_tool->trans_info[X1] = (x0 + x1) / 2.0 + 0.5;
    }

  if (height > 0)
    {
      tg_tool->trans_info[Y0] = y0;
      tg_tool->trans_info[Y1] = y1;
    }
  else if (fabs (y0 - tg_tool->trans_info[Y0]) < EPSILON)
    {
      tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] + 1.0;
    }
  else if (fabs (y1 - tg_tool->trans_info[Y1]) < EPSILON)
    {
      tg_tool->trans_info[Y0] = tg_tool->trans_info[Y1] - 1.0;
    }
  else
    {
      tg_tool->trans_info[Y0] = (y0 + y1) / 2.0 - 0.5;
      tg_tool->trans_info[Y1] = (y0 + y1) / 2.0 + 0.5;
    }

  if (width <= 0 || height <= 0)
    clay_transform_tool_recalc_matrix (tr_tool, tool->display);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
clay_scale_tool_size_notify (GtkWidget             *box,
                             GParamSpec            *pspec,
                             ClayTransformGridTool *tg_tool)
{
  ClayTransformGridOptions *options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);

  if (! strcmp (pspec->name, "width") ||
      ! strcmp (pspec->name, "height"))
    {
      gint width;
      gint height;
      gint old_width;
      gint old_height;

      g_object_get (box,
                    "width",  &width,
                    "height", &height,
                    NULL);

      old_width  = ROUND (tg_tool->trans_info[X1] - tg_tool->trans_info[X0]);
      old_height = ROUND (tg_tool->trans_info[Y1] - tg_tool->trans_info[Y0]);

      if ((width != old_width) || (height != old_height))
        {
          ClayTool          *tool    = CLAY_TOOL (tg_tool);
          ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

          if (options->frompivot_scale)
            {
              gdouble center_x;
              gdouble center_y;

              center_x = (tg_tool->trans_info[X0] +
                          tg_tool->trans_info[X1]) / 2.0;
              center_y = (tg_tool->trans_info[Y0] +
                          tg_tool->trans_info[Y1]) / 2.0;

              tg_tool->trans_info[X0] = center_x - width  / 2.0;
              tg_tool->trans_info[Y0] = center_y - height / 2.0;
              tg_tool->trans_info[X1] = center_x + width  / 2.0;
              tg_tool->trans_info[Y1] = center_y + height / 2.0;
            }
          else
            {
              tg_tool->trans_info[X1] = tg_tool->trans_info[X0] + width;
              tg_tool->trans_info[Y1] = tg_tool->trans_info[Y0] + height;
            }

          clay_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

          clay_transform_tool_recalc_matrix (tr_tool, tool->display);
        }
    }
  else if (! strcmp (pspec->name, "keep-aspect"))
    {
      gboolean constrain;

      g_object_get (box,
                    "keep-aspect", &constrain,
                    NULL);

      if (constrain != options->constrain_scale)
        {
          gint width;
          gint height;

          g_object_get (box,
                        "width",  &width,
                        "height", &height,
                        NULL);

          g_object_set (options,
                        "constrain-scale", constrain,
                        NULL);
        }
    }
}
