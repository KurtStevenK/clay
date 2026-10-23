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
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolhandlegrid.h"
#include "display/claytoolgui.h"

#include "clayhandletransformoptions.h"
#include "clayhandletransformtool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


/* the transformation is defined by 8 points:
 *
 * 4 points on the original image and 4 corresponding points on the
 * transformed image. The first N_HANDLES points on the transformed
 * image are visible as handles.
 *
 * For these handles, the constants TRANSFORM_HANDLE_N,
 * TRANSFORM_HANDLE_S, TRANSFORM_HANDLE_E and TRANSFORM_HANDLE_W are
 * used. Actually, it makes no sense to name the handles with north,
 * south, east, and west.  But this way, we don't need to define even
 * more enum constants.
 */

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
  OX0,
  OY0,
  OX1,
  OY1,
  OX2,
  OY2,
  OX3,
  OY3,
  N_HANDLES
};


/*  local function prototypes  */

static void             clay_handle_transform_tool_modifier_key   (ClayTool                 *tool,
                                                                   GdkModifierType           key,
                                                                   gboolean                  press,
                                                                   GdkModifierType           state,
                                                                   ClayDisplay              *display);

static void             clay_handle_transform_tool_matrix_to_info (ClayTransformGridTool    *tg_tool,
                                                                   const ClayMatrix3        *transform);
static void             clay_handle_transform_tool_prepare        (ClayTransformGridTool    *tg_tool);
static ClayToolWidget * clay_handle_transform_tool_get_widget     (ClayTransformGridTool    *tg_tool);
static void             clay_handle_transform_tool_update_widget  (ClayTransformGridTool    *tg_tool);
static void             clay_handle_transform_tool_widget_changed (ClayTransformGridTool    *tg_tool);

static void             clay_handle_transform_tool_info_to_points (ClayGenericTransformTool *generic);


G_DEFINE_TYPE (ClayHandleTransformTool, clay_handle_transform_tool,
               CLAY_TYPE_GENERIC_TRANSFORM_TOOL)

#define parent_class clay_handle_transform_tool_parent_class


void
clay_handle_transform_tool_register (ClayToolRegisterCallback  callback,
                                     gpointer                  data)
{
  (* callback) (CLAY_TYPE_HANDLE_TRANSFORM_TOOL,
                CLAY_TYPE_HANDLE_TRANSFORM_OPTIONS,
                clay_handle_transform_options_gui,
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-handle-transform-tool",
                _("Handle Transform"),
                _("Handle Transform Tool: "
                  "Deform the layer, selection or path with handles"),
                N_("_Handle Transform"), "<shift>L",
                NULL, CLAY_HELP_TOOL_HANDLE_TRANSFORM,
                CLAY_ICON_TOOL_HANDLE_TRANSFORM,
                data);
}

static void
clay_handle_transform_tool_class_init (ClayHandleTransformToolClass *klass)
{
  ClayToolClass                 *tool_class    = CLAY_TOOL_CLASS (klass);
  ClayTransformToolClass        *tr_class   = CLAY_TRANSFORM_TOOL_CLASS (klass);
  ClayTransformGridToolClass    *tg_class   = CLAY_TRANSFORM_GRID_TOOL_CLASS (klass);
  ClayGenericTransformToolClass *generic_class = CLAY_GENERIC_TRANSFORM_TOOL_CLASS (klass);

  tool_class->modifier_key      = clay_handle_transform_tool_modifier_key;

  tg_class->matrix_to_info      = clay_handle_transform_tool_matrix_to_info;
  tg_class->prepare             = clay_handle_transform_tool_prepare;
  tg_class->get_widget          = clay_handle_transform_tool_get_widget;
  tg_class->update_widget       = clay_handle_transform_tool_update_widget;
  tg_class->widget_changed      = clay_handle_transform_tool_widget_changed;

  generic_class->info_to_points = clay_handle_transform_tool_info_to_points;

  tr_class->undo_desc           = C_("undo-type", "Handle transform");
  tr_class->progress_text       = _("Handle transformation");
}

static void
clay_handle_transform_tool_init (ClayHandleTransformTool *ht_tool)
{
  ht_tool->saved_handle_mode = CLAY_HANDLE_MODE_ADD_TRANSFORM;
}

static void
clay_handle_transform_tool_modifier_key (ClayTool        *tool,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state,
                                         ClayDisplay     *display)
{
  ClayHandleTransformTool    *ht_tool = CLAY_HANDLE_TRANSFORM_TOOL (tool);
  ClayHandleTransformOptions *options;
  GdkModifierType             shift   = clay_get_extend_selection_mask ();
  GdkModifierType             ctrl    = clay_get_constrain_behavior_mask ();
  ClayTransformHandleMode     handle_mode;

  options = CLAY_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tool);

  handle_mode = options->handle_mode;

  if (press)
    {
      if (key == (state & (shift | ctrl)))
        {
          /*  first modifier pressed  */
          ht_tool->saved_handle_mode = options->handle_mode;
        }
    }
  else
    {
      if (! (state & (shift | ctrl)))
        {
          /*  last modifier released  */
          handle_mode = ht_tool->saved_handle_mode;
        }
    }

  if (state & shift)
    {
      handle_mode = CLAY_HANDLE_MODE_MOVE;
    }
  else if (state & ctrl)
    {
      handle_mode = CLAY_HANDLE_MODE_REMOVE;
    }

  if (handle_mode != options->handle_mode)
    {
      g_object_set (options,
                    "handle-mode", handle_mode,
                    NULL);
    }

  CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                state, display);
}

static void
clay_handle_transform_tool_matrix_to_info (ClayTransformGridTool *tg_tool,
                                           const ClayMatrix3     *transform)
{
  clay_matrix3_transform_point (transform,
                                tg_tool->trans_info[OX0],
                                tg_tool->trans_info[OY0],
                                &tg_tool->trans_info[X0],
                                &tg_tool->trans_info[Y0]);
  clay_matrix3_transform_point (transform,
                                tg_tool->trans_info[OX1],
                                tg_tool->trans_info[OY1],
                                &tg_tool->trans_info[X1],
                                &tg_tool->trans_info[Y1]);
  clay_matrix3_transform_point (transform,
                                tg_tool->trans_info[OX2],
                                tg_tool->trans_info[OY2],
                                &tg_tool->trans_info[X2],
                                &tg_tool->trans_info[Y2]);
  clay_matrix3_transform_point (transform,
                                tg_tool->trans_info[OX3],
                                tg_tool->trans_info[OY3],
                                &tg_tool->trans_info[X3],
                                &tg_tool->trans_info[Y3]);
}

static void
clay_handle_transform_tool_prepare (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->prepare (tg_tool);

  tg_tool->trans_info[X0]        = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y0]        = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X1]        = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y1]        = (gdouble) tr_tool->y1;
  tg_tool->trans_info[X2]        = (gdouble) tr_tool->x1;
  tg_tool->trans_info[Y2]        = (gdouble) tr_tool->y2;
  tg_tool->trans_info[X3]        = (gdouble) tr_tool->x2;
  tg_tool->trans_info[Y3]        = (gdouble) tr_tool->y2;
  tg_tool->trans_info[OX0]       = (gdouble) tr_tool->x1;
  tg_tool->trans_info[OY0]       = (gdouble) tr_tool->y1;
  tg_tool->trans_info[OX1]       = (gdouble) tr_tool->x2;
  tg_tool->trans_info[OY1]       = (gdouble) tr_tool->y1;
  tg_tool->trans_info[OX2]       = (gdouble) tr_tool->x1;
  tg_tool->trans_info[OY2]       = (gdouble) tr_tool->y2;
  tg_tool->trans_info[OX3]       = (gdouble) tr_tool->x2;
  tg_tool->trans_info[OY3]       = (gdouble) tr_tool->y2;
  tg_tool->trans_info[N_HANDLES] = 0;
}

static ClayToolWidget *
clay_handle_transform_tool_get_widget (ClayTransformGridTool *tg_tool)
{
  ClayTool                   *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool          *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayHandleTransformOptions *options;
  ClayDisplayShell           *shell   = clay_display_get_shell (tool->display);
  ClayToolWidget             *widget;

  options = CLAY_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);

  widget = clay_tool_handle_grid_new (shell,
                                      tr_tool->x1,
                                      tr_tool->y1,
                                      tr_tool->x2,
                                      tr_tool->y2);

  g_object_set (widget,
                "n-handles", (gint) tg_tool->trans_info[N_HANDLES],
                "orig-x1",   tg_tool->trans_info[OX0],
                "orig-y1",   tg_tool->trans_info[OY0],
                "orig-x2",   tg_tool->trans_info[OX1],
                "orig-y2",   tg_tool->trans_info[OY1],
                "orig-x3",   tg_tool->trans_info[OX2],
                "orig-y3",   tg_tool->trans_info[OY2],
                "orig-x4",   tg_tool->trans_info[OX3],
                "orig-y4",   tg_tool->trans_info[OY3],
                "trans-x1",  tg_tool->trans_info[X0],
                "trans-y1",  tg_tool->trans_info[Y0],
                "trans-x2",  tg_tool->trans_info[X1],
                "trans-y2",  tg_tool->trans_info[Y1],
                "trans-x3",  tg_tool->trans_info[X2],
                "trans-y3",  tg_tool->trans_info[Y2],
                "trans-x4",  tg_tool->trans_info[X3],
                "trans-y4",  tg_tool->trans_info[Y3],
                NULL);

  g_object_bind_property (G_OBJECT (options), "handle-mode",
                          G_OBJECT (widget),  "handle-mode",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  return widget;
}

static void
clay_handle_transform_tool_update_widget (ClayTransformGridTool *tg_tool)
{
  ClayMatrix3 transform;
  gboolean    transform_valid;

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  transform_valid = clay_transform_grid_tool_info_to_matrix (tg_tool,
                                                             &transform);

  g_object_set (tg_tool->widget,
                "show-guides", transform_valid,
                "n-handles",   (gint) tg_tool->trans_info[N_HANDLES],
                "orig-x1",     tg_tool->trans_info[OX0],
                "orig-y1",     tg_tool->trans_info[OY0],
                "orig-x2",     tg_tool->trans_info[OX1],
                "orig-y2",     tg_tool->trans_info[OY1],
                "orig-x3",     tg_tool->trans_info[OX2],
                "orig-y3",     tg_tool->trans_info[OY2],
                "orig-x4",     tg_tool->trans_info[OX3],
                "orig-y4",     tg_tool->trans_info[OY3],
                "trans-x1",    tg_tool->trans_info[X0],
                "trans-y1",    tg_tool->trans_info[Y0],
                "trans-x2",    tg_tool->trans_info[X1],
                "trans-y2",    tg_tool->trans_info[Y1],
                "trans-x3",    tg_tool->trans_info[X2],
                "trans-y3",    tg_tool->trans_info[Y2],
                "trans-x4",    tg_tool->trans_info[X3],
                "trans-y4",    tg_tool->trans_info[Y3],
                NULL);
}

static void
clay_handle_transform_tool_widget_changed (ClayTransformGridTool *tg_tool)
{
  gint n_handles;

  g_object_get (tg_tool->widget,
                "n-handles", &n_handles,
                "orig-x1",   &tg_tool->trans_info[OX0],
                "orig-y1",   &tg_tool->trans_info[OY0],
                "orig-x2",   &tg_tool->trans_info[OX1],
                "orig-y2",   &tg_tool->trans_info[OY1],
                "orig-x3",   &tg_tool->trans_info[OX2],
                "orig-y3",   &tg_tool->trans_info[OY2],
                "orig-x4",   &tg_tool->trans_info[OX3],
                "orig-y4",   &tg_tool->trans_info[OY3],
                "trans-x1",  &tg_tool->trans_info[X0],
                "trans-y1",  &tg_tool->trans_info[Y0],
                "trans-x2",  &tg_tool->trans_info[X1],
                "trans-y2",  &tg_tool->trans_info[Y1],
                "trans-x3",  &tg_tool->trans_info[X2],
                "trans-y3",  &tg_tool->trans_info[Y2],
                "trans-x4",  &tg_tool->trans_info[X3],
                "trans-y4",  &tg_tool->trans_info[Y3],
                NULL);

  tg_tool->trans_info[N_HANDLES] = n_handles;

  CLAY_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
clay_handle_transform_tool_info_to_points (ClayGenericTransformTool *generic)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (generic);

  generic->input_points[0]  = (ClayVector2) {tg_tool->trans_info[OX0],
                                             tg_tool->trans_info[OY0]};
  generic->input_points[1]  = (ClayVector2) {tg_tool->trans_info[OX1],
                                             tg_tool->trans_info[OY1]};
  generic->input_points[2]  = (ClayVector2) {tg_tool->trans_info[OX2],
                                             tg_tool->trans_info[OY2]};
  generic->input_points[3]  = (ClayVector2) {tg_tool->trans_info[OX3],
                                             tg_tool->trans_info[OY3]};

  generic->output_points[0] = (ClayVector2) {tg_tool->trans_info[X0],
                                             tg_tool->trans_info[Y0]};
  generic->output_points[1] = (ClayVector2) {tg_tool->trans_info[X1],
                                             tg_tool->trans_info[Y1]};
  generic->output_points[2] = (ClayVector2) {tg_tool->trans_info[X2],
                                             tg_tool->trans_info[Y2]};
  generic->output_points[3] = (ClayVector2) {tg_tool->trans_info[X3],
                                             tg_tool->trans_info[Y3]};
}
