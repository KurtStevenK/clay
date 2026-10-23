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

#include "dialogs-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydata.h"
#include "core/claydatafactory.h"

#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"

#include "data-delete-dialog.h"

#include "clay-intl.h"


typedef struct _DataDeleteDialog DataDeleteDialog;

struct _DataDeleteDialog
{
  ClayDataFactory *factory;
  ClayData        *data;
  ClayContext     *context;
  GtkWidget       *parent;
};


/*  local function prototypes  */

static void  data_delete_dialog_response (GtkWidget        *dialog,
                                          gint              response_id,
                                          DataDeleteDialog *private);


/*  public functions  */

GtkWidget *
data_delete_dialog_new (ClayDataFactory *factory,
                        ClayData        *data,
                        ClayContext     *context,
                        GtkWidget       *parent)
{
  DataDeleteDialog *private;
  GtkWidget        *dialog;

  g_return_val_if_fail (CLAY_IS_DATA_FACTORY (factory), NULL);
  g_return_val_if_fail (CLAY_IS_DATA (data), NULL);
  g_return_val_if_fail (context == NULL || CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (DataDeleteDialog);

  private->factory = factory;
  private->data    = data;
  private->context = context;
  private->parent  = parent;

  dialog = clay_message_dialog_new (_("Delete Object"),
                                    CLAY_ICON_EDIT_DELETE,
                                    gtk_widget_get_toplevel (parent), 0,
                                    clay_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("_Delete"), GTK_RESPONSE_OK,

                                    NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (data, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (data_delete_dialog_response),
                    private);

  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("Delete '%s'?"),
                                     clay_object_get_name (data));
  clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                             _("Are you sure you want to remove '%s' "
                               "from the list and delete it on disk?"),
                             clay_object_get_name (data));

  return dialog;
}


/*  private functions  */

static void
data_delete_dialog_response (GtkWidget        *dialog,
                             gint              response_id,
                             DataDeleteDialog *private)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      ClayDataFactory *factory    = private->factory;
      ClayData        *data       = private->data;
      ClayContainer   *container;
      ClayObject      *new_active = NULL;
      GError          *error      = NULL;

      container = clay_data_factory_get_container (factory);

      if (private->context &&
          CLAY_OBJECT (data) ==
          clay_context_get_by_type (private->context,
                                    clay_container_get_child_type (container)))
        {
          new_active = clay_container_get_neighbor_of (container,
                                                       CLAY_OBJECT (data));
        }

      if (! clay_data_factory_data_delete (factory, data, TRUE, &error))
        {
          clay_message (clay_data_factory_get_clay (factory),
                        G_OBJECT (private->parent), CLAY_MESSAGE_ERROR,
                        "%s", error->message);
          g_clear_error (&error);
        }

      if (new_active)
        clay_context_set_by_type (private->context,
                                  clay_container_get_child_type (container),
                                  new_active);
    }

  g_slice_free (DataDeleteDialog, private);
}
