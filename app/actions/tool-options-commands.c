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

#include "actions-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claydatafactory.h"
#include "core/claytoolinfo.h"
#include "core/claytooloptions.h"
#include "core/claytoolpreset.h"

#include "widgets/claydataeditor.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayeditor.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claytooloptionseditor.h"
#include "widgets/clayuimanager.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "dialogs/data-delete-dialog.h"

#include "tool-options-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   tool_options_show_preset_editor (Clay           *clay,
                                               ClayEditor     *editor,
                                               ClayToolPreset *preset);


/*  public functions  */

void
tool_options_save_new_preset_cmd_callback (ClayAction *action,
                                           GVariant   *value,
                                           gpointer    user_data)
{
  ClayEditor  *editor  = CLAY_EDITOR (user_data);
  Clay        *clay    = clay_editor_get_ui_manager (editor)->clay;
  ClayContext *context = clay_get_user_context (clay);
  ClayData    *data;

  data = clay_data_factory_data_new (context->clay->tool_preset_factory,
                                     context, _("Untitled"));

  tool_options_show_preset_editor (clay, editor, CLAY_TOOL_PRESET (data));
}

void
tool_options_save_preset_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayEditor     *editor    = CLAY_EDITOR (data);
  Clay           *clay      = clay_editor_get_ui_manager (editor)->clay;
  ClayContext    *context   = clay_get_user_context (clay);
  ClayToolInfo   *tool_info = clay_context_get_tool (context);
  ClayToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (ClayToolPreset *)
    clay_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      clay_config_sync (G_OBJECT (tool_info->tool_options),
                        G_OBJECT (preset->tool_options), 0);

      tool_options_show_preset_editor (clay, editor, preset);
    }
}

void
tool_options_restore_preset_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayEditor     *editor    = CLAY_EDITOR (data);
  Clay           *clay      = clay_editor_get_ui_manager (editor)->clay;
  ClayContext    *context   = clay_get_user_context (clay);
  ClayToolInfo   *tool_info = clay_context_get_tool (context);
  ClayToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (ClayToolPreset *)
    clay_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      if (clay_context_get_tool_preset (context) != preset)
        clay_context_set_tool_preset (context, preset);
      else
        clay_context_tool_preset_changed (context);
    }
}

void
tool_options_edit_preset_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayEditor     *editor    = CLAY_EDITOR (data);
  Clay           *clay      = clay_editor_get_ui_manager (editor)->clay;
  ClayContext    *context   = clay_get_user_context (clay);
  ClayToolInfo   *tool_info = clay_context_get_tool (context);
  ClayToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (ClayToolPreset *)
    clay_container_get_child_by_index (tool_info->presets, index);

  if (preset)
    {
      tool_options_show_preset_editor (clay, editor, preset);
    }
}

void
tool_options_delete_preset_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayEditor     *editor    = CLAY_EDITOR (data);
  ClayContext    *context   = clay_get_user_context (clay_editor_get_ui_manager (editor)->clay);
  ClayToolInfo   *tool_info = clay_context_get_tool (context);
  ClayToolPreset *preset;
  gint            index;

  index = g_variant_get_int32 (value);

  preset = (ClayToolPreset *)
    clay_container_get_child_by_index (tool_info->presets, index);

  if (preset &&
      clay_data_is_deletable (CLAY_DATA (preset)))
    {
      ClayDataFactory *factory = context->clay->tool_preset_factory;
      GtkWidget       *dialog;

      dialog = data_delete_dialog_new (factory, CLAY_DATA (preset), NULL,
                                       GTK_WIDGET (editor));
      gtk_widget_set_visible (dialog, TRUE);
    }
}

void
tool_options_reset_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayEditor   *editor    = CLAY_EDITOR (data);
  ClayContext  *context   = clay_get_user_context (clay_editor_get_ui_manager (editor)->clay);
  ClayToolInfo *tool_info = clay_context_get_tool (context);

  clay_config_reset (CLAY_CONFIG (tool_info->tool_options));
}

void
tool_options_reset_all_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayEditor *editor = CLAY_EDITOR (data);
  GtkWidget  *dialog;

  dialog = clay_message_dialog_new (_("Reset All Tool Options"),
                                    CLAY_ICON_DIALOG_QUESTION,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    clay_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Reset"),  GTK_RESPONSE_OK,

                                    NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("Do you really want to reset all "
                                       "tool options to default values?"));

  if (clay_dialog_run (CLAY_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      Clay  *clay = clay_editor_get_ui_manager (editor)->clay;
      GList *list;

      for (list = clay_get_tool_info_iter (clay);
           list;
           list = g_list_next (list))
        {
          ClayToolInfo *tool_info = list->data;

          clay_config_reset (CLAY_CONFIG (tool_info->tool_options));
        }
    }

  gtk_widget_destroy (dialog);
}


/*  private functions  */

static void
tool_options_show_preset_editor (Clay           *clay,
                                 ClayEditor     *editor,
                                 ClayToolPreset *preset)
{
  GtkWidget *dockable;

  dockable =
    clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                               clay,
                                               clay_dialog_factory_get_singleton (),
                                               clay_widget_get_monitor (GTK_WIDGET (editor)),
                                               "clay-tool-preset-editor");

  clay_data_editor_set_data (CLAY_DATA_EDITOR (gtk_bin_get_child (GTK_BIN (dockable))),
                             CLAY_DATA (preset));
}
