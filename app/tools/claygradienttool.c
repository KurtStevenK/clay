/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Major improvements for interactivity
 * Copyright (C) 2014 Michael Henning <drawoc@darkrefraction.com>
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

#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/clayguiconfig.h"

#include "gegl/clay-gegl-utils.h"

#include "operations/clay-operation-config.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claydrawable.h"
#include "core/claydrawable-filters.h"
#include "core/claydrawable-gradient.h"
#include "core/claydrawablefilter.h"
#include "core/clayerror.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/clayimage-undo-push.h"
#include "core/clayprogress.h"
#include "core/clayprojection.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claytoolline.h"

#include "claygradientoptions.h"
#include "claygradienttool.h"
#include "claygradienttool-editor.h"
#include "claytoolcontrol.h"
#include "claytools-utils.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   clay_gradient_tool_dispose             (GObject               *object);

static gboolean clay_gradient_tool_initialize        (ClayTool              *tool,
                                                      ClayDisplay           *display,
                                                      GError               **error);
static void   clay_gradient_tool_control             (ClayTool              *tool,
                                                      ClayToolAction         action,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_button_press        (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayButtonPressType    press_type,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_button_release      (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayButtonReleaseType  release_type,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_motion              (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      guint32                time,
                                                      GdkModifierType        state,
                                                      ClayDisplay           *display);
static gboolean clay_gradient_tool_key_press         (ClayTool              *tool,
                                                      GdkEventKey           *kevent,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_modifier_key        (ClayTool              *tool,
                                                      GdkModifierType        key,
                                                      gboolean               press,
                                                      GdkModifierType        state,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_cursor_update       (ClayTool              *tool,
                                                      const ClayCoords      *coords,
                                                      GdkModifierType        state,
                                                      ClayDisplay           *display);
static const gchar * clay_gradient_tool_can_undo     (ClayTool              *tool,
                                                      ClayDisplay           *display);
static const gchar * clay_gradient_tool_can_redo     (ClayTool              *tool,
                                                      ClayDisplay           *display);
static gboolean  clay_gradient_tool_undo             (ClayTool              *tool,
                                                      ClayDisplay           *display);
static gboolean  clay_gradient_tool_redo             (ClayTool              *tool,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_options_notify      (ClayTool              *tool,
                                                      ClayToolOptions       *options,
                                                      const GParamSpec      *pspec);

static void   clay_gradient_tool_start               (ClayGradientTool      *gradient_tool,
                                                      const ClayCoords      *coords,
                                                      ClayDisplay           *display);
static void   clay_gradient_tool_halt                (ClayGradientTool      *gradient_tool);
static void   clay_gradient_tool_commit              (ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_line_changed        (ClayToolWidget        *widget,
                                                      ClayGradientTool      *gradient_tool);
static void   clay_gradient_tool_line_response       (ClayToolWidget        *widget,
                                                      gint                   response_id,
                                                      ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_precalc_shapeburst  (ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_create_graph        (ClayGradientTool      *gradient_tool);
static void   clay_gradient_tool_update_graph        (ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_fg_bg_changed       (ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_gradient_dirty      (ClayGradientTool      *gradient_tool);
static void   clay_gradient_tool_set_gradient        (ClayGradientTool      *gradient_tool,
                                                      ClayGradient          *gradient);

static gboolean clay_gradient_tool_is_shapeburst     (ClayGradientTool      *gradient_tool);

static void   clay_gradient_tool_create_filter       (ClayGradientTool      *gradient_tool,
                                                      ClayDrawable          *drawable);
static ClayDrawableFilter * clay_gradient_tool_create_nde_filter
                                                     (ClayGradientTool      *gradient_tool,
                                                      GeglNode              *copy_node,
                                                      ClayDrawable          *drawable);
static void   clay_gradient_tool_copy_node           (GeglNode              *node,
                                                      GeglNode              *copy_to_node);
static void   clay_gradient_tool_filter_flush        (ClayDrawableFilter    *filter,
                                                      ClayTool              *tool);


G_DEFINE_TYPE (ClayGradientTool, clay_gradient_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_gradient_tool_parent_class


void
clay_gradient_tool_register (ClayToolRegisterCallback  callback,
                             gpointer                  data)
{
  (* callback) (CLAY_TYPE_GRADIENT_TOOL,
                CLAY_TYPE_GRADIENT_OPTIONS,
                clay_gradient_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND |
                CLAY_CONTEXT_PROP_MASK_OPACITY    |
                CLAY_CONTEXT_PROP_MASK_PAINT_MODE |
                CLAY_CONTEXT_PROP_MASK_GRADIENT,
                "clay-gradient-tool",
                _("Gradient"),
                _("Gradient Tool: Fill selected area with a color gradient"),
                N_("Gra_dient"), "G",
                NULL, CLAY_HELP_TOOL_GRADIENT,
                CLAY_ICON_TOOL_GRADIENT,
                data);
}

static void
clay_gradient_tool_class_init (ClayGradientToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->dispose      = clay_gradient_tool_dispose;

  tool_class->initialize     = clay_gradient_tool_initialize;
  tool_class->control        = clay_gradient_tool_control;
  tool_class->button_press   = clay_gradient_tool_button_press;
  tool_class->button_release = clay_gradient_tool_button_release;
  tool_class->motion         = clay_gradient_tool_motion;
  tool_class->key_press      = clay_gradient_tool_key_press;
  tool_class->modifier_key   = clay_gradient_tool_modifier_key;
  tool_class->cursor_update  = clay_gradient_tool_cursor_update;
  tool_class->can_undo       = clay_gradient_tool_can_undo;
  tool_class->can_redo       = clay_gradient_tool_can_redo;
  tool_class->undo           = clay_gradient_tool_undo;
  tool_class->redo           = clay_gradient_tool_redo;
  tool_class->options_notify = clay_gradient_tool_options_notify;
}

static void
clay_gradient_tool_init (ClayGradientTool *gradient_tool)
{
  ClayTool *tool = CLAY_TOOL (gradient_tool);

  clay_tool_control_set_scroll_lock        (tool->control, TRUE);
  clay_tool_control_set_preserve           (tool->control, FALSE);
  clay_tool_control_set_dirty_mask         (tool->control,
                                            CLAY_DIRTY_IMAGE           |
                                            CLAY_DIRTY_IMAGE_STRUCTURE |
                                            CLAY_DIRTY_DRAWABLE        |
                                            CLAY_DIRTY_ACTIVE_DRAWABLE);
  clay_tool_control_set_dirty_action       (tool->control,
                                            CLAY_TOOL_ACTION_COMMIT);
  clay_tool_control_set_wants_click        (tool->control, TRUE);
  clay_tool_control_set_wants_double_click (tool->control, TRUE);
  clay_tool_control_set_active_modifiers   (tool->control,
                                            CLAY_TOOL_ACTIVE_MODIFIERS_SEPARATE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_GRADIENT);
  clay_tool_control_set_action_opacity     (tool->control,
                                            "context-opacity-set");
  clay_tool_control_set_action_object_1    (tool->control,
                                            "context-gradient-select-set");

  gradient_tool->existing_filter = NULL;

  clay_draw_tool_set_default_status (CLAY_DRAW_TOOL (tool),
                                     _("Click-Drag to draw a gradient"));
}

static void
clay_gradient_tool_dispose (GObject *object)
{
  ClayGradientTool *gradient_tool = CLAY_GRADIENT_TOOL (object);

  clay_gradient_tool_set_gradient (gradient_tool, NULL);

  gradient_tool->existing_filter = NULL;
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static gboolean
clay_gradient_tool_initialize (ClayTool     *tool,
                               ClayDisplay  *display,
                               GError      **error)
{
  ClayImage           *image       = clay_display_get_image (display);
  ClayGradientOptions *options     = CLAY_GRADIENT_TOOL_GET_OPTIONS (tool);
  ClayGuiConfig       *config      = CLAY_GUI_CONFIG (display->clay->config);
  ClayItem            *locked_item = NULL;
  GList               *drawables;
  ClayDrawable        *drawable;

  if (! CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  drawables = clay_image_get_selected_drawables (image);
  if (g_list_length (drawables) != 1)
    {
      if (g_list_length (drawables) > 1)
        clay_tool_message_literal (tool, display,
                                   _("Cannot paint on multiple drawables. Select only one."));
      else
        clay_tool_message_literal (tool, display, _("No active drawables."));

      g_list_free (drawables);

      return FALSE;
    }

  /* Only a single drawable at this point. */
  drawable = drawables->data;
  g_list_free (drawables);

  if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot modify the pixels of layer groups."));
      return FALSE;
    }

  if (clay_item_is_content_locked (CLAY_ITEM (drawable), &locked_item))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("The selected layer's pixels are locked."));
      if (error)
        clay_tools_blink_lock_box (display->clay, locked_item);
      return FALSE;
    }

  if (! clay_item_is_visible (CLAY_ITEM (drawable)) &&
      ! config->edit_non_visible)
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("The selected item is not visible."));
      return FALSE;
    }

  if (! clay_context_get_gradient (CLAY_CONTEXT (options)))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("No gradient available for use with this tool."));
      return FALSE;
    }

  /* The Gradient Tool is only destructive if "Editable Gradient" is enabled */
  CLAY_TOOL_GET_CLASS (tool)->is_destructive = ! options->editable_gradient;

  return TRUE;
}

static void
clay_gradient_tool_control (ClayTool       *tool,
                            ClayToolAction  action,
                            ClayDisplay    *display)
{
  ClayGradientTool *gradient_tool = CLAY_GRADIENT_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_gradient_tool_halt (gradient_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_gradient_tool_commit (gradient_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_gradient_tool_button_press (ClayTool            *tool,
                                 const ClayCoords    *coords,
                                 guint32              time,
                                 GdkModifierType      state,
                                 ClayButtonPressType  press_type,
                                 ClayDisplay         *display)
{
  ClayGradientTool *gradient_tool = CLAY_GRADIENT_TOOL (tool);

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! gradient_tool->widget)
    {
      clay_gradient_tool_start (gradient_tool, coords, display);

      clay_tool_widget_hover (gradient_tool->widget, coords, state, TRUE);
    }

  /* call start_edit() before widget_button_press(), because we need to record
   * the undo state before widget_button_press() potentially changes it.  note
   * that if widget_button_press() return FALSE, nothing changes and no undo
   * step is created.
   */
  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    clay_gradient_tool_editor_start_edit (gradient_tool);

  if (clay_tool_widget_button_press (gradient_tool->widget, coords, time, state,
                                     press_type))
    {
      gradient_tool->grab_widget = gradient_tool->widget;
    }

  if (press_type == CLAY_BUTTON_PRESS_NORMAL)
    clay_tool_control_activate (tool->control);
}

static void
clay_gradient_tool_button_release (ClayTool              *tool,
                                   const ClayCoords      *coords,
                                   guint32                time,
                                   GdkModifierType        state,
                                   ClayButtonReleaseType  release_type,
                                   ClayDisplay           *display)
{
  ClayGradientTool    *gradient_tool = CLAY_GRADIENT_TOOL (tool);
  ClayGradientOptions *options       = CLAY_GRADIENT_TOOL_GET_OPTIONS (tool);

  clay_tool_pop_status (tool, display);

  clay_tool_control_halt (tool->control);

  if (gradient_tool->grab_widget)
    {
      clay_tool_widget_button_release (gradient_tool->grab_widget,
                                       coords, time, state, release_type);
      gradient_tool->grab_widget = NULL;

      if (options->instant && ! gradient_tool->existing_filter)
        {
          if (release_type == CLAY_BUTTON_RELEASE_CANCEL)
            clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, display);
          else
            clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, display);
        }
    }

  if (! options->instant)
    {
      clay_gradient_tool_editor_end_edit (gradient_tool,
                                          release_type ==
                                          CLAY_BUTTON_RELEASE_CANCEL);
    }
}

static void
clay_gradient_tool_motion (ClayTool         *tool,
                           const ClayCoords *coords,
                           guint32           time,
                           GdkModifierType   state,
                           ClayDisplay      *display)
{
  ClayGradientTool *gradient_tool = CLAY_GRADIENT_TOOL (tool);

  if (gradient_tool->grab_widget)
    {
      clay_tool_widget_motion (gradient_tool->grab_widget, coords, time, state);
    }
}

static gboolean
clay_gradient_tool_key_press (ClayTool    *tool,
                              GdkEventKey *kevent,
                              ClayDisplay *display)
{
  ClayGradientTool *gradient_tool = CLAY_GRADIENT_TOOL (tool);
  ClayDrawTool     *draw_tool     = CLAY_DRAW_TOOL (tool);
  gboolean          result;

  /* call start_edit() before widget_key_press(), because we need to record the
   * undo state before widget_key_press() potentially changes it.  note that if
   * widget_key_press() return FALSE, nothing changes and no undo step is
   * created.
   */
  if (display == draw_tool->display)
    clay_gradient_tool_editor_start_edit (gradient_tool);

  result = CLAY_TOOL_CLASS (parent_class)->key_press (tool, kevent, display);

  if (display == draw_tool->display)
    clay_gradient_tool_editor_end_edit (gradient_tool, FALSE);

  return result;
}

static void
clay_gradient_tool_modifier_key (ClayTool        *tool,
                                 GdkModifierType  key,
                                 gboolean         press,
                                 GdkModifierType  state,
                                 ClayDisplay     *display)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (tool);

  if (key == clay_get_extend_selection_mask ())
    {
      if (options->instant_toggle &&
          gtk_widget_get_sensitive (options->instant_toggle))
        {
          g_object_set (options,
                        "instant", ! options->instant,
                        NULL);
        }
    }
}

static void
clay_gradient_tool_cursor_update (ClayTool         *tool,
                                  const ClayCoords *coords,
                                  GdkModifierType   state,
                                  ClayDisplay      *display)
{
  ClayGuiConfig *config    = CLAY_GUI_CONFIG (display->clay->config);
  ClayImage     *image     = clay_display_get_image (display);
  GList         *drawables = clay_image_get_selected_drawables (image);

  if (g_list_length (drawables) != 1                      ||
      clay_viewable_get_children (drawables->data)        ||
      clay_item_is_content_locked (drawables->data, NULL) ||
      ! (clay_item_is_visible (drawables->data) ||
         config->edit_non_visible))
    {
      clay_tool_set_cursor (tool, display,
                            clay_tool_control_get_cursor (tool->control),
                            clay_tool_control_get_tool_cursor (tool->control),
                            CLAY_CURSOR_MODIFIER_BAD);
      g_list_free (drawables);
      return;
    }
  g_list_free (drawables);

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static const gchar *
clay_gradient_tool_can_undo (ClayTool    *tool,
                             ClayDisplay *display)
{
  return clay_gradient_tool_editor_can_undo (CLAY_GRADIENT_TOOL (tool));
}

static const gchar *
clay_gradient_tool_can_redo (ClayTool    *tool,
                             ClayDisplay *display)
{
  return clay_gradient_tool_editor_can_redo (CLAY_GRADIENT_TOOL (tool));
}

static gboolean
clay_gradient_tool_undo (ClayTool    *tool,
                         ClayDisplay *display)
{
  return clay_gradient_tool_editor_undo (CLAY_GRADIENT_TOOL (tool));
}

static gboolean
clay_gradient_tool_redo (ClayTool    *tool,
                         ClayDisplay *display)
{
  return clay_gradient_tool_editor_redo (CLAY_GRADIENT_TOOL (tool));
}

static void
clay_gradient_tool_options_notify (ClayTool         *tool,
                                   ClayToolOptions  *options,
                                   const GParamSpec *pspec)
{
  ClayContext         *context          = CLAY_CONTEXT (options);
  ClayGradientTool    *gradient_tool    = CLAY_GRADIENT_TOOL (tool);
  ClayGradientOptions *gradient_options = CLAY_GRADIENT_TOOL_GET_OPTIONS (tool);

  if (! strcmp (pspec->name, "gradient"))
    {
      clay_gradient_tool_set_gradient (gradient_tool, context->gradient);

      if (gradient_tool->filter)
        clay_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->render_node &&
           gegl_node_find_property (gradient_tool->render_node, pspec->name))
    {
      /* Sync any property changes on the config object that match the op */
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (options), pspec->name, &value);
      gegl_node_set_property (gradient_tool->render_node, pspec->name, &value);

      g_value_unset (&value);

      if (! strcmp (pspec->name, "gradient-type"))
        {
          ClayRepeatMode   gradient_repeat;
          ClayRepeatMode   node_repeat;
          ClayGradientType gradient_type;

          gradient_repeat = CLAY_PAINT_OPTIONS (options)->gradient_options->gradient_repeat;
          gradient_type   = CLAY_GRADIENT_OPTIONS (options)->gradient_type;
          gegl_node_get (gradient_tool->render_node,
                         "gradient-repeat", &node_repeat,
                         NULL);

          if (gradient_type >= CLAY_GRADIENT_SHAPEBURST_ANGULAR)
            {
              /* These gradient types are only meant to work with repeat
               * value of "none" so these are the only ones where we
               * don't keep the render node and the gradient options in
               * sync.
               * We could instead reset the "gradient-repeat" value on
               * ClayGradientOptions, but I assume one would want to revert
               * back to the last set value if changing back the
               * gradient type. So instead we just make the option
               * insensitive (both in GUI and in render).
               */
              if (node_repeat != CLAY_REPEAT_NONE)
                gegl_node_set (gradient_tool->render_node,
                               "gradient-repeat", CLAY_REPEAT_NONE,
                               NULL);
            }
          else if (node_repeat != gradient_repeat)
            {
              gegl_node_set (gradient_tool->render_node,
                             "gradient-repeat", gradient_repeat,
                             NULL);
            }

          if (clay_gradient_tool_is_shapeburst (gradient_tool))
            clay_gradient_tool_precalc_shapeburst (gradient_tool);

          clay_gradient_tool_update_graph (gradient_tool);
        }

      clay_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->render_node                    &&
           clay_gradient_tool_is_shapeburst (gradient_tool) &&
           g_strcmp0 (pspec->name, "distance-metric") == 0)
    {
      g_clear_object (&gradient_tool->dist_buffer);
      clay_gradient_tool_precalc_shapeburst (gradient_tool);
      clay_gradient_tool_update_graph (gradient_tool);
      clay_drawable_filter_apply (gradient_tool->filter, NULL);
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "opacity"))
    {
      clay_drawable_filter_set_opacity (gradient_tool->filter,
                                        clay_context_get_opacity (context));
    }
  else if (gradient_tool->filter &&
           ! strcmp (pspec->name, "paint-mode"))
    {
      clay_drawable_filter_set_mode (gradient_tool->filter,
                                     clay_context_get_paint_mode (context),
                                     CLAY_LAYER_COLOR_SPACE_AUTO,
                                     CLAY_LAYER_COLOR_SPACE_AUTO,
                                     clay_layer_mode_get_paint_composite_mode (
                                       clay_context_get_paint_mode (context)));
    }
  else if (! strcmp (pspec->name, "editable-gradient"))
    {
      if (gradient_tool->filter)
        {
          ClayContainer *filters = NULL;
          gint           count;

          filters = clay_drawable_get_filters (tool->drawables->data);
          count   = clay_container_get_n_children (filters);

          if (CLAY_GRADIENT_OPTIONS (options)->editable_gradient)
            clay_container_reorder (filters,
                                    CLAY_OBJECT (gradient_tool->filter), 0);
          else
            clay_container_reorder (filters,
                                    CLAY_OBJECT (gradient_tool->filter),
                                    count - 1);

          clay_drawable_update (tool->drawables->data, 0, 0, -1, -1);
          clay_image_flush (clay_item_get_image (tool->drawables->data));
        }

      CLAY_TOOL_GET_CLASS (tool)->is_destructive =
        ! gradient_options->editable_gradient;
    }

  clay_gradient_tool_editor_options_notify (gradient_tool, options, pspec);
}

static void
clay_gradient_tool_start (ClayGradientTool *gradient_tool,
                          const ClayCoords *coords,
                          ClayDisplay      *display)
{
  ClayTool            *tool      = CLAY_TOOL (gradient_tool);
  ClayDisplayShell    *shell     = clay_display_get_shell (display);
  ClayImage           *image     = clay_display_get_image (display);
  GList               *drawables = clay_image_get_selected_drawables (image);
  ClayGradientOptions *options   = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context   = CLAY_CONTEXT (options);
  ClayContainer       *filters;

  g_return_if_fail (g_list_length (drawables) == 1);

  if (options->instant_toggle)
    gtk_widget_set_sensitive (options->instant_toggle, FALSE);

  tool->display   = display;
  g_list_free (tool->drawables);
  tool->drawables = drawables;

  gradient_tool->start_x = coords->x;
  gradient_tool->start_y = coords->y;
  gradient_tool->end_x   = coords->x;
  gradient_tool->end_y   = coords->y;

  gradient_tool->widget = clay_tool_line_new (shell,
                                              gradient_tool->start_x,
                                              gradient_tool->start_y,
                                              gradient_tool->end_x,
                                              gradient_tool->end_y);

  g_object_set (gradient_tool->widget,
                "status-title", _("Gradient: "),
                NULL);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), gradient_tool->widget);

  g_signal_connect (gradient_tool->widget, "changed",
                    G_CALLBACK (clay_gradient_tool_line_changed),
                    gradient_tool);
  g_signal_connect (gradient_tool->widget, "response",
                    G_CALLBACK (clay_gradient_tool_line_response),
                    gradient_tool);

  g_signal_connect_swapped (context, "background-changed",
                            G_CALLBACK (clay_gradient_tool_fg_bg_changed),
                            gradient_tool);
  g_signal_connect_swapped (context, "foreground-changed",
                            G_CALLBACK (clay_gradient_tool_fg_bg_changed),
                            gradient_tool);

  clay_gradient_tool_create_filter (gradient_tool, tool->drawables->data);

  /* Initially sync all of the properties */
  clay_operation_config_sync_node (G_OBJECT (options),
                                   gradient_tool->render_node);

  /* We don't allow repeat values for some shapes. */
  if (options->gradient_type >= CLAY_GRADIENT_SHAPEBURST_ANGULAR)
    gegl_node_set (gradient_tool->render_node,
                   "gradient-repeat", CLAY_REPEAT_NONE,
                   NULL);

  /* Connect signal handlers for the gradient */
  clay_gradient_tool_set_gradient (gradient_tool, context->gradient);

  if (clay_gradient_tool_is_shapeburst (gradient_tool))
    clay_gradient_tool_precalc_shapeburst (gradient_tool);

  clay_draw_tool_start (CLAY_DRAW_TOOL (gradient_tool), display);

  clay_gradient_tool_editor_start (gradient_tool);

  clay_drawable_filter_apply (gradient_tool->filter, NULL);

  /* If destructive, move this operation below any non-destructive filters that
   * may be active, so that it's directly affect the raw pixels. */
  if (! options->editable_gradient)
    {
      filters =
        clay_drawable_get_filters (clay_drawable_filter_get_drawable (gradient_tool->filter));

      if (clay_container_have (filters, CLAY_OBJECT (gradient_tool->filter)))
        {
          gint end_index = clay_container_get_n_children (filters) - 1;
          gint index     = clay_container_get_child_index (filters,
                                                           CLAY_OBJECT (gradient_tool->filter));

          if (end_index > 0 && index != end_index)
            clay_container_reorder (filters, CLAY_OBJECT (gradient_tool->filter),
                                    end_index);
        }
    }
}

static void
clay_gradient_tool_halt (ClayGradientTool *gradient_tool)
{
  ClayTool            *tool    = CLAY_TOOL (gradient_tool);
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context = CLAY_CONTEXT (options);

  clay_gradient_tool_editor_halt (gradient_tool);

  if (gradient_tool->graph)
    {
      g_clear_object (&gradient_tool->graph);
      gradient_tool->render_node    = NULL;
#if 0
      gradient_tool->subtract_node  = NULL;
      gradient_tool->divide_node    = NULL;
#endif
      gradient_tool->dist_node      = NULL;
    }

  g_clear_object (&gradient_tool->dist_buffer);

  if (gradient_tool->filter)
    {
      clay_tool_control_push_preserve (tool->control, TRUE);

      clay_drawable_filter_abort (gradient_tool->filter);
      g_object_unref (gradient_tool->filter);
      gradient_tool->filter = NULL;

      clay_tool_control_pop_preserve (tool->control);

      clay_image_flush (clay_display_get_image (tool->display));
    }

  clay_gradient_tool_set_tentative_gradient (gradient_tool, NULL);

  g_signal_handlers_disconnect_by_func (context,
                                        G_CALLBACK (clay_gradient_tool_fg_bg_changed),
                                        gradient_tool);

  if (tool->display)
    clay_tool_pop_status (tool, tool->display);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (gradient_tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (gradient_tool));

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&gradient_tool->widget);

  tool->display   = NULL;
  g_list_free (tool->drawables);
  tool->drawables = NULL;

  if (options->instant_toggle && ! gradient_tool->existing_filter)
    gtk_widget_set_sensitive (options->instant_toggle, TRUE);
}

static void
clay_gradient_tool_commit (ClayGradientTool *gradient_tool)
{
  ClayTool            *tool    = CLAY_TOOL (gradient_tool);
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  if (gradient_tool->filter)
    {
      /* halt the editor before committing the filter so that the image-flush
       * idle source is removed, to avoid flushing the image, and hence
       * restarting the projection rendering, while applying the filter.
       */
      clay_gradient_tool_editor_halt (gradient_tool);

      clay_tool_control_push_preserve (tool->control, TRUE);

      if (gradient_tool->existing_filter != NULL)
        {
          GeglNode *existing_node;

          existing_node =
            clay_drawable_filter_get_operation (gradient_tool->existing_filter);

          clay_image_undo_push_filter_modified (clay_display_get_image (tool->display),
                                                _("Edit filter"),
                                                tool->drawables->data,
                                                gradient_tool->existing_filter);

          clay_gradient_tool_copy_node (gradient_tool->render_node,
                                        existing_node);

          clay_filter_set_active (CLAY_FILTER (gradient_tool->existing_filter),
                                  TRUE);
          clay_drawable_update (tool->drawables->data, 0, 0, -1, -1);

          gradient_tool->existing_filter = NULL;
          gtk_widget_set_sensitive (options->instant_toggle, TRUE);
          gtk_widget_set_sensitive (options->editable_gradient_toggle, TRUE);
        }
      else if (options->editable_gradient)
        {
          ClayDrawableFilter *filter = NULL;

          filter = clay_gradient_tool_create_nde_filter (gradient_tool,
                                                         gradient_tool->render_node,
                                                         tool->drawables->data);

          clay_drawable_filter_commit (filter, TRUE, CLAY_PROGRESS (tool),
                                       FALSE);

          clay_image_undo_push_filter_add (clay_display_get_image (tool->display),
                                           _("Gradient"),
                                           tool->drawables->data,
                                           filter);
          g_object_unref (filter);
        }

      if (! options->editable_gradient &&
          gradient_tool->existing_filter == NULL)
        clay_drawable_filter_commit (gradient_tool->filter, FALSE,
                                     CLAY_PROGRESS (tool), FALSE);
      else
        clay_drawable_filter_abort (gradient_tool->filter);

      g_clear_object (&gradient_tool->filter);

      clay_tool_control_pop_preserve (tool->control);

      clay_image_flush (clay_display_get_image (tool->display));
    }
}

static void
clay_gradient_tool_line_changed (ClayToolWidget   *widget,
                                 ClayGradientTool *gradient_tool)
{
  gdouble  start_x;
  gdouble  start_y;
  gdouble  end_x;
  gdouble  end_y;
  gboolean update = FALSE;

  g_object_get (widget,
                "x1", &start_x,
                "y1", &start_y,
                "x2", &end_x,
                "y2", &end_y,
                NULL);

  if (start_x != gradient_tool->start_x ||
      start_y != gradient_tool->start_y ||
      end_x   != gradient_tool->end_x   ||
      end_y   != gradient_tool->end_y)
    {
      gradient_tool->start_x = start_x;
      gradient_tool->start_y = start_y;
      gradient_tool->end_x   = end_x;
      gradient_tool->end_y   = end_y;

      update = TRUE;
    }

  if (clay_gradient_tool_editor_line_changed (gradient_tool))
    update = TRUE;

  if (update)
    {
      clay_gradient_tool_update_graph (gradient_tool);
      clay_drawable_filter_apply (gradient_tool->filter, NULL);
    }
}

static void
clay_gradient_tool_line_response (ClayToolWidget *widget,
                                  gint            response_id,
                                  ClayGradientTool  *gradient_tool)
{
  ClayTool *tool = CLAY_TOOL (gradient_tool);

  switch (response_id)
    {
    case CLAY_TOOL_WIDGET_RESPONSE_CONFIRM:
      clay_tool_control (tool, CLAY_TOOL_ACTION_COMMIT, tool->display);
      break;

    case CLAY_TOOL_WIDGET_RESPONSE_CANCEL:
      clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);
      break;
    }
}

static void
clay_gradient_tool_precalc_shapeburst (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayTool            *tool    = CLAY_TOOL (gradient_tool);
  gint                 x, y, width, height;

  if (gradient_tool->dist_buffer || ! tool->drawables)
    return;

  g_return_if_fail (g_list_length (tool->drawables) == 1);

  if (! clay_item_mask_intersect (CLAY_ITEM (tool->drawables->data),
                                  &x, &y, &width, &height))
    return;

  gradient_tool->dist_buffer =
    clay_drawable_gradient_shapeburst_distmap (tool->drawables->data,
                                               options->distance_metric,
                                               GEGL_RECTANGLE (x, y, width, height),
                                               CLAY_PROGRESS (gradient_tool));

  if (gradient_tool->dist_node)
    gegl_node_set (gradient_tool->dist_node,
                   "buffer", gradient_tool->dist_buffer,
                   NULL);

  clay_progress_end (CLAY_PROGRESS (gradient_tool));
}


/* gegl graph stuff */

static void
clay_gradient_tool_create_graph (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context = CLAY_CONTEXT (options);
  GeglNode            *output;

  /* render_node is not supposed to be recreated */
  g_return_if_fail (gradient_tool->graph == NULL);

  gradient_tool->graph = gegl_node_new ();

  gradient_tool->dist_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:buffer-source",
                         "buffer",    gradient_tool->dist_buffer,
                         NULL);

#if 0
  gradient_tool->subtract_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:subtract",
                         NULL);

  gradient_tool->divide_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "gegl:divide",
                         NULL);
#endif

  gradient_tool->render_node =
    gegl_node_new_child (gradient_tool->graph,
                         "operation", "clay:gradient",
                         "context", context,
                         NULL);

  output = gegl_node_get_output_proxy (gradient_tool->graph, "output");

  gegl_node_link_many (gradient_tool->dist_node,
#if 0
                       gradient_tool->subtract_node,
                       gradient_tool->divide_node,
#endif
                       gradient_tool->render_node,
                       output,
                       NULL);

  clay_gegl_node_set_underlying_operation (gradient_tool->graph,
                                           gradient_tool->render_node);

  clay_gradient_tool_update_graph (gradient_tool);
}

static void
clay_gradient_tool_update_graph (ClayGradientTool *gradient_tool)
{
  ClayTool            *tool    = CLAY_TOOL (gradient_tool);
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  gint                 off_x, off_y;

  clay_item_get_offset (CLAY_ITEM (tool->drawables->data), &off_x, &off_y);

#if 0
  if (clay_gradient_tool_is_shapeburst (gradient_tool))
    {
      gfloat start, end;

      gegl_buffer_get (gradient_tool->dist_buffer,
                       GEGL_RECTANGLE (gradient_tool->start_x - off_x,
                                       gradient_tool->start_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &start,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_get (gradient_tool->dist_buffer,
                       GEGL_RECTANGLE (gradient_tool->end_x - off_x,
                                       gradient_tool->end_y - off_y,
                                       1, 1),
                       1.0, babl_format("Y float"), &end,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (start != end)
        {
          gegl_node_set (gradient_tool->subtract_node,
                         "value", (gdouble) start,
                         NULL);
          gegl_node_set (gradient_tool->divide_node,
                         "value", (gdouble) (end - start),
                         NULL);
        }
    }
  else
#endif
    {
      GeglRectangle roi;
      gdouble       start_x, start_y;
      gdouble       end_x,   end_y;

      clay_item_mask_intersect (CLAY_ITEM (tool->drawables->data),
                                &roi.x, &roi.y, &roi.width, &roi.height);

      start_x = gradient_tool->start_x - off_x;
      start_y = gradient_tool->start_y - off_y;
      end_x   = gradient_tool->end_x   - off_x;
      end_y   = gradient_tool->end_y   - off_y;

      clay_drawable_gradient_adjust_coords (tool->drawables->data,
                                            options->gradient_type,
                                            &roi,
                                            &start_x, &start_y, &end_x, &end_y);

      gegl_node_set (gradient_tool->render_node,
                     "start-x", start_x,
                     "start-y", start_y,
                     "end-x",   end_x,
                     "end-y",   end_y,
                     NULL);
    }
}

static void
clay_gradient_tool_fg_bg_changed (ClayGradientTool *gradient_tool)
{
  if (! gradient_tool->filter || ! gradient_tool->gradient)
    return;

  if (clay_gradient_has_fg_bg_segments (gradient_tool->gradient))
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (gradient_tool->render_node,
                     "gradient", gradient_tool->gradient,
                     NULL);

      /* Update the filter */
      clay_drawable_filter_apply (gradient_tool->filter, NULL);

      clay_gradient_tool_editor_fg_bg_changed (gradient_tool);
    }
}

static void
clay_gradient_tool_gradient_dirty (ClayGradientTool *gradient_tool)
{
  if (! gradient_tool->filter)
    return;

  if (! gradient_tool->tentative_gradient)
    {
      /* Set a property on the node. Otherwise it will cache and refuse to update */
      gegl_node_set (gradient_tool->render_node,
                     "gradient", gradient_tool->gradient,
                     NULL);

      /* Update the filter */
      clay_drawable_filter_apply (gradient_tool->filter, NULL);
    }

  clay_gradient_tool_editor_gradient_dirty (gradient_tool);
}

static void
clay_gradient_tool_set_gradient (ClayGradientTool *gradient_tool,
                                 ClayGradient     *gradient)
{
  if (gradient_tool->gradient)
    g_signal_handlers_disconnect_by_func (gradient_tool->gradient,
                                          G_CALLBACK (clay_gradient_tool_gradient_dirty),
                                          gradient_tool);


  g_set_object (&gradient_tool->gradient, gradient);

  if (gradient_tool->gradient)
    {
      g_signal_connect_swapped (gradient_tool->gradient, "dirty",
                                G_CALLBACK (clay_gradient_tool_gradient_dirty),
                                gradient_tool);

      if (gradient_tool->render_node)
        gegl_node_set (gradient_tool->render_node,
                       "gradient", gradient_tool->gradient,
                       NULL);
    }

  clay_gradient_tool_editor_gradient_changed (gradient_tool);
}

static gboolean
clay_gradient_tool_is_shapeburst (ClayGradientTool *gradient_tool)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);

  return options->gradient_type >= CLAY_GRADIENT_SHAPEBURST_ANGULAR &&
         options->gradient_type <= CLAY_GRADIENT_SHAPEBURST_DIMPLED;
}


/* image map stuff */

static void
clay_gradient_tool_create_filter (ClayGradientTool *gradient_tool,
                                  ClayDrawable     *drawable)
{
  ClayGradientOptions *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context = CLAY_CONTEXT (options);

  if (! gradient_tool->graph)
    clay_gradient_tool_create_graph (gradient_tool);

  gradient_tool->filter = clay_drawable_filter_new (drawable,
                                                    C_("undo-type", "Gradient"),
                                                    gradient_tool->graph,
                                                    CLAY_ICON_TOOL_GRADIENT);
  clay_drawable_filter_set_temporary (gradient_tool->filter, TRUE);

  clay_drawable_filter_set_region (gradient_tool->filter,
                                   CLAY_FILTER_REGION_DRAWABLE);
  clay_drawable_filter_set_opacity (gradient_tool->filter,
                                    clay_context_get_opacity (context));
  clay_drawable_filter_set_mode (gradient_tool->filter,
                                 clay_context_get_paint_mode (context),
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 clay_layer_mode_get_paint_composite_mode (
                                   clay_context_get_paint_mode (context)));

  g_signal_connect (gradient_tool->filter, "flush",
                    G_CALLBACK (clay_gradient_tool_filter_flush),
                    gradient_tool);
}

static ClayDrawableFilter *
clay_gradient_tool_create_nde_filter (ClayGradientTool *gradient_tool,
                                      GeglNode         *copy_node,
                                      ClayDrawable     *drawable)
{
  ClayImage           *image;
  ClayChannel         *mask;
  ClayDrawableFilter  *filter;
  ClayGradientOptions *options  = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext         *context  = CLAY_CONTEXT (options);
  GeglNode            *nde_node = gegl_node_new ();
  const gchar         *op_name  = "clay:gradient";

  image = clay_item_get_image (CLAY_ITEM (drawable));
  mask  = clay_image_get_mask (image);

  /* Use image context rather than tool options context so this
   * persists after tool is changed out */
  gegl_node_set (nde_node,
                 "operation", op_name,
                 "context",   clay_get_user_context (image->clay),
                 NULL);

  clay_gradient_tool_copy_node (copy_node, nde_node);

  filter = clay_drawable_filter_new (drawable,
                                     C_("undo-type", "Gradient"),
                                     nde_node,
                                     CLAY_ICON_TOOL_GRADIENT);
  g_clear_object (&nde_node);

  clay_drawable_filter_set_region (filter,
                                   CLAY_FILTER_REGION_DRAWABLE);
  clay_drawable_filter_set_opacity (filter,
                                    clay_context_get_opacity (context));
  clay_drawable_filter_set_mode (filter,
                                 clay_context_get_paint_mode (context),
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 clay_layer_mode_get_paint_composite_mode (
                                   clay_context_get_paint_mode (context)));

  clay_drawable_filter_apply_with_mask (filter, mask, NULL);
  clay_drawable_filter_layer_mask_freeze (filter);

  return filter;
}

static void
clay_gradient_tool_copy_node (GeglNode *node,
                              GeglNode *copy_to_node)
{
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = gegl_operation_list_properties ("clay:gradient", &n_pspecs);
  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      if (strcmp (pspec->name, "context") != 0)
        {
          g_value_init (&value, pspec->value_type);
          gegl_node_get_property (node, pspec->name, &value);

          gegl_node_set_property (copy_to_node, pspec->name,
                                  &value);
          g_value_unset (&value);
        }
    }
  g_free (pspecs);
}

static void
clay_gradient_tool_filter_flush (ClayDrawableFilter *filter,
                                 ClayTool           *tool)
{
  ClayImage *image = clay_display_get_image (tool->display);

  clay_projection_flush (clay_image_get_projection (image));
}

/*  public functions     */

void
clay_gradient_tool_set_existing_filter (ClayGradientTool   *gradient_tool,
                                        ClayDisplay        *display,
                                        gint                drawable_id,
                                        gint                filter_index)
{
  ClayTool                *tool    = CLAY_TOOL (gradient_tool);
  ClayGradientOptions     *options = CLAY_GRADIENT_TOOL_GET_OPTIONS (gradient_tool);
  ClayContext             *context = CLAY_CONTEXT (options);
  ClayContainer           *filters;
  ClayDrawable            *drawable;
  ClayDrawableFilterMask  *mask;
  ClayCoords               start_coords = { 0, 0 };
  ClayCoords               end_coords   = { 0, 0 };
  GeglNode                *existing_node;
  gchar                   *name;

  g_return_if_fail (CLAY_IS_GRADIENT_TOOL (gradient_tool));
  g_return_if_fail (display != NULL || CLAY_IS_DISPLAY (display));

  drawable = CLAY_DRAWABLE (clay_item_get_by_id (clay_display_get_clay (display),
                                                 drawable_id));

  g_return_if_fail (drawable != NULL || CLAY_IS_DRAWABLE (drawable));

  filters = clay_drawable_get_filters (drawable);
  gradient_tool->existing_filter =
    CLAY_DRAWABLE_FILTER (clay_container_get_child_by_index (filters,
                                                             filter_index));

  g_return_if_fail (gradient_tool->existing_filter != NULL ||
                    CLAY_IS_DRAWABLE_FILTER (gradient_tool->existing_filter));

  clay_filter_set_active (CLAY_FILTER (gradient_tool->existing_filter), FALSE);

  existing_node =
    clay_drawable_filter_get_operation (gradient_tool->existing_filter);
  gegl_node_get (existing_node,
                 "start-x", &start_coords.x,
                 "start-y", &start_coords.y,
                 "end-x",   &end_coords.x,
                 "end-y",   &end_coords.y,
                 NULL);

  clay_gradient_tool_button_press (tool, &start_coords, 0, GDK_BUTTON1_MASK,
                                   CLAY_BUTTON_PRESS_NORMAL, display);

  g_object_set (gradient_tool->widget,
                "x2", end_coords.x,
                "y2", end_coords.y,
                NULL);

  clay_gradient_tool_copy_node (existing_node, gradient_tool->render_node);

  clay_drawable_filter_set_temporary (gradient_tool->filter, TRUE);
  clay_drawable_filter_set_region (gradient_tool->filter,
                                   CLAY_FILTER_REGION_DRAWABLE);
  clay_drawable_filter_set_opacity (gradient_tool->filter,
                                    clay_context_get_opacity (context));
  clay_drawable_filter_set_mode (gradient_tool->filter,
                                 clay_context_get_paint_mode (context),
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 CLAY_LAYER_COLOR_SPACE_AUTO,
                                 clay_layer_mode_get_paint_composite_mode (
                                   clay_context_get_paint_mode (context)));

  mask = clay_drawable_filter_get_mask (gradient_tool->existing_filter);
  clay_drawable_filter_apply_with_mask (gradient_tool->filter,
                                        CLAY_CHANNEL (mask), NULL);
  clay_drawable_filter_layer_mask_freeze (gradient_tool->filter);

  name = g_strdup_printf (_("Editing '%s'..."),
                          clay_object_get_name (gradient_tool->filter));
  g_object_set (gradient_tool->filter, "name", name, NULL);
  g_free (name);

  clay_container_reorder (filters, CLAY_OBJECT (gradient_tool->filter),
                          filter_index);

  /* Update gradient display */
  clay_gradient_tool_line_changed (gradient_tool->widget, gradient_tool);

  clay_gradient_tool_button_release (tool, &end_coords, 0, GDK_BUTTON1_MASK,
                                     CLAY_BUTTON_RELEASE_NORMAL, display);

  /* Temporary turn off instant mode since it would defeat the purpose of
   * editing a filter */
  gtk_widget_set_sensitive (options->instant_toggle, FALSE);
  gtk_widget_set_sensitive (options->editable_gradient_toggle, FALSE);
}


/*  protected functions  */


void
clay_gradient_tool_set_tentative_gradient (ClayGradientTool *gradient_tool,
                                           ClayGradient     *gradient)
{
  g_return_if_fail (CLAY_IS_GRADIENT_TOOL (gradient_tool));
  g_return_if_fail (gradient == NULL || CLAY_IS_GRADIENT (gradient));

  if (g_set_object (&gradient_tool->tentative_gradient, gradient))
    {
      if (gradient_tool->render_node)
        {
          gegl_node_set (gradient_tool->render_node,
                         "gradient", gradient ? gradient : gradient_tool->gradient,
                         NULL);

          clay_drawable_filter_apply (gradient_tool->filter, NULL);
        }
    }
}
