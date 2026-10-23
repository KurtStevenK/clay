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

#include "libclaybase/claybase.h"
#include "libclaythumb/claythumb.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimagefile.h"
#include "core/clayimage.h"

#include "file/file-open.h"

#include "widgets/clayclipboard.h"
#include "widgets/claycontainerview.h"
#include "widgets/claycontainerview-utils.h"
#include "widgets/claydocumentview.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplay-foreach.h"
#include "display/claydisplayshell.h"

#include "documents-commands.h"
#include "file-commands.h"

#include "clay-intl.h"


typedef struct
{
  const gchar *name;
  gboolean     found;
} RaiseClosure;


/*  local function prototypes  */

static void   documents_open_image    (GtkWidget     *editor,
                                       ClayContext   *context,
                                       ClayImagefile *imagefile);
static void   documents_raise_display (ClayDisplay   *display,
                                       RaiseClosure  *closure);



/*  public functions */

void
documents_open_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayContainer       *container;
  ClayImagefile       *imagefile;
  GList               *images = NULL;
  GList               *list;

  context   = clay_container_view_get_context (editor->view);
  container = clay_container_view_get_container (editor->view);

  imagefile = clay_context_get_imagefile (context);

  if (clay_container_view_get_selected (editor->view, &images) > 1)
    {
      for (list = images; list; list = list->next)
        documents_open_image (GTK_WIDGET (editor), context,
                              CLAY_IMAGEFILE (list->data));

      g_list_free (list);
    }
  else
    {
      if (imagefile && clay_container_have (container, CLAY_OBJECT (imagefile)))
        {
          documents_open_image (GTK_WIDGET (editor), context, imagefile);
        }
      else
        {
          file_file_open_dialog (context->clay, NULL, GTK_WIDGET (editor));
        }
    }
  if (images)
    g_list_free (images);
}

void
documents_raise_or_open_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayContainer       *container;
  ClayImagefile       *imagefile;

  context   = clay_container_view_get_context (editor->view);
  container = clay_container_view_get_container (editor->view);

  imagefile = clay_context_get_imagefile (context);

  if (imagefile && clay_container_have (container, CLAY_OBJECT (imagefile)))
    {
      RaiseClosure closure;

      closure.name  = clay_object_get_name (imagefile);
      closure.found = FALSE;

      clay_container_foreach (CLAY_CONTAINER (context->clay->displays),
                              (GFunc) documents_raise_display,
                              &closure);

      if (! closure.found)
        documents_open_image (GTK_WIDGET (editor), context, imagefile);
    }
}

void
documents_file_open_dialog_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayContainer       *container;
  ClayImagefile       *imagefile;

  context   = clay_container_view_get_context (editor->view);
  container = clay_container_view_get_container (editor->view);

  imagefile = clay_context_get_imagefile (context);

  if (imagefile && clay_container_have (container, CLAY_OBJECT (imagefile)))
    {
      file_file_open_dialog (context->clay,
                             clay_imagefile_get_file (imagefile),
                             GTK_WIDGET (editor));
    }
}

void
documents_copy_location_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayImagefile       *imagefile;

  context   = clay_container_view_get_context (editor->view);
  imagefile = clay_context_get_imagefile (context);

  if (imagefile)
    clay_clipboard_set_text (context->clay,
                             clay_object_get_name (imagefile));
}

void
documents_show_in_file_manager_cmd_callback (ClayAction *action,
                                             GVariant   *value,
                                             gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayImagefile       *imagefile;

  context   = clay_container_view_get_context (editor->view);
  imagefile = clay_context_get_imagefile (context);

  if (imagefile)
    {
      GFile  *file  = g_file_new_for_uri (clay_object_get_name (imagefile));
      GError *error = NULL;

      if (! clay_file_show_in_file_manager (file, &error))
        {
          clay_message (context->clay, G_OBJECT (editor),
                        CLAY_MESSAGE_ERROR,
                        _("Can't show file in file manager: %s"),
                        error->message);
          g_clear_error (&error);
        }

      g_object_unref (file);
    }
}

void
documents_remove_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  const gchar         *uri;
  GList               *images = NULL;
  GList               *list;

  if (clay_container_view_get_selected (editor->view, &images) > 0)
    {
      for (list = images; list; list = list->next)
        {
          uri = clay_object_get_name (CLAY_IMAGEFILE (list->data));

          gtk_recent_manager_remove_item (gtk_recent_manager_get_default (),
                                          uri, NULL);
          clay_container_view_set_1_selected (editor->view,
                                              CLAY_VIEWABLE (list->data));
          clay_container_view_remove_active (editor->view);
        }
      g_list_free (list);
      g_list_free (images);
    }
}

void
documents_clear_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayContainerEditor *editor  = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context = clay_container_view_get_context (editor->view);
  Clay                *clay    = context->clay;
  GtkWidget           *dialog;

  dialog = clay_message_dialog_new (_("Clear Document History"),
                                    CLAY_ICON_SHRED,
                                    GTK_WIDGET (editor),
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    clay_standard_help_func, NULL,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                           "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("Clear the Recent Documents list?"));

  clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the document history will "
                               "permanently remove all images from "
                               "the recent documents list."));

  if (clay_dialog_run (CLAY_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      GtkRecentManager *manager = gtk_recent_manager_get_default ();
      GList            *items;
      GList            *list;

      items = gtk_recent_manager_get_items (manager);

      for (list = items; list; list = list->next)
        {
          GtkRecentInfo *info = list->data;

          if (gtk_recent_info_has_application (info,
                                               "Clay Image Beautification Program"))
            {
              gtk_recent_manager_remove_item (manager,
                                              gtk_recent_info_get_uri (info),
                                              NULL);
            }

          gtk_recent_info_unref (info);
        }

      g_list_free (items);

      clay_container_clear (clay->documents);
    }

  gtk_widget_destroy (dialog);
}

void
documents_recreate_preview_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContext         *context;
  ClayContainer       *container;
  ClayImagefile       *imagefile;

  context   = clay_container_view_get_context (editor->view);
  container = clay_container_view_get_container (editor->view);

  imagefile = clay_context_get_imagefile (context);

  if (imagefile && clay_container_have (container, CLAY_OBJECT (imagefile)))
    {
      GError *error = NULL;

      if (! clay_imagefile_create_thumbnail (imagefile,
                                             context, NULL,
                                             context->clay->config->thumbnail_size,
                                             FALSE, &error))
        {
          clay_message_literal (context->clay,
                                NULL , CLAY_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}

void
documents_reload_previews_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;

  container = clay_container_view_get_container (editor->view);

  clay_container_foreach (container,
                          (GFunc) clay_imagefile_update,
                          editor->view);
}

static void
documents_remove_dangling_foreach (ClayImagefile *imagefile,
                                   ClayContainer *container)
{
  ClayThumbnail *thumbnail = clay_imagefile_get_thumbnail (imagefile);

  if (clay_thumbnail_peek_image (thumbnail) == CLAY_THUMB_STATE_NOT_FOUND)
    {
      const gchar *uri = clay_object_get_name (imagefile);

      gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri,
                                      NULL);

      clay_container_remove (container, CLAY_OBJECT (imagefile));
    }
}

void
documents_remove_dangling_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayContainerEditor *editor = CLAY_CONTAINER_EDITOR (data);
  ClayContainer       *container;

  container = clay_container_view_get_container (editor->view);

  clay_container_foreach (container,
                          (GFunc) documents_remove_dangling_foreach,
                          container);
}


/*  private functions  */

static void
documents_open_image (GtkWidget     *editor,
                      ClayContext   *context,
                      ClayImagefile *imagefile)
{
  GFile              *file;
  ClayImage          *image;
  ClayPDBStatusType   status;
  GError             *error = NULL;

  file = clay_imagefile_get_file (imagefile);

  image = file_open_with_display (context->clay, context, NULL, file, FALSE,
                                  G_OBJECT (clay_widget_get_monitor (editor)),
                                  &status, &error);

  if (! image && status != CLAY_PDB_CANCEL)
    {
      clay_message (context->clay, G_OBJECT (editor), CLAY_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    clay_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }
}

static void
documents_raise_display (ClayDisplay  *display,
                         RaiseClosure *closure)
{
  ClayImage *image;
  GFile     *file;
  gchar     *uri = NULL;

  image = clay_display_get_image (display);

  if (! image)
    return;

  file = clay_image_get_file (image);
  if (! file)
    file = clay_image_get_imported_file (image);

  if (file)
    uri = g_file_get_uri (file);

  if (! g_strcmp0 (closure->name, uri))
    {
      closure->found = TRUE;
      clay_display_shell_present (clay_display_get_shell (display));
    }

  g_free (uri);
}
