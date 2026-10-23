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

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/claypalette.h"

#include "widgets/claycontainerview.h"
#include "widgets/claydatafactoryview.h"
#include "widgets/claydialogfactory.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/clayview.h"
#include "widgets/claywidgets-utils.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "palettes-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   palettes_merge_callback (GtkWidget   *widget,
                                       const gchar *palette_name,
                                       gpointer     data);


/*  public functions */

void
palettes_import_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                  clay_widget_get_monitor (widget),
                                  NULL /*ui_manager*/,
                                  widget,
                                  "clay-palette-import-dialog", -1, TRUE);
}

void
palettes_merge_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  GtkWidget           *dialog;

#define MERGE_DIALOG_KEY "clay-palettes-merge-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (editor), MERGE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = clay_query_string_box (_("Merge Palettes"),
                                      GTK_WIDGET (editor),
                                      clay_standard_help_func,
                                      CLAY_HELP_PALETTE_MERGE,
                                      _("Enter a name for the merged palette"),
                                      NULL,
                                      G_OBJECT (editor), "destroy",
                                      palettes_merge_callback,
                                      editor, NULL);

      dialogs_attach_dialog (G_OBJECT (editor), MERGE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
palettes_merge_callback (GtkWidget   *widget,
                         const gchar *palette_name,
                         gpointer     data)
{
  ClayContainerEditor *editor = data;
  ClayDataFactoryView *view   = data;
  ClayDataFactory     *factory;
  ClayContext         *context;
  ClayPalette         *new_palette;
  GList               *selected = NULL;
  GList               *list;

  context = clay_container_view_get_context (editor->view);
  factory = clay_data_factory_view_get_data_factory (view);

  if (clay_container_view_get_selected (editor->view, &selected) < 2)
    {
      clay_message_literal (context->clay,
                            G_OBJECT (editor), CLAY_MESSAGE_WARNING,
                            _("There must be at least two palettes selected "
                              "to merge."));
      g_list_free (selected);
      return;
    }

  new_palette = CLAY_PALETTE (clay_data_factory_data_new (factory, context,
                                                          palette_name));

  for (list = selected; list; list = g_list_next (list))
    {
      ClayPalette *palette = list->data;
      GList       *cols;

      for (cols = clay_palette_get_colors (palette);
           cols;
           cols = g_list_next (cols))
        {
          ClayPaletteEntry *entry = cols->data;

          clay_palette_add_entry (new_palette, -1,
                                  entry->name,
                                  entry->color);
        }
    }

  g_list_free (selected);
}
