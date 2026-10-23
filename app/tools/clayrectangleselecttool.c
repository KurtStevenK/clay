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

#include "core/claychannel-select.h"
#include "core/claychannel.h"
#include "core/clayimage.h"
#include "core/claylayer-floating-selection.h"
#include "core/claypickable.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolrectangle.h"

#include "clayeditselectiontool.h"
#include "clayrectangleoptions.h"
#include "clayrectangleselecttool.h"
#include "clayrectangleselectoptions.h"
#include "claytoolcontrol.h"

#include "clay-intl.h"


struct _ClayRectangleSelectToolPrivate
{
  ClayChannelOps  operation;    /* remember for use when modifying   */
  gboolean        use_saved_op; /* use operation or get from options */

  gdouble         press_x;
  gdouble         press_y;

  ClayToolWidget *widget;
  ClayToolWidget *grab_widget;
  GList          *bindings;
};


static void     clay_rectangle_select_tool_control        (ClayTool              *tool,
                                                           ClayToolAction         action,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_button_press   (ClayTool              *tool,
                                                           const ClayCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           ClayButtonPressType    press_type,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_button_release (ClayTool              *tool,
                                                           const ClayCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           ClayButtonReleaseType  release_type,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_motion         (ClayTool              *tool,
                                                           const ClayCoords      *coords,
                                                           guint32                time,
                                                           GdkModifierType        state,
                                                           ClayDisplay           *display);
static gboolean clay_rectangle_select_tool_key_press      (ClayTool              *tool,
                                                           GdkEventKey           *kevent,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_oper_update    (ClayTool              *tool,
                                                           const ClayCoords      *coords,
                                                           GdkModifierType        state,
                                                           gboolean               proximity,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_cursor_update  (ClayTool              *tool,
                                                           const ClayCoords      *coords,
                                                           GdkModifierType        state,
                                                           ClayDisplay           *display);
static void     clay_rectangle_select_tool_options_notify (ClayTool              *tool,
                                                           ClayToolOptions       *options,
                                                           const GParamSpec      *pspec);

static gboolean clay_rectangle_select_tool_select         (ClayRectangleSelectTool *rect_tool,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);
static void     clay_rectangle_select_tool_real_select    (ClayRectangleSelectTool *rect_tool,
                                                           ClayChannelOps         operation,
                                                           gint                   x,
                                                           gint                   y,
                                                           gint                   w,
                                                           gint                   h);

static void     clay_rectangle_select_tool_rectangle_response
                                                          (ClayToolWidget          *widget,
                                                           gint                     response_id,
                                                           ClayRectangleSelectTool *rect_tool);
static void     clay_rectangle_select_tool_rectangle_change_complete
                                                          (ClayToolWidget          *widget,
                                                           ClayRectangleSelectTool *rect_tool);

static void     clay_rectangle_select_tool_start          (ClayRectangleSelectTool *rect_tool,
                                                           ClayDisplay             *display);
static void     clay_rectangle_select_tool_commit         (ClayRectangleSelectTool *rect_tool);
static void     clay_rectangle_select_tool_halt           (ClayRectangleSelectTool *rect_tool);

static ClayChannelOps
                clay_rectangle_select_tool_get_operation  (ClayRectangleSelectTool *rect_tool);
static void     clay_rectangle_select_tool_update_option_defaults
                                                          (ClayRectangleSelectTool *rect_tool,
                                                           gboolean                 ignore_pending);
static void     clay_rectangle_select_tool_update         (ClayRectangleSelectTool *rect_tool);
static void     clay_rectangle_select_tool_auto_shrink    (ClayRectangleSelectTool *rect_tool);


G_DEFINE_TYPE_WITH_PRIVATE (ClayRectangleSelectTool, clay_rectangle_select_tool,
                            CLAY_TYPE_SELECTION_TOOL)

#define parent_class clay_rectangle_select_tool_parent_class


void
clay_rectangle_select_tool_register (ClayToolRegisterCallback  callback,
                                     gpointer                  data)
{
  (* callback) (CLAY_TYPE_RECTANGLE_SELECT_TOOL,
                CLAY_TYPE_RECTANGLE_SELECT_OPTIONS,
                clay_rectangle_select_options_gui,
                0,
                "clay-rect-select-tool",
                _("Rectangle Select"),
                _("Rectangle Select Tool: Select a rectangular region"),
                N_("_Rectangle Select"), "R",
                NULL, CLAY_HELP_TOOL_RECT_SELECT,
                CLAY_ICON_TOOL_RECT_SELECT,
                data);
}

static void
clay_rectangle_select_tool_class_init (ClayRectangleSelectToolClass *klass)
{
  ClayToolClass *tool_class = CLAY_TOOL_CLASS (klass);

  tool_class->control        = clay_rectangle_select_tool_control;
  tool_class->button_press   = clay_rectangle_select_tool_button_press;
  tool_class->button_release = clay_rectangle_select_tool_button_release;
  tool_class->motion         = clay_rectangle_select_tool_motion;
  tool_class->key_press      = clay_rectangle_select_tool_key_press;
  tool_class->oper_update    = clay_rectangle_select_tool_oper_update;
  tool_class->cursor_update  = clay_rectangle_select_tool_cursor_update;
  tool_class->options_notify = clay_rectangle_select_tool_options_notify;

  klass->select              = clay_rectangle_select_tool_real_select;
}

static void
clay_rectangle_select_tool_init (ClayRectangleSelectTool *rect_tool)
{
  ClayTool *tool = CLAY_TOOL (rect_tool);

  rect_tool->private =
    clay_rectangle_select_tool_get_instance_private (rect_tool);

  clay_tool_control_set_wants_click      (tool->control, TRUE);
  clay_tool_control_set_active_modifiers (tool->control,
                                          CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision        (tool->control,
                                          CLAY_CURSOR_PRECISION_PIXEL_BORDER);
  clay_tool_control_set_tool_cursor      (tool->control,
                                          CLAY_TOOL_CURSOR_RECT_SELECT);
  clay_tool_control_set_preserve         (tool->control, FALSE);
  clay_tool_control_set_dirty_mask       (tool->control,
                                          CLAY_DIRTY_IMAGE_SIZE |
                                          CLAY_DIRTY_SELECTION);
  clay_tool_control_set_dirty_action     (tool->control,
                                          CLAY_TOOL_ACTION_COMMIT);
}

static void
clay_rectangle_select_tool_control (ClayTool       *tool,
                                    ClayToolAction  action,
                                    ClayDisplay    *display)
{
  ClayRectangleSelectTool *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_rectangle_select_tool_halt (rect_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_rectangle_select_tool_commit (rect_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_rectangle_select_tool_button_press (ClayTool            *tool,
                                         const ClayCoords    *coords,
                                         guint32              time,
                                         GdkModifierType      state,
                                         ClayButtonPressType  press_type,
                                         ClayDisplay         *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *private   = rect_tool->private;
  ClayRectangleFunction           function;

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);

  if (clay_selection_tool_start_edit (CLAY_SELECTION_TOOL (tool),
                                      display, coords))
    {
      /* In some cases we want to finish the rectangle select tool
       * and hand over responsibility to the selection tool
       */

      gboolean zero_rect = FALSE;

      if (private->widget)
        {
          gdouble x1, y1, x2, y2;

          clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (private->widget),
                                               &x1, &y1, &x2, &y2);
          if (x1 == x2 && y1 == y2)
            zero_rect = TRUE;
        }

      /* Don't commit a zero-size rectangle, it would look like a
       * click to commit() and that could anchor the floating
       * selection or do other evil things. Instead, simply cancel a
       * zero-size rectangle. See bug #796073.
       */
      if (zero_rect)
        clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
      else
        clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);

      clay_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
      return;
    }

  if (! tool->display)
    {
      clay_rectangle_select_tool_start (rect_tool, display);

      clay_tool_widget_hover (private->widget, coords, state, TRUE);

      /* HACK: force CREATING on a newly created rectangle; otherwise,
       * the above binding of properties would cause the rectangle to
       * start with the size from tool options.
       */
      clay_tool_rectangle_set_function (CLAY_TOOL_RECTANGLE (private->widget),
                                        CLAY_TOOL_RECTANGLE_CREATING);
    }

  /* if the shift or ctrl keys are down, we don't want to adjust, we
   * want to create a new rectangle, regardless of pointer loc
   */
  if (state & (clay_get_extend_selection_mask () |
               clay_get_modify_selection_mask ()))
    {
      clay_tool_rectangle_set_function (CLAY_TOOL_RECTANGLE (private->widget),
                                        CLAY_TOOL_RECTANGLE_CREATING);
    }

  if (clay_tool_widget_button_press (private->widget, coords, time, state,
                                     press_type))
    {
      private->grab_widget = private->widget;
    }

  private->press_x = coords->x;
  private->press_y = coords->y;

  /* if we have an existing rectangle in the current display, then
   * we have already "executed", and need to undo at this point,
   * unless the user has done something in the meantime
   */
  function =
    clay_tool_rectangle_get_function (CLAY_TOOL_RECTANGLE (private->widget));

  if (function == CLAY_TOOL_RECTANGLE_CREATING)
    private->use_saved_op = FALSE;

  clay_selection_tool_start_change (
    CLAY_SELECTION_TOOL (tool),
    function == CLAY_TOOL_RECTANGLE_CREATING,
    clay_rectangle_select_tool_get_operation (rect_tool));

  clay_tool_control_activate (tool->control);
}

static void
clay_rectangle_select_tool_button_release (ClayTool              *tool,
                                           const ClayCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           ClayButtonReleaseType  release_type,
                                           ClayDisplay           *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *priv      = rect_tool->private;

  clay_tool_control_halt (tool->control);

  clay_selection_tool_end_change (CLAY_SELECTION_TOOL (tool),
                                  /* if the user has not moved the mouse,
                                   * cancel the change
                                   */
                                  release_type == CLAY_BUTTON_RELEASE_CLICK ||
                                  release_type == CLAY_BUTTON_RELEASE_CANCEL);

  clay_tool_pop_status (tool, display);

  if (priv->grab_widget)
    {
      clay_tool_widget_button_release (priv->grab_widget,
                                       coords, time, state, release_type);
      priv->grab_widget = NULL;
    }
}

static void
clay_rectangle_select_tool_motion (ClayTool         *tool,
                                   const ClayCoords *coords,
                                   guint32           time,
                                   GdkModifierType   state,
                                   ClayDisplay      *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->grab_widget)
    {
      clay_tool_widget_motion (priv->grab_widget, coords, time, state);
    }
}

static gboolean
clay_rectangle_select_tool_key_press (ClayTool    *tool,
                                      GdkEventKey *kevent,
                                      ClayDisplay *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      if (clay_tool_widget_key_press (priv->widget, kevent))
        return TRUE;
    }

  return clay_edit_selection_tool_key_press (tool, kevent, display);
}

static void
clay_rectangle_select_tool_oper_update (ClayTool         *tool,
                                        const ClayCoords *coords,
                                        GdkModifierType   state,
                                        gboolean          proximity,
                                        ClayDisplay      *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *priv      = rect_tool->private;

  if (priv->widget && display == tool->display)
    {
      clay_tool_widget_hover (priv->widget, coords, state, proximity);
    }

  CLAY_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
clay_rectangle_select_tool_cursor_update (ClayTool         *tool,
                                          const ClayCoords *coords,
                                          GdkModifierType   state,
                                          ClayDisplay      *display)
{
  ClayRectangleSelectTool        *rect_tool = CLAY_RECTANGLE_SELECT_TOOL (tool);
  ClayRectangleSelectToolPrivate *private   = rect_tool->private;
  ClayCursorType                  cursor    = CLAY_CURSOR_CROSSHAIR_SMALL;
  ClayCursorModifier              modifier  = CLAY_CURSOR_MODIFIER_NONE;

  if (private->widget && display == tool->display)
    {
      clay_tool_widget_get_cursor (private->widget, coords, state,
                                   &cursor, NULL, &modifier);
    }

  clay_tool_control_set_cursor          (tool->control, cursor);
  clay_tool_control_set_cursor_modifier (tool->control, modifier);

  /* override the previous if shift or ctrl are down */
  if (state & (clay_get_extend_selection_mask () |
               clay_get_modify_selection_mask ()))
    {
      clay_tool_control_set_cursor (tool->control,
                                    CLAY_CURSOR_CROSSHAIR_SMALL);
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_rectangle_select_tool_options_notify (ClayTool         *tool,
                                           ClayToolOptions  *options,
                                           const GParamSpec *pspec)
{
  if (! strcmp (pspec->name, "antialias")      ||
      ! strcmp (pspec->name, "feather")        ||
      ! strcmp (pspec->name, "feather-radius") ||
      ! strcmp (pspec->name, "round-corners")  ||
      ! strcmp (pspec->name, "corner-radius"))
    {
      clay_rectangle_select_tool_update (CLAY_RECTANGLE_SELECT_TOOL (tool));
    }

  CLAY_TOOL_CLASS (parent_class)->options_notify (tool, options, pspec);
}

static gboolean
clay_rectangle_select_tool_select (ClayRectangleSelectTool *rect_tool,
                                   gint                     x,
                                   gint                     y,
                                   gint                     w,
                                   gint                     h)
{
  ClayTool       *tool  = CLAY_TOOL (rect_tool);
  ClayImage      *image = clay_display_get_image (tool->display);
  gboolean        rectangle_exists;
  ClayChannelOps  operation;

  clay_tool_pop_status (tool, tool->display);

  rectangle_exists = (x <= clay_image_get_width  (image) &&
                      y <= clay_image_get_height (image) &&
                      x + w >= 0                         &&
                      y + h >= 0                         &&
                      w > 0                              &&
                      h > 0);

  operation = clay_rectangle_select_tool_get_operation (rect_tool);

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    {
      clay_selection_tool_start_change (CLAY_SELECTION_TOOL (rect_tool),
                                        FALSE,
                                        operation);

      CLAY_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->select (rect_tool,
                                                                operation,
                                                                x, y, w, h);

      clay_selection_tool_end_change (CLAY_SELECTION_TOOL (rect_tool),
                                      FALSE);
    }

  return rectangle_exists;
}

static void
clay_rectangle_select_tool_real_select (ClayRectangleSelectTool *rect_tool,
                                        ClayChannelOps           operation,
                                        gint                     x,
                                        gint                     y,
                                        gint                     w,
                                        gint                     h)
{
  ClayTool                   *tool    = CLAY_TOOL (rect_tool);
  ClaySelectionOptions       *options = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);
  ClayRectangleSelectOptions *rect_options;
  ClayChannel                *channel;

  rect_options = CLAY_RECTANGLE_SELECT_TOOL_GET_OPTIONS (tool);

  channel = clay_image_get_mask (clay_display_get_image (tool->display));

  if (rect_options->round_corners)
    {
      /* To prevent elliptification of the rectangle,
       * we must cap the corner radius.
       */
      gdouble max    = MIN (w / 2.0, h / 2.0);
      gdouble radius = MIN (rect_options->corner_radius, max);

      clay_channel_select_round_rect (channel,
                                      x, y, w, h,
                                      radius, radius,
                                      operation,
                                      options->antialias,
                                      options->feather,
                                      options->feather_radius,
                                      options->feather_radius,
                                      TRUE);
    }
  else
    {
      clay_channel_select_rectangle (channel,
                                     x, y, w, h,
                                     operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius,
                                     TRUE);
    }
}

static void
clay_rectangle_select_tool_rectangle_response (ClayToolWidget          *widget,
                                               gint                     response_id,
                                               ClayRectangleSelectTool *rect_tool)
{
  ClayTool *tool = CLAY_TOOL (rect_tool);

  switch (response_id)
    {
    case CLAY_TOOL_WIDGET_RESPONSE_CONFIRM:
      {
        gdouble x1, y1, x2, y2;

        clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (widget),
                                             &x1, &y1, &x2, &y2);
        if (x1 == x2 && y1 == y2)
          {
            /*  if there are no extents, we got here because of a
             *  click, call commit() directly because we might want to
             *  reconfigure the rectangle and continue, instead of
             *  HALTing it like calling COMMIT would do
             */
            clay_rectangle_select_tool_commit (rect_tool);

            clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (widget),
                                                 &x1, &y1, &x2, &y2);
            if (x1 == x2 && y1 == y2)
              {
                /*  if there still is no rectangle after the
                 *  tool_commit(), the click was outside the selection
                 *  and we HALT to get rid of a zero-size tool widget.
                 */
                clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
              }
         }
        else
          {
            clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);
          }
      }
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_CANCEL:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_rectangle_select_tool_rectangle_change_complete (ClayToolWidget          *widget,
                                                      ClayRectangleSelectTool *rect_tool)
{
  clay_rectangle_select_tool_update (rect_tool);
}

static void
clay_rectangle_select_tool_start (ClayRectangleSelectTool *rect_tool,
                                  ClayDisplay             *display)
{
  static const gchar *properties[] =
  {
    "highlight",
    "highlight-opacity",
    "guide",
    "round-corners",
    "corner-radius",
    "x",
    "y",
    "width",
    "height",
    "fixed-rule-active",
    "fixed-rule",
    "desired-fixed-width",
    "desired-fixed-height",
    "desired-fixed-size-width",
    "desired-fixed-size-height",
    "aspect-numerator",
    "aspect-denominator",
    "fixed-center"
  };

  ClayTool                       *tool    = CLAY_TOOL (rect_tool);
  ClayRectangleSelectToolPrivate *private = rect_tool->private;
  ClayDisplayShell               *shell   = clay_display_get_shell (display);
  ClayRectangleSelectOptions     *options;
  ClayToolWidget                 *widget;
  gboolean                        draw_ellipse;
  gint                            i;

  options = CLAY_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  tool->display = display;

  private->widget = widget = clay_tool_rectangle_new (shell);

  draw_ellipse = CLAY_RECTANGLE_SELECT_TOOL_GET_CLASS (rect_tool)->draw_ellipse;

  g_object_set (widget,
                "draw-ellipse", draw_ellipse,
                "status-title", draw_ellipse ? _("Ellipse: ") : _("Rectangle: "),
                NULL);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), widget);

  for (i = 0; i < G_N_ELEMENTS (properties); i++)
    {
      GBinding *binding =
        g_object_bind_property (G_OBJECT (options), properties[i],
                                G_OBJECT (widget),  properties[i],
                                G_BINDING_SYNC_CREATE |
                                G_BINDING_BIDIRECTIONAL);

      private->bindings = g_list_prepend (private->bindings, binding);
    }

  clay_rectangle_options_connect (CLAY_RECTANGLE_OPTIONS (options),
                                  clay_display_get_image (shell->display),
                                  G_CALLBACK (clay_rectangle_select_tool_auto_shrink),
                                  rect_tool);

  g_signal_connect (widget, "response",
                    G_CALLBACK (clay_rectangle_select_tool_rectangle_response),
                    rect_tool);
  g_signal_connect (widget, "change-complete",
                    G_CALLBACK (clay_rectangle_select_tool_rectangle_change_complete),
                    rect_tool);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

/* This function is called if the user clicks and releases the left
 * button without moving it.  There are the things we might want
 * to do here:
 * 1) If there is an existing rectangle and we are inside it, we
 *    convert it into a selection.
 * 2) If there is an existing rectangle and we are outside it, we
 *    clear it.
 * 3) If there is no rectangle and there is a floating selection,
 *    we anchor it.
 * 4) If there is no rectangle and we are inside the selection, we
 *    create a rectangle from the selection bounds.
 * 5) If there is no rectangle and we are outside the selection,
 *    we clear the selection.
 */
static void
clay_rectangle_select_tool_commit (ClayRectangleSelectTool *rect_tool)
{
  ClayTool                       *tool = CLAY_TOOL (rect_tool);
  ClayRectangleSelectToolPrivate *priv = rect_tool->private;

  if (priv->widget)
    {
      gdouble x1, y1, x2, y2;
      gint    w, h;

      clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (priv->widget),
                                           &x1, &y1, &x2, &y2);
      w = x2 - x1;
      h = y2 - y1;

      if (w == 0 && h == 0)
        {
          ClayImage   *image     = clay_display_get_image (tool->display);
          ClayChannel *selection = clay_image_get_mask (image);
          gint         press_x;
          gint         press_y;

          if (clay_image_get_floating_selection (image))
            {
              floating_sel_anchor (clay_image_get_floating_selection (image));
              clay_image_flush (image);

              return;
            }

          press_x = ROUND (priv->press_x);
          press_y = ROUND (priv->press_y);

          /*  if the click was inside the marching ants  */
          if (clay_pickable_get_opacity_at (CLAY_PICKABLE (selection),
                                            press_x, press_y) > 0.5)
            {
              gint x, y, w, h;

              if (clay_item_bounds (CLAY_ITEM (selection), &x, &y, &w, &h))
                {
                  g_object_set (priv->widget,
                                "x1", (gdouble) x,
                                "y1", (gdouble) y,
                                "x2", (gdouble) (x + w),
                                "y2", (gdouble) (y + h),
                                NULL);
                }

              clay_rectangle_select_tool_update (rect_tool);
            }
          else
            {
              ClayChannelOps  operation;

              /* prevent this change from halting the tool */
              clay_tool_control_push_preserve (tool->control, TRUE);

              /* We can conceptually think of a click outside of the
               * selection as adding a 0px selection. Behave intuitively
               * for the current selection mode
               */
              operation = clay_rectangle_select_tool_get_operation (rect_tool);

              switch (operation)
                {
                case CLAY_CHANNEL_OP_REPLACE:
                case CLAY_CHANNEL_OP_INTERSECT:
                  clay_channel_clear (selection, NULL, TRUE);
                  clay_image_flush (image);
                  break;

                case CLAY_CHANNEL_OP_ADD:
                case CLAY_CHANNEL_OP_SUBTRACT:
                default:
                  /* Do nothing */
                  break;
                }

              clay_tool_control_pop_preserve (tool->control);
            }
        }

      clay_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
    }
}

static void
clay_rectangle_select_tool_halt (ClayRectangleSelectTool *rect_tool)
{
  ClayTool                       *tool = CLAY_TOOL (rect_tool);
  ClayRectangleSelectToolPrivate *priv = rect_tool->private;
  ClayRectangleSelectOptions     *options;

  options = CLAY_RECTANGLE_SELECT_TOOL_GET_OPTIONS (rect_tool);

  if (tool->display)
    {
      ClayDisplayShell *shell = clay_display_get_shell (tool->display);

      clay_display_shell_set_highlight (shell, NULL, 0.0);

      clay_rectangle_options_disconnect (CLAY_RECTANGLE_OPTIONS (options),
                                         G_CALLBACK (clay_rectangle_select_tool_auto_shrink),
                                         rect_tool);
    }

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  /*  disconnect bindings manually so they are really gone *now*, we
   *  might be in the middle of a signal emission that keeps the
   *  widget and its bindings alive.
   */
  g_list_free_full (priv->bindings, (GDestroyNotify) g_object_unref);
  priv->bindings = NULL;

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&priv->widget);

  tool->display = NULL;

  clay_rectangle_select_tool_update_option_defaults (rect_tool, TRUE);
}

static ClayChannelOps
clay_rectangle_select_tool_get_operation (ClayRectangleSelectTool *rect_tool)
{
  ClayRectangleSelectToolPrivate *priv = rect_tool->private;
  ClaySelectionOptions           *options;

  options = CLAY_SELECTION_TOOL_GET_OPTIONS (rect_tool);

  if (priv->use_saved_op)
    return priv->operation;
  else
    return options->operation;
}

/**
 * clay_rectangle_select_tool_update_option_defaults:
 * @crop_tool:
 * @ignore_pending: %TRUE to ignore any pending crop rectangle.
 *
 * Sets the default Fixed: Aspect ratio and Fixed: Size option
 * properties.
 */
static void
clay_rectangle_select_tool_update_option_defaults (ClayRectangleSelectTool *rect_tool,
                                                   gboolean                 ignore_pending)
{
  ClayRectangleSelectToolPrivate *priv = rect_tool->private;
  ClayTool                       *tool = CLAY_TOOL (rect_tool);
  ClayRectangleOptions           *rect_options;

  rect_options = CLAY_RECTANGLE_OPTIONS (clay_tool_get_options (tool));

  if (priv->widget && ! ignore_pending)
    {
      /* There is a pending rectangle and we should not ignore it, so
       * set default Fixed: Size to the same as the current pending
       * rectangle width/height.
       */

      clay_tool_rectangle_pending_size_set (CLAY_TOOL_RECTANGLE (priv->widget),
                                            G_OBJECT (rect_options),
                                            "default-aspect-numerator",
                                            "default-aspect-denominator");

      g_object_set (G_OBJECT (rect_options),
                    "use-string-current", TRUE,
                    NULL);
    }
  else
    {
      g_object_set (G_OBJECT (rect_options),
                    "default-aspect-numerator",   1.0,
                    "default-aspect-denominator", 1.0,
                    NULL);

      g_object_set (G_OBJECT (rect_options),
                    "use-string-current", FALSE,
                    NULL);
    }
}

static void
clay_rectangle_select_tool_update (ClayRectangleSelectTool *rect_tool)
{
  ClayTool                       *tool = CLAY_TOOL (rect_tool);
  ClayRectangleSelectToolPrivate *priv = rect_tool->private;

  /* prevent change in selection from halting the tool */
  clay_tool_control_push_preserve (tool->control, TRUE);

  if (tool->display && ! clay_tool_control_is_active (tool->control))
    {
      gdouble x1, y1, x2, y2;

      clay_tool_rectangle_get_public_rect (CLAY_TOOL_RECTANGLE (priv->widget),
                                           &x1, &y1, &x2, &y2);

      clay_rectangle_select_tool_select (rect_tool,
                                         x1, y1, x2 - x1, y2 - y1);

      if (! priv->use_saved_op)
        {
          ClaySelectionOptions *options;

          options = CLAY_SELECTION_TOOL_GET_OPTIONS (tool);

          /* remember the operation now in case we modify the rectangle */
          priv->operation    = options->operation;
          priv->use_saved_op = TRUE;
        }
    }

  clay_tool_control_pop_preserve (tool->control);

  clay_rectangle_select_tool_update_option_defaults (rect_tool, FALSE);
}

static void
clay_rectangle_select_tool_auto_shrink (ClayRectangleSelectTool *rect_tool)
{
  ClayRectangleSelectToolPrivate *private = rect_tool->private;
  gboolean                        shrink_merged;

  g_object_get (clay_tool_get_options (CLAY_TOOL (rect_tool)),
                "shrink-merged", &shrink_merged,
                NULL);

  clay_tool_rectangle_auto_shrink (CLAY_TOOL_RECTANGLE (private->widget),
                                   shrink_merged);
}
