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
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "operations/clay-operation-config.h"

#include "core/claydata.h"
#include "core/claygradient.h"
#include "core/clay-gradients.h"
#include "core/clayimage.h"

#include "widgets/claycolorpanel.h"
#include "widgets/clayeditor.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolgui.h"
#include "display/claytoolline.h"

#include "claygradientoptions.h"
#include "claygradienttool.h"
#include "claygradienttool-editor.h"

#include "clay-intl.h"


#define EPSILON 2e-10


typedef enum
{
  DIRECTION_NONE,
  DIRECTION_LEFT,
  DIRECTION_RIGHT
} Direction;


typedef struct
{
  /* line endpoints at the beginning of the operation */
  gdouble       start_x;
  gdouble       start_y;
  gdouble       end_x;
  gdouble       end_y;

  /* copy of the gradient at the beginning of the operation, owned by the gradient
   * info, or NULL, if the gradient isn't affected
   */
  ClayGradient *gradient;

  /* handle added by the operation, or HANDLE_NONE */
  gint          added_handle;
  /* handle removed by the operation, or HANDLE_NONE */
  gint          removed_handle;
  /* selected handle at the end of the operation, or HANDLE_NONE */
  gint          selected_handle;
} GradientInfo;


/*  local function prototypes  */

static gboolean              clay_gradient_tool_editor_line_can_add_slider           (ClayToolLine          *line,
                                                                                      gdouble                value,
                                                                                      ClayGradientTool      *gradient_tool);
static gint                  clay_gradient_tool_editor_line_add_slider               (ClayToolLine          *line,
                                                                                      gdouble                value,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_line_prepare_to_remove_slider (ClayToolLine          *line,
                                                                                      gint                   slider,
                                                                                      gboolean               remove,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_line_remove_slider            (ClayToolLine          *line,
                                                                                      gint                   slider,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_line_selection_changed        (ClayToolLine          *line,
                                                                                      ClayGradientTool      *gradient_tool);
static gboolean              clay_gradient_tool_editor_line_handle_clicked           (ClayToolLine          *line,
                                                                                      gint                   handle,
                                                                                      GdkModifierType        state,
                                                                                      ClayButtonPressType    press_type,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_gui_response                  (ClayToolGui           *gui,
                                                                                      gint                   response_id,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_color_entry_color_clicked     (ClayColorButton       *button,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_color_entry_color_changed     (ClayColorButton       *button,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_color_entry_color_response    (ClayColorButton       *button,
                                                                                      ClayColorDialogState   state,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_color_entry_type_changed      (GtkComboBox           *combo,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_endpoint_se_value_changed     (ClaySizeEntry         *se,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_stop_se_value_changed         (ClaySizeEntry        *se,
                                                                                      ClayGradientTool     *gradient_tool);

static void                  clay_gradient_tool_editor_stop_delete_clicked           (GtkWidget             *button,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_midpoint_se_value_changed     (ClaySizeEntry        *se,
                                                                                      ClayGradientTool     *gradient_tool);

static void                  clay_gradient_tool_editor_midpoint_type_changed         (GtkComboBox           *combo,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_midpoint_color_changed        (GtkComboBox           *combo,
                                                                                      ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_midpoint_new_stop_clicked     (GtkWidget             *button,
                                                                                      ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_midpoint_center_clicked       (GtkWidget             *button,
                                                                                      ClayGradientTool      *gradient_tool);

static gboolean              clay_gradient_tool_editor_flush_idle                    (ClayGradientTool      *gradient_tool);

static gboolean              clay_gradient_tool_editor_is_gradient_editable          (ClayGradientTool      *gradient_tool);

static gboolean              clay_gradient_tool_editor_handle_is_endpoint            (ClayGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              clay_gradient_tool_editor_handle_is_stop                (ClayGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static gboolean              clay_gradient_tool_editor_handle_is_midpoint            (ClayGradientTool      *gradient_tool,
                                                                                      gint                   handle);
static ClayGradientSegment * clay_gradient_tool_editor_handle_get_segment            (ClayGradientTool      *gradient_tool,
                                                                                      gint                   handle);

static void                  clay_gradient_tool_editor_block_handlers                (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_unblock_handlers              (ClayGradientTool      *gradient_tool);
static gboolean              clay_gradient_tool_editor_are_handlers_blocked          (ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_freeze_gradient               (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_thaw_gradient                 (ClayGradientTool      *gradient_tool);

static gint                  clay_gradient_tool_editor_add_stop                      (ClayGradientTool      *gradient_tool,
                                                                                      gdouble                value);
static void                  clay_gradient_tool_editor_delete_stop                   (ClayGradientTool      *gradient_tool,
                                                                                      gint                   slider);
static gint                  clay_gradient_tool_editor_midpoint_to_stop              (ClayGradientTool      *gradient_tool,
                                                                                      gint                   slider);

static void                  clay_gradient_tool_editor_update_sliders                (ClayGradientTool      *gradient_tool);

static void                  clay_gradient_tool_editor_purge_gradient_history        (GSList               **stack);
static void                  clay_gradient_tool_editor_purge_gradient                (ClayGradientTool      *gradient_tool);

static GtkWidget           * clay_gradient_tool_editor_color_entry_new               (ClayGradientTool      *gradient_tool,
                                                                                      const gchar           *title,
                                                                                      Direction              direction,
                                                                                      GtkWidget             *chain_button,
                                                                                      GtkWidget            **color_panel,
                                                                                      GtkWidget            **type_combo);
static void                  clay_gradient_tool_editor_init_endpoint_gui             (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_init_stop_gui                 (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_init_midpoint_gui             (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_update_endpoint_gui           (ClayGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  clay_gradient_tool_editor_update_stop_gui               (ClayGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  clay_gradient_tool_editor_update_midpoint_gui           (ClayGradientTool      *gradient_tool,
                                                                                      gint                   selection);
static void                  clay_gradient_tool_editor_update_gui                    (ClayGradientTool      *gradient_tool);

static GradientInfo        * clay_gradient_tool_editor_gradient_info_new             (ClayGradientTool      *gradient_tool);
static void                  clay_gradient_tool_editor_gradient_info_free            (GradientInfo          *info);
static void                  clay_gradient_tool_editor_gradient_info_apply           (ClayGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info,
                                                                                      gboolean               set_selection);
static gboolean              clay_gradient_tool_editor_gradient_info_is_trivial      (ClayGradientTool      *gradient_tool,
                                                                                      const GradientInfo    *info);


/*  private functions  */


static gboolean
clay_gradient_tool_editor_line_can_add_slider (ClayToolLine  *line,
                                               gdouble        value,
                                               ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gdouble              offset  = options->offset / 100.0;

  return clay_gradient_tool_editor_is_gradient_editable (gradient_tool) &&
         value >= offset;
}

static gint
clay_gradient_tool_editor_line_add_slider (ClayToolLine     *line,
                                           gdouble           value,
                                           ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions    *paint_options = CLAY_PAINT_OPTIONS (options);
  gdouble              offset        = options->offset / 100.0;

  /* adjust slider value according to the offset */
  value = (value - offset) / (1.0 - offset);

  /* flip the slider value, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    value = 1.0 - value;

  return clay_gradient_tool_editor_add_stop (gradient_tool, value);
}

static void
clay_gradient_tool_editor_line_prepare_to_remove_slider (ClayToolLine     *line,
                                                         gint              slider,
                                                         gboolean          remove,
                                                         ClayGradientTool *gradient_tool)
{
  if (remove)
    {
      GradientInfo *info;
      ClayGradient *tentative_gradient;

      /* show a tentative gradient, demonstrating the result of actually
       * removing the slider
       */

      info = gradient_tool->undo_stack->data;

      if (info->added_handle == slider)
        {
          /* see comment in clay_gradient_tool_editor_delete_stop() */

          clay_assert (info->gradient != NULL);

          tentative_gradient = g_object_ref (info->gradient);
        }
      else
        {
          ClayGradientSegment *seg;
          gint                 i;

          tentative_gradient =
            CLAY_GRADIENT (clay_data_duplicate (CLAY_DATA (gradient_tool->gradient)));

          seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

          i = clay_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                          gradient_tool->gradient->segments,
                                                          seg) - 1;

          seg = clay_gradient_segment_get_nth (tentative_gradient->segments, i);

          clay_gradient_segment_range_merge (tentative_gradient,
                                             seg, seg->next, NULL, NULL);
        }

      clay_gradient_tool_set_tentative_gradient (gradient_tool, tentative_gradient);

      g_object_unref (tentative_gradient);
    }
  else
    {
      clay_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
    }
}

static void
clay_gradient_tool_editor_line_remove_slider (ClayToolLine     *line,
                                              gint              slider,
                                              ClayGradientTool *gradient_tool)
{
  clay_gradient_tool_editor_delete_stop (gradient_tool, slider);
  clay_gradient_tool_set_tentative_gradient (gradient_tool, NULL);
}

static void
clay_gradient_tool_editor_line_selection_changed (ClayToolLine     *line,
                                                  ClayGradientTool *gradient_tool)
{
  gint selection;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (gradient_tool->gui)
    {
      /* hide all color dialogs */
      clay_color_panel_dialog_response (
        CLAY_COLOR_PANEL (gradient_tool->endpoint_color_panel),
        CLAY_COLOR_DIALOG_OK);
      clay_color_panel_dialog_response (
        CLAY_COLOR_PANEL (gradient_tool->stop_left_color_panel),
        CLAY_COLOR_DIALOG_OK);
      clay_color_panel_dialog_response (
        CLAY_COLOR_PANEL (gradient_tool->stop_right_color_panel),
        CLAY_COLOR_DIALOG_OK);

      /* reset the stop colors chain button */
      if (clay_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
        {
          const ClayGradientSegment *seg;
          gboolean                   homogeneous;

          seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool,
                                                              selection);

          homogeneous = clay_color_is_perceptually_identical (seg->right_color, seg->next->left_color) &&
                        seg->right_color_type == seg->next->left_color_type;

          clay_chain_button_set_active (
            CLAY_CHAIN_BUTTON (gradient_tool->stop_chain_button), homogeneous);
        }
    }

  clay_gradient_tool_editor_update_gui (gradient_tool);
}

static gboolean
clay_gradient_tool_editor_line_handle_clicked (ClayToolLine        *line,
                                               gint                 handle,
                                               GdkModifierType      state,
                                               ClayButtonPressType  press_type,
                                               ClayGradientTool    *gradient_tool)
{
  if (clay_gradient_tool_editor_handle_is_midpoint (gradient_tool, handle))
    {
      if (press_type == CLAY_BUTTON_PRESS_DOUBLE &&
          clay_gradient_tool_editor_is_gradient_editable (gradient_tool))
        {
          gint stop;

          stop = clay_gradient_tool_editor_midpoint_to_stop (gradient_tool, handle);

          clay_tool_line_set_selection (line, stop);

          /* return FALSE, so that the new slider can be dragged immediately */
          return FALSE;
        }
    }

  return FALSE;
}


static void
clay_gradient_tool_editor_gui_response (ClayToolGui      *gui,
                                        gint              response_id,
                                        ClayGradientTool *gradient_tool)
{
  switch (response_id)
    {
    default:
      clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget),
                                    CLAY_TOOL_LINE_HANDLE_NONE);
      break;
    }
}

static void
clay_gradient_tool_editor_color_entry_color_clicked (ClayColorButton  *button,
                                                     ClayGradientTool *gradient_tool)
{
  clay_gradient_tool_editor_start_edit (gradient_tool);
}

static void
clay_gradient_tool_editor_color_entry_color_changed (ClayColorButton  *button,
                                                     ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions    *paint_options = CLAY_PAINT_OPTIONS (options);
  gint                 selection;
  GeglColor           *color;
  Direction            direction;
  GtkWidget           *chain_button;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  color = clay_color_button_get_color (button);

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                        "clay-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (button),
                                    "clay-gradient-tool-editor-chain-button");

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case CLAY_TOOL_LINE_HANDLE_START:
          selection = CLAY_TOOL_LINE_HANDLE_END;
          break;

        case CLAY_TOOL_LINE_HANDLE_END:
          selection = CLAY_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      g_clear_object (&seg->left_color);
      seg->left_color      = g_object_ref (color);
      seg->left_color_type = CLAY_GRADIENT_COLOR_FIXED;
      break;

    case CLAY_TOOL_LINE_HANDLE_END:
      g_clear_object (&seg->right_color);
      seg->right_color      = g_object_ref (color);
      seg->right_color_type = CLAY_GRADIENT_COLOR_FIXED;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           clay_chain_button_get_active (CLAY_CHAIN_BUTTON (chain_button))))
        {
          g_clear_object (&seg->right_color);
          seg->right_color      = gegl_color_duplicate (color);
          seg->right_color_type = CLAY_GRADIENT_COLOR_FIXED;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           clay_chain_button_get_active (CLAY_CHAIN_BUTTON (chain_button))))
        {
          g_clear_object (&seg->next->left_color);
          seg->next->left_color      = g_object_ref (color);
          seg->next->left_color_type = CLAY_GRADIENT_COLOR_FIXED;
        }
    }

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  g_object_unref (color);
}

static void
clay_gradient_tool_editor_color_entry_color_response (ClayColorButton      *button,
                                                      ClayColorDialogState  state,
                                                      ClayGradientTool     *gradient_tool)
{
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_color_entry_type_changed (GtkComboBox      *combo,
                                                    ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions    *paint_options = CLAY_PAINT_OPTIONS (options);
  gint                 selection;
  gint                 color_type;
  Direction            direction;
  GtkWidget           *chain_button;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (! clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo), &color_type))
    return;

  direction =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (combo),
                                        "clay-gradient-tool-editor-direction"));
  chain_button = g_object_get_data (G_OBJECT (combo),
                                    "clay-gradient-tool-editor-chain-button");

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case CLAY_TOOL_LINE_HANDLE_START:
          selection = CLAY_TOOL_LINE_HANDLE_END;
          break;

        case CLAY_TOOL_LINE_HANDLE_END:
          selection = CLAY_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      seg->left_color_type = color_type;
      break;

    case CLAY_TOOL_LINE_HANDLE_END:
      seg->right_color_type = color_type;
      break;

    default:
      if (direction == DIRECTION_LEFT ||
          (chain_button               &&
           clay_chain_button_get_active (CLAY_CHAIN_BUTTON (chain_button))))
        {
          seg->right_color_type = color_type;
        }

      if (direction == DIRECTION_RIGHT ||
          (chain_button                &&
           clay_chain_button_get_active (CLAY_CHAIN_BUTTON (chain_button))))
        {
          seg->next->left_color_type = color_type;
        }
    }

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_endpoint_se_value_changed (ClaySizeEntry    *se,
                                                     ClayGradientTool *gradient_tool)
{
  gint    selection;
  gdouble x;
  gdouble y;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (selection == CLAY_TOOL_LINE_HANDLE_NONE)
    return;

  x = clay_size_entry_get_refval (se, 0);
  y = clay_size_entry_get_refval (se, 1);

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_block_handlers (gradient_tool);

  switch (selection)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      g_object_set (gradient_tool->widget,
                    "x1", x,
                    "y1", y,
                    NULL);
      break;

    case CLAY_TOOL_LINE_HANDLE_END:
      g_object_set (gradient_tool->widget,
                    "x2", x,
                    "y2", y,
                    NULL);
      break;

    default:
      clay_assert_not_reached ();
    }

  clay_gradient_tool_editor_unblock_handlers (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_stop_se_value_changed (ClaySizeEntry    *se,
                                                 ClayGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (selection == CLAY_TOOL_LINE_HANDLE_NONE)
    return;

  value = clay_size_entry_get_refval (se, 0) / 100.0;

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  clay_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg, seg,
                                        seg->left, value);
  clay_gradient_segment_range_compress (gradient_tool->gradient,
                                        seg->next, seg->next,
                                        value, seg->next->right);

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_stop_delete_clicked (GtkWidget        *button,
                                               ClayGradientTool *gradient_tool)
{
  gint selection;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  clay_gradient_tool_editor_delete_stop (gradient_tool, selection);
}

static void
clay_gradient_tool_editor_midpoint_se_value_changed (ClaySizeEntry    *se,
                                                     ClayGradientTool *gradient_tool)
{
  gint                 selection;
  gdouble              value;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (selection == CLAY_TOOL_LINE_HANDLE_NONE)
    return;

  value = clay_size_entry_get_refval (se, 0) / 100.0;

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->middle = value;

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_midpoint_type_changed (GtkComboBox      *combo,
                                                 ClayGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 type;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (! clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo), &type))
    return;

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->type = type;

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_midpoint_color_changed (GtkComboBox      *combo,
                                                  ClayGradientTool *gradient_tool)
{
  gint                 selection;
  gint                 color;
  ClayGradientSegment *seg;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  if (! clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo), &color))
    return;

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  seg->color = color;

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static void
clay_gradient_tool_editor_midpoint_new_stop_clicked (GtkWidget        *button,
                                                     ClayGradientTool *gradient_tool)
{
  gint selection;
  gint stop;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  stop = clay_gradient_tool_editor_midpoint_to_stop (gradient_tool, selection);

  clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget), stop);
}

static void
clay_gradient_tool_editor_midpoint_center_clicked (GtkWidget        *button,
                                                   ClayGradientTool *gradient_tool)
{
  gint                 selection;
  ClayGradientSegment *seg;

  selection =
    clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  clay_gradient_segment_range_recenter_handles (gradient_tool->gradient, seg, seg);

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gboolean
clay_gradient_tool_editor_flush_idle (ClayGradientTool *gradient_tool)
{
  ClayDisplay *display = CLAY_TOOL (gradient_tool)->display;

  clay_image_flush (clay_display_get_image (display));

  gradient_tool->flush_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static gboolean
clay_gradient_tool_editor_is_gradient_editable (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return ! options->modify_active ||
         clay_data_is_writable (CLAY_DATA (gradient_tool->gradient));
}

static gboolean
clay_gradient_tool_editor_handle_is_endpoint (ClayGradientTool *gradient_tool,
                                              gint              handle)
{
  return handle == CLAY_TOOL_LINE_HANDLE_START ||
         handle == CLAY_TOOL_LINE_HANDLE_END;
}

static gboolean
clay_gradient_tool_editor_handle_is_stop (ClayGradientTool *gradient_tool,
                                          gint              handle)
{
  gint n_sliders;

  clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= 0 && handle < n_sliders / 2;
}

static gboolean
clay_gradient_tool_editor_handle_is_midpoint (ClayGradientTool *gradient_tool,
                                              gint              handle)
{
  gint n_sliders;

  clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget), &n_sliders);

  return handle >= n_sliders / 2;
}

static ClayGradientSegment *
clay_gradient_tool_editor_handle_get_segment (ClayGradientTool *gradient_tool,
                                              gint              handle)
{
  switch (handle)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      return gradient_tool->gradient->segments;

    case CLAY_TOOL_LINE_HANDLE_END:
      return clay_gradient_segment_get_last (gradient_tool->gradient->segments);

    default:
      {
        const ClayControllerSlider *sliders;
        gint                        n_sliders;
        gint                        seg_i;

        sliders = clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                              &n_sliders);

        clay_assert (handle >= 0 && handle < n_sliders);

        seg_i = GPOINTER_TO_INT (sliders[handle].data);

        return clay_gradient_segment_get_nth (gradient_tool->gradient->segments,
                                              seg_i);
      }
    }
}

static void
clay_gradient_tool_editor_block_handlers (ClayGradientTool *gradient_tool)
{
  gradient_tool->block_handlers_count++;
}

static void
clay_gradient_tool_editor_unblock_handlers (ClayGradientTool *gradient_tool)
{
  clay_assert (gradient_tool->block_handlers_count > 0);

  gradient_tool->block_handlers_count--;
}

static gboolean
clay_gradient_tool_editor_are_handlers_blocked (ClayGradientTool *gradient_tool)
{
  return gradient_tool->block_handlers_count > 0;
}

static void
clay_gradient_tool_editor_freeze_gradient (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayGradient        *custom;
  GradientInfo           *info;

  clay_gradient_tool_editor_block_handlers (gradient_tool);

  custom = clay_gradients_get_custom (CLAY_CONTEXT (options)->clay);

  if (gradient_tool->gradient == custom || options->modify_active)
    {
      clay_assert (clay_gradient_tool_editor_is_gradient_editable (gradient_tool));

      clay_data_freeze (CLAY_DATA (gradient_tool->gradient));
    }
  else
    {
      /* copy the active gradient to the custom gradient, and make the custom
       * gradient active.
       */
      clay_data_freeze (CLAY_DATA (custom));

      clay_data_copy (CLAY_DATA (custom), CLAY_DATA (gradient_tool->gradient));

      clay_context_set_gradient (CLAY_CONTEXT (options), custom);

      clay_assert (gradient_tool->gradient == custom);
      clay_assert (clay_gradient_tool_editor_is_gradient_editable (gradient_tool));
    }

  if (gradient_tool->edit_count > 0)
    {
      info = gradient_tool->undo_stack->data;

      if (! info->gradient)
        {
          info->gradient =
            CLAY_GRADIENT (clay_data_duplicate (CLAY_DATA (gradient_tool->gradient)));
        }
    }
}

static void
clay_gradient_tool_editor_thaw_gradient (ClayGradientTool *gradient_tool)
{
  clay_data_thaw (CLAY_DATA (gradient_tool->gradient));

  clay_gradient_tool_editor_update_sliders (gradient_tool);
  clay_gradient_tool_editor_update_gui (gradient_tool);

  clay_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gint
clay_gradient_tool_editor_add_stop (ClayGradientTool *gradient_tool,
                                    gdouble           value)
{
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions    *paint_options = CLAY_PAINT_OPTIONS (options);
  ClayGradientSegment *seg;
  gint                 stop;
  GradientInfo           *info;

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  clay_gradient_split_at (gradient_tool->gradient,
                          CLAY_CONTEXT (options), NULL, value,
                          paint_options->gradient_options->gradient_blend_color_space,
                          &seg, NULL);

  stop =
    clay_gradient_segment_range_get_n_segments (gradient_tool->gradient,
                                                gradient_tool->gradient->segments,
                                                seg) - 1;

  info               = gradient_tool->undo_stack->data;
  info->added_handle = stop;

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return stop;
}

static void
clay_gradient_tool_editor_delete_stop (ClayGradientTool *gradient_tool,
                                       gint              slider)
{
  GradientInfo *info;

  clay_assert (clay_gradient_tool_editor_handle_is_stop (gradient_tool, slider));

  clay_gradient_tool_editor_start_edit (gradient_tool);
  clay_gradient_tool_editor_freeze_gradient (gradient_tool);

  info = gradient_tool->undo_stack->data;

  if (info->added_handle == slider)
    {
      /* when removing a stop that was added as part of the current action,
       * restore the original gradient at the beginning of the action, rather
       * than deleting the stop from the current gradient, so that the affected
       * midpoint returns to its state at the beginning of the action, instead
       * of being reset.
       *
       * note that this assumes that the gradient hasn't changed in any other
       * way during the action, which is ugly, but currently always true.
       */

      clay_assert (info->gradient != NULL);

      clay_data_copy (CLAY_DATA (gradient_tool->gradient),
                      CLAY_DATA (info->gradient));

      g_clear_object (&info->gradient);

      info->added_handle = CLAY_TOOL_LINE_HANDLE_NONE;
    }
  else
    {
      ClayGradientSegment *seg;

      seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      clay_gradient_segment_range_merge (gradient_tool->gradient,
                                         seg, seg->next, NULL, NULL);

      info->removed_handle = slider;
    }

  clay_gradient_tool_editor_thaw_gradient (gradient_tool);
  clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
}

static gint
clay_gradient_tool_editor_midpoint_to_stop (ClayGradientTool *gradient_tool,
                                            gint              slider)
{
  const ClayControllerSlider *sliders;

  clay_assert (clay_gradient_tool_editor_handle_is_midpoint (gradient_tool, slider));

  sliders = clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                        NULL);

  if (sliders[slider].value > sliders[slider].min + EPSILON &&
      sliders[slider].value < sliders[slider].max - EPSILON)
    {
      const ClayGradientSegment *seg;
      gint                       stop;
      GradientInfo                 *info;

      seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, slider);

      stop = clay_gradient_tool_editor_add_stop (gradient_tool, seg->middle);

      info                 = gradient_tool->undo_stack->data;
      info->removed_handle = slider;

      slider = stop;
    }

  return slider;
}

static void
clay_gradient_tool_editor_update_sliders (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions  *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions     *paint_options = CLAY_PAINT_OPTIONS (options);
  gdouble               offset        = options->offset / 100.0;
  gboolean              editable;
  ClayControllerSlider *sliders;
  gint                  n_sliders;
  gint                  n_segments;
  ClayGradientSegment  *seg;
  ClayControllerSlider *slider;
  gint                  i;

  if (! gradient_tool->widget || options->instant)
    return;

  editable = clay_gradient_tool_editor_is_gradient_editable (gradient_tool);

  n_segments = clay_gradient_segment_range_get_n_segments (
    gradient_tool->gradient, gradient_tool->gradient->segments, NULL);

  n_sliders = (n_segments - 1) + /* gradient stops, between each adjacent
                                  * pair of segments */
              (n_segments);      /* midpoints, inside each segment */

  sliders = g_new (ClayControllerSlider, n_sliders);

  slider = sliders;

  /* initialize the gradient-stop sliders */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg->next;
       seg = seg->next, i++)
    {
      *slider = CLAY_CONTROLLER_SLIDER_DEFAULT;

      slider->value     = seg->right;
      slider->min       = seg->left;
      slider->max       = seg->next->right;

      slider->movable   = editable;
      slider->removable = editable;

      slider->data      = GINT_TO_POINTER (i);

      slider++;
    }

  /* initialize the midpoint sliders */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg;
       seg = seg->next, i++)
    {
      *slider = CLAY_CONTROLLER_SLIDER_DEFAULT;

      slider->value    = seg->middle;
      slider->min      = seg->left;
      slider->max      = seg->right;

      /* hide midpoints of zero-length segments, since they'd otherwise
       * prevent the segment's endpoints from being selected
       */
      slider->visible  = fabs (slider->max - slider->min) > EPSILON;
      slider->movable  = editable;

      slider->autohide = TRUE;
      slider->type     = CLAY_HANDLE_FILLED_CIRCLE;
      slider->size     = 0.6;

      slider->data     = GINT_TO_POINTER (i);

      slider++;
    }

  /* flip the slider limits and values, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      for (i = 0; i < n_sliders; i++)
        {
          gdouble temp;

          sliders[i].value = 1.0 - sliders[i].value;
          temp             = sliders[i].min;
          sliders[i].min   = 1.0 - sliders[i].max;
          sliders[i].max   = 1.0 - temp;
        }
    }

  /* adjust the sliders according to the offset */
  for (i = 0; i < n_sliders; i++)
    {
      sliders[i].value = (1.0 - offset) * sliders[i].value + offset;
      sliders[i].min   = (1.0 - offset) * sliders[i].min   + offset;
      sliders[i].max   = (1.0 - offset) * sliders[i].max   + offset;
    }

  /* avoid updating the gradient in clay_gradient_tool_editor_line_changed() */
  clay_gradient_tool_editor_block_handlers (gradient_tool);

  clay_tool_line_set_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                              sliders, n_sliders);

  clay_gradient_tool_editor_unblock_handlers (gradient_tool);

  g_free (sliders);
}

static void
clay_gradient_tool_editor_purge_gradient_history (GSList **stack)
{
  GSList *link;

  /* eliminate all history steps that modify the gradient */
  while ((link = *stack))
    {
      GradientInfo *info = link->data;

      if (info->gradient)
        {
          clay_gradient_tool_editor_gradient_info_free (info);

          *stack = g_slist_delete_link (*stack, link);
        }
      else
        {
          stack = &link->next;
        }
    }
}

static void
clay_gradient_tool_editor_purge_gradient (ClayGradientTool *gradient_tool)
{
  if (gradient_tool->widget)
    {
      clay_gradient_tool_editor_update_sliders (gradient_tool);

      clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget),
                                    CLAY_TOOL_LINE_HANDLE_NONE);
    }

  clay_gradient_tool_editor_purge_gradient_history (&gradient_tool->undo_stack);
  clay_gradient_tool_editor_purge_gradient_history (&gradient_tool->redo_stack);
}

static GtkWidget *
clay_gradient_tool_editor_color_entry_new (ClayGradientTool  *gradient_tool,
                                           const gchar       *title,
                                           Direction          direction,
                                           GtkWidget         *chain_button,
                                           GtkWidget        **color_panel,
                                           GtkWidget        **type_combo)
{
  ClayContext *context = CLAY_CONTEXT (CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool));
  GtkWidget   *hbox;
  GtkWidget   *button;
  GtkWidget   *combo;
  GeglColor   *color   = gegl_color_new ("transparent");

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  /* the color panel */
  *color_panel = button = clay_color_panel_new (title, color,
                                                CLAY_COLOR_AREA_SMALL_CHECKS,
                                                24, 24);
  g_object_unref (color);
  clay_color_button_set_update (CLAY_COLOR_BUTTON (button), TRUE);
  clay_color_panel_set_context (CLAY_COLOR_PANEL (button), context);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);
  gtk_widget_set_visible (button, TRUE);

  g_object_set_data (G_OBJECT (button),
                     "clay-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (button),
                     "clay-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (clay_gradient_tool_editor_color_entry_color_clicked),
                    gradient_tool);
  g_signal_connect (button, "color-changed",
                    G_CALLBACK (clay_gradient_tool_editor_color_entry_color_changed),
                    gradient_tool);
  g_signal_connect (button, "response",
                    G_CALLBACK (clay_gradient_tool_editor_color_entry_color_response),
                    gradient_tool);

  /* the color type combo */
  *type_combo = combo = clay_enum_combo_box_new (CLAY_TYPE_GRADIENT_COLOR);
  gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
  gtk_widget_set_visible (combo, TRUE);

  g_object_set_data (G_OBJECT (combo),
                     "clay-gradient-tool-editor-direction",
                     GINT_TO_POINTER (direction));
  g_object_set_data (G_OBJECT (combo),
                     "clay-gradient-tool-editor-chain-button",
                     chain_button);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (clay_gradient_tool_editor_color_entry_type_changed),
                    gradient_tool);

  return hbox;
}

static void
clay_gradient_tool_editor_init_endpoint_gui (ClayGradientTool *gradient_tool)
{
  ClayDisplay      *display = CLAY_TOOL (gradient_tool)->display;
  ClayDisplayShell *shell   = clay_display_get_shell (display);
  ClayImage        *image   = clay_display_get_image (display);
  gdouble           xres;
  gdouble           yres;
  GtkWidget        *editor;
  GtkWidget        *grid;
  GtkWidget        *label;
  GtkWidget        *spinbutton;
  GtkWidget        *se;
  GtkWidget        *hbox;
  gint              row     = 0;

  clay_image_get_resolution (image, &xres, &yres);

  /* the endpoint editor */
  gradient_tool->endpoint_editor =
  editor                         = clay_editor_new ();
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_set_visible (grid, TRUE);

  /* the position labels */
  label = gtk_label_new (_("X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new (_("Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the position size entry */
  spinbutton = clay_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

  gradient_tool->endpoint_se =
  se                         = clay_size_entry_new (1, clay_unit_pixel (), "%a",
                                                    TRUE, TRUE, FALSE, 6,
                                                    CLAY_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_row_spacing (GTK_GRID (se), 4);
  gtk_grid_set_column_spacing (GTK_GRID (se), 2);

  clay_size_entry_add_field (CLAY_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (se), spinbutton, 1, 0, 1, 1);
  gtk_widget_set_visible (spinbutton, TRUE);

  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 2);
  gtk_widget_set_visible (se, TRUE);

  clay_size_entry_set_unit (CLAY_SIZE_ENTRY (se), shell->unit);

  clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (se), 0, xres, FALSE);
  clay_size_entry_set_resolution (CLAY_SIZE_ENTRY (se), 1, yres, FALSE);

  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (se), 0,
                                         -CLAY_MAX_IMAGE_SIZE,
                                         CLAY_MAX_IMAGE_SIZE);
  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (se), 1,
                                         -CLAY_MAX_IMAGE_SIZE,
                                         CLAY_MAX_IMAGE_SIZE);

  clay_size_entry_set_size (CLAY_SIZE_ENTRY (se), 0,
                            0, clay_image_get_width (image));
  clay_size_entry_set_size (CLAY_SIZE_ENTRY (se), 1,
                            0, clay_image_get_height (image));

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (clay_gradient_tool_editor_endpoint_se_value_changed),
                    gradient_tool);

  row += 2;

  /* the color label */
  label = gtk_label_new (_("Color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the color entry */
  hbox = clay_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Endpoint Color"), DIRECTION_NONE, NULL,
    &gradient_tool->endpoint_color_panel, &gradient_tool->endpoint_type_combo);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, row, 1, 1);
  gtk_widget_set_visible (hbox, TRUE);

  row++;
}

static void
clay_gradient_tool_editor_init_stop_gui (ClayGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *grid2;
  GtkWidget *button;
  GtkWidget *hbox;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->stop_editor =
  editor                     = clay_editor_new ();
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_set_visible (grid, TRUE);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the position size entry */
  gradient_tool->stop_se =
  se                     = clay_size_entry_new (1, clay_unit_percent (), "%a",
                                                FALSE, TRUE, FALSE, 6,
                                                CLAY_SIZE_ENTRY_UPDATE_SIZE);
  clay_size_entry_show_unit_menu (CLAY_SIZE_ENTRY (se), FALSE);
  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 1);
  gtk_widget_set_visible (se, TRUE);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (clay_gradient_tool_editor_stop_se_value_changed),
                    gradient_tool);

  row++;

  /* the color labels */
  label = gtk_label_new (_("Left color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  label = gtk_label_new (_("Right color:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the color entries grid */
  grid2 = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid2), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid2), 2);
  gtk_grid_attach (GTK_GRID (grid), grid2, 1, row, 1, 2);
  gtk_widget_set_visible (grid2, TRUE);

  /* the color entries chain button */
  gradient_tool->stop_chain_button =
  button                           = clay_chain_button_new (CLAY_CHAIN_RIGHT);
  gtk_grid_attach (GTK_GRID (grid2), button, 1, 0, 1, 2);
  gtk_widget_set_visible (button, TRUE);

  /* the color entries */
  hbox = clay_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_LEFT, button,
    &gradient_tool->stop_left_color_panel, &gradient_tool->stop_left_type_combo);
  gtk_grid_attach (GTK_GRID (grid2), hbox, 0, 0, 1, 1);
  gtk_widget_set_visible (hbox, TRUE);

  hbox = clay_gradient_tool_editor_color_entry_new (
    gradient_tool, _("Change Stop Color"), DIRECTION_RIGHT, button,
    &gradient_tool->stop_right_color_panel, &gradient_tool->stop_right_type_combo);
  gtk_grid_attach (GTK_GRID (grid2), hbox, 0, 1, 1, 1);
  gtk_widget_set_visible (hbox, TRUE);

  row += 2;

  /* the action buttons separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grid), separator, 0, row, 2, 1);
  gtk_widget_set_visible (separator, TRUE);

  row++;

  /* the delete button */
  clay_editor_add_button (CLAY_EDITOR (editor),
                          CLAY_ICON_EDIT_DELETE, _("Delete stop"),
                          NULL,
                          G_CALLBACK (clay_gradient_tool_editor_stop_delete_clicked),
                          NULL, G_OBJECT (gradient_tool));
}

static void
clay_gradient_tool_editor_init_midpoint_gui (ClayGradientTool *gradient_tool)
{
  GtkWidget *editor;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *se;
  GtkWidget *combo;
  GtkWidget *separator;
  gint       row = 0;

  /* the stop editor */
  gradient_tool->midpoint_editor =
  editor                         = clay_editor_new ();
  gtk_box_pack_start (GTK_BOX (clay_tool_gui_get_vbox (gradient_tool->gui)),
                      editor, FALSE, TRUE, 0);

  /* the main grid */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_box_pack_start (GTK_BOX (editor), grid, FALSE, TRUE, 0);
  gtk_widget_set_visible (grid, TRUE);

  /* the position label */
  label = gtk_label_new (_("Position:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the position size entry */
  gradient_tool->midpoint_se =
  se                         = clay_size_entry_new (1, clay_unit_percent (), "%a",
                                                    FALSE, TRUE, FALSE, 6,
                                                    CLAY_SIZE_ENTRY_UPDATE_SIZE);
  clay_size_entry_show_unit_menu (CLAY_SIZE_ENTRY (se), FALSE);
  gtk_grid_attach (GTK_GRID (grid), se, 1, row, 1, 1);
  gtk_widget_set_visible (se, TRUE);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (clay_gradient_tool_editor_midpoint_se_value_changed),
                    gradient_tool);

  row++;

  /* the type label */
  label = gtk_label_new (_("Blending:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the type combo */
  gradient_tool->midpoint_type_combo =
  combo                              = clay_enum_combo_box_new (CLAY_TYPE_GRADIENT_SEGMENT_TYPE);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  gtk_widget_set_visible (combo, TRUE);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (clay_gradient_tool_editor_midpoint_type_changed),
                    gradient_tool);

  row++;

  /* the color label */
  label = gtk_label_new (_("Coloring:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  /* the color combo */
  gradient_tool->midpoint_color_combo =
  combo                              = clay_enum_combo_box_new (CLAY_TYPE_GRADIENT_SEGMENT_COLOR);
  gtk_grid_attach (GTK_GRID (grid), combo, 1, row, 1, 1);
  gtk_widget_set_visible (combo, TRUE);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (clay_gradient_tool_editor_midpoint_color_changed),
                    gradient_tool);

  row++;

  /* the action buttons separator */
  separator = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grid), separator, 0, row, 2, 1);
  gtk_widget_set_visible (separator, TRUE);

  row++;

  /* the new stop button */
  gradient_tool->midpoint_new_stop_button =
    clay_editor_add_button (CLAY_EDITOR (editor),
                            CLAY_ICON_DOCUMENT_NEW, _("New stop at midpoint"),
                            NULL,
                            G_CALLBACK (clay_gradient_tool_editor_midpoint_new_stop_clicked),
                            NULL, G_OBJECT (gradient_tool));

  /* the center button */
  gradient_tool->midpoint_center_button =
    clay_editor_add_button (CLAY_EDITOR (editor),
                            CLAY_ICON_CENTER_HORIZONTAL, _("Center midpoint"),
                            NULL,
                            G_CALLBACK (clay_gradient_tool_editor_midpoint_center_clicked),
                            NULL, G_OBJECT (gradient_tool));
}

static void
clay_gradient_tool_editor_update_endpoint_gui (ClayGradientTool *gradient_tool,
                                               gint              selection)
{
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions    *paint_options = CLAY_PAINT_OPTIONS (options);
  ClayContext         *context       = CLAY_CONTEXT (options);
  gboolean             editable;
  ClayGradientSegment *seg;
  const gchar         *title;
  gdouble              x;
  gdouble              y;
  GeglColor           *color = NULL;
  ClayGradientColor    color_type;

  editable = clay_gradient_tool_editor_is_gradient_editable (gradient_tool);

  switch (selection)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      g_object_get (gradient_tool->widget,
                    "x1", &x,
                    "y1", &y,
                    NULL);
      break;

    case CLAY_TOOL_LINE_HANDLE_END:
      g_object_get (gradient_tool->widget,
                    "x2", &x,
                    "y2", &y,
                    NULL);
      break;

    default:
      clay_assert_not_reached ();
    }

  /* swap the endpoint handles, if necessary */
  if (paint_options->gradient_options->gradient_reverse)
    {
      switch (selection)
        {
        case CLAY_TOOL_LINE_HANDLE_START:
          selection = CLAY_TOOL_LINE_HANDLE_END;
          break;

        case CLAY_TOOL_LINE_HANDLE_END:
          selection = CLAY_TOOL_LINE_HANDLE_START;
          break;
        }
    }

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  switch (selection)
    {
    case CLAY_TOOL_LINE_HANDLE_START:
      title = _("Start Endpoint");

      color = clay_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                                         seg);
      color_type = seg->left_color_type;
      break;

    case CLAY_TOOL_LINE_HANDLE_END:
      title = _("End Endpoint");

      color = clay_gradient_segment_get_right_flat_color (gradient_tool->gradient, context,
                                                          seg);
      color_type = seg->right_color_type;
      break;

    default:
      clay_assert_not_reached ();
    }

  clay_tool_gui_set_title (gradient_tool->gui, title);

  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (gradient_tool->endpoint_se), 0, x);
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (gradient_tool->endpoint_se), 1, y);

  clay_color_button_set_color (CLAY_COLOR_BUTTON (gradient_tool->endpoint_color_panel), color);
  clay_int_combo_box_set_active (
    CLAY_INT_COMBO_BOX (gradient_tool->endpoint_type_combo), color_type);

  gtk_widget_set_sensitive (gradient_tool->endpoint_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->endpoint_type_combo,  editable);

  gtk_widget_set_visible (gradient_tool->endpoint_editor, TRUE);

  g_object_unref (color);
}

static void
clay_gradient_tool_editor_update_stop_gui (ClayGradientTool *gradient_tool,
                                           gint              selection)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context = CLAY_CONTEXT (options);
  gboolean             editable;
  ClayGradientSegment *seg;
  gint                 index;
  gchar               *title;
  gdouble              min;
  gdouble              max;
  gdouble              value;
  GeglColor           *left_color;
  ClayGradientColor    left_color_type;
  GeglColor           *right_color;
  ClayGradientColor    right_color_type;

  editable = clay_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Stop %d"), index + 1);

  min   = seg->left;
  max   = seg->next->right;
  value = seg->right;

  left_color = clay_gradient_segment_get_right_flat_color (gradient_tool->gradient, context, seg);
  left_color_type = seg->right_color_type;

  right_color = clay_gradient_segment_get_left_flat_color (gradient_tool->gradient, context,
                                                           seg->next);
  right_color_type = seg->next->left_color_type;

  clay_tool_gui_set_title (gradient_tool->gui, title);

  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (gradient_tool->stop_se),
                                         0, 100.0 * min, 100.0 * max);
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (gradient_tool->stop_se),
                              0, 100.0 * value);

  clay_color_button_set_color (
    CLAY_COLOR_BUTTON (gradient_tool->stop_left_color_panel), left_color);
  clay_int_combo_box_set_active (
    CLAY_INT_COMBO_BOX (gradient_tool->stop_left_type_combo), left_color_type);

  clay_color_button_set_color (
    CLAY_COLOR_BUTTON (gradient_tool->stop_right_color_panel), right_color);
  clay_int_combo_box_set_active (
    CLAY_INT_COMBO_BOX (gradient_tool->stop_right_type_combo), right_color_type);

  gtk_widget_set_sensitive (gradient_tool->stop_se,                editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_color_panel,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_left_type_combo,   editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_color_panel, editable);
  gtk_widget_set_sensitive (gradient_tool->stop_right_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->stop_chain_button,      editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (clay_editor_get_button_box (CLAY_EDITOR (gradient_tool->stop_editor))),
    editable);

  g_free (title);
  g_object_unref (left_color);
  g_object_unref (right_color);

  gtk_widget_set_visible (gradient_tool->stop_editor, TRUE);
}

static void
clay_gradient_tool_editor_update_midpoint_gui (ClayGradientTool *gradient_tool,
                                               gint              selection)
{
  gboolean                    editable;
  const ClayGradientSegment  *seg;
  gint                        index;
  gchar                      *title;
  gdouble                     min;
  gdouble                     max;
  gdouble                     value;
  ClayGradientSegmentType     type;
  ClayGradientSegmentColor    color;

  editable = clay_gradient_tool_editor_is_gradient_editable (gradient_tool);

  seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, selection);

  index = GPOINTER_TO_INT (
    clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                NULL)[selection].data);

  title = g_strdup_printf (_("Midpoint %d"), index + 1);

  min   = seg->left;
  max   = seg->right;
  value = seg->middle;
  type  = seg->type;
  color = seg->color;

  clay_tool_gui_set_title (gradient_tool->gui, title);

  clay_size_entry_set_refval_boundaries (CLAY_SIZE_ENTRY (gradient_tool->midpoint_se),
                                         0, 100.0 * min, 100.0 * max);
  clay_size_entry_set_refval (CLAY_SIZE_ENTRY (gradient_tool->midpoint_se),
                              0, 100.0 * value);

  clay_int_combo_box_set_active (
    CLAY_INT_COMBO_BOX (gradient_tool->midpoint_type_combo), type);

  clay_int_combo_box_set_active (
    CLAY_INT_COMBO_BOX (gradient_tool->midpoint_color_combo), color);

  gtk_widget_set_sensitive (gradient_tool->midpoint_new_stop_button,
                            value > min + EPSILON && value < max - EPSILON);
  gtk_widget_set_sensitive (gradient_tool->midpoint_center_button,
                            fabs (value - (min + max) / 2.0) > EPSILON);

  gtk_widget_set_sensitive (gradient_tool->midpoint_se,          editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_type_combo,  editable);
  gtk_widget_set_sensitive (gradient_tool->midpoint_color_combo, editable);
  gtk_widget_set_sensitive (
    GTK_WIDGET (clay_editor_get_button_box (CLAY_EDITOR (gradient_tool->midpoint_editor))),
    editable);

  g_free (title);

  gtk_widget_set_visible (gradient_tool->midpoint_editor, TRUE);
}

static void
clay_gradient_tool_editor_update_gui (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  if (gradient_tool->gradient && gradient_tool->widget && ! options->instant)
    {
      gint selection;

      selection =
        clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

      if (selection != CLAY_TOOL_LINE_HANDLE_NONE)
        {
          if (! gradient_tool->gui)
            {
              ClayDisplayShell *shell;

              shell = clay_tool_widget_get_shell (gradient_tool->widget);

              gradient_tool->gui =
                clay_tool_gui_new (CLAY_TOOL (gradient_tool)->tool_info,
                                   NULL, NULL, NULL, NULL,
                                   clay_widget_get_monitor (GTK_WIDGET (shell)),
                                   TRUE,

                                   _("_Close"), GTK_RESPONSE_CLOSE,

                                   NULL);

              clay_tool_gui_set_shell (gradient_tool->gui, shell);
              clay_tool_gui_set_viewable (gradient_tool->gui,
                                          CLAY_VIEWABLE (gradient_tool->gradient));
              clay_tool_gui_set_auto_overlay (gradient_tool->gui, TRUE);

              g_signal_connect (gradient_tool->gui, "response",
                                G_CALLBACK (clay_gradient_tool_editor_gui_response),
                                gradient_tool);

              clay_gradient_tool_editor_init_endpoint_gui (gradient_tool);
              clay_gradient_tool_editor_init_stop_gui     (gradient_tool);
              clay_gradient_tool_editor_init_midpoint_gui (gradient_tool);
            }

          clay_gradient_tool_editor_block_handlers (gradient_tool);

          if (clay_gradient_tool_editor_handle_is_endpoint (gradient_tool, selection))
            clay_gradient_tool_editor_update_endpoint_gui (gradient_tool, selection);
          else
            gtk_widget_set_visible (gradient_tool->endpoint_editor, FALSE);

          if (clay_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            clay_gradient_tool_editor_update_stop_gui (gradient_tool, selection);
          else
            gtk_widget_set_visible (gradient_tool->stop_editor, FALSE);

          if (clay_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            clay_gradient_tool_editor_update_midpoint_gui (gradient_tool, selection);
          else
            gtk_widget_set_visible (gradient_tool->midpoint_editor, FALSE);

          clay_gradient_tool_editor_unblock_handlers (gradient_tool);

          clay_tool_gui_show (gradient_tool->gui);

          return;
        }
    }

  if (gradient_tool->gui)
    clay_tool_gui_hide (gradient_tool->gui);
}

static GradientInfo *
clay_gradient_tool_editor_gradient_info_new (ClayGradientTool *gradient_tool)
{
  GradientInfo *info = g_slice_new (GradientInfo);

  info->start_x         = gradient_tool->start_x;
  info->start_y         = gradient_tool->start_y;
  info->end_x           = gradient_tool->end_x;
  info->end_y           = gradient_tool->end_y;

  info->gradient        = NULL;

  info->added_handle    = CLAY_TOOL_LINE_HANDLE_NONE;
  info->removed_handle  = CLAY_TOOL_LINE_HANDLE_NONE;
  info->selected_handle = CLAY_TOOL_LINE_HANDLE_NONE;

  return info;
}

static void
clay_gradient_tool_editor_gradient_info_free (GradientInfo *info)
{
  if (info->gradient)
    g_object_unref (info->gradient);

  g_slice_free (GradientInfo, info);
}

static void
clay_gradient_tool_editor_gradient_info_apply (ClayGradientTool   *gradient_tool,
                                               const GradientInfo *info,
                                               gboolean            set_selection)
{
  gint selection;

  clay_assert (gradient_tool->widget   != NULL);
  clay_assert (gradient_tool->gradient != NULL);

  /* pick the handle to select */
  if (info->gradient)
    {
      if (info->removed_handle != CLAY_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop-deletion or midpoint-to-stop operation;
           * select the removed handle
           */
          selection = info->removed_handle;
        }
      else if (info->added_handle != CLAY_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a stop addition operation */
          clay_assert (clay_gradient_tool_editor_handle_is_stop (gradient_tool,
                                                                 info->added_handle));

          selection =
            clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

          /* if the selected handle is a stop... */
          if (clay_gradient_tool_editor_handle_is_stop (gradient_tool, selection))
            {
              /* if the added handle is selected, clear the selection */
              if (selection == info->added_handle)
                selection = CLAY_TOOL_LINE_HANDLE_NONE;
              /* otherwise, keep the currently selected stop, possibly
               * adjusting its handle index
               */
              else if (selection > info->added_handle)
                selection--;
            }
          /* otherwise, if the selected handle is a midpoint... */
          else if (clay_gradient_tool_editor_handle_is_midpoint (gradient_tool, selection))
            {
              const ClayControllerSlider *sliders;
              gint                        seg_i;

              sliders =
                clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                            NULL);

              seg_i = GPOINTER_TO_INT (sliders[selection].data);

              /* if the midpoint belongs to one of the two segments incident to
               * the added stop, clear the selection
               */
              if (seg_i == info->added_handle ||
                  seg_i == info->added_handle + 1)
                {
                  selection = CLAY_TOOL_LINE_HANDLE_NONE;
                }
              /* otherwise, keep the currently selected stop, adjusting its
               * handle index
               */
              else
                {
                  /* midpoint handles follow stop handles; since we removed a
                   * stop, we must decrement the handle index
                   */
                  selection--;

                  if (seg_i > info->added_handle)
                    selection--;
                }
            }
          /* otherwise, don't change the selection */
          else
            {
              set_selection = FALSE;
            }
        }
      else if (info->selected_handle != CLAY_TOOL_LINE_HANDLE_NONE)
        {
          /* we're undoing a property change operation; select the handle
           * corresponding to the affected object
           */
          selection = info->selected_handle;
        }
      else
        {
          /* something went wrong... */
          g_warn_if_reached ();

          set_selection = FALSE;
        }
    }
  else if ((info->start_x != gradient_tool->start_x  ||
            info->start_y != gradient_tool->start_y) &&
           (info->end_x   == gradient_tool->end_x    &&
            info->end_y   == gradient_tool->end_y))
    {
      /* we're undoing a start-endpoint move operation; select the start
       * endpoint
       */
      selection = CLAY_TOOL_LINE_HANDLE_START;
    }
  else if ((info->end_x   != gradient_tool->end_x    ||
            info->end_y   != gradient_tool->end_y)   &&
           (info->start_x == gradient_tool->start_x  &&
            info->start_y == gradient_tool->start_y))

    {
      /* we're undoing am end-endpoint move operation; select the end
       * endpoint
       */
      selection = CLAY_TOOL_LINE_HANDLE_END;
    }
  else
    {
      /* we're undoing a line move operation; don't change the selection */
      set_selection = FALSE;
    }

  clay_gradient_tool_editor_block_handlers (gradient_tool);

  g_object_set (gradient_tool->widget,
                "x1", info->start_x,
                "y1", info->start_y,
                "x2", info->end_x,
                "y2", info->end_y,
                NULL);

  if (info->gradient)
    {
      clay_gradient_tool_editor_freeze_gradient (gradient_tool);

      clay_data_copy (CLAY_DATA (gradient_tool->gradient),
                      CLAY_DATA (info->gradient));

      clay_gradient_tool_editor_thaw_gradient (gradient_tool);
    }

  if (set_selection)
    {
      clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget),
                                    selection);
    }

  clay_gradient_tool_editor_update_gui (gradient_tool);

  clay_gradient_tool_editor_unblock_handlers (gradient_tool);
}

static gboolean
clay_gradient_tool_editor_gradient_info_is_trivial (ClayGradientTool   *gradient_tool,
                                                    const GradientInfo *info)
{
  const ClayGradientSegment *seg1;
  const ClayGradientSegment *seg2;

  if (info->start_x != gradient_tool->start_x ||
      info->start_y != gradient_tool->start_y ||
      info->end_x   != gradient_tool->end_x   ||
      info->end_y   != gradient_tool->end_y)
    {
      return FALSE;
    }

  if (info->gradient)
    {
      for (seg1 = info->gradient->segments, seg2 = gradient_tool->gradient->segments;
           seg1 && seg2;
           seg1 = seg1->next, seg2 = seg2->next)
        {
          if (memcmp (seg1, seg2, G_STRUCT_OFFSET (ClayGradientSegment, prev)))
            return FALSE;
        }

      if (seg1 || seg2)
        return FALSE;
    }

  return TRUE;
}


/*  public functions  */

void
clay_gradient_tool_editor_options_notify (ClayGradientTool *gradient_tool,
                                          ClayToolOptions  *options,
                                          const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "modify-active"))
    {
      clay_gradient_tool_editor_update_sliders (gradient_tool);
      clay_gradient_tool_editor_update_gui (gradient_tool);
    }
  else if (! strcmp (pspec->name, "gradient-reverse"))
    {
      clay_gradient_tool_editor_update_sliders (gradient_tool);

      /* if an endpoint is selected, swap the selected endpoint */
      if (gradient_tool->widget)
        {
          gint selection;

          selection =
            clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

          switch (selection)
            {
            case CLAY_TOOL_LINE_HANDLE_START:
              clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget),
                                            CLAY_TOOL_LINE_HANDLE_END);
              break;

            case CLAY_TOOL_LINE_HANDLE_END:
              clay_tool_line_set_selection (CLAY_TOOL_LINE (gradient_tool->widget),
                                            CLAY_TOOL_LINE_HANDLE_START);
              break;
            }
        }
    }
  else if (gradient_tool->render_node &&
           gegl_node_find_property (gradient_tool->render_node, pspec->name))
    {
      clay_gradient_tool_editor_update_sliders (gradient_tool);
    }
}

void
clay_gradient_tool_editor_start (ClayGradientTool *gradient_tool)
{
  g_signal_connect (gradient_tool->widget, "can-add-slider",
                    G_CALLBACK (clay_gradient_tool_editor_line_can_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "add-slider",
                    G_CALLBACK (clay_gradient_tool_editor_line_add_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "prepare-to-remove-slider",
                    G_CALLBACK (clay_gradient_tool_editor_line_prepare_to_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "remove-slider",
                    G_CALLBACK (clay_gradient_tool_editor_line_remove_slider),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "selection-changed",
                    G_CALLBACK (clay_gradient_tool_editor_line_selection_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "handle-clicked",
                    G_CALLBACK (clay_gradient_tool_editor_line_handle_clicked),
                    gradient_tool);
}

void
clay_gradient_tool_editor_halt (ClayGradientTool *gradient_tool)
{
  g_clear_object (&gradient_tool->gui);

  gradient_tool->edit_count = 0;

  if (gradient_tool->undo_stack)
    {
      g_slist_free_full (gradient_tool->undo_stack,
                         (GDestroyNotify) clay_gradient_tool_editor_gradient_info_free);
      gradient_tool->undo_stack = NULL;
    }

  if (gradient_tool->redo_stack)
    {
      g_slist_free_full (gradient_tool->redo_stack,
                         (GDestroyNotify) clay_gradient_tool_editor_gradient_info_free);
      gradient_tool->redo_stack = NULL;
    }

  if (gradient_tool->flush_idle_id)
    {
      g_source_remove (gradient_tool->flush_idle_id);
      gradient_tool->flush_idle_id = 0;
    }
}

gboolean
clay_gradient_tool_editor_line_changed (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions        *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayPaintOptions           *paint_options = CLAY_PAINT_OPTIONS (options);
  gdouble                     offset        = options->offset / 100.0;
  const ClayControllerSlider *sliders;
  gint                        n_sliders;
  gint                        i;
  ClayGradientSegment        *seg;
  gboolean                    changed       = FALSE;

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return FALSE;

  if (! gradient_tool->gradient || offset == 1.0)
    return FALSE;

  sliders = clay_tool_line_get_sliders (CLAY_TOOL_LINE (gradient_tool->widget),
                                        &n_sliders);

  if (n_sliders == 0)
    return FALSE;

  /* update the midpoints first, since moving the gradient stops may change the
   * gradient's midpoints w.r.t. the sliders, but not the other way around.
   */
  for (seg = gradient_tool->gradient->segments, i = n_sliders / 2;
       seg;
       seg = seg->next, i++)
    {
      gdouble value;

      value = sliders[i].value;

      /* adjust slider value according to the offset */
      value = (value - offset) / (1.0 - offset);

      /* flip the slider value, if necessary */
      if (paint_options->gradient_options->gradient_reverse)
        value = 1.0 - value;

      if (fabs (value - seg->middle) > EPSILON)
        {
          if (! changed)
            {
              clay_gradient_tool_editor_start_edit (gradient_tool);
              clay_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, i);

              changed = TRUE;
            }

          seg->middle = value;
        }
    }

  /* update the gradient stops */
  for (seg = gradient_tool->gradient->segments, i = 0;
       seg->next;
       seg = seg->next, i++)
    {
      gdouble value;

      value = sliders[i].value;

      /* adjust slider value according to the offset */
      value = (value - offset) / (1.0 - offset);

      /* flip the slider value, if necessary */
      if (paint_options->gradient_options->gradient_reverse)
        value = 1.0 - value;

      if (fabs (value - seg->right) > EPSILON)
        {
          if (! changed)
            {
              clay_gradient_tool_editor_start_edit (gradient_tool);
              clay_gradient_tool_editor_freeze_gradient (gradient_tool);

              /* refetch the segment, since the gradient might have changed */
              seg = clay_gradient_tool_editor_handle_get_segment (gradient_tool, i);

              changed = TRUE;
            }

          clay_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg, seg,
                                                seg->left, value);
          clay_gradient_segment_range_compress (gradient_tool->gradient,
                                                seg->next, seg->next,
                                                value, seg->next->right);
        }
    }

  if (changed)
    {
      clay_gradient_tool_editor_thaw_gradient (gradient_tool);
      clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);
    }

  clay_gradient_tool_editor_update_gui (gradient_tool);

  return changed;
}

void
clay_gradient_tool_editor_fg_bg_changed (ClayGradientTool *gradient_tool)
{
  clay_gradient_tool_editor_update_gui (gradient_tool);
}

void
clay_gradient_tool_editor_gradient_dirty (ClayGradientTool *gradient_tool)
{
  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  clay_gradient_tool_editor_purge_gradient (gradient_tool);
}

void
clay_gradient_tool_editor_gradient_changed (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context = CLAY_CONTEXT (options);

  if (options->modify_active_frame)
    {
      gtk_widget_set_sensitive (options->modify_active_frame,
                                gradient_tool->gradient !=
                                clay_gradients_get_custom (context->clay));
    }

  if (options->modify_active_hint)
    {
      gtk_widget_set_visible (options->modify_active_hint,
                              gradient_tool->gradient &&
                              ! clay_data_is_writable (CLAY_DATA (gradient_tool->gradient)));
    }

  if (clay_gradient_tool_editor_are_handlers_blocked (gradient_tool))
    return;

  clay_gradient_tool_editor_purge_gradient (gradient_tool);
}

const gchar *
clay_gradient_tool_editor_can_undo (ClayGradientTool *gradient_tool)
{
  if (! gradient_tool->undo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

const gchar *
clay_gradient_tool_editor_can_redo (ClayGradientTool *gradient_tool)
{
  if (! gradient_tool->redo_stack || gradient_tool->edit_count > 0)
    return NULL;

  return _("Gradient Step");
}

gboolean
clay_gradient_tool_editor_undo (ClayGradientTool *gradient_tool)
{
  ClayTool  *tool = CLAY_TOOL (gradient_tool);
  GradientInfo *info;
  GradientInfo *new_info;

  clay_assert (gradient_tool->undo_stack != NULL);
  clay_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->undo_stack->data;

  new_info = clay_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        CLAY_GRADIENT (clay_data_duplicate (CLAY_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on redo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack, info);
  gradient_tool->redo_stack = g_slist_prepend (gradient_tool->redo_stack, new_info);

  clay_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  clay_gradient_tool_editor_gradient_info_free (info);

  /* the initial state of the gradient tool is not useful; we might as well halt */
  if (! gradient_tool->undo_stack)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  return TRUE;
}

gboolean
clay_gradient_tool_editor_redo (ClayGradientTool *gradient_tool)
{
  GradientInfo *info;
  GradientInfo *new_info;

  clay_assert (gradient_tool->redo_stack != NULL);
  clay_assert (gradient_tool->edit_count == 0);

  info = gradient_tool->redo_stack->data;

  new_info = clay_gradient_tool_editor_gradient_info_new (gradient_tool);

  if (info->gradient)
    {
      new_info->gradient =
        CLAY_GRADIENT (clay_data_duplicate (CLAY_DATA (gradient_tool->gradient)));

      /* swap the added and removed handles, so that gradient_info_apply() does
       * the right thing on undo
       */
      new_info->added_handle    = info->removed_handle;
      new_info->removed_handle  = info->added_handle;
      new_info->selected_handle = info->selected_handle;
    }

  gradient_tool->redo_stack = g_slist_remove (gradient_tool->redo_stack, info);
  gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, new_info);

  clay_gradient_tool_editor_gradient_info_apply (gradient_tool, info, TRUE);
  clay_gradient_tool_editor_gradient_info_free (info);

  return TRUE;
}

void
clay_gradient_tool_editor_start_edit (ClayGradientTool *gradient_tool)
{
  if (gradient_tool->edit_count++ == 0)
    {
      GradientInfo *info;

      info = clay_gradient_tool_editor_gradient_info_new (gradient_tool);

      gradient_tool->undo_stack = g_slist_prepend (gradient_tool->undo_stack, info);

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) clay_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}

void
clay_gradient_tool_editor_end_edit (ClayGradientTool *gradient_tool,
                                    gboolean          cancel)
{
  /* can happen when halting using esc */
  if (gradient_tool->edit_count == 0)
    return;

  if (--gradient_tool->edit_count == 0)
    {
      GradientInfo *info = gradient_tool->undo_stack->data;

      info->selected_handle =
        clay_tool_line_get_selection (CLAY_TOOL_LINE (gradient_tool->widget));

      if (cancel ||
          clay_gradient_tool_editor_gradient_info_is_trivial (gradient_tool, info))
        {
          /* if the edit is canceled, or if nothing changed, undo the last
           * step
           */
          clay_gradient_tool_editor_gradient_info_apply (gradient_tool, info, FALSE);

          gradient_tool->undo_stack = g_slist_remove (gradient_tool->undo_stack,
                                                   info);
          clay_gradient_tool_editor_gradient_info_free (info);
        }
      else
        {
          /* otherwise, blow the redo stack */
          g_slist_free_full (gradient_tool->redo_stack,
                             (GDestroyNotify) clay_gradient_tool_editor_gradient_info_free);
          gradient_tool->redo_stack = NULL;
        }

      /*  update the undo actions / menu items  */
      if (! gradient_tool->flush_idle_id)
        {
          gradient_tool->flush_idle_id =
            g_idle_add ((GSourceFunc) clay_gradient_tool_editor_flush_idle,
                        gradient_tool);
        }
    }
}
