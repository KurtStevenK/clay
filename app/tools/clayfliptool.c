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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydisplayconfig.h"

#include "core/claydrawable-transform.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-flip.h"
#include "core/clayimage-item-list.h"
#include "core/clayimage-pick-item.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claypickable.h"
#include "core/clayprogress.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claycanvasitem.h"
#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-appearance.h"

#include "clayflipoptions.h"
#include "clayfliptool.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void         clay_flip_tool_button_press  (ClayTool             *tool,
                                                  const ClayCoords     *coords,
                                                  guint32               time,
                                                  GdkModifierType       state,
                                                  ClayButtonPressType   press_type,
                                                  ClayDisplay          *display);
static gboolean     clay_flip_tool_key_press     (ClayTool             *tool,
                                                  GdkEventKey          *kevent,
                                                  ClayDisplay          *display);
static void         clay_flip_tool_modifier_key  (ClayTool             *tool,
                                                  GdkModifierType       key,
                                                  gboolean              press,
                                                  GdkModifierType       state,
                                                  ClayDisplay          *display);
static void         clay_flip_tool_oper_update   (ClayTool             *tool,
                                                  const ClayCoords     *coords,
                                                  GdkModifierType       state,
                                                  gboolean              proximity,
                                                  ClayDisplay          *display);
static void         clay_flip_tool_cursor_update (ClayTool             *tool,
                                                  const ClayCoords     *coords,
                                                  GdkModifierType       state,
                                                  ClayDisplay          *display);

static void         clay_flip_tool_draw          (ClayDrawTool         *draw_tool);

static gchar      * clay_flip_tool_get_undo_desc (ClayTransformTool    *tr_tool);
static GeglBuffer * clay_flip_tool_transform     (ClayTransformTool    *tr_tool,
                                                  GList                *objects,
                                                  GeglBuffer           *orig_buffer,
                                                  gint                  orig_offset_x,
                                                  gint                  orig_offset_y,
                                                  ClayColorProfile    **buffer_profile,
                                                  gint                 *new_offset_x,
                                                  gint                 *new_offset_y);

static ClayOrientationType   clay_flip_tool_get_flip_type  (ClayFlipTool      *flip);


G_DEFINE_TYPE (ClayFlipTool, clay_flip_tool, CLAY_TYPE_TRANSFORM_TOOL)

#define parent_class clay_flip_tool_parent_class


void
clay_flip_tool_register (ClayToolRegisterCallback  callback,
                         gpointer                  data)
{
  (* callback) (CLAY_TYPE_FLIP_TOOL,
                CLAY_TYPE_FLIP_OPTIONS,
                clay_flip_options_gui,
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-flip-tool",
                _("Flip"),
                _("Flip Tool: "
                  "Reverse the layer, selection or path horizontally or vertically"),
                N_("_Flip"), "<shift>F",
                NULL, CLAY_HELP_TOOL_FLIP,
                CLAY_ICON_TOOL_FLIP,
                data);
}

static void
clay_flip_tool_class_init (ClayFlipToolClass *klass)
{
  ClayToolClass          *tool_class      = CLAY_TOOL_CLASS (klass);
  ClayDrawToolClass      *draw_tool_class = CLAY_DRAW_TOOL_CLASS (klass);
  ClayTransformToolClass *tr_class        = CLAY_TRANSFORM_TOOL_CLASS (klass);

  tool_class->button_press  = clay_flip_tool_button_press;
  tool_class->key_press     = clay_flip_tool_key_press;
  tool_class->modifier_key  = clay_flip_tool_modifier_key;
  tool_class->oper_update   = clay_flip_tool_oper_update;
  tool_class->cursor_update = clay_flip_tool_cursor_update;

  draw_tool_class->draw     = clay_flip_tool_draw;

  tr_class->get_undo_desc   = clay_flip_tool_get_undo_desc;
  tr_class->transform       = clay_flip_tool_transform;

  tr_class->undo_desc       = C_("undo-type", "Flip");
  tr_class->progress_text   = _("Flipping");
}

static void
clay_flip_tool_init (ClayFlipTool *flip_tool)
{
  ClayTool *tool = CLAY_TOOL (flip_tool);

  clay_tool_control_set_snap_to            (tool->control, FALSE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_PIXEL_CENTER);
  clay_tool_control_set_cursor             (tool->control, CLAY_CURSOR_MOUSE);
  clay_tool_control_set_toggle_cursor      (tool->control, CLAY_CURSOR_MOUSE);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_FLIP_HORIZONTAL);
  clay_tool_control_set_toggle_tool_cursor (tool->control,
                                            CLAY_TOOL_CURSOR_FLIP_VERTICAL);

  flip_tool->guide = NULL;
}

static void
clay_flip_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tool);

  tool->display = display;

  clay_transform_tool_transform (tr_tool, display);

  clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
}

static gboolean
clay_flip_tool_key_press (ClayTool    *tool,
                          GdkEventKey *kevent,
                          ClayDisplay *display)
{
  ClayTransformTool   *tr_tool = CLAY_TRANSFORM_TOOL (tool);
  ClayFlipOptions     *options = CLAY_FLIP_TOOL_GET_OPTIONS (tool);
  ClayOrientationType  flip_type;

  flip_type = options->flip_type;

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
    case GDK_KEY_Down:
      options->flip_type = CLAY_ORIENTATION_VERTICAL;
      break;

    case GDK_KEY_Right:
    case GDK_KEY_Left:
      options->flip_type = CLAY_ORIENTATION_HORIZONTAL;
      break;

    default:
      return TRUE;
    }

  clay_transform_tool_transform (tr_tool, display);

  g_object_set (options, "flip-type", flip_type, NULL);

  return CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);
}

static void
clay_flip_tool_modifier_key (ClayTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             ClayDisplay     *display)
{
  ClayFlipOptions *options = CLAY_FLIP_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_toggle_behavior_mask ())
    {
      switch (options->flip_type)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          g_object_set (options,
                        "flip-type", CLAY_ORIENTATION_VERTICAL,
                        NULL);
          break;

        case CLAY_ORIENTATION_VERTICAL:
          g_object_set (options,
                        "flip-type", CLAY_ORIENTATION_HORIZONTAL,
                        NULL);
          break;

        default:
          break;
        }
    }
}

static void
clay_flip_tool_oper_update (ClayTool         *tool,
                            const ClayCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            ClayDisplay      *display)
{
  ClayFlipTool     *flip      = CLAY_FLIP_TOOL (tool);
  ClayDrawTool     *draw_tool = CLAY_DRAW_TOOL (tool);
  ClayFlipOptions  *options   = CLAY_FLIP_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell *shell     = clay_display_get_shell (display);
  ClayImage        *image     = clay_display_get_image (display);
  ClayGuide        *guide     = NULL;

  if (clay_display_shell_get_show_guides (shell) &&
      proximity)
    {
      gint snap_distance = display->config->snap_distance;

      guide = clay_image_pick_guide (image, coords->x, coords->y,
                                     FUNSCALEX (shell, snap_distance),
                                     FUNSCALEY (shell, snap_distance));
    }

  if (flip->guide != guide ||
      (guide && ! clay_draw_tool_is_active (draw_tool)))
    {
      clay_draw_tool_pause (draw_tool);

      if (clay_draw_tool_is_active (draw_tool) &&
          draw_tool->display != display)
        clay_draw_tool_stop (draw_tool);

      flip->guide = guide;

      if (! clay_draw_tool_is_active (draw_tool))
        clay_draw_tool_start (draw_tool, display);

      clay_draw_tool_resume (draw_tool);
    }

  gtk_widget_set_sensitive (options->direction_frame, guide == NULL);

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool,
                                               coords, state, proximity,
                                               display);
}

static void
clay_flip_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayTransformTool *tr_tool = CLAY_TRANSFORM_TOOL (tool);
  ClayFlipTool      *flip    = CLAY_FLIP_TOOL (tool);
  GList             *selected_objects;

  selected_objects = clay_transform_tool_check_selected_objects (tr_tool, display, NULL);

  if (! selected_objects)
    {
      clay_tool_set_cursor (tool, display,
                            clay_tool_control_get_cursor (tool->control),
                            clay_tool_control_get_tool_cursor (tool->control),
                            CLAY_CURSOR_MODIFIER_BAD);
      return;
    }
  g_list_free (selected_objects);

  clay_tool_control_set_toggled (tool->control,
                                 clay_flip_tool_get_flip_type (flip) ==
                                 CLAY_ORIENTATION_VERTICAL);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_flip_tool_draw (ClayDrawTool *draw_tool)
{
  ClayFlipTool *flip = CLAY_FLIP_TOOL (draw_tool);

  if (flip->guide)
    {
      ClayCanvasItem *item;
      ClayGuideStyle  style;

      style = clay_guide_get_style (flip->guide);

      item = clay_draw_tool_add_guide (draw_tool,
                                       clay_guide_get_orientation (flip->guide),
                                       clay_guide_get_position (flip->guide),
                                       style);
      clay_canvas_item_set_highlight (item, TRUE);
    }
}

static gchar *
clay_flip_tool_get_undo_desc (ClayTransformTool *tr_tool)
{
  ClayFlipTool *flip = CLAY_FLIP_TOOL (tr_tool);

  switch (clay_flip_tool_get_flip_type (flip))
    {
    case CLAY_ORIENTATION_HORIZONTAL:
      return g_strdup (C_("undo-type", "Flip horizontally"));

    case CLAY_ORIENTATION_VERTICAL:
      return g_strdup (C_("undo-type", "Flip vertically"));

    default:
      /* probably this is not actually reached today, but
       * could be if someone defined FLIP_DIAGONAL, say...
       */
      return CLAY_TRANSFORM_TOOL_CLASS (parent_class)->get_undo_desc (tr_tool);
    }
}

static GeglBuffer *
clay_flip_tool_transform (ClayTransformTool *tr_tool,
                          GList             *objects,
                          GeglBuffer        *orig_buffer,
                          gint               orig_offset_x,
                          gint               orig_offset_y,
                          ClayColorProfile **buffer_profile,
                          gint              *new_offset_x,
                          gint              *new_offset_y)
{
  ClayFlipTool         *flip        = CLAY_FLIP_TOOL (tr_tool);
  ClayFlipOptions      *options     = CLAY_FLIP_TOOL_GET_OPTIONS (tr_tool);
  ClayTransformOptions *tr_options  = CLAY_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  ClayContext          *context     = CLAY_CONTEXT (options);
  ClayOrientationType   flip_type   = CLAY_ORIENTATION_UNKNOWN;
  gdouble               axis        = 0.0;
  gboolean              clip_result = FALSE;
  GeglBuffer           *ret         = NULL;

  flip_type = clay_flip_tool_get_flip_type (flip);

  if (flip->guide)
    {
      axis = clay_guide_get_position (flip->guide);
    }
  else
    {
      switch (flip_type)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          axis = ((gdouble) tr_tool->x1 +
                  (gdouble) (tr_tool->x2 - tr_tool->x1) / 2.0);
          break;

        case CLAY_ORIENTATION_VERTICAL:
          axis = ((gdouble) tr_tool->y1 +
                  (gdouble) (tr_tool->y2 - tr_tool->y1) / 2.0);
          break;

        default:
          break;
        }
    }

  switch (tr_options->clip)
    {
    case CLAY_TRANSFORM_RESIZE_ADJUST:
      clip_result = FALSE;
      break;

    case CLAY_TRANSFORM_RESIZE_CLIP:
      clip_result = TRUE;
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  if (orig_buffer)
    {
      /*  this happens when transforming a selection cut out of a
       *  normal drawable
       */

      g_return_val_if_fail (CLAY_IS_DRAWABLE (objects->data), NULL);

      ret = clay_drawable_transform_buffer_flip (CLAY_DRAWABLE (objects->data),
                                                 context,
                                                 orig_buffer,
                                                 orig_offset_x,
                                                 orig_offset_y,
                                                 flip_type, axis,
                                                 clip_result,
                                                 buffer_profile,
                                                 new_offset_x,
                                                 new_offset_y);
    }
  else if (g_list_length (objects) == 1 && CLAY_IS_IMAGE (objects->data))
    {
      /*  this happens for images  */
      ClayTransformToolClass *tr_class = CLAY_TRANSFORM_TOOL_GET_CLASS (tr_tool);
      ClayProgress           *progress;

      progress = clay_progress_start (CLAY_PROGRESS (tr_tool), FALSE,
                                      "%s", tr_class->progress_text);

      clay_image_flip_full (CLAY_IMAGE (objects->data), context,
                            flip_type, axis, clip_result,
                            progress);

      if (progress)
        clay_progress_end (progress);
    }
  else
    {
      /*  this happens for entire drawables, paths and layer groups  */
      clay_image_item_list_flip (clay_item_get_image (objects->data),
                                 objects, context,
                                 flip_type, axis, clip_result);
    }

  return ret;
}

static ClayOrientationType
clay_flip_tool_get_flip_type (ClayFlipTool *flip)
{
  ClayFlipOptions *options = CLAY_FLIP_TOOL_GET_OPTIONS (flip);

  if (flip->guide)
    {
      switch (clay_guide_get_orientation (flip->guide))
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          return CLAY_ORIENTATION_VERTICAL;

        case CLAY_ORIENTATION_VERTICAL:
          return CLAY_ORIENTATION_HORIZONTAL;

        default:
          return clay_guide_get_orientation (flip->guide);
        }
    }
  else
    {
      return options->flip_type;
    }
}
