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

#include "actions-types.h"

#include "config/clayrc.h"
#include "core/clay.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "actions.h"
#include "dialogs-commands.h"

#include "clay-intl.h"


/*  public functions  */

void
dialogs_create_toplevel_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  GtkWidget   *widget;
  const gchar *identifier;
  return_if_no_widget (widget, data);

  identifier = g_variant_get_string (value, NULL);

  if (identifier)
    clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                    clay_widget_get_monitor (widget),
                                    NULL /*ui_manager*/,
                                    widget,
                                    identifier, -1, TRUE);
}

void
dialogs_create_dockable_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  const gchar *identifier;
  return_if_no_clay   (clay, data);
  return_if_no_widget (widget, data);

  identifier = g_variant_get_string (value, NULL);

  if (identifier)
    clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                               clay,
                                               clay_dialog_factory_get_singleton (),
                                               clay_widget_get_monitor (widget),
                                               identifier);
}

void
dialogs_language_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  Clay        *clay;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  const gchar *code;
  gchar       *current = NULL;

  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  code = g_variant_get_string (value, NULL);
  if (code && g_strcmp0 (code, "system") == 0)
    code = NULL;

  g_object_get (clay->edit_config, "language", &current, NULL);
  if (g_strcmp0 (current, code) == 0)
    {
      g_free (current);
      return;
    }
  g_free (current);

  g_object_set (clay->edit_config, "language", code, NULL);

  /* Language is a restart property, so it is not copied onto the
   * running config. Write clayrc now. An idle autosave can be skipped
   * when preferences are open, or lost if the process exits first.
   */
  clay_rc_save (CLAY_RC (clay->edit_config));

  dialog = gtk_message_dialog_new (GTK_IS_WINDOW (gtk_widget_get_toplevel (widget)) ?
                                   GTK_WINDOW (gtk_widget_get_toplevel (widget)) : NULL,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_INFO,
                                   GTK_BUTTONS_OK,
                                   _("Language saved"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("Clay will use this language the next time it starts."));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
