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

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claychannel-select.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/clayimage-pick-color.h"
#include "core/clayselection.h"
#include "core/claytoolinfo.h"

/* FIXME: #include "tools/tools-types.h" */
#include "tools/tools-types.h"
#include "tools/clayregionselectoptions.h"

#include "clayselectioneditor.h"
#include "claydnd.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "clayview.h"
#include "clayviewrenderer.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void  clay_selection_editor_docked_iface_init (ClayDockedInterface *iface);

static void   clay_selection_editor_constructed    (GObject             *object);

static void   clay_selection_editor_set_image      (ClayImageEditor     *editor,
                                                    ClayImage           *image);

static void   clay_selection_editor_set_context    (ClayDocked          *docked,
                                                    ClayContext         *context);

static gboolean clay_selection_view_button_press   (GtkWidget           *widget,
                                                    GdkEventButton      *bevent,
                                                    ClaySelectionEditor *editor);
static void   clay_selection_editor_drop_color     (GtkWidget           *widget,
                                                    gint                 x,
                                                    gint                 y,
                                                    GeglColor           *color,
                                                    gpointer             data);

static void   clay_selection_editor_mask_changed   (ClayImage           *image,
                                                    ClaySelectionEditor *editor);


G_DEFINE_TYPE_WITH_CODE (ClaySelectionEditor, clay_selection_editor,
                         CLAY_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_selection_editor_docked_iface_init))

#define parent_class clay_selection_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_selection_editor_class_init (ClaySelectionEditorClass *klass)
{
  GObjectClass         *object_class       = G_OBJECT_CLASS (klass);
  ClayImageEditorClass *image_editor_class = CLAY_IMAGE_EDITOR_CLASS (klass);

  object_class->constructed     = clay_selection_editor_constructed;

  image_editor_class->set_image = clay_selection_editor_set_image;
}

static void
clay_selection_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_selection_editor_set_context;
}

static void
clay_selection_editor_init (ClaySelectionEditor *editor)
{
  GtkWidget *frame;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_set_visible (frame, TRUE);

  editor->view = clay_view_new_by_types (NULL,
                                         CLAY_TYPE_VIEW,
                                         CLAY_TYPE_SELECTION,
                                         CLAY_VIEW_SIZE_HUGE,
                                         0, TRUE);
  gtk_widget_set_size_request (editor->view,
                               CLAY_VIEW_SIZE_HUGE, CLAY_VIEW_SIZE_HUGE);
  clay_view_set_expand (CLAY_VIEW (editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), editor->view);
  gtk_widget_set_visible (editor->view, TRUE);

  g_signal_connect (editor->view, "button-press-event",
                    G_CALLBACK (clay_selection_view_button_press),
                    editor);

  clay_dnd_color_dest_add (editor->view,
                           clay_selection_editor_drop_color,
                           editor);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

static void
clay_selection_editor_constructed (GObject *object)
{
  ClaySelectionEditor *editor = CLAY_SELECTION_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  editor->all_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "select",
                                   "select-all", NULL);

  editor->none_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "select",
                                   "select-none", NULL);

  editor->invert_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "select",
                                   "select-invert", NULL);

  editor->save_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "select",
                                   "select-save", NULL);

  editor->path_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "paths",
                                   "paths-selection-to-path",
                                   "paths-selection-to-path-advanced",
                                   GDK_SHIFT_MASK,
                                   NULL);

  editor->stroke_button =
    clay_editor_add_action_button (CLAY_EDITOR (editor), "select",
                                   "select-stroke",
                                   "select-stroke-last-values",
                                   GDK_SHIFT_MASK,
                                   NULL);
}

static void
clay_selection_editor_set_image (ClayImageEditor *image_editor,
                                 ClayImage       *image)
{
  ClaySelectionEditor *editor = CLAY_SELECTION_EDITOR (image_editor);

  if (image_editor->image)
    {
      g_signal_handlers_disconnect_by_func (image_editor->image,
                                            clay_selection_editor_mask_changed,
                                            editor);
    }

  CLAY_IMAGE_EDITOR_CLASS (parent_class)->set_image (image_editor, image);

  if (image)
    {
      g_signal_connect (image, "mask-changed",
                        G_CALLBACK (clay_selection_editor_mask_changed),
                        editor);

      clay_view_set_viewable (CLAY_VIEW (editor->view),
                              CLAY_VIEWABLE (clay_image_get_mask (image)));
    }
  else
    {
      clay_view_set_viewable (CLAY_VIEW (editor->view), NULL);
    }
}

static void
clay_selection_editor_set_context (ClayDocked  *docked,
                                   ClayContext *context)
{
  ClaySelectionEditor *editor = CLAY_SELECTION_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  clay_view_renderer_set_context (CLAY_VIEW (editor->view)->renderer,
                                  context);
}


/*  public functions  */

GtkWidget *
clay_selection_editor_new (ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_SELECTION_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Selection>",
                       "ui-path",         "/selection-popup",
                       NULL);
}

static gboolean
clay_selection_view_button_press (GtkWidget           *widget,
                                  GdkEventButton      *bevent,
                                  ClaySelectionEditor *editor)
{
  ClayImageEditor         *image_editor = CLAY_IMAGE_EDITOR (editor);
  ClayViewRenderer        *renderer;
  ClayToolInfo            *tool_info;
  ClaySelectionOptions    *sel_options;
  ClayRegionSelectOptions *options;
  ClayChannelOps           operation;
  GList                   *drawables;
  gint                     x, y;
  GeglColor               *color;

  if (! image_editor->image)
    return TRUE;

  renderer = CLAY_VIEW (editor->view)->renderer;

  tool_info = clay_get_tool_info (image_editor->image->clay,
                                  "clay-by-color-select-tool");

  if (! tool_info)
    return TRUE;

  sel_options = CLAY_SELECTION_OPTIONS (tool_info->tool_options);
  options     = CLAY_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawables = clay_image_get_selected_drawables (image_editor->image);

  if (! drawables)
    return TRUE;

  operation = clay_modifiers_to_channel_op (bevent->state);

  x = clay_image_get_width  (image_editor->image) * bevent->x / renderer->width;
  y = clay_image_get_height (image_editor->image) * bevent->y / renderer->height;

  color = gegl_color_new ("black");
  if (clay_image_pick_color (image_editor->image, drawables, x, y,
                             FALSE, options->sample_merged,
                             FALSE, 0.0,
                             NULL,
                             NULL, &color))
    {
      clay_channel_select_by_color (clay_image_get_mask (image_editor->image),
                                    drawables,
                                    options->sample_merged,
                                    color,
                                    options->threshold / 255.0,
                                    options->select_transparent,
                                    options->select_criterion,
                                    operation,
                                    sel_options->antialias,
                                    sel_options->feather,
                                    sel_options->feather_radius,
                                    sel_options->feather_radius);
      clay_image_flush (image_editor->image);
    }

  g_list_free (drawables);
  g_object_unref (color);

  return TRUE;
}

static void
clay_selection_editor_drop_color (GtkWidget *widget,
                                  gint       x,
                                  gint       y,
                                  GeglColor *color,
                                  gpointer   data)
{
  ClayImageEditor         *editor = CLAY_IMAGE_EDITOR (data);
  ClayToolInfo            *tool_info;
  ClaySelectionOptions    *sel_options;
  ClayRegionSelectOptions *options;
  GList                   *drawables;

  if (! editor->image)
    return;

  tool_info = clay_get_tool_info (editor->image->clay,
                                  "clay-by-color-select-tool");
  if (! tool_info)
    return;

  sel_options = CLAY_SELECTION_OPTIONS (tool_info->tool_options);
  options     = CLAY_REGION_SELECT_OPTIONS (tool_info->tool_options);

  drawables = clay_image_get_selected_drawables (editor->image);

  if (! drawables)
    return;

  clay_channel_select_by_color (clay_image_get_mask (editor->image),
                                drawables,
                                options->sample_merged,
                                color,
                                options->threshold / 255.0,
                                options->select_transparent,
                                options->select_criterion,
                                sel_options->operation,
                                sel_options->antialias,
                                sel_options->feather,
                                sel_options->feather_radius,
                                sel_options->feather_radius);
  clay_image_flush (editor->image);
  g_list_free (drawables);
}

static void
clay_selection_editor_mask_changed (ClayImage           *image,
                                    ClaySelectionEditor *editor)
{
  clay_view_renderer_invalidate (CLAY_VIEW (editor->view)->renderer);
}
