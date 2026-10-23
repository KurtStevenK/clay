/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claygeglprocedure.c
 * Copyright (C) 2016-2018 Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "actions-types.h"

#include "config/clayguiconfig.h"

#include "operations/clay-operation-config.h"
#include "operations/clayoperationsettings.h"

#include "core/clay.h"
#include "core/clay-memsize.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydisplay.h"
#include "core/claydrawable-operation.h"
#include "core/claydrawablefilter.h"
#include "core/clayimage.h"
#include "core/claylayermask.h"
#include "core/clayparamspecs.h"
#include "core/claysettings.h"
#include "core/claytoolinfo.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claystringaction.h"
#include "widgets/clayuimanager.h"

#include "tools/clayoperationtool.h"
#include "tools/tool_manager.h"

#include "claygeglprocedure.h"

#include "clay-intl.h"


static void     clay_gegl_procedure_finalize            (GObject        *object);

static gint64   clay_gegl_procedure_get_memsize         (ClayObject     *object,
                                                         gint64         *gui_size);

static gchar  * clay_gegl_procedure_get_description     (ClayViewable   *viewable,
                                                         gchar         **tooltip);

static const gchar * clay_gegl_procedure_get_help_id    (ClayProcedure  *procedure);
static const gchar * clay_gegl_procedure_get_menu_label (ClayProcedure  *procedure);
static gboolean      clay_gegl_procedure_get_sensitive  (ClayProcedure  *procedure,
                                                         ClayObject     *object,
                                                         const gchar   **reason);
static ClayValueArray * clay_gegl_procedure_execute     (ClayProcedure  *procedure,
                                                         Clay           *clay,
                                                         ClayContext    *context,
                                                         ClayProgress   *progress,
                                                         ClayValueArray *args,
                                                         GError        **error);
static void     clay_gegl_procedure_execute_async       (ClayProcedure  *procedure,
                                                         Clay           *clay,
                                                         ClayContext    *context,
                                                         ClayProgress   *progress,
                                                         ClayValueArray *args,
                                                         ClayDisplay    *display);


G_DEFINE_TYPE (ClayGeglProcedure, clay_gegl_procedure,
               CLAY_TYPE_PROCEDURE)

#define parent_class clay_gegl_procedure_parent_class


static void
clay_gegl_procedure_class_init (ClayGeglProcedureClass *klass)
{
  GObjectClass       *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass    *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass  *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayProcedureClass *proc_class        = CLAY_PROCEDURE_CLASS (klass);

  object_class->finalize            = clay_gegl_procedure_finalize;

  clay_object_class->get_memsize    = clay_gegl_procedure_get_memsize;

  viewable_class->default_icon_name = "clay-gegl";
  viewable_class->get_description   = clay_gegl_procedure_get_description;

  proc_class->get_help_id           = clay_gegl_procedure_get_help_id;
  proc_class->get_menu_label        = clay_gegl_procedure_get_menu_label;
  proc_class->get_sensitive         = clay_gegl_procedure_get_sensitive;
  proc_class->execute               = clay_gegl_procedure_execute;
  proc_class->execute_async         = clay_gegl_procedure_execute_async;
}

static void
clay_gegl_procedure_init (ClayGeglProcedure *proc)
{
}

static void
clay_gegl_procedure_finalize (GObject *object)
{
  ClayGeglProcedure *proc = CLAY_GEGL_PROCEDURE (object);

  g_clear_object (&proc->default_settings);

  g_clear_pointer (&proc->operation,  g_free);
  g_clear_pointer (&proc->menu_label, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_gegl_procedure_get_memsize (ClayObject *object,
                                 gint64     *gui_size)
{
  ClayGeglProcedure *proc    = CLAY_GEGL_PROCEDURE (object);
  gint64             memsize = 0;

  memsize += clay_string_get_memsize (proc->operation);
  memsize += clay_string_get_memsize (proc->menu_label);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
clay_gegl_procedure_get_description (ClayViewable  *viewable,
                                     gchar        **tooltip)
{
  ClayProcedure *procedure = CLAY_PROCEDURE (viewable);

  if (tooltip)
    *tooltip = g_strdup (clay_procedure_get_blurb (procedure));

  return g_strdup (clay_procedure_get_label (procedure));
}

static const gchar *
clay_gegl_procedure_get_help_id (ClayProcedure *procedure)
{
  ClayGeglProcedure *proc = CLAY_GEGL_PROCEDURE (procedure);
  GList             *managers;
  ClayActionGroup   *group;
  const gchar       *help_id = NULL;

  managers = clay_ui_managers_from_name ("<Image>");
  group    = clay_ui_manager_get_action_group (managers->data, "filters");

  if (procedure->help_id)
    {
      return procedure->help_id;
    }
  else if (group)
    {
      GList *actions;
      GList *iter;

      actions = clay_action_group_list_actions (group);
      for (iter = actions; iter; iter = iter->next)
        if (CLAY_IS_STRING_ACTION (iter->data) &&
            g_strcmp0 (CLAY_STRING_ACTION (iter->data)->value, proc->operation) == 0)
          {
            help_id = clay_action_get_help_id (iter->data);
            break;
          }

      g_list_free (actions);
    }

  return help_id == NULL ? CLAY_HELP_TOOL_GEGL : help_id;
}

static const gchar *
clay_gegl_procedure_get_menu_label (ClayProcedure *procedure)
{
  ClayGeglProcedure *proc = CLAY_GEGL_PROCEDURE (procedure);

  if (proc->menu_label)
    return proc->menu_label;

  return CLAY_PROCEDURE_CLASS (parent_class)->get_menu_label (procedure);
}

static gboolean
clay_gegl_procedure_get_sensitive (ClayProcedure  *procedure,
                                   ClayObject     *object,
                                   const gchar   **reason)
{
  ClayImage *image     = CLAY_IMAGE (object);
  GList     *drawables = NULL;
  gboolean   sensitive = FALSE;

  if (image)
    drawables = clay_image_get_selected_drawables (image);

  if (g_list_length (drawables) == 1)
    {
      ClayDrawable *drawable  = drawables->data;
      ClayItem     *item;

      if (CLAY_IS_LAYER_MASK (drawable))
        item = CLAY_ITEM (clay_layer_mask_get_layer (CLAY_LAYER_MASK (drawable)));
      else
        item = CLAY_ITEM (drawable);

      sensitive = ! clay_item_is_content_locked (item, NULL);

      if (clay_viewable_get_children (CLAY_VIEWABLE (drawable)) &&
          CLAY_GEGL_PROCEDURE (procedure)->has_aux)
        sensitive = FALSE;
    }

  g_list_free (drawables);

  return sensitive;
}

static ClayValueArray *
clay_gegl_procedure_execute (ClayProcedure   *procedure,
                             Clay            *clay,
                             ClayContext     *context,
                             ClayProgress    *progress,
                             ClayValueArray  *args,
                             GError         **error)
{
  ClayImage  *image;
  gint        n_drawables;
  GObject   **drawables;
  GObject    *config;

  image       = g_value_get_object (clay_value_array_index (args, 1));
  drawables   = g_value_get_boxed  (clay_value_array_index (args, 2));
  config      = g_value_get_object (clay_value_array_index (args, 3));

  n_drawables = clay_core_object_array_get_length ((GObject **) drawables);

  if (n_drawables == 1)
    {
      GeglNode *node;

      node = gegl_node_new_child (NULL,
                                  "operation",
                                  CLAY_GEGL_PROCEDURE (procedure)->operation,
                                  NULL);

      clay_drawable_apply_operation_with_config (CLAY_DRAWABLE (drawables[0]),
                                                 progress,
                                                 clay_procedure_get_label (procedure),
                                                 node, config);

      g_object_unref (node);

      clay_image_flush (image);

      return clay_procedure_get_return_values (procedure, TRUE, NULL);
    }

  return clay_procedure_get_return_values (procedure, FALSE, NULL);
}

static void
clay_gegl_procedure_execute_async (ClayProcedure  *procedure,
                                   Clay           *clay,
                                   ClayContext    *context,
                                   ClayProgress   *progress,
                                   ClayValueArray *args,
                                   ClayDisplay    *display)
{
  ClayGeglProcedure *gegl_procedure = CLAY_GEGL_PROCEDURE (procedure);
  ClayRunMode        run_mode;
  ClayObject        *settings;
  ClayTool          *active_tool;
  const gchar       *tool_name;

  run_mode = g_value_get_enum   (clay_value_array_index (args, 0));
  settings = g_value_get_object (clay_value_array_index (args, 3));

  if (! settings &&
      (run_mode != CLAY_RUN_INTERACTIVE ||
       CLAY_GUI_CONFIG (clay->config)->filter_tool_use_last_settings))
    {
      /*  if we didn't get settings passed, get the last used settings  */

      GType          config_type;
      ClayContainer *container;

      config_type = G_VALUE_TYPE (clay_value_array_index (args, 3));

      container = clay_operation_config_get_container (clay, config_type,
                                                       (GCompareFunc)
                                                       clay_settings_compare);

      settings = clay_container_get_child_by_index (container, 0);

      /*  only use the settings if they are automatically created
       *  "last used" values, not if they were saved explicitly and
       *  have a zero timestamp; and if they are not a separator.
       */
      if (settings &&
          (CLAY_SETTINGS (settings)->time == 0 ||
           ! clay_object_get_name (settings)))
        {
          settings = NULL;
        }
    }

  if (run_mode == CLAY_RUN_NONINTERACTIVE ||
      run_mode == CLAY_RUN_WITH_LAST_VALS)
    {
      if (settings || run_mode == CLAY_RUN_NONINTERACTIVE)
        {
          ClayValueArray *return_vals;

          g_value_set_object (clay_value_array_index (args, 3), settings);
          return_vals = clay_procedure_execute (procedure, clay, context, progress,
                                                args, NULL);
          clay_value_array_unref (return_vals);
          return;
        }

      clay_message (clay,
                    G_OBJECT (progress), CLAY_MESSAGE_WARNING,
                    _("There are no last settings for '%s', "
                      "showing the filter dialog instead."),
                    clay_procedure_get_label (procedure));
    }

  if (! strcmp (gegl_procedure->operation, "clay:brightness-contrast"))
    {
      tool_name = "clay-brightness-contrast-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "clay:curves"))
    {
      tool_name = "clay-curves-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "clay:levels"))
    {
      tool_name = "clay-levels-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "clay:threshold"))
    {
      tool_name = "clay-threshold-tool";
    }
  else if (! strcmp (gegl_procedure->operation, "clay:offset"))
    {
      tool_name = "clay-offset-tool";
    }
  else
    {
      tool_name = "clay-operation-tool";
    }

  active_tool = tool_manager_get_active (clay);

  /*  do not use the passed context because we need to set the active
   *  tool on the global user context
   */
  context = clay_get_user_context (clay);

  if (strcmp (clay_object_get_name (active_tool->tool_info), tool_name))
    {
      ClayToolInfo *tool_info = clay_get_tool_info (clay, tool_name);

      if (CLAY_IS_TOOL_INFO (tool_info))
        clay_context_set_tool (context, tool_info);
    }
  else
    {
      clay_context_tool_changed (context);
    }

  active_tool = tool_manager_get_active (clay);

  if (! strcmp (clay_object_get_name (active_tool->tool_info), tool_name))
    {
      /*  Remember the procedure that created this tool, because
       *  we can't just switch to an operation tool using
       *  clay_context_set_tool(), we also have to go through the
       *  initialization code below, otherwise we end up with a
       *  dummy tool that does nothing. See bug #776370.
       */
      g_object_set_data_full (G_OBJECT (active_tool), "clay-gegl-procedure",
                              g_object_ref (procedure),
                              (GDestroyNotify) g_object_unref);

      if (! strcmp (tool_name, "clay-operation-tool"))
        {
          clay_operation_tool_set_operation (CLAY_OPERATION_TOOL (active_tool),
                                             gegl_procedure->filter,
                                             gegl_procedure->operation,
                                             clay_procedure_get_label (procedure),
                                             clay_procedure_get_label (procedure),
                                             clay_procedure_get_label (procedure),
                                             clay_viewable_get_icon_name (CLAY_VIEWABLE (procedure)),
                                             clay_procedure_get_help_id (procedure));
        }

      /* For custom Clay operations, we need to set the existing filter to edit
       * before initializing so it doesn't create a separate filter. */
      if (gegl_procedure->filter)
        CLAY_FILTER_TOOL (active_tool)->existing_filter = gegl_procedure->filter;

      tool_manager_initialize_active (clay, display);

      /* For Clay-specific GEGL operations, we need to copy over the
       * config object stored in the GeglNode */
      if (gegl_procedure->filter)
        {
          GeglNode *node;

          clay_filter_set_active (CLAY_FILTER (gegl_procedure->filter), FALSE);

          node = clay_drawable_filter_get_operation (gegl_procedure->filter);

          if (clay_operation_config_is_custom (clay, gegl_procedure->operation))
            {
              gegl_node_get (node,
                             "config", &settings,
                             NULL);
            }
          else
            {
              GParamSpec       **pspecs;
              guint              n_pspecs;
              gdouble            opacity;
              ClayLayerMode      paint_mode;
              ClayFilterRegion   region;
              gboolean           clip;

              opacity    = clay_drawable_filter_get_opacity (gegl_procedure->filter);
              paint_mode = clay_drawable_filter_get_paint_mode (gegl_procedure->filter);
              region     = clay_drawable_filter_get_region (gegl_procedure->filter);
              clip       = clay_drawable_filter_get_clip (gegl_procedure->filter);

              settings =
                g_object_new (clay_operation_config_get_type (active_tool->tool_info->clay,
                                                              gegl_procedure->operation,
                                                              clay_tool_get_icon_name (active_tool),
                                                              CLAY_TYPE_OPERATION_SETTINGS),
                              NULL);

              pspecs = gegl_operation_list_properties (gegl_procedure->operation, &n_pspecs);

              for (gint i = 0; i < n_pspecs; i++)
                {
                  GValue      value      = G_VALUE_INIT;
                  GParamSpec *pspec      = pspecs[i];
                  GParamSpec *clay_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings),
                                                                         pspec->name);

                  g_value_init (&value, pspec->value_type);
                  gegl_node_get_property (node, pspec->name,
                                          &value);

                  if (clay_pspec)
                    g_object_set_property (G_OBJECT (settings), clay_pspec->name,
                                           &value);
                  else
                    g_critical ("%s: property '%s' of operation '%s' doesn't exist in config %s",
                                G_STRFUNC, pspec->name, gegl_procedure->operation,
                                g_type_name (G_TYPE_FROM_INSTANCE (settings)));
                  g_value_unset (&value);
                }
              g_free (pspecs);

              g_object_set (settings,
                            "clay-opacity", opacity,
                            "clay-mode",    paint_mode,
                            "clay-region",  region,
                            "clay-clip",    clip,
                            NULL);
            }
        }

      if (settings)
        clay_filter_tool_set_config (CLAY_FILTER_TOOL (active_tool),
                                     CLAY_CONFIG (settings));
    }
}


/*  public functions  */

ClayProcedure *
clay_gegl_procedure_new (Clay               *clay,
                         ClayDrawableFilter *filter_to_edit,
                         ClayRunMode         default_run_mode,
                         ClayObject         *default_settings,
                         const gchar        *operation,
                         const gchar        *name,
                         const gchar        *menu_label,
                         const gchar        *tooltip,
                         const gchar        *icon_name,
                         const gchar        *help_id)
{
  ClayProcedure     *procedure;
  ClayGeglProcedure *gegl_procedure;
  GeglNode          *node;
  GType              config_type;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (operation != NULL, NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (menu_label != NULL, NULL);

  config_type = clay_operation_config_get_type (clay, operation, icon_name,
                                                CLAY_TYPE_OPERATION_SETTINGS);

  procedure = g_object_new (CLAY_TYPE_GEGL_PROCEDURE, NULL);

  gegl_procedure = CLAY_GEGL_PROCEDURE (procedure);

  gegl_procedure->filter           = filter_to_edit;
  gegl_procedure->operation        = g_strdup (operation);
  gegl_procedure->default_run_mode = default_run_mode;
  gegl_procedure->menu_label       = g_strdup (menu_label);

  if (default_settings)
    gegl_procedure->default_settings = g_object_ref (default_settings);

  /* TODO: Once we can serialize ClayDrawable, remove so that
   * filters with aux nodes can be non-destructive */
  node = gegl_node_new_child (NULL, "operation", operation, NULL);
  gegl_procedure->has_aux = gegl_node_has_pad (node, "aux");
  g_object_unref (node);

  clay_object_set_name (CLAY_OBJECT (procedure), name);
  clay_viewable_set_icon_name (CLAY_VIEWABLE (procedure), icon_name);
  clay_procedure_set_help (procedure,
                           tooltip,
                           tooltip,
                           help_id);
  clay_procedure_set_static_attribution (procedure,
                                         "author", "copyright", "date");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Run mode",
                                                     "Run mode",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_core_object_array ("drawables",
                                                                  "Drawables",
                                                                  "Input drawables",
                                                                  CLAY_TYPE_DRAWABLE,
                                                                  CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("settings",
                                                    "Settings",
                                                    "Settings",
                                                    config_type,
                                                    CLAY_PARAM_READWRITE));

  return procedure;
}

gboolean
clay_gegl_procedure_is_editing_filter (ClayGeglProcedure *procedure)
{
  g_return_val_if_fail (CLAY_IS_GEGL_PROCEDURE (procedure), FALSE);

  return (procedure->filter != NULL);
}
