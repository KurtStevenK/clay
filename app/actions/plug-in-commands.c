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

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/clay-filter-history.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayitem.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "pdb/clayprocedure.h"

#include "plug-in/claypluginmanager.h"
#include "plug-in/claypluginmanager-data.h"

#include "widgets/claybufferview.h"
#include "widgets/claycontainerview.h"
#include "widgets/claydatafactoryview.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayimageeditor.h"
#include "widgets/clayitemtreeview.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "plug-in-commands.h"
#include "procedure-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   plug_in_reset_all_response (GtkWidget *dialog,
                                          gint       response_id,
                                          Clay      *clay);


/*  public functions  */

void
plug_in_run_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  Clay           *clay;
  ClayValueArray *args    = NULL;
  ClayDisplay    *display = NULL;
  ClayProcedure  *procedure;
  gsize           hack;
  return_if_no_clay (clay, data);

  hack = g_variant_get_uint64 (value);

  procedure = GSIZE_TO_POINTER (hack);

  switch (procedure->proc_type)
    {
    case CLAY_PDB_PROC_TYPE_PERSISTENT:
      args = procedure_commands_get_run_mode_arg (procedure);
      break;

    case CLAY_PDB_PROC_TYPE_PLUGIN:
    case CLAY_PDB_PROC_TYPE_TEMPORARY:
      if (CLAY_IS_DATA_FACTORY_VIEW (data) ||
          CLAY_IS_BUFFER_VIEW (data))
        {
          ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
          ClayContainer       *container;
          ClayContext         *context;
          ClayObject          *object;

          container = clay_container_view_get_container (editor->view);
          context   = clay_container_view_get_context (editor->view);

          object = clay_context_get_by_type (context,
                                             clay_container_get_child_type (container));

          args = procedure_commands_get_data_args (procedure, object);
        }
      else if (CLAY_IS_IMAGE_EDITOR (data))
        {
          ClayImageEditor *editor = CLAY_IMAGE_EDITOR (data);
          ClayImage       *image;

          image = clay_image_editor_get_image (editor);

          args = procedure_commands_get_image_args (procedure, image);
        }
      else if (CLAY_IS_ITEM_TREE_VIEW (data))
        {
          ClayItemTreeView *view = CLAY_ITEM_TREE_VIEW (data);
          ClayImage        *image;
          GList            *items = NULL;

          image = clay_item_tree_view_get_image (view);

          if (image)
            {
              GType item_type = CLAY_ITEM_TREE_VIEW_GET_CLASS (view)->item_type;

              items = clay_image_get_selected_items (image, item_type);
            }

          args = procedure_commands_get_items_args (procedure, image, items);
        }
      else
        {
          display = action_data_get_display (data);

          args = procedure_commands_get_display_args (procedure, display, NULL);
        }
      break;

    case CLAY_PDB_PROC_TYPE_INTERNAL:
      g_warning ("Unhandled procedure type.");
      break;
    }

  if (args)
    {
      if (procedure_commands_run_procedure_async (procedure, clay,
                                                  CLAY_PROGRESS (display),
                                                  CLAY_RUN_INTERACTIVE, args,
                                                  display))
        {
          /* remember only image plug-ins */
          if (procedure->num_args >= 2 &&
              CLAY_IS_PARAM_SPEC_IMAGE (procedure->args[1]))
            {
              clay_filter_history_add (clay, procedure);
            }
        }

      clay_value_array_unref (args);
    }
}

void
plug_in_reset_all_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  Clay      *clay;
  GtkWidget *dialog;
  return_if_no_clay (clay, data);

#define RESET_FILTERS_DIALOG_KEY "clay-reset-all-filters-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (clay), RESET_FILTERS_DIALOG_KEY);

  if (! dialog)
    {
      dialog = clay_message_dialog_new (_("Reset all Filters"),
                                        CLAY_ICON_DIALOG_QUESTION,
                                        NULL, 0,
                                        clay_standard_help_func, NULL,

                                        _("_Cancel"), GTK_RESPONSE_CANCEL,
                                        _("_Reset"),  GTK_RESPONSE_OK,

                                        NULL);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (plug_in_reset_all_response),
                        clay);

      clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                         _("Do you really want to reset all "
                                           "filters to default values?"));

      dialogs_attach_dialog (G_OBJECT (clay), RESET_FILTERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
plug_in_reset_all_response (GtkWidget *dialog,
                            gint       response_id,
                            Clay      *clay)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    clay_plug_in_manager_data_free (clay->plug_in_manager);
}
