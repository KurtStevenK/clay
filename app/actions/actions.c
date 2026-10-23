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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayitem.h"
#include "core/claytooloptions.h"
#include "core/claytoolinfo.h"

#include "widgets/clayactionfactory.h"
#include "widgets/clayactiongroup.h"
#include "widgets/claycontainereditor.h"
#include "widgets/claycontainerview.h"
#include "widgets/claydock.h"
#include "widgets/claydockable.h"
#include "widgets/claydockwindow.h"
#include "widgets/clayimageeditor.h"
#include "widgets/clayitemtreeview.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/clayimagewindow.h"
#include "display/claynavigationeditor.h"
#include "display/claystatusbar.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "brush-editor-actions.h"
#include "brushes-actions.h"
#include "buffers-actions.h"
#include "channels-actions.h"
#include "colormap-actions.h"
#include "context-actions.h"
#include "cursor-info-actions.h"
#include "dashboard-actions.h"
#include "debug-actions.h"
#include "dialogs-actions.h"
#include "dock-actions.h"
#include "dockable-actions.h"
#include "documents-actions.h"
#include "drawable-actions.h"
#include "dynamics-actions.h"
#include "dynamics-editor-actions.h"
#include "edit-actions.h"
#include "error-console-actions.h"
#include "file-actions.h"
#include "filters-actions.h"
#include "fonts-actions.h"
#include "gradient-editor-actions.h"
#include "gradients-actions.h"
#include "help-actions.h"
#include "image-actions.h"
#include "images-actions.h"
#include "layers-actions.h"
#include "mypaint-brushes-actions.h"
#include "palette-editor-actions.h"
#include "palettes-actions.h"
#include "paths-actions.h"
#include "patterns-actions.h"
#include "plug-in-actions.h"
#include "quick-mask-actions.h"
#include "sample-points-actions.h"
#include "select-actions.h"
#include "templates-actions.h"
#include "text-editor-actions.h"
#include "text-tool-actions.h"
#include "tool-options-actions.h"
#include "tool-path-actions.h"
#include "tool-presets-actions.h"
#include "tool-preset-editor-actions.h"
#include "tools-actions.h"
#include "view-actions.h"
#include "windows-actions.h"

#include "clay-intl.h"


/*  global variables  */

ClayActionFactory *global_action_factory = NULL;
GHashTable        *aux_filter_hash_table = NULL;


/*  private variables  */

static const ClayActionFactoryEntry action_groups[] =
{
  { "brush-editor", N_("Brush Editor"), CLAY_ICON_BRUSH,
    brush_editor_actions_setup,
    brush_editor_actions_update },
  { "brushes", N_("Brushes"), CLAY_ICON_BRUSH,
    brushes_actions_setup,
    brushes_actions_update },
  { "buffers", N_("Buffers"), CLAY_ICON_BUFFER,
    buffers_actions_setup,
    buffers_actions_update },
  { "channels", N_("Channels"), CLAY_ICON_CHANNEL,
    channels_actions_setup,
    channels_actions_update },
  { "colormap", N_("Colormap"), CLAY_ICON_COLORMAP,
    colormap_actions_setup,
    colormap_actions_update },
  { "context", N_("Context"), CLAY_ICON_DIALOG_TOOL_OPTIONS /* well... */,
    context_actions_setup,
    context_actions_update },
  { "cursor-info", N_("Pointer Information"), NULL,
    cursor_info_actions_setup,
    cursor_info_actions_update },
  { "dashboard", N_("Dashboard"), CLAY_ICON_DIALOG_DASHBOARD,
    dashboard_actions_setup,
    dashboard_actions_update },
  { "debug", N_("Debug"), NULL,
    debug_actions_setup,
    debug_actions_update },
  { "dialogs", N_("Dialogs"), NULL,
    dialogs_actions_setup,
    dialogs_actions_update },
  { "dock", N_("Dock"), NULL,
    dock_actions_setup,
    dock_actions_update },
  { "dockable", N_("Dockable"), NULL,
    dockable_actions_setup,
    dockable_actions_update },
  { "documents", N_("Document History"), NULL,
    documents_actions_setup,
    documents_actions_update },
  { "drawable", N_("Drawable"), CLAY_ICON_LAYER,
    drawable_actions_setup,
    drawable_actions_update },
  { "dynamics", N_("Paint Dynamics"), CLAY_ICON_DYNAMICS,
    dynamics_actions_setup,
    dynamics_actions_update },
  { "dynamics-editor", N_("Paint Dynamics Editor"), CLAY_ICON_DYNAMICS,
    dynamics_editor_actions_setup,
    dynamics_editor_actions_update },
  { "edit", N_("Edit"), CLAY_ICON_EDIT,
    edit_actions_setup,
    edit_actions_update },
  { "error-console", N_("Error Console"), CLAY_ICON_DIALOG_WARNING,
    error_console_actions_setup,
    error_console_actions_update },
  { "file", N_("File"), "text-x-generic",
    file_actions_setup,
    file_actions_update },
  { "filters", N_("Filters"), CLAY_ICON_GEGL,
    filters_actions_setup,
    filters_actions_update },
  { "fonts", N_("Fonts"), CLAY_ICON_FONT,
    fonts_actions_setup,
    fonts_actions_update },
  { "gradient-editor", N_("Gradient Editor"), CLAY_ICON_GRADIENT,
    gradient_editor_actions_setup,
    gradient_editor_actions_update },
  { "gradients", N_("Gradients"), CLAY_ICON_GRADIENT,
    gradients_actions_setup,
    gradients_actions_update },
  { "tool-presets", N_("Tool Presets"), CLAY_ICON_TOOL_PRESET,
    tool_presets_actions_setup,
    tool_presets_actions_update },
  { "tool-preset-editor", N_("Tool Preset Editor"), CLAY_ICON_TOOL_PRESET,
    tool_preset_editor_actions_setup,
    tool_preset_editor_actions_update },
  { "help", N_("Help"), "help-browser",
    help_actions_setup,
    help_actions_update },
  { "image", N_("Image"), CLAY_ICON_IMAGE,
    image_actions_setup,
    image_actions_update },
  { "images", N_("Images"), CLAY_ICON_IMAGE,
    images_actions_setup,
    images_actions_update },
  { "layers", N_("Layers"), CLAY_ICON_LAYER,
    layers_actions_setup,
    layers_actions_update },
  { "mypaint-brushes", N_("MyPaint Brushes"), CLAY_ICON_MYPAINT_BRUSH,
    mypaint_brushes_actions_setup,
    mypaint_brushes_actions_update },
  { "palette-editor", N_("Palette Editor"), CLAY_ICON_PALETTE,
    palette_editor_actions_setup,
    palette_editor_actions_update },
  { "palettes", N_("Palettes"), CLAY_ICON_PALETTE,
    palettes_actions_setup,
    palettes_actions_update },
  { "patterns", N_("Patterns"), CLAY_ICON_PATTERN,
    patterns_actions_setup,
    patterns_actions_update },
  { "plug-in", N_("Plug-ins"), CLAY_ICON_PLUGIN,
    plug_in_actions_setup,
    plug_in_actions_update },
  { "quick-mask", N_("Quick Mask"), CLAY_ICON_QUICK_MASK_ON,
    quick_mask_actions_setup,
    quick_mask_actions_update },
  { "sample-points", N_("Sample Points"), CLAY_ICON_SAMPLE_POINT,
    sample_points_actions_setup,
    sample_points_actions_update },
  { "select", N_("Select"), CLAY_ICON_SELECTION,
    select_actions_setup,
    select_actions_update },
  { "templates", N_("Templates"), CLAY_ICON_TEMPLATE,
    templates_actions_setup,
    templates_actions_update },
  { "text-tool", N_("Text Tool"), CLAY_ICON_EDIT,
    text_tool_actions_setup,
    text_tool_actions_update },
  { "text-editor", N_("Text Editor"), CLAY_ICON_EDIT,
    text_editor_actions_setup,
    text_editor_actions_update },
  { "tool-options", N_("Tool Options"), CLAY_ICON_DIALOG_TOOL_OPTIONS,
    tool_options_actions_setup,
    tool_options_actions_update },
  { "tool-path", N_("Tool Path"), CLAY_ICON_PATH,
    tool_path_actions_setup,
    tool_path_actions_update },
  { "tools", N_("Tools"), CLAY_ICON_DIALOG_TOOLS,
    tools_actions_setup,
    tools_actions_update },
  { "paths", N_("Paths"), CLAY_ICON_PATH,
    paths_actions_setup,
    paths_actions_update },
  { "view", N_("View"), CLAY_ICON_VISIBLE,
    view_actions_setup,
    view_actions_update },
  { "windows", N_("Windows"), NULL,
    windows_actions_setup,
    windows_actions_update }
};


/*  public functions  */

void
actions_init (Clay *clay)
{
  gint i;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (global_action_factory == NULL);

  global_action_factory = clay_action_factory_new (clay);

  for (i = 0; i < G_N_ELEMENTS (action_groups); i++)
    clay_action_factory_group_register (global_action_factory,
                                        action_groups[i].identifier,
                                        gettext (action_groups[i].label),
                                        action_groups[i].icon_name,
                                        action_groups[i].setup_func,
                                        action_groups[i].update_func);

  aux_filter_hash_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

void
actions_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (global_action_factory != NULL);
  g_return_if_fail (global_action_factory->clay == clay);

  g_clear_object (&global_action_factory);
  g_hash_table_unref (aux_filter_hash_table);
}

/* XXX Temporary code to store the list of filter operations with an
 * "aux" input. This won't be necessary anymore once these filters can
 * be applied non-destructively too in the future.
 */
void
actions_filter_set_aux (const gchar *action_name)
{
  g_hash_table_add (aux_filter_hash_table, (gpointer) g_strdup (action_name));
}

gboolean
actions_filter_get_aux (const gchar *action_name)
{
  return g_hash_table_lookup (aux_filter_hash_table, action_name) != NULL;
}

Clay *
action_data_get_clay (gpointer data)
{
  Clay            *result    = NULL;
  static gboolean  recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (CLAY_IS_CLAY (data))
    result = data;

  if (! result)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        result = display->clay;
    }

  if (! result)
    {
      ClayContext *context = action_data_get_context (data);

      if (context)
        result = context->clay;
    }

  recursion = FALSE;

  return result;
}

ClayContext *
action_data_get_context (gpointer data)
{
  ClayContext     *result    = NULL;
  static gboolean  recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (CLAY_IS_DOCK (data))
    result = clay_dock_get_context ((ClayDock *) data);
  else if (CLAY_IS_DOCK_WINDOW (data))
    result = clay_dock_window_get_context (((ClayDockWindow *) data));
  else if (CLAY_IS_CONTAINER_VIEW (data))
    result = clay_container_view_get_context ((ClayContainerView *) data);
  else if (CLAY_IS_CONTAINER_EDITOR (data))
    result = clay_container_view_get_context (((ClayContainerEditor *) data)->view);
  else if (CLAY_IS_IMAGE_EDITOR (data))
    result = ((ClayImageEditor *) data)->context;
  else if (CLAY_IS_NAVIGATION_EDITOR (data))
    result = ((ClayNavigationEditor *) data)->context;

  if (! result)
    {
      Clay *clay = action_data_get_clay (data);

      if (clay)
        result = clay_get_user_context (clay);
    }

  recursion = FALSE;

  return result;
}

ClayImage *
action_data_get_image (gpointer data)
{
  ClayImage       *result    = NULL;
  static gboolean  recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (CLAY_IS_ITEM_TREE_VIEW (data))
    {
      result = clay_item_tree_view_get_image ((ClayItemTreeView *) data);
      recursion = FALSE;
      return result;
    }
  else if (CLAY_IS_IMAGE_EDITOR (data))
    {
      result = ((ClayImageEditor *) data)->image;
      recursion = FALSE;
      return result;
    }

  if (! result)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        result = clay_display_get_image (display);
    }

  if (! result)
    {
      ClayContext *context = action_data_get_context (data);

      if (context)
        result = clay_context_get_image (context);
    }

  recursion = FALSE;

  return result;
}

ClayDisplay *
action_data_get_display (gpointer data)
{
  ClayDisplay     *result    = NULL;
  static gboolean  recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (CLAY_IS_DISPLAY (data))
    result = data;
  else if (CLAY_IS_IMAGE_WINDOW (data))
    {
      ClayDisplayShell *shell = clay_image_window_get_active_shell (data);
      result = shell ? shell->display : NULL;
    }

  if (! result)
    {
      ClayContext *context = action_data_get_context (data);

      if (context)
        result = clay_context_get_display (context);
    }

  recursion = FALSE;

  return result;
}

ClayDisplayShell *
action_data_get_shell (gpointer data)
{
  ClayDisplayShell *result    = NULL;
  static gboolean   recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (! result)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        result = clay_display_get_shell (display);
    }

  recursion = FALSE;

  return result;
}

GtkWidget *
action_data_get_widget (gpointer data)
{
  GtkWidget       *result    = NULL;
  static gboolean  recursion = FALSE;

  if (! data || recursion)
    return NULL;

  recursion = TRUE;

  if (GTK_IS_WIDGET (data))
    result = data;

  if (! result)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        result = GTK_WIDGET (clay_display_get_shell (display));
    }

  if (! result)
    result = dialogs_get_toolbox ();

  recursion = FALSE;

  return result;
}

gint
action_data_sel_count (gpointer data)
{
  if (CLAY_IS_CONTAINER_EDITOR (data))
    {
      ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);

      return clay_container_view_get_selected (editor->view, NULL);
    }
  else
    {
      return 0;
    }
}

/* action_select_value:
 * @select_type:
 * @value:
 * @min:
 * max:
 * def:
 * small_inc:
 * inc:
 * skip_inc:
 * delta_factor:
 * wrap:
 *
 * For any valid enum @value (which are all negative), the corresponding
 * action computes the semantic value (default, first, next, etc.).
 * For a positive @value, it is considered as a per-thousand value
 * between the @min and @max (possibly wrapped if @wrap is set, clamped
 * otherwise), allowing to compute the returned value.
 *
 * Returns: the computed value to use for the action.
 */
gdouble
action_select_value (ClayActionSelectType  select_type,
                     gdouble               value,
                     gdouble               min,
                     gdouble               max,
                     gdouble               def,
                     gdouble               small_inc,
                     gdouble               inc,
                     gdouble               skip_inc,
                     gdouble               delta_factor,
                     gboolean              wrap)
{
  switch (select_type)
    {
    case CLAY_ACTION_SELECT_SET_TO_DEFAULT:
      value = def;
      break;

    case CLAY_ACTION_SELECT_FIRST:
      value = min;
      break;

    case CLAY_ACTION_SELECT_LAST:
      value = max;
      break;

    case CLAY_ACTION_SELECT_SMALL_PREVIOUS:
      value -= small_inc;
      break;

    case CLAY_ACTION_SELECT_SMALL_NEXT:
      value += small_inc;
      break;

    case CLAY_ACTION_SELECT_PREVIOUS:
      value -= inc;
      break;

    case CLAY_ACTION_SELECT_NEXT:
      value += inc;
      break;

    case CLAY_ACTION_SELECT_SKIP_PREVIOUS:
      value -= skip_inc;
      break;

    case CLAY_ACTION_SELECT_SKIP_NEXT:
      value += skip_inc;
      break;

    case CLAY_ACTION_SELECT_PERCENT_PREVIOUS:
      g_return_val_if_fail (delta_factor >= 0.0, value);
      value /= (1.0 + delta_factor);
      break;

    case CLAY_ACTION_SELECT_PERCENT_NEXT:
      g_return_val_if_fail (delta_factor >= 0.0, value);
      value *= (1.0 + delta_factor);
      break;

    default:
      if ((gint) select_type >= 0)
        value = (gdouble) select_type * (max - min) / 1000.0 + min;
      else
        g_return_val_if_reached (value);
      break;
    }

  if (wrap)
    {
      while (value < min)
        value = max - (min - value);

      while (value > max)
        value = min + (value - max);
    }
  else
    {
      value = CLAMP (value, min, max);
    }

  return value;
}

void
action_select_property (ClayActionSelectType  select_type,
                        ClayDisplay          *display,
                        GObject              *object,
                        const gchar          *property_name,
                        gdouble               small_inc,
                        gdouble               inc,
                        gdouble               skip_inc,
                        gdouble               delta_factor,
                        gboolean              wrap)
{
  GParamSpec *pspec;

  g_return_if_fail (display == NULL || CLAY_IS_DISPLAY (display));
  g_return_if_fail (G_IS_OBJECT (object));
  g_return_if_fail (property_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                        property_name);

  if (G_IS_PARAM_SPEC_DOUBLE (pspec))
    {
      gdouble value;

      g_object_get (object, property_name, &value, NULL);

      value = action_select_value (select_type,
                                   value,
                                   G_PARAM_SPEC_DOUBLE (pspec)->minimum,
                                   G_PARAM_SPEC_DOUBLE (pspec)->maximum,
                                   G_PARAM_SPEC_DOUBLE (pspec)->default_value,
                                   small_inc, inc, skip_inc, delta_factor, wrap);

      g_object_set (object, property_name, value, NULL);

      if (display)
        {
          const gchar *blurb = g_param_spec_get_blurb (pspec);

          if (blurb)
            {
              /*  value description and new value shown in the status bar  */
              action_message (display, object, _("%s: %.2f"), blurb, value);
            }
        }
    }
  else if (G_IS_PARAM_SPEC_INT (pspec))
    {
      gint value;

      g_object_get (object, property_name, &value, NULL);

      value = action_select_value (select_type,
                                   value,
                                   G_PARAM_SPEC_INT (pspec)->minimum,
                                   G_PARAM_SPEC_INT (pspec)->maximum,
                                   G_PARAM_SPEC_INT (pspec)->default_value,
                                   small_inc, inc, skip_inc, delta_factor, wrap);

      g_object_set (object, property_name, value, NULL);

      if (display)
        {
          const gchar *blurb = g_param_spec_get_blurb (pspec);

          if (blurb)
            {
              /*  value description and new value shown in the status bar  */
              action_message (display, object, _("%s: %d"), blurb, value);
            }
        }
    }
  else
    {
      g_return_if_reached ();
    }
}

ClayObject *
action_select_object (ClayActionSelectType  select_type,
                      ClayContainer        *container,
                      ClayObject           *current)
{
  gint select_index;
  gint n_children;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (current == NULL || CLAY_IS_OBJECT (current), NULL);

  if (! current                               &&
      select_type != CLAY_ACTION_SELECT_FIRST &&
      select_type != CLAY_ACTION_SELECT_LAST)
    return NULL;

  n_children = clay_container_get_n_children (container);

  if (n_children == 0)
    return NULL;

  switch (select_type)
    {
    case CLAY_ACTION_SELECT_FIRST:
      select_index = 0;
      break;

    case CLAY_ACTION_SELECT_LAST:
      select_index = n_children - 1;
      break;

    case CLAY_ACTION_SELECT_PREVIOUS:
      select_index = clay_container_get_child_index (container, current) - 1;
      break;

    case CLAY_ACTION_SELECT_NEXT:
      select_index = clay_container_get_child_index (container, current) + 1;
      break;

    case CLAY_ACTION_SELECT_SKIP_PREVIOUS:
      select_index = clay_container_get_child_index (container, current) - 10;
      break;

    case CLAY_ACTION_SELECT_SKIP_NEXT:
      select_index = clay_container_get_child_index (container, current) + 10;
      break;

    case CLAY_ACTION_SELECT_FLAT_PREVIOUS:
      /* XXX We only support this case and the next for ClayItem
       * containers.
       */
      g_return_val_if_fail (CLAY_IS_ITEM (current), NULL);
      select_index = clay_container_get_child_index (container, current) - 1;
      if (select_index < 0)
        {
          ClayItem *parent = clay_item_get_parent (CLAY_ITEM (current));

          if (parent)
            return CLAY_OBJECT (parent);
          else
            return current;
        }
      else
        {
          ClayObject    *prev;
          ClayContainer *prev_container;

          prev = clay_container_get_child_by_index (container, select_index);
          while ((prev_container = clay_viewable_get_children (CLAY_VIEWABLE (prev))) != NULL)
            {
              if (clay_viewable_get_expanded (CLAY_VIEWABLE (prev)))
                {
                  gint n_prev_children;

                  n_prev_children = clay_container_get_n_children (prev_container);
                  if (n_prev_children > 0)
                    {
                      select_index = n_prev_children - 1;
                      container = prev_container;
                      prev = clay_container_get_child_by_index (container, select_index);
                      continue;
                    }
                }
              break;
            }
        }
      break;

    case CLAY_ACTION_SELECT_FLAT_NEXT:
        {
          ClayContainer *current_container;

          g_return_val_if_fail (CLAY_IS_ITEM (current), NULL);

          if ((current_container = clay_viewable_get_children (CLAY_VIEWABLE (current))) != NULL &&
              clay_viewable_get_expanded (CLAY_VIEWABLE (current))                               &&
              clay_container_get_n_children (current_container) > 0)
            {
              select_index = 0;
              container = current_container;
            }
          else
            {
              select_index = clay_container_get_child_index (container, current) + 1;
              if (select_index >= n_children)
                {
                  while (TRUE)
                    {
                      ClayItem      *parent;
                      ClayContainer *parent_container;
                      gint           n_parent_children;

                      parent = clay_item_get_parent (CLAY_ITEM (current));
                      if (parent == NULL)
                        break;

                      parent_container = clay_item_get_container (parent);
                      n_parent_children = clay_container_get_n_children (parent_container);
                      if (clay_container_get_child_index (parent_container, CLAY_OBJECT (parent)) + 1 < n_parent_children)
                        {
                          container = parent_container;
                          select_index = clay_container_get_child_index (parent_container, CLAY_OBJECT (parent)) + 1;
                          break;
                        }

                      current = CLAY_OBJECT (parent);
                    }
                }
            }
        }
      break;

    default:
      if ((gint) select_type >= 0)
        select_index = (gint) select_type;
      else
        g_return_val_if_reached (current);
      break;
    }

  n_children = clay_container_get_n_children (container);
  select_index = CLAMP (select_index, 0, n_children - 1);

  return clay_container_get_child_by_index (container, select_index);
}

void
action_message (ClayDisplay *display,
                GObject     *object,
                const gchar *format,
                ...)
{
  ClayDisplayShell *shell     = clay_display_get_shell (display);
  ClayStatusbar    *statusbar = clay_display_shell_get_statusbar (shell);
  const gchar      *icon_name = NULL;
  va_list           args;

  if (CLAY_IS_TOOL_OPTIONS (object))
    {
      ClayToolInfo *tool_info = CLAY_TOOL_OPTIONS (object)->tool_info;

      icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (tool_info));
    }
  else if (CLAY_IS_VIEWABLE (object))
    {
      icon_name = clay_viewable_get_icon_name (CLAY_VIEWABLE (object));
    }

  va_start (args, format);
  clay_statusbar_push_temp_valist (statusbar, CLAY_MESSAGE_INFO,
                                   icon_name, format, args);
  va_end (args);
}
