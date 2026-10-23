/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytemplateview.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claytemplate.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claymenufactory.h"
#include "claytemplateview.h"
#include "claydnd.h"
#include "clayhelp-ids.h"
#include "clayviewrenderer.h"
#include "clayuimanager.h"

#include "clay-intl.h"


static void   clay_template_view_activate_item (ClayContainerEditor *editor,
                                                ClayViewable        *viewable);


G_DEFINE_TYPE (ClayTemplateView, clay_template_view,
               CLAY_TYPE_CONTAINER_EDITOR);

#define parent_class clay_template_view_parent_class


static void
clay_template_view_class_init (ClayTemplateViewClass *klass)
{
  ClayContainerEditorClass *editor_class = CLAY_CONTAINER_EDITOR_CLASS (klass);

  editor_class->activate_item = clay_template_view_activate_item;
}

static void
clay_template_view_init (ClayTemplateView *view)
{
  view->create_button    = NULL;
  view->new_button       = NULL;
  view->duplicate_button = NULL;
  view->edit_button      = NULL;
  view->delete_button    = NULL;
}

GtkWidget *
clay_template_view_new (ClayViewType     view_type,
                        ClayContainer   *container,
                        ClayContext     *context,
                        gint             view_size,
                        gint             view_border_width,
                        ClayMenuFactory *menu_factory)
{
  ClayTemplateView    *template_view;
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

  template_view = g_object_new (CLAY_TYPE_TEMPLATE_VIEW,
                                "view-type",         view_type,
                                "container",         container,
                                "context",           context,
                                "view-size",         view_size,
                                "view-border-width", view_border_width,
                                "menu-factory",      menu_factory,
                                "menu-identifier",   "<Templates>",
                                "ui-path",           "/templates-popup",
                                NULL);

  editor = CLAY_CONTAINER_EDITOR (template_view);

  if (CLAY_IS_CONTAINER_TREE_VIEW (editor->view))
    {
      ClayContainerTreeView *tree_view;

      tree_view = CLAY_CONTAINER_TREE_VIEW (editor->view);

      clay_container_tree_view_connect_name_edited (tree_view,
                                                    G_CALLBACK (clay_container_tree_view_name_edited),
                                                    tree_view);
    }

  template_view->create_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "templates",
                                   "templates-create-image", NULL);

  template_view->new_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "templates",
                                   "templates-new", NULL);

  template_view->duplicate_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "templates",
                                   "templates-duplicate", NULL);

  template_view->edit_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "templates",
                                   "templates-edit", NULL);

  template_view->delete_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor->view), "templates",
                                   "templates-delete", NULL);

  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->create_button),
                                  CLAY_TYPE_TEMPLATE);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->duplicate_button),
                                  CLAY_TYPE_TEMPLATE);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->edit_button),
                                  CLAY_TYPE_TEMPLATE);
  clay_container_view_enable_dnd (editor->view,
                                  GTK_BUTTON (template_view->delete_button),
                                  CLAY_TYPE_TEMPLATE);

  clay_ui_manager_update (clay_editor_get_ui_manager (CLAY_EDITOR (editor->view)),
                          editor);

  return GTK_WIDGET (template_view);
}

static void
clay_template_view_activate_item (ClayContainerEditor *editor,
                                  ClayViewable        *viewable)
{
  ClayTemplateView *view = CLAY_TEMPLATE_VIEW (editor);
  ClayContainer    *container;

  if (CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item)
    CLAY_CONTAINER_EDITOR_CLASS (parent_class)->activate_item (editor, viewable);

  container = clay_container_view_get_container (editor->view);

  if (viewable && clay_container_have (container, CLAY_OBJECT (viewable)))
    {
      gtk_button_clicked (GTK_BUTTON (view->create_button));
    }
}
