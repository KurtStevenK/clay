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
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydata.h"
#include "core/claydatafactory.h"

#include "file/file-open.h"

#include "widgets/clayclipboard.h"
#include "widgets/claycontainerview.h"
#include "widgets/claydataeditor.h"
#include "widgets/claydatafactoryview.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"
#include "widgets/claywidgets-utils.h"

#include "dialogs/data-delete-dialog.h"

#include "actions.h"
#include "data-commands.h"

#include "clay-intl.h"


/*  public functions  */

void
data_open_as_image_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context =
    clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data && clay_data_get_file (data))
    {
      GFile             *file   = clay_data_get_file (data);
      GtkWidget         *widget = GTK_WIDGET (view);
      ClayImage         *image;
      ClayPDBStatusType  status;
      GError            *error = NULL;

      image = file_open_with_display (context->clay, context, NULL,
                                      file, FALSE,
                                      G_OBJECT (clay_widget_get_monitor (widget)),
                                      &status, &error);

      if (! image && status != CLAY_PDB_CANCEL)
        {
          clay_message (context->clay, G_OBJECT (view),
                        CLAY_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        clay_file_get_utf8_name (file), error->message);
          g_clear_error (&error);
        }
    }
}

void
data_new_cmd_callback (ClayAction *action,
                       GVariant   *value,
                       gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);

  if (clay_data_factory_view_has_data_new_func (view))
    {
      ClayDataFactory *factory;
      ClayContext     *context;
      ClayData        *data;

      factory = clay_data_factory_view_get_data_factory (view);

      context =
        clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

      data = clay_data_factory_data_new (factory, context, _("Untitled"));

      if (data)
        {
          GtkWidget *edit_button;

          clay_context_set_by_type (context,
                                    clay_data_factory_view_get_child_type (view),
                                    CLAY_OBJECT (data));

          edit_button = clay_data_factory_view_get_edit_button (view);
          if (edit_button && gtk_widget_get_visible (edit_button))
            gtk_button_clicked (GTK_BUTTON (clay_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_duplicate_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context = clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data && clay_data_factory_view_have (view, CLAY_OBJECT (data)))
    {
      ClayData *new_data;

      new_data = clay_data_factory_data_duplicate (clay_data_factory_view_get_data_factory (view), data);

      if (new_data)
        {
          GtkWidget *edit_button;

          clay_context_set_by_type (context,
                                    clay_data_factory_view_get_child_type (view),
                                    CLAY_OBJECT (new_data));

          edit_button = clay_data_factory_view_get_edit_button (view);
          if (edit_button && gtk_widget_get_visible (edit_button))
            gtk_button_clicked (GTK_BUTTON (clay_data_factory_view_get_edit_button (view)));
        }
    }
}

void
data_copy_location_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context = clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data)
    {
      GFile *file = clay_data_get_file (data);

      if (file)
        {
          gchar *uri = g_file_get_uri (file);

          clay_clipboard_set_text (context->clay, uri);
          g_free (uri);
        }
    }
}

void
data_show_in_file_manager_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context = clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data)
    {
      GFile *file = clay_data_get_file (data);

      if (file)
        {
          GError *error = NULL;

          if (! clay_file_show_in_file_manager (file, &error))
            {
              clay_message (context->clay, G_OBJECT (view),
                            CLAY_MESSAGE_ERROR,
                            _("Can't show file in file manager: %s"),
                            error->message);
              g_clear_error (&error);
            }
        }
    }
}

void
data_delete_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context =
    clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data                          &&
      clay_data_is_deletable (data) &&
      clay_data_factory_view_have (view, CLAY_OBJECT (data)))
    {
      ClayDataFactory *factory;
      GtkWidget       *dialog;

      factory = clay_data_factory_view_get_data_factory (view);

      dialog = data_delete_dialog_new (factory, data, context,
                                       GTK_WIDGET (view));
      gtk_widget_set_visible (dialog, TRUE);
    }
}

void
data_refresh_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  Clay                *clay;
  return_if_no_clay (clay, user_data);

  clay_set_busy (clay);
  clay_data_factory_data_refresh (clay_data_factory_view_get_data_factory (view),
                                  action_data_get_context (user_data));
  clay_unset_busy (clay);
}

void
data_edit_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    user_data)
{
  ClayDataFactoryView *view = CLAY_DATA_FACTORY_VIEW (user_data);
  ClayContext         *context;
  ClayData            *data;

  context = clay_container_view_get_context (CLAY_CONTAINER_EDITOR (view)->view);

  data = (ClayData *)
    clay_context_get_by_type (context,
                              clay_data_factory_view_get_child_type (view));

  if (data && clay_data_factory_view_have (view, CLAY_OBJECT (data)))
    {
      GdkMonitor *monitor = clay_widget_get_monitor (GTK_WIDGET (view));
      GtkWidget  *dockable;
      GtkWidget  *editor  = NULL;

      dockable =
        clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (context->clay)),
                                                   context->clay,
                                                   clay_dialog_factory_get_singleton (),
                                                   monitor,
                                                   g_variant_get_string (value,
                                                                         NULL));

      if (dockable)
        editor = gtk_bin_get_child (GTK_BIN (dockable));

      if (editor && CLAY_IS_DATA_EDITOR (editor))
        clay_data_editor_set_data (CLAY_DATA_EDITOR (editor), data);
    }
}
