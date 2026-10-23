/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claycomponenteditor.c
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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

#include "widgets-types.h"

#include "core/claychannel.h"
#include "core/clayimage.h"

#include "claycellrendererviewable.h"
#include "claycomponenteditor.h"
#include "claydnd.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "clayviewrendererimage.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


enum
{
  COLUMN_CHANNEL,
  COLUMN_VISIBLE,
  COLUMN_RENDERER,
  COLUMN_NAME,
  N_COLUMNS
};


static void clay_component_editor_docked_iface_init (ClayDockedInterface *iface);

static void clay_component_editor_set_context       (ClayDocked          *docked,
                                                     ClayContext         *context);

static void clay_component_editor_set_image         (ClayImageEditor     *editor,
                                                     ClayImage           *image);

static void clay_component_editor_create_components (ClayComponentEditor *editor);
static void clay_component_editor_clear_components  (ClayComponentEditor *editor);
static void clay_component_editor_clicked         (GtkCellRendererToggle *cellrenderertoggle,
                                                   gchar                 *path,
                                                   GdkModifierType        state,
                                                   ClayComponentEditor   *editor);
static gboolean clay_component_editor_select        (GtkTreeSelection    *selection,
                                                     GtkTreeModel        *model,
                                                     GtkTreePath         *path,
                                                     gboolean             path_currently_selected,
                                                     gpointer             data);
static gboolean clay_component_editor_button_press  (GtkWidget           *widget,
                                                     GdkEventButton      *bevent,
                                                     ClayComponentEditor *editor);
static void clay_component_editor_renderer_update   (ClayViewRenderer    *renderer,
                                                     ClayComponentEditor *editor);
static void clay_component_editor_mode_changed      (ClayImage           *image,
                                                     ClayComponentEditor *editor);
static void clay_component_editor_alpha_changed     (ClayImage           *image,
                                                     ClayComponentEditor *editor);
static void clay_component_editor_visibility_changed(ClayImage           *image,
                                                     ClayChannelType      channel,
                                                     ClayComponentEditor *editor);
static void clay_component_editor_active_changed    (ClayImage           *image,
                                                     ClayChannelType      channel,
                                                     ClayComponentEditor *editor);
static ClayImage * clay_component_editor_drag_component (GtkWidget       *widget,
                                                         ClayContext    **context,
                                                         ClayChannelType *channel,
                                                         gpointer         data);


G_DEFINE_TYPE_WITH_CODE (ClayComponentEditor, clay_component_editor,
                         CLAY_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_component_editor_docked_iface_init))

#define parent_class clay_component_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_component_editor_class_init (ClayComponentEditorClass *klass)
{
  ClayImageEditorClass *image_editor_class = CLAY_IMAGE_EDITOR_CLASS (klass);

  image_editor_class->set_image = clay_component_editor_set_image;
}

static void
clay_component_editor_init (ClayComponentEditor *editor)
{
  GtkWidget    *frame;
  GtkListStore *list;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, FALSE, FALSE, 0);
  gtk_widget_set_visible (frame, TRUE);

  list = gtk_list_store_new (N_COLUMNS,
                             G_TYPE_INT,
                             G_TYPE_BOOLEAN,
                             CLAY_TYPE_VIEW_RENDERER,
                             G_TYPE_STRING);
  editor->model = GTK_TREE_MODEL (list);

  editor->view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (editor->model));
  g_object_unref (list);

  gtk_tree_view_set_headers_visible (editor->view, FALSE);
  gtk_tree_view_set_enable_search (editor->view, FALSE);

  editor->eye_column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (editor->view, editor->eye_column);

  editor->eye_cell = clay_cell_renderer_toggle_new (CLAY_ICON_VISIBLE);
  gtk_tree_view_column_pack_start (editor->eye_column, editor->eye_cell,
                                   FALSE);
  gtk_tree_view_column_set_attributes (editor->eye_column, editor->eye_cell,
                                       "active", COLUMN_VISIBLE,
                                       NULL);

  g_signal_connect (editor->eye_cell, "clicked",
                    G_CALLBACK (clay_component_editor_clicked),
                    editor);

  editor->renderer_cell = clay_cell_renderer_viewable_new ();
  gtk_tree_view_insert_column_with_attributes (editor->view,
                                               -1, NULL,
                                               editor->renderer_cell,
                                               "renderer", COLUMN_RENDERER,
                                               NULL);

  gtk_tree_view_insert_column_with_attributes (editor->view,
                                               -1, NULL,
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (editor->view));
  gtk_widget_set_visible (GTK_WIDGET (editor->view), TRUE);

  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (clay_component_editor_button_press),
                    editor);

  editor->selection = gtk_tree_view_get_selection (editor->view);
  gtk_tree_selection_set_mode (editor->selection, GTK_SELECTION_MULTIPLE);

  gtk_tree_selection_set_select_function (editor->selection,
                                          clay_component_editor_select,
                                          editor, NULL);

  clay_dnd_component_source_add (GTK_WIDGET (editor->view),
                                 clay_component_editor_drag_component,
                                 editor);
}

static void
clay_component_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_component_editor_set_context;
}

static void
clay_component_editor_set_context (ClayDocked  *docked,
                                   ClayContext *context)
{
  ClayComponentEditor *editor = CLAY_COMPONENT_EDITOR (docked);
  GtkTreeIter          iter;
  gboolean             iter_valid;

  parent_docked_iface->set_context (docked, context);

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      ClayViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      clay_view_renderer_set_context (renderer, context);
      g_object_unref (renderer);
    }
}

static void
clay_component_editor_set_image (ClayImageEditor *editor,
                                 ClayImage       *image)
{
  ClayComponentEditor *component_editor = CLAY_COMPONENT_EDITOR (editor);

  if (editor->image)
    {
      clay_component_editor_clear_components (component_editor);

      g_signal_handlers_disconnect_by_func (editor->image,
                                            clay_component_editor_mode_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            clay_component_editor_alpha_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            clay_component_editor_visibility_changed,
                                            component_editor);
      g_signal_handlers_disconnect_by_func (editor->image,
                                            clay_component_editor_active_changed,
                                            component_editor);
    }

  CLAY_IMAGE_EDITOR_CLASS (parent_class)->set_image (editor, image);

  if (editor->image)
    {
      clay_component_editor_create_components (component_editor);

      g_signal_connect (editor->image, "mode-changed",
                        G_CALLBACK (clay_component_editor_mode_changed),
                        component_editor);
      g_signal_connect (editor->image, "alpha-changed",
                        G_CALLBACK (clay_component_editor_alpha_changed),
                        component_editor);
      g_signal_connect (editor->image, "component-visibility-changed",
                        G_CALLBACK (clay_component_editor_visibility_changed),
                        component_editor);
      g_signal_connect (editor->image, "component-active-changed",
                        G_CALLBACK (clay_component_editor_active_changed),
                        component_editor);
    }
}

GtkWidget *
clay_component_editor_new (gint             view_size,
                           ClayMenuFactory *menu_factory)
{
  ClayComponentEditor *editor;

  g_return_val_if_fail (view_size > 0 &&
                        view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  editor = g_object_new (CLAY_TYPE_COMPONENT_EDITOR,
                         "menu-factory",    menu_factory,
                         "menu-identifier", "<Channels>",
                         "ui-path",         "/channels-popup",
                         NULL);

  clay_component_editor_set_view_size (editor, view_size);

  return GTK_WIDGET (editor);
}

void
clay_component_editor_set_view_size (ClayComponentEditor *editor,
                                     gint                 view_size)
{
  GtkWidget       *tree_widget;
  GtkStyleContext *tree_style;
  GtkBorder        border;
  GtkTreeIter      iter;
  gboolean         iter_valid;
  gint             icon_size;

  g_return_if_fail (CLAY_IS_COMPONENT_EDITOR (editor));
  g_return_if_fail (view_size >  0 &&
                    view_size <= CLAY_VIEWABLE_MAX_PREVIEW_SIZE);

  tree_widget = GTK_WIDGET (editor->view);
  tree_style  = gtk_widget_get_style_context (tree_widget);

  gtk_style_context_save (tree_style);
  gtk_style_context_add_class (tree_style, GTK_STYLE_CLASS_BUTTON);
  gtk_style_context_get_border (tree_style, 0, &border);
  gtk_style_context_restore (tree_style);

  g_object_get (editor->eye_cell, "icon-size", &icon_size, NULL);
  icon_size = MIN (icon_size, MAX (view_size - (border.left + border.right),
                                   view_size - (border.top + border.bottom)));
  g_object_set (editor->eye_cell, "icon-size", icon_size, NULL);

  for (iter_valid = gtk_tree_model_get_iter_first (editor->model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (editor->model, &iter))
    {
      ClayViewRenderer *renderer;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_RENDERER, &renderer,
                          -1);

      clay_view_renderer_set_size (renderer, view_size, 1);
      g_object_unref (renderer);
    }

  editor->view_size = view_size;

  gtk_tree_view_columns_autosize (editor->view);
}

static void
clay_component_editor_create_components (ClayComponentEditor *editor)
{
  ClayImage       *image        = CLAY_IMAGE_EDITOR (editor)->image;
  gint             n_components = 0;
  ClayChannelType  components[MAX_CHANNELS];
  GEnumClass      *enum_class;
  gint             i;

  switch (clay_image_get_base_type (image))
    {
    case CLAY_RGB:
      n_components  = 3;
      components[0] = CLAY_CHANNEL_RED;
      components[1] = CLAY_CHANNEL_GREEN;
      components[2] = CLAY_CHANNEL_BLUE;
      break;

    case CLAY_GRAY:
      n_components  = 1;
      components[0] = CLAY_CHANNEL_GRAY;
      break;

    case CLAY_INDEXED:
      n_components  = 1;
      components[0] = CLAY_CHANNEL_INDEXED;
      break;
    }

  if (clay_image_has_alpha (image))
    components[n_components++] = CLAY_CHANNEL_ALPHA;

  enum_class = g_type_class_ref (CLAY_TYPE_CHANNEL_TYPE);

  for (i = 0; i < n_components; i++)
    {
      ClayViewRenderer *renderer;
      GtkTreeIter       iter;
      GEnumValue       *enum_value;
      const gchar      *desc;
      gboolean          visible;

      visible = clay_image_get_component_visible (image, components[i]);

      renderer = clay_view_renderer_new (CLAY_IMAGE_EDITOR (editor)->context,
                                         G_TYPE_FROM_INSTANCE (image),
                                         editor->view_size, 1, FALSE);
      clay_view_renderer_set_viewable (renderer, CLAY_VIEWABLE (image));
      clay_view_renderer_remove_idle (renderer);

      CLAY_VIEW_RENDERER_IMAGE (renderer)->channel = components[i];

      g_signal_connect (renderer, "update",
                        G_CALLBACK (clay_component_editor_renderer_update),
                        editor);

      enum_value = g_enum_get_value (enum_class, components[i]);
      desc = clay_enum_value_get_desc (enum_class, enum_value);

      gtk_list_store_append (GTK_LIST_STORE (editor->model), &iter);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_CHANNEL,  components[i],
                          COLUMN_VISIBLE,  visible,
                          COLUMN_RENDERER, renderer,
                          COLUMN_NAME,     desc,
                          -1);

      g_object_unref (renderer);

      if (clay_image_get_component_active (image, components[i]))
        gtk_tree_selection_select_iter (editor->selection, &iter);
    }

  g_type_class_unref (enum_class);
}

static void
clay_component_editor_clear_components (ClayComponentEditor *editor)
{
  gtk_list_store_clear (GTK_LIST_STORE (editor->model));

  /*  Clear the renderer so that it don't reference the viewable.
   *  See bug #149906.
   */
  g_object_set (editor->renderer_cell, "renderer", NULL, NULL);
}

static void
clay_component_editor_clicked (GtkCellRendererToggle *cellrenderertoggle,
                               gchar                 *path_str,
                               GdkModifierType        state,
                               ClayComponentEditor   *editor)
{
  GtkTreePath *path;
  GtkTreeIter  iter;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (editor->model, &iter, path))
    {
      ClayImage       *image = CLAY_IMAGE_EDITOR (editor)->image;
      ClayChannelType  channel;
      gboolean         active;

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);
      g_object_get (cellrenderertoggle,
                    "active", &active,
                    NULL);

      clay_image_set_component_visible (image, channel, !active);
      clay_image_flush (image);
    }

  gtk_tree_path_free (path);
}

static gboolean
clay_component_editor_select (GtkTreeSelection *selection,
                              GtkTreeModel     *model,
                              GtkTreePath      *path,
                              gboolean          path_currently_selected,
                              gpointer          data)
{
  ClayComponentEditor *editor = CLAY_COMPONENT_EDITOR (data);
  GtkTreeIter          iter;
  ClayChannelType      channel;
  gboolean             active;

  gtk_tree_model_get_iter (editor->model, &iter, path);
  gtk_tree_model_get (editor->model, &iter,
                      COLUMN_CHANNEL, &channel,
                      -1);

  active = clay_image_get_component_active (CLAY_IMAGE_EDITOR (editor)->image,
                                            channel);

  return active != path_currently_selected;
}

static gboolean
clay_component_editor_button_press (GtkWidget           *widget,
                                    GdkEventButton      *bevent,
                                    ClayComponentEditor *editor)
{
  GtkTreeViewColumn *column;
  GtkTreePath       *path;

  editor->clicked_component = -1;

  if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
                                     bevent->x,
                                     bevent->y,
                                     &path, &column, NULL, NULL))
    {
      GtkTreeIter     iter;
      ClayChannelType channel;
      gboolean        active;

      active = gtk_tree_selection_path_is_selected (editor->selection, path);

      gtk_tree_model_get_iter (editor->model, &iter, path);

      gtk_tree_path_free (path);

      gtk_tree_model_get (editor->model, &iter,
                          COLUMN_CHANNEL, &channel,
                          -1);

      editor->clicked_component = channel;

      if (gdk_event_triggers_context_menu ((GdkEvent *) bevent))
        {
          clay_editor_popup_menu_at_pointer (CLAY_EDITOR (editor), (GdkEvent *) bevent);
        }
      else if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS && bevent->button == 1 &&
               column != editor->eye_column)
        {
          ClayImage *image = CLAY_IMAGE_EDITOR (editor)->image;

          clay_image_set_component_active (image, channel, ! active);
          clay_image_flush (image);
        }
    }

  return FALSE;
}

static gboolean
clay_component_editor_get_iter (ClayComponentEditor *editor,
                                ClayChannelType      channel,
                                GtkTreeIter         *iter)
{
  gint index;

  index = clay_image_get_component_index (CLAY_IMAGE_EDITOR (editor)->image,
                                          channel);

  if (index != -1)
    return gtk_tree_model_iter_nth_child (editor->model, iter, NULL, index);

  return FALSE;
}

static void
clay_component_editor_renderer_update (ClayViewRenderer    *renderer,
                                       ClayComponentEditor *editor)
{
  ClayChannelType channel = CLAY_VIEW_RENDERER_IMAGE (renderer)->channel;
  GtkTreeIter     iter;

  if (clay_component_editor_get_iter (editor, channel, &iter))
    {
      GtkTreePath *path;

      path = gtk_tree_model_get_path (editor->model, &iter);
      gtk_tree_model_row_changed (editor->model, path, &iter);
      gtk_tree_path_free (path);
    }
}

static void
clay_component_editor_mode_changed (ClayImage           *image,
                                    ClayComponentEditor *editor)
{
  clay_component_editor_clear_components (editor);
  clay_component_editor_create_components (editor);
}

static void
clay_component_editor_alpha_changed (ClayImage           *image,
                                     ClayComponentEditor *editor)
{
  clay_component_editor_clear_components (editor);
  clay_component_editor_create_components (editor);
}

static void
clay_component_editor_visibility_changed (ClayImage           *image,
                                          ClayChannelType      channel,
                                          ClayComponentEditor *editor)
{
  GtkTreeIter iter;

  if (clay_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean visible = clay_image_get_component_visible (image, channel);

      gtk_list_store_set (GTK_LIST_STORE (editor->model), &iter,
                          COLUMN_VISIBLE, visible,
                          -1);
    }
}

static void
clay_component_editor_active_changed (ClayImage           *image,
                                      ClayChannelType      channel,
                                      ClayComponentEditor *editor)
{
  GtkTreeIter iter;

  if (clay_component_editor_get_iter (editor, channel, &iter))
    {
      gboolean active = clay_image_get_component_active (image, channel);

      if (gtk_tree_selection_iter_is_selected (editor->selection, &iter) !=
          active)
        {
          if (active)
            gtk_tree_selection_select_iter (editor->selection, &iter);
          else
            gtk_tree_selection_unselect_iter (editor->selection, &iter);
        }
    }
}

static ClayImage *
clay_component_editor_drag_component (GtkWidget        *widget,
                                      ClayContext     **context,
                                      ClayChannelType  *channel,
                                      gpointer          data)
{
  ClayComponentEditor *editor = CLAY_COMPONENT_EDITOR (data);

  if (CLAY_IMAGE_EDITOR (editor)->image &&
      editor->clicked_component != -1)
    {
      if (channel)
        *channel = editor->clicked_component;

      if (context)
        *context = CLAY_IMAGE_EDITOR (editor)->context;

      return CLAY_IMAGE_EDITOR (editor)->image;
    }

  return NULL;
}
