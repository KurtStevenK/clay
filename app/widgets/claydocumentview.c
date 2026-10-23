/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydocumentview.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimagefile.h"

#include "claycontainerview.h"
#include "claydocumentview.h"
#include "claydnd.h"
#include "clayeditor.h"
#include "claymenufactory.h"
#include "clayuimanager.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void    clay_document_view_activate_item (ClayContainerEditor *editor,
                                                 ClayViewable        *viewable);
static GList * clay_document_view_drag_uri_list (GtkWidget           *widget,
                                                 gpointer             data);


G_DEFINE_TYPE (ClayDocumentView, clay_document_view,
               CLAY_TYPE_CONTAINER_EDITOR)

#define parent_class clay_document_view_parent_class


static void
clay_document_view_class_init (ClayDocumentViewClass *klass)
{
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = clay_document_view_activate_item;
}

static void
clay_document_view_init (ClayDocumentView *view)
{
  view->open_button    = NULL;
  view->remove_button  = NULL;
  view->refresh_button = NULL;
}

GtkWidget *
clay_document_view_new (ClayViewType     view_type,
                        ClayContainer   *container,
                        ClayContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        ClayMenuFactory *menu_factory)
{
  ClayDocumentView    *document_view;
  ClayContainerEditor *editor;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  document_view = g_object_new (CLAY_TYPE_DOCUMENT_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Documents>",
                                "ui-path",           "/documents-popup",
                                NULL);

  editor = CLAY_CONTAINER_EDITOR (document_view);

  clay_container_editor_set_selection_mode (editor, GTK_SELECTION_MULTIPLE);

  document_view->open_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "documents",
                                   "documents-open",
                                   "documents-raise-or-open",
                                   GDK_SHIFT_MASK,
                                   "documents-file-open-dialog",
                                   clay_get_toggle_behavior_mask (),
                                   NULL);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->open_button),
                                  CLAY_TYPE_IMAGEFILE);

  document_view->remove_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "documents",
                                   "documents-remove", NULL);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (document_view->remove_button),
                                  CLAY_TYPE_IMAGEFILE);

  clay_editor_add_action_button (CLAY_EDITOR (editor->view), "documents",
                                 "documents-clear", NULL);

  document_view->refresh_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "documents",
                                   "documents-recreate-preview",
                                   "documents-reload-previews",
                                   GDK_SHIFT_MASK,
                                   "documents-remove-dangling",
                                   clay_get_toggle_behavior_mask (),
                                   NULL);

  if (view_type == CLAY_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = clay_container_view_get_dnd_widget (editor->view);

      clay_dnd_uri_list_source_add (dnd_widget,
                                    clay_document_view_drag_uri_list,
                                    editor);
    }

  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (document_view);
}

static void
clay_document_view_activate_item (ClayContainerEditor *editor,
                                  ClayViewable        *viewable)
{
  ClayDocumentView *view = CLAY_DOCUMENT_VIEW (editor);
  ClayContainer    *container;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->open_button));
    }
}

static GList *
clay_document_view_drag_uri_list (GtkWidget *widget,
                                  gpointer   data)
{
  ClayViewable *viewable = clay_dnd_get_drag_viewable (widget);

  if (viewable)
    {
      const gchar *uri = clay_object_get_name (viewable);

      return g_list_append (NULL, g_strdup (uri));
    }

  return NULL;
}
