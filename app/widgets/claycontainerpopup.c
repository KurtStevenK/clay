/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycontainerpopup.c
 * Copyright (C) 2003-2014 Michael Natterer <mitch@gimp.org>
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
#include <gdk/gdkkeysyms.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/claycontainer.h"
#include "core/clayviewable.h"
#include "core/clay-utils.h"

#include "claycontainerbox.h"
#include "claycontainereditor.h"
#include "claycontainerpopup.h"
#include "claycontainerlistview.h"
#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claydialogfactory.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"
#include "claywindowstrategy.h"

#include "clay-intl.h"


static void   clay_container_popup_finalize         (GObject            *object);

static void   clay_container_popup_confirm          (ClayPopup          *popup);

static void   clay_container_popup_create_view      (ClayContainerPopup *popup);

static void   clay_container_popup_smaller_clicked  (GtkWidget          *button,
                                                     ClayContainerPopup *popup);
static void   clay_container_popup_larger_clicked   (GtkWidget          *button,
                                                     ClayContainerPopup *popup);
static void   clay_container_popup_view_type_toggled(GtkWidget          *button,
                                                     ClayContainerPopup *popup);
static void   clay_container_popup_dialog_clicked   (GtkWidget          *button,
                                                     ClayContainerPopup *popup);

static void   clay_container_popup_context_changed  (ClayContext        *context,
                                                     ClayViewable       *viewable,
                                                     ClayContainerPopup *popup);


G_DEFINE_TYPE (ClayContainerPopup, clay_container_popup, CLAY_TYPE_POPUP)

#define parent_class clay_container_popup_parent_class


static void
clay_container_popup_class_init (ClayContainerPopupClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  ClayPopupClass *popup_class  = CLAY_POPUP_CLASS (klass);

  object_class->finalize = clay_container_popup_finalize;

  popup_class->confirm   = clay_container_popup_confirm;
}

static void
clay_container_popup_init (ClayContainerPopup *popup)
{
  popup->view_type         = CLAY_VIEW_TYPE_LIST;
  popup->default_view_size = CLAY_VIEW_SIZE_SMALL;
  popup->view_size         = CLAY_VIEW_SIZE_SMALL;
  popup->view_border_width = 1;

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_set_visible (popup->frame, TRUE);
}

static void
clay_container_popup_finalize (GObject *object)
{
  ClayContainerPopup *popup = CLAY_CONTAINER_POPUP (object);

  g_clear_object (&popup->context);

  g_clear_pointer (&popup->dialog_identifier, g_free);
  g_clear_pointer (&popup->dialog_icon_name,  g_free);
  g_clear_pointer (&popup->dialog_tooltip,    g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_container_popup_confirm (ClayPopup *popup)
{
  ClayContainerPopup *c_popup = CLAY_CONTAINER_POPUP (popup);
  ClayObject         *object;

  object = clay_context_get_by_type (c_popup->context,
                                     clay_container_get_child_type (c_popup->container));
  clay_context_set_by_type (c_popup->orig_context,
                            clay_container_get_child_type (c_popup->container),
                            object);

  CLAY_POPUP_CLASS (parent_class)->confirm (popup);
}

GtkWidget *
clay_container_popup_new (ClayContainer     *container,
                          ClayContext       *context,
                          ClayViewType       view_type,
                          gint               default_view_size,
                          gint               view_size,
                          gint               view_border_width,
                          ClayDialogFactory *dialog_factory,
                          const gchar       *dialog_identifier,
                          const gchar       *dialog_icon_name,
                          const gchar       *dialog_tooltip)
{
  ClayContainerPopup *popup;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (default_view_size >  0 &&
                        default_view_size <= CLAY_VIEWABLE_MAX_POPUP_SIZE,
                        NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= CLAY_VIEWABLE_MAX_POPUP_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (dialog_factory == NULL ||
                        CLAY_IS_DIALOG_FACTORY (dialog_factory), NULL);
  if (dialog_factory)
    {
      g_return_val_if_fail (dialog_identifier != NULL, NULL);
      g_return_val_if_fail (dialog_icon_name != NULL, NULL);
      g_return_val_if_fail (dialog_tooltip != NULL, NULL);
    }

  popup = g_object_new (CLAY_TYPE_CONTAINER_POPUP,
                        "type", GTK_WINDOW_POPUP,
                        NULL);
  gtk_window_set_resizable (GTK_WINDOW (popup), FALSE);

  popup->container    = container;
  popup->orig_context = context;
  popup->context      = clay_context_new (context->clay, "popup", context);

  popup->view_type         = view_type;
  popup->default_view_size = default_view_size;
  popup->view_size         = view_size;
  popup->view_border_width = view_border_width;

  g_signal_connect (popup->context,
                    clay_context_type_to_signal_name (clay_container_get_child_type (container)),
                    G_CALLBACK (clay_container_popup_context_changed),
                    popup);

  if (dialog_factory)
    {
      popup->dialog_factory    = dialog_factory;
      popup->dialog_identifier = g_strdup (dialog_identifier);
      popup->dialog_icon_name  = g_strdup (dialog_icon_name);
      popup->dialog_tooltip    = g_strdup (dialog_tooltip);
    }

  clay_container_popup_create_view (popup);

  return GTK_WIDGET (popup);
}

ClayViewType
clay_container_popup_get_view_type (ClayContainerPopup *popup)
{
  g_return_val_if_fail (CLAY_IS_CONTAINER_POPUP (popup), CLAY_VIEW_TYPE_LIST);

  return popup->view_type;
}

void
clay_container_popup_set_view_type (ClayContainerPopup *popup,
                                    ClayViewType        view_type)
{
  g_return_if_fail (CLAY_IS_CONTAINER_POPUP (popup));

  if (view_type != popup->view_type)
    {
      popup->view_type = view_type;

      gtk_widget_destroy (GTK_WIDGET (popup->editor));
      clay_container_popup_create_view (popup);
    }
}

ClayViewSize
clay_container_popup_get_view_size (ClayContainerPopup *popup)
{
  g_return_val_if_fail (CLAY_IS_CONTAINER_POPUP (popup), CLAY_VIEW_SIZE_SMALL);

  return popup->view_size;
}

void
clay_container_popup_set_view_size (ClayContainerPopup *popup,
                                    ClayViewSize        view_size)
{
  GtkWidget     *scrolled_win;
  GtkWidget     *viewport;
  GtkAllocation  allocation;

  g_return_if_fail (CLAY_IS_CONTAINER_POPUP (popup));

  scrolled_win = CLAY_CONTAINER_BOX (popup->editor->view)->scrolled_win;
  viewport     = gtk_bin_get_child (GTK_BIN (scrolled_win));

  gtk_widget_get_allocation (viewport, &allocation);

  view_size = CLAMP (view_size, CLAY_VIEW_SIZE_TINY,
                     MIN (CLAY_VIEW_SIZE_GIGANTIC,
                          allocation.width - 2 * popup->view_border_width));

  if (view_size != popup->view_size)
    {
      popup->view_size = view_size;

      clay_container_view_set_view_size (popup->editor->view,
                                         popup->view_size,
                                         popup->view_border_width);
    }
}


/*  private functions  */

static void
clay_container_popup_create_view (ClayContainerPopup *popup)
{
  ClayEditor  *editor;
  GtkWidget   *button;
  const gchar *signal_name;
  GType        child_type;
  gint         rows;
  gint         columns;

  popup->editor = g_object_new (CLAY_TYPE_CONTAINER_EDITOR,
                                "view-type",         popup->view_type,
                                "container",         popup->container,
                                "context",           popup->context,
                                "view-size",         popup->view_size,
                                "view-border-width", popup->view_border_width,
                                NULL);

  clay_container_view_set_reorderable (CLAY_CONTAINER_VIEW (popup->editor->view),
                                       FALSE);

  if (popup->view_type == CLAY_VIEW_TYPE_LIST)
    {
      ClayContainerView *view = popup->editor->view;
      GtkWidget         *entry;

      entry = gtk_search_entry_new ();
      gtk_box_pack_end (GTK_BOX (view), entry, FALSE, FALSE, 0);

      if (CLAY_IS_CONTAINER_TREE_VIEW (view))
        {
          ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);

          gtk_tree_view_set_search_entry (GTK_TREE_VIEW (tree_view->view),
                                          GTK_ENTRY (entry));
        }
      else
        {
          ClayContainerListView *list_view = CLAY_CONTAINER_LIST_VIEW (view);

          clay_container_list_view_set_search_entry (list_view,
                                                     GTK_SEARCH_ENTRY (entry));
        }

      gtk_widget_set_visible (entry, TRUE);
    }

  /* lame workaround for bug #761998 */
  if (popup->default_view_size >= CLAY_VIEW_SIZE_LARGE)
    {
      rows    = 6;
      columns = 6;
    }
  else
    {
      rows    = 10;
      columns = 6;
    }

  clay_container_box_set_size_request (CLAY_CONTAINER_BOX (popup->editor->view),
                                       columns * (popup->default_view_size +
                                                  2 * popup->view_border_width),
                                       rows    * (popup->default_view_size +
                                                  2 * popup->view_border_width));

  if (CLAY_IS_EDITOR (popup->editor->view))
    clay_editor_set_show_name (CLAY_EDITOR (popup->editor->view), FALSE);

  gtk_container_add (GTK_CONTAINER (popup->frame), GTK_WIDGET (popup->editor));
  gtk_widget_set_visible (GTK_WIDGET (popup->editor), TRUE);

  editor = CLAY_EDITOR (popup->editor->view);

  clay_editor_add_button (editor, "zoom-out",
                          _("Smaller Previews"), NULL,
                          G_CALLBACK (clay_container_popup_smaller_clicked),
                          NULL,
                          G_OBJECT (popup));
  clay_editor_add_button (editor, "zoom-in",
                          _("Larger Previews"), NULL,
                          G_CALLBACK (clay_container_popup_larger_clicked),
                          NULL,
                          G_OBJECT (popup));

  button = clay_editor_add_icon_box (editor, CLAY_TYPE_VIEW_TYPE, "clay",
                                     G_CALLBACK (clay_container_popup_view_type_toggled),
                                     popup);
  clay_int_radio_group_set_active (GTK_RADIO_BUTTON (button), popup->view_type);

  if (popup->dialog_factory)
    clay_editor_add_button (editor, popup->dialog_icon_name,
                            popup->dialog_tooltip, NULL,
                            G_CALLBACK (clay_container_popup_dialog_clicked),
                            NULL,
                            G_OBJECT (popup));

  gtk_widget_grab_focus (GTK_WIDGET (popup->editor));

  /* Special-casing the object types managed by the context to make sure
   * the right items are selected when opening the popup.
   */
  child_type  = clay_container_get_child_type (popup->container);
  signal_name = clay_context_type_to_signal_name (child_type);

  if (signal_name)
    {
      ClayObject *object;

      object = clay_context_get_by_type (popup->orig_context, child_type);

      clay_container_view_set_1_selected (popup->editor->view,
                                          CLAY_VIEWABLE (object));
    }
}

static void
clay_container_popup_smaller_clicked (GtkWidget          *button,
                                      ClayContainerPopup *popup)
{
  gint view_size;

  view_size = clay_container_view_get_view_size (popup->editor->view, NULL);

  clay_container_popup_set_view_size (popup, clay_view_size_get_smaller (view_size));
}

static void
clay_container_popup_larger_clicked (GtkWidget          *button,
                                     ClayContainerPopup *popup)
{
  gint view_size;

  view_size = clay_container_view_get_view_size (popup->editor->view, NULL);

  clay_container_popup_set_view_size (popup, clay_view_size_get_larger (view_size));
}

static void
clay_container_popup_view_type_toggled (GtkWidget          *button,
                                        ClayContainerPopup *popup)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
    {
      ClayViewType view_type;

      view_type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                                      "clay-item-data"));

      clay_container_popup_set_view_type (popup, view_type);
    }
}

static void
clay_container_popup_dialog_clicked (GtkWidget          *button,
                                     ClayContainerPopup *popup)
{
  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (popup->context->clay)),
                                             popup->context->clay,
                                             popup->dialog_factory,
                                             clay_widget_get_monitor (button),
                                             popup->dialog_identifier);
  g_signal_emit_by_name (popup, "cancel");
}

static void
clay_container_popup_context_changed (ClayContext        *context,
                                      ClayViewable       *viewable,
                                      ClayContainerPopup *popup)
{
  GdkEvent  *current_event;
  GtkWidget *current_widget = GTK_WIDGET (popup);
  gboolean   confirm        = FALSE;

  current_event = gtk_get_current_event ();

  if (current_event && gtk_widget_get_window (current_widget))
    {
      GdkWindow *event_window = gdk_window_get_effective_toplevel (((GdkEventAny *) current_event)->window);
      GdkWindow *popup_window = gdk_window_get_effective_toplevel (gtk_widget_get_window (current_widget));

      /* We need to differentiate a context change as a consequence of
       * an event on another widget.
       */
      if ((((GdkEventAny *) current_event)->type == GDK_BUTTON_PRESS ||
           ((GdkEventAny *) current_event)->type == GDK_BUTTON_RELEASE) &&
          event_window == popup_window)
        confirm = TRUE;

      gdk_event_free (current_event);
    }

  if (confirm)
    g_signal_emit_by_name (popup, "confirm");
}
