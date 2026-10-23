/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claychanneltreeview.c
 * Copyright (C) 2001-2009 Michael Natterer <mitch@gimp.org>
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"

#include "clayactiongroup.h"
#include "claychanneltreeview.h"
#include "claycomponenteditor.h"
#include "claycontainerview.h"
#include "claydnd.h"
#include "claydocked.h"
#include "clayhelp-ids.h"
#include "clayuimanager.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


struct _ClayChannelTreeViewPrivate
{
  GtkWidget *component_editor;

  GtkWidget *toselection_button;
};


static void  clay_channel_tree_view_view_iface_init   (ClayContainerViewInterface *iface);

static void   clay_channel_tree_view_constructed      (GObject                 *object);

static void   clay_channel_tree_view_drop_viewables   (ClayContainerTreeView   *view,
                                                       GList                   *src_viewables,
                                                       ClayViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   clay_channel_tree_view_drop_component   (ClayContainerTreeView   *tree_view,
                                                       ClayImage               *image,
                                                       ClayChannelType          component,
                                                       ClayViewable            *dest_viewable,
                                                       GtkTreeViewDropPosition  drop_pos);
static void   clay_channel_tree_view_set_image        (ClayItemTreeView        *item_view,
                                                       ClayImage               *image);
static ClayItem * clay_channel_tree_view_item_new     (ClayImage               *image);

static void   clay_channel_tree_view_set_context      (ClayContainerView       *view,
                                                       ClayContext             *context);
static void   clay_channel_tree_view_set_view_size    (ClayContainerView       *view);


G_DEFINE_TYPE_WITH_CODE (ClayChannelTreeView, clay_channel_tree_view,
                         CLAY_TYPE_DRAWABLE_TREE_VIEW,
                         G_ADD_PRIVATE (ClayChannelTreeView)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_channel_tree_view_view_iface_init))

#define parent_class clay_channel_tree_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_channel_tree_view_class_init (ClayChannelTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  ClayContainerTreeViewClass *view_class   = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);
  ClayItemTreeViewClass      *iv_class     = CLAY_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed    = clay_channel_tree_view_constructed;

  view_class->drop_viewables   = clay_channel_tree_view_drop_viewables;
  view_class->drop_component   = clay_channel_tree_view_drop_component;

  iv_class->set_image          = clay_channel_tree_view_set_image;

  iv_class->item_type          = CLAY_TYPE_CHANNEL;
  iv_class->signal_name        = "selected-channels-changed";

  iv_class->new_item           = clay_channel_tree_view_item_new;

  iv_class->action_group        = "channels";
  iv_class->activate_action     = "channels-edit-attributes";
  iv_class->new_action          = "channels-new";
  iv_class->new_default_action  = "channels-new-last-values";
  iv_class->raise_action        = "channels-raise";
  iv_class->raise_top_action    = "channels-raise-to-top";
  iv_class->lower_action        = "channels-lower";
  iv_class->lower_bottom_action = "channels-lower-to-bottom";
  iv_class->duplicate_action    = "channels-duplicate";
  iv_class->delete_action       = "channels-delete";

  iv_class->move_cursor_up_action    = "channels-select-previous";
  iv_class->move_cursor_down_action  = "channels-select-next";
  iv_class->move_cursor_start_action = "channels-select-top";
  iv_class->move_cursor_end_action   = "channels-select-bottom";

  iv_class->lock_content_help_id    = CLAY_HELP_CHANNEL_LOCK_PIXELS;
  iv_class->lock_position_help_id   = CLAY_HELP_CHANNEL_LOCK_POSITION;
  iv_class->lock_visibility_help_id = CLAY_HELP_CHANNEL_LOCK_VISIBILITY;
}

static void
clay_channel_tree_view_view_iface_init (ClayContainerViewInterface *view_iface)
{
  parent_view_iface = g_type_interface_peek_parent (view_iface);

  view_iface->set_context   = clay_channel_tree_view_set_context;
  view_iface->set_view_size = clay_channel_tree_view_set_view_size;
}

static void
clay_channel_tree_view_init (ClayChannelTreeView *view)
{
  view->priv = clay_channel_tree_view_get_instance_private (view);

  view->priv->component_editor   = NULL;
  view->priv->toselection_button = NULL;
}

static void
clay_channel_tree_view_constructed (GObject *object)
{
  ClayChannelTreeView   *view      = CLAY_CHANNEL_TREE_VIEW (object);
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  clay_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), CLAY_TYPE_LAYER,
                               NULL, tree_view);
  clay_dnd_viewable_dest_add  (GTK_WIDGET (tree_view->view), CLAY_TYPE_LAYER_MASK,
                               NULL, tree_view);
  clay_dnd_component_dest_add (GTK_WIDGET (tree_view->view),
                               NULL, tree_view);

  view->priv->toselection_button =
    clay_editor_add_action_button (CLAY_EDITOR (view), "channels",
                                   "channels-selection-replace",
                                   "channels-selection-add",
                                   extend_mask,
                                   "channels-selection-subtract",
                                   modify_mask,
                                   "channels-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (view),
                                  GTK_BUTTON (view->priv->toselection_button),
                                  CLAY_TYPE_CHANNEL);
  gtk_box_reorder_child (clay_editor_get_button_box (CLAY_EDITOR (view)),
                         view->priv->toselection_button, 4);
}


/*  ClayContainerTreeView methods  */

static void
clay_channel_tree_view_drop_viewables (ClayContainerTreeView   *tree_view,
                                       GList                   *src_viewables,
                                       ClayViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView      *item_view = CLAY_ITEM_TREE_VIEW (tree_view);
  ClayImage             *image     = clay_item_tree_view_get_image (item_view);
  ClayItemTreeViewClass *item_view_class;
  GList                 *iter;

  item_view_class = CLAY_ITEM_TREE_VIEW_GET_CLASS (item_view);

  for (iter = src_viewables; iter; iter = iter->next)
    {
      ClayViewable *src_viewable = iter->data;

      if (CLAY_IS_DRAWABLE (src_viewable) &&
          (image != clay_item_get_image (CLAY_ITEM (src_viewable)) ||
           G_TYPE_FROM_INSTANCE (src_viewable) != item_view_class->item_type))
        {
          ClayItem *new_item;
          ClayItem *parent;
          gint      index;

          index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                                      drop_pos,
                                                      (ClayViewable **) &parent);

          new_item = clay_item_convert (CLAY_ITEM (src_viewable),
                                        clay_item_tree_view_get_image (item_view),
                                        item_view_class->item_type);

          clay_image_add_item (image, new_item, parent, index, TRUE);
          clay_image_flush (image);

          return;
        }
    }

  CLAY_CONTAINER_TREE_VIEW_CLASS (parent_class)->drop_viewables (tree_view,
                                                                 src_viewables,
                                                                 dest_viewable,
                                                                 drop_pos);
}

static void
clay_channel_tree_view_drop_component (ClayContainerTreeView   *tree_view,
                                       ClayImage               *src_image,
                                       ClayChannelType          component,
                                       ClayViewable            *dest_viewable,
                                       GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (tree_view);
  ClayImage        *image     = clay_item_tree_view_get_image (item_view);
  ClayItem         *new_item;
  ClayChannel      *parent;
  gint              index;
  const gchar      *desc;
  gchar            *name;

  index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (ClayViewable **) &parent);

  clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                       NULL, NULL, &desc, NULL);
  name = g_strdup_printf (_("%s Channel Copy"), desc);

  new_item = CLAY_ITEM (clay_channel_new_from_component (src_image, component,
                                                         name, NULL));

  /*  copied components are invisible by default so subsequent copies
   *  of components don't affect each other
   */
  clay_item_set_visible (new_item, FALSE, FALSE);

  g_free (name);

  if (src_image != image)
    CLAY_ITEM_GET_CLASS (new_item)->convert (new_item, image,
                                             CLAY_TYPE_CHANNEL);

  clay_image_add_channel (image, CLAY_CHANNEL (new_item), parent, index, TRUE);

  clay_image_flush (image);
}


/*  ClayItemTreeView methods  */

static void
clay_channel_tree_view_set_image (ClayItemTreeView *item_view,
                                  ClayImage        *image)
{
  ClayChannelTreeView *channel_view = CLAY_CHANNEL_TREE_VIEW (item_view);

  if (! channel_view->priv->component_editor)
    {
      ClayContainerView *view = CLAY_CONTAINER_VIEW (item_view);
      gint               view_size;

      view_size = clay_container_view_get_view_size (view, NULL);

      channel_view->priv->component_editor =
        clay_component_editor_new (view_size,
                                   clay_editor_get_menu_factory (CLAY_EDITOR (item_view)));
      clay_docked_set_context (CLAY_DOCKED (channel_view->priv->component_editor),
                               clay_container_view_get_context (view));
      gtk_box_pack_start (GTK_BOX (item_view), channel_view->priv->component_editor,
                          FALSE, FALSE, 0);
      gtk_box_reorder_child (GTK_BOX (item_view),
                             channel_view->priv->component_editor, 0);
    }

  if (! image)
    gtk_widget_set_visible (channel_view->priv->component_editor, FALSE);

  clay_image_editor_set_image (CLAY_IMAGE_EDITOR (channel_view->priv->component_editor),
                               image);

  CLAY_ITEM_TREE_VIEW_CLASS (parent_class)->set_image (item_view, image);

  if (clay_item_tree_view_get_image (item_view))
    gtk_widget_set_visible (channel_view->priv->component_editor, TRUE);
}

static ClayItem *
clay_channel_tree_view_item_new (ClayImage *image)
{
  ClayChannel *new_channel;
  GeglColor   *color = gegl_color_new ("black");

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_EDIT_PASTE,
                               _("New Channel"));

  clay_color_set_alpha (color, 0.5);
  new_channel = clay_channel_new (image,
                                  clay_image_get_width (image),
                                  clay_image_get_height (image),
                                  _("Channel"), color);
  g_object_unref (color);

  clay_image_add_channel (image, new_channel,
                          CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  clay_image_undo_group_end (image);

  return CLAY_ITEM (new_channel);
}


/*  ClayContainerView methods  */

static void
clay_channel_tree_view_set_context (ClayContainerView *view,
                                    ClayContext       *context)
{
  ClayChannelTreeView *channel_view = CLAY_CHANNEL_TREE_VIEW (view);

  parent_view_iface->set_context (view, context);

  if (channel_view->priv->component_editor)
    clay_docked_set_context (CLAY_DOCKED (channel_view->priv->component_editor),
                             context);
}

static void
clay_channel_tree_view_set_view_size (ClayContainerView *view)
{
  ClayChannelTreeView *channel_view = CLAY_CHANNEL_TREE_VIEW (view);
  gint                 view_size;

  parent_view_iface->set_view_size (view);

  view_size = clay_container_view_get_view_size (view, NULL);

  if (channel_view->priv->component_editor)
    clay_component_editor_set_view_size (CLAY_COMPONENT_EDITOR (channel_view->priv->component_editor),
                                         view_size);
}
