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

#include "libclayconfig/clayconfig.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/clayitem.h"
#include "core/clayitemundo.h"

#include "dialogs/dialogs.h"
#include "dialogs/fill-dialog.h"
#include "dialogs/stroke-dialog.h"

#include "actions.h"
#include "items-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   items_fill_callback   (GtkWidget         *dialog,
                                     GList             *items,
                                     GList             *drawables,
                                     ClayContext       *context,
                                     ClayFillOptions   *options,
                                     gpointer           user_data);
static void   items_stroke_callback (GtkWidget         *dialog,
                                     GList             *items,
                                     GList             *drawables,
                                     ClayContext       *context,
                                     ClayStrokeOptions *options,
                                     gpointer           user_data);


/*  public functions  */

void
items_visible_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            ClayImage  *image,
                            GList      *items)
{
  ClayUndo *undo;
  gboolean  push_undo = TRUE;
  GList    *start     = NULL;
  GList    *iter;
  gboolean  visible   = g_variant_get_boolean (value);
  gint      n_items   = 0;

  for (iter = items; iter; iter = iter->next)
    {
      if (visible && clay_item_get_visible (iter->data))
        {
          /* If any of the items are already visible, we don't
           * toggle the selection visibility.
           */
          return;
        }
    }

  for (iter = items; iter; iter = iter->next)
    if (visible != clay_item_get_visible (iter->data))
      {
        if (start == NULL)
          start = iter;
        n_items++;
      }

  if (n_items == 0)
    {
      return;
    }
  else if (n_items == 1)
    {
      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_ITEM_VISIBILITY);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (start->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_VISIBILITY,
                                   "Item visibility");
    }

  for (iter = start; iter; iter = iter->next)
    if (visible != clay_item_get_visible (iter->data))
      clay_item_set_visible (iter->data, visible, push_undo);

  if (n_items != 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
items_lock_content_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 ClayImage  *image,
                                 GList      *items)
{
  GList    *locked_items = NULL;
  GList    *iter;
  gchar    *undo_label;
  gboolean  locked       = g_variant_get_boolean (value);

  for (iter = items; iter; iter = iter->next)
    if (clay_item_can_lock_content (iter->data))
      {
        if (! locked && ! clay_item_get_lock_content (iter->data))
          {
            /* When unlocking, we expect all selected items to be locked. */
            g_list_free (locked_items);
            return;
          }
        else if (locked != clay_item_get_lock_content (iter->data))
          {
            locked_items = g_list_prepend (locked_items, iter->data);
          }
      }

  if (! locked_items)
    return;

  if (locked)
    undo_label = _("Lock content");
  else
    undo_label = _("Unlock content");

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_LOCK_CONTENTS,
                               undo_label);

  for (iter = locked_items; iter; iter = iter->next)
    clay_item_set_lock_content (iter->data, locked, TRUE);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (locked_items);
}

void
items_lock_position_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  ClayImage  *image,
                                  GList      *items)
{
  GList    *iter;
  GList    *locked_items = NULL;
  gchar    *undo_label;
  gboolean  locked       = g_variant_get_boolean (value);

  for (iter = items; iter; iter = iter->next)
    if (clay_item_can_lock_position (iter->data))
      {
        if (! locked && ! clay_item_get_lock_position (iter->data))
          {
           /* When unlocking, we expect all selected items to be locked. */
            g_list_free (locked_items);
            return;
          }
        else if (locked != clay_item_get_lock_position (iter->data))
          {
            locked_items = g_list_prepend (locked_items, iter->data);
          }
      }

  if (! locked_items)
    return;

  if (locked)
    undo_label = _("Lock position");
  else
    undo_label = _("Unlock position");

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_LOCK_POSITION,
                               undo_label);

  for (iter = locked_items; iter; iter = iter->next)
    clay_item_set_lock_position (iter->data, locked, TRUE);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (locked_items);
}

void
items_color_tag_cmd_callback (ClayAction   *action,
                              ClayImage    *image,
                              GList        *items,
                              ClayColorTag  color_tag)
{
  ClayUndo *undo;
  gboolean  push_undo = TRUE;
  GList    *iter;

  if (g_list_length (items) == 1)
    {
      undo = clay_image_undo_can_compress (image, CLAY_TYPE_ITEM_UNDO,
                                           CLAY_UNDO_ITEM_COLOR_TAG);

      if (undo && CLAY_ITEM_UNDO (undo)->item == CLAY_ITEM (items->data))
        push_undo = FALSE;
    }
  else
    {
      /* TODO: undo groups cannot be compressed so far. */
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                                   "Item color tag");
    }

  for (iter = items; iter; iter = iter->next)
    if (color_tag != clay_item_get_color_tag (iter->data))
      clay_item_set_color_tag (iter->data, color_tag, push_undo);

  if (g_list_length (items) != 1)
    clay_image_undo_group_end (image);

  clay_image_flush (image);
}

void
items_fill_cmd_callback (ClayAction  *action,
                         ClayImage   *image,
                         GList       *items,
                         const gchar *dialog_title,
                         const gchar *dialog_icon_name,
                         const gchar *dialog_help_id,
                         gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = clay_image_get_selected_drawables (image);

  if (! drawables)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("There are no selected items to fill."));
      return;
    }

  dialog = fill_dialog_new (items,
                            drawables,
                            action_data_get_context (data),
                            dialog_title,
                            dialog_icon_name,
                            dialog_help_id,
                            widget,
                            CLAY_DIALOG_CONFIG (image->clay->config)->fill_options,
                            items_fill_callback,
                            NULL);

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_fill_last_vals_cmd_callback (ClayAction *action,
                                   ClayImage  *image,
                                   GList      *items,
                                   gpointer    data)
{
  GList            *drawables;
  ClayDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = clay_image_get_selected_drawables (image);

  if (! drawables)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("There are no selected layers or channels to fill."));
      return;
    }

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_DRAWABLE_MOD,
                               "Fill");

  for (GList *iter = items; iter; iter = iter->next)
    if (! clay_item_fill (iter->data, drawables,
                          config->fill_options, TRUE, NULL, &error))
      {
        clay_message_literal (image->clay, G_OBJECT (widget),
                              CLAY_MESSAGE_WARNING, error->message);
        g_clear_error (&error);
        break;
      }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
  g_list_free (drawables);
}

void
items_stroke_cmd_callback (ClayAction  *action,
                           ClayImage   *image,
                           GList       *items,
                           const gchar *dialog_title,
                           const gchar *dialog_icon_name,
                           const gchar *dialog_help_id,
                           gpointer     data)
{
  GList        *drawables;
  GtkWidget    *dialog;
  GtkWidget    *widget;
  return_if_no_widget (widget, data);

  drawables = clay_image_get_selected_drawables (image);

  if (! drawables)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }


  dialog = stroke_dialog_new (items,
                              drawables,
                              action_data_get_context (data),
                              dialog_title,
                              dialog_icon_name,
                              dialog_help_id,
                              widget,
                              CLAY_DIALOG_CONFIG (image->clay->config)->stroke_options,
                              items_stroke_callback,
                              NULL);

  gtk_window_present (GTK_WINDOW (dialog));
  g_list_free (drawables);
}

void
items_stroke_last_vals_cmd_callback (ClayAction *action,
                                     ClayImage  *image,
                                     GList      *items,
                                     gpointer    data)
{
  GList            *drawables;
  ClayDialogConfig *config;
  GtkWidget        *widget;
  GError           *error = NULL;
  return_if_no_widget (widget, data);

  drawables = clay_image_get_selected_drawables (image);

  if (! drawables)
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("There are no selected layers or channels to stroke to."));
      return;
    }

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_DRAWABLE_MOD,
                               "Stroke");

  for (GList *iter = items; iter; iter = iter->next)
    if (! clay_item_stroke (iter->data, drawables,
                            action_data_get_context (data),
                            config->stroke_options, NULL,
                            TRUE, NULL, &error))
      {
        clay_message_literal (image->clay, G_OBJECT (widget),
                              CLAY_MESSAGE_WARNING, error->message);
        g_clear_error (&error);
        break;
      }

  clay_image_undo_group_end (image);
  clay_image_flush (image);
  g_list_free (drawables);
}


/*  private functions  */

static void
items_fill_callback (GtkWidget       *dialog,
                     GList           *items,
                     GList           *drawables,
                     ClayContext     *context,
                     ClayFillOptions *options,
                     gpointer         user_data)
{
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (context->clay->config);
  ClayImage        *image  = clay_item_get_image (items->data);
  GError           *error  = NULL;

  clay_config_sync (G_OBJECT (options),
                    G_OBJECT (config->fill_options), 0);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_DRAWABLE_MOD,
                               "Fill");

  for (GList *iter = items; iter; iter = iter->next)
    if (! clay_item_fill (iter->data, drawables, options, TRUE, NULL, &error))
      {
        clay_message_literal (context->clay,
                              G_OBJECT (dialog),
                              CLAY_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  clay_image_undo_group_end (image);
  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
items_stroke_callback (GtkWidget         *dialog,
                       GList             *items,
                       GList             *drawables,
                       ClayContext       *context,
                       ClayStrokeOptions *options,
                       gpointer           data)
{
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (context->clay->config);
  ClayImage        *image  = clay_item_get_image (items->data);
  GError           *error  = NULL;

  clay_config_sync (G_OBJECT (options),
                    G_OBJECT (config->stroke_options), 0);

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_DRAWABLE_MOD,
                               "Stroke");

  for (GList *iter = items; iter; iter = iter->next)
    if (! clay_item_stroke (iter->data, drawables, context, options, NULL,
                            TRUE, NULL, &error))
      {
        clay_message_literal (context->clay,
                              G_OBJECT (dialog),
                              CLAY_MESSAGE_WARNING,
                              error ? error->message : "NULL");

        g_clear_error (&error);
        break;
      }

  clay_image_undo_group_end (image);
  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}
