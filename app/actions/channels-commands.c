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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claychannel-select.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydrawable-fill.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"

#include "widgets/clayaction.h"
#include "widgets/claycolorpanel.h"
#include "widgets/claycomponenteditor.h"
#include "widgets/claydock.h"
#include "widgets/clayhelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"

#include "actions.h"
#include "channels-commands.h"
#include "items-commands.h"

#include "clay-intl.h"


#define RGBA_EPSILON 1e-6


/*  local function prototypes  */

static void   channels_new_callback             (GtkWidget     *dialog,
                                                 ClayImage     *image,
                                                 ClayChannel   *channel,
                                                 ClayContext   *context,
                                                 const gchar   *channel_name,
                                                 GeglColor     *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 ClayColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gboolean       channel_lock_visibility,
                                                 gpointer       user_data);
static void   channels_edit_attributes_callback (GtkWidget     *dialog,
                                                 ClayImage     *image,
                                                 ClayChannel   *channel,
                                                 ClayContext   *context,
                                                 const gchar   *channel_name,
                                                 GeglColor     *channel_color,
                                                 gboolean       save_selection,
                                                 gboolean       channel_visible,
                                                 ClayColorTag   channel_color_tag,
                                                 gboolean       channel_lock_content,
                                                 gboolean       channel_lock_position,
                                                 gboolean       channel_lock_visibility,
                                                 gpointer       user_data);


/*  public functions  */

void
channels_edit_attributes_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage   *image;
  ClayChannel *channel;
  GList       *channels;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_channels (image, channels, data);
  return_if_no_widget (widget, data);

#define EDIT_DIALOG_KEY "clay-channel-edit-attributes-dialog"

  if (g_list_length (channels) != 1)
    return;

  channel = channels->data;
  dialog  = dialogs_get_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY);

  if (! dialog)
    {
      ClayItem *item = CLAY_ITEM (channel);

      dialog = channel_options_dialog_new (image, channel,
                                           action_data_get_context (data),
                                           widget,
                                           _("Channel Attributes"),
                                           "clay-channel-edit",
                                           CLAY_ICON_EDIT,
                                           _("Edit Channel Attributes"),
                                           CLAY_HELP_CHANNEL_EDIT,
                                           _("Edit Channel Color"),
                                           _("_Fill opacity:"),
                                           FALSE,
                                           clay_object_get_name (channel),
                                           channel->color,
                                           clay_item_get_visible (item),
                                           clay_item_get_color_tag (item),
                                           clay_item_get_lock_content (item),
                                           clay_item_get_lock_position (item),
                                           clay_item_get_lock_visibility (item),
                                           channels_edit_attributes_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (channel), EDIT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define NEW_DIALOG_KEY "clay-channel-new-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), NEW_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("New Channel"),
                                           "clay-channel-new",
                                           CLAY_ICON_CHANNEL,
                                           _("Create a New Channel"),
                                           CLAY_HELP_CHANNEL_NEW,
                                           _("New Channel Color"),
                                           _("_Fill opacity:"),
                                           TRUE,
                                           config->channel_new_name,
                                           config->channel_new_color,
                                           TRUE,
                                           CLAY_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           FALSE,
                                           channels_new_callback,
                                           NULL);

      dialogs_attach_dialog (G_OBJECT (image), NEW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
channels_new_last_vals_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage        *image;
  ClayChannel      *channel;
  ClayDialogConfig *config;
  return_if_no_image (image, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  channel = clay_channel_new (image,
                              clay_image_get_width (image),
                              clay_image_get_height (image),
                              config->channel_new_name,
                              config->channel_new_color);

  clay_drawable_fill (CLAY_DRAWABLE (channel),
                      action_data_get_context (data),
                      CLAY_FILL_TRANSPARENT);

  clay_image_add_channel (image, channel,
                          CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);
}

void
channels_raise_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        {
          raised_channels = g_list_prepend (raised_channels, iter->data);
        }
      else
        {
          clay_image_flush (image);
          g_list_free (raised_channels);
          return;
        }
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel",
                                         "Raise Channels",
                                         g_list_length (raised_channels)));
  raised_channels = g_list_reverse (raised_channels);
  for (iter = raised_channels; iter; iter = iter->next)
    clay_image_raise_item (image, iter->data, NULL);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_raise_to_top_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  GList     *iter;
  GList     *raised_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      gint index;

      index = clay_item_get_index (iter->data);
      if (index > 0)
        raised_channels = g_list_prepend (raised_channels, iter->data);
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Raise Channel to Top",
                                         "Raise Channels to Top",
                                         g_list_length (raised_channels)));

  for (iter = raised_channels; iter; iter = iter->next)
    clay_image_raise_item_to_top (image, iter->data);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (raised_channels);
}

void
channels_lower_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        {
          lowered_channels = g_list_prepend (lowered_channels, iter->data);
        }
      else
        {
          clay_image_flush (image);
          g_list_free (lowered_channels);
          return;
        }
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel",
                                         "Lower Channels",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    clay_image_lower_item (image, iter->data, NULL);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_lower_to_bottom_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  GList     *iter;
  GList     *lowered_channels = NULL;
  return_if_no_channels (image, channels, data);

  for (iter = channels; iter; iter = iter->next)
    {
      GList *layer_list;
      gint   index;

      layer_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
      index = clay_item_get_index (iter->data);
      if (index < g_list_length (layer_list) - 1)
        lowered_channels = g_list_prepend (lowered_channels, iter->data);
    }

  clay_image_undo_group_start (image,
                               CLAY_UNDO_GROUP_ITEM_DISPLACE,
                               ngettext ("Lower Channel to Bottom",
                                         "Lower Channels to Bottom",
                                         g_list_length (lowered_channels)));

  for (iter = lowered_channels; iter; iter = iter->next)
    clay_image_lower_item_to_bottom (image, iter->data);

  clay_image_flush (image);
  clay_image_undo_group_end (image);

  g_list_free (lowered_channels);
}

void
channels_duplicate_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage   *image  = NULL;
  GList       *channels;
  ClayChannel *parent = CLAY_IMAGE_ACTIVE_PARENT;

  if (CLAY_IS_COMPONENT_EDITOR (data))
    {
      ClayChannelType  component;
      ClayChannel     *new_channel;
      const gchar     *desc;
      gchar           *name;

      return_if_no_image (image, data);

      component = CLAY_COMPONENT_EDITOR (data)->clicked_component;

      clay_enum_get_value (CLAY_TYPE_CHANNEL_TYPE, component,
                           NULL, NULL, &desc, NULL);

      name = g_strdup_printf (_("%s Channel Copy"), desc);

      new_channel = clay_channel_new_from_component (image, component,
                                                     name, NULL);

      /*  copied components are invisible by default so subsequent copies
       *  of components don't affect each other
       */
      clay_item_set_visible (CLAY_ITEM (new_channel), FALSE, FALSE);
      clay_image_add_channel (image, new_channel, parent, -1, TRUE);

      g_free (name);
    }
  else
    {
      GList *new_channels = NULL;
      GList *iter;

      return_if_no_channels (image, channels, data);

      channels = g_list_copy (channels);
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_CHANNEL_ADD,
                                   _("Duplicate channels"));
      for (iter = channels; iter; iter = iter->next)
        {
          ClayChannel *new_channel;

          new_channel = CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (iter->data),
                                                           G_TYPE_FROM_INSTANCE (iter->data)));

          /*  use the actual parent here, not CLAY_IMAGE_ACTIVE_PARENT because
           *  the latter would add a duplicated group inside itself instead of
           *  above it
           */
          clay_image_add_channel (image, new_channel,
                                  clay_channel_get_parent (iter->data),
                                  clay_item_get_index (iter->data),
                                  TRUE);
          new_channels = g_list_prepend (new_channels, new_channel);
        }

      clay_image_set_selected_channels (image, new_channels);
      g_list_free (channels);
      g_list_free (new_channels);

      clay_image_undo_group_end (image);
    }
  clay_image_flush (image);
}

void
channels_delete_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  GList     *iter;
  return_if_no_channels (image, channels, data);

  channels = g_list_copy (channels);
  if (g_list_length (channels) > 1)
    {
      gchar *undo_name;

      undo_name = g_strdup_printf (C_("undo-type", "Remove %d Channels"),
                                   g_list_length (channels));
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_ITEM_REMOVE,
                                   undo_name);
    }

  for (iter = channels; iter; iter = iter->next)
    clay_image_remove_channel (image, iter->data, TRUE, NULL);

  if (g_list_length (channels) > 1)
    clay_image_undo_group_end (image);

  g_list_free (channels);
  clay_image_flush (image);
}

void
channels_to_selection_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayChannelOps  op;
  ClayImage      *image;

  op = (ClayChannelOps) g_variant_get_int32 (value);

  if (CLAY_IS_COMPONENT_EDITOR (data))
    {
      ClayChannelType component;
      return_if_no_image (image, data);

      component = CLAY_COMPONENT_EDITOR (data)->clicked_component;

      clay_channel_select_component (clay_image_get_mask (image), component,
                                     op, FALSE, 0.0, 0.0);
    }
  else
    {
      GList *channels;
      GList *iter;
      return_if_no_channels (image, channels, data);

      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                   _("Channels to selection"));

      for (iter = channels; iter; iter = iter->next)
        {
          clay_item_to_selection (iter->data, op, TRUE, FALSE, 0.0, 0.0);

          if (op == CLAY_CHANNEL_OP_REPLACE && iter == channels)
            op = CLAY_CHANNEL_OP_ADD;
        }

      clay_image_undo_group_end (image);
    }

  clay_image_flush (image);
}

void
channels_visible_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_visible_cmd_callback (action, value, image, channels);
}

void
channels_lock_content_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_lock_content_cmd_callback (action, value, image, channels);
}

void
channels_lock_position_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage *image;
  GList     *channels;
  return_if_no_channels (image, channels, data);

  items_lock_position_cmd_callback (action, value, image, channels);
}

void
channels_color_tag_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  ClayImage    *image;
  GList        *channels;
  ClayColorTag  color_tag;
  return_if_no_channels (image, channels, data);

  color_tag = (ClayColorTag) g_variant_get_int32 (value);

  items_color_tag_cmd_callback (action, image, channels, color_tag);
}

void
channels_select_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayImage            *image;
  GList                *channels;
  GList                *new_channels = NULL;
  GList                *iter;
  ClayActionSelectType  select_type;
  gboolean              run_once;
  return_if_no_image (image, data);

  select_type = (ClayActionSelectType) g_variant_get_int32 (value);

  channels = clay_image_get_selected_channels (image);
  run_once = (g_list_length (channels) == 0);

  for (iter = channels; iter || run_once; iter = iter ? iter->next : NULL)
    {
      ClayChannel   *new_channel = NULL;
      ClayContainer *container;

      if (iter)
        {
          container = clay_item_get_container (CLAY_ITEM (iter->data));
        }
      else /* run_once */
        {
          container = clay_image_get_channels (image);
          run_once  = FALSE;
        }
      new_channel = (ClayChannel *) action_select_object (select_type,
                                                          container,
                                                          iter ? iter->data : NULL);

      if (new_channel)
        new_channels = g_list_prepend (new_channels, new_channel);
    }

  if (new_channels)
    {
      clay_image_set_selected_channels (image, new_channels);
      clay_image_flush (image);
    }

  g_list_free (new_channels);
}

/*  private functions  */

static void
channels_new_callback (GtkWidget     *dialog,
                       ClayImage     *image,
                       ClayChannel   *channel,
                       ClayContext   *context,
                       const gchar   *channel_name,
                       GeglColor     *channel_color,
                       gboolean       save_selection,
                       gboolean       channel_visible,
                       ClayColorTag   channel_color_tag,
                       gboolean       channel_lock_content,
                       gboolean       channel_lock_position,
                       gboolean       channel_lock_visibility,
                       gpointer       user_data)
{
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

  g_object_set (config,
                "channel-new-name",  channel_name,
                "channel-new-color", channel_color,
                NULL);

  if (save_selection)
    {
      ClayChannel *selection = clay_image_get_mask (image);

      channel = CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (selection),
                                                   CLAY_TYPE_CHANNEL));

      clay_object_set_name (CLAY_OBJECT (channel),
                            config->channel_new_name);
      clay_channel_set_color (channel, config->channel_new_color, FALSE);
    }
  else
    {
      channel = clay_channel_new (image,
                                  clay_image_get_width  (image),
                                  clay_image_get_height (image),
                                  config->channel_new_name,
                                  config->channel_new_color);

      clay_drawable_fill (CLAY_DRAWABLE (channel), context,
                          CLAY_FILL_TRANSPARENT);
    }

  clay_item_set_visible (CLAY_ITEM (channel), channel_visible, FALSE);
  clay_item_set_color_tag (CLAY_ITEM (channel), channel_color_tag, FALSE);
  clay_item_set_lock_content (CLAY_ITEM (channel), channel_lock_content, FALSE);
  clay_item_set_lock_position (CLAY_ITEM (channel), channel_lock_position, FALSE);
  clay_item_set_lock_visibility (CLAY_ITEM (channel), channel_lock_visibility, FALSE);

  clay_image_add_channel (image, channel,
                          CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
channels_edit_attributes_callback (GtkWidget     *dialog,
                                   ClayImage     *image,
                                   ClayChannel   *channel,
                                   ClayContext   *context,
                                   const gchar   *channel_name,
                                   GeglColor     *channel_color,
                                   gboolean       save_selection,
                                   gboolean       channel_visible,
                                   ClayColorTag   channel_color_tag,
                                   gboolean       channel_lock_content,
                                   gboolean       channel_lock_position,
                                   gboolean       channel_lock_visibility,
                                   gpointer       user_data)
{
  ClayItem *item = CLAY_ITEM (channel);

  if (strcmp (channel_name, clay_object_get_name (channel))                   ||
      ! clay_color_is_perceptually_identical (channel_color, channel->color) ||
      channel_visible         != clay_item_get_visible (item)                 ||
      channel_color_tag       != clay_item_get_color_tag (item)               ||
      channel_lock_content    != clay_item_get_lock_content (item)            ||
      channel_lock_position   != clay_item_get_lock_position (item)           ||
      channel_lock_visibility != clay_item_get_lock_visibility (item))
    {
      clay_image_undo_group_start (image,
                                   CLAY_UNDO_GROUP_ITEM_PROPERTIES,
                                   _("Channel Attributes"));

      if (strcmp (channel_name, clay_object_get_name (channel)))
        clay_item_rename (CLAY_ITEM (channel), channel_name, NULL);

      if (! clay_color_is_perceptually_identical (channel_color, channel->color))
        clay_channel_set_color (channel, channel_color, TRUE);

      if (channel_visible != clay_item_get_visible (item))
        clay_item_set_visible (item, channel_visible, TRUE);

      if (channel_color_tag != clay_item_get_color_tag (item))
        clay_item_set_color_tag (item, channel_color_tag, TRUE);

      if (channel_lock_content != clay_item_get_lock_content (item))
        clay_item_set_lock_content (item, channel_lock_content, TRUE);

      if (channel_lock_position != clay_item_get_lock_position (item))
        clay_item_set_lock_position (item, channel_lock_position, TRUE);

      if (channel_lock_visibility != clay_item_get_lock_visibility (item))
        clay_item_set_lock_visibility (item, channel_lock_visibility, TRUE);

      clay_image_undo_group_end (image);

      clay_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
