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
#include <gdk/gdkkeysyms.h>

#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "gegl/clayapplicator.h"
#include "gegl/clay-gegl-nodes.h"
#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/clay-transform-resize.h"
#include "core/clay-transform-utils.h"
#include "core/clayboundary.h"
#include "core/claycontainer.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/clayfilter.h"
#include "core/claygrouplayer.h"
#include "core/clayimage.h"
#include "core/clayimage-item-list.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claypickable.h"
#include "core/clayprojection.h"
#include "core/claytoolinfo.h"
#include "core/clayviewable.h"

#include "path/claypath.h"
#include "path/claystroke.h"

#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolgui.h"
#include "display/claytoolwidget.h"

#include "claytoolcontrol.h"
#include "claytransformgridoptions.h"
#include "claytransformgridtool.h"
#include "claytransformgridtoolundo.h"
#include "claytransformoptions.h"

#include "clay-intl.h"


#define EPSILON 1e-6


#define RESPONSE_RESET     1
#define RESPONSE_READJUST  2

#define UNDO_COMPRESS_TIME (0.5 * G_TIME_SPAN_SECOND)


typedef struct
{
  ClayTransformGridTool *tg_tool;

  ClayDrawable          *drawable;
  ClayDrawableFilter    *filter;

  ClayDrawable          *root_drawable;

  GeglNode              *transform_node;
  GeglNode              *crop_node;

  ClayMatrix3            transform;
  GeglRectangle          bounds;
} Filter;

typedef struct
{
  ClayTransformGridTool *tg_tool;
  ClayDrawable          *root_drawable;
} AddFilterData;

typedef struct
{
  gint64                 time;
  ClayTransformDirection direction;
  TransInfo              trans_infos[2];
} UndoInfo;


static void      clay_transform_grid_tool_finalize           (GObject                *object);

static gboolean  clay_transform_grid_tool_initialize         (ClayTool               *tool,
                                                              ClayDisplay            *display,
                                                              GError                **error);
static void      clay_transform_grid_tool_control            (ClayTool               *tool,
                                                              ClayToolAction          action,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_button_press       (ClayTool               *tool,
                                                              const ClayCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              ClayButtonPressType     press_type,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_button_release     (ClayTool               *tool,
                                                              const ClayCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              ClayButtonReleaseType   release_type,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_motion             (ClayTool               *tool,
                                                              const ClayCoords       *coords,
                                                              guint32                 time,
                                                              GdkModifierType         state,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_modifier_key       (ClayTool               *tool,
                                                              GdkModifierType         key,
                                                              gboolean                press,
                                                              GdkModifierType         state,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_cursor_update      (ClayTool               *tool,
                                                              const ClayCoords       *coords,
                                                              GdkModifierType         state,
                                                              ClayDisplay            *display);
static const gchar * clay_transform_grid_tool_can_undo       (ClayTool               *tool,
                                                              ClayDisplay            *display);
static const gchar * clay_transform_grid_tool_can_redo       (ClayTool               *tool,
                                                              ClayDisplay            *display);
static gboolean  clay_transform_grid_tool_undo               (ClayTool               *tool,
                                                              ClayDisplay            *display);
static gboolean  clay_transform_grid_tool_redo               (ClayTool               *tool,
                                                              ClayDisplay            *display);
static void      clay_transform_grid_tool_options_notify     (ClayTool               *tool,
                                                              ClayToolOptions        *options,
                                                              const GParamSpec       *pspec);

static void      clay_transform_grid_tool_draw               (ClayDrawTool           *draw_tool);

static void      clay_transform_grid_tool_recalc_matrix      (ClayTransformTool      *tr_tool);
static gchar   * clay_transform_grid_tool_get_undo_desc      (ClayTransformTool      *tr_tool);
static ClayTransformDirection clay_transform_grid_tool_get_direction
                                                             (ClayTransformTool      *tr_tool);
static GeglBuffer * clay_transform_grid_tool_transform       (ClayTransformTool      *tr_tool,
                                                              GList                  *objects,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              ClayColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static void     clay_transform_grid_tool_real_apply_info     (ClayTransformGridTool  *tg_tool,
                                                              const TransInfo         info);
static gchar  * clay_transform_grid_tool_real_get_undo_desc  (ClayTransformGridTool  *tg_tool);
static void     clay_transform_grid_tool_real_update_widget  (ClayTransformGridTool  *tg_tool);
static void     clay_transform_grid_tool_real_widget_changed (ClayTransformGridTool  *tg_tool);
static GeglBuffer * clay_transform_grid_tool_real_transform  (ClayTransformGridTool  *tg_tool,
                                                              GList                  *objects,
                                                              GeglBuffer             *orig_buffer,
                                                              gint                    orig_offset_x,
                                                              gint                    orig_offset_y,
                                                              ClayColorProfile      **buffer_profile,
                                                              gint                   *new_offset_x,
                                                              gint                   *new_offset_y);

static void      clay_transform_grid_tool_widget_changed     (ClayToolWidget         *widget,
                                                              ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_widget_response    (ClayToolWidget         *widget,
                                                              gint                    response_id,
                                                              ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_filter_flush       (ClayDrawableFilter     *filter,
                                                              ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_halt               (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_commit             (ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_dialog             (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_dialog_update      (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_prepare            (ClayTransformGridTool  *tg_tool,
                                                              ClayDisplay            *display);
static ClayToolWidget * clay_transform_grid_tool_get_widget  (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_update_widget      (ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_response           (ClayToolGui            *gui,
                                                              gint                    response_id,
                                                              ClayTransformGridTool  *tg_tool);

static gboolean  clay_transform_grid_tool_composited_preview (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_update_sensitivity (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_update_preview     (ClayTransformGridTool  *tg_tool);
static void      clay_transform_grid_tool_update_filters     (ClayTransformGridTool  *tg_tool);
static void   clay_transform_grid_tool_hide_selected_objects (ClayTransformGridTool  *tg_tool,
                                                              GList                  *objects);
static void   clay_transform_grid_tool_show_selected_objects (ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_add_filter         (ClayDrawable           *drawable,
                                                              AddFilterData          *data);
static void      clay_transform_grid_tool_remove_filter      (ClayDrawable           *drawable,
                                                              ClayTransformGridTool  *tg_tool);

static void      clay_transform_grid_tool_effective_mode_changed
                                                             (ClayLayer              *layer,
                                                              ClayTransformGridTool  *tg_tool);

static Filter  * filter_new                                  (ClayTransformGridTool  *tg_tool,
                                                              ClayDrawable           *drawable,
                                                              ClayDrawable           *root_drawable,
                                                              gboolean                add_filter);
static void      filter_free                                 (Filter                 *filter);

static UndoInfo * undo_info_new  (void);
static void       undo_info_free (UndoInfo *info);

static gboolean   trans_info_equal  (const TransInfo  trans_info1,
                                     const TransInfo  trans_info2);
static gboolean   trans_infos_equal (const TransInfo *trans_infos1,
                                     const TransInfo *trans_infos2);


G_DEFINE_TYPE (ClayTransformGridTool, clay_transform_grid_tool, CLAY_TYPE_TRANSFORM_TOOL)

#define parent_class clay_transform_grid_tool_parent_class


static void
clay_transform_grid_tool_class_init (ClayTransformGridToolClass *klass)
{
  GObjectClass           *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass          *tool_class   = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass      *draw_class   = CLAY_DRAW_TOOL_CLASS (klass);
  ClayTransformToolClass *tr_class     = CLAY_TRANSFORM_TOOL_CLASS (klass);

  object_class->finalize     = clay_transform_grid_tool_finalize;

  tool_class->initialize     = clay_transform_grid_tool_initialize;
  tool_class->control        = clay_transform_grid_tool_control;
  tool_class->button_press   = clay_transform_grid_tool_button_press;
  tool_class->button_release = clay_transform_grid_tool_button_release;
  tool_class->motion         = clay_transform_grid_tool_motion;
  tool_class->modifier_key   = clay_transform_grid_tool_modifier_key;
  tool_class->cursor_update  = clay_transform_grid_tool_cursor_update;
  tool_class->can_undo       = clay_transform_grid_tool_can_undo;
  tool_class->can_redo       = clay_transform_grid_tool_can_redo;
  tool_class->undo           = clay_transform_grid_tool_undo;
  tool_class->redo           = clay_transform_grid_tool_redo;
  tool_class->options_notify = clay_transform_grid_tool_options_notify;

  draw_class->draw           = clay_transform_grid_tool_draw;

  tr_class->recalc_matrix    = clay_transform_grid_tool_recalc_matrix;
  tr_class->get_undo_desc    = clay_transform_grid_tool_get_undo_desc;
  tr_class->get_direction    = clay_transform_grid_tool_get_direction;
  tr_class->transform        = clay_transform_grid_tool_transform;

  klass->info_to_matrix      = NULL;
  klass->matrix_to_info      = NULL;
  klass->apply_info          = clay_transform_grid_tool_real_apply_info;
  klass->get_undo_desc       = clay_transform_grid_tool_real_get_undo_desc;
  klass->dialog              = NULL;
  klass->dialog_update       = NULL;
  klass->prepare             = NULL;
  klass->readjust            = NULL;
  klass->get_widget          = NULL;
  klass->update_widget       = clay_transform_grid_tool_real_update_widget;
  klass->widget_changed      = clay_transform_grid_tool_real_widget_changed;
  klass->transform           = clay_transform_grid_tool_real_transform;

  klass->ok_button_label     = _("_Transform");
}

static void
clay_transform_grid_tool_init (ClayTransformGridTool *tg_tool)
{
  ClayTool *tool = CLAY_TOOL (tg_tool);

  clay_tool_control_set_scroll_lock      (tool->control, TRUE);
  clay_tool_control_set_preserve         (tool->control, FALSE);
  clay_tool_control_set_dirty_mask       (tool->control,
                                          CLAY_DIRTY_IMAGE_SIZE      |
                                          CLAY_DIRTY_IMAGE_STRUCTURE |
                                          CLAY_DIRTY_DRAWABLE        |
                                          CLAY_DIRTY_SELECTION       |
                                          CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_active_modifiers (tool->control,
                                          CLAY_TOOL_ACTIVE_MODIFIERS_SAME);
  clay_tool_control_set_precision        (tool->control,
                                          CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_cursor           (tool->control,
                                          CLAY_CURSOR_CROSSHAIR_SMALL);
  clay_tool_control_set_action_opacity   (tool->control,
                                          "tools-transform-preview-opacity-set");

  tg_tool->strokes = g_ptr_array_new ();
}

static void
clay_transform_grid_tool_finalize (GObject *object)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (object);

  g_clear_object (&tg_tool->gui);
  g_clear_pointer (&tg_tool->strokes, g_ptr_array_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_transform_grid_tool_initialize (ClayTool     *tool,
                                     ClayDisplay  *display,
                                     GError      **error)
{
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tool);
  ClayTransformGridTool    *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);
  ClayImage                *image      = clay_display_get_image (display);
  GList                    *drawables;
  GList                    *objects;
  GList                    *iter;
  UndoInfo                 *undo_info;

  objects = clay_transform_tool_check_selected_objects (tr_tool, display, error);

  if (! objects)
    return FALSE;

  drawables = clay_image_get_selected_drawables (image);
  if (g_list_length (drawables) < 1)
    {
      clay_tool_message_literal (tool, display, _("No selected drawables."));
      g_list_free (drawables);
      g_list_free (objects);
      return FALSE;
    }

  tool->display    = display;
  tool->drawables  = drawables;

  tr_tool->objects = objects;

  for (iter = objects; iter; iter = iter->next)
    if (CLAY_IS_DRAWABLE (iter->data))
      clay_viewable_preview_freeze (iter->data);

  /*  Initialize the transform_grid tool dialog  */
  if (! tg_tool->gui)
    clay_transform_grid_tool_dialog (tg_tool);

  /*  Find the transform bounds for some tools (like scale,
   *  perspective) that actually need the bounds for initializing
   */
  clay_transform_tool_bounds (tr_tool, display);

  /*  Initialize the tool-specific trans_info, and adjust the tool dialog  */
  clay_transform_grid_tool_prepare (tg_tool, display);

  /*  Recalculate the tool's transformation matrix  */
  clay_transform_tool_recalc_matrix (tr_tool, display);

  /*  Get the on-canvas gui  */
  tg_tool->widget = clay_transform_grid_tool_get_widget (tg_tool);

  clay_transform_grid_tool_hide_selected_objects (tg_tool, objects);

  /*  start drawing the bounding box and handles...  */
  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);

  /* Initialize undo and redo lists */
  undo_info = undo_info_new ();
  tg_tool->undo_list = g_list_prepend (NULL, undo_info);
  tg_tool->redo_list = NULL;

  /*  Save the current transformation info  */
  memcpy (undo_info->trans_infos, tg_tool->trans_infos,
          sizeof (tg_tool->trans_infos));

  if (tg_options->direction_chain_button)
    gtk_widget_set_sensitive (tg_options->direction_chain_button, TRUE);

  return TRUE;
}

static void
clay_transform_grid_tool_control (ClayTool       *tool,
                                  ClayToolAction  action,
                                  ClayDisplay    *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_transform_grid_tool_halt (tg_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      if (tool->display)
        clay_transform_grid_tool_commit (tg_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_transform_grid_tool_button_press (ClayTool            *tool,
                                       const ClayCoords    *coords,
                                       guint32              time,
                                       GdkModifierType      state,
                                       ClayButtonPressType  press_type,
                                       ClayDisplay         *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      clay_tool_widget_hover (tg_tool->widget, coords, state, TRUE);

      if (clay_tool_widget_button_press (tg_tool->widget, coords, time, state,
                                         press_type))
        {
          tg_tool->grab_widget = tg_tool->widget;
        }
    }

  clay_tool_control_activate (tool->control);
}

static void
clay_transform_grid_tool_button_release (ClayTool              *tool,
                                         const ClayCoords      *coords,
                                         guint32                time,
                                         GdkModifierType        state,
                                         ClayButtonReleaseType  release_type,
                                         ClayDisplay           *display)
{
  ClayTransformTool     *tr_tool = CLAY_TRANSFORM_TOOL (tool);
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  clay_tool_control_halt (tool->control);

  if (tg_tool->grab_widget)
    {
      clay_tool_widget_button_release (tg_tool->grab_widget,
                                       coords, time, state, release_type);
      tg_tool->grab_widget = NULL;
    }

  if (release_type != CLAY_BUTTON_RELEASE_CANCEL)
    {
      /* We're done with an interaction, save it on the undo list */
      clay_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
    }
  else
    {
      UndoInfo *undo_info = tg_tool->undo_list->data;

      /*  Restore the last saved state  */
      memcpy (tg_tool->trans_infos, undo_info->trans_infos,
              sizeof (tg_tool->trans_infos));

      /*  recalculate the tool's transformation matrix  */
      clay_transform_tool_recalc_matrix (tr_tool, display);
    }
}

static void
clay_transform_grid_tool_motion (ClayTool         *tool,
                                 const ClayCoords *coords,
                                 guint32           time,
                                 GdkModifierType   state,
                                 ClayDisplay      *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->grab_widget)
    {
      clay_tool_widget_motion (tg_tool->grab_widget, coords, time, state);
    }
}

static void
clay_transform_grid_tool_modifier_key (ClayTool        *tool,
                                       GdkModifierType  key,
                                       gboolean         press,
                                       GdkModifierType  state,
                                       ClayDisplay     *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  if (tg_tool->widget)
    {
      CLAY_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                    state, display);
    }
  else
    {
      ClayTransformGridOptions *options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);

      if (key == clay_get_constrain_behavior_mask ())
        {
          g_object_set (options,
                        "frompivot-scale",       ! options->frompivot_scale,
                        "frompivot-shear",       ! options->frompivot_shear,
                        "frompivot-perspective", ! options->frompivot_perspective,
                        NULL);
        }
      else if (key == clay_get_extend_selection_mask ())
        {
          g_object_set (options,
                        "cornersnap",            ! options->cornersnap,
                        "constrain-move",        ! options->constrain_move,
                        "constrain-scale",       ! options->constrain_scale,
                        "constrain-rotate",      ! options->constrain_rotate,
                        "constrain-shear",       ! options->constrain_shear,
                        "constrain-perspective", ! options->constrain_perspective,
                        NULL);
        }
    }
}

static void
clay_transform_grid_tool_cursor_update (ClayTool         *tool,
                                        const ClayCoords *coords,
                                        GdkModifierType   state,
                                        ClayDisplay      *display)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tool);
  GList             *objects;

  objects = clay_transform_tool_check_selected_objects (tr_tool, display, NULL);

  if (display != tool->display && ! objects)
    {
      clay_tool_set_cursor (tool, display,
                            clay_tool_control_get_cursor (tool->control),
                            clay_tool_control_get_tool_cursor (tool->control),
                            CLAY_CURSOR_MODIFIER_BAD);
      return;
    }
  g_list_free (objects);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
clay_transform_grid_tool_can_undo (ClayTool    *tool,
                                   ClayDisplay *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->undo_list || ! tg_tool->undo_list->next)
    return NULL;

  return _("Transform Step");
}

static const gchar *
clay_transform_grid_tool_can_redo (ClayTool    *tool,
                                   ClayDisplay *display)
{
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tool);

  if (! tg_tool->redo_list)
    return NULL;

  return _("Transform Step");
}

static gboolean
clay_transform_grid_tool_undo (ClayTool    *tool,
                               ClayDisplay *display)
{
  ClayTransformTool      *tr_tool    = CLAY_TRANSFORM_TOOL (tool);
  ClayTransformGridTool  *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tool);
  ClayTransformOptions   *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  ClayTransformDirection  direction;

  undo_info = tg_tool->undo_list->data;
  direction = undo_info->direction;

  /* Move undo_info from undo_list to redo_list */
  tg_tool->redo_list = g_list_prepend (tg_tool->redo_list, undo_info);
  tg_tool->undo_list = g_list_remove (tg_tool->undo_list, undo_info);

  undo_info = tg_tool->undo_list->data;

  /*  Restore the previous transformation info  */
  memcpy (tg_tool->trans_infos, undo_info->trans_infos,
          sizeof (tg_tool->trans_infos));

  /*  Restore the previous transformation direction  */
  if (direction != tr_options->direction)
    {
      g_object_set (tr_options,
                    "direction", direction,
                    NULL);
    }

  /*  recalculate the tool's transformation matrix  */
  clay_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static gboolean
clay_transform_grid_tool_redo (ClayTool    *tool,
                               ClayDisplay *display)
{
  ClayTransformTool      *tr_tool    = CLAY_TRANSFORM_TOOL (tool);
  ClayTransformGridTool  *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tool);
  ClayTransformOptions   *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tool);
  UndoInfo               *undo_info;
  ClayTransformDirection  direction;

  undo_info = tg_tool->redo_list->data;
  direction = undo_info->direction;

  /* Move undo_info from redo_list to undo_list */
  tg_tool->undo_list = g_list_prepend (tg_tool->undo_list, undo_info);
  tg_tool->redo_list = g_list_remove (tg_tool->redo_list, undo_info);

  /*  Restore the previous transformation info  */
  memcpy (tg_tool->trans_infos, undo_info->trans_infos,
          sizeof (tg_tool->trans_infos));

  /*  Restore the previous transformation direction  */
  if (direction != tr_options->direction)
    {
      g_object_set (tr_options,
                    "direction", direction,
                    NULL);
    }

  /*  recalculate the tool's transformation matrix  */
  clay_transform_tool_recalc_matrix (tr_tool, display);

  return TRUE;
}

static void
clay_transform_grid_tool_options_notify (ClayTool         *tool,
                                         ClayToolOptions  *options,
                                         const GParamSpec *pspec)
{
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tool);
  ClayTransformGridTool    *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_OPTIONS (options);

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);

  if (! strcmp (pspec->name, "type"))
    {
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      return;
    }

  if (! tg_tool->widget)
    return;

  if (! strcmp (pspec->name, "direction"))
    {
      /*  recalculate the tool's transformation matrix  */
      clay_transform_tool_recalc_matrix (tr_tool, tool->display);
    }
  else if (! strcmp (pspec->name, "show-preview") ||
           ! strcmp (pspec->name, "composited-preview"))
    {
      if (tg_tool->previews)
        {
          ClayDisplay *display;
          GList       *objects;

          display = tool->display;
          objects = clay_transform_tool_get_selected_objects (tr_tool, display);

          if (objects)
            {
              if (tg_options->show_preview &&
                  ! clay_transform_grid_tool_composited_preview (tg_tool))
                {
                  clay_transform_grid_tool_hide_selected_objects (tg_tool, objects);
                }
              else
                {
                  clay_transform_grid_tool_show_selected_objects (tg_tool);
                }

              g_list_free (objects);
            }

          clay_transform_grid_tool_update_preview (tg_tool);
        }
    }
  else if (! strcmp (pspec->name, "interpolation") ||
           ! strcmp (pspec->name, "clip")          ||
           ! strcmp (pspec->name, "preview-opacity"))
    {
      clay_transform_grid_tool_update_preview (tg_tool);
    }
  else if (g_str_has_prefix (pspec->name, "constrain-") ||
           g_str_has_prefix (pspec->name, "frompivot-") ||
           ! strcmp (pspec->name, "fixedpivot") ||
           ! strcmp (pspec->name, "cornersnap"))
    {
      clay_transform_grid_tool_dialog_update (tg_tool);
    }
}

static void
clay_transform_grid_tool_draw (ClayDrawTool *draw_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (draw_tool);
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (draw_tool);
  ClayTransformGridTool    *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (draw_tool);
  ClayTransformGridOptions *options    = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_OPTIONS (options);
  ClayDisplayShell         *shell      = clay_display_get_shell (tool->display);
  ClayImage                *image      = clay_display_get_image (tool->display);
  ClayMatrix3               matrix     = tr_tool->transform;
  ClayCanvasItem           *item;

  if (tr_options->direction == CLAY_TRANSFORM_BACKWARD)
    clay_matrix3_invert (&matrix);

  if (tr_options->type == CLAY_TRANSFORM_TYPE_LAYER ||
      tr_options->type == CLAY_TRANSFORM_TYPE_IMAGE)
    {
      GList *pickables = NULL;
      GList *iter;

      if (tr_options->type == CLAY_TRANSFORM_TYPE_IMAGE)
        {
          if (! shell->show_all)
            pickables = g_list_prepend (pickables, image);
          else
            pickables = g_list_prepend (pickables, clay_image_get_projection (image));
        }
      else
        {
          for (iter = tool->drawables; iter; iter = iter->next)
            pickables = g_list_prepend (pickables, iter->data);
        }

      for (iter = pickables; iter; iter = iter->next)
        {
          ClayCanvasItem *preview;

          preview = clay_draw_tool_add_transform_preview (draw_tool,
                                                          CLAY_PICKABLE (iter->data),
                                                          &matrix,
                                                          tr_tool->x1,
                                                          tr_tool->y1,
                                                          tr_tool->x2,
                                                          tr_tool->y2);
          tg_tool->previews = g_list_prepend (tg_tool->previews, preview);

          /* NOT g_set_weak_pointr() because the pointer is already set */
          g_object_add_weak_pointer (G_OBJECT (tg_tool->previews->data),
                                     (gpointer) &tg_tool->previews->data);
        }
      g_list_free (pickables);
    }

  if (tr_options->type == CLAY_TRANSFORM_TYPE_SELECTION)
    {
      const ClayBoundSeg *segs_in;
      const ClayBoundSeg *segs_out;
      gint                n_segs_in;
      gint                n_segs_out;

      clay_channel_boundary (clay_image_get_mask (image),
                             &segs_in, &segs_out,
                             &n_segs_in, &n_segs_out,
                             0, 0, 0, 0);

      if (segs_in)
        {
          g_set_weak_pointer
            (&tg_tool->boundary_in,
             clay_draw_tool_add_boundary (draw_tool,
                                          segs_in, n_segs_in,
                                          &matrix,
                                          0, 0));

          clay_canvas_item_set_visible (tg_tool->boundary_in,
                                        tr_tool->transform_valid);
        }

      if (segs_out)
        {
          g_set_weak_pointer
            (&tg_tool->boundary_out,
             clay_draw_tool_add_boundary (draw_tool,
                                          segs_out, n_segs_out,
                                          &matrix,
                                          0, 0));

          clay_canvas_item_set_visible (tg_tool->boundary_out,
                                        tr_tool->transform_valid);
        }
    }
  else if (tr_options->type == CLAY_TRANSFORM_TYPE_PATH)
    {
      GList *iter;

      for (iter = clay_image_get_selected_paths (image); iter; iter = iter->next)
        {
          ClayPath   *path   = iter->data;
          ClayStroke *stroke = NULL;

          while ((stroke = clay_path_stroke_get_next (path, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = clay_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  item =
                    clay_draw_tool_add_strokes (draw_tool,
                                                &g_array_index (coords,
                                                                ClayCoords, 0),
                                                coords->len, &matrix, FALSE);

                  g_ptr_array_add (tg_tool->strokes, item);
                  g_object_weak_ref (G_OBJECT (item),
                                     (GWeakNotify) g_ptr_array_remove,
                                     tg_tool->strokes);

                  clay_canvas_item_set_visible (item, tr_tool->transform_valid);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }

  CLAY_DRAW_TOOL_CLASS (parent_class)->draw (draw_tool);

  clay_transform_grid_tool_update_preview (tg_tool);
}

static void
clay_transform_grid_tool_recalc_matrix (ClayTransformTool *tr_tool)
{
  ClayTransformGridTool    *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tr_tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tr_tool);

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      ClayMatrix3 forward_transform;
      ClayMatrix3 backward_transform;
      gboolean    forward_transform_valid;
      gboolean    backward_transform_valid;

      tg_tool->trans_info      = tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD];
      forward_transform_valid  = clay_transform_grid_tool_info_to_matrix (
                                   tg_tool, &forward_transform);

      tg_tool->trans_info      = tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD];
      backward_transform_valid = clay_transform_grid_tool_info_to_matrix (
                                   tg_tool, &backward_transform);

      if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tg_options->direction_linked)
        {
          ClayMatrix3 transform = tr_tool->transform;

          switch (tr_options->direction)
            {
            case CLAY_TRANSFORM_FORWARD:
              if (forward_transform_valid)
                {
                  clay_matrix3_invert (&transform);

                  backward_transform = forward_transform;
                  clay_matrix3_mult (&transform, &backward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD];
                  clay_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &backward_transform);
                  backward_transform_valid =
                    clay_transform_grid_tool_info_to_matrix (
                      tg_tool, &backward_transform);
                }
              break;

            case CLAY_TRANSFORM_BACKWARD:
              if (backward_transform_valid)
                {
                  forward_transform = backward_transform;
                  clay_matrix3_mult (&transform, &forward_transform);

                  tg_tool->trans_info =
                    tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD];
                  clay_transform_grid_tool_matrix_to_info (tg_tool,
                                                           &forward_transform);
                  forward_transform_valid =
                    clay_transform_grid_tool_info_to_matrix (
                      tg_tool, &forward_transform);
                }
              break;
            }
        }
      else if (forward_transform_valid && backward_transform_valid)
        {
          tr_tool->transform = backward_transform;
          clay_matrix3_invert (&tr_tool->transform);
          clay_matrix3_mult (&forward_transform, &tr_tool->transform);
        }

      tr_tool->transform_valid = forward_transform_valid &&
                                 backward_transform_valid;
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  clay_transform_grid_tool_dialog_update (tg_tool);
  clay_transform_grid_tool_update_sensitivity (tg_tool);
  clay_transform_grid_tool_update_widget (tg_tool);
  clay_transform_grid_tool_update_preview (tg_tool);

  if (tg_tool->gui)
    clay_tool_gui_show (tg_tool->gui);
}

static gchar *
clay_transform_grid_tool_get_undo_desc (ClayTransformTool *tr_tool)
{
  ClayTransformGridTool *tg_tool    = CLAY_TRANSFORM_GRID_TOOL (tr_tool);
  ClayTransformOptions  *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  gchar                 *result;

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      TransInfo trans_info;

      memcpy (&trans_info, &tg_tool->init_trans_info, sizeof (TransInfo));

      tg_tool->trans_info = trans_info;
      clay_transform_grid_tool_matrix_to_info (tg_tool, &tr_tool->transform);
      result = CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD],
                             tg_tool->init_trans_info))
    {
      tg_tool->trans_info = tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD];
      result = CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);
    }
  else if (trans_info_equal (tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD],
                             tg_tool->init_trans_info))
    {
      gchar *desc;

      tg_tool->trans_info = tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD];
      desc = CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_undo_desc (
        tg_tool);

      result = g_strdup_printf (_("%s (Corrective)"), desc);

      g_free (desc);
    }
  else
    {
      result = CLAY_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (
        tr_tool);
    }

  tg_tool->trans_info = tg_tool->trans_infos[tr_options->direction];

  return result;
}

static ClayTransformDirection
clay_transform_grid_tool_get_direction (ClayTransformTool *tr_tool)
{
  return CLAY_TRANSFORM_FORWARD;
}

static GeglBuffer *
clay_transform_grid_tool_transform (ClayTransformTool  *tr_tool,
                                    GList              *objects,
                                    GeglBuffer         *orig_buffer,
                                    gint                orig_offset_x,
                                    gint                orig_offset_y,
                                    ClayColorProfile  **buffer_profile,
                                    gint               *new_offset_x,
                                    gint               *new_offset_y)
{
  ClayTool              *tool    = CLAY_TOOL (tr_tool);
  ClayTransformGridTool *tg_tool = CLAY_TRANSFORM_GRID_TOOL (tr_tool);
  ClayDisplay           *display = tool->display;
  ClayImage             *image   = clay_display_get_image (display);
  GeglBuffer            *new_buffer;

  /*  Send the request for the transformation to the tool...
   */
  new_buffer =
    CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->transform (tg_tool,
                                                             objects,
                                                             orig_buffer,
                                                             orig_offset_x,
                                                             orig_offset_y,
                                                             buffer_profile,
                                                             new_offset_x,
                                                             new_offset_y);

  clay_image_undo_push (image, CLAY_TYPE_TRANSFORM_GRID_TOOL_UNDO,
                        CLAY_UNDO_TRANSFORM_GRID, NULL,
                        0,
                        "transform-tool", tg_tool,
                        NULL);

  return new_buffer;
}

static void
clay_transform_grid_tool_real_apply_info (ClayTransformGridTool *tg_tool,
                                          const TransInfo        info)
{
  memcpy (tg_tool->trans_info, info, sizeof (TransInfo));
}

static gchar *
clay_transform_grid_tool_real_get_undo_desc (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  return CLAY_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
}

static void
clay_transform_grid_tool_real_update_widget (ClayTransformGridTool *tg_tool)
{
  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      ClayMatrix3 transform;

      clay_transform_grid_tool_info_to_matrix (tg_tool, &transform);

      g_object_set (tg_tool->widget,
                    "transform", &transform,
                    NULL);
    }
}

static void
clay_transform_grid_tool_real_widget_changed (ClayTransformGridTool *tg_tool)
{
  ClayTool          *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayToolWidget    *widget  = tg_tool->widget;

  /* suppress the call to ClayTransformGridTool::update_widget() when
   * recalculating the matrix
   */
  tg_tool->widget = NULL;

  clay_transform_tool_recalc_matrix (tr_tool, tool->display);

  tg_tool->widget = widget;
}

static GeglBuffer *
clay_transform_grid_tool_real_transform (ClayTransformGridTool  *tg_tool,
                                         GList                  *objects,
                                         GeglBuffer             *orig_buffer,
                                         gint                    orig_offset_x,
                                         gint                    orig_offset_y,
                                         ClayColorProfile      **buffer_profile,
                                         gint                   *new_offset_x,
                                         gint                   *new_offset_y)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  return CLAY_TRANSFORM_TOOL_CLASS (parent_class)->transform (tr_tool,
                                                              objects,
                                                              orig_buffer,
                                                              orig_offset_x,
                                                              orig_offset_y,
                                                              buffer_profile,
                                                              new_offset_x,
                                                              new_offset_y);
}

static void
clay_transform_grid_tool_widget_changed (ClayToolWidget        *widget,
                                         ClayTransformGridTool *tg_tool)
{
  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed)
    CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->widget_changed (tg_tool);
}

static void
clay_transform_grid_tool_widget_response (ClayToolWidget        *widget,
                                          gint                   response_id,
                                          ClayTransformGridTool *tg_tool)
{
  switch (response_id)
    {
    case CLAY_TOOL_WIDGET_RESPONSE_CONFIRM:
      clay_transform_grid_tool_response (NULL, GTK_RESPONSE_OK, tg_tool);
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_CANCEL:
      clay_transform_grid_tool_response (NULL, GTK_RESPONSE_CANCEL, tg_tool);
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_RESET:
      clay_transform_grid_tool_response (NULL, RESPONSE_RESET, tg_tool);
      break;
    }
}

static void
clay_transform_grid_tool_filter_flush (ClayDrawableFilter    *filter,
                                       ClayTransformGridTool *tg_tool)
{
  ClayTool  *tool = CLAY_TOOL (tg_tool);
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

static void
clay_transform_grid_tool_halt (ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (tg_tool);
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GList                    *iter;

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tg_tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tg_tool));

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tg_tool), NULL);
  g_clear_object (&tg_tool->widget);

  g_clear_pointer (&tg_tool->filters, g_hash_table_unref);
  g_clear_pointer (&tg_tool->preview_drawables, g_list_free);

  for (iter = tg_tool->previews; iter; iter = iter->next)
    {
      if (iter->data)
        g_object_unref (iter->data);
    }
  g_clear_pointer (&tg_tool->previews, g_list_free);

  if (tg_tool->gui)
    clay_tool_gui_hide (tg_tool->gui);

  if (tg_tool->redo_list)
    {
      g_list_free_full (tg_tool->redo_list, (GDestroyNotify) undo_info_free);
      tg_tool->redo_list = NULL;
    }

  if (tg_tool->undo_list)
    {
      g_list_free_full (tg_tool->undo_list, (GDestroyNotify) undo_info_free);
      tg_tool->undo_list = NULL;
    }

  clay_transform_grid_tool_show_selected_objects (tg_tool);

  if (tg_options->direction_chain_button)
    {
      g_object_set (tg_options,
                    "direction-linked", FALSE,
                    NULL);

      gtk_widget_set_sensitive (tg_options->direction_chain_button, FALSE);
    }

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (tr_tool->objects)
    {
      GList *iter;

      for (iter = tr_tool->objects; iter; iter = iter->next)
        if (CLAY_IS_DRAWABLE (iter->data))
          clay_viewable_preview_thaw (iter->data);

      g_list_free (tr_tool->objects);
      tr_tool->objects = NULL;
    }
}

static void
clay_transform_grid_tool_commit (ClayTransformGridTool *tg_tool)
{
  ClayTool          *tool    = CLAY_TOOL (tg_tool);
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayDisplay       *display = tool->display;

  /* undraw the tool before we muck around with the transform matrix */
  clay_draw_tool_stop (CLAY_DRAW_TOOL (tg_tool));

  clay_transform_tool_transform (tr_tool, display);
}

static void
clay_transform_grid_tool_dialog (ClayTransformGridTool *tg_tool)
{
  ClayTool         *tool      = CLAY_TOOL (tg_tool);
  ClayToolInfo     *tool_info = tool->tool_info;
  ClayDisplayShell *shell;
  const gchar      *ok_button_label;

  if (! CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog)
    return;

  g_return_if_fail (tool->display != NULL);

  shell = clay_display_get_shell (tool->display);

  ok_button_label = CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->ok_button_label;

  tg_tool->gui = clay_tool_gui_new (tool_info,
                                    NULL, NULL, NULL, NULL,
                                    clay_widget_get_monitor (GTK_WIDGET (shell)),
                                    TRUE,
                                    NULL);

  clay_tool_gui_add_button   (tg_tool->gui, _("_Reset"),     RESPONSE_RESET);
  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust)
    clay_tool_gui_add_button (tg_tool->gui, _("Re_adjust"),  RESPONSE_READJUST);
  clay_tool_gui_add_button   (tg_tool->gui, _("_Cancel"),    GTK_RESPONSE_CANCEL);
  clay_tool_gui_add_button   (tg_tool->gui, ok_button_label, GTK_RESPONSE_OK);

  clay_tool_gui_set_auto_overlay (tg_tool->gui, TRUE);
  clay_tool_gui_set_default_response (tg_tool->gui, GTK_RESPONSE_OK);

  clay_tool_gui_set_alternative_button_order (tg_tool->gui,
                                              RESPONSE_RESET,
                                              RESPONSE_READJUST,
                                              GTK_RESPONSE_OK,
                                              GTK_RESPONSE_CANCEL,
                                              -1);

  g_signal_connect (tg_tool->gui, "response",
                    G_CALLBACK (clay_transform_grid_tool_response),
                    tg_tool);

  CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog (tg_tool);
}

static void
clay_transform_grid_tool_dialog_update (ClayTransformGridTool *tg_tool)
{
  if (tg_tool->gui &&
      CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update)
    {
      CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->dialog_update (tg_tool);
    }
}

static void
clay_transform_grid_tool_prepare (ClayTransformGridTool *tg_tool,
                                  ClayDisplay           *display)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  if (tg_tool->gui)
    {
      GList *objects = clay_transform_tool_get_selected_objects (tr_tool, display);

      clay_tool_gui_set_shell (tg_tool->gui, clay_display_get_shell (display));
      clay_tool_gui_set_viewables (tg_tool->gui, objects);
      g_list_free (objects);
    }

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare)
    {
      tg_tool->trans_info = tg_tool->init_trans_info;
      CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->prepare (tg_tool);

      memcpy (tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
      memcpy (tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD],
              tg_tool->init_trans_info, sizeof (TransInfo));
    }

  clay_matrix3_identity (&tr_tool->transform);
  tr_tool->transform_valid = TRUE;
}

static ClayToolWidget *
clay_transform_grid_tool_get_widget (ClayTransformGridTool *tg_tool)
{
  static const gchar *properties[] =
  {
    "constrain-move",
    "constrain-scale",
    "constrain-rotate",
    "constrain-shear",
    "constrain-perspective",
    "frompivot-scale",
    "frompivot-shear",
    "frompivot-perspective",
    "cornersnap",
    "fixedpivot"
  };

  ClayToolWidget *widget = NULL;

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget)
    {
      ClayTransformGridOptions *options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
      gint                      i;

      widget = CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->get_widget (tg_tool);

      clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tg_tool), widget);

      g_object_bind_property (G_OBJECT (options), "grid-type",
                              G_OBJECT (widget),  "guide-type",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);
      g_object_bind_property (G_OBJECT (options), "grid-size",
                              G_OBJECT (widget),  "n-guides",
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);

      for (i = 0; i < G_N_ELEMENTS (properties); i++)
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      g_signal_connect (widget, "changed",
                        G_CALLBACK (clay_transform_grid_tool_widget_changed),
                        tg_tool);
      g_signal_connect (widget, "response",
                        G_CALLBACK (clay_transform_grid_tool_widget_response),
                        tg_tool);
    }

  return widget;
}

static void
clay_transform_grid_tool_update_widget (ClayTransformGridTool *tg_tool)
{
  if (tg_tool->widget &&
      CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget)
    {
      g_signal_handlers_block_by_func (
        tg_tool->widget,
        G_CALLBACK (clay_transform_grid_tool_widget_changed),
        tg_tool);

      CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->update_widget (tg_tool);

      g_signal_handlers_unblock_by_func (
        tg_tool->widget,
        G_CALLBACK (clay_transform_grid_tool_widget_changed),
        tg_tool);
    }
}

static void
clay_transform_grid_tool_response (ClayToolGui           *gui,
                                   gint                   response_id,
                                   ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (tg_tool);
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  ClayDisplay              *display    = tool->display;

  /* we can get here while already committing a transformation.  just return in
   * this case.  see issue #4734.
   */
  if (! clay_draw_tool_is_active (CLAY_DRAW_TOOL (tg_tool)))
    return;

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        gboolean direction_linked;

        /*  restore the initial transformation info  */
        memcpy (tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));
        memcpy (tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD],
                tg_tool->init_trans_info,
                sizeof (TransInfo));

        /*  recalculate the tool's transformation matrix  */
        direction_linked             = tg_options->direction_linked;
        tg_options->direction_linked = FALSE;
        clay_transform_tool_recalc_matrix (tr_tool, display);
        tg_options->direction_linked = direction_linked;

        /*  push the restored info to the undo stack  */
        clay_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
      }
      break;

    case RESPONSE_READJUST:
      if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust       &&
          CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info &&
          tr_tool->transform_valid)
        {
          TransInfo old_trans_infos[2];
          gboolean  direction_linked;
          gboolean  transform_valid;

          /*  save the current transformation info  */
          memcpy (old_trans_infos, tg_tool->trans_infos,
                  sizeof (old_trans_infos));

          /*  readjust the transformation info to view  */
          CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->readjust (tg_tool);

          /*  recalculate the tool's transformation matrix, preserving the
           *  overall transformation
           */
          direction_linked             = tg_options->direction_linked;
          tg_options->direction_linked = TRUE;
          clay_transform_tool_recalc_matrix (tr_tool, display);
          tg_options->direction_linked = direction_linked;

          transform_valid = tr_tool->transform_valid;

          /*  if the resulting transformation is invalid, or if the
           *  transformation info is already adjusted to view ...
           */
          if (! transform_valid ||
              trans_infos_equal (old_trans_infos, tg_tool->trans_infos))
            {
              /*  ... readjust the transformation info to the item bounds  */
              ClayMatrix3 transform = tr_tool->transform;

              if (tr_options->direction == CLAY_TRANSFORM_BACKWARD)
                clay_matrix3_invert (&transform);

              CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->apply_info (
                tg_tool, tg_tool->init_trans_info);
              CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
                tg_tool, &transform);

              /*  recalculate the tool's transformation matrix, preserving the
               *  overall transformation
               */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = TRUE;
              clay_transform_tool_recalc_matrix (tr_tool, display);
              tg_options->direction_linked = direction_linked;

              if (! tr_tool->transform_valid ||
                  ! trans_infos_equal (old_trans_infos, tg_tool->trans_infos))
                {
                  transform_valid = tr_tool->transform_valid;
                }
            }

          if (transform_valid)
            {
              /*  push the new info to the undo stack  */
              clay_transform_grid_tool_push_internal_undo (tg_tool, FALSE);
            }
          else
            {
              /*  restore the old transformation info  */
              memcpy (tg_tool->trans_infos, old_trans_infos,
                      sizeof (old_trans_infos));

              /*  recalculate the tool's transformation matrix  */
              direction_linked             = tg_options->direction_linked;
              tg_options->direction_linked = FALSE;
              clay_transform_tool_recalc_matrix (tr_tool, display);
              tg_options->direction_linked = direction_linked;

              clay_tool_message_literal (tool, tool->display,
                                         _("Cannot readjust the transformation"));
            }
        }
      break;

    case GTK_RESPONSE_OK:
      g_return_if_fail (display != NULL);
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);
      break;

    default:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);

      /*  update the undo actions / menu items  */
      if (display)
        clay_image_flush (clay_display_get_image (display));
      break;
    }
}

static gboolean
clay_transform_grid_tool_composited_preview (ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (tg_tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  ClayImage                *image      = clay_display_get_image (tool->display);

  return tg_options->composited_preview                &&
         tr_options->type == CLAY_TRANSFORM_TYPE_LAYER &&
         clay_channel_is_empty (clay_image_get_mask (image));
}

static void
clay_transform_grid_tool_update_sensitivity (ClayTransformGridTool *tg_tool)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tg_tool);

  if (! tg_tool->gui)
    return;

  clay_tool_gui_set_response_sensitive (
    tg_tool->gui, GTK_RESPONSE_OK,
    tr_tool->transform_valid);

  clay_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_RESET,
    ! (trans_info_equal (tg_tool->trans_infos[CLAY_TRANSFORM_FORWARD],
                         tg_tool->init_trans_info) &&
       trans_info_equal (tg_tool->trans_infos[CLAY_TRANSFORM_BACKWARD],
                         tg_tool->init_trans_info)));

  clay_tool_gui_set_response_sensitive (
    tg_tool->gui, RESPONSE_READJUST,
    tr_tool->transform_valid);
}

static void
clay_transform_grid_tool_update_preview (ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool       = CLAY_TOOL (tg_tool);
  ClayTransformTool        *tr_tool    = CLAY_TRANSFORM_TOOL (tg_tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
  ClayTransformGridOptions *tg_options = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  GList                    *iter;
  gint                      i;

  if (! tool->display)
    return;

  if (tg_options->show_preview                              &&
      clay_transform_grid_tool_composited_preview (tg_tool) &&
      tr_tool->transform_valid)
    {
      GHashTableIter  iter;
      ClayDrawable   *drawable;
      Filter         *filter;
      gboolean        flush = FALSE;

      if (! tg_tool->filters)
        {
          tg_tool->filters = g_hash_table_new_full (
            g_direct_hash, g_direct_equal,
            NULL, (GDestroyNotify) filter_free);

          clay_transform_grid_tool_update_filters (tg_tool);
        }

      g_hash_table_iter_init (&iter, tg_tool->filters);

      while (g_hash_table_iter_next (&iter,
                                     (gpointer *) &drawable,
                                     (gpointer *) &filter))
        {
          ClayMatrix3   transform;
          GeglRectangle bounds;
          gint          offset_x;
          gint          offset_y;
          gint          width;
          gint          height;
          gint          x1, y1;
          gint          x2, y2;
          gboolean      update = FALSE;

          if (! filter->filter)
            continue;

          clay_item_get_offset (CLAY_ITEM (drawable), &offset_x, &offset_y);

          width  = clay_item_get_width  (CLAY_ITEM (drawable));
          height = clay_item_get_height (CLAY_ITEM (drawable));

          clay_matrix3_identity (&transform);
          clay_matrix3_translate (&transform, +offset_x, +offset_y);
          clay_matrix3_mult (&tr_tool->transform, &transform);
          clay_matrix3_translate (&transform, -offset_x, -offset_y);

          clay_transform_resize_boundary (&tr_tool->transform,
                                          clay_item_get_clip (
                                            CLAY_ITEM (filter->root_drawable),
                                            tr_options->clip),
                                          offset_x,         offset_y,
                                          offset_x + width, offset_y + height,
                                          &x1,              &y1,
                                          &x2,              &y2);

          bounds.x      = x1 - offset_x;
          bounds.y      = y1 - offset_y;
          bounds.width  = x2 - x1;
          bounds.height = y2 - y1;

          if (! clay_matrix3_equal (&transform, &filter->transform))
            {
              filter->transform = transform;

              clay_gegl_node_set_matrix (filter->transform_node, &transform);

              update = TRUE;
            }

          if (! gegl_rectangle_equal (&bounds, &filter->bounds))
            {
              filter->bounds = bounds;

              gegl_node_set (filter->crop_node,
                             "x",      (gdouble) bounds.x,
                             "y",      (gdouble) bounds.y,
                             "width",  (gdouble) bounds.width,
                             "height", (gdouble) bounds.height,
                             NULL);

              update = TRUE;
            }

          if (CLAY_IS_LAYER (drawable))
            {
              clay_drawable_filter_set_add_alpha (
                filter->filter,
                tr_options->interpolation != CLAY_INTERPOLATION_NONE);
            }

          if (update)
            {
              if (tg_options->synchronous_preview)
                {
                  g_signal_handlers_block_by_func (
                    filter->filter,
                    G_CALLBACK (clay_transform_grid_tool_filter_flush),
                    tg_tool);
                }

              clay_drawable_filter_apply (filter->filter, NULL);

              if (tg_options->synchronous_preview)
                {
                  g_signal_handlers_unblock_by_func (
                    filter->filter,
                    G_CALLBACK (clay_transform_grid_tool_filter_flush),
                    tg_tool);

                  flush = TRUE;
                }
            }
        }

      if (flush)
        {
          ClayImage *image = clay_display_get_image (tool->display);

          clay_projection_flush_now (clay_image_get_projection (image), TRUE);
          clay_display_flush_now (tool->display);
        }
    }
  else
    {
      g_clear_pointer (&tg_tool->filters, g_hash_table_unref);
      g_clear_pointer (&tg_tool->preview_drawables, g_list_free);
    }

  for (iter = tg_tool->previews; iter; iter = iter->next)
    {
      ClayCanvasItem *preview = iter->data;

      if (preview == NULL)
        continue;

      if (tg_options->show_preview                                &&
          ! clay_transform_grid_tool_composited_preview (tg_tool) &&
          tr_tool->transform_valid)
        {
          clay_canvas_item_begin_change (preview);
          clay_canvas_item_set_visible (preview, TRUE);
          g_object_set (
            preview,
            "transform", &tr_tool->transform,
            "clip",      clay_item_get_clip (CLAY_ITEM (tool->drawables->data),
                                             tr_options->clip),
            "opacity",   tg_options->preview_opacity,
            NULL);
          clay_canvas_item_end_change (preview);
        }
      else
        {
          clay_canvas_item_set_visible (preview, FALSE);
        }
    }

  if (tg_tool->boundary_in)
    {
      clay_canvas_item_begin_change (tg_tool->boundary_in);
      clay_canvas_item_set_visible (tg_tool->boundary_in,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_in,
                    "transform", &tr_tool->transform,
                    NULL);
      clay_canvas_item_end_change (tg_tool->boundary_in);
    }

  if (tg_tool->boundary_out)
    {
      clay_canvas_item_begin_change (tg_tool->boundary_out);
      clay_canvas_item_set_visible (tg_tool->boundary_out,
                                    tr_tool->transform_valid);
      g_object_set (tg_tool->boundary_out,
                    "transform", &tr_tool->transform,
                    NULL);
      clay_canvas_item_end_change (tg_tool->boundary_out);
    }

  for (i = 0; i < tg_tool->strokes->len; i++)
    {
      ClayCanvasItem *item = g_ptr_array_index (tg_tool->strokes, i);

      clay_canvas_item_begin_change (item);
      clay_canvas_item_set_visible (item, tr_tool->transform_valid);
      g_object_set (item,
                    "transform", &tr_tool->transform,
                    NULL);
      clay_canvas_item_end_change (item);
    }
}

static void
clay_transform_grid_tool_update_filters (ClayTransformGridTool *tg_tool)
{
  ClayTool                 *tool = CLAY_TOOL (tg_tool);
  GHashTable               *new_drawables;
  GList                    *drawables;
  GList                    *iter;
  ClayDrawable             *drawable;
  GHashTableIter            hash_iter;

  if (! tg_tool->filters)
    return;

  drawables = clay_image_item_list_filter (g_list_copy (tool->drawables));

  new_drawables = g_hash_table_new (g_direct_hash, g_direct_equal);

  for (iter = drawables; iter; iter = g_list_next (iter))
    g_hash_table_add (new_drawables, iter->data);

  for (iter = tg_tool->preview_drawables; iter; iter = g_list_next (iter))
    {
      drawable = iter->data;

      if (! g_hash_table_remove (new_drawables, drawable))
        clay_transform_grid_tool_remove_filter (drawable, tg_tool);
    }

  g_hash_table_iter_init (&hash_iter, new_drawables);

  while (g_hash_table_iter_next (&hash_iter, (gpointer *) &drawable, NULL))
    {
      AddFilterData data;

      data.tg_tool       = tg_tool;
      data.root_drawable = drawable;

      clay_transform_grid_tool_add_filter (drawable, &data);
    }

  g_hash_table_unref (new_drawables);

  g_list_free (tg_tool->preview_drawables);
  tg_tool->preview_drawables = drawables;
}

static void
clay_transform_grid_tool_hide_selected_objects (ClayTransformGridTool *tg_tool,
                                                GList                 *objects)
{
  ClayTransformGridOptions *options    = CLAY_TRANSFORM_GRID_TOOL_GET_OPTIONS (tg_tool);
  ClayTransformOptions     *tr_options = CLAY_TRANSFORM_OPTIONS (options);
  ClayDisplay              *display    = CLAY_TOOL (tg_tool)->display;
  ClayImage                *image      = clay_display_get_image (display);

  g_return_if_fail (tr_options->type != CLAY_TRANSFORM_TYPE_IMAGE ||
                    (g_list_length (objects) == 1 && CLAY_IS_IMAGE (objects->data)));

  g_list_free (tg_tool->hidden_objects);
  tg_tool->hidden_objects = NULL;

  if (options->show_preview)
    {
      if (tr_options->type == CLAY_TRANSFORM_TYPE_IMAGE)
        {
          tg_tool->hidden_objects = g_list_copy (objects);
          clay_display_shell_set_show_image (clay_display_get_shell (display),
                                             FALSE);
        }
      else
        {
          /*  hide only complete layers and channels, not layer masks  */
          GList *iter;

          for (iter = objects; iter; iter = iter->next)
            {
              if (tr_options->type == CLAY_TRANSFORM_TYPE_LAYER &&
                  ! options->composited_preview                 &&
                  CLAY_IS_DRAWABLE (iter->data)                 &&
                  ! CLAY_IS_LAYER_MASK (iter->data)             &&
                  clay_item_get_visible (iter->data)            &&
                  clay_channel_is_empty (clay_image_get_mask (image)))
                {
                  tg_tool->hidden_objects = g_list_prepend (tg_tool->hidden_objects, iter->data);

                  clay_item_set_visible (iter->data, FALSE, FALSE);
                }
            }

          clay_projection_flush (clay_image_get_projection (image));
        }
    }
}

static void
clay_transform_grid_tool_show_selected_objects (ClayTransformGridTool *tg_tool)
{
  if (tg_tool->hidden_objects)
    {
      ClayDisplay *display = CLAY_TOOL (tg_tool)->display;
      ClayImage   *image   = clay_display_get_image (display);
      GList       *iter;

      for (iter = tg_tool->hidden_objects; iter; iter = iter->next)
        {
          if (CLAY_IS_ITEM (iter->data))
            {
              clay_item_set_visible (CLAY_ITEM (iter->data), TRUE,
                                     FALSE);
            }
          else
            {
              g_return_if_fail (CLAY_IS_IMAGE (iter->data));

              clay_display_shell_set_show_image (clay_display_get_shell (display),
                                                 TRUE);
            }
        }

      g_list_free (tg_tool->hidden_objects);
      tg_tool->hidden_objects = NULL;

      clay_image_flush (image);
    }
}

static void
clay_transform_grid_tool_add_filter (ClayDrawable  *drawable,
                                     AddFilterData *data)
{
  Filter        *filter;
  ClayLayerMode  mode = CLAY_LAYER_MODE_NORMAL;

  if (CLAY_IS_LAYER (drawable))
    mode = clay_layer_get_mode (CLAY_LAYER (drawable));

  if (mode != CLAY_LAYER_MODE_PASS_THROUGH)
    {
      filter = filter_new (data->tg_tool, drawable, data->root_drawable, TRUE);
    }
  else
    {
      ClayContainer *container;

      filter = filter_new (data->tg_tool, drawable, data->root_drawable, FALSE);

      container = clay_viewable_get_children (CLAY_VIEWABLE (drawable));

      clay_container_foreach (container,
                              (GFunc) clay_transform_grid_tool_add_filter,
                              data);
    }

  g_hash_table_insert (data->tg_tool->filters, drawable, filter);

  if (CLAY_IS_LAYER (drawable))
    {
      ClayLayerMask *mask = clay_layer_get_mask (CLAY_LAYER (drawable));

      if (mask)
        clay_transform_grid_tool_add_filter (CLAY_DRAWABLE (mask), data);
    }
}

static void
clay_transform_grid_tool_remove_filter (ClayDrawable          *drawable,
                                        ClayTransformGridTool *tg_tool)
{
  Filter *filter = g_hash_table_lookup (tg_tool->filters, drawable);

  if (CLAY_IS_LAYER (drawable))
    {
      ClayLayerMask *mask = clay_layer_get_mask (CLAY_LAYER (drawable));

      if (mask)
        clay_transform_grid_tool_remove_filter (CLAY_DRAWABLE (mask), tg_tool);
    }

  if (! filter->filter)
    {
      ClayContainer *container;

      container = clay_viewable_get_children (CLAY_VIEWABLE (drawable));

      clay_container_foreach (container,
                              (GFunc) clay_transform_grid_tool_remove_filter,
                              tg_tool);
    }

  g_hash_table_remove (tg_tool->filters, drawable);
}

static void
clay_transform_grid_tool_effective_mode_changed (ClayLayer             *layer,
                                                 ClayTransformGridTool *tg_tool)
{
  Filter        *filter = g_hash_table_lookup (tg_tool->filters, layer);
  ClayLayerMode  mode;
  gboolean       old_pass_through;
  gboolean       new_pass_through;

  clay_layer_get_effective_mode (layer, &mode, NULL, NULL, NULL);

  old_pass_through = ! filter->filter;
  new_pass_through = mode == CLAY_LAYER_MODE_PASS_THROUGH;

  if (old_pass_through != new_pass_through)
    {
      AddFilterData data;

      data.tg_tool       = tg_tool;
      data.root_drawable = filter->root_drawable;

      clay_transform_grid_tool_remove_filter (CLAY_DRAWABLE (layer), tg_tool);
      clay_transform_grid_tool_add_filter    (CLAY_DRAWABLE (layer), &data);

      clay_transform_grid_tool_update_preview (tg_tool);
    }
}

static Filter *
filter_new (ClayTransformGridTool *tg_tool,
            ClayDrawable          *drawable,
            ClayDrawable          *root_drawable,
            gboolean               add_filter)
{
  Filter   *filter = g_slice_new0 (Filter);
  GeglNode *node;
  GeglNode *input_node;
  GeglNode *output_node;

  filter->tg_tool       = tg_tool;
  filter->drawable      = drawable;
  filter->root_drawable = root_drawable;

  if (add_filter)
    {
      node = gegl_node_new ();

      input_node  = gegl_node_get_input_proxy (node, "input");
      output_node = gegl_node_get_input_proxy (node, "output");

      filter->transform_node = gegl_node_new_child (
        node,
        "operation", "gegl:transform",
        "near-z",    CLAY_TRANSFORM_NEAR_Z,
        "sampler",   GEGL_SAMPLER_NEAREST,
        NULL);

      filter->crop_node = gegl_node_new_child (
        node,
        "operation", "gegl:crop",
        NULL);

      gegl_node_link_many (input_node,
                           filter->transform_node,
                           filter->crop_node,
                           output_node,
                           NULL);

      clay_gegl_node_set_underlying_operation (node, filter->transform_node);

      filter->filter =
        clay_drawable_filter_new (drawable,
                                  CLAY_TRANSFORM_TOOL_GET_CLASS (tg_tool)->undo_desc,
                                  node,
                                  clay_tool_get_icon_name (CLAY_TOOL (tg_tool)));
      clay_drawable_filter_set_temporary (filter->filter, TRUE);

      clay_drawable_filter_set_clip (filter->filter, FALSE);
      clay_drawable_filter_set_override_constraints (filter->filter, TRUE);

      g_signal_connect (
        filter->filter, "flush",
        G_CALLBACK (clay_transform_grid_tool_filter_flush),
        tg_tool);

      g_object_unref (node);
    }

  if (CLAY_IS_GROUP_LAYER (drawable))
    {
      g_signal_connect (
        drawable, "effective-mode-changed",
        G_CALLBACK (clay_transform_grid_tool_effective_mode_changed),
        tg_tool);
    }

  return filter;
}

static void
filter_free (Filter *filter)
{
  if (filter->filter)
    {
      clay_drawable_filter_abort (filter->filter);

      g_object_unref (filter->filter);
    }

  if (CLAY_IS_GROUP_LAYER (filter->drawable))
    {
      g_signal_handlers_disconnect_by_func (
        filter->drawable,
        clay_transform_grid_tool_effective_mode_changed,
        filter->tg_tool);
    }

  g_slice_free (Filter, filter);
}

static UndoInfo *
undo_info_new (void)
{
  return g_slice_new0 (UndoInfo);
}

static void
undo_info_free (UndoInfo *info)
{
  g_slice_free (UndoInfo, info);
}

static gboolean
trans_info_equal (const TransInfo trans_info1,
                  const TransInfo trans_info2)
{
  gint i;

  for (i = 0; i < TRANS_INFO_SIZE; i++)
    {
      if (fabs (trans_info1[i] - trans_info2[i]) > EPSILON)
        return FALSE;
    }

  return TRUE;
}

static gboolean
trans_infos_equal (const TransInfo *trans_infos1,
                   const TransInfo *trans_infos2)
{
  return trans_info_equal (trans_infos1[CLAY_TRANSFORM_FORWARD],
                           trans_infos2[CLAY_TRANSFORM_FORWARD]) &&
         trans_info_equal (trans_infos1[CLAY_TRANSFORM_BACKWARD],
                           trans_infos2[CLAY_TRANSFORM_BACKWARD]);
}

gboolean
clay_transform_grid_tool_info_to_matrix (ClayTransformGridTool *tg_tool,
                                         ClayMatrix3           *transform)
{
  g_return_val_if_fail (CLAY_IS_TRANSFORM_GRID_TOOL (tg_tool), FALSE);
  g_return_val_if_fail (transform != NULL, FALSE);

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix)
    {
      return CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->info_to_matrix (
        tg_tool, transform);
    }

  return FALSE;
}

void
clay_transform_grid_tool_matrix_to_info (ClayTransformGridTool *tg_tool,
                                         const ClayMatrix3     *transform)
{
  g_return_if_fail (CLAY_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (transform != NULL);

  if (CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info)
    {
      return CLAY_TRANSFORM_GRID_TOOL_GET_CLASS (tg_tool)->matrix_to_info (
        tg_tool, transform);
    }
}

void
clay_transform_grid_tool_push_internal_undo (ClayTransformGridTool *tg_tool,
                                             gboolean               compress)
{
  UndoInfo *undo_info;

  g_return_if_fail (CLAY_IS_TRANSFORM_GRID_TOOL (tg_tool));
  g_return_if_fail (tg_tool->undo_list != NULL);

  undo_info = tg_tool->undo_list->data;

  /* push current state on the undo list and set this state as the
   * current state, but avoid doing this if there were no changes
   */
  if (! trans_infos_equal (undo_info->trans_infos, tg_tool->trans_infos))
    {
      ClayTransformOptions *tr_options = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tg_tool);
      gint64                time       = 0;
      gboolean              flush = FALSE;

      if (tg_tool->undo_list->next == NULL)
        flush = TRUE;

      if (compress)
        time = g_get_monotonic_time ();

      if (! compress || time - undo_info->time >= UNDO_COMPRESS_TIME)
        {
          undo_info = undo_info_new ();

          tg_tool->undo_list = g_list_prepend (tg_tool->undo_list, undo_info);
        }

      undo_info->time      = time;
      undo_info->direction = tr_options->direction;
      memcpy (undo_info->trans_infos, tg_tool->trans_infos,
              sizeof (tg_tool->trans_infos));

      /* If we undid anything and started interacting, we have to
       * discard the redo history
       */
      if (tg_tool->redo_list)
        {
          g_list_free_full (tg_tool->redo_list,
                            (GDestroyNotify) undo_info_free);
          tg_tool->redo_list = NULL;

          flush = TRUE;
        }

      clay_transform_grid_tool_update_sensitivity (tg_tool);

      /*  update the undo actions / menu items  */
      if (flush)
        {
          clay_image_flush (
            clay_display_get_image (CLAY_TOOL (tg_tool)->display));
        }
    }
}
