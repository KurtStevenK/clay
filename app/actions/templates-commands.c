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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage-new.h"
#include "core/claytemplate.h"

#include "widgets/claycontainerview.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claytemplateeditor.h"
#include "widgets/claytemplateview.h"
#include "widgets/claywidgets-utils.h"

#include "dialogs/dialogs.h"
#include "dialogs/template-options-dialog.h"

#include "actions.h"
#include "templates-commands.h"

#include "clay-intl.h"


typedef struct
{
  ClayContext   *context;
  ClayContainer *container;
  ClayTemplate  *template;
} TemplateDeleteData;


/*  local function prototypes  */

static void   templates_new_callback     (GtkWidget          *dialog,
                                          ClayTemplate       *template,
                                          ClayTemplate       *edit_template,
                                          ClayContext        *context,
                                          gpointer            user_data);
static void   templates_edit_callback    (GtkWidget          *dialog,
                                          ClayTemplate       *template,
                                          ClayTemplate       *edit_template,
                                          ClayContext        *context,
                                          gpointer            user_data);
static void   templates_delete_response  (GtkWidget          *dialog,
                                          gint                response_id,
                                          TemplateDeleteData *delete_data);
static void   templates_delete_data_free (TemplateDeleteData *delete_data);


/*  public functions */

void
templates_create_image_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  Clay                *clay;
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayTemplate        *template;
  return_if_no_clay (clay, data);

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  template = clay_context_get_template (context);

  if (template && clay_container_have (container, CLAY_OBJECT (template)))
    {
      GtkWidget *widget = GTK_WIDGET (editor);
      ClayImage *image;

      image = clay_image_new_from_template (clay, template, context);
      clay_create_display (clay, image, clay_template_get_unit (template), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (image);

      clay_image_new_set_last_template (clay, template);
    }
}

void
templates_new_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  GtkWidget           *dialog;

  context = clay_container_view_get_context (editor->view);

#define NEW_DIALOG_KEY "clay-template-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (context->clay), NEW_DIALOG_KEY);

  if (! dialog)
    {
      dialog = template_options_dialog_new (NULL, context,
                                            GTK_WIDGET (editor),
                                            _("New Template"),
                                            "clay-template-new",
                                            CLAY_ICON_TEMPLATE,
                                            _("Create a New Template"),
                                            CLAY_HELP_TEMPLATE_NEW,
                                            templates_new_callback,
                                            NULL);

      dialogs_attach_dialog (G_OBJECT (context->clay), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
templates_duplicate_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayTemplate        *template;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  template = clay_context_get_template (context);

  if (template && clay_container_have (container, CLAY_OBJECT (template)))
    {
      ClayTemplate *new_template;

      new_template = clay_config_duplicate (CLAY_CONFIG (template));

      clay_container_add (container, CLAY_OBJECT (new_template));
      clay_context_set_by_type (context,
                                clay_container_get_child_type (container),
                                CLAY_OBJECT (new_template));
      g_object_unref (new_template);

      templates_edit_cmd_callback (action, value, data);
    }
}

void
templates_edit_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayTemplate        *template;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  template = clay_context_get_template (context);

  if (template && clay_container_have (container, CLAY_OBJECT (template)))
    {
      GtkWidget *dialog;

#define EDIT_DIALOG_KEY "clay-template-edit-dialog"

      dialog = dialogs_get_dialog (G_OBJECT (template), EDIT_DIALOG_KEY);

      if (! dialog)
        {
          dialog = template_options_dialog_new (template, context,
                                                GTK_WIDGET (editor),
                                                _("Edit Template"),
                                                "clay-template-edit",
                                                CLAY_ICON_EDIT,
                                                _("Edit Template"),
                                                CLAY_HELP_TEMPLATE_EDIT,
                                                templates_edit_callback,
                                                NULL);

          dialogs_attach_dialog (G_OBJECT (template), EDIT_DIALOG_KEY, dialog);
        }

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
templates_delete_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;
  ClayContext         *context;
  ClayTemplate        *template;

  container = clay_container_view_get_container (editor->view);
  context   = clay_container_view_get_context (editor->view);

  template = clay_context_get_template (context);

  if (template && clay_container_have (container, CLAY_OBJECT (template)))
    {
      TemplateDeleteData *delete_data = g_slice_new (TemplateDeleteData);
      GtkWidget          *dialog;

      delete_data->context   = context;
      delete_data->container = container;
      delete_data->template  = template;

      dialog =
        clay_message_dialog_new (_("Delete Template"), CLAY_ICON_EDIT_DELETE,
                                 GTK_WIDGET (editor), 0,
                                 clay_standard_help_func, NULL,

                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                 _("_Delete"), GTK_RESPONSE_OK,

                                 NULL);

      clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                               GTK_RESPONSE_OK,
                                               GTK_RESPONSE_CANCEL,
                                               -1);

      g_object_weak_ref (G_OBJECT (dialog),
                         (GWeakNotify) templates_delete_data_free, delete_data);

      g_signal_connect_object (template, "disconnect",
                               G_CALLBACK (gtk_widget_destroy),
                               dialog, G_CONNECT_SWAPPED);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (templates_delete_response),
                        delete_data);

      clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                         _("Are you sure you want to delete "
                                           "template '%s' from the list and "
                                           "from disk?"),
                                         clay_object_get_name (template));
      gtk_widget_set_visible (dialog, TRUE);
    }
}


/*  private functions  */

static void
templates_new_callback (GtkWidget    *dialog,
                        ClayTemplate *template,
                        ClayTemplate *edit_template,
                        ClayContext  *context,
                        gpointer      user_data)
{
  clay_container_add (context->clay->templates, CLAY_OBJECT (edit_template));
  clay_context_set_template (clay_get_user_context (context->clay),
                             edit_template);

  gtk_widget_destroy (dialog);
}

static void
templates_edit_callback (GtkWidget    *dialog,
                         ClayTemplate *template,
                         ClayTemplate *edit_template,
                         ClayContext  *context,
                         gpointer      user_data)
{
  clay_config_sync (G_OBJECT (edit_template),
                    G_OBJECT (template), 0);

  gtk_widget_destroy (dialog);
}

static void
templates_delete_response (GtkWidget          *dialog,
                           gint                response_id,
                           TemplateDeleteData *delete_data)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      ClayObject *new_active = NULL;

      if (delete_data->template ==
          clay_context_get_template (delete_data->context))
        {
          new_active = clay_container_get_neighbor_of (delete_data->container,
                                                       CLAY_OBJECT (delete_data->template));
        }

      if (clay_container_have (delete_data->container,
                               CLAY_OBJECT (delete_data->template)))
        {
          if (new_active)
            clay_context_set_by_type (delete_data->context,
                                      clay_container_get_child_type (delete_data->container),
                                      new_active);

          clay_container_remove (delete_data->container,
                                 CLAY_OBJECT (delete_data->template));
        }
    }

  gtk_widget_destroy (dialog);
}

static void
templates_delete_data_free (TemplateDeleteData *delete_data)
{
  g_slice_free (TemplateDeleteData, delete_data);
}
