/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * fill-dialog.c
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
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

#include "dialogs-types.h"

#include "core/claydrawable.h"
#include "core/clayfilloptions.h"

#include "widgets/clayfilleditor.h"
#include "widgets/clayviewabledialog.h"

#include "fill-dialog.h"

#include "clay-intl.h"


#define RESPONSE_RESET 1


typedef struct _FillDialog FillDialog;

struct _FillDialog
{
  GList            *items;
  GList            *drawables;
  ClayContext      *context;
  ClayFillOptions  *options;
  ClayFillCallback  callback;
  gpointer          user_data;
};


/*  local function prototypes  */

static void  fill_dialog_free     (FillDialog *private);
static void  fill_dialog_response (GtkWidget  *dialog,
                                   gint        response_id,
                                   FillDialog *private);


/*  public function  */

GtkWidget *
fill_dialog_new (GList            *items,
                 GList            *drawables,
                 ClayContext      *context,
                 const gchar      *title,
                 const gchar      *icon_name,
                 const gchar      *help_id,
                 GtkWidget        *parent,
                 ClayFillOptions  *options,
                 ClayFillCallback  callback,
                 gpointer          user_data)
{
  FillDialog *private;
  GtkWidget  *dialog;
  GtkWidget  *main_vbox;
  GtkWidget  *fill_editor;

  g_return_val_if_fail (items, NULL);
  g_return_val_if_fail (drawables, NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (FillDialog);

  private->items     = g_list_copy (items);
  private->drawables = g_list_copy (drawables);
  private->context   = context;
  private->options   = clay_fill_options_new (context->clay, context, TRUE);
  private->callback  = callback;
  private->user_data = user_data;

  clay_config_sync (G_OBJECT (options),
                    G_OBJECT (private->options), 0);

  dialog = clay_viewable_dialog_new (g_list_copy (items), context,
                                     title, "clay-fill-options",
                                     icon_name,
                                     _("Choose Fill Style"),
                                     parent,
                                     clay_standard_help_func,
                                     help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Fill"),   GTK_RESPONSE_OK,

                                     NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) fill_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (fill_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);

  fill_editor = clay_fill_editor_new (private->options, FALSE, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), fill_editor, FALSE, FALSE, 0);
  gtk_widget_set_visible (fill_editor, TRUE);

  return dialog;
}


/*  private functions  */

static void
fill_dialog_free (FillDialog *private)
{
  g_object_unref (private->options);
  g_list_free (private->drawables);
  g_list_free (private->items);

  g_slice_free (FillDialog, private);
}

static void
fill_dialog_response (GtkWidget  *dialog,
                      gint        response_id,
                      FillDialog *private)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      clay_config_reset (CLAY_CONFIG (private->options));
      break;

    case GTK_RESPONSE_OK:
      private->callback (dialog,
                         private->items,
                         private->drawables,
                         private->context,
                         private->options,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}
