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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-merge.h"
#include "core/clayimage-undo.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"
#include "core/claytoolinfo.h"

#include "pdb/claypdb.h"
#include "pdb/clayprocedure.h"

#include "path/claypath.h"
#include "path/claypath-export.h"
#include "path/claypath-import.h"
#include "path/clayvectorlayer.h"

#include "tools/claytools-utils.h"

#include "widgets/clayaction.h"
#include "widgets/clayclipboard.h"
#include "widgets/clayhelp-ids.h"

#include "display/claydisplay.h"

#include "tools/claypathtool.h"
#include "tools/tool_manager.h"

#include "dialogs/dialogs.h"
#include "dialogs/path-export-dialog.h"
#include "dialogs/path-import-dialog.h"
#include "dialogs/path-options-dialog.h"

#include "actions.h"
#include "items-commands.h"
#include "paths-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   paths_new_callback             (GtkWidget       *dialog,
                                              ClayImage       *image,
                                              ClayPath        *path,
                                              ClayContext     *context,
                                              const gchar     *path_name,
                                              gboolean         path_visible,
                                              ClayColorTag     path_color_tag,
                                              gboolean         path_lock_content,
                                              gboolean         path_lock_position,
                                              gboolean         path_lock_visibility,
                                              gpointer         user_data);
static void   paths_edit_attributes_callback (GtkWidget       *dialog,
                                              ClayImage       *image,
                                              ClayPath        *path,
                                              ClayContext     *context,
                                              const gchar     *path_name,
                                              gboolean         path_visible,
                                              ClayColorTag     path_color_tag,
                                              gboolean         path_lock_content,
                                              gboolean         path_lock_position,
                                              gboolean         path_lock_visibility,
                                              gpointer         user_data);
static void   paths_import_callback          (GtkWidget       *dialog,
                                              ClayImage       *image,
                                              GFile           *file,
                                              GFile           *import_folder,
                                              gboolean         merge_paths,
                                              gboolean         scale_paths,
                                              gpointer         user_data);
static void   paths_export_cmd_callback      (gpointer         data,
                                              gboolean         selected_only);
static void   paths_export_callback          (GtkNativeDialog *dialog,
                                              ClayImage       *image,
                                              GFile           *file,
                                              GFile           *export_folder,
                                              gboolean         active_only,
                                              gpointer         user_data);


/*  public functions  */

void
paths_edit_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  ClayTool  *active_tool;
  return_if_no_paths (image, paths, data);

  if (g_list_length (paths) != 1)
    return;

  active_tool = tool_manager_get_active (image->clay);

  if (! CLAY_IS_PATH_TOOL (active_tool))
    {
      ClayToolInfo *tool_info = clay_get_tool_info (image->clay,
                                                    "clay-path-tool");

      if (CLAY_IS_TOOL_INFO (tool_info))
        {
          clay_context_set_tool (action_data_get_context (data), tool_info);
          active_tool = tool_manager_get_active (image->clay);
        }
    }

  if (CLAY_IS_PATH_TOOL (active_tool))
    clay_path_tool_set_path (CLAY_PATH_TOOL (active_tool), NULL, paths->data);
}

void
paths_edit_attributes_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage   *image;
  GList       *paths;
  ClayPath    *path;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  if (g_list_length (paths) != 1)
    return;

  path = paths->data;

#define EDIT_DIALOG_KEY "clay-path-edit-attributes-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (path), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      ClayItem *item = CLAY_ITEM (path);

      dialog = path_options_dialog_new (image, path,
                                        action_data_get_context (data),
                                        widget,
                                        _("Path Attributes"),
                                        "clay-path-edit",
                                        CLAY_ICON_EDIT,
                                        _("Edit Path Attributes"),
                                        CLAY_HELP_PATH_EDIT,
                                        clay_object_get_name (path),
                                        clay_item_get_visible (item),
                                        clay_item_get_color_tag (item),
                                        clay_item_get_lock_content (item),
                                        clay_item_get_lock_position (item),
                                        clay_item_get_lock_visibility (item),
                                        paths_edit_attributes_callback,
                                        NULL);

      dialogs_attach_dialog (G_OBJECT (path), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_new_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "clay-path-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      dialog = path_options_dialog_new (image, NULL,
                                        action_data_get_context (data),
                                        widget,
                                        _("New Path"),
                                        "clay-path-new",
                                        CLAY_ICON_PATH,
                                        _("Create a New Path"),
                                        CLAY_HELP_PATH_NEW,
                                        config->path_new_name,
                                        FALSE,
                                        CLAY_COLOR_TAG_NONE,
                                        FALSE,
                                        FALSE,
                                        FALSE,
                                        paths_new_callback,
                                        NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_new_last_vals_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage        *image;
  ClayPath         *path;
  ClayDialogConfig *config;
  return_if_no_image (image, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  path = clay_path_new (image, config->path_new_name);
  clay_image_add_path (image, path,
                       CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);
}

void
paths_raise_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        {
          moved_list = g_list_prepend (moved_list, iter->data);
        }
      else
        {
          clay_image_flush (image);
          g_list_free (moved_list);
          return;
        }
    }

  if (moved_list)
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path",
                                             "Raise Paths",
                                             g_list_length (moved_list)));

      moved_list = g_list_reverse (moved_list);
      for (iter = moved_list; iter; iter = iter->next)
        clay_image_raise_item (image, CLAY_ITEM (iter->data), NULL);

      clay_image_flush (image);
      clay_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
paths_raise_to_top_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Raise Path to Top",
                                             "Raise Paths to Top",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        clay_image_raise_item_to_top (image, CLAY_ITEM (iter->data));

      clay_image_flush (image);
      clay_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
paths_lower_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *paths_list;
      gint   index;

      paths_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (paths_list) - 1)
        {
          moved_list = g_list_prepend (moved_list, iter->data);
        }
      else
        {
          clay_image_flush (image);
          g_list_free (moved_list);
          return;
        }
    }

  if (moved_list)
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path",
                                             "Lower Paths",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        clay_image_lower_item (image, CLAY_ITEM (iter->data), NULL);

      clay_image_flush (image);
      clay_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
paths_lower_to_bottom_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GList     *list;
  GList     *iter;
  GList     *moved_list = NULL;
  return_if_no_paths (image, list, data);

  for (iter = list; iter; iter = iter->next)
    {
      GList *paths_list;
      gint   index;

      paths_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (paths_list) - 1)
        moved_list = g_list_prepend (moved_list, iter->data);
    }

  if (moved_list)
    {
      moved_list = g_list_reverse (moved_list);
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_DISPLACE,
                                   ngettext ("Lower Path to Bottom",
                                             "Lower Paths to Bottom",
                                             g_list_length (moved_list)));

      for (iter = moved_list; iter; iter = iter->next)
        clay_image_lower_item_to_bottom (image, CLAY_ITEM (iter->data));

      clay_image_flush (image);
      clay_image_undo_group_end (image);
      g_list_free (moved_list);
    }
}

void
paths_duplicate_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  GList     *new_paths = NULL;
  GList     *iter;
  return_if_no_paths (image, paths, data);

  paths = g_list_copy (paths);
  paths = g_list_reverse (paths);

  /* TODO: proper undo group. */
  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_PATHS_IMPORT,
                               _("Duplicate Paths"));
  for (iter = paths; iter; iter = iter->next)
    {
      ClayPath *new_path;

      new_path = CLAY_PATH (clay_item_duplicate (iter->data,
                                                 G_TYPE_FROM_INSTANCE (iter->data)));
      /*  use the actual parent here, not CLAY_IMAGE_ACTIVE_PARENT because
       *  the latter would add a duplicated group inside itself instead of
       *  above it
       */
      clay_image_add_path (image, new_path,
                           clay_path_get_parent (iter->data), -1,
                           TRUE);
      new_paths = g_list_prepend (new_paths, new_path);
    }
  if (new_paths)
    {
      clay_image_set_selected_paths (image, new_paths);
      clay_image_flush (image);
    }
  clay_image_undo_group_end (image);
  g_list_free (paths);
}

void
paths_delete_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  GtkWidget *widget;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  paths = g_list_copy (paths);

  /* Initial check to make sure none are connected to vector layers */
  for (GList *iter = paths; iter; iter = iter->next)
    {
      if (clay_path_attached_to_vector_layer (CLAY_PATH (iter->data), image))
        {
          clay_message_literal (image->clay, G_OBJECT (widget),
                                CLAY_MESSAGE_WARNING,
                                _("Cannot delete paths attached to vector "
                                  "layers"));
          clay_tools_blink_item (image->clay, CLAY_ITEM (iter->data));
          g_list_free (paths);
          return;
        }
    }

  /* TODO: proper undo group. */
  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_PATHS_IMPORT,
                               _("Remove Paths"));

  for (GList *iter = paths; iter; iter = iter->next)
    clay_image_remove_path (image, iter->data, TRUE, NULL);

  clay_image_undo_group_end (image);
  clay_image_flush (image);
  g_list_free (paths);
}

void
paths_merge_visible_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage   *image;
  GList       *paths;
  GtkWidget   *widget;
  ClayPath    *path;
  GList       *list;
  GError      *error = NULL;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  /* Make sure none of the visible paths are attached to vector layers
   * before attempting to merge them. */
  for (list = clay_image_get_path_iter (image);
       list; list = g_list_next (list))
    {
      path = list->data;

      if (clay_item_get_visible (CLAY_ITEM (path)) &&
          clay_path_attached_to_vector_layer (path, image))
        {
          clay_message_literal (image->clay, G_OBJECT (widget),
                                CLAY_MESSAGE_WARNING,
                                _("Cannot merge paths attached to vector "
                                  "layers"));
          clay_tools_blink_item (image->clay, CLAY_ITEM (path));
          return;
        }
    }

  if (! clay_image_merge_visible_paths (image, &error))
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  clay_image_flush (image);
}

void
path_to_vector_layer_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayImage       *image;
  GList           *paths;
  ClayVectorLayer *layer;
  return_if_no_paths (image, paths, data);

  layer = clay_vector_layer_new (image, paths->data,
                                 clay_get_user_context (image->clay));
  clay_image_add_layer (image,
                        CLAY_LAYER (layer),
                        CLAY_IMAGE_ACTIVE_PARENT,
                        -1,
                        TRUE);
  clay_vector_layer_refresh (layer);

  clay_image_flush (image);
}

void
paths_to_selection_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage      *image;
  GList          *paths;
  GList          *iter;
  ClayChannelOps  operation;
  return_if_no_paths (image, paths, data);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_DRAWABLE_MOD,
                               _("Paths to selection"));

  operation = (ClayChannelOps) g_variant_get_int32 (value);

  for (iter = paths; iter; iter = iter->next)
    {
      clay_item_to_selection (iter->data, operation, TRUE, FALSE, 0, 0);

      if (operation == CLAY_CHANNEL_OP_REPLACE && iter == paths)
        operation = CLAY_CHANNEL_OP_ADD;
    }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
}

void
paths_selection_to_paths_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage      *image;
  GtkWidget      *widget;
  ClayProcedure  *procedure;
  ClayValueArray *args;
  ClayDisplay    *display;
  gboolean        advanced;
  GError         *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  advanced = (gboolean) g_variant_get_int32 (value);

  procedure = clay_pdb_lookup_procedure (image->clay->pdb,
                                         "plug-in-sel2path");

  if (! procedure)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_ERROR,
                            "Selection to path procedure lookup failed.");
      return;
    }

  display = clay_context_get_display (action_data_get_context (data));

  args = clay_procedure_get_arguments (procedure);

  g_value_set_enum   (clay_value_array_index (args, 0),
                      advanced ?
                      CLAY_RUN_INTERACTIVE : CLAY_RUN_NONINTERACTIVE);
  g_value_set_object (clay_value_array_index (args, 1),
                      image);

  clay_procedure_execute_async (procedure, image->clay,
                                action_data_get_context (data),
                                CLAY_PROGRESS (display), args,
                                display, &error);

  clay_value_array_unref (args);

  if (error)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_ERROR,
                            error->message);
      g_error_free (error);
    }
}

void
paths_fill_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_fill_cmd_callback (action, image, paths,
                           _("Fill Path"),
                           CLAY_ICON_TOOL_BUCKET_FILL,
                           CLAY_HELP_PATH_FILL,
                           data);
}

void
paths_fill_last_vals_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_fill_last_vals_cmd_callback (action, image, paths, data);
}

void
paths_stroke_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_stroke_cmd_callback (action, image, paths,
                             _("Stroke Path"),
                             CLAY_ICON_PATH_STROKE,
                             CLAY_HELP_PATH_STROKE,
                             data);
}

void
paths_stroke_last_vals_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_stroke_last_vals_cmd_callback (action, image, paths, data);
}

void
paths_copy_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage   *image;
  GList       *paths;
  gchar       *svg;
  return_if_no_paths (image, paths, data);

  svg = clay_path_export_string (image, paths);

  if (svg)
    {
      clay_clipboard_set_svg (image->clay, svg);
      g_free (svg);
    }
}

void
paths_paste_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  gchar     *svg;
  gsize      svg_size;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  svg = clay_clipboard_get_svg (image->clay, &svg_size);

  if (svg)
    {
      GError *error = NULL;

      if (! clay_path_import_buffer (image, svg, svg_size,
                                     TRUE, FALSE,
                                     CLAY_IMAGE_ACTIVE_PARENT, -1,
                                     NULL, &error))
        {
          clay_message (image->clay, G_OBJECT (widget), CLAY_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }
      else
        {
          clay_image_flush (image);
        }

      g_free (svg);
    }
}

void
paths_export_selected_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  paths_export_cmd_callback (data, TRUE);
}

void
paths_export_all_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  paths_export_cmd_callback (data, FALSE);
}

static void
paths_export_cmd_callback (gpointer data,
                           gboolean selected_only)
{
  ClayImage        *image;
  ClayDialogConfig *config;
  GList            *paths;
  GFile            *folder = NULL;
  GtkWidget        *widget;
  GtkNativeDialog  *dialog;
  return_if_no_paths (image, paths, data);
  return_if_no_widget (widget, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  if (config->path_export_path)
    folder = clay_file_new_for_config_path (config->path_export_path,
                                            NULL);

  dialog = path_export_dialog_new (image, widget,
                                   folder,
                                   selected_only,
                                   paths_export_callback,
                                   NULL);

  if (folder)
    g_object_unref (folder);

  gtk_native_dialog_show (dialog);
}

void
paths_import_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define IMPORT_DIALOG_KEY "clay-paths-import-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      GFile            *folder = NULL;

      if (config->path_import_path)
        folder = clay_file_new_for_config_path (config->path_import_path,
                                                NULL);

      dialog = path_import_dialog_new (image, widget,
                                       folder,
                                       config->path_import_merge,
                                       config->path_import_scale,
                                       paths_import_callback,
                                       NULL);

      dialogs_attach_dialog (G_OBJECT (image), IMPORT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
paths_visible_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_visible_cmd_callback (action, value, image, paths);
}

void
paths_lock_content_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_lock_content_cmd_callback (action, value, image, paths);
}

void
paths_lock_position_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage *image;
  GList     *paths;
  return_if_no_paths (image, paths, data);

  items_lock_position_cmd_callback (action, value, image, paths);
}

void
paths_color_tag_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage    *image;
  GList        *paths;
  ClayColorTag  color_tag;
  return_if_no_paths (image, paths, data);

  color_tag = (ClayColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, paths, color_tag);
}


/*  private functions  */

static void
paths_new_callback (GtkWidget    *dialog,
                    ClayImage    *image,
                    ClayPath     *path,
                    ClayContext  *context,
                    const gchar  *paths_name,
                    gboolean      paths_visible,
                    ClayColorTag  paths_color_tag,
                    gboolean      paths_lock_content,
                    gboolean      paths_lock_position,
                    gboolean      paths_lock_visibility,
                    gpointer      user_data)
{
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

  g_object_set (config,
                "path-new-name", paths_name,
                NULL);

  path = clay_path_new (image, config->path_new_name);
  clay_item_set_visible (CLAY_ITEM (path), paths_visible, FALSE);
  clay_item_set_color_tag (CLAY_ITEM (path), paths_color_tag, FALSE);
  clay_item_set_lock_content (CLAY_ITEM (path), paths_lock_content, FALSE);
  clay_item_set_lock_position (CLAY_ITEM (path), paths_lock_position, FALSE);
  clay_item_set_lock_visibility (CLAY_ITEM (path), paths_lock_visibility, FALSE);

  clay_image_add_path (image, path,
                       CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
paths_edit_attributes_callback (GtkWidget    *dialog,
                                ClayImage    *image,
                                ClayPath     *path,
                                ClayContext  *context,
                                const gchar  *paths_name,
                                gboolean      paths_visible,
                                ClayColorTag  paths_color_tag,
                                gboolean      paths_lock_content,
                                gboolean      paths_lock_position,
                                gboolean      paths_lock_visibility,
                                gpointer      user_data)
{
  ClayItem *item = CLAY_ITEM (path);

  if (strcmp (paths_name, clay_object_get_name (path))            ||
      paths_visible         != clay_item_get_visible (item)       ||
      paths_color_tag       != clay_item_get_color_tag (item)     ||
      paths_lock_content    != clay_item_get_lock_content (item)  ||
      paths_lock_position   != clay_item_get_lock_position (item) ||
      paths_lock_visibility != clay_item_get_lock_visibility (item))
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Path Attributes"));

      if (strcmp (paths_name, clay_object_get_name (path)))
        clay_item_rename (CLAY_ITEM (path), paths_name, NULL);

      if (paths_visible != clay_item_get_visible (item))
        clay_item_set_visible (item, paths_visible, TRUE);

      if (paths_color_tag != clay_item_get_color_tag (item))
        clay_item_set_color_tag (item, paths_color_tag, TRUE);

      if (paths_lock_content != clay_item_get_lock_content (item))
        clay_item_set_lock_content (item, paths_lock_content, TRUE);

      if (paths_lock_position != clay_item_get_lock_position (item))
        clay_item_set_lock_position (item, paths_lock_position, TRUE);

      if (paths_lock_visibility != clay_item_get_lock_visibility (item))
        clay_item_set_lock_visibility (item, paths_lock_visibility, TRUE);

      clay_image_undo_group_end (image);

      clay_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

static void
paths_import_callback (GtkWidget *dialog,
                       ClayImage *image,
                       GFile     *file,
                       GFile     *import_folder,
                       gboolean   merge_paths,
                       gboolean   scale_paths,
                       gpointer   user_data)
{
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  gchar            *path   = NULL;
  GError           *error  = NULL;

  if (import_folder)
    path = clay_file_get_config_path (import_folder, NULL);

  g_object_set (config,
                "path-import-path",  path,
                "path-import-merge", merge_paths,
                "path-import-scale", scale_paths,
                NULL);

  if (path)
    g_free (path);

  if (clay_path_import_file (image, file,
                             config->path_import_merge,
                             config->path_import_scale,
                             CLAY_IMAGE_ACTIVE_PARENT, -1,
                             NULL, &error))
    {
      clay_image_flush (image);
    }
  else
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_error_free (error);
      return;
    }

  gtk_widget_destroy (dialog);
}

static void
paths_export_callback (GtkNativeDialog *dialog,
                       ClayImage       *image,
                       GFile           *file,
                       GFile           *export_folder,
                       gboolean         active_only,
                       gpointer         user_data)
{
  ClayDialogConfig *config  = CLAY_DIALOG_CONFIG (image->clay->config);
  GList            *paths   = NULL;
  gchar            *path    = NULL;
  GError           *error   = NULL;

  if (export_folder)
    path = clay_file_get_config_path (export_folder, NULL);

  g_object_set (config,
                "path-export-path",        path,
                "path-export-active-only", active_only,
                NULL);

  if (path)
    g_free (path);

  if (config->path_export_active_only)
    paths = clay_image_get_selected_paths (image);

  if (! clay_path_export_file (image, paths, file, &error))
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);
      return;
    }

  gtk_native_dialog_destroy (dialog);
}

void
paths_select_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage            *image;
  GList                *new_paths = NULL;
  GList                *paths;
  GList                *iter;
  ClayActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  paths = clay_image_get_selected_paths (image);
  run_once = (g_list_length (paths) == 0);

  for (iter = paths; iter || run_once; iter = iter ? iter->next : NULL)
    {
      ClayPath      *new_vec;
      ClayContainer *container;

      if (iter)
        {
          container = clay_item_get_container (CLAY_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = clay_image_get_paths (image);
          run_once  = FALSE;
        }
      new_vec = (ClayPath *) action_select_object (select_type,
                                                   container,
                                                   iter ? iter->data : NULL);
      if (new_vec)
        new_paths = g_list_prepend (new_paths, new_vec);
    }

  if (new_paths)
    {
      clay_image_set_selected_paths (image, new_paths);
      clay_image_flush (image);
    }

  g_list_free (new_paths);
}
