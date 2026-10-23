/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybufferview.c
 * Copyright (C) 2001-2004 Michael Natterer <mitch@gimp.org>
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
#include "core/claybuffer.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claybufferview.h"
#include "claydnd.h"
#include "claydocked.h"
#include "clayeditor.h"
#include "clayhelp-ids.h"
#include "clayview.h"
#include "clayviewrendererbuffer.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void   clay_buffer_view_docked_iface_init (ClayDockedInterface *iface);

static void   clay_buffer_view_set_context       (ClayDocked          *docked,
                                                  ClayContext         *context);

static void   clay_buffer_view_activate_item     (ClayContainerEditor *editor,
                                                  ClayViewable        *viewable);

static void   clay_buffer_view_clipboard_changed (Clay                *clay,
                                                  ClayBufferView      *buffer_view);
static void   clay_buffer_view_view_notify       (ClayContainerView   *view,
                                                  GParamSpec          *pspec,
                                                  ClayBufferView      *buffer_view);


G_DEFINE_TYPE_WITH_CODE (ClayBufferView, clay_buffer_view,
                         CLAY_TYPE_CONTAINER_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_buffer_view_docked_iface_init))

#define parent_class clay_buffer_view_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_buffer_view_class_init (ClayBufferViewClass *klass)
{
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = clay_buffer_view_activate_item;
}

static void
clay_buffer_view_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_buffer_view_set_context;
}

static void
clay_buffer_view_init (ClayBufferView *view)
{
}

static void
clay_buffer_view_set_context (ClayDocked  *docked,
                              ClayContext *context)
{
  ClayBufferView *view = CLAY_BUFFER_VIEW (docked);

  parent_docked_iface->set_context (docked, context);

  clay_view_renderer_set_context (CLAY_VIEW (view->clipboard_view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
clay_buffer_view_new (ClayViewType     view_type,
                      ClayContainer   *container,
                      ClayContext     *context,
                      gint             view_size,
                      gint             view_border_width,
                      ClayMenuFactory *menu_factory)
{
  ClayBufferView      *buffer_view;
  ClayContainerEditor *editor;
  GtkWidget           *frame;
  GtkWidget           *hbox;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, FALSE);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        FALSE);

  buffer_view = g_object_new (CLAY_TYPE_BUFFER_VIEW,
                              "view-type",         view_type,
                              "container",         container,
                              "context",           context,
                              "view-size",         view_size,
                              "view-border-width", view_border_width,
                              "menu-factory",      menu_factory,
                              "menu-identifier",   "<Buffers>",
                              "ui-path",           "/buffers-popup",
                              NULL);

  editor = CLAY_CONTAINER_EDITOR (buffer_view);

  if (CLAY_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      ClayContainerTreeView *tree_view;

      tree_view = CLAY_CONTAINER_TREE_VIEW (editor->view);

      clay_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (clay_container_tree_view_name_edited),
                                                    tree_view);
    }

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (editor), frame, 0);
  gtk_widget_set_visible (frame, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_set_visible (hbox, TRUE);

  /* FIXME: enable preview of a clipboard image, not just buffer */
  buffer_view->clipboard_view =
    clay_view_new_full_by_types (NULL,
                                 CLAY_TYPE_VIEW,
                                 CLAY_TYPE_BUFFER,
                                 view_size, view_size, view_border_width,
                                 FALSE, FALSE, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_view,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (buffer_view->clipboard_view, TRUE);

  g_signal_connect_object (editor->view, "notify::view-size",
                           G_CALLBACK (clay_buffer_view_view_notify),
                           buffer_view, 0);
  g_signal_connect_object (editor->view, "notify::view-border-width",
                           G_CALLBACK (clay_buffer_view_view_notify),
                           buffer_view, 0);

  buffer_view->clipboard_label = gtk_label_new (_("(None)"));
  gtk_box_pack_start (GTK_BOX (hbox), buffer_view->clipboard_label,
                      FALSE, FALSE, 0);
  gtk_widget_set_visible (buffer_view->clipboard_label, TRUE);

  g_signal_connect_object (context->clay, "clipboard-changed",
                           G_CALLBACK (clay_buffer_view_clipboard_changed),
                           G_OBJECT (buffer_view), 0);

  clay_buffer_view_clipboard_changed (context->clay, buffer_view);

  buffer_view->paste_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "buffers",
                                   "buffers-paste",
                                   "buffers-paste-in-place",
                                   clay_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_into_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "buffers",
                                   "buffers-paste-into",
                                   "buffers-paste-into-in-place",
                                   clay_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_layer_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-layer",
                                   "buffers-paste-as-new-layer-in-place",
                                   clay_get_extend_selection_mask (),
                                   NULL);

  buffer_view->paste_as_new_image_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "buffers",
                                   "buffers-paste-as-new-image", NULL);

  buffer_view->delete_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "buffers",
                                   "buffers-delete", NULL);

  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_button),
                                  CLAY_TYPE_BUFFER);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_into_button),
                                  CLAY_TYPE_BUFFER);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_layer_button),
                                  CLAY_TYPE_BUFFER);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->paste_as_new_image_button),
                                  CLAY_TYPE_BUFFER);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (buffer_view->delete_button),
                                  CLAY_TYPE_BUFFER);

  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (buffer_view);
}


/*  private functions  */

static void
clay_buffer_view_activate_item (ClayContainerEditor *editor,
                                ClayViewable        *viewable)
{
  ClayBufferView *view = CLAY_BUFFER_VIEW (editor);
  ClayContainer  *container;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->paste_button));
    }
}

static void
clay_buffer_view_clipboard_changed (Clay           *clay,
                                    ClayBufferView *buffer_view)
{
  ClayBuffer *buffer = clay_get_clipboard_buffer (clay);

  clay_view_set_viewable (CLAY_VIEW (buffer_view->clipboard_view),
                          CLAY_VIEWABLE (buffer));

  if (buffer)
    {
      gchar *desc = clay_viewable_get_description (CLAY_VIEWABLE (buffer),
                                                   NULL);
      gtk_label_set_text (GTK_LABEL (buffer_view->clipboard_label), desc);
      g_free (desc);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (buffer_view->clipboard_label), _("(None)"));
    }
}

static void
clay_buffer_view_view_notify (ClayContainerView *container_view,
                              GParamSpec        *pspec,
                              ClayBufferView    *buffer_view)
{
  ClayView *view = CLAY_VIEW (buffer_view->clipboard_view);
  gint      view_size;
  gint      view_border_width;

  view_size = clay_container_view_get_view_size (container_view,
                                                 &view_border_width);

  clay_view_renderer_set_size_full (view->renderer,
                                    view_size, view_size, view_border_width);
}
