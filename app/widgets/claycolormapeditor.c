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
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-colormap.h"
#include "core/claypalette.h"
#include "core/clayprojection.h"

#include "claycolordialog.h"
#include "claycolormapeditor.h"
#include "claycolormapselection.h"
#include "claydialogfactory.h"
#include "claydocked.h"
#include "claymenufactory.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


static void       clay_colormap_editor_docked_iface_init  (ClayDockedInterface   *face);

static void       clay_colormap_editor_constructed        (GObject               *object);
static void       clay_colormap_editor_dispose            (GObject               *object);

static void       clay_colormap_editor_unmap              (GtkWidget             *widget);

static void       clay_colormap_editor_set_context        (ClayDocked            *docked,
                                                           ClayContext           *context);

static void       clay_colormap_editor_color_update       (ClayColorDialog       *dialog,
                                                           GeglColor             *color,
                                                           ClayColorDialogState   state,
                                                           ClayColormapEditor    *editor);

static gboolean   clay_colormap_editor_entry_button_press (GtkWidget             *widget,
                                                           GdkEvent              *event,
                                                           gpointer               user_data);
static gboolean   clay_colormap_editor_entry_popup        (GtkWidget             *widget,
                                                           gpointer               user_data);
static void       clay_colormap_editor_color_clicked      (ClayColormapEditor    *editor,
                                                           ClayPaletteEntry      *entry,
                                                           GdkModifierType        state);
static void       clay_colormap_editor_notify_index       (ClayColormapSelection *selection,
                                                           const GParamSpec      *pspec,
                                                           ClayColormapEditor    *editor);


G_DEFINE_TYPE_WITH_CODE (ClayColormapEditor, clay_colormap_editor,
                         CLAY_TYPE_IMAGE_EDITOR,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_DOCKED,
                                                clay_colormap_editor_docked_iface_init))

#define parent_class clay_colormap_editor_parent_class

static ClayDockedInterface *parent_docked_iface = NULL;


static void
clay_colormap_editor_class_init (ClayColormapEditorClass* klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = clay_colormap_editor_constructed;
  object_class->dispose        = clay_colormap_editor_dispose;

  widget_class->unmap          = clay_colormap_editor_unmap;
}

static void
clay_colormap_editor_docked_iface_init (ClayDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (CLAY_TYPE_DOCKED);

  iface->set_context = clay_colormap_editor_set_context;
}

static void
clay_colormap_editor_init (ClayColormapEditor *editor)
{
}

static void
clay_colormap_editor_constructed (GObject *object)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (object);
  GdkModifierType     extend_mask;
  GdkModifierType     modify_mask;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* Editor buttons. */
  extend_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_EXTEND_SELECTION);
  modify_mask = gtk_widget_get_modifier_mask (GTK_WIDGET (object),
                                              GDK_MODIFIER_INTENT_MODIFY_SELECTION);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "colormap",
                                 "colormap-edit-color",
                                 NULL);
  clay_editor_add_action_button (CLAY_EDITOR (editor), "colormap",
                                 "colormap-delete-color",
                                 NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "colormap",
                                 "colormap-add-color-from-fg",
                                 "colormap-add-color-from-bg",
                                 clay_get_toggle_behavior_mask (),
                                 NULL);

  clay_editor_add_action_button (CLAY_EDITOR (editor), "colormap",
                                 "colormap-selection-replace",
                                 "colormap-selection-add",
                                 extend_mask,
                                 "colormap-selection-subtract",
                                 modify_mask,
                                 "colormap-selection-intersect",
                                 extend_mask | modify_mask,
                                 NULL);
}

static void
clay_colormap_editor_dispose (GObject *object)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (object);

  g_clear_pointer (&editor->color_dialog, gtk_widget_destroy);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_colormap_editor_unmap (GtkWidget *widget)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (widget);

  if (editor->color_dialog)
    gtk_widget_set_visible (editor->color_dialog, FALSE);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
clay_colormap_editor_set_context (ClayDocked  *docked,
                                  ClayContext *context)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (docked);

  parent_docked_iface->set_context (docked, context);

  if (editor->selection)
    gtk_widget_destroy (editor->selection);
  editor->selection = NULL;

  /* Main selection widget. */
  if (context)
    {
      editor->selection = clay_colormap_selection_new (context);
      gtk_box_pack_start (GTK_BOX (editor), editor->selection, TRUE, TRUE, 0);
      gtk_widget_set_visible (editor->selection, TRUE);

      g_signal_connect_swapped (editor->selection, "color-clicked",
                                G_CALLBACK (clay_colormap_editor_color_clicked),
                                editor);
      g_signal_connect_swapped (editor->selection, "color-activated",
                                G_CALLBACK (clay_colormap_editor_edit_color),
                                editor);
      g_signal_connect (editor->selection, "button-press-event",
                        G_CALLBACK (clay_colormap_editor_entry_button_press),
                        editor);
      g_signal_connect (editor->selection, "popup-menu",
                        G_CALLBACK (clay_colormap_editor_entry_popup),
                        editor);
      g_signal_connect (editor->selection, "notify::index",
                        G_CALLBACK (clay_colormap_editor_notify_index),
                        editor);
    }
}


/*  public functions  */

GtkWidget *
clay_colormap_editor_new (ClayMenuFactory *menu_factory)
{
  g_return_val_if_fail (CLAY_IS_MENU_FACTORY (menu_factory), NULL);

  return g_object_new (CLAY_TYPE_COLORMAP_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<Colormap>",
                       "ui-path",         "/colormap-popup",
                       NULL);
}

void
clay_colormap_editor_edit_color (ClayColormapEditor *editor)
{
  ClayImage *image;
  GeglColor *color;
  gchar     *desc;
  gint       index;

  g_return_if_fail (CLAY_IS_COLORMAP_EDITOR (editor));

  image = CLAY_IMAGE_EDITOR (editor)->image;
  index = clay_colormap_selection_get_index (CLAY_COLORMAP_SELECTION (editor->selection),
                                             NULL);

  if (index == -1)
    /* No colormap. */
    return;

  color = clay_image_get_colormap_entry (image, index);

  desc = g_strdup_printf (_("Edit colormap entry #%d"), index);

  if (! editor->color_dialog)
    {
      editor->color_dialog =
        clay_color_dialog_new (CLAY_VIEWABLE (image),
                               CLAY_IMAGE_EDITOR (editor)->context,
                               FALSE,
                               _("Edit Colormap Entry"),
                               CLAY_ICON_COLORMAP,
                               desc,
                               GTK_WIDGET (editor),
                               clay_dialog_factory_get_singleton (),
                               "clay-colormap-editor-color-dialog",
                               color, TRUE, FALSE);

      g_signal_connect (editor->color_dialog, "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &editor->color_dialog);

      g_signal_connect (editor->color_dialog, "update",
                        G_CALLBACK (clay_colormap_editor_color_update),
                        editor);
    }
  else
    {
      clay_viewable_dialog_set_viewables (CLAY_VIEWABLE_DIALOG (editor->color_dialog),
                                          g_list_prepend (NULL, image),
                                          CLAY_IMAGE_EDITOR (editor)->context);
      g_object_set (editor->color_dialog, "description", desc, NULL);
      clay_color_dialog_set_color (CLAY_COLOR_DIALOG (editor->color_dialog), color);

      if (! gtk_widget_get_visible (editor->color_dialog))
        clay_dialog_factory_position_dialog (clay_dialog_factory_get_singleton (),
                                             "clay-colormap-editor-color-dialog",
                                             editor->color_dialog,
                                             clay_widget_get_monitor (GTK_WIDGET (editor)));
    }

  g_free (desc);

  gtk_window_present (GTK_WINDOW (editor->color_dialog));
}

void
clay_colormap_editor_delete_color (ClayColormapEditor *editor)
{
  ClayColormapSelection *selection;
  ClayImage             *image;
  gint                   index;

  g_return_if_fail (CLAY_IS_COLORMAP_EDITOR (editor));
  g_return_if_fail (clay_colormap_editor_is_color_deletable (editor));

  image     = CLAY_IMAGE_EDITOR (editor)->image;
  selection = CLAY_COLORMAP_SELECTION (editor->selection);
  index     = clay_colormap_selection_get_index (selection, NULL);

  clay_image_delete_colormap_entry (image, index, TRUE);
}

gboolean
clay_colormap_editor_is_color_deletable (ClayColormapEditor *editor)
{
  ClayColormapSelection *selection;
  ClayImage             *image;
  gint                   index;

  g_return_val_if_fail (CLAY_IS_COLORMAP_EDITOR (editor), FALSE);

  image     = CLAY_IMAGE_EDITOR (editor)->image;
  selection = CLAY_COLORMAP_SELECTION (editor->selection);

  if (! selection)
    return FALSE;

  index = clay_colormap_selection_get_index (selection, NULL);

  if (index == -1)
    /* No colormap. */
    return FALSE;
  else
    return ! clay_image_colormap_is_index_used (image, index);
}

gint
clay_colormap_editor_get_index (ClayColormapEditor *editor,
                                GeglColor          *search)
{
  g_return_val_if_fail (CLAY_IS_COLORMAP_EDITOR (editor), 0);

  return clay_colormap_selection_get_index (CLAY_COLORMAP_SELECTION (editor->selection), search);
}

gboolean
clay_colormap_editor_set_index (ClayColormapEditor *editor,
                                gint                index,
                                GeglColor          *color)
{
  g_return_val_if_fail (CLAY_IS_COLORMAP_EDITOR (editor), FALSE);

  return clay_colormap_selection_set_index (CLAY_COLORMAP_SELECTION (editor->selection), index, color);
}

gint
clay_colormap_editor_max_index (ClayColormapEditor *editor)
{
  g_return_val_if_fail (CLAY_IS_COLORMAP_EDITOR (editor), -1);

  return clay_colormap_selection_max_index (CLAY_COLORMAP_SELECTION (editor->selection));
}

static void
clay_colormap_editor_color_update (ClayColorDialog      *dialog,
                                   GeglColor            *color,
                                   ClayColorDialogState  state,
                                   ClayColormapEditor   *editor)
{
  ClayImageEditor *image_editor = CLAY_IMAGE_EDITOR (editor);
  ClayImage       *image        = image_editor->image;
  gboolean         push_undo    = FALSE;

  switch (state)
    {
    case CLAY_COLOR_DIALOG_OK:
        {
          push_undo = TRUE;

          if ((guint)state & (guint)clay_get_toggle_behavior_mask ())
            clay_context_set_background (image_editor->context, color);
          else
            clay_context_set_foreground (image_editor->context, color);
        }
      /* Fall through */

    case CLAY_COLOR_DIALOG_CANCEL:
      gtk_widget_set_visible (editor->color_dialog, FALSE);
      break;

    case CLAY_COLOR_DIALOG_UPDATE:
      break;
    }

  if (image &&
      clay_image_get_base_type (image) == CLAY_INDEXED)
    {
      gint col_index;

      col_index = clay_colormap_selection_get_index (CLAY_COLORMAP_SELECTION (editor->selection),
                                                     NULL);
      if (push_undo)
        {
          GeglColor *old_color;

          old_color = clay_color_selection_get_old_color (CLAY_COLOR_SELECTION (dialog->selection));

          /* Restore old color for undo */
          clay_image_set_colormap_entry (image, col_index, old_color, FALSE);

          g_object_unref (old_color);
        }

      clay_image_set_colormap_entry (image, col_index, color, push_undo);

      if (push_undo)
        clay_image_flush (image);
      else
        clay_projection_flush (clay_image_get_projection (image));
    }
}

static gboolean
clay_colormap_editor_entry_button_press (GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   user_data)
{
  if (gdk_event_triggers_context_menu (event))
    {
      clay_editor_popup_menu_at_pointer (CLAY_EDITOR (user_data), event);
      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static gboolean
clay_colormap_editor_entry_popup (GtkWidget *widget,
                                  gpointer   user_data)
{
  ClayColormapEditor *editor = CLAY_COLORMAP_EDITOR (user_data);
  ClayColormapSelection *selection = CLAY_COLORMAP_SELECTION (widget);
  ClayPaletteEntry *selected;
  GdkRectangle rect;

  selected = clay_colormap_selection_get_selected_entry (selection);
  if (!selected)
    return GDK_EVENT_PROPAGATE;

  clay_colormap_selection_get_entry_rect (selection, selected, &rect);
  return clay_editor_popup_menu_at_rect (CLAY_EDITOR (editor),
                                         gtk_widget_get_window (widget),
                                         &rect, GDK_GRAVITY_CENTER, GDK_GRAVITY_NORTH_WEST,
                                         NULL);
}

static void
clay_colormap_editor_color_clicked (ClayColormapEditor *editor,
                                    ClayPaletteEntry   *entry,
                                    GdkModifierType     state)
{
  ClayImageEditor *image_editor = CLAY_IMAGE_EDITOR (editor);

  if (state & clay_get_toggle_behavior_mask ())
    clay_context_set_background (image_editor->context, entry->color);
  else
    clay_context_set_foreground (image_editor->context, entry->color);

}

static void
clay_colormap_editor_notify_index (ClayColormapSelection *selection,
                                   const GParamSpec      *pspec,
                                   ClayColormapEditor    *editor)
{
  g_return_if_fail (CLAY_IS_COLORMAP_EDITOR (editor));

  clay_editor_set_action_sensitive (CLAY_EDITOR (editor), "colormap",
                                    "colormap-delete-color",
                                    clay_colormap_editor_is_color_deletable (editor),
                                    _("The color is used in this indexed image"));
}
