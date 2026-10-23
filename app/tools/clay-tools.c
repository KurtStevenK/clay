/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis and others
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

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "widgets/claywidgets-utils.h"

#include "core/clay.h"
#include "core/clay-contexts.h"
#include "core/clay-internal-data.h"
#include "core/claycontext.h"
#include "core/claylist.h"
#include "core/claytoolgroup.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"

#include "clay-tool-options-manager.h"
#include "clay-tools.h"
#include "claytooloptions-gui.h"
#include "tool_manager.h"

#include "clayairbrushtool.h"
#include "clayaligntool.h"
#include "claybrightnesscontrasttool.h"
#include "claybucketfilltool.h"
#include "claybycolorselecttool.h"
#include "claycagetool.h"
#include "clayclonetool.h"
#include "claycolorpickertool.h"
#include "clayconvolvetool.h"
#include "claycroptool.h"
#include "claycurvestool.h"
#include "claydodgeburntool.h"
#include "clayellipseselecttool.h"
#include "clayerasertool.h"
#include "clayfliptool.h"
#include "clayfreeselecttool.h"
#include "clayforegroundselecttool.h"
#include "clayfuzzyselecttool.h"
#include "claygegltool.h"
#include "claygradienttool.h"
#include "clayhandletransformtool.h"
#include "clayhealtool.h"
#include "clayinktool.h"
#include "clayiscissorstool.h"
#include "claylevelstool.h"
#include "clayoperationtool.h"
#include "claypathtool.h"
#include "claymagnifytool.h"
#include "claymeasuretool.h"
#include "claymovetool.h"
#include "claymybrushtool.h"
#include "claynpointdeformationtool.h"
#include "clayoffsettool.h"
#include "claypaintbrushtool.h"
#include "claypaintselecttool.h"
#include "claypenciltool.h"
#include "clayperspectiveclonetool.h"
#include "clayperspectivetool.h"
#include "claythresholdtool.h"
#include "clayrectangleselecttool.h"
#include "clayrotatetool.h"
#include "clayseamlessclonetool.h"
#include "clayscaletool.h"
#include "claysheartool.h"
#include "claysmudgetool.h"
#include "claytexttool.h"
#include "claytransform3dtool.h"
#include "clayunifiedtransformtool.h"
#include "claywarptool.h"

#include "clay-intl.h"


#define TOOL_RC_FILE_VERSION 1


/*  local function prototypes  */

static void   clay_tools_register       (GType                   tool_type,
                                         GType                   tool_options_type,
                                         ClayToolOptionsGUIFunc  options_gui_func,
                                         ClayContextPropMask     context_props,
                                         const gchar            *identifier,
                                         const gchar            *label,
                                         const gchar            *tooltip,
                                         const gchar            *menu_label,
                                         const gchar            *menu_accel,
                                         const gchar            *help_domain,
                                         const gchar            *help_data,
                                         const gchar            *icon_name,
                                         gpointer                data);

static void   clay_tools_copy_structure (Clay                   *clay,
                                         ClayContainer          *src_container,
                                         ClayContainer          *dest_container,
                                         GHashTable             *tools);

/*  private variables  */

static GBinding *toolbox_groups_binding = NULL;
static gboolean  tool_options_deleted   = FALSE;


/*  public functions  */

void
clay_tools_init (Clay *clay)
{
  ClayToolRegisterFunc register_funcs[] =
  {
    /*  selection tools */

    clay_rectangle_select_tool_register,
    clay_ellipse_select_tool_register,
    clay_free_select_tool_register,
    clay_fuzzy_select_tool_register,
    clay_by_color_select_tool_register,
    clay_iscissors_tool_register,
    clay_foreground_select_tool_register,
    clay_paint_select_tool_register,

    /*  path tool */

    clay_path_tool_register,

    /*  non-modifying tools  */

    clay_color_picker_tool_register,
    clay_magnify_tool_register,
    clay_measure_tool_register,

    /*  transform tools  */

    clay_move_tool_register,
    clay_align_tool_register,
    clay_crop_tool_register,
    clay_unified_transform_tool_register,
    clay_rotate_tool_register,
    clay_scale_tool_register,
    clay_shear_tool_register,
    clay_handle_transform_tool_register,
    clay_perspective_tool_register,
    clay_transform_3d_tool_register,
    clay_flip_tool_register,
    clay_cage_tool_register,
    clay_warp_tool_register,
    clay_n_point_deformation_tool_register,

    /*  paint tools  */

    clay_seamless_clone_tool_register,
    clay_text_tool_register,
    clay_bucket_fill_tool_register,
    clay_gradient_tool_register,
    clay_pencil_tool_register,
    clay_paintbrush_tool_register,
    clay_eraser_tool_register,
    clay_airbrush_tool_register,
    clay_ink_tool_register,
    clay_mybrush_tool_register,
    clay_clone_tool_register,
    clay_heal_tool_register,
    clay_perspective_clone_tool_register,
    clay_convolve_tool_register,
    clay_smudge_tool_register,
    clay_dodge_burn_tool_register,

    /*  filter tools  */

    clay_brightness_contrast_tool_register,
    clay_threshold_tool_register,
    clay_levels_tool_register,
    clay_curves_tool_register,
    clay_offset_tool_register,
    clay_gegl_tool_register,
    clay_operation_tool_register
  };

  gint i;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_tool_options_create_folder ();

  clay_container_freeze (clay->tool_info_list);

  for (i = 0; i < G_N_ELEMENTS (register_funcs); i++)
    {
      register_funcs[i] (clay_tools_register, clay);
    }

  clay_container_thaw (clay->tool_info_list);

  clay_tool_options_manager_init (clay);

  tool_manager_init (clay);

  toolbox_groups_binding = g_object_bind_property (
    clay->config,            "toolbox-groups",
    clay->tool_item_ui_list, "flat",
    G_BINDING_INVERT_BOOLEAN |
    G_BINDING_SYNC_CREATE);
}

void
clay_tools_exit (Clay *clay)
{
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  g_clear_object (&toolbox_groups_binding);

  tool_manager_exit (clay);

  clay_tool_options_manager_exit (clay);

  for (list = clay_get_tool_info_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = list->data;

      clay_tools_set_tool_options_gui (tool_info->tool_options, NULL);
    }
}

void
clay_tools_restore (Clay *clay)
{
  ClayObject *object;
  GList      *list;
  GError     *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /* restore tool order */
  clay_tools_reset (clay, clay->tool_item_list, TRUE);

  /* make the generic operation tool invisible by default */
  object = clay_container_get_child_by_name (clay->tool_info_list,
                                             "clay-operation-tool");
  if (object)
    g_object_set (object, "visible", FALSE, NULL);

  for (list = clay_get_tool_info_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = CLAY_TOOL_INFO (list->data);

      /*  get default values from prefs (see bug #120832)  */
      clay_config_reset (CLAY_CONFIG (tool_info->tool_options));
    }

  if (! clay_contexts_load (clay, &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  if (! clay_internal_data_load (clay, &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_WARNING, error->message);
      g_clear_error (&error);
    }

  /*  make sure there is always a tool active, so broken config files
   *  can't leave us with no initial tool
   */
  if (! clay_context_get_tool (clay_get_user_context (clay)))
    {
      clay_context_set_tool (clay_get_user_context (clay),
                             clay_get_tool_info_iter (clay)->data);
    }

  for (list = clay_get_tool_info_iter (clay);
       list;
       list = g_list_next (list))
    {
      ClayToolInfo           *tool_info = CLAY_TOOL_INFO (list->data);
      ClayToolOptionsGUIFunc  options_gui_func;

      /*  copy all context properties except those the tool actually
       *  uses, because the subsequent deserialize() on the tool
       *  options will only set the properties that were set to
       *  non-default values at the time of saving, and we want to
       *  keep these default values as if they have been saved.
       * (see bug #541586).
       */
      clay_context_copy_properties (clay_get_user_context (clay),
                                    CLAY_CONTEXT (tool_info->tool_options),
                                    CLAY_CONTEXT_PROP_MASK_ALL &~
                                    (tool_info->context_props    |
                                     CLAY_CONTEXT_PROP_MASK_TOOL |
                                     CLAY_CONTEXT_PROP_MASK_PAINT_INFO));

      clay_tool_options_deserialize (tool_info->tool_options, NULL);

      options_gui_func = g_object_get_data (G_OBJECT (tool_info),
                                            "clay-tool-options-gui-func");

      if (! options_gui_func)
        options_gui_func = clay_tool_options_empty_gui;

      clay_tools_set_tool_options_gui_func (tool_info->tool_options,
                                            options_gui_func);
    }
}

void
clay_tools_save (Clay     *clay,
                 gboolean  save_tool_options,
                 gboolean  always_save)
{
  ClayConfigWriter *writer;
  GFile            *file;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (save_tool_options && (! tool_options_deleted || always_save))
    {
      GList  *list;
      GError *error = NULL;

      if (! clay_contexts_save (clay, &error))
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      if (! clay_internal_data_save (clay, &error))
        {
          clay_message_literal (clay, NULL, CLAY_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }

      clay_tool_options_create_folder ();

      for (list = clay_get_tool_info_iter (clay);
           list;
           list = g_list_next (list))
        {
          ClayToolInfo *tool_info = CLAY_TOOL_INFO (list->data);

          clay_tool_options_serialize (tool_info->tool_options, NULL);
        }
    }

  file = clay_directory_file ("toolrc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  writer = clay_config_writer_new_from_file (file, TRUE, "Clay toolrc", NULL);

  if (writer)
    {
      clay_tools_serialize (clay, clay->tool_item_list, writer);

      clay_config_writer_finish (writer, "end of toolrc", NULL);
    }

  g_object_unref (file);
}

gboolean
clay_tools_clear (Clay    *clay,
                  GError **error)
{
  GList    *list;
  gboolean  success = TRUE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  for (list = clay_get_tool_info_iter (clay);
       list && success;
       list = g_list_next (list))
    {
      ClayToolInfo *tool_info = CLAY_TOOL_INFO (list->data);

      success = clay_tool_options_delete (tool_info->tool_options, NULL);
    }

  if (success)
    success = clay_contexts_clear (clay, error);

  if (success)
    success = clay_internal_data_clear (clay, error);

  if (success)
    tool_options_deleted = TRUE;

  return success;
}

gboolean
clay_tools_serialize (Clay             *clay,
                      ClayContainer    *container,
                      ClayConfigWriter *writer)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (CLAY_IS_CONTAINER (container), FALSE);

  clay_config_writer_open (writer, "file-version");
  clay_config_writer_printf (writer, "%d", TOOL_RC_FILE_VERSION);
  clay_config_writer_close (writer);

  clay_config_writer_linefeed (writer);

  return clay_config_serialize (CLAY_CONFIG (container), writer, NULL);
}

gboolean
clay_tools_deserialize (Clay          *clay,
                        ClayContainer *container,
                        GScanner      *scanner)
{
  enum
  {
    FILE_VERSION = 1
  };

  ClayContainer *src_container;
  GTokenType     token;
  guint          scope_id;
  guint          old_scope_id;
  gint           file_version = 0;
  gboolean       result       = FALSE;

  scope_id     = g_type_qname (CLAY_TYPE_TOOL_GROUP);
  old_scope_id = g_scanner_set_scope (scanner, scope_id);

  g_scanner_scope_add_symbol (scanner, scope_id,
                              "file-version",
                              GINT_TO_POINTER (FILE_VERSION));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token &&
         (token != G_TOKEN_LEFT_PAREN                 ||
          ! file_version))
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case FILE_VERSION:
              token = G_TOKEN_INT;
              if (clay_scanner_parse_int (scanner, &file_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  g_scanner_set_scope (scanner, old_scope_id);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);

      return FALSE;
    }
  else if (file_version != TOOL_RC_FILE_VERSION)
    {
      g_scanner_error (scanner, "wrong toolrc file format version");

      return FALSE;
    }

  clay_container_freeze (container);

  /* make sure the various ClayToolItem types are registered */
  g_type_class_unref (g_type_class_ref (CLAY_TYPE_TOOL_GROUP));
  g_type_class_unref (g_type_class_ref (CLAY_TYPE_TOOL_INFO));

  clay_container_clear (container);

  src_container = g_object_new (CLAY_TYPE_LIST,
                                "child-type", CLAY_TYPE_TOOL_ITEM,
                                "append",     TRUE,
                                NULL);

  if (clay_config_deserialize (CLAY_CONFIG (src_container),
                               scanner, 0, NULL))
    {
      GHashTable *tools;
      GList      *list;

      result = TRUE;

      tools = g_hash_table_new (g_direct_hash, g_direct_equal);

      clay_tools_copy_structure (clay, src_container, container, tools);

      for (list = clay_get_tool_info_iter (clay);
           list;
           list = g_list_next (list))
        {
          ClayToolInfo *tool_info = list->data;

          if (! tool_info->hidden && ! g_hash_table_contains (tools, tool_info))
            {
              if (tool_info->experimental)
                {
                  /* if an experimental tool is not in the file, just add it to
                   * the tool-item list.
                   */
                  clay_container_add (container, CLAY_OBJECT (tool_info));
                }
              else
                {
                  /* otherwise, it means we added a new stable tool.  this must
                   * be the user toolrc file; rejct it, so that we fall back to
                   * the default toolrc file, which should contain the missing
                   * tool.
                   */
                  g_scanner_error (scanner, "missing tools in toolrc file");

                  result = FALSE;

                  break;
                }
            }
        }

      g_hash_table_unref (tools);
    }

  g_object_unref (src_container);

  clay_container_thaw (container);

  return result;
}

void
clay_tools_reset (Clay          *clay,
                  ClayContainer *container,
                  gboolean       user_toolrc)
{
  GList *files = NULL;
  GList *list;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_CONTAINER (container));

  if (user_toolrc)
    files = g_list_prepend (files, clay_directory_file       ("toolrc", NULL));
  files = g_list_prepend (files, clay_sysconf_directory_file ("toolrc", NULL));

  files = g_list_reverse (files);

  clay_container_freeze (container);

  clay_container_clear (container);

  for (list = files; list; list = g_list_next (list))
    {
      GScanner *scanner;
      GFile    *file  = list->data;
      GError   *error = NULL;

      if (clay->be_verbose)
        g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

      scanner = clay_scanner_new_file (file, &error);

      if (scanner && clay_tools_deserialize (clay, container, scanner))
        {
          clay_scanner_unref (scanner);

          break;
        }
      else
        {
          if (error->code != G_IO_ERROR_NOT_FOUND)
            {
              clay_message_literal (clay, NULL,
                                    CLAY_MESSAGE_WARNING, error->message);
            }

          g_clear_error (&error);

          clay_container_clear (container);
        }

      g_clear_pointer (&scanner, clay_scanner_unref);
    }

  g_list_free_full (files, g_object_unref);

  if (clay_container_is_empty (container))
    {
      if (clay->be_verbose)
        g_print ("Using default tool order\n");

      clay_tools_copy_structure (clay, clay->tool_info_list, container, NULL);
    }

  clay_container_thaw (container);
}


/*  private functions  */

static void
clay_tools_register (GType                   tool_type,
                     GType                   tool_options_type,
                     ClayToolOptionsGUIFunc  options_gui_func,
                     ClayContextPropMask     context_props,
                     const gchar            *identifier,
                     const gchar            *label,
                     const gchar            *tooltip,
                     const gchar            *menu_label,
                     const gchar            *menu_accel,
                     const gchar            *help_domain,
                     const gchar            *help_data,
                     const gchar            *icon_name,
                     gpointer                data)
{
  Clay         *clay = (Clay *) data;
  ClayToolInfo *tool_info;
  const gchar  *paint_core_name;
  gboolean      visible;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (g_type_is_a (tool_type, CLAY_TYPE_TOOL));
  g_return_if_fail (tool_options_type == G_TYPE_NONE ||
                    g_type_is_a (tool_options_type, CLAY_TYPE_TOOL_OPTIONS));

  if (tool_options_type == G_TYPE_NONE)
    tool_options_type = CLAY_TYPE_TOOL_OPTIONS;

  if (tool_type == CLAY_TYPE_PENCIL_TOOL)
    {
      paint_core_name = "clay-pencil";
    }
  else if (tool_type == CLAY_TYPE_PAINTBRUSH_TOOL)
    {
      paint_core_name = "clay-paintbrush";
    }
  else if (tool_type == CLAY_TYPE_ERASER_TOOL)
    {
      paint_core_name = "clay-eraser";
    }
  else if (tool_type == CLAY_TYPE_AIRBRUSH_TOOL)
    {
      paint_core_name = "clay-airbrush";
    }
  else if (tool_type == CLAY_TYPE_CLONE_TOOL)
    {
      paint_core_name = "clay-clone";
    }
  else if (tool_type == CLAY_TYPE_HEAL_TOOL)
    {
      paint_core_name = "clay-heal";
    }
  else if (tool_type == CLAY_TYPE_PERSPECTIVE_CLONE_TOOL)
    {
      paint_core_name = "clay-perspective-clone";
    }
  else if (tool_type == CLAY_TYPE_CONVOLVE_TOOL)
    {
      paint_core_name = "clay-convolve";
    }
  else if (tool_type == CLAY_TYPE_SMUDGE_TOOL)
    {
      paint_core_name = "clay-smudge";
    }
  else if (tool_type == CLAY_TYPE_DODGE_BURN_TOOL)
    {
      paint_core_name = "clay-dodge-burn";
    }
  else if (tool_type == CLAY_TYPE_INK_TOOL)
    {
      paint_core_name = "clay-ink";
    }
  else if (tool_type == CLAY_TYPE_MYBRUSH_TOOL)
    {
      paint_core_name = "clay-mybrush";
    }
  else
    {
      paint_core_name = "clay-paintbrush";
    }

  tool_info = clay_tool_info_new (clay,
                                  tool_type,
                                  tool_options_type,
                                  context_props,
                                  identifier,
                                  label,
                                  tooltip,
                                  menu_label,
                                  menu_accel,
                                  help_domain,
                                  help_data,
                                  paint_core_name,
                                  icon_name);

  visible = (! g_type_is_a (tool_type, CLAY_TYPE_FILTER_TOOL));

  clay_tool_item_set_visible (CLAY_TOOL_ITEM (tool_info), visible);

  /* hack to hide the operation tool entirely */
  if (tool_type == CLAY_TYPE_OPERATION_TOOL)
    tool_info->hidden = TRUE;

  /* hack to not require experimental tools to be present in toolrc */
  if (tool_type == CLAY_TYPE_N_POINT_DEFORMATION_TOOL ||
      tool_type == CLAY_TYPE_SEAMLESS_CLONE_TOOL      ||
      tool_type == CLAY_TYPE_PAINT_SELECT_TOOL)
    {
      tool_info->experimental = TRUE;
    }

  g_object_set_data (G_OBJECT (tool_info), "clay-tool-options-gui-func",
                     options_gui_func);

  clay_container_add (clay->tool_info_list, CLAY_OBJECT (tool_info));
  g_object_unref (tool_info);

  if (tool_type == CLAY_TYPE_PAINTBRUSH_TOOL)
    clay_tool_info_set_standard (clay, tool_info);
}

static void
clay_tools_copy_structure (Clay          *clay,
                           ClayContainer *src_container,
                           ClayContainer *dest_container,
                           GHashTable    *tools)
{
  GList *list;

  for (list = CLAY_LIST (src_container)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayToolItem *src_tool_item  = list->data;
      ClayToolItem *dest_tool_item = NULL;

      if (CLAY_IS_TOOL_GROUP (src_tool_item))
        {
          dest_tool_item = CLAY_TOOL_ITEM (clay_tool_group_new ());

          clay_tools_copy_structure (
            clay,
            clay_viewable_get_children (CLAY_VIEWABLE (src_tool_item)),
            clay_viewable_get_children (CLAY_VIEWABLE (dest_tool_item)),
            tools);

          clay_tool_group_set_active_tool (
            CLAY_TOOL_GROUP (dest_tool_item),
            clay_tool_group_get_active_tool (CLAY_TOOL_GROUP (src_tool_item)));
        }
      else
        {
          dest_tool_item = CLAY_TOOL_ITEM (
            clay_get_tool_info (clay, clay_object_get_name (src_tool_item)));

          if (dest_tool_item)
            {
              if (! CLAY_TOOL_INFO (dest_tool_item)->hidden)
                {
                  g_object_ref (dest_tool_item);

                  if (tools)
                    g_hash_table_add (tools, dest_tool_item);
                }
              else
                {
                  dest_tool_item = NULL;
                }
            }
        }

      if (dest_tool_item)
        {
          clay_tool_item_set_visible (
            dest_tool_item,
            clay_tool_item_get_visible (src_tool_item));

          clay_container_add (dest_container,
                              CLAY_OBJECT (dest_tool_item));

          g_object_unref (dest_tool_item);
        }
    }
}
