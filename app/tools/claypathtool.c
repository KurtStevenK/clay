/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Path tool
 * Copyright (C) 2003 Simon Budig  <simon@gimp.org>
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
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-item.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayrasterizable.h"
#include "core/claystrokeoptions.h"
#include "core/claytoolinfo.h"
#include "core/clayundostack.h"

#include "paint/claypaintoptions.h" /* CLAY_PAINT_OPTIONS_CONTEXT_MASK */

#include "path/claypath.h"
#include "path/clayvectorlayer.h"
#include "path/clayvectorlayeroptions.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydockcontainer.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymenufactory.h"
#include "widgets/clayuimanager.h"
#include "widgets/clayviewabledialog.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claytoolpath.h"

#include "claypathoptions.h"
#include "claypathtool.h"
#include "claytoolcontrol.h"

#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "clay-intl.h"


#define TOGGLE_MASK  clay_get_extend_selection_mask ()
#define MOVE_MASK    GDK_MOD1_MASK
#define INSDEL_MASK  clay_get_toggle_behavior_mask ()

#define RESPONSE_NEW 1

/*  local function prototypes  */

static void     clay_path_tool_constructed        (GObject               *object);
static void     clay_path_tool_dispose            (GObject               *object);

static void     clay_path_tool_control            (ClayTool              *tool,
                                                   ClayToolAction         action,
                                                   ClayDisplay           *display);
static void     clay_path_tool_button_press       (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayButtonPressType    press_type,
                                                   ClayDisplay           *display);
static void     clay_path_tool_button_release     (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayButtonReleaseType  release_type,
                                                   ClayDisplay           *display);
static void     clay_path_tool_motion             (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   ClayDisplay           *display);
static void     clay_path_tool_modifier_key       (ClayTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   ClayDisplay           *display);
static void     clay_path_tool_cursor_update      (ClayTool              *tool,
                                                   const ClayCoords      *coords,
                                                   GdkModifierType        state,
                                                   ClayDisplay           *display);

static void     clay_path_tool_start              (ClayPathTool          *path_tool,
                                                   ClayDisplay           *display);
static void     clay_path_tool_halt               (ClayPathTool          *path_tool);

static void     clay_path_tool_image_changed      (ClayPathTool          *path_tool,
                                                   ClayImage             *image,
                                                   ClayContext           *context);
static void     clay_path_tool_image_selected_layers_changed
                                                  (ClayPathTool          *path_tool);
static void     clay_path_tool_update_selected_layers
                                                  (ClayPathTool          *path_tool,
                                                   gboolean               is_image_switch);

static void     clay_path_tool_tool_path_changed  (ClayToolWidget        *tool_path,
                                                   ClayPathTool          *path_tool);
static void     clay_path_tool_tool_path_begin_change
                                                  (ClayToolWidget        *tool_path,
                                                   const gchar           *desc,
                                                   ClayPathTool          *path_tool);
static void     clay_path_tool_tool_path_end_change
                                                  (ClayToolWidget        *tool_path,
                                                   gboolean               success,
                                                   ClayPathTool          *path_tool);
static void     clay_path_tool_tool_path_activate (ClayToolWidget        *tool_path,
                                                   GdkModifierType        state,
                                                   ClayPathTool          *path_tool);

static void     clay_path_tool_path_changed       (ClayImage             *image,
                                                   ClayPathTool          *path_tool);
static void     clay_path_tool_path_removed       (ClayPath              *path,
                                                   ClayPathTool          *path_tool);

static void     clay_path_tool_to_selection       (ClayPathTool          *path_tool);
static void     clay_path_tool_to_selection_extended
                                                  (ClayPathTool          *path_tool,
                                                   GdkModifierType        state);

static void     clay_path_tool_create_vector_layer(ClayPathTool          *path_tool);

static void     clay_path_tool_vector_layer_path_changed
                                                  (ClayVectorLayerOptions *options,
                                                   const GParamSpec       *pspec,
                                                   ClayPathTool           *tool);

static void     clay_path_tool_vector_change_notify
                                                  (GObject               *options,
                                                   const GParamSpec      *pspec,
                                                   ClayVectorLayer       *layer);

static void     clay_path_tool_layer_rasterized   (ClayVectorLayer       *layer,
                                                   gboolean               is_rasterized,
                                                   ClayPathTool          *tool);

static void     clay_path_tool_set_layer          (ClayPathTool          *path_tool,
                                                   ClayVectorLayer       *vector_layer);

static void     clay_path_tool_confirm_dialog     (ClayPathTool          *path_tool,
                                                   ClayVectorLayer       *layer);
static void     clay_path_tool_confirm_response   (ClayViewableDialog    *dialog,
                                                   gint                   response_id,
                                                   ClayPathTool          *path_tool);


G_DEFINE_TYPE (ClayPathTool, clay_path_tool, CLAY_TYPE_DRAW_TOOL)

#define parent_class clay_path_tool_parent_class


void
clay_path_tool_register (ClayToolRegisterCallback callback,
                         gpointer                 data)
{
  (* callback) (CLAY_TYPE_PATH_TOOL,
                CLAY_TYPE_PATH_OPTIONS,
                clay_path_options_gui,
                CLAY_PAINT_OPTIONS_CONTEXT_MASK |
                CLAY_CONTEXT_PROP_MASK_PATTERN  |
                CLAY_CONTEXT_PROP_MASK_GRADIENT, /* for stroking */
                "clay-path-tool",
                _("Paths"),
                _("Paths Tool: Create and edit paths"),
                N_("Pat_hs"), "b",
                NULL, CLAY_HELP_TOOL_PATH,
                CLAY_ICON_TOOL_PATH,
                data);
}

static void
clay_path_tool_class_init (ClayPathToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayToolClass *tool_class   = CLAY_TOOL_CLASS (klass);

  object_class->constructed  = clay_path_tool_constructed;
  object_class->dispose      = clay_path_tool_dispose;

  tool_class->control        = clay_path_tool_control;
  tool_class->button_press   = clay_path_tool_button_press;
  tool_class->button_release = clay_path_tool_button_release;
  tool_class->motion         = clay_path_tool_motion;
  tool_class->modifier_key   = clay_path_tool_modifier_key;
  tool_class->cursor_update  = clay_path_tool_cursor_update;
  tool_class->is_destructive = FALSE;
}

static void
clay_path_tool_init (ClayPathTool *path_tool)
{
  ClayTool *tool = CLAY_TOOL (path_tool);

  clay_tool_control_set_handle_empty_image (tool->control, TRUE);
  clay_tool_control_set_precision          (tool->control,
                                            CLAY_CURSOR_PRECISION_SUBPIXEL);
  clay_tool_control_set_tool_cursor        (tool->control,
                                            CLAY_TOOL_CURSOR_PATHS);

  path_tool->saved_mode = CLAY_PATH_MODE_DESIGN;
}

static void
clay_path_tool_constructed (GObject *object)
{
  ClayPathTool  *path_tool = CLAY_PATH_TOOL (object);
  ClayContext   *context;
  ClayToolInfo  *tool_info;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  tool_info = CLAY_TOOL (path_tool)->tool_info;

  context = clay_get_user_context (tool_info->clay);

  g_signal_connect_object (context, "image-changed",
                           G_CALLBACK (clay_path_tool_image_changed),
                           path_tool, G_CONNECT_SWAPPED);

  clay_path_tool_image_changed (path_tool,
                                clay_context_get_image (context),
                                context);
}

static void
clay_path_tool_dispose (GObject *object)
{
  ClayPathTool *path_tool = CLAY_PATH_TOOL (object);

  clay_path_tool_set_path (path_tool, NULL, NULL);
  g_clear_object (&path_tool->widget);

  clay_path_tool_image_changed (path_tool, NULL, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_path_tool_control (ClayTool       *tool,
                        ClayToolAction  action,
                        ClayDisplay    *display)
{
  ClayPathTool *path_tool = CLAY_PATH_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_path_tool_halt (path_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
clay_path_tool_button_press (ClayTool            *tool,
                             const ClayCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             ClayButtonPressType  press_type,
                             ClayDisplay         *display)
{
  ClayPathTool *path_tool = CLAY_PATH_TOOL (tool);

  if (tool->display && display != tool->display)
    clay_tool_control (tool, CLAY_TOOL_ACTION_HALT, tool->display);

  if (! tool->display)
    {
      clay_path_tool_start (path_tool, display);

      clay_tool_widget_hover (path_tool->widget, coords, state, TRUE);
    }

  if (clay_tool_widget_button_press (path_tool->widget, coords, time, state,
                                     press_type))
    {
      path_tool->grab_widget = path_tool->widget;
    }

  clay_tool_control_activate (tool->control);
}

static void
clay_path_tool_button_release (ClayTool              *tool,
                               const ClayCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               ClayButtonReleaseType  release_type,
                               ClayDisplay           *display)
{
  ClayPathTool *path_tool = CLAY_PATH_TOOL (tool);

  clay_tool_control_halt (tool->control);

  if (path_tool->grab_widget)
    {
      clay_tool_widget_button_release (path_tool->grab_widget,
                                       coords, time, state, release_type);
      path_tool->grab_widget = NULL;
    }
}

static void
clay_path_tool_motion (ClayTool         *tool,
                       const ClayCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       ClayDisplay      *display)
{
  ClayPathTool *path_tool = CLAY_PATH_TOOL (tool);

  if (path_tool->grab_widget)
    {
      clay_tool_widget_motion (path_tool->grab_widget, coords, time, state);
    }
}

static void
clay_path_tool_modifier_key (ClayTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             ClayDisplay     *display)
{
  ClayPathTool    *path_tool = CLAY_PATH_TOOL (tool);
  ClayPathOptions *options   = CLAY_PATH_TOOL_GET_OPTIONS (tool);

  if (key == TOGGLE_MASK)
    return;

  if (key == INSDEL_MASK || key == MOVE_MASK)
    {
      ClayPathMode button_mode = options->edit_mode;

      if (press)
        {
          if (key == (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  first modifier pressed  */

              path_tool->saved_mode = options->edit_mode;
            }
        }
      else
        {
          if (! (state & (INSDEL_MASK | MOVE_MASK)))
            {
              /*  last modifier released  */

              button_mode = path_tool->saved_mode;
            }
        }

      if (state & MOVE_MASK)
        {
          button_mode = CLAY_PATH_MODE_MOVE;
        }
      else if (state & INSDEL_MASK)
        {
          button_mode = CLAY_PATH_MODE_EDIT;
        }

      if (button_mode != options->edit_mode)
        {
          g_object_set (options, "path-edit-mode", button_mode, NULL);
        }
    }
}

static void
clay_path_tool_cursor_update (ClayTool         *tool,
                              const ClayCoords *coords,
                              GdkModifierType   state,
                              ClayDisplay      *display)
{
  ClayPathTool     *path_tool = CLAY_PATH_TOOL (tool);
  ClayDisplayShell *shell     = clay_display_get_shell (display);

  if (display != tool->display || ! path_tool->widget)
    {
      ClayToolCursorType tool_cursor = CLAY_TOOL_CURSOR_PATHS;

      if (clay_image_pick_path (clay_display_get_image (display),
                                coords->x, coords->y,
                                FUNSCALEX (shell,
                                           CLAY_TOOL_HANDLE_SIZE_CIRCLE / 2),
                                FUNSCALEY (shell,
                                           CLAY_TOOL_HANDLE_SIZE_CIRCLE / 2)))
        {
          tool_cursor = CLAY_TOOL_CURSOR_HAND;
        }

      clay_tool_control_set_tool_cursor (tool->control, tool_cursor);
    }

  CLAY_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
clay_path_tool_start (ClayPathTool *path_tool,
                      ClayDisplay  *display)
{
  ClayTool         *tool    = CLAY_TOOL (path_tool);
  ClayPathOptions  *options = CLAY_PATH_TOOL_GET_OPTIONS (tool);
  ClayDisplayShell *shell   = clay_display_get_shell (display);
  ClayToolWidget   *widget;

  tool->display = display;

  path_tool->widget = widget = clay_tool_path_new (shell);

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), widget);

  g_object_bind_property (G_OBJECT (options), "path-edit-mode",
                          G_OBJECT (widget),  "edit-mode",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (G_OBJECT (options), "path-polygonal",
                          G_OBJECT (widget),  "polygonal",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
  g_object_bind_property (G_OBJECT (options), "path-auto-connect",
                          G_OBJECT (widget),  "auto-connect",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);

  clay_tool_path_set_path (CLAY_TOOL_PATH (widget),
                           path_tool->path);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (clay_path_tool_tool_path_changed),
                    path_tool);
  g_signal_connect (widget, "begin-change",
                    G_CALLBACK (clay_path_tool_tool_path_begin_change),
                    path_tool);
  g_signal_connect (widget, "end-change",
                    G_CALLBACK (clay_path_tool_tool_path_end_change),
                    path_tool);
  g_signal_connect (widget, "activate",
                    G_CALLBACK (clay_path_tool_tool_path_activate),
                    path_tool);

  clay_draw_tool_start (CLAY_DRAW_TOOL (tool), display);
}

static void
clay_path_tool_halt (ClayPathTool *path_tool)
{
  ClayTool *tool = CLAY_TOOL (path_tool);

  if (tool->display)
    clay_tool_pop_status (tool, tool->display);

  clay_path_tool_set_path (path_tool, NULL, NULL);

  if (clay_draw_tool_is_active (CLAY_DRAW_TOOL (tool)))
    clay_draw_tool_stop (CLAY_DRAW_TOOL (tool));

  clay_draw_tool_set_widget (CLAY_DRAW_TOOL (tool), NULL);
  g_clear_object (&path_tool->widget);

  tool->display = NULL;
}

static void
clay_path_tool_image_changed (ClayPathTool *path_tool,
                              ClayImage    *image,
                              ClayContext  *context)
{
  ClayPathOptions *options = CLAY_PATH_TOOL_GET_OPTIONS (path_tool);

  if (path_tool->current_image)
    {
      g_signal_handlers_disconnect_by_func (path_tool->current_image,
                                            clay_path_tool_image_selected_layers_changed,
                                            NULL);

      g_signal_handlers_disconnect_by_func (options->vector_layer_button,
                                            clay_path_tool_create_vector_layer,
                                            path_tool);
    }

  g_set_weak_pointer (&path_tool->current_image, image);

  if (path_tool->current_image)
    {
      g_signal_connect_object (path_tool->current_image, "selected-layers-changed",
                               G_CALLBACK (clay_path_tool_image_selected_layers_changed),
                               path_tool, G_CONNECT_SWAPPED);

      g_signal_connect_object (options->vector_layer_button, "clicked",
                               G_CALLBACK (clay_path_tool_create_vector_layer),
                               path_tool, G_CONNECT_SWAPPED);
    }

  clay_path_tool_update_selected_layers (path_tool, TRUE);

  if (options->vector_layer_button)
    gtk_widget_set_sensitive (options->vector_layer_button,
                              path_tool->current_image ? TRUE : FALSE);
}

static void
clay_path_tool_image_selected_layers_changed (ClayPathTool *path_tool)
{
  clay_path_tool_update_selected_layers (path_tool, FALSE);
}

static void
clay_path_tool_update_selected_layers (ClayPathTool *path_tool,
                                       gboolean      is_image_switch)
{
  GList           *current_layers = NULL;
  ClayVectorLayer *vector_layer   = NULL;

  if (path_tool->current_image)
    current_layers = clay_image_get_selected_layers (path_tool->current_image);

  /* If we've selected a single vector layer, make its path editable */
  if (current_layers                      &&
      g_list_length (current_layers) == 1 &&
      CLAY_IS_VECTOR_LAYER (CLAY_ITEM (current_layers->data)))
    {
      vector_layer = current_layers->data;

      if (clay_item_is_vector_layer (CLAY_ITEM (current_layers->data)))
        {
          clay_path_tool_set_layer (path_tool, vector_layer);
        }
      else if (! is_image_switch)
        {
          clay_path_tool_confirm_dialog (path_tool, vector_layer);
        }
      else
        {
          clay_path_tool_set_layer (path_tool, NULL);
        }
    }
  else
    {
      clay_path_tool_set_layer (path_tool, NULL);
    }
}

static void
clay_path_tool_tool_path_changed (ClayToolWidget *tool_path,
                                  ClayPathTool   *path_tool)
{
  ClayDisplayShell *shell = clay_tool_widget_get_shell (tool_path);
  ClayImage        *image = clay_display_get_image (shell->display);
  ClayPath         *path;

  g_object_get (tool_path, "path", &path, NULL);

  if (path != path_tool->path)
    {
      if (path && ! clay_item_is_attached (CLAY_ITEM (path)))
        {
          clay_image_add_path (image, path,
                               CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
          clay_image_flush (image);

          clay_path_tool_set_path (path_tool, NULL, path);
        }
      else
        {
          clay_path_tool_set_path (path_tool, NULL, path);

          if (path)
            {
              GList *list = g_list_prepend (NULL, path);

              clay_image_set_selected_paths (image, list);
              g_list_free (list);
            }
        }
    }

  if (path)
    g_object_unref (path);
}

static void
clay_path_tool_tool_path_begin_change (ClayToolWidget *tool_path,
                                       const gchar    *desc,
                                       ClayPathTool   *path_tool)
{
  ClayDisplayShell *shell = clay_tool_widget_get_shell (tool_path);
  ClayImage        *image = clay_display_get_image (shell->display);

  clay_image_undo_push_path_mod (image, desc, path_tool->path);
}

static void
clay_path_tool_tool_path_end_change (ClayToolWidget *tool_path,
                                     gboolean        success,
                                     ClayPathTool   *path_tool)
{
  ClayDisplayShell *shell = clay_tool_widget_get_shell (tool_path);
  ClayImage        *image = clay_display_get_image (shell->display);

  if (! success)
    {
      ClayUndo            *undo;
      ClayUndoAccumulator  accum = { 0, };

      undo = clay_undo_stack_pop_undo (clay_image_get_undo_stack (image),
                                       CLAY_UNDO_MODE_UNDO, &accum);

      clay_image_undo_event (image, CLAY_UNDO_EVENT_UNDO_EXPIRED, undo);

      clay_undo_free (undo, CLAY_UNDO_MODE_UNDO);
      g_object_unref (undo);
    }

  clay_image_flush (image);
}

static void
clay_path_tool_tool_path_activate (ClayToolWidget  *tool_path,
                                   GdkModifierType  state,
                                   ClayPathTool    *path_tool)
{
  clay_path_tool_to_selection_extended (path_tool, state);
}

static void
clay_path_tool_path_changed (ClayImage    *image,
                             ClayPathTool *path_tool)
{
  ClayPath *path = NULL;

  /* The path tool can only work on one path at a time. */
  if (g_list_length (clay_image_get_selected_paths (image)) == 1)
    path = clay_image_get_selected_paths (image)->data;

  clay_path_tool_set_path (path_tool, NULL, path);
}

static void
clay_path_tool_path_removed (ClayPath     *path,
                             ClayPathTool *path_tool)
{
  clay_path_tool_set_path (path_tool, NULL, NULL);
}

void
clay_path_tool_set_path (ClayPathTool    *path_tool,
                         ClayVectorLayer *layer,
                         ClayPath        *path)
{
  ClayTool        *tool;
  ClayImage       *current_image = NULL;
  ClayItem        *item          = NULL;
  ClayPathOptions *options;

  g_return_if_fail (CLAY_IS_PATH_TOOL (path_tool));
  g_return_if_fail (path == NULL || CLAY_IS_PATH (path));
  g_return_if_fail (layer == NULL || path == NULL);

  tool    = CLAY_TOOL (path_tool);
  options = CLAY_PATH_TOOL_GET_OPTIONS (path_tool);

  if (layer != NULL)
    {
      path          = clay_vector_layer_get_path (layer);
      current_image = clay_item_get_image (CLAY_ITEM (layer));
    }

  if (path)
    item = CLAY_ITEM (path);

  if (path == path_tool->path)
    return;

  if (path_tool->current_vector_layer && path && ! layer)
    {
      ClayImage *image      = path_tool->current_image;
      GList     *all_layers = clay_image_get_layer_list (image);
      GList     *layers     = NULL;

      for (GList *iter = all_layers; iter; iter = iter->next)
        {
          if (clay_item_is_vector_layer (iter->data) &&
              clay_vector_layer_get_path (iter->data) == path)
            layers = g_list_prepend (layers, iter->data);
        }
      clay_image_set_selected_layers (image, layers);
      g_list_free (all_layers);
      g_list_free (layers);
    }

  if (path_tool->path)
    {
      ClayImage *old_image;

      old_image = clay_item_get_image (CLAY_ITEM (path_tool->path));

      g_signal_handlers_disconnect_by_func (old_image,
                                            clay_path_tool_path_changed,
                                            path_tool);
      g_signal_handlers_disconnect_by_func (path_tool->path,
                                            clay_path_tool_path_removed,
                                            path_tool);

      g_clear_object (&path_tool->path);

      if (options->to_selection_button)
        {
          gtk_widget_set_sensitive (options->to_selection_button, FALSE);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                clay_path_tool_to_selection,
                                                tool);
          g_signal_handlers_disconnect_by_func (options->to_selection_button,
                                                clay_path_tool_to_selection_extended,
                                                tool);
        }
    }

  /* If we're looking at a path instead of a vector layer, grab its image */
  if (! current_image && item)
    current_image = clay_item_get_image (item);

  if (! path ||
      (tool->display &&
       clay_display_get_image (tool->display) != current_image))
    {
      clay_path_tool_halt (path_tool);
    }

  if (layer || ! path)
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create New Vector Layer"));
  else
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create Vector Layer from Path"));

  if (! path)
    return;

  path_tool->path = g_object_ref (path);

  g_signal_connect_object (current_image, "selected-paths-changed",
                           G_CALLBACK (clay_path_tool_path_changed),
                           path_tool, 0);
  g_signal_connect_object (path, "removed",
                           G_CALLBACK (clay_path_tool_path_removed),
                           path_tool, 0);

  if (options->to_selection_button)
    {
      g_signal_connect_swapped (options->to_selection_button, "clicked",
                                G_CALLBACK (clay_path_tool_to_selection),
                                tool);
      g_signal_connect_swapped (options->to_selection_button, "extended-clicked",
                                G_CALLBACK (clay_path_tool_to_selection_extended),
                                tool);
      gtk_widget_set_sensitive (options->to_selection_button, TRUE);
    }

  if (tool->display)
    {
      clay_tool_path_set_path (CLAY_TOOL_PATH (path_tool->widget), path);
    }
  else
    {
      ClayContext *context = clay_get_user_context (tool->tool_info->clay);
      ClayDisplay *display = clay_context_get_display (context);

      if (! display ||
          clay_display_get_image (display) != current_image)
        {
          GList *list;

          display = NULL;

          for (list = clay_get_display_iter (current_image->clay);
               list;
               list = g_list_next (list))
            {
              display = list->data;

              if (clay_display_get_image (display) == current_image)
                {
                  clay_context_set_display (context, display);
                  break;
                }

              display = NULL;
            }
        }

      if (display)
        clay_path_tool_start (path_tool, display);
    }

  if (options->edit_mode != CLAY_PATH_MODE_DESIGN)
    g_object_set (options, "path-edit-mode",
                  CLAY_PATH_MODE_DESIGN, NULL);
}

static void
clay_path_tool_to_selection (ClayPathTool *path_tool)
{
  clay_path_tool_to_selection_extended (path_tool, 0);
}

static void
clay_path_tool_to_selection_extended (ClayPathTool    *path_tool,
                                      GdkModifierType  state)
{
  ClayImage *image;

  if (! path_tool->path)
    return;

  image = clay_item_get_image (CLAY_ITEM (path_tool->path));

  clay_item_to_selection (CLAY_ITEM (path_tool->path),
                          clay_modifiers_to_channel_op (state),
                          TRUE, FALSE, 0, 0);
  clay_image_flush (image);
}

static void
clay_path_tool_create_vector_layer (ClayPathTool *path_tool)
{
  ClayImage       *image;
  ClayVectorLayer *layer;
  ClayPathOptions *options = CLAY_PATH_TOOL_GET_OPTIONS (path_tool);
  gboolean         needs_new_path;

  if (! path_tool->current_image)
    return;

  image = path_tool->current_image;

  g_signal_handlers_block_by_func (image,
                                   clay_path_tool_image_selected_layers_changed,
                                   path_tool);

  /* If there's already an active vector layer, or no paths are
   * selected, create a new blank path. */
  needs_new_path = (path_tool->current_vector_layer || ! path_tool->path);
  if (needs_new_path)
    {
      ClayPath *new_path;

      /* An undo group is needed to combine both the path and
       * vector layer creation into a single undo item */
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE,
                                   _("Add Vector Layer"));

      new_path = clay_path_new (image, _("Vector Layer"));

      clay_image_add_path (image, new_path, CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
      clay_path_tool_set_path (path_tool, NULL, new_path);

      g_signal_handlers_disconnect_by_func (options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->fill_options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->stroke_options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
    }

  layer = clay_vector_layer_new (image, path_tool->path,
                                 clay_get_user_context (image->clay));
  g_object_set (layer->options,
                "enable-fill",   options->enable_fill,
                "enable-stroke", options->enable_stroke,
                NULL);
  clay_config_sync (G_OBJECT (options->fill_options),
                    G_OBJECT (layer->options->fill_options), 0);
  clay_config_sync (G_OBJECT (options->stroke_options),
                    G_OBJECT (layer->options->stroke_options), 0);

  clay_image_add_layer (image,
                        CLAY_LAYER (layer),
                        CLAY_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);

  if (needs_new_path)
    clay_image_undo_group_end (image);

  clay_path_tool_set_layer (path_tool, layer);

  clay_vector_layer_refresh (layer);
  clay_image_flush (image);

  g_signal_handlers_unblock_by_func (image,
                                     clay_path_tool_image_selected_layers_changed,
                                     path_tool);
}

static void
clay_path_tool_vector_layer_path_changed (ClayVectorLayerOptions *options,
                                          const GParamSpec       *pspec,
                                          ClayPathTool           *tool)
{
  clay_path_tool_set_path (tool, tool->current_vector_layer, NULL);
}

static void
clay_path_tool_vector_change_notify (GObject          *options,
                                     const GParamSpec *pspec,
                                     ClayVectorLayer  *layer)
{
  ClayImage *image;
  gboolean   needs_update = FALSE;

  image = clay_item_get_image (CLAY_ITEM (layer));

  g_signal_handlers_block_by_func (options,
                                   clay_path_tool_vector_change_notify,
                                   layer);

  if (CLAY_IS_STROKE_OPTIONS (options))
    {
      ClayStrokeOptions *stroke_options = CLAY_STROKE_OPTIONS (options);

      clay_config_sync (G_OBJECT (stroke_options),
                        G_OBJECT (layer->options->stroke_options), 0);

      needs_update = TRUE;
    }
  else if (CLAY_IS_FILL_OPTIONS (options))
    {
      ClayFillOptions *fill_options = CLAY_FILL_OPTIONS (options);

      clay_config_sync (G_OBJECT (fill_options),
                        G_OBJECT (layer->options->fill_options), 0);

      needs_update = TRUE;
    }
  else if (CLAY_IS_PATH_OPTIONS (options))
    {
      ClayPathOptions *path_options = CLAY_PATH_OPTIONS (options);

      g_object_set (layer->options,
                    "enable-fill", path_options->enable_fill,
                    "enable-stroke", path_options->enable_stroke,
                    NULL);

      needs_update = TRUE;
    }

  g_signal_handlers_unblock_by_func (options,
                                     clay_path_tool_vector_change_notify,
                                     layer);

  if (needs_update)
    {
      clay_vector_layer_refresh (layer);
      clay_image_flush (image);
    }
}

static void
clay_path_tool_layer_rasterized (ClayVectorLayer  *layer,
                                 gboolean          is_rasterized,
                                 ClayPathTool     *tool)
{
  if (! is_rasterized && ! CLAY_TOOL (tool)->display)
    {
      GList *current_layers = NULL;

      if (tool->current_image)
        current_layers = clay_image_get_selected_layers (tool->current_image);

      if (current_layers                      &&
          g_list_length (current_layers) == 1 &&
          current_layers->data == layer)
        clay_path_tool_set_layer (tool, layer);
    }
  else if (is_rasterized && CLAY_TOOL (tool)->display)
    {
      clay_path_tool_set_layer (tool, NULL);
    }
}

static void
clay_path_tool_set_layer (ClayPathTool    *path_tool,
                          ClayVectorLayer *vector_layer)
{
  ClayPathOptions        *options;
  ClayVectorLayerOptions *layer_options;

  g_return_if_fail (CLAY_IS_PATH_TOOL (path_tool));

  options = CLAY_PATH_TOOL_GET_OPTIONS (path_tool);

  if (path_tool->current_vector_layer)
    {
      layer_options = clay_vector_layer_get_options (path_tool->current_vector_layer);

      if (layer_options)
        g_signal_handlers_disconnect_by_func (layer_options,
                                              clay_path_tool_vector_layer_path_changed,
                                              path_tool);

      g_signal_handlers_disconnect_by_func (options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->fill_options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);
      g_signal_handlers_disconnect_by_func (options->stroke_options,
                                            clay_path_tool_vector_change_notify,
                                            path_tool->current_vector_layer);

      g_signal_handlers_disconnect_by_func (path_tool->current_vector_layer,
                                            G_CALLBACK (clay_path_tool_layer_rasterized),
                                            path_tool);
    }

  path_tool->current_vector_layer = vector_layer;
  clay_path_tool_set_path (path_tool, vector_layer, NULL);

  if (vector_layer)
    {
      layer_options = clay_vector_layer_get_options (vector_layer);
      g_signal_connect_object (layer_options, "notify::path",
                               G_CALLBACK (clay_path_tool_vector_layer_path_changed),
                               path_tool, 0);

      g_object_set (options,
                    "enable-fill",   vector_layer->options->enable_fill,
                    "enable_stroke", vector_layer->options->enable_stroke,
                    NULL);
      clay_config_sync (G_OBJECT (vector_layer->options->stroke_options),
                        G_OBJECT (options->stroke_options), 0);
      clay_config_sync (G_OBJECT (vector_layer->options->fill_options),
                        G_OBJECT (options->fill_options), 0);

      g_signal_connect_object (options, "notify::enable-fill",
                               G_CALLBACK (clay_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options, "notify::enable-stroke",
                               G_CALLBACK (clay_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options->fill_options, "notify",
                               G_CALLBACK (clay_path_tool_vector_change_notify),
                               vector_layer, 0);
      g_signal_connect_object (options->stroke_options, "notify",
                               G_CALLBACK (clay_path_tool_vector_change_notify),
                               vector_layer, 0);

      g_signal_connect_object (vector_layer, "set-rasterized",
                               G_CALLBACK (clay_path_tool_layer_rasterized),
                               path_tool, 0);
    }

  if (vector_layer)
    gtk_button_set_label (GTK_BUTTON (options->vector_layer_button),
                          _("Create New Vector Layer"));
}

static void
clay_path_tool_confirm_dialog (ClayPathTool    *path_tool,
                               ClayVectorLayer *layer)
{
  ClayTool         *tool    = CLAY_TOOL (path_tool);
  ClayContext      *context = clay_get_user_context (tool->tool_info->clay);
  ClayDisplay      *display = clay_context_get_display (context);
  ClayDisplayShell *shell   = display ? clay_display_get_shell (display) : NULL;
  GtkWidget        *dialog;
  GtkWidget        *vbox;
  GtkWidget        *label;

  if (path_tool->confirm_dialog)
    {
      gtk_window_present (GTK_WINDOW (path_tool->confirm_dialog));
      return;
    }

  dialog = clay_viewable_dialog_new (g_list_prepend (NULL, layer),
                                     CLAY_CONTEXT (clay_tool_get_options (tool)),
                                     _("Confirm Path Editing"),
                                     "clay-path-tool-confirm",
                                     CLAY_ICON_LAYER_VECTOR_LAYER,
                                     _("Confirm Path Editing"),
                                     GTK_WIDGET (shell),
                                     clay_standard_help_func, NULL,

                                     _("Create _New Layer"), RESPONSE_NEW,
                                     _("_Cancel"),           GTK_RESPONSE_CANCEL,
                                     _("_Edit Anyway"),      GTK_RESPONSE_ACCEPT,

                                     NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_NEW,
                                            GTK_RESPONSE_ACCEPT,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (clay_path_tool_confirm_response),
                    path_tool);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox, TRUE);

  label = gtk_label_new (_("The vector layer you picked was rasterized. "
                           "Editing its path will discard any modifications."));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_set_visible (label, TRUE);

  gtk_widget_set_visible (dialog, TRUE);

  path_tool->confirm_dialog = dialog;
  g_signal_connect_swapped (dialog, "destroy",
                            G_CALLBACK (g_nullify_pointer),
                            &path_tool->confirm_dialog);
}

static void
clay_path_tool_confirm_response (ClayViewableDialog *dialog,
                                 gint                response_id,
                                 ClayPathTool       *path_tool)
{
  ClayVectorLayer *layer = dialog->viewables ? dialog->viewables->data : NULL;
  ClayImage       *image;
  ClayPath        *path;

  gtk_widget_destroy (GTK_WIDGET (dialog));

  if (layer && layer->options)
    {
      switch (response_id)
        {
        case RESPONSE_NEW:
          path  = clay_vector_layer_get_path (layer);
          image = clay_item_get_image (CLAY_ITEM (path));
          path  = CLAY_PATH (clay_item_duplicate (CLAY_ITEM (path),
                                                  G_TYPE_FROM_INSTANCE (path)));
          clay_image_add_path (image, path, CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
          clay_path_tool_set_path (path_tool, NULL, path);
          clay_path_tool_create_vector_layer (path_tool);
          break;

        case GTK_RESPONSE_ACCEPT:
          clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));
          clay_path_tool_set_layer (path_tool, layer);
          break;

        default:
          clay_path_tool_set_layer (path_tool, NULL);
          break;
        }
    }
}
