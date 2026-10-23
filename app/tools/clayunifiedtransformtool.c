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

#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-transform.h"
#include "display/claytoolgui.h"
#include "display/claytooltransformgrid.h"

#include "claytoolcontrol.h"
#include "claytransformgridoptions.h"
#include "clayunifiedtransformtool.h"

#include "clay-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3,
  PIVOT_X,
  PIVOT_Y,
};


/*  local function prototypes  */

static void             clay_unified_transform_tool_matrix_to_info (ClayTransformGridTool    *tg_tool,
                                                                    const ClayMatrix3        *transform);
static void             clay_unified_transform_tool_apply_info     (ClayTransformGridTool    *tg_tool,
                                                                    const TransInfo           info);
static void             clay_unified_transform_tool_prepare        (ClayTransformGridTool    *tg_tool);
static void             clay_unified_transform_tool_readjust       (ClayTransformGridTool    *tg_tool);
static ClayToolWidget * clay_unified_transform_tool_get_widget     (ClayTransformGridTool    *tg_tool);
static void             clay_unified_transform_tool_update_widget  (ClayTransformGridTool    *tg_tool);
static void             clay_unified_transform_tool_widget_changed (ClayTransformGridTool    *tg_tool);

static void             clay_unified_transform_tool_info_to_points (ClayGenericTransformTool *generic);


G_DEFINE_TYPE (ClayUnifiedTransformTool, clay_unified_transform_tool,
               CLAY_TYPE_GENERIC_TRANSFORM_TOOL)

#define parent_class clay_unified_transform_tool_parent_class


void
clay_unified_transform_tool_register (ClayToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (CLAY_TYPE_UNIFIED_TRANSFORM_TOOL,
                CLAY_TYPE_TRANSFORM_GRID_OPTIONS,
                clay_transform_grid_options_gui,
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-unified-transform-tool",
                _("Unified Transform"),
                _("Unified Transform Tool: "
                  "Transform the layer, selection or path"),
                N_("_Unified Transform"), "<shift>T",
                NULL, CLAY_HELP_TOOL_UNIFIED_TRANSFORM,
                CLAY_ICON_TOOL_UNIFIED_TRANSFORM,
                data);
}

static void
clay_unified_transform_tool_class_init (ClayUnifiedTransformToolClass *klass)
{
  ClayTransformToolClass        *tr_class      = CLAY_TRANSFORM_TOOL_CLASS (klass);
  ClayTransformGridToolClass    *tg_class      = CLAY_TRANSFORM_GRID_TOOL_CLASS (klass);
  ClayGenericTransformToolClass *generic_class = CLAY_GENERIC_TRANSFORM_TOOL_CLASS (klass);

  tg_class->matrix_to_info      = clay_unified_transform_tool_matrix_to_info;
  tg_class->apply_info          = clay_unified_transform_tool_apply_info;
  tg_class->prepare             = clay_unified_transform_tool_prepare;
  tg_class->readjust            = clay_unified_transform_tool_readjust;
  tg_class->get_widget          = clay_unified_transform_tool_get_widget;
  tg_class->update_widget       = clay_unified_transform_tool_update_widget;
  tg_class->widget_changed      = clay_unified_transform_tool_widget_changed;

  generic_class->info_to_points = clay_unified_transform_tool_info_to_points;

  tr_class->undo_desc           = C_("undo-type", "Unified Transform");
  tr_class->progress_text       = _("Unified transform");
}

static void
clay_unified_transform_tool_init (ClayUnifiedTransformTool *unified_tool)
{
}

static void
clay_unified_transform_tool_matrix_to_info (ClayTransformGridTool *tg_tool,
                                            const ClayMatrix3     *transform)
{
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);

  if (! tg_options->fixedpivot)
    {
      ClayMatrix3 transfer;

      clay_transform_grid_tool_info_to_matrix (tg_tool, &transfer);
      clay_matrix3_invert (&transfer);
      clay_matrix3_mult (transform, &transfer);

      clay_matrix3_transform_point (&transfer,
                                    tg_tool->trans_info[PIVOT_X],
                                    tg_tool->trans_info[PIVOT_Y],
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);
    }

  clay_matrix3_transform_point (transform,
                                tr_tool->x1,
                                tr_tool->y1,
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x2,
                                tr_tool->y1,
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x1,
                                tr_tool->y2,
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x2,
                                tr_tool->y2,
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);
}

static void
clay_unified_transform_tool_apply_info (ClayTransformGridTool *tg_tool,
                                        const TransInfo        info)
{
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);

  tg_tool->trans_info[X0] = info[X0];
  tg_tool->trans_info[Y0] = info[Y0];

  tg_tool->trans_info[X1] = info[X1];
  tg_tool->trans_info[Y1] = info[Y1];

  tg_tool->trans_info[X2] = info[X2];
  tg_tool->trans_info[Y2] = info[Y2];

  tg_tool->trans_info[X3] = info[X3];
  tg_tool->trans_info[Y3] = info[Y3];

  if (! tg_options->fixedpivot)
    {
      tg_tool->trans_info[PIVOT_X] = info[PIVOT_X];
      tg_tool->trans_info[PIVOT_Y] = info[PIVOT_Y];
    }
}

static void
clay_unified_transform_tool_prepare (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->prepare (tg_tool);

  tg_tool->trans_info[PIVOT_X] = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[PIVOT_Y] = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  tg_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1] = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X2] = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y2] = (gdouble) tr_tool->y2;
  tg_tool->trans_info[X3] = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y3] = (gdouble) tr_tool->y2;
}

static void
clay_unified_transform_tool_readjust (ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  ClayDisplayShell         *shell      = clay_display_get_shell (tool->display);
  gdouble                   x;
  gdouble                   y;
  gdouble                   r;

  x = shell->disp_width  / 2.0;
  y = shell->disp_height / 2.0;
  r = MAX (MIN (x, y) / G_SQRT2 -
           CLAY_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0,
           CLAY_TOOL_TRANSFORM_GRID_MAX_HANDLE_SIZE / 2.0);

  if (! tg_options->fixedpivot)
    {
      clay_display_shell_untransform_xy_f (shell,
                                           x, y,
                                           &tg_tool->trans_info[PIVOT_X],
                                           &tg_tool->trans_info[PIVOT_Y]);
    }

  clay_display_shell_untransform_xy_f (shell,
                                       x - r, y - r,
                                       &tg_tool->trans_info[X0],
                                       &tg_tool->trans_info[Y0]);
  clay_display_shell_untransform_xy_f (shell,
                                       x + r, y - r,
                                       &tg_tool->trans_info[X1],
                                       &tg_tool->trans_info[Y1]);
  clay_display_shell_untransform_xy_f (shell,
                                       x - r, y + r,
                                       &tg_tool->trans_info[X2],
                                       &tg_tool->trans_info[Y2]);
  clay_display_shell_untransform_xy_f (shell,
                                       x + r, y + r,
                                       &tg_tool->trans_info[X3],
                                       &tg_tool->trans_info[Y3]);
}

static ClayToolWidget *
clay_unified_transform_tool_get_widget (ClayTransformGridTool *tg_tool)
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
                "pivot-x",                 (tr_tool->x1 + tr_tool->x2) / 2.0,
                "pivot-y",                 (tr_tool->y1 + tr_tool->y2) / 2.0,
                "inside-function",         CLAY_TRANSFORM_FUNCTION_MOVE,
                "outside-function",        CLAY_TRANSFORM_FUNCTION_ROTATE,
                "use-corner-handles",      TRUE,
                "use-perspective-handles", TRUE,
                "use-side-handles",        TRUE,
                "use-shear-handles",       TRUE,
                "use-pivot-handle",        TRUE,
                NULL);

  return widget;
}

static void
clay_unified_transform_tool_update_widget (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  g_object_set (tg_tool->widget,
                "x1",      (gdouble) tr_tool->x1,
                "y1",      (gdouble) tr_tool->y1,
                "x2",      (gdouble) tr_tool->x2,
                "y2",      (gdouble) tr_tool->y2,
                "pivot-x", tg_tool->trans_info[PIVOT_X],
                "pivot-y", tg_tool->trans_info[PIVOT_Y],
                NULL);
}

static void
clay_unified_transform_tool_widget_changed (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayMatrix3       *transform;

  g_object_get (tg_tool->widget,
                "transform", &transform,
                "pivot-x",   &tg_tool->trans_info[PIVOT_X],
                "pivot-y",   &tg_tool->trans_info[PIVOT_Y],
                NULL);

  clay_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y1,
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y1,
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x1, tr_tool->y2,
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  clay_matrix3_transform_point (transform,
                                tr_tool->x2, tr_tool->y2,
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);

  g_free (transform);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
clay_unified_transform_tool_info_to_points (ClayGenericTransformTool *generic)
{
  ClayTransformTool     *tr_tool = CLAY_TRANSFORM_TOOL (generic);
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (generic);

  generic->input_points[0]  = (ClayVector2) {tr_tool->x1, tr_tool->y1};
  generic->input_points[1]  = (ClayVector2) {tr_tool->x2, tr_tool->y1};
  generic->input_points[2]  = (ClayVector2) {tr_tool->x1, tr_tool->y2};
  generic->input_points[3]  = (ClayVector2) {tr_tool->x2, tr_tool->y2};

  generic->output_points[0] = (ClayVector2) {tg_tool->trans_info[X0],
                                             tg_tool->trans_info[Y0]};
  generic->output_points[1] = (ClayVector2) {tg_tool->trans_info[X1],
                                             tg_tool->trans_info[Y1]};
  generic->output_points[2] = (ClayVector2) {tg_tool->trans_info[X2],
                                             tg_tool->trans_info[Y2]};
  generic->output_points[3] = (ClayVector2) {tg_tool->trans_info[X3],
                                             tg_tool->trans_info[Y3]};
}
