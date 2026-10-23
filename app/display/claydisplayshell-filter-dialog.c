/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1999 Manish Singh
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

#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "display-types.h"

#include "core/clay.h"
#include "core/clayviewable.h"

#include "widgets/claycolordisplayeditor.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayviewabledialog.h"

#include "claydisplay.h"
#include "claydisplayshell.h"
#include "claydisplayshell-filter.h"
#include "claydisplayshell-filter-dialog.h"

#include "clay-intl.h"


typedef struct
{
  ClayDisplayShell      *shell;
  GtkWidget             *dialog;

  ClayColorDisplayStack *old_stack;
} ColorDisplayDialog;


/*  local function prototypes  */

static void clay_display_shell_filter_dialog_response (GtkWidget          *widget,
                                                       gint                response_id,
                                                       ColorDisplayDialog *cdd);

static void clay_display_shell_filter_dialog_free     (ColorDisplayDialog *cdd);


/*  public functions  */

GtkWidget *
clay_display_shell_filter_dialog_new (ClayDisplayShell *shell)
{
  ClayImage          *image;
  ColorDisplayDialog *cdd;
  GtkWidget          *editor;

  g_return_val_if_fail (CLAY_IS_DISPLAY_SHELL (shell), NULL);

  image = clay_display_get_image (shell->display);

  cdd = g_slice_new0 (ColorDisplayDialog);

  cdd->shell  = shell;
  cdd->dialog = clay_viewable_dialog_new (g_list_prepend (NULL, image),
                                          clay_get_user_context (shell->display->clay),
                                          _("Color Display Filters"),
                                          "clay-display-filters",
                                          CLAY_ICON_DISPLAY_FILTER,
                                          _("Configure Color Display Filters"),
                                          GTK_WIDGET (cdd->shell),
                                          clay_standard_help_func,
                                          CLAY_HELP_DISPLAY_FILTER_DIALOG,

                                          _("_Cancel"), GTK_RESPONSE_CANCEL,
                                          _("_OK"),     GTK_RESPONSE_OK,

                                          NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (cdd->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_destroy_with_parent (GTK_WINDOW (cdd->dialog), TRUE);

  g_object_weak_ref (G_OBJECT (cdd->dialog),
                     (GWeakNotify) clay_display_shell_filter_dialog_free, cdd);

  g_signal_connect (cdd->dialog, "response",
                    G_CALLBACK (clay_display_shell_filter_dialog_response),
                    cdd);

  if (shell->filter_stack)
    {
      cdd->old_stack = clay_color_display_stack_clone (shell->filter_stack);

      g_object_weak_ref (G_OBJECT (cdd->dialog),
                         (GWeakNotify) g_object_unref, cdd->old_stack);
    }
  else
    {
      ClayColorDisplayStack *stack = clay_color_display_stack_new ();

      clay_display_shell_filter_set (shell, stack);
      g_object_unref (stack);
    }

  editor = clay_color_display_editor_new (shell->display->clay,
                                          shell->filter_stack,
                                          clay_display_shell_get_color_config (shell),
                                          CLAY_COLOR_MANAGED (shell));
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (cdd->dialog))),
                      editor, TRUE, TRUE, 0);
  gtk_widget_set_visible (editor, TRUE);

  return cdd->dialog;
}


/*  private functions  */

static void
clay_display_shell_filter_dialog_response (GtkWidget          *widget,
                                           gint                response_id,
                                           ColorDisplayDialog *cdd)
{
  if (response_id != GTK_RESPONSE_OK)
    clay_display_shell_filter_set (cdd->shell, cdd->old_stack);

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
}

static void
clay_display_shell_filter_dialog_free (ColorDisplayDialog *cdd)
{
  g_slice_free (ColorDisplayDialog, cdd);
}
