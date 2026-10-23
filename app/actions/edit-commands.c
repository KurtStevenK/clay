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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/clay.h"
#include "core/clay-edit.h"
#include "core/claybuffer.h"
#include "core/claycontainer.h"
#include "core/claydrawable.h"
#include "core/claydrawable-edit.h"
#include "core/clayfilloptions.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"
#include "core/claylayermask.h"
#include "core/claylinklayer.h"
#include "core/claylist.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayrasterizable.h"

#include "path/claypath-import.h"
#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "widgets/clayclipboard.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claymessagebox.h"
#include "widgets/claymessagedialog.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"
#include "display/claydisplayshell-transform.h"

#include "tools/claytools-utils.h"
#include "tools/tool_manager.h"

#include "actions.h"
#include "edit-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static gboolean   check_drawable_alpha               (ClayDrawable  *drawable,
                                                      gpointer       data);
static void       edit_paste                         (ClayDisplay   *display,
                                                      ClayPasteType  paste_type,
                                                      gboolean       merged,
                                                      gboolean       try_svg);
static void       cut_named_buffer_callback          (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);
static void       copy_named_buffer_callback         (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);
static void       copy_named_visible_buffer_callback (GtkWidget     *widget,
                                                      const gchar   *name,
                                                      gpointer       data);


/*  public functions  */

void
edit_undo_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_undo_active (image->clay, display) ||
      clay_image_undo (image))
    {
      clay_image_flush (image);
    }
}

void
edit_redo_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  if (tool_manager_redo_active (image->clay, display) ||
      clay_image_redo (image))
    {
      clay_image_flush (image);
    }
}

void
edit_strong_undo_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  if (clay_image_strong_undo (image))
    clay_image_flush (image);
}

void
edit_strong_redo_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  if (clay_image_strong_redo (image))
    clay_image_flush (image);
}

void
edit_undo_clear_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage     *image;
  ClayUndoStack *undo_stack;
  ClayUndoStack *redo_stack;
  GtkWidget     *widget;
  GtkWidget     *dialog;
  gchar         *size;
  gint64         memsize;
  gint64         guisize;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = clay_message_dialog_new (_("Clear Undo History"),
                                    CLAY_ICON_DIALOG_WARNING,
                                    widget,
                                    GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    clay_standard_help_func,
                                    CLAY_HELP_EDIT_UNDO_CLEAR,

                                    _("_Cancel"), GTK_RESPONSE_CANCEL,
                                    _("Cl_ear"),  GTK_RESPONSE_OK,

                                    NULL);

  clay_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect_object (gtk_widget_get_toplevel (widget), "unmap",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  g_signal_connect_object (image, "disconnect",
                           G_CALLBACK (gtk_widget_destroy),
                           dialog, G_CONNECT_SWAPPED);

  clay_message_box_set_primary_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                                     _("Really clear image's undo history?"));

  undo_stack = clay_image_get_undo_stack (image);
  redo_stack = clay_image_get_redo_stack (image);

  memsize =  clay_object_get_memsize (CLAY_OBJECT (undo_stack), &guisize);
  memsize += guisize;
  memsize += clay_object_get_memsize (CLAY_OBJECT (redo_stack), &guisize);
  memsize += guisize;

  size = g_format_size (memsize);

  clay_message_box_set_text (CLAY_MESSAGE_DIALOG (dialog)->box,
                             _("Clearing the undo history of this "
                               "image will gain %s of memory."), size);
  g_free (size);

  if (clay_dialog_run (CLAY_DIALOG (dialog)) == GTK_RESPONSE_OK)
    {
      clay_image_undo_disable (image);
      clay_image_undo_enable (image);
      clay_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}

void
edit_cut_cmd_callback (ClayAction *action,
                       GVariant   *value,
                       gpointer    data)
{
  ClayImage    *image;
  GList        *drawables;
  GList        *iter;
  ClayObject   *cut;
  GError       *error = NULL;
  return_if_no_drawables (image, drawables, data);

  for (iter = drawables; iter; iter = iter->next)
    if (! check_drawable_alpha (iter->data, data))
      {
        g_list_free (drawables);
        return;
      }

  cut = clay_edit_cut (image, drawables, action_data_get_context (data), &error);

  if (cut)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        {
          gchar *msg;

          if (CLAY_IS_IMAGE (cut))
            msg = g_strdup_printf (ngettext ("Cut layer to the clipboard.",
                                             "Cut %d layers to the clipboard.",
                                             g_list_length (drawables)),
                                   g_list_length (drawables));
          else
            msg = g_strdup (_("Cut pixels to the clipboard."));

          clay_message_literal (image->clay,
                                G_OBJECT (display), CLAY_MESSAGE_INFO,
                                msg);
          g_free (msg);
        }

      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay,
                            G_OBJECT (action_data_get_display (data)),
                            CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

void
edit_copy_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayImage    *image;
  GList        *drawables;
  ClayObject   *copy;
  GError       *error = NULL;
  return_if_no_drawables (image, drawables, data);

  copy = clay_edit_copy (image, drawables, action_data_get_context (data),
                         FALSE, &error);

  if (copy)
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        clay_message_literal (image->clay,
                              G_OBJECT (display), CLAY_MESSAGE_INFO,
                              CLAY_IS_IMAGE (copy) ?
                              _("Copied layer to the clipboard.") :
                              _("Copied pixels to the clipboard."));

      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay,
                            G_OBJECT (action_data_get_display (data)),
                            CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }

  g_list_free (drawables);
}

void
edit_copy_visible_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage *image;
  GError    *error = NULL;
  return_if_no_image (image, data);

  if (clay_edit_copy_visible (image, action_data_get_context (data), &error))
    {
      ClayDisplay *display = action_data_get_display (data);

      if (display)
        clay_message_literal (image->clay,
                              G_OBJECT (display), CLAY_MESSAGE_INFO,
                              _("Copied pixels to the clipboard."));

      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay,
                            G_OBJECT (action_data_get_display (data)),
                            CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}

void
edit_paste_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage     *image;
  ClayDisplay   *display    = action_data_get_display (data);
  ClayPasteType  paste_type = (ClayPasteType) g_variant_get_int32 (value);
  ClayPasteType  converted_type;
  GList         *drawables;
  gboolean       merged     = FALSE;

  if (paste_type == CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING)
    {
      if (! display || ! clay_display_get_image (display))
        {
          edit_paste_as_new_image_cmd_callback (action, value, data);
          return;
        }
    }

  return_if_no_image (image, data);

  if (! display)
    return;

  switch (paste_type)
    {
    case CLAY_PASTE_TYPE_FLOATING:
    case CLAY_PASTE_TYPE_FLOATING_IN_PLACE:
    case CLAY_PASTE_TYPE_FLOATING_INTO:
    case CLAY_PASTE_TYPE_FLOATING_INTO_IN_PLACE:
      edit_paste (display, paste_type, merged, TRUE);
      break;

    case CLAY_PASTE_TYPE_NEW_LAYER:
    case CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE:
      edit_paste (display, paste_type, merged, FALSE);
      break;

    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING:
    case CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE:
      merged = TRUE;
    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING:
    case CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE:
      drawables = clay_image_get_selected_drawables (image);

      if (drawables &&
         (g_list_length (drawables) == 1) &&
          CLAY_IS_LAYER_MASK (drawables->data))
        {
          converted_type = (paste_type == CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING ||
                            paste_type == CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING) ?
                            CLAY_PASTE_TYPE_FLOATING :
                            CLAY_PASTE_TYPE_FLOATING_IN_PLACE;

          edit_paste (display, converted_type, merged, TRUE);
        }
      else
        {
          converted_type = (paste_type == CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING ||
                            paste_type == CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING) ?
                            CLAY_PASTE_TYPE_NEW_LAYER :
                            CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE;

          edit_paste (display, converted_type, merged, TRUE);
        }
      g_list_free (drawables);

      break;
    }
}

void
edit_paste_as_new_image_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  Clay       *clay;
  GtkWidget  *widget;
  ClayObject *paste;
  ClayImage  *image = NULL;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  paste = clay_clipboard_get_object (clay);

  if (paste)
    {
      image = clay_edit_paste_as_new_image (clay, paste,
                                            action_data_get_context (data));
      g_object_unref (paste);
    }

  if (image)
    {
      clay_create_display (clay, image, clay_unit_pixel (), 1.0,
                           G_OBJECT (clay_widget_get_monitor (widget)));
      g_object_unref (image);
    }
  else
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_WARNING,
                            _("There is no image data in the clipboard "
                              "to paste."));
    }
}

void
edit_named_cut_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = clay_query_string_box (_("Cut Named"), widget,
                                  clay_standard_help_func,
                                  CLAY_HELP_BUFFER_CUT,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  cut_named_buffer_callback,
                                  image, NULL);
  gtk_widget_set_visible (dialog, TRUE);
}

void
edit_named_copy_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = clay_query_string_box (_("Copy Named"), widget,
                                  clay_standard_help_func,
                                  CLAY_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_buffer_callback,
                                  image, NULL);
  gtk_widget_set_visible (dialog, TRUE);
}

void
edit_named_copy_visible_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  dialog = clay_query_string_box (_("Copy Visible Named"), widget,
                                  clay_standard_help_func,
                                  CLAY_HELP_BUFFER_COPY,
                                  _("Enter a name for this buffer"),
                                  NULL,
                                  G_OBJECT (image), "disconnect",
                                  copy_named_visible_buffer_callback,
                                  image, NULL);
  gtk_widget_set_visible (dialog, TRUE);
}

void
edit_named_paste_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  Clay      *clay;
  GtkWidget *widget;
  return_if_no_clay (clay, data);
  return_if_no_widget (widget, data);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (clay)),
                                             clay,
                                             clay_dialog_factory_get_singleton (),
                                             clay_widget_get_monitor (widget),
                                             "clay-buffer-list|clay-buffer-grid");
}

void
edit_clear_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage *image;
  GList     *drawables;
  GList     *iter;

  return_if_no_drawables (image, drawables, data);

  for (iter = drawables; iter; iter = iter->next)
    /* Return if any has a locked alpha. */
    if (! check_drawable_alpha (iter->data, data))
      {
        g_list_free (drawables);
        return;
      }

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT,
                               _("Clear"));

  for (iter = drawables; iter; iter = iter->next)
    if (! clay_viewable_get_children (CLAY_VIEWABLE (iter->data))    &&
        ! clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL) &&
        ! (CLAY_IS_RASTERIZABLE (iter->data) &&
          ! clay_rasterizable_is_rasterized (iter->data)))
      clay_drawable_edit_clear (iter->data, action_data_get_context (data));

  clay_image_undo_group_end (image);
  clay_image_flush (image);
  g_list_free (drawables);
}

void
edit_fill_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  ClayImage       *image;
  GList           *drawables;
  GList           *iter;
  ClayFillType     fill_type;
  ClayFillOptions *options;
  GError          *error = NULL;
  return_if_no_drawables (image, drawables, data);

  fill_type = (ClayFillType) g_variant_get_int32 (value);

  options = clay_fill_options_new (action_data_get_clay (data), NULL, FALSE);

  if (clay_fill_options_set_by_fill_type (options,
                                          action_data_get_context (data),
                                          fill_type, &error))
    {
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT,
                                   clay_fill_options_get_undo_desc (options));

      for (iter = drawables; iter; iter = iter->next)
        {
          /* Skip layers that can't be filled, like pixel-locked layers */
          if (clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL) ||
              clay_item_is_link_layer (CLAY_ITEM (iter->data)))
            continue;

          if (clay_item_is_text_layer (iter->data) ||
              clay_item_is_vector_layer (iter->data))
            {
              ClayPattern *pattern = NULL;
              GeglColor   *color   = NULL;

              if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_FG_COLOR)
                color = clay_context_get_foreground (CLAY_CONTEXT (options));
              else if (clay_fill_options_get_style (options) == CLAY_FILL_STYLE_BG_COLOR)
                color = clay_context_get_background (CLAY_CONTEXT (options));
              else
                pattern = clay_context_get_pattern (CLAY_CONTEXT (options));

              if (color)
                {
                  if (clay_item_is_vector_layer (iter->data))
                    {
                      clay_vector_layer_set (CLAY_VECTOR_LAYER (iter->data),
                                             NULL,
                                             "enable-fill",
                                             TRUE,
                                             "fill-style",
                                             CLAY_CUSTOM_STYLE_SOLID_COLOR,
                                             "fill-color",
                                             color, NULL);
                      clay_vector_layer_refresh (iter->data);
                    }
                  else
                    {
                      clay_text_layer_set (iter->data, NULL, "color", color,
                                           NULL);
                    }
                }
              else if (pattern &&
                       clay_item_is_vector_layer (iter->data))
                {
                  clay_vector_layer_set (CLAY_VECTOR_LAYER (iter->data), NULL,
                                         "enable-fill",
                                         TRUE,
                                         "fill-style",
                                         CLAY_CUSTOM_STYLE_PATTERN,
                                         "fill-pattern",
                                         pattern,
                                         NULL);
                  clay_vector_layer_refresh (iter->data);
                }
            }
          else
            {
              clay_drawable_edit_fill (iter->data, options, NULL);
            }
        }

      clay_image_undo_group_end (image);
      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }

  g_list_free (drawables);
  g_object_unref (options);
}


/*  private functions  */

static gboolean
check_drawable_alpha (ClayDrawable *drawable,
                      gpointer      data)
{
  ClayLayer *locked_layer = NULL;

  if (clay_drawable_has_alpha (drawable) &&
      CLAY_IS_LAYER (drawable)           &&
      clay_layer_is_alpha_locked (CLAY_LAYER (drawable), &locked_layer))
    {
      Clay        *clay    = action_data_get_clay    (data);
      ClayDisplay *display = action_data_get_display (data);

      if (clay && display)
        {
          clay_message_literal (
            clay, G_OBJECT (display), CLAY_MESSAGE_WARNING,
            _("A selected layer's alpha channel is locked."));

          clay_tools_blink_lock_box (clay, CLAY_ITEM (locked_layer));
        }

      return FALSE;
    }

  return TRUE;
}

static void
edit_paste (ClayDisplay   *display,
            ClayPasteType  paste_type,
            gboolean       merged,
            gboolean       try_svg)
{
  ClayImage  *image = clay_display_get_image (display);
  ClayObject *paste;

  g_return_if_fail (paste_type != CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING          &&
                    paste_type != CLAY_PASTE_TYPE_NEW_LAYER_OR_FLOATING_IN_PLACE &&
                    paste_type != CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING   &&
                    paste_type != CLAY_PASTE_TYPE_NEW_MERGED_LAYER_OR_FLOATING_IN_PLACE);

  if (try_svg)
    {
      gchar *svg;
      gsize  svg_size;

      svg = clay_clipboard_get_svg (display->clay, &svg_size);

      if (svg)
        {
          if (clay_path_import_buffer (image, svg, svg_size,
                                       TRUE, FALSE,
                                       CLAY_IMAGE_ACTIVE_PARENT, -1,
                                       NULL, NULL))
            {
              clay_image_flush (image);
            }

          g_free (svg);

          return;
        }
    }

  paste = clay_clipboard_get_object (display->clay);

  if (paste)
    {
      ClayDisplayShell *shell     = clay_display_get_shell (display);
      GList            *drawables = clay_image_get_selected_drawables (image);
      GList            *pasted_layers;
      gint              x, y;
      gint              width, height;

      if (g_list_length (drawables) != 1 ||
          (paste_type != CLAY_PASTE_TYPE_NEW_LAYER &&
           paste_type != CLAY_PASTE_TYPE_NEW_LAYER_IN_PLACE))
        {
          if (g_list_length (drawables) != 1)
            {
              clay_message_literal (display->clay, G_OBJECT (display),
                                    CLAY_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target is not a single layer or layer mask."));
            }
          else if (clay_viewable_get_children (CLAY_VIEWABLE (drawables->data)))
            {
              clay_message_literal (display->clay, G_OBJECT (display),
                                    CLAY_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target is a layer group."));
            }
          else if (clay_item_is_content_locked (CLAY_ITEM (drawables->data), NULL))
            {
              clay_message_literal (display->clay, G_OBJECT (display),
                                    CLAY_MESSAGE_INFO,
                                    _("Pasted as new layer because the "
                                      "target's pixels are locked."));
            }

          /* the actual paste-type conversion happens in clay_edit_paste() */
        }

      clay_display_shell_untransform_viewport (
        shell,
        ! clay_display_shell_get_infinite_canvas (shell),
        &x, &y, &width, &height);

      if ((pasted_layers = clay_edit_paste (image, drawables, paste, paste_type,
                                            clay_get_user_context (display->clay),
                                            merged, x, y, width, height)))
        {
          clay_image_set_selected_layers (image, pasted_layers);

          g_list_free (pasted_layers);
          clay_image_flush (image);
        }

      g_list_free (drawables);
      g_object_unref (paste);
    }
  else
    {
      clay_message_literal (display->clay, G_OBJECT (display),
                            CLAY_MESSAGE_WARNING,
                            _("There is no image data in the clipboard "
                              "to paste."));
    }
}

static void
cut_named_buffer_callback (GtkWidget   *widget,
                           const gchar *name,
                           gpointer     data)
{
  ClayImage *image     = CLAY_IMAGE (data);
  GList     *drawables = clay_image_get_selected_drawables (image);
  GError    *error     = NULL;

  if (! drawables)
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            _("There are no selected layers or channels to cut from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (clay_edit_named_cut (image, name, drawables,
                           clay_get_user_context (image->clay), &error))
    {
      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

static void
copy_named_buffer_callback (GtkWidget   *widget,
                            const gchar *name,
                            gpointer     data)
{
  ClayImage *image     = CLAY_IMAGE (data);
  GList     *drawables = clay_image_get_selected_drawables (image);
  GError    *error     = NULL;

  if (! drawables)
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            _("There are no selected layers or channels to copy from."));
      return;
    }

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (clay_edit_named_copy (image, name, drawables,
                            clay_get_user_context (image->clay), &error))
    {
      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}

static void
copy_named_visible_buffer_callback (GtkWidget   *widget,
                                    const gchar *name,
                                    gpointer     data)
{
  ClayImage *image = CLAY_IMAGE (data);
  GError    *error = NULL;

  if (! (name && strlen (name)))
    name = _("(Unnamed Buffer)");

  if (clay_edit_named_copy_visible (image, name,
                                    clay_get_user_context (image->clay),
                                    &error))
    {
      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
}
