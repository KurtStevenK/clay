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

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claylist.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayundostack.h"

#include "claycontainertreeview.h"
#include "claycontainerview.h"
#include "claydocked.h"
#include "clayhelp-ids.h"
#include "claymenufactory.h"
#include "clayundoeditor.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_VIEW_SIZE
};


static void   clay_undo_editor_docked_iface_init (ClayDockedInterface *iface);

static void   clay_undo_editor_constructed    (GObject           *object);
static void   clay_undo_editor_set_property   (GObject           *object,
                                               guint              property_id,
                                               const GValue      *value,
                                               GParamSpec        *pspec);

static void   clay_undo_editor_set_image      (ClayImageEditor   *editor,
                                               ClayImage         *image);

static void   clay_undo_editor_set_context    (ClayDocked        *docked,
                                               ClayContext       *context);

static void   clay_undo_editor_fill           (ClayUndoEditor    *editor);
static void   clay_undo_editor_clear          (ClayUndoEditor    *editor);

static void   clay_undo_editor_undo_event     (ClayImage         *image,
                                               ClayUndoEvent      event,
                                               ClayUndo          *undo,
                                               ClayUndoEditor    *editor);

static void   clay_undo_editor_selection_changed
                                              (ClayContainerView *view,
                                               ClayUndoEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (ClayUndoEditor, clay_undo_editor,
                         CLAY_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_undo_editor_docked_iface_init))

#define parent_class clay_undo_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_undo_editor_class_init (ClayUndoEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  ClayImageEditorClass *image_editor_class = CLAY_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = clay_undo_editor_constructed;
  object_class->set_property    = clay_undo_editor_set_property;

  image_editor_class->set_image = clay_undo_editor_set_image;

  g_object_class_install_property (object_class, PROP_VIEW_SIZE,
                                   g_param_spec_enum ("view-size",
                                                      NULL, NULL,
                                                      CLAY_TYPE_VIEW_SIZE,
                                                      CLAY_VIEW_SIZE_LARGE,
                                                      CLAY_PARAM_WRITABLE |
                                                      G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_undo_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_undo_editor_set_context;
}

static void
clay_undo_editor_init (ClayUndoEditor *undo_editor)
{
}

static void
clay_undo_editor_constructed (GObject *object)
{
  ClayUndoEditor *undo_editor = CLAY_UNDO_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  undo_editor->view = clay_container_tree_view_new (NULL, NULL,
                                                    undo_editor->view_size,
                                                    1);

  gtk_box_pack_start (GTK_BOX (undo_editor), undo_editor->view, TRUE, TRUE, 0);
  gtk_widget_set_visible (undo_editor->view, TRUE);

  g_signal_connect (undo_editor->view, "selection-changed",
                    G_CALLBACK (clay_undo_editor_selection_changed),
                    undo_editor);

  undo_editor->undo_button =
    clay_editor_add_action_button (CLAY_EDITOR (undo_editor), "edit",
                                   "edit-undo", NULL);

  undo_editor->redo_button =
    clay_editor_add_action_button (CLAY_EDITOR (undo_editor), "edit",
                                   "edit-redo", NULL);

  undo_editor->clear_button =
    clay_editor_add_action_button (CLAY_EDITOR (undo_editor), "edit",
                                   "edit-undo-clear", NULL);
}

static void
clay_undo_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayUndoEditor *undo_editor = CLAY_UNDO_EDITOR (object);

  switch (property_id)
    {
    case PROP_VIEW_SIZE:
      undo_editor->view_size = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_undo_editor_set_image (ClayImageEditor *image_editor,
                            ClayImage       *image)
{
  ClayUndoEditor *editor = CLAY_UNDO_EDITOR (image_editor);

  if (image_editor->image)
    {
      clay_undo_editor_clear (editor);

      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            clay_undo_editor_undo_event,
                                            editor);
    }

  CLAY_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image_editor->image)
    {
      if (clay_image_undo_is_enabled (image_editor->image))
        clay_undo_editor_fill (editor);

      g_signal_connect (image_editor->image, "undo-event",
                        G_CALLBACK (clay_undo_editor_undo_event),
                        editor);
    }
}

static void
clay_undo_editor_set_context (ClayDocked  *docked,
                              ClayContext *context)
{
  ClayUndoEditor *editor = CLAY_UNDO_EDITOR (docked);

  g_set_object (&editor->context, context);

  /* This calls clay_undo_editor_set_image(), so make sure that it
   * isn't called before editor->context has been initialized.
   */
  parent_docked_iface->set_context (docked, context);

  clay_container_view_set_context (CLAY_CONTAINER_VIEW (editor->view),
                                   context);
}


/*  public functions  */

GtkWidget *
clay_undo_editor_new (ClayCoreConfig  *config,
                      ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_CORE_CONFIG (config), NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_UNDO_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Undo>",
                       "ui-path",         "/undo-popup",
                       "view-size",       config->undo_preview_size,
                       NULL);
}


/*  private functions  */

static void
clay_undo_editor_fill (ClayUndoEditor *editor)
{
  ClayImage     *image      = CLAY_IMAGE_EDITOR (editor)->image;
  ClayUndoStack *undo_stack = clay_image_get_undo_stack (image);
  ClayUndoStack *redo_stack = clay_image_get_redo_stack (image);
  ClayUndo      *top_undo_item;
  GList         *list;

  /*  create a container as model for the undo history list  */
  editor->container = clay_list_new (CLAY_TYPE_UNDO, FALSE);
  editor->base_item = g_object_new (CLAY_TYPE_UNDO,
                                    "image", image,
                                    "name",  _("[ Base Image ]"),
                                    NULL);

  /*  the list prepends its items, so first add the redo items in
   *  reverse (ascending) order...
   */
  for (list = CLAY_LIST (redo_stack->undos)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      clay_container_add (editor->container, CLAY_OBJECT (list->data));
    }

  /*  ...then add the undo items in descending order...  */
  for (list = CLAY_LIST (undo_stack->undos)->queue->head;
       list;
       list = g_list_next (list))
    {
      /*  Don't add the topmost item if it is an open undo group,
       *  it will be added upon closing of the group.
       */
      if (list->prev || ! CLAY_IS_UNDO_STACK (list->data) ||
          clay_image_get_undo_group_count (image) == 0)
        {
          clay_container_add (editor->container, CLAY_OBJECT (list->data));
        }
    }

  /*  ...finally, the first item is the special "base_item" which stands
   *  for the image with no more undos available to pop
   */
  clay_container_add (editor->container, CLAY_OBJECT (editor->base_item));

  /*  display the container  */
  clay_container_view_set_container (CLAY_CONTAINER_VIEW (editor->view),
                                     editor->container);

  top_undo_item = clay_undo_stack_peek (undo_stack);

  g_signal_handlers_block_by_func (editor->view,
                                   clay_undo_editor_selection_changed,
                                   editor);

  /*  select the current state of the image  */
  if (top_undo_item)
    {
      clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (editor->view),
                                          CLAY_VIEWABLE (top_undo_item));
      clay_undo_create_preview (top_undo_item, editor->context, FALSE);
    }
  else
    {
      clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (editor->view),
                                          CLAY_VIEWABLE (editor->base_item));
      clay_undo_create_preview (editor->base_item, editor->context, TRUE);
    }

  g_signal_handlers_unblock_by_func (editor->view,
                                     clay_undo_editor_selection_changed,
                                     editor);
}

static void
clay_undo_editor_clear (ClayUndoEditor *editor)
{
  if (editor->container)
    {
      clay_container_view_set_container (CLAY_CONTAINER_VIEW (editor->view),
                                         NULL);
      g_clear_object (&editor->container);
    }

  g_clear_object (&editor->base_item);
}

static void
clay_undo_editor_undo_event (ClayImage      *image,
                             ClayUndoEvent   event,
                             ClayUndo       *undo,
                             ClayUndoEditor *editor)
{
  ClayUndoStack *undo_stack    = clay_image_get_undo_stack (image);
  ClayUndo      *top_undo_item = clay_undo_stack_peek (undo_stack);

  switch (event)
    {
    case CLAY_UNDO_EVENT_UNDO_PUSHED:
      if (! editor->container)
        {
          if (clay_image_undo_is_enabled (image))
            clay_undo_editor_fill (editor);
          break;
        }

      g_signal_handlers_block_by_func (editor->view,
                                       clay_undo_editor_selection_changed,
                                       editor);

      clay_container_insert (editor->container, CLAY_OBJECT (undo), -1);
      clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (editor->view),
                                          CLAY_VIEWABLE (undo));
      clay_undo_create_preview (undo, editor->context, FALSE);

      g_signal_handlers_unblock_by_func (editor->view,
                                         clay_undo_editor_selection_changed,
                                         editor);
      break;

    case CLAY_UNDO_EVENT_UNDO_EXPIRED:
    case CLAY_UNDO_EVENT_REDO_EXPIRED:
      clay_container_remove (editor->container, CLAY_OBJECT (undo));
      break;

    case CLAY_UNDO_EVENT_UNDO:
    case CLAY_UNDO_EVENT_REDO:
      g_signal_handlers_block_by_func (editor->view,
                                       clay_undo_editor_selection_changed,
                                       editor);

      if (top_undo_item)
        {
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (editor->view),
                                              CLAY_VIEWABLE (top_undo_item));
          clay_undo_create_preview (top_undo_item, editor->context, FALSE);
        }
      else
        {
          clay_container_view_set_1_selected (CLAY_CONTAINER_VIEW (editor->view),
                                              CLAY_VIEWABLE (editor->base_item));
          clay_undo_create_preview (editor->base_item, editor->context, TRUE);
        }

      g_signal_handlers_unblock_by_func (editor->view,
                                         clay_undo_editor_selection_changed,
                                         editor);
      break;

    case CLAY_UNDO_EVENT_UNDO_FREE:
      if (clay_image_undo_is_enabled (image))
        clay_undo_editor_clear (editor);
      break;

    case CLAY_UNDO_EVENT_UNDO_FREEZE:
      clay_undo_editor_clear (editor);
      break;

    case CLAY_UNDO_EVENT_UNDO_THAW:
      clay_undo_editor_fill (editor);
      break;
    }
}

static void
clay_undo_editor_selection_changed (ClayContainerView *view,
                                    ClayUndoEditor    *editor)
{
  ClayImage     *image      = CLAY_IMAGE_EDITOR (editor)->image;
  ClayUndoStack *undo_stack = clay_image_get_undo_stack (image);
  ClayUndoStack *redo_stack = clay_image_get_redo_stack (image);
  ClayUndo      *top_undo_item;
  ClayUndo      *undo;

  undo = CLAY_UNDO (clay_container_view_get_1_selected (view));

  if (! undo)
    return;

  top_undo_item = clay_undo_stack_peek (undo_stack);

  if (undo == editor->base_item)
    {
      /*  the base_item was selected, pop all available undo items
       */
      while (top_undo_item != NULL)
        {
          if (! clay_image_undo (image))
            break;

          top_undo_item = clay_undo_stack_peek (undo_stack);
        }
    }
  else if (clay_container_have (undo_stack->undos, CLAY_OBJECT (undo)))
    {
      /*  the selected item is on the undo stack, pop undos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if(! clay_image_undo (image))
            break;

          top_undo_item = clay_undo_stack_peek (undo_stack);
        }
    }
  else if (clay_container_have (redo_stack->undos, CLAY_OBJECT (undo)))
    {
      /*  the selected item is on the redo stack, pop redos until it
       *  is on top of the undo stack
       */
      while (top_undo_item != undo)
        {
          if (! clay_image_redo (image))
            break;

          top_undo_item = clay_undo_stack_peek (undo_stack);
        }
    }

  clay_image_flush (image);
}
