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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/claytoolgroup.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"
#include "core/claytoolpreset.h"

#include "display/claydisplay.h"

#include "widgets/claycairo-wilber.h"

#include "claygegltool.h"
#include "claytool.h"
#include "claytoolcontrol.h"
#include "claytransformgridtool.h"
#include "tool_manager.h"


typedef struct _ClayToolManager ClayToolManager;

struct _ClayToolManager
{
  Clay          *clay;

  ClayTool      *active_tool;
  GSList        *tool_stack;

  GList         *history;
  gboolean       block_initialization;

  ClayToolGroup *active_tool_group;

  ClayImage     *image;

  GQuark         image_clean_handler_id;
  GQuark         image_dirty_handler_id;
  GQuark         image_saving_handler_id;
};


/*  local function prototypes  */

static ClayToolManager * tool_manager_get                        (Clay            *clay);

static void              tool_manager_select_tool                (ClayToolManager *tool_manager,
                                                                  ClayTool        *tool);

static void              tool_manager_set_active_tool_group      (ClayToolManager *tool_manager,
                                                                  ClayToolGroup   *tool_group);

static void              tool_manager_tool_changed               (ClayContext     *user_context,
                                                                  ClayToolInfo    *tool_info,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_preset_changed             (ClayContext     *user_context,
                                                                  ClayToolPreset  *preset,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_image_clean_dirty          (ClayImage       *image,
                                                                  ClayDirtyMask    dirty_mask,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_image_saving               (ClayImage       *image,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_tool_ancestry_changed      (ClayToolInfo    *tool_info,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_group_active_tool_changed  (ClayToolGroup   *tool_group,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_image_changed              (ClayContext     *context,
                                                                  ClayImage       *image,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_selected_drawables_changed (ClayImage       *image,
                                                                  ClayToolManager *tool_manager);
static void              tool_manager_active_tool_deactivated    (ClayContext     *context,
                                                                  ClayToolManager *tool_manager);

static void              tool_manager_cast_spell                 (ClayToolInfo    *tool_info);


static GQuark tool_manager_quark = 0;


/*  public functions  */

void
tool_manager_init (Clay *clay)
{
  ClayToolManager *tool_manager;
  ClayContext     *user_context;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (tool_manager_quark == 0);

  tool_manager_quark = g_quark_from_static_string ("clay-tool-manager");

  tool_manager = g_slice_new0 (ClayToolManager);

  tool_manager->clay                 = clay;
  tool_manager->history              = NULL;
  tool_manager->block_initialization = FALSE;

  g_object_set_qdata (G_OBJECT (clay), tool_manager_quark, tool_manager);

  tool_manager->image_clean_handler_id =
    clay_container_add_handler (clay->images, "clean",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_dirty_handler_id =
    clay_container_add_handler (clay->images, "dirty",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_saving_handler_id =
    clay_container_add_handler (clay->images, "saving",
                                G_CALLBACK (tool_manager_image_saving),
                                tool_manager);

  user_context = clay_get_user_context (clay);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_manager_tool_changed),
                    tool_manager);
  g_signal_connect (user_context, "tool-preset-changed",
                    G_CALLBACK (tool_manager_preset_changed),
                    tool_manager);
  g_signal_connect (user_context, "image-changed",
                    G_CALLBACK (tool_manager_image_changed),
                    tool_manager);

  tool_manager_image_changed (user_context,
                              clay_context_get_image (user_context),
                              tool_manager);
  tool_manager_selected_drawables_changed (clay_context_get_image (user_context),
                                           tool_manager);

  tool_manager_tool_changed (user_context,
                             clay_context_get_tool (user_context),
                             tool_manager);
}

void
tool_manager_exit (Clay *clay)
{
  ClayToolManager *tool_manager;
  ClayContext     *user_context;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  g_return_if_fail (tool_manager != NULL);

  user_context = clay_get_user_context (clay);

  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_tool_changed,
                                        tool_manager);
  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_preset_changed,
                                        tool_manager);
  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_image_changed,
                                        tool_manager);

  if (tool_manager->image)
    g_signal_handlers_disconnect_by_func (tool_manager->image,
                                          tool_manager_selected_drawables_changed,
                                          tool_manager);

  clay_container_remove_handler (clay->images,
                                 tool_manager->image_clean_handler_id);
  clay_container_remove_handler (clay->images,
                                 tool_manager->image_dirty_handler_id);
  clay_container_remove_handler (clay->images,
                                 tool_manager->image_saving_handler_id);

  if (tool_manager->active_tool)
    {
      g_signal_handlers_disconnect_by_func (
        tool_manager->active_tool->tool_info,
        tool_manager_tool_ancestry_changed,
        tool_manager);

      g_clear_object (&tool_manager->active_tool);
    }

  tool_manager_set_active_tool_group (tool_manager, NULL);

  g_list_free (tool_manager->history);

  g_slice_free (ClayToolManager, tool_manager);

  g_object_set_qdata (G_OBJECT (clay), tool_manager_quark, NULL);
}

ClayTool *
tool_manager_get_active (Clay *clay)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  tool_manager = tool_manager_get (clay);

  return tool_manager->active_tool;
}

void
tool_manager_push_tool (Clay     *clay,
                        ClayTool *tool)
{
  ClayToolManager *tool_manager;
  ClayDisplay     *focus_display = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_TOOL (tool));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      focus_display = tool_manager->active_tool->focus_display;

      tool_manager->tool_stack = g_slist_prepend (tool_manager->tool_stack,
                                                  tool_manager->active_tool);

      g_object_ref (tool_manager->tool_stack->data);
    }

  tool_manager_select_tool (tool_manager, tool);

  if (focus_display)
    tool_manager_focus_display_active (clay, focus_display);
}

void
tool_manager_pop_tool (Clay *clay)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->tool_stack)
    {
      ClayTool *tool = tool_manager->tool_stack->data;

      tool_manager->tool_stack = g_slist_remove (tool_manager->tool_stack,
                                                 tool);

      tool_manager_select_tool (tool_manager, tool);

      g_object_unref (tool);
    }
}

void
tool_manager_swap_tools (Clay     *clay,
                         gboolean  block_initialization)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (g_list_length (tool_manager->history) > 1)
    {
      tool_manager->block_initialization = block_initialization;
      clay_context_set_tool (tool_manager->clay->user_context,
                             g_list_nth_data (tool_manager->history, 1));
      tool_manager->block_initialization = FALSE;
    }
}

gboolean
tool_manager_initialize_active (Clay        *clay,
                                ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (CLAY_IS_DISPLAY (display), FALSE);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      ClayTool *tool = tool_manager->active_tool;

      if (clay_tool_initialize (tool, display))
        {
          ClayImage *image = clay_display_get_image (display);

          g_list_free (tool->drawables);
          tool->drawables = clay_image_get_selected_drawables (image);

          return TRUE;
        }
    }

  return FALSE;
}

void
tool_manager_control_active (Clay           *clay,
                             ClayToolAction  action,
                             ClayDisplay    *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      ClayTool *tool = tool_manager->active_tool;

      if (display && clay_tool_has_display (tool, display))
        {
          clay_tool_control (tool, action, display);
        }
      else if (action == CLAY_TOOL_ACTION_HALT)
        {
          if (clay_tool_control_is_active (tool->control))
            clay_tool_control_halt (tool->control);
        }
    }
}

void
tool_manager_button_press_active (Clay                *clay,
                                  const ClayCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  ClayButtonPressType  press_type,
                                  ClayDisplay         *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      clay_tool_button_press (tool_manager->active_tool,
                              coords, time, state, press_type,
                              display);
    }
}

void
tool_manager_button_release_active (Clay             *clay,
                                    const ClayCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    ClayDisplay      *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      clay_tool_button_release (tool_manager->active_tool,
                                coords, time, state,
                                display);
    }
}

void
tool_manager_motion_active (Clay             *clay,
                            const ClayCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            ClayDisplay      *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      clay_tool_motion (tool_manager->active_tool,
                        coords, time, state,
                        display);
    }
}

gboolean
tool_manager_key_press_active (Clay        *clay,
                               GdkEventKey *kevent,
                               ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_key_press (tool_manager->active_tool,
                                  kevent,
                                  display);
    }

  return FALSE;
}

gboolean
tool_manager_key_release_active (Clay        *clay,
                                 GdkEventKey *kevent,
                                 ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_key_release (tool_manager->active_tool,
                                    kevent,
                                    display);
    }

  return FALSE;
}

void
tool_manager_focus_display_active (Clay        *clay,
                                   ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool &&
      ! clay_tool_control_is_active (tool_manager->active_tool->control))
    {
      clay_tool_set_focus_display (tool_manager->active_tool,
                                   display);
    }
}

void
tool_manager_modifier_state_active (Clay            *clay,
                                    GdkModifierType  state,
                                    ClayDisplay     *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool &&
      ! clay_tool_control_is_active (tool_manager->active_tool->control))
    {
      clay_tool_set_modifier_state (tool_manager->active_tool,
                                    state,
                                    display);
    }
}

void
tool_manager_active_modifier_state_active (Clay            *clay,
                                           GdkModifierType  state,
                                           ClayDisplay     *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      clay_tool_set_active_modifier_state (tool_manager->active_tool,
                                           state,
                                           display);
    }
}

void
tool_manager_oper_update_active (Clay             *clay,
                                 const ClayCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 ClayDisplay      *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool &&
      ! clay_tool_control_is_active (tool_manager->active_tool->control))
    {
      clay_tool_oper_update (tool_manager->active_tool,
                             coords, state, proximity,
                             display);
    }
}

void
tool_manager_cursor_update_active (Clay             *clay,
                                   const ClayCoords *coords,
                                   GdkModifierType   state,
                                   ClayDisplay      *display)
{
  ClayToolManager *tool_manager;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool &&
      ! clay_tool_control_is_active (tool_manager->active_tool->control))
    {
      clay_tool_cursor_update (tool_manager->active_tool,
                               coords, state,
                               display);
    }
}

const gchar *
tool_manager_can_undo_active (Clay        *clay,
                              ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_can_undo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

const gchar *
tool_manager_can_redo_active (Clay        *clay,
                              ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_can_redo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

gboolean
tool_manager_undo_active (Clay        *clay,
                          ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_undo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

gboolean
tool_manager_redo_active (Clay        *clay,
                          ClayDisplay *display)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_redo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

ClayUIManager *
tool_manager_get_popup_active (Clay             *clay,
                               const ClayCoords *coords,
                               GdkModifierType   state,
                               ClayDisplay      *display,
                               const gchar     **ui_path)
{
  ClayToolManager *tool_manager;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  tool_manager = tool_manager_get (clay);

  if (tool_manager->active_tool)
    {
      return clay_tool_get_popup (tool_manager->active_tool,
                                  coords, state,
                                  display,
                                  ui_path);
    }

  return NULL;
}


/*  private functions  */

static ClayToolManager *
tool_manager_get (Clay *clay)
{
  return g_object_get_qdata (G_OBJECT (clay), tool_manager_quark);
}

static void
tool_manager_select_tool (ClayToolManager *tool_manager,
                          ClayTool        *tool)
{
  Clay *clay = tool_manager->clay;

  /*  reset the previously selected tool, but only if it is not only
   *  temporarily pushed to the tool stack
   */
  if (tool_manager->active_tool)
    {
      if (! tool_manager->tool_stack ||
          tool_manager->active_tool != tool_manager->tool_stack->data)
        {
          ClayTool    *active_tool = tool_manager->active_tool;
          ClayDisplay *display;

          /*  NULL image returns any display (if there is any)  */
          display = clay_tool_has_image (active_tool, NULL);

          tool_manager_control_active (clay, CLAY_TOOL_ACTION_HALT, display);
          tool_manager_focus_display_active (clay, NULL);
        }
    }

  if (tool_manager->history == NULL ||
      tool_manager->history->data != tool->tool_info)
    {
      /* Update the history up to 3 element maximum and avoiding
       * duplicates.
       */
      GList *found;

      if ((found = g_list_find (tool_manager->history, tool->tool_info)))
        {
          tool_manager->history = g_list_remove_link (tool_manager->history, found);
          tool_manager->history = g_list_concat (found, tool_manager->history);
        }
      else
        {
          tool_manager->history = g_list_prepend (tool_manager->history, tool->tool_info);
          if (g_list_length (tool_manager->history) > 3)
            tool_manager->history = g_list_delete_link (tool_manager->history,
                                                        g_list_last (tool_manager->history));
        }

      if (g_list_length (tool_manager->history) > 1)
        {
          /* Never store filter tool in history (only as current tool). */
          GList        *prev_list = g_list_nth (tool_manager->history, 1);
          ClayToolInfo *prev_tool = prev_list->data;

          if (g_type_is_a (prev_tool->tool_type, CLAY_TYPE_FILTER_TOOL))
            tool_manager->history = g_list_delete_link (tool_manager->history, prev_list);
        }
    }

  g_set_object (&tool_manager->active_tool, tool);
}

static void
tool_manager_set_active_tool_group (ClayToolManager *tool_manager,
                                    ClayToolGroup   *tool_group)
{
  if (tool_group != tool_manager->active_tool_group)
    {
      if (tool_manager->active_tool_group)
        {
          g_signal_handlers_disconnect_by_func (
            tool_manager->active_tool_group,
            tool_manager_group_active_tool_changed,
            tool_manager);
        }

      g_set_weak_pointer (&tool_manager->active_tool_group, tool_group);

      if (tool_manager->active_tool_group)
        {
          g_signal_connect (
            tool_manager->active_tool_group, "active-tool-changed",
            G_CALLBACK (tool_manager_group_active_tool_changed),
            tool_manager);
        }
    }
}

static void
tool_manager_tool_changed (ClayContext     *user_context,
                           ClayToolInfo    *tool_info,
                           ClayToolManager *tool_manager)
{
  ClayTool *new_tool = NULL;

  if (! tool_info)
    return;

  if (! g_type_is_a (tool_info->tool_type, CLAY_TYPE_TOOL))
    {
      g_warning ("%s: tool_info->tool_type is no ClayTool subclass",
                 G_STRFUNC);
      return;
    }

  /* FIXME: clay_busy HACK */
  if (user_context->clay->busy)
    {
      if (G_TYPE_FROM_INSTANCE (tool_manager->active_tool) !=
          tool_info->tool_type)
        {
          /*  there may be contexts waiting for the user_context's "tool-changed"
           *  signal, so stop emitting it.
           */
          g_signal_stop_emission_by_name (user_context, "tool-changed");

          g_signal_handlers_block_by_func (user_context,
                                           tool_manager_tool_changed,
                                           tool_manager);

          /*  explicitly set the current tool  */
          clay_context_set_tool (user_context,
                                 tool_manager->active_tool->tool_info);

          g_signal_handlers_unblock_by_func (user_context,
                                             tool_manager_tool_changed,
                                             tool_manager);
        }

      return;
    }

  g_return_if_fail (tool_manager->tool_stack == NULL);

  if (tool_manager->active_tool)
    {
      tool_manager_active_tool_deactivated (user_context, tool_manager);
    }

  g_signal_connect (tool_info, "ancestry-changed",
                    G_CALLBACK (tool_manager_tool_ancestry_changed),
                    tool_manager);

  tool_manager_tool_ancestry_changed (tool_info, tool_manager);

  new_tool = g_object_new (tool_info->tool_type,
                           "tool-info", tool_info,
                           NULL);

  tool_manager_select_tool (tool_manager, new_tool);

  /* Auto-activate any transform or GEGL operation tools */
  if (! tool_manager->block_initialization &&
      (CLAY_IS_TRANSFORM_GRID_TOOL (new_tool) ||
       CLAY_IS_GEGL_TOOL (new_tool)))
    {
      ClayDisplay *new_display;

      new_display = clay_context_get_display (user_context);
      if (new_display && clay_display_get_image (new_display))
        tool_manager_initialize_active (user_context->clay, new_display);
    }

  g_object_unref (new_tool);

  if (clay_widget_animation_enabled ())
    {
      /* ??? */
      tool_manager_cast_spell (tool_info);
    }
}

static void
tool_manager_copy_tool_options (GObject *src,
                                GObject *dest)
{
  GList *diff;

  diff = clay_config_diff (src, dest, G_PARAM_READWRITE);

  if (diff)
    {
      GList *list;

      g_object_freeze_notify (dest);

      for (list = diff; list; list = list->next)
        {
          GParamSpec *prop_spec = list->data;

          if (g_type_is_a (prop_spec->owner_type, CLAY_TYPE_TOOL_OPTIONS) &&
              ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
            {
              GValue value = G_VALUE_INIT;

              g_value_init (&value, prop_spec->value_type);

              g_object_get_property (src,  prop_spec->name, &value);
              g_object_set_property (dest, prop_spec->name, &value);

              g_value_unset (&value);
            }
        }

      g_object_thaw_notify (dest);

      g_list_free (diff);
    }
}

static void
tool_manager_preset_changed (ClayContext     *user_context,
                             ClayToolPreset  *preset,
                             ClayToolManager *tool_manager)
{
  ClayToolInfo *preset_tool;

  if (! preset || user_context->clay->busy)
    return;

  preset_tool = clay_context_get_tool (CLAY_CONTEXT (preset->tool_options));

  /* first, select the preset's tool, even if it's already the active
   * tool
   */
  clay_context_set_tool (user_context, preset_tool);

  /* then, copy the context properties the preset remembers, possibly
   * changing some tool options due to the "link brush stuff to brush
   * defaults" settings in claytooloptions.c
   */
  clay_context_copy_properties (CLAY_CONTEXT (preset->tool_options),
                                user_context,
                                clay_tool_preset_get_prop_mask (preset));

  /* finally, copy all tool options properties, overwriting any
   * changes resulting from setting the context properties above, we
   * really want exactly what is in the preset and nothing else
   */
  tool_manager_copy_tool_options (G_OBJECT (preset->tool_options),
                                  G_OBJECT (preset_tool->tool_options));
}

static void
tool_manager_image_clean_dirty (ClayImage       *image,
                                ClayDirtyMask    dirty_mask,
                                ClayToolManager *tool_manager)
{
  ClayTool *tool = tool_manager->active_tool;

  if (tool &&
      ! clay_tool_control_get_preserve (tool->control) &&
      (clay_tool_control_get_dirty_mask (tool->control) & dirty_mask))
    {
      ClayDisplay *display = clay_tool_has_image (tool, image);

      if (display)
        {
          tool_manager_control_active (
            image->clay,
            clay_tool_control_get_dirty_action (tool->control),
            display);
        }
    }
}

static void
tool_manager_image_saving (ClayImage       *image,
                           ClayToolManager *tool_manager)
{
  ClayTool *tool = tool_manager->active_tool;

  if (tool &&
      ! clay_tool_control_get_preserve (tool->control))
    {
      ClayDisplay *display = clay_tool_has_image (tool, image);

      if (display)
        tool_manager_control_active (image->clay, CLAY_TOOL_ACTION_COMMIT,
                                     display);
    }
}

static void
tool_manager_tool_ancestry_changed (ClayToolInfo    *tool_info,
                                    ClayToolManager *tool_manager)
{
  ClayViewable *parent;

  parent = clay_viewable_get_parent (CLAY_VIEWABLE (tool_info));

  if (parent)
    {
      clay_tool_group_set_active_tool_info (CLAY_TOOL_GROUP (parent),
                                            tool_info);
    }

  tool_manager_set_active_tool_group (tool_manager, CLAY_TOOL_GROUP (parent));
}

static void
tool_manager_group_active_tool_changed (ClayToolGroup   *tool_group,
                                        ClayToolManager *tool_manager)
{
  clay_context_set_tool (tool_manager->clay->user_context,
                         clay_tool_group_get_active_tool_info (tool_group));
}

static void
tool_manager_image_changed (ClayContext     *context,
                            ClayImage       *image,
                            ClayToolManager *tool_manager)
{
  if (tool_manager->image)
    g_signal_handlers_disconnect_by_func (tool_manager->image,
                                          tool_manager_selected_drawables_changed,
                                          tool_manager);

  tool_manager->image = image;

  /* Turn off auto-activated tools when switching images */
  if (image                                                    &&
      (CLAY_IS_TRANSFORM_GRID_TOOL (tool_manager->active_tool) ||
       CLAY_IS_GEGL_TOOL (tool_manager->active_tool)))
    {
      clay_context_set_tool (context,
                             tool_manager->active_tool->tool_info);

      tool_manager_active_tool_deactivated (context, tool_manager);
    }

  if (image)
    g_signal_connect (tool_manager->image, "selected-drawables-changed",
                      G_CALLBACK (tool_manager_selected_drawables_changed),
                      tool_manager);
}

static void
tool_manager_selected_drawables_changed (ClayImage       *image,
                                         ClayToolManager *tool_manager)
{
  /* Re-activate transform tools when changing selected layers */
  if (image                                                    &&
      (CLAY_IS_TRANSFORM_GRID_TOOL (tool_manager->active_tool) ||
       CLAY_IS_GEGL_TOOL (tool_manager->active_tool)))
    {
      clay_context_set_tool (tool_manager->clay->user_context,
                             tool_manager->active_tool->tool_info);

      tool_manager_tool_changed (tool_manager->clay->user_context,
                                 clay_context_get_tool (tool_manager->clay->user_context),
                                 tool_manager);
    }
}

static void
tool_manager_active_tool_deactivated (ClayContext     *context,
                                      ClayToolManager *tool_manager)
{
  ClayDisplay *display;

  display = clay_tool_has_image (tool_manager->active_tool, NULL);

  /*  NULL image returns any display (if there is any)  */
  if (display)
    tool_manager_control_active (context->clay, CLAY_TOOL_ACTION_COMMIT,
                                 display);

  /*  commit the old tool's operation before creating the new tool
   *  because creating a tool might mess with the old tool's
   *  options (old and new tool might be the same)
   */
  g_signal_handlers_disconnect_by_func (tool_manager->active_tool->tool_info,
                                        tool_manager_tool_ancestry_changed,
                                        tool_manager);
}

static void
tool_manager_cast_spell (ClayToolInfo *tool_info)
{
  typedef struct
  {
    const gchar *sequence;
    GCallback    func;
  } Spell;

  static const Spell spells[] =
  {
    { .sequence = "clay-warp-tool\0"
                  "clay-iscissors-tool\0"
                  "clay-gradient-tool\0"
                  "clay-path-tool\0"
                  "clay-ellipse-select-tool\0"
                  "clay-rect-select-tool\0",
      .func     = clay_cairo_wilber_toggle_pointer_eyes
    }
  };

  static const gchar *spell_progress[G_N_ELEMENTS (spells)];
  const gchar        *tool_name;
  gint                i;

  tool_name = clay_object_get_name (CLAY_OBJECT (tool_info));

  for (i = 0; i < G_N_ELEMENTS (spells); i++)
    {
      if (! spell_progress[i])
        spell_progress[i] = spells[i].sequence;

      while (spell_progress[i])
        {
          if (! strcmp (tool_name, spell_progress[i]))
            {
              spell_progress[i] += strlen (spell_progress[i]) + 1;

              if (! *spell_progress[i])
                {
                  spell_progress[i] = NULL;

                  spells[i].func ();
                }

              break;
            }
          else
            {
              if (spell_progress[i] == spells[i].sequence)
                spell_progress[i] = NULL;
              else
                spell_progress[i] = spells[i].sequence;
            }
        }
    }
}
