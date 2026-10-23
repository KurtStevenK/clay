/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypathstreeview.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/clayimage.h"

#include "path/claypath.h"
#include "path/claypath-export.h"
#include "path/claypath-import.h"

#include "clayactiongroup.h"
#include "claycontainerview.h"
#include "claydnd.h"
#include "clayhelp-ids.h"
#include "clayuimanager.h"
#include "claypathtreeview.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void    clay_path_tree_view_view_iface_init (ClayContainerViewInterface  *iface);

static void      clay_path_tree_view_constructed   (GObject                     *object);

static void      clay_path_tree_view_set_container (ClayContainerView           *view,
                                                    ClayContainer               *container);
static void      clay_path_tree_view_drop_svg      (ClayContainerTreeView       *tree_view,
                                                    const gchar                 *svg_data,
                                                    gsize                        svg_data_len,
                                                    ClayViewable                *dest_viewable,
                                                    GtkTreeViewDropPosition      drop_pos);
static ClayItem * clay_path_tree_view_item_new     (ClayImage                   *image);
static guchar   * clay_path_tree_view_drag_svg     (GtkWidget                   *widget,
                                                    gsize                       *svg_data_len,
                                                    gpointer                     data);


G_DEFINE_TYPE_WITH_CODE (ClayPathTreeView, clay_path_tree_view,
                         CLAY_TYPE_ITEM_TREE_VIEW,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONTAINER_VIEW,
                                                clay_path_tree_view_view_iface_init))

#define parent_class clay_path_tree_view_parent_class

static ClayContainerViewInterface *parent_view_iface = NULL;


static void
clay_path_tree_view_class_init (ClayPathTreeViewClass *klass)
{
  GObjectClass               *object_class = G_OBJECT_CLASS (klass);
  ClayContainerTreeViewClass *view_class   = CLAY_CONTAINER_TREE_VIEW_CLASS (klass);
  ClayItemTreeViewClass      *pv_vlass     = CLAY_ITEM_TREE_VIEW_CLASS (klass);

  object_class->constructed    = clay_path_tree_view_constructed;

  view_class->drop_svg         = clay_path_tree_view_drop_svg;

  pv_vlass->item_type          = CLAY_TYPE_PATH;
  pv_vlass->signal_name        = "selected-paths-changed";

  pv_vlass->new_item           = clay_path_tree_view_item_new;

  pv_vlass->action_group        = "paths";
  pv_vlass->activate_action     = "paths-edit";
  pv_vlass->new_action          = "paths-new";
  pv_vlass->new_default_action  = "paths-new-last-values";
  pv_vlass->raise_action        = "paths-raise";
  pv_vlass->raise_top_action    = "paths-raise-to-top";
  pv_vlass->lower_action        = "paths-lower";
  pv_vlass->lower_bottom_action = "paths-lower-to-bottom";
  pv_vlass->duplicate_action    = "paths-duplicate";
  pv_vlass->delete_action       = "paths-delete";

  pv_vlass->move_cursor_up_action    = "paths-select-previous";
  pv_vlass->move_cursor_down_action  = "paths-select-next";
  pv_vlass->move_cursor_start_action = "paths-select-top";
  pv_vlass->move_cursor_end_action   = "paths-select-bottom";

  pv_vlass->lock_content_icon_name    = CLAY_ICON_LOCK_PATH;
  pv_vlass->lock_content_tooltip      = _("Lock path");
  pv_vlass->lock_content_help_id      = CLAY_HELP_PATH_LOCK_STROKES;
  pv_vlass->lock_position_icon_name   = CLAY_ICON_LOCK_POSITION;
  pv_vlass->lock_position_tooltip     = _("Lock path position");
  pv_vlass->lock_position_help_id     = CLAY_HELP_PATH_LOCK_POSITION;
  pv_vlass->lock_visibility_icon_name = CLAY_ICON_LOCK_VISIBILITY;
  pv_vlass->lock_visibility_tooltip   = _("Lock path visibility");
  pv_vlass->lock_position_help_id     = CLAY_HELP_PATH_LOCK_VISIBILITY;
}

static void
clay_path_tree_view_view_iface_init (ClayContainerViewInterface *iface)
{
  parent_view_iface = g_type_interface_peek_parent (iface);

  iface->set_container = clay_path_tree_view_set_container;
}

static void
clay_path_tree_view_init (ClayPathTreeView *view)
{
}

static void
clay_path_tree_view_constructed (GObject *object)
{
  ClayEditor            *editor    = CLAY_EDITOR (object);
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (object);
  ClayPathTreeView      *view      = CLAY_PATH_TREE_VIEW (object);
  GdkModifierType        extend_mask;
  GdkModifierType        modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  view->to_selection_button =
    clay_editor_add_action_button (editor, "paths",
                                   "paths-selection-replace",
                                   "paths-selection-add",
                                   extend_mask,
                                   "paths-selection-subtract",
                                   modify_mask,
                                   "paths-selection-intersect",
                                   extend_mask | modify_mask,
                                   NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->to_selection_button),
                                  CLAY_TYPE_PATH);
  gtk_box_reorder_child (clay_editor_get_button_box (editor),
                         view->to_selection_button, 4);

  view->to_path_button =
    clay_editor_add_action_button (editor, "paths",
                                   "paths-selection-to-path",
                                   "paths-selection-to-path-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);
  gtk_box_reorder_child (clay_editor_get_button_box (editor),
                         view->to_path_button, 5);

  view->stroke_button =
    clay_editor_add_action_button (editor, "paths",
                                   "paths-stroke",
                                   "paths-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
  clay_container_view_enable_dnd (CLAY_CONTAINER_VIEW (editor),
                                  GTK_BUTTON (view->stroke_button),
                                  CLAY_TYPE_PATH);
  gtk_box_reorder_child (clay_editor_get_button_box (editor),
                         view->stroke_button, 6);

  clay_dnd_svg_dest_add (GTK_WIDGET (tree_view->view), NULL, view);
}

static void
clay_path_tree_view_set_container (ClayContainerView *view,
                                   ClayContainer     *container)
{
  ClayContainerTreeView *tree_view = CLAY_CONTAINER_TREE_VIEW (view);
  ClayContainer         *old_container;

  old_container = clay_container_view_get_container (CLAY_CONTAINER_VIEW (view));

  if (old_container && ! container)
    {
      clay_dnd_svg_source_remove (GTK_WIDGET (tree_view->view));
    }

  parent_view_iface->set_container (view, container);

  if (! old_container && container)
    {
      clay_dnd_svg_source_add (GTK_WIDGET (tree_view->view),
                               clay_path_tree_view_drag_svg,
                               tree_view);
    }
}

static void
clay_path_tree_view_drop_svg (ClayContainerTreeView   *tree_view,
                              const gchar             *svg_data,
                              gsize                    svg_data_len,
                              ClayViewable            *dest_viewable,
                              GtkTreeViewDropPosition  drop_pos)
{
  ClayItemTreeView *item_view = CLAY_ITEM_TREE_VIEW (tree_view);
  ClayImage        *image     = clay_item_tree_view_get_image (item_view);
  ClayPath         *parent;
  gint              index;
  GError           *error = NULL;

  if (image->clay->be_verbose)
    g_print ("%s: SVG dropped (len = %d)\n", G_STRFUNC, (gint) svg_data_len);

  index = clay_item_tree_view_get_drop_index (item_view, dest_viewable,
                                              drop_pos,
                                              (ClayViewable **) &parent);

  if (! clay_path_import_buffer (image, svg_data, svg_data_len,
                                 TRUE, FALSE, parent, index, NULL, &error))
    {
      clay_message_literal (image->clay,
                            G_OBJECT (tree_view), CLAY_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);
    }
  else
    {
      clay_image_flush (image);
    }
}

static ClayItem *
clay_path_tree_view_item_new (ClayImage *image)
{
  ClayPath *new_path;

  new_path = clay_path_new (image, _("Path"));

  clay_image_add_path (image, new_path,
                       CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);

  return CLAY_ITEM (new_path);
}

static guchar *
clay_path_tree_view_drag_svg (GtkWidget *widget,
                              gsize     *svg_data_len,
                              gpointer   data)
{
  ClayItemTreeView *view  = CLAY_ITEM_TREE_VIEW (data);
  ClayImage        *image = clay_item_tree_view_get_image (view);
  GList            *items;
  gchar            *svg_data = NULL;

  items = clay_image_get_selected_paths (image);

  *svg_data_len = 0;

  if (items)
    {
      svg_data = clay_path_export_string (image, items);

      if (svg_data)
        *svg_data_len = strlen (svg_data);
    }

  return (guchar *) svg_data;
}
