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

#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claychannel-combine.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydrawable-fill.h"
#include "core/claygrouplayer.h"
#include "core/clayimage.h"
#include "core/clayimage-merge.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayitemundo.h"
#include "core/claylayerpropundo.h"
#include "core/claylayer-floating-selection.h"
#include "core/claylayer-new.h"
#include "core/claylink.h"
#include "core/claylinklayer.h"
#include "core/claylist.h"
#include "core/claypickable.h"
#include "core/claypickable-auto-shrink.h"
#include "core/clayrasterizable.h"
#include "core/claytoolinfo.h"
#include "core/clayundostack.h"
#include "core/clayprogress.h"

#include "path/claypath.h"
#include "path/claypath-warp.h"
#include "path/claystroke.h"
#include "path/clayvectorlayer.h"
#include "path/clayvectorlayeroptions.h"

#include "text/claytext.h"
#include "text/claytext-path.h"
#include "text/claytextlayer.h"

#include "widgets/clayaction.h"
#include "widgets/claydock.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayopendialog.h"
#include "widgets/clayprogressdialog.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/clayimagewindow.h"

#include "tools/claypathtool.h"
#include "tools/claytexttool.h"
#include "tools/claytools-utils.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/layer-add-mask-dialog.h"
#include "dialogs/layer-options-dialog.h"
#include "dialogs/resize-dialog.h"
#include "dialogs/scale-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "layers-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   layers_new_callback             (GtkWidget             *dialog,
                                               ClayImage             *image,
                                               ClayLayer             *layer,
                                               ClayContext           *context,
                                               const gchar           *layer_name,
                                               ClayLayerMode          layer_mode,
                                               ClayLayerColorSpace    layer_blend_space,
                                               ClayLayerColorSpace    layer_composite_space,
                                               ClayLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               ClayFillType           layer_fill_type,
                                               ClayLink              *link,
                                               ClayPath              *path,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               ClayColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_visibility,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_edit_attributes_callback (GtkWidget             *dialog,
                                               ClayImage             *image,
                                               ClayLayer             *layer,
                                               ClayContext           *context,
                                               const gchar           *layer_name,
                                               ClayLayerMode          layer_mode,
                                               ClayLayerColorSpace    layer_blend_space,
                                               ClayLayerColorSpace    layer_composite_space,
                                               ClayLayerCompositeMode layer_composite_mode,
                                               gdouble                layer_opacity,
                                               ClayFillType           layer_fill_type,
                                               ClayLink              *link,
                                               ClayPath              *path,
                                               gint                   layer_width,
                                               gint                   layer_height,
                                               gint                   layer_offset_x,
                                               gint                   layer_offset_y,
                                               gboolean               layer_visible,
                                               ClayColorTag           layer_color_tag,
                                               gboolean               layer_lock_pixels,
                                               gboolean               layer_lock_position,
                                               gboolean               layer_lock_visibility,
                                               gboolean               layer_lock_alpha,
                                               gboolean               rename_text_layer,
                                               gpointer               user_data);
static void   layers_add_mask_callback        (GtkWidget             *dialog,
                                               GList                 *layers,
                                               ClayAddMaskType        add_mask_type,
                                               ClayChannel           *channel,
                                               gboolean               invert,
                                               gboolean               edit_mask,
                                               gpointer               user_data);
static void   layers_scale_callback           (GtkWidget             *dialog,
                                               ClayViewable          *viewable,
                                               gint                   width,
                                               gint                   height,
                                               ClayUnit              *unit,
                                               ClayInterpolationType  interpolation,
                                               gdouble                xresolution,
                                               gdouble                yresolution,
                                               ClayUnit              *resolution_unit,
                                               gpointer               user_data);
static void   layers_resize_callback          (GtkWidget             *dialog,
                                               ClayViewable          *viewable,
                                               ClayContext           *context,
                                               gint                   width,
                                               gint                   height,
                                               ClayUnit              *unit,
                                               gint                   offset_x,
                                               gint                   offset_y,
                                               gdouble                unused0,
                                               gdouble                unused1,
                                               ClayUnit              *unused2,
                                               ClayFillType           fill_type,
                                               ClayItemSet            unused3,
                                               gboolean               unused4,
                                               gpointer               data);

static gint   layers_mode_index               (ClayLayerMode          layer_mode,
                                               const ClayLayerMode   *modes,
                                               gint                   n_modes);


/*  private variables  */

static ClayUnit              *layer_resize_unit   = NULL;
static ClayUnit              *layer_scale_unit    = NULL;
static ClayInterpolationType  layer_scale_interp  = -1;


/*  public functions  */

void
layers_edit_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GtkWidget *widget;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  if (clay_item_is_text_layer (CLAY_ITEM (layers->data)))
    layers_edit_text_cmd_callback (action, value, data);
  else if (clay_item_is_vector_layer (CLAY_ITEM (layers->data)))
    layers_edit_vector_cmd_callback (action, value, data);
  else
    layers_edit_attributes_cmd_callback (action, value, data);
}

void
layers_edit_text_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  ClayLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  ClayTool  *active_tool;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;
  g_return_if_fail (clay_item_is_text_layer (CLAY_ITEM (layer)));

  active_tool = tool_manager_get_active (image->clay);

  if (! CLAY_IS_TEXT_TOOL (active_tool))
    {
      ClayToolInfo *tool_info = clay_get_tool_info (image->clay,
                                                    "clay-text-tool");

      if (CLAY_IS_TOOL_INFO (tool_info))
        {
          clay_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->clay);
        }
    }

  if (CLAY_IS_TEXT_TOOL (active_tool))
    {
      if (clay_text_tool_set_layer (CLAY_TEXT_TOOL (active_tool), layer))
        {
          ClayDisplayShell *shell;

          shell = clay_display_get_shell (active_tool->display);
          gtk_widget_grab_focus (shell->canvas);
        }
    }
}

void
layers_edit_vector_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage *image;
  ClayLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  ClayTool  *active_tool;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;

  if (! clay_item_is_vector_layer (CLAY_ITEM (layer)))
    {
      layers_edit_attributes_cmd_callback (action, value, data);
      return;
    }

  active_tool = tool_manager_get_active (image->clay);

  if (! CLAY_IS_PATH_TOOL (active_tool))
    {
      ClayToolInfo *tool_info;

      tool_info = (ClayToolInfo *)
        clay_container_get_child_by_name (image->clay->tool_info_list,
                                          "clay-path-tool");

      if (CLAY_IS_TOOL_INFO (tool_info))
        {
          clay_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->clay);
        }
    }

  if (CLAY_IS_PATH_TOOL (active_tool))
    clay_path_tool_set_path (CLAY_PATH_TOOL (active_tool), CLAY_VECTOR_LAYER (layer), NULL);
}

void
layers_edit_attributes_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  ClayLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (g_list_length (layers) != 1)
    return;

  layer = layers->data;

  if (clay_layer_is_floating_sel (layer))
    return;

#define EDIT_DIALOG_KEY "clay-layer-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      ClayItem *item = CLAY_ITEM (layer);

      dialog = layer_options_dialog_new (clay_item_get_image (CLAY_ITEM (layer)),
                                         layer,
                                         action_data_get_context (data),
                                         widget,
                                         _("Layer Attributes"),
                                         "clay-layer-edit",
                                         CLAY_ICON_EDIT,
                                         _("Edit Layer Attributes"),
                                         CLAY_HELP_LAYER_EDIT,
                                         clay_object_get_name (layer),
                                         clay_layer_get_mode (layer),
                                         clay_layer_get_blend_space (layer),
                                         clay_layer_get_composite_space (layer),
                                         clay_layer_get_composite_mode (layer),
                                         clay_layer_get_opacity (layer),
                                         0 /* unused */,
                                         clay_item_get_visible (item),
                                         clay_item_get_color_tag (item),
                                         clay_item_get_lock_content (item),
                                         clay_item_get_lock_position (item),
                                         clay_item_get_lock_visibility (item),
                                         clay_layer_get_lock_alpha (layer),
                                         layers_edit_attributes_callback,
                                         NULL);

      dialogs_attach_dialog (G_OBJECT (layer), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  ClayLayer *floating_sel;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if ((floating_sel = clay_image_get_floating_selection (image)))
    {
      GError *error = NULL;

      if (! floating_sel_to_layer (floating_sel, &error))
        {
          clay_message_literal (image->clay,
                                G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
          return;
        }

      clay_image_flush (image);
      return;
    }

#define NEW_DIALOG_KEY "clay-layer-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config     = CLAY_DIALOG_CONFIG (image->clay->config);
      const gchar      *title;
      gchar            *desc;
      gint              n_layers;
      ClayLayerMode     layer_mode = config->layer_new_mode;

      n_layers = g_list_length (clay_image_get_selected_layers (image));
      title = ngettext ("New Layer", "New Layers", n_layers > 0 ? n_layers : 1);
      desc  = ngettext ("Create a New Layer", "Create %d New Layers", n_layers > 0 ? n_layers : 1);
      desc  = g_strdup_printf (desc, n_layers > 0 ? n_layers : 1);

      if (layer_mode == CLAY_LAYER_MODE_NORMAL ||
          layer_mode == CLAY_LAYER_MODE_NORMAL_LEGACY)
        {
          layer_mode = clay_image_get_default_new_layer_mode (image);
        }

      dialog = layer_options_dialog_new (image, NULL,
                                         action_data_get_context (data),
                                         widget,
                                         title,
                                         "clay-layer-new",
                                         CLAY_ICON_LAYER,
                                         desc,
                                         CLAY_HELP_LAYER_NEW,
                                         config->layer_new_name,
                                         layer_mode,
                                         config->layer_new_blend_space,
                                         config->layer_new_composite_space,
                                         config->layer_new_composite_mode,
                                         config->layer_new_opacity,
                                         config->layer_new_fill_type,
                                         TRUE,
                                         CLAY_COLOR_TAG_NONE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         layers_new_callback,
                                         NULL);
      g_free (desc);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_new_last_vals_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayImage        *image;
  GtkWidget        *widget;
  ClayLayer        *layer;
  ClayDialogConfig *config;
  GList            *layers;
  GList            *new_layers = NULL;
  GList            *iter;
  ClayLayerMode     layer_mode;
  gint              n_layers;
  gboolean          run_once;

  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  /*  If there is a floating selection, the new command transforms
   *  the current fs into a new layer
   */
  if (clay_image_get_floating_selection (image))
    {
      layers_new_cmd_callback (action, value, data);
      return;
    }

  layer_mode = config->layer_new_mode;

  if (layer_mode == CLAY_LAYER_MODE_NORMAL ||
      layer_mode == CLAY_LAYER_MODE_NORMAL_LEGACY)
    {
      layer_mode = clay_image_get_default_new_layer_mode (image);
    }

  layers   = clay_image_get_selected_layers (image);
  layers   = g_list_copy (layers);
  n_layers = g_list_length (layers);
  run_once = (n_layers == 0);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));

  for (iter = layers; iter || run_once ; iter = iter ? iter->next : NULL)
    {
      ClayLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (clay_viewable_get_children (CLAY_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = CLAY_LAYER (clay_item_get_parent (iter->data));
              position = clay_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent   = NULL;
          position = -1;
        }
      layer = clay_layer_new (image,
                              clay_image_get_width  (image),
                              clay_image_get_height (image),
                              clay_image_get_layer_format (image, TRUE),
                              config->layer_new_name,
                              config->layer_new_opacity,
                              layer_mode);

      clay_drawable_fill (CLAY_DRAWABLE (layer),
                          action_data_get_context (data),
                          config->layer_new_fill_type);
      clay_layer_set_blend_space (layer,
                                  config->layer_new_blend_space, FALSE);
      clay_layer_set_composite_space (layer,
                                      config->layer_new_composite_space, FALSE);
      clay_layer_set_composite_mode (layer,
                                     config->layer_new_composite_mode, FALSE);

      clay_image_add_layer (image, layer, parent, position, TRUE);
      new_layers = g_list_prepend (new_layers, layer);
    }

  clay_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_new_from_visible_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage        *image;
  ClayDisplayShell *shell;
  ClayLayer        *layer;
  ClayPickable     *pickable;
  ClayColorProfile *profile;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  pickable = clay_display_shell_get_canvas_pickable (shell);

  clay_pickable_flush (pickable);

  profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));

  layer = clay_layer_new_from_gegl_buffer (clay_pickable_get_buffer (pickable),
                                           image,
                                           clay_image_get_layer_format (image,
                                                                        TRUE),
                                           _("Visible"),
                                           CLAY_OPACITY_OPAQUE,
                                           clay_image_get_default_new_layer_mode (image),
                                           profile);

  clay_image_add_layer (image, layer, CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);
}

void
layers_new_group_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  ClayLayer *group;
  ClayLayer *parent     = NULL;
  GList     *new_layers = NULL;
  GList     *layers;
  GList     *iter;
  gint       position   = 0;
  gint       n_layers;

  return_if_no_image (image, data);

  layers   = clay_image_get_selected_layers (image);
  layers   = g_list_copy (layers);
  n_layers = g_list_length (layers);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_ADD,
                               (n_layers > 1) ? _("Group selected layers") :
                                                _("New layer group"));

  group = clay_group_layer_new (image);
  /* Insert at top of selection */
  if (layers && layers->data)
    {
      parent   = CLAY_LAYER (clay_item_get_parent (layers->data));
      position = clay_item_get_index (layers->data);
    }
  clay_image_add_layer (image, group, parent, position, TRUE);
  new_layers = g_list_prepend (new_layers, group);

  /* For now, we only group selected layers in the new layer group if
   * more than one is selected. Otherwise, we just add a new empty
   * layer group to the image */
  if (n_layers > 1)
    {
      position = 0;
      for (iter = layers; iter; iter = iter ? iter->next : NULL)
        {
          clay_image_reorder_item (image, CLAY_ITEM (iter->data),
                                   CLAY_ITEM (group), position, TRUE, NULL);
          position++;
        }
    }
  clay_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_select_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage            *image;
  GList                *new_layers = NULL;
  GList                *layers;
  GList                *iter;
  ClayActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  layers   = clay_image_get_selected_layers (image);
  run_once = (g_list_length (layers) == 0);

  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      ClayLayer     *new_layer;
      ClayContainer *container;

      if (iter)
        {
          container = clay_item_get_container (CLAY_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = clay_image_get_layers (image);
          run_once  = FALSE;
        }
      new_layer = (ClayLayer *) action_select_object (select_type,
                                                      container,
                                                      iter ? iter->data : NULL);
      if (new_layer)
        new_layers = g_list_prepend (new_layers, new_layer);
    }

  if (new_layers)
    {
      clay_image_set_selected_layers (image, new_layers);
      clay_image_flush (image);
    }

  g_list_free (new_layers);
}

void
layers_raise_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        {
          raised_layers = g_list_prepend (raised_layers, iter->data);
        }
      else
        {
          g_list_free (raised_layers);
          return;
        }
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer",
                                         "Raise Layers",
                                         g_list_length (raised_layers)));

  raised_layers = g_list_reverse (raised_layers);
  for (iter = raised_layers; iter; iter = iter->next)
    clay_image_raise_item (image, iter->data, NULL);
  g_list_free (raised_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_raise_to_top_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GList     *raised_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        raised_layers = g_list_prepend (raised_layers, iter->data);
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Layer to Top",
                                         "Raise Layers to Top",
                                         g_list_length (raised_layers)));

  for (iter = raised_layers; iter; iter = iter->next)
    clay_image_raise_item_to_top (image, iter->data);

  g_list_free (raised_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_lower_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        {
          lowered_layers = g_list_prepend (lowered_layers, iter->data);
        }
      else
        {
          g_list_free (lowered_layers);
          return;
        }
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer",
                                         "Lower Layers",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    clay_image_lower_item (image, iter->data, NULL);

  g_list_free (lowered_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_lower_to_bottom_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GList     *lowered_layers = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_layers = g_list_prepend (lowered_layers, iter->data);
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Layer to Bottom",
                                         "Lower Layers to Bottom",
                                         g_list_length (lowered_layers)));

  for (iter = lowered_layers; iter; iter = iter->next)
    clay_image_lower_item_to_bottom (image, iter->data);

  g_list_free (lowered_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_duplicate_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *new_layers = NULL;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  layers = g_list_copy (layers);
  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_ADD,
                               _("Duplicate layers"));
  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *new_layer;

      new_layer = CLAY_LAYER (clay_item_duplicate (CLAY_ITEM (iter->data),
                                                   G_TYPE_FROM_INSTANCE (iter->data)));

      /*  use the actual parent here, not CLAY_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      clay_image_add_layer (image, new_layer,
                            clay_layer_get_parent (iter->data),
                            clay_item_get_index (iter->data),
                            TRUE);
      clay_drawable_enable_resize_undo (CLAY_DRAWABLE (new_layer));
      new_layers = g_list_prepend (new_layers, new_layer);
    }

  clay_image_set_selected_layers (image, new_layers);
  g_list_free (layers);
  g_list_free (new_layers);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_anchor_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) == 1 &&
      clay_layer_is_floating_sel (layers->data))
    {
      floating_sel_anchor (layers->data);
      clay_image_flush (image);
    }
}

void
layers_merge_down_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage   *image;
  GList       *layers;
  ClayDisplay *display;
  ClayItem    *item  = NULL;
  GError      *error = NULL;

  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  layers = clay_image_merge_down (image, layers, action_data_get_context (data),
                                  CLAY_EXPAND_AS_NECESSARY,
                                  CLAY_PROGRESS (display),
                                  &item, &error);

  if (error)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (display), CLAY_MESSAGE_WARNING,
                            error->message);
      if (item)
        clay_tools_blink_item (image->clay, item);
      g_clear_error (&error);
      return;
    }

  clay_image_set_selected_layers (image, layers);
  g_list_free (layers);

  clay_image_flush (image);
}

void
layers_merge_group_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *merge_layers = NULL;
  GList     *iter;
  GtkWidget *widget;

  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_viewable_get_children (CLAY_VIEWABLE (iter->data)))
        {
          GList *iter2;

          for (iter2 = layers; iter2; iter2 = iter2->next)
            {
              if (iter->data == iter2->data)
                continue;

              /* Do not merge a layer when we already merge one of its
               * ancestors.
               */
              if (clay_viewable_is_ancestor (iter2->data, iter->data))
                break;

              /* Do not merge a layer which has a little sister (same
               * parent and smaller index) or a little cousin (one of
               * its ancestors is a little sister) of a pass-through
               * group layer.
               * These will be rendered and merged through the
               * pass-through by definition.
               */
              if (clay_viewable_get_children (CLAY_VIEWABLE (iter2->data)) &&
                  clay_layer_get_mode (iter2->data) == CLAY_LAYER_MODE_PASS_THROUGH)
                {
                  ClayLayer *pass_through_parent = clay_layer_get_parent (iter2->data);
                  ClayLayer *cousin              = iter->data;
                  gboolean   ignore = FALSE;

                  do
                    {
                      ClayLayer *cousin_parent = clay_layer_get_parent (cousin);

                      if (pass_through_parent == cousin_parent &&
                          clay_item_get_index (CLAY_ITEM (iter2->data)) < clay_item_get_index (CLAY_ITEM (cousin)))
                        {
                          ignore = TRUE;
                          break;
                        }

                      cousin = cousin_parent;
                    }
                  while (cousin != NULL);

                  if (ignore)
                    break;
                }
            }

          if (iter2 == NULL)
            merge_layers = g_list_prepend (merge_layers, iter->data);
        }
    }

  for (iter = merge_layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mode (iter->data) == CLAY_LAYER_MODE_PASS_THROUGH)
        {
          clay_message_literal (image->clay, G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                                _("Cannot merge a pass through layer group."));
          clay_tools_blink_item (image->clay, CLAY_ITEM (iter->data));
          g_list_free (merge_layers);
          return;
        }
    }

  if (g_list_length (merge_layers) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Merge %d Layer Groups"),
                                   g_list_length (merge_layers));
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_LAYERS_MERGE,
                                   undo_name);
      g_free (undo_name);
    }

  for (iter = merge_layers; iter; iter = iter->next)
    clay_image_merge_group_layer (image, CLAY_GROUP_LAYER (iter->data));

  if (g_list_length (merge_layers) > 1)
    clay_image_undo_group_end (image);

  g_list_free (merge_layers);

  clay_image_flush (image);
}

void
layers_delete_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *removed_layers;
  GList     *iter;
  GList     *iter2;

  return_if_no_image (image, data);

  layers = clay_image_get_selected_layers (image);

  /*TODO: we should have a failsafe to determine when we are going to
   * delete all layers (i.e. all layers of first level at least) and
   * forbid it. */

  /* Copy of the original selection. */
  removed_layers = g_list_copy (layers);

  /* Removing children layers (they will be removed anyway by removing
   * the parent).
   */
  for (iter = removed_layers; iter; iter = iter->next)
    {
      for (iter2 = removed_layers; iter2; iter2 = iter2->next)
        {
          if (iter->data != iter2->data &&
              clay_viewable_is_ancestor (iter2->data, iter->data))
            {
              removed_layers = g_list_delete_link (removed_layers, iter);
              iter = removed_layers;
              break;
            }
        }
    }

  if (g_list_length (removed_layers) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Remove %d Layers"),
                                   g_list_length (removed_layers));
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = removed_layers; iter; iter = iter->next)
    clay_image_remove_layer (image, iter->data, TRUE, NULL);

  if (g_list_length (removed_layers) > 1)
    clay_image_undo_group_end (image);

  g_list_free (removed_layers);

  clay_image_flush (image);
}

void
layers_rasterize_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;

  return_if_no_layers (image, layers, data);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                               _("Rasterize Layers"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (CLAY_IS_RASTERIZABLE (iter->data) && ! clay_rasterizable_is_rasterized (iter->data))
        clay_rasterizable_rasterize (CLAY_RASTERIZABLE (iter->data), TRUE);
    }

  clay_image_undo_group_end (image);
}

void
layers_revert_rasterize_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;

  return_if_no_layers (image, layers, data);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                               _("Revert Rasterize"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (CLAY_IS_RASTERIZABLE (iter->data) && clay_rasterizable_is_rasterized (iter->data))
        clay_rasterizable_restore (CLAY_RASTERIZABLE (iter->data));
    }

  clay_image_undo_group_end (image);
}

void
layers_text_to_path_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   path_added = FALSE;
  return_if_no_layers (image, layers, data);

  /* TODO: have the proper undo group. */
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PATHS_IMPORT,
                               _("Add Paths"));

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayer *layer = iter->data;

      if (CLAY_IS_TEXT_LAYER (layer))
        {
          ClayPath *path;
          gint      x, y;

          path = clay_text_path_new (image, CLAY_TEXT_LAYER (layer)->text);

          clay_item_get_offset (CLAY_ITEM (layer), &x, &y);
          clay_item_translate (CLAY_ITEM (path), x, y, FALSE);

          clay_image_add_path (image, path,
                               CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
          path_added = TRUE;
        }
    }

  clay_image_undo_group_end (image);

  if (path_added)
    clay_image_flush (image);
}

void
layers_text_along_path_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage   *image;
  GList       *layers;
  GList       *paths;
  ClayLayer   *layer;
  ClayPath    *path;
  return_if_no_layers (image, layers, data);
  return_if_no_paths (image, paths, data);

  if (g_list_length (layers) != 1 || g_list_length (paths) != 1)
    return;

  layer = layers->data;
  path  = paths->data;
  if (CLAY_IS_TEXT_LAYER (layer))
    {
      gdouble   box_width;
      gdouble   box_height;
      ClayPath *new_path;
      gdouble   offset;

      box_width  = clay_item_get_width  (CLAY_ITEM (layer));
      box_height = clay_item_get_height (CLAY_ITEM (layer));

      new_path = clay_text_path_new (image, CLAY_TEXT_LAYER (layer)->text);

      offset = 0;
      switch (CLAY_TEXT_LAYER (layer)->text->base_dir)
        {
        case CLAY_TEXT_DIRECTION_LTR:
        case CLAY_TEXT_DIRECTION_RTL:
          offset = 0.5 * box_height;
          break;
        case CLAY_TEXT_DIRECTION_TTB_RTL:
        case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
        case CLAY_TEXT_DIRECTION_TTB_LTR:
        case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
          {
            ClayStroke *stroke = NULL;

            while ((stroke = clay_path_stroke_get_next (new_path, stroke)))
              {
                clay_stroke_rotate (stroke, 0, 0, 270);
                clay_stroke_translate (stroke, 0, box_width);
              }
          }
          offset = 0.5 * box_width;
          break;
        }

      clay_path_warp_path (path, new_path, offset);

      clay_item_set_visible (CLAY_ITEM (new_path), TRUE, FALSE);

      clay_image_add_path (image, new_path,
                           CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
      clay_image_flush (image);
    }
}

void
layers_resize_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  ClayLayer *layer;
  GList     *layers;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define RESIZE_DIALOG_KEY "clay-resize-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config  = CLAY_DIALOG_CONFIG (image->clay->config);
      ClayDisplay      *display = NULL;

      if (CLAY_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_resize_unit == NULL)
        layer_resize_unit = clay_unit_pixel ();

      if (layer_resize_unit != clay_unit_percent () && display)
        layer_resize_unit = clay_display_get_shell (display)->unit;

      dialog = resize_dialog_new (CLAY_VIEWABLE (layer),
                                  action_data_get_context (data),
                                  _("Set Layer Boundary Size"),
                                  "clay-layer-resize",
                                  widget,
                                  clay_standard_help_func,
                                  CLAY_HELP_LAYER_RESIZE,
                                  layer_resize_unit,
                                  config->layer_resize_fill_type,
                                  CLAY_ITEM_SET_NONE,
                                  FALSE,
                                  layers_resize_callback,
                                  NULL);

      dialogs_attach_dialog (G_OBJECT (layer), RESIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_resize_to_image_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  if (g_list_length (layers) > 1)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_RESIZE,
                                 _("Layers to Image Size"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (CLAY_IS_RASTERIZABLE (iter->data) &&
          ! clay_rasterizable_is_rasterized (iter->data))
        continue;

      clay_layer_resize_to_image (iter->data,
                                  action_data_get_context (data),
                                  CLAY_FILL_TRANSPARENT);
    }

  if (g_list_length (layers) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_scale_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  ClayLayer *layer;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

#define SCALE_DIALOG_KEY "clay-scale-dialog"

  g_return_if_fail (g_list_length (layers) == 1);

  layer = layers->data;

  dialog = dialogs_get_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      ClayDisplay *display = NULL;

      if (CLAY_IS_IMAGE_WINDOW (data))
        display = action_data_get_display (data);

      if (layer_scale_unit == NULL)
        layer_scale_unit = clay_unit_pixel ();;

      if (layer_scale_unit != clay_unit_percent () && display)
        layer_scale_unit = clay_display_get_shell (display)->unit;

      if (layer_scale_interp == -1)
        layer_scale_interp = image->clay->config->interpolation_type;

      dialog = scale_dialog_new (CLAY_VIEWABLE (layer),
                                 action_data_get_context (data),
                                 _("Scale Layer"), "clay-layer-scale",
                                 widget,
                                 clay_standard_help_func, CLAY_HELP_LAYER_SCALE,
                                 layer_scale_unit,
                                 layer_scale_interp,
                                 layers_scale_callback,
                                 display);

      dialogs_attach_dialog (G_OBJECT (layer), SCALE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_crop_to_selection_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  gchar     *desc;
  gint       x, y;
  gint       width, height;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  desc = g_strdup_printf (ngettext ("Crop Layer to Selection",
                                    "Crop %d Layers to Selection",
                                    g_list_length (layers)),
                          g_list_length (layers));
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      gint off_x, off_y;

      if (CLAY_IS_RASTERIZABLE (iter->data) &&
          ! clay_rasterizable_is_rasterized (iter->data))
        continue;

      clay_item_get_offset (CLAY_ITEM (iter->data), &off_x, &off_y);
      off_x -= x;
      off_y -= y;

      clay_item_resize (CLAY_ITEM (iter->data),
                        action_data_get_context (data), CLAY_FILL_TRANSPARENT,
                        width, height, off_x, off_y);
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_crop_to_content_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  gchar     *desc;
  gint       x, y;
  gint       width, height;
  gint       n_croppable = 0;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  for (iter = layers; iter; iter = iter->next)
    {
      switch (clay_pickable_auto_shrink (CLAY_PICKABLE (iter->data),
                                         0, 0,
                                         clay_item_get_width  (iter->data),
                                         clay_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case CLAY_AUTO_SHRINK_SHRINK:
          n_croppable++;
          break;

        case CLAY_AUTO_SHRINK_EMPTY:
          /* Cannot crop because the layer has no content. */
        case CLAY_AUTO_SHRINK_UNSHRINKABLE:
          /* Cannot crop because the active layer is already cropped to
           * its content. */
          break;
        }
    }

  if (n_croppable == 0)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_INFO,
                            _("Cannot crop because none of the selected"
                              " layers have content or they are already"
                              " cropped to their content."));
      return;
    }

  desc = g_strdup_printf (ngettext ("Crop Layer to Content",
                                    "Crop %d Layers to Content",
                                    n_croppable),
                          n_croppable);
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_RESIZE, desc);
  g_free (desc);

  for (iter = layers; iter; iter = iter->next)
    {
      switch (clay_pickable_auto_shrink (CLAY_PICKABLE (iter->data),
                                         0, 0,
                                         clay_item_get_width  (iter->data),
                                         clay_item_get_height (iter->data),
                                         &x, &y, &width, &height))
        {
        case CLAY_AUTO_SHRINK_SHRINK:
          clay_item_resize (iter->data,
                            action_data_get_context (data), CLAY_FILL_TRANSPARENT,
                            width, height, -x, -y);

          break;
        default:
          break;
        }
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_mask_add_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  GtkWidget *widget;
  GtkWidget *dialog;
  GList     *update_layers = NULL;
  gint       n_channels = 0;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  for (iter = layers; iter; iter = iter->next)
    {
      g_return_if_fail (CLAY_IS_LAYER (iter->data));

      if (! clay_layer_get_mask (iter->data))
        {
          update_layers = g_list_prepend (update_layers, iter->data);
          n_channels++;
        }
    }
  if (n_channels == 0)
    /* No layers or they all have masks already. */
    return;

#define ADD_MASK_DIALOG_KEY "clay-add-mask-dialog"

  for (iter = update_layers; iter; iter = iter->next)
    {
      dialog = dialogs_get_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY);
      if (dialog)
        break;
    }

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      dialog = layer_add_mask_dialog_new (update_layers, action_data_get_context (data),
                                          widget,
                                          config->layer_add_mask_type,
                                          config->layer_add_mask_invert,
                                          config->layer_add_mask_edit_mask,
                                          layers_add_mask_callback,
                                          NULL);

      for (iter = update_layers; iter; iter = iter->next)
        dialogs_attach_dialog (G_OBJECT (iter->data), ADD_MASK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
layers_mask_add_last_vals_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayImage        *image;
  GList            *layers;
  GList            *iter;
  GtkWidget        *widget;
  ClayDialogConfig *config;
  ClayChannel      *channel = NULL;
  ClayLayerMask    *mask;
  return_if_no_layers (image, layers, data);
  return_if_no_widget (widget, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  if (config->layer_add_mask_type == CLAY_ADD_MASK_CHANNEL)
    {
      GList *selected_channels;

      selected_channels = clay_image_get_selected_channels (image);

      if (selected_channels)
        {
          channel = selected_channels->data;
        }
      else
        {
          ClayContainer *channels = clay_image_get_channels (image);

          channel = CLAY_CHANNEL (clay_container_get_first_child (channels));
        }

      if (! channel)
        {
          layers_mask_add_cmd_callback (action, value, data);
          return;
        }
    }

  for (iter = layers; iter; iter = iter->next)
    {
      if (! clay_layer_get_mask (iter->data))
        break;
    }
  if (iter == NULL)
    /* No layers or they all have masks already. */
    return;

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));
  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        continue;

      mask = clay_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        clay_channel_invert (CLAY_CHANNEL (mask), FALSE);

      clay_layer_add_mask (iter->data, mask,
                           config->layer_add_mask_edit_mask,
                           TRUE, NULL);
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_mask_apply_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayMaskApplyMode  mode;
  ClayImage         *image;
  GList             *layers;
  GList             *iter;
  gchar             *undo_text = NULL;
  ClayUndoType       undo_type = CLAY_UNDO_GROUP_NONE;

  return_if_no_layers (image, layers, data);

  mode = (ClayMaskApplyMode) g_variant_get_int32 (value);
  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data)         &&
          (mode != CLAY_MASK_APPLY ||
           (! clay_item_is_rasterizable (iter->data)                  &&
            ! clay_viewable_get_children (CLAY_VIEWABLE (iter->data)) &&
            ! clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL))))
        break;
    }

  if (iter == NULL)
    /* No layers or none have applicable masks. */
    return;

  switch (mode)
    {
    case CLAY_MASK_APPLY:
      undo_type = CLAY_UNDO_GROUP_MASK;
      undo_text = _("Apply Layer Masks");
      break;
    case CLAY_MASK_DISCARD:
      undo_type = CLAY_UNDO_GROUP_MASK;
      undo_text = _("Delete Layer Masks");
      break;
    default:
      g_warning ("%s: unhandled ClayMaskApplyMode %d\n",
                 G_STRFUNC, mode);
      break;
    }

  if (undo_type != CLAY_UNDO_GROUP_NONE)
    clay_image_undo_group_start (image, undo_type, undo_text);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        {
          if (mode == CLAY_MASK_APPLY &&
              (clay_item_is_rasterizable (CLAY_ITEM (iter->data))      ||
               clay_viewable_get_children (CLAY_VIEWABLE (iter->data)) ||
               clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL)))
            /* Layer groups, rasterizable items and content-locked items
             * cannot apply masks.
             */
            continue;

          clay_layer_apply_mask (iter->data, mode, TRUE);
        }
    }

  if (undo_type != CLAY_UNDO_GROUP_NONE)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_mask_edit_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active = g_variant_get_boolean (value);
  return_if_no_layers (image, layers, data);

  /* Multiple-layer selection cannot edit masks. */
  active = active && (g_list_length (layers) == 1);

  for (iter = layers; iter; iter = iter->next)
    if (clay_layer_get_mask (iter->data))
      clay_layer_set_edit_mask (iter->data, active);

  clay_image_flush (image);
}

void
layers_mask_show_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active  = g_variant_get_boolean (value);
  gint       n_masks = 0;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        {
          if (active && clay_layer_get_show_mask (iter->data))
            {
              /* if switching "show mask" on, and any selected layer's
               * mask is already visible, bail out because that's
               * exactly the logic we use in the ui for multiple
               * visible layer masks.
               */
              return;
            }

          if (clay_layer_get_show_mask (iter->data) != active)
            n_masks++;
        }
    }

  if (n_masks == 0)
    return;

  if (n_masks > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_LAYER_ADD,
                                 _("Show Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        clay_layer_set_show_mask (iter->data, active, TRUE);
    }

  if (n_masks > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_mask_disable_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   active  = g_variant_get_boolean (value);
  gint       n_masks = 0;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        {
          if (active && ! clay_layer_get_apply_mask (iter->data))
            {
              /* if switching "disable mask" on, and any selected
               * layer's mask is already disabled, bail out because
               * that's exactly the logic we use in the ui for multiple
               * disabled layer masks.
               */
              return;
            }

          if ((! clay_layer_get_apply_mask (iter->data)) != active)
            n_masks++;
        }
    }

  if (n_masks == 0)
    return;

  if (n_masks > 1)
    clay_image_undo_group_start (image,
                                 CLAY_UNDO_GROUP_LAYER_ADD,
                                 _("Disable Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        clay_layer_set_apply_mask (iter->data, ! active, TRUE);
    }

  if (n_masks > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_mask_to_selection_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage     *image;
  GList         *layers;
  GList         *iter;
  GList         *masks = NULL;
  return_if_no_layers (image, layers, data);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_get_mask (iter->data))
        masks = g_list_prepend (masks, clay_layer_get_mask (iter->data));
    }

  if (masks)
    {
      ClayChannelOps operation = (ClayChannelOps) g_variant_get_int32 (value);

      switch (operation)
        {
        case CLAY_CHANNEL_OP_REPLACE:
          clay_channel_push_undo (clay_image_get_mask (image),
                                  C_("undo-type", "Masks to Selection"));
          break;
        case CLAY_CHANNEL_OP_ADD:
          clay_channel_push_undo (clay_image_get_mask (image),
                                  C_("undo-type", "Add Masks to Selection"));
          break;
        case CLAY_CHANNEL_OP_SUBTRACT:
          clay_channel_push_undo (clay_image_get_mask (image),
                                  C_("undo-type", "Subtract Masks from Selection"));
          break;
        case CLAY_CHANNEL_OP_INTERSECT:
          clay_channel_push_undo (clay_image_get_mask (image),
                                  C_("undo-type", "Intersect Masks with Selection"));
          break;
        }

      clay_channel_combine_items (clay_image_get_mask (image),
                                  masks, operation);
      g_list_free (masks);

      clay_image_flush (image);
    }
}

void
layers_alpha_add_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Add Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (! clay_drawable_has_alpha (iter->data))
      clay_layer_add_alpha (iter->data);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_alpha_remove_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  return_if_no_layers (image, layers, data);

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_ADD_ALPHA,
                               _("Remove Alpha Channel"));

  for (iter = layers; iter; iter = iter->next)
    if (clay_drawable_has_alpha (iter->data))
      clay_layer_remove_alpha (iter->data, action_data_get_context (data));

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_alpha_to_selection_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayImage      *image;
  ClayDisplay    *display;
  GList          *layers;
  ClayChannelOps  operation;
  return_if_no_layers (image, layers, data);
  return_if_no_display (display, data);

  operation = (ClayChannelOps) g_variant_get_int32 (value);

  switch (operation)
    {
    case CLAY_CHANNEL_OP_REPLACE:
      clay_channel_push_undo (clay_image_get_mask (image),
                              C_("undo-type", "Alpha to Selection"));
      break;
    case CLAY_CHANNEL_OP_ADD:
      clay_channel_push_undo (clay_image_get_mask (image),
                              C_("undo-type", "Add Alpha to Selection"));
      break;
    case CLAY_CHANNEL_OP_SUBTRACT:
      clay_channel_push_undo (clay_image_get_mask (image),
                              C_("undo-type", "Subtract Alpha from Selection"));
      break;
    case CLAY_CHANNEL_OP_INTERSECT:
      clay_channel_push_undo (clay_image_get_mask (image),
                              C_("undo-type", "Intersect Alpha with Selection"));
      break;
    }

  clay_channel_combine_items (clay_image_get_mask (image),
                              layers, operation);

  clay_image_flush (image);

  if (clay_channel_is_empty (clay_image_get_mask (image)))
    {
      clay_message_literal (image->clay, G_OBJECT (display),
                            CLAY_MESSAGE_WARNING,
                            _("Empty Selection"));
    }
}

void
layers_opacity_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage            *image;
  GList                *layers;
  GList                *iter;
  gdouble               opacity;
  ClayUndo             *undo;
  ClayActionSelectType  select_type;
  gboolean        push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);
  if (g_list_length (layers) == 1)
    {
      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_LAYER_OPACITY);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      opacity = action_select_value (select_type,
                                     clay_layer_get_opacity (iter->data),
                                     0.0, 1.0, 1.0,
                                     1.0 / 255.0, 0.01, 0.1, 0.0, FALSE);
      clay_layer_set_opacity (iter->data, opacity, push_undo);
    }

  if (g_list_length (layers) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_mode_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage            *image;
  GList                *layers;
  GList                *iter;
  ClayActionSelectType  select_type;
  gboolean              push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  if (g_list_length (layers) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_LAYER_MODE);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (layers->data))
        push_undo = FALSE;
    }

  if (g_list_length (layers) > 1)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_OPACITY,
                                 _("Set layers opacity"));

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayerMode *modes;
      gint           n_modes;
      ClayLayerMode  layer_mode;
      gint           index;

      layer_mode = clay_layer_get_mode (iter->data);

      modes = clay_layer_mode_get_context_array (layer_mode,
                                                 CLAY_LAYER_MODE_CONTEXT_LAYER,
                                                 &n_modes);
      index = layers_mode_index (layer_mode, modes, n_modes);
      index = action_select_value (select_type,
                                   index, 0, n_modes - 1, 0,
                                   0.0, 1.0, 1.0, 0.0, FALSE);
      layer_mode = modes[index];
      g_free (modes);

      clay_layer_set_mode (iter->data, layer_mode, push_undo);
    }

  if (g_list_length (layers) > 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
layers_blend_space_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  ClayLayerColorSpace  blend_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  blend_space = (ClayLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayerMode mode;

      mode = clay_layer_get_mode (iter->data);
      if (clay_layer_mode_is_blend_space_mutable (mode) &&
          blend_space != clay_layer_get_blend_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_LAYER_PROP_UNDO,
                                           CLAY_UNDO_LAYER_MODE);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' blend space"));

      for (iter = update_layers; iter; iter = iter->next)
        clay_layer_set_blend_space (iter->data, blend_space, push_undo);

      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_end (image);

      g_list_free (update_layers);

      clay_image_flush (image);
    }
}

void
layers_composite_space_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage           *image;
  GList               *layers;
  GList               *update_layers = NULL;
  GList               *iter;
  ClayLayerColorSpace  composite_space;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_space = (ClayLayerColorSpace) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayerMode mode;

      mode = clay_layer_get_mode (iter->data);
      if (clay_layer_mode_is_composite_space_mutable (mode) &&
          composite_space != clay_layer_get_composite_space (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_LAYER_PROP_UNDO,
                                           CLAY_UNDO_LAYER_MODE);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite space"));

      for (iter = update_layers; iter; iter = iter->next)
        clay_layer_set_composite_space (iter->data, composite_space, push_undo);

      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_end (image);

      g_list_free (update_layers);

      clay_image_flush (image);
    }
}

void
layers_composite_mode_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage              *image;
  GList                  *layers;
  GList                  *update_layers = NULL;
  GList                  *iter;
  ClayLayerCompositeMode  composite_mode;
  gboolean             push_undo = TRUE;
  return_if_no_layers (image, layers, data);

  composite_mode = (ClayLayerCompositeMode) g_variant_get_int32 (value);

  for (iter = layers; iter; iter = iter->next)
    {
      ClayLayerMode mode;

      mode = clay_layer_get_mode (iter->data);
      if (clay_layer_mode_is_composite_mode_mutable (mode) &&
          composite_mode != clay_layer_get_composite_mode (iter->data))
        update_layers = g_list_prepend (update_layers, iter->data);
    }

  if (g_list_length (update_layers) == 1)
    {
      ClayUndo *undo;

      undo = clay_image_undo_can_compress (image, CLAY_TYPE_LAYER_PROP_UNDO,
                                           CLAY_UNDO_LAYER_MODE);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (update_layers->data))
        push_undo = FALSE;
    }

  if (update_layers)
    {
      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_start (image, CLAY_UNDO_GROUP_LAYER_MODE,
                                     _("Set layers' composite mode"));

      for (iter = update_layers; iter; iter = iter->next)
        clay_layer_set_composite_mode (iter->data, composite_mode, push_undo);

      if (g_list_length (update_layers) > 1)
        clay_image_undo_group_end (image);

      g_list_free (update_layers);

      clay_image_flush (image);
    }
}

void
layers_visible_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_visible_cmd_callback (action, value, image, layers);
}

void
layers_lock_content_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_lock_content_cmd_callback (action, value, image, layers);
}

void
layers_lock_position_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  return_if_no_layers (image, layers, data);

  items_lock_position_cmd_callback (action, value, image, layers);
}

void
layers_lock_alpha_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage *image;
  GList     *layers;
  GList     *iter;
  gboolean   lock_alpha;
  gboolean   lock_change = FALSE;
  return_if_no_layers (image, layers, data);

  lock_alpha = g_variant_get_boolean (value);

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_can_lock_alpha (iter->data))
        {
          /* Similar trick as in layers_mask_show_cmd_callback().
           * When unlocking, we expect all selected layers to be locked,
           * otherwise SET_ACTIVE() calls in layers-actions.c will
           * trigger lock updates.
           */
          if (! lock_alpha && ! clay_layer_get_lock_alpha (iter->data))
            return;
          if (lock_alpha != clay_layer_get_lock_alpha (iter->data))
            lock_change = TRUE;
        }
    }
  if (! lock_change)
    /* No layer locks would be changed. */
    return;

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_LOCK_ALPHA,
                               lock_alpha ? _("Lock alpha channels") : _("Unlock alpha channels"));

  for (iter = layers; iter; iter = iter->next)
    {
      if (clay_layer_can_lock_alpha (iter->data))
        {
          if (lock_alpha != clay_layer_get_lock_alpha (iter->data))
            clay_layer_set_lock_alpha (iter->data, lock_alpha, TRUE);
        }
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
layers_color_tag_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage    *image;
  GList        *layers;
  ClayColorTag  color_tag;
  return_if_no_layers (image, layers, data);

  color_tag = (ClayColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, layers, color_tag);
}


/*  private functions  */

static void
layers_new_callback (GtkWidget              *dialog,
                     ClayImage              *image,
                     ClayLayer              *layer,
                     ClayContext            *context,
                     const gchar            *layer_name,
                     ClayLayerMode           layer_mode,
                     ClayLayerColorSpace     layer_blend_space,
                     ClayLayerColorSpace     layer_composite_space,
                     ClayLayerCompositeMode  layer_composite_mode,
                     gdouble                 layer_opacity,
                     ClayFillType            layer_fill_type,
                     ClayLink               *link,
                     ClayPath               *path,
                     gint                    layer_width,
                     gint                    layer_height,
                     gint                    layer_offset_x,
                     gint                    layer_offset_y,
                     gboolean                layer_visible,
                     ClayColorTag            layer_color_tag,
                     gboolean                layer_lock_pixels,
                     gboolean                layer_lock_position,
                     gboolean                layer_lock_visibility,
                     gboolean                layer_lock_alpha,
                     gboolean                rename_text_layer, /* unused */
                     gpointer                user_data)
{
  ClayDialogConfig *config     = CLAY_DIALOG_CONFIG (image->clay->config);
  GList            *layers     = clay_image_get_selected_layers (image);
  GList            *new_layers = NULL;
  GList            *iter;
  gint              n_layers   = g_list_length (layers);
  gboolean          run_once   = (n_layers == 0);

  g_return_if_fail (link == NULL);

  g_object_set (config,
                "layer-new-name",            layer_name,
                "layer-new-mode",            layer_mode,
                "layer-new-blend-space",     layer_blend_space,
                "layer-new-composite-space", layer_composite_space,
                "layer-new-composite-mode",  layer_composite_mode,
                "layer-new-opacity",         layer_opacity,
                "layer-new-fill-type",       layer_fill_type,
                NULL);

  layers = g_list_copy (layers);
  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_ADD,
                               ngettext ("New layer",
                                         "New layers",
                                         n_layers > 0 ? n_layers : 1));
  for (iter = layers; iter || run_once; iter = iter ? iter->next : NULL)
    {
      ClayLayer *parent;
      gint       position;

      run_once = FALSE;
      if (iter)
        {
          if (clay_viewable_get_children (CLAY_VIEWABLE (iter->data)))
            {
              parent   = iter->data;
              position = 0;
            }
          else
            {
              parent   = CLAY_LAYER (clay_item_get_parent (iter->data));
              position = clay_item_get_index (iter->data);
            }
        }
      else /* run_once */
        {
          parent = NULL;
          position = 0;
        }

      layer = clay_layer_new (image, layer_width, layer_height,
                              clay_image_get_layer_format (image, TRUE),
                              config->layer_new_name,
                              config->layer_new_opacity,
                              config->layer_new_mode);

      if (layer)
        {
          clay_item_set_offset (CLAY_ITEM (layer), layer_offset_x, layer_offset_y);
          clay_drawable_fill (CLAY_DRAWABLE (layer), context,
                              config->layer_new_fill_type);
          clay_item_set_visible (CLAY_ITEM (layer), layer_visible, FALSE);
          clay_item_set_color_tag (CLAY_ITEM (layer), layer_color_tag, FALSE);
          clay_item_set_lock_content (CLAY_ITEM (layer), layer_lock_pixels,
                                      FALSE);
          clay_item_set_lock_position (CLAY_ITEM (layer), layer_lock_position,
                                       FALSE);
          clay_item_set_lock_visibility (CLAY_ITEM (layer), layer_lock_visibility,
                                         FALSE);
          clay_layer_set_lock_alpha (layer, layer_lock_alpha, FALSE);
          clay_layer_set_blend_space (layer, layer_blend_space, FALSE);
          clay_layer_set_composite_space (layer, layer_composite_space, FALSE);
          clay_layer_set_composite_mode (layer, layer_composite_mode, FALSE);

          clay_image_add_layer (image, layer, parent, position, TRUE);
          clay_image_flush (image);

          new_layers = g_list_prepend (new_layers, layer);
        }
      else
        {
          g_warning ("%s: could not allocate new layer", G_STRFUNC);
        }
    }

  clay_image_undo_group_end (image);
  clay_image_set_selected_layers (image, new_layers);

  g_list_free (layers);
  g_list_free (new_layers);
  gtk_widget_destroy (dialog);
}

static void
layers_edit_attributes_callback (GtkWidget              *dialog,
                                 ClayImage              *image,
                                 ClayLayer              *layer,
                                 ClayContext            *context,
                                 const gchar            *layer_name,
                                 ClayLayerMode           layer_mode,
                                 ClayLayerColorSpace     layer_blend_space,
                                 ClayLayerColorSpace     layer_composite_space,
                                 ClayLayerCompositeMode  layer_composite_mode,
                                 gdouble                 layer_opacity,
                                 ClayFillType            unused1,
                                 ClayLink               *link,
                                 ClayPath               *path,
                                 gint                    unused2,
                                 gint                    unused3,
                                 gint                    layer_offset_x,
                                 gint                    layer_offset_y,
                                 gboolean                layer_visible,
                                 ClayColorTag            layer_color_tag,
                                 gboolean                layer_lock_pixels,
                                 gboolean                layer_lock_position,
                                 gboolean                layer_lock_visibility,
                                 gboolean                layer_lock_alpha,
                                 gboolean                rename_text_layer,
                                 gpointer                user_data)
{
  ClayItem *item = CLAY_ITEM (layer);

  if (strcmp (layer_name, clay_object_get_name (layer))               ||
      layer_mode            != clay_layer_get_mode (layer)            ||
      layer_blend_space     != clay_layer_get_blend_space (layer)     ||
      layer_composite_space != clay_layer_get_composite_space (layer) ||
      layer_composite_mode  != clay_layer_get_composite_mode (layer)  ||
      layer_opacity         != clay_layer_get_opacity (layer)         ||
      layer_offset_x        != clay_item_get_offset_x (item)          ||
      layer_offset_y        != clay_item_get_offset_y (item)          ||
      layer_visible         != clay_item_get_visible (item)           ||
      layer_color_tag       != clay_item_get_color_tag (item)         ||
      layer_lock_pixels     != clay_item_get_lock_content (item)      ||
      layer_lock_position   != clay_item_get_lock_position (item)     ||
      layer_lock_visibility != clay_item_get_lock_visibility (item)   ||
      layer_lock_alpha      != clay_layer_get_lock_alpha (layer)      ||
      link || path)
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Layer Attributes"));

      if (strcmp (layer_name, clay_object_get_name (layer)))
        {
          GError *error = NULL;

          if (! clay_item_rename (CLAY_ITEM (layer), layer_name, &error))
            {
              clay_message_literal (image->clay,
                                    G_OBJECT (dialog), CLAY_MESSAGE_WARNING,
                                    error->message);
              g_clear_error (&error);
            }
        }

      if (layer_mode != clay_layer_get_mode (layer))
        clay_layer_set_mode (layer, layer_mode, TRUE);

      if (layer_blend_space != clay_layer_get_blend_space (layer))
        clay_layer_set_blend_space (layer, layer_blend_space, TRUE);

      if (layer_composite_space != clay_layer_get_composite_space (layer))
        clay_layer_set_composite_space (layer, layer_composite_space, TRUE);

      if (layer_composite_mode != clay_layer_get_composite_mode (layer))
        clay_layer_set_composite_mode (layer, layer_composite_mode, TRUE);

      if (layer_opacity != clay_layer_get_opacity (layer))
        clay_layer_set_opacity (layer, layer_opacity, TRUE);

      if (! clay_item_is_vector_layer (item) &&
          (layer_offset_x != clay_item_get_offset_x (item) ||
           layer_offset_y != clay_item_get_offset_y (item)))
        {
          clay_item_translate (item,
                               layer_offset_x - clay_item_get_offset_x (item),
                               layer_offset_y - clay_item_get_offset_y (item),
                               TRUE);
        }

      if (layer_visible != clay_item_get_visible (item))
        clay_item_set_visible (item, layer_visible, TRUE);

      if (layer_color_tag != clay_item_get_color_tag (item))
        clay_item_set_color_tag (item, layer_color_tag, TRUE);

      if (layer_lock_pixels != clay_item_get_lock_content (item))
        clay_item_set_lock_content (item, layer_lock_pixels, TRUE);

      if (layer_lock_position != clay_item_get_lock_position (item))
        clay_item_set_lock_position (item, layer_lock_position, TRUE);

      if (layer_lock_visibility != clay_item_get_lock_visibility (item))
        clay_item_set_lock_visibility (item, layer_lock_visibility, TRUE);

      if (layer_lock_alpha != clay_layer_get_lock_alpha (layer))
        clay_layer_set_lock_alpha (layer, layer_lock_alpha, TRUE);

      if (CLAY_IS_LINK_LAYER (layer) && link)
        clay_link_layer_set_link (CLAY_LINK_LAYER (layer), link, TRUE);

      if (clay_item_is_vector_layer (item) && path)
          clay_vector_layer_set_path (CLAY_VECTOR_LAYER (layer), path, TRUE);

      clay_image_undo_group_end (image);
      clay_image_flush (image);
    }

  if (CLAY_IS_RASTERIZABLE (layer))
    clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (layer),
                                       rename_text_layer);

  gtk_widget_destroy (dialog);
}

static void
layers_add_mask_callback (GtkWidget       *dialog,
                          GList           *layers,
                          ClayAddMaskType  add_mask_type,
                          ClayChannel     *channel,
                          gboolean         invert,
                          gboolean         edit_mask,
                          gpointer         user_data)
{
  ClayImage        *image  = clay_item_get_image (CLAY_ITEM (layers->data));
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  ClayLayerMask    *mask;
  GList            *iter;
  GError           *error = NULL;

  g_object_set (config,
                "layer-add-mask-type",      add_mask_type,
                "layer-add-mask-invert",    invert,
                "layer-add-mask-edit-mask", edit_mask,
                NULL);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_LAYER_ADD,
                               _("Add Layer Masks"));

  for (iter = layers; iter; iter = iter->next)
    {
      mask = clay_layer_create_mask (iter->data,
                                     config->layer_add_mask_type,
                                     channel);

      if (config->layer_add_mask_invert)
        clay_channel_invert (CLAY_CHANNEL (mask), FALSE);

      if (! clay_layer_add_mask (iter->data, mask, edit_mask, TRUE, &error))
        {
          clay_message_literal (image->clay,
                                G_OBJECT (dialog), CLAY_MESSAGE_WARNING,
                                error->message);
          g_object_unref (mask);
          g_clear_error (&error);

          clay_image_undo_group_end (image);
          return;
        }
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
layers_scale_callback (GtkWidget             *dialog,
                       ClayViewable          *viewable,
                       gint                   width,
                       gint                   height,
                       ClayUnit              *unit,
                       ClayInterpolationType  interpolation,
                       gdouble                xresolution,    /* unused */
                       gdouble                yresolution,    /* unused */
                       ClayUnit              *resolution_unit,/* unused */
                       gpointer               user_data)
{
  ClayDisplay *display = CLAY_DISPLAY (user_data);

  layer_scale_unit   = unit;
  layer_scale_interp = interpolation;

  if (width > 0 && height > 0)
    {
      ClayItem     *item = CLAY_ITEM (viewable);
      ClayProgress *progress;
      GtkWidget    *progress_dialog = NULL;

      gtk_widget_destroy (dialog);

      if (width  == clay_item_get_width  (item) &&
          height == clay_item_get_height (item))
        return;

      if (display)
        {
          progress = CLAY_PROGRESS (display);
        }
      else
        {
          progress_dialog = clay_progress_dialog_new ();
          progress = CLAY_PROGRESS (progress_dialog);
        }

      progress = clay_progress_start (progress, FALSE, _("Scaling"));

      clay_item_scale_by_origin (item,
                                 width, height, interpolation,
                                 progress, TRUE);

      if (progress)
        clay_progress_end (progress);

      if (progress_dialog)
        gtk_widget_destroy (progress_dialog);

      clay_image_flush (clay_item_get_image (item));
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
layers_resize_callback (GtkWidget    *dialog,
                        ClayViewable *viewable,
                        ClayContext  *context,
                        gint          width,
                        gint          height,
                        ClayUnit     *unit,
                        gint          offset_x,
                        gint          offset_y,
                        gdouble       unused0,
                        gdouble       unused1,
                        ClayUnit     *unused2,
                        ClayFillType  fill_type,
                        ClayItemSet   unused3,
                        gboolean      unused4,
                        gpointer      user_data)
{
  layer_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      ClayItem         *item   = CLAY_ITEM (viewable);
      ClayImage        *image  = clay_item_get_image (item);
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      g_object_set (config,
                    "layer-resize-fill-type", fill_type,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == clay_item_get_width  (item) &&
          height == clay_item_get_height (item))
        return;

      clay_item_resize (item, context, fill_type,
                        width, height, offset_x, offset_y);
      clay_image_flush (clay_item_get_image (item));
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static gint
layers_mode_index (ClayLayerMode         layer_mode,
                   const ClayLayerMode  *modes,
                   gint                  n_modes)
{
  gint i = 0;

  while (i < (n_modes - 1) && modes[i] != layer_mode)
    i++;

  return i;
}
