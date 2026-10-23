/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-tool-options-manager.c
 * Copyright (C) 2018 Michael Natterer <mitch@gimp.org>
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

#include "libclayconfig/clayconfig.h"

#include "tools-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claytoolinfo.h"

#include "paint/claypaintoptions.h"

#include "widgets/claywidgets-utils.h"

#include "clay-tool-options-manager.h"


typedef struct _ClayToolOptionsManager ClayToolOptionsManager;

struct _ClayToolOptionsManager
{
  Clay                *clay;
  ClayPaintOptions    *global_paint_options;
  ClayContextPropMask  global_props;

  ClayToolInfo        *active_tool;
};


/*  local function prototypes  */

static ClayContextPropMask
              tool_options_manager_get_global_props
                                                 (ClayCoreConfig         *config);

static void   tool_options_manager_global_notify (ClayCoreConfig         *config,
                                                  const GParamSpec       *pspec,
                                                  ClayToolOptionsManager *manager);
static void   tool_options_manager_paint_options_notify
                                                 (ClayPaintOptions       *src,
                                                  const GParamSpec       *pspec,
                                                  ClayPaintOptions       *dest);

static void   tool_options_manager_copy_paint_props
                                                 (ClayPaintOptions       *src,
                                                  ClayPaintOptions       *dest,
                                                  ClayContextPropMask     prop_mask);

static void   tool_options_manager_tool_changed  (ClayContext            *user_context,
                                                  ClayToolInfo           *tool_info,
                                                  ClayToolOptionsManager *manager);


static GQuark manager_quark = 0;


/*  public functions  */

void
clay_tool_options_manager_init (Clay *clay)
{
  ClayToolOptionsManager *manager;
  ClayContext            *user_context;
  GList                  *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (manager_quark == 0);

  manager_quark = g_quark_from_static_string ("clay-tool-options-manager");

  manager = g_slice_new0 (ClayToolOptionsManager);

  g_object_set_qdata (G_OBJECT (clay), manager_quark, manager);

  manager->clay = clay;

  manager->global_paint_options =
    g_object_new (CLAY_TYPE_PAINT_OPTIONS,
                  "clay", clay,
                  "name", "tool-options-manager-global-paint-options",
                  NULL);

  manager->global_props = tool_options_manager_get_global_props (clay->config);

  user_context = clay_get_user_context (clay);

  for (list = clay_get_tool_info_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = list->data;

      /*  the global props that are actually used by the tool are
       *  always shared with the user context by undefining them...
       */
      clay_context_define_properties (CLAY_CONTEXT (tool_info->tool_options),
                                      manager->global_props &
                                      tool_info->context_props,
                                      FALSE);

      /*  ...and setting the user context as parent
       */
      clay_context_set_parent (CLAY_CONTEXT (tool_info->tool_options),
                               user_context);

      /*  make sure paint tools also share their brush, dynamics,
       *  gradient properties if the resp. context properties are
       *  global
       */
      if (CLAY_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          g_signal_connect (tool_info->tool_options, "notify",
                            G_CALLBACK (tool_options_manager_paint_options_notify),
                            manager->global_paint_options);

          g_signal_connect (manager->global_paint_options, "notify",
                            G_CALLBACK (tool_options_manager_paint_options_notify),
                            tool_info->tool_options);

          tool_options_manager_copy_paint_props (manager->global_paint_options,
                                                 CLAY_PAINT_OPTIONS (tool_info->tool_options),
                                                 tool_info->context_props &
                                                 manager->global_props);
        }
    }

  g_signal_connect (clay->config, "notify::global-brush",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-dynamics",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-pattern",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-palette",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-gradient",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-font",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (clay->config, "notify::global-expand",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_options_manager_tool_changed),
                    manager);

  tool_options_manager_tool_changed (user_context,
                                     clay_context_get_tool (user_context),
                                     manager);
}

void
clay_tool_options_manager_exit (Clay *clay)
{
  ClayToolOptionsManager *manager;
  ClayContext            *user_context;
  GList                  *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  manager = g_object_get_qdata (G_OBJECT (clay), manager_quark);

  g_return_if_fail (manager != NULL);

  user_context = clay_get_user_context (clay);

  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_options_manager_tool_changed,
                                        manager);

  g_signal_handlers_disconnect_by_func (clay->config,
                                        tool_options_manager_global_notify,
                                        manager);

  for (list = clay_get_tool_info_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = list->data;

      clay_context_set_parent (CLAY_CONTEXT (tool_info->tool_options), NULL);

      if (CLAY_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          g_signal_handlers_disconnect_by_func (tool_info->tool_options,
                                                tool_options_manager_paint_options_notify,
                                                manager->global_paint_options);

          g_signal_handlers_disconnect_by_func (manager->global_paint_options,
                                                tool_options_manager_paint_options_notify,
                                                tool_info->tool_options);
        }
    }

  g_clear_object (&manager->global_paint_options);

  g_slice_free (ClayToolOptionsManager, manager);

  g_object_set_qdata (G_OBJECT (clay), manager_quark, NULL);
}


/*  private functions  */

static ClayContextPropMask
tool_options_manager_get_global_props (ClayCoreConfig *config)
{
  ClayContextPropMask global_props = 0;

  /*  FG and BG are always shared between all tools  */
  global_props |= CLAY_CONTEXT_PROP_MASK_FOREGROUND;
  global_props |= CLAY_CONTEXT_PROP_MASK_BACKGROUND;

  if (config->global_brush)
    global_props |= CLAY_CONTEXT_PROP_MASK_BRUSH;
  if (config->global_dynamics)
    global_props |= CLAY_CONTEXT_PROP_MASK_DYNAMICS;
  if (config->global_pattern)
    global_props |= CLAY_CONTEXT_PROP_MASK_PATTERN;
  if (config->global_palette)
    global_props |= CLAY_CONTEXT_PROP_MASK_PALETTE;
  if (config->global_gradient)
    global_props |= CLAY_CONTEXT_PROP_MASK_GRADIENT;
  if (config->global_font)
    global_props |= CLAY_CONTEXT_PROP_MASK_FONT;
  if (config->global_expand)
    global_props |= CLAY_CONTEXT_PROP_MASK_EXPAND;

  return global_props;
}

static void
tool_options_manager_global_notify (ClayCoreConfig         *config,
                                    const GParamSpec       *pspec,
                                    ClayToolOptionsManager *manager)
{
  ClayContextPropMask  global_props;
  ClayContextPropMask  enabled_global_props;
  ClayContextPropMask  disabled_global_props;
  GList               *list;

  global_props = tool_options_manager_get_global_props (config);

  enabled_global_props  = global_props & ~manager->global_props;
  disabled_global_props = manager->global_props & ~global_props;

  /*  copy the newly enabled global props to all tool options, and
   *  disconnect the newly disabled ones from the user context
   */
  for (list = clay_get_tool_info_iter (manager->clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = list->data;

      /*  don't change the active tool, it is always fully connected
       *  to the user_context anyway because we set its
       *  defined/undefined context props in tool_changed()
       */
      if (tool_info == manager->active_tool)
        continue;

      /*  defining the newly disabled ones disconnects them from the
       *  parent user context
       */
      clay_context_define_properties (CLAY_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props &
                                      disabled_global_props,
                                      TRUE);

      /*  undefining the newly enabled ones copies the value from the
       *  parent user context
       */
      clay_context_define_properties (CLAY_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props &
                                      enabled_global_props,
                                      FALSE);

      if (CLAY_IS_PAINT_OPTIONS (tool_info->tool_options))
        tool_options_manager_copy_paint_props (manager->global_paint_options,
                                               CLAY_PAINT_OPTIONS (tool_info->tool_options),
                                               tool_info->context_props &
                                               enabled_global_props);
    }

  manager->global_props = global_props;
}

static void
tool_options_manager_paint_options_notify (ClayPaintOptions *src,
                                           const GParamSpec *pspec,
                                           ClayPaintOptions *dest)
{
  Clay                   *clay   = CLAY_CONTEXT (src)->clay;
  ClayCoreConfig         *config = clay->config;
  ClayToolOptionsManager *manager;
  ClayToolInfo           *tool_info;
  ClayContextPropMask     prop_mask = 0;
  gboolean                active    = FALSE;

  manager = g_object_get_qdata (G_OBJECT (clay), manager_quark);

  /*  one of the options is the global one, the other is the tool's,
   *  get the tool_info from the tool's options
   */
  if (manager->global_paint_options == src)
    tool_info = clay_context_get_tool (CLAY_CONTEXT (dest));
  else
    tool_info = clay_context_get_tool (CLAY_CONTEXT (src));

  if (tool_info == manager->active_tool)
    active = TRUE;

  if ((active || config->global_brush) &&
      tool_info->context_props & CLAY_CONTEXT_PROP_MASK_BRUSH)
    {
      prop_mask |= CLAY_CONTEXT_PROP_MASK_BRUSH;
    }

  if ((active || config->global_dynamics) &&
      tool_info->context_props & CLAY_CONTEXT_PROP_MASK_DYNAMICS)
    {
      prop_mask |= CLAY_CONTEXT_PROP_MASK_DYNAMICS;
    }

  if ((active || config->global_gradient) &&
      tool_info->context_props & CLAY_CONTEXT_PROP_MASK_GRADIENT)
    {
      prop_mask |= CLAY_CONTEXT_PROP_MASK_GRADIENT;
    }

  if ((active || config->global_expand) &&
      tool_info->context_props & CLAY_CONTEXT_PROP_MASK_EXPAND)
    {
      prop_mask |= CLAY_CONTEXT_PROP_MASK_EXPAND;
    }

  if (clay_paint_options_is_prop (pspec->name, prop_mask))
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (src), pspec->name, &value);

      g_signal_handlers_block_by_func (dest,
                                       tool_options_manager_paint_options_notify,
                                       src);

      g_object_set_property (G_OBJECT (dest), pspec->name, &value);

      g_signal_handlers_unblock_by_func (dest,
                                         tool_options_manager_paint_options_notify,
                                         src);

      g_value_unset (&value);
    }
}

static void
tool_options_manager_copy_paint_props (ClayPaintOptions    *src,
                                       ClayPaintOptions    *dest,
                                       ClayContextPropMask  prop_mask)
{
  g_signal_handlers_block_by_func (dest,
                                   tool_options_manager_paint_options_notify,
                                   src);

  clay_paint_options_copy_props (src, dest, prop_mask);

  g_signal_handlers_unblock_by_func (dest,
                                     tool_options_manager_paint_options_notify,
                                     src);
}

static void
tool_options_manager_tool_changed (ClayContext            *user_context,
                                   ClayToolInfo           *tool_info,
                                   ClayToolOptionsManager *manager)
{
  if (tool_info == manager->active_tool)
    return;

  /*  FIXME: clay_busy HACK
   *  the tool manager will stop the emission, so simply return
   */
  if (manager->clay->busy)
    return;

  if (manager->active_tool)
    {
      ClayToolInfo *active = manager->active_tool;

      /*  disconnect the old active tool from all context properties
       *  it uses, but are not currently global
       */
      clay_context_define_properties (CLAY_CONTEXT (active->tool_options),
                                      active->context_props &
                                      ~manager->global_props,
                                      TRUE);
    }

  manager->active_tool = tool_info;

  if (manager->active_tool)
    {
      ClayToolInfo *active = manager->active_tool;

      /*  make sure the tool options GUI always exists, this call
       *  creates it if needed, so tools always have their option GUI
       *  available even if the tool options dockable is not open, see
       *  for example issue #3435
       */
      clay_tools_get_tool_options_gui (active->tool_options);

      /*  copy the new tool's context properties that are not
       *  currently global to the user context, so they get used by
       *  everything
       */
      clay_context_copy_properties (CLAY_CONTEXT (active->tool_options),
                                    clay_get_user_context (manager->clay),
                                    active->context_props &
                                    ~manager->global_props);

      if (CLAY_IS_PAINT_OPTIONS (active->tool_options))
        tool_options_manager_copy_paint_props (CLAY_PAINT_OPTIONS (active->tool_options),
                                               manager->global_paint_options,
                                               active->context_props &
                                               ~manager->global_props);

      /*  then, undefine these properties so the tool syncs with the
       *  user context automatically
       */
      clay_context_define_properties (CLAY_CONTEXT (active->tool_options),
                                      active->context_props &
                                      ~manager->global_props,
                                      FALSE);
    }
}
