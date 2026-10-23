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

#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "claycontainerview.h"
#include "clayeditor.h"
#include "clayimageview.h"
#include "claydnd.h"
#include "claymenufactory.h"
#include "clayuimanager.h"
#include "clayviewrenderer.h"

#include "clay-intl.h"


static void   clay_image_view_finalize              (GObject             *object);

static void   clay_image_view_image_changed         (ClayContainerView   *container_view,
                                                     ClayImageView       *view);
static void   clay_image_view_display_count_changed (ClayImage           *image,
                                                     gint                 display_count,
                                                     ClayImageView       *view);

static void   clay_image_view_select_item           (ClayContainerEditor *editor,
                                                     ClayViewable        *viewable);
static void   clay_image_view_activate_item         (ClayContainerEditor *editor,
                                                     ClayViewable        *viewable);


G_DEFINE_TYPE (ClayImageView, clay_image_view, CLAY_TYPE_CONTAINER_EDITOR)

#define parent_class clay_image_view_parent_class


static void
clay_image_view_class_init (ClayImageViewClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  object_class->finalize      = clay_image_view_finalize;

  editor_class->select_item   = clay_image_view_select_item;
  editor_class->activate_item = clay_image_view_activate_item;
}

static void
clay_image_view_init (ClayImageView *view)
{
  view->raise_button  = NULL;
  view->new_button    = NULL;
  view->delete_button = NULL;
}

static void
clay_image_view_finalize (GObject *object)
{
  ClayImageView *view = CLAY_IMAGE_VIEW (object);

  g_clear_weak_pointer (&view->image);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
clay_image_view_new (ClayViewType     view_type,
                     ClayContainer   *container,
                     ClayContext     *context,
                     gint             view_size,
                     gint             view_border_width,
                     ClayMenuFactory *menu_factory)
{
  ClayImageView       *image_view;
  ClayContainerEditor *editor;

  g_return_val_if_fail (CLAY_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (view_border_width >= 0 &&
                        view_border_width <= CLAY_VIEW_MAX_BORDER_WIDTH,
                        NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  image_view = g_object_new (CLAY_TYPE_IMAGE_VIEW,
                             "view-type",         view_type,
                             "container",         container,
                             "context",           context,
                             "view-size",         view_size,
                             "view-border-width", view_border_width,
                             "menu-factory",      menu_factory,
                             "menu-identifier",   "<Images>",
                             "ui-path",           "/images-popup",
                             NULL);

  editor = CLAY_CONTAINER_EDITOR (image_view);

  image_view->raise_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "images",
                                   "images-raise-views", NULL);

  image_view->new_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "images",
                                   "images-new-view", NULL);

  image_view->delete_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "images",
                                   "images-delete", NULL);

  if (view_type == CLAY_VIEW_TYPE_LIST)
    {
      GtkWidget *dnd_widget;

      dnd_widget = clay_container_view_get_dnd_widget (editor->view);

      clay_dnd_xds_source_add (dnd_widget,
                               (ClayDndDragViewableFunc) clay_dnd_get_drag_viewable,
                               NULL);
    }

  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->raise_button),
                                  CLAY_TYPE_IMAGE);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->new_button),
                                  CLAY_TYPE_IMAGE);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (image_view->delete_button),
                                  CLAY_TYPE_IMAGE);

  /* The selection for this ClayContainerView is known to be an image.
   * The ClayContainerView class will automatically connect to the
   * "image-changed" signal from the context, but it doesn't tell us
   * anything about whether the display count of the selected image
   * changes. Yet this is important information as some action in the
   * "Images" action group may be set sensitive depending on this count.
   */
  g_signal_connect (editor->view,
                    "selection-changed",
                    G_CALLBACK (clay_image_view_image_changed),
                    image_view);
  clay_image_view_image_changed (editor->view, image_view);

  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (image_view);
}

static void
clay_image_view_image_changed (ClayContainerView *container_view,
                               ClayImageView     *view)
{
  if (view->image)
    g_signal_handlers_disconnect_by_func (view->image,
                                          G_CALLBACK (clay_image_view_display_count_changed),
                                          view);
  g_set_weak_pointer (&view->image,
                      CLAY_IMAGE (clay_container_view_get_1_selected (container_view)));
  if (view->image)
    g_signal_connect_object (view->image, "display-count-changed",
                             G_CALLBACK (clay_image_view_display_count_changed),
                             view, 0);
}

static void
clay_image_view_display_count_changed (ClayImage     *image,
                                       gint           display_count,
                                       ClayImageView *view)
{
  ClayContainerEditor *editor;

  editor = CLAY_CONTAINER_EDITOR (view);
  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor->view)),
                          editor);
}

static void
clay_image_view_select_item (ClayContainerEditor *editor,
                             ClayViewable        *viewable)
{
  ClayImageView *view = CLAY_IMAGE_VIEW (editor);
  ClayContainer *container;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->select_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      if (view->raise_button)
        gtk_button_clicked (GTK_BUTTON (view->raise_button));
    }
}

static void
clay_image_view_activate_item (ClayContainerEditor *editor,
                               ClayViewable        *viewable)
{
  ClayImageView *view = CLAY_IMAGE_VIEW (editor);
  ClayContainer *container;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->raise_button));
    }
}
