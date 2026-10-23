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

#include "actions-types.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/clayimage.h"
#include "core/clayimage-quick-mask.h"

#include "widgets/clayhelp-ids.h"

#include "dialogs/dialogs.h"
#include "dialogs/channel-options-dialog.h"
#include "dialogs/item-options-dialog.h"

#include "actions.h"
#include "quick-mask-commands.h"

#include "clay-intl.h"


#define RGBA_EPSILON 1e-6


/*  local function prototypes  */

static void   quick_mask_configure_callback (GtkWidget     *dialog,
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


/*  public functions */

void
quick_mask_toggle_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage *image;
  gboolean   active;
  return_if_no_image (image, data);

  active = g_variant_get_boolean (value);

  if (active != clay_image_get_quick_mask_state (image))
    {
      clay_image_set_quick_mask_state (image, active);
      clay_image_flush (image);
    }
}

void
quick_mask_invert_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage *image;
  gboolean   inverted;
  return_if_no_image (image, data);

  inverted = (gboolean) g_variant_get_int32 (value);

  if (inverted != clay_image_get_quick_mask_inverted (image))
    {
      clay_image_quick_mask_invert (image);
      clay_image_flush (image);
    }
}

void
quick_mask_configure_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

#define CONFIGURE_DIALOG_KEY "clay-image-quick-mask-configure-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY);

  if (! dialog)
    {
      GeglColor *color;

      color = clay_image_get_quick_mask_color (image);

      dialog = channel_options_dialog_new (image, NULL,
                                           action_data_get_context (data),
                                           widget,
                                           _("Quick Mask Attributes"),
                                           "clay-quick-mask-edit",
                                           CLAY_ICON_QUICK_MASK_ON,
                                           _("Edit Quick Mask Attributes"),
                                           CLAY_HELP_QUICK_MASK_EDIT,
                                           _("Edit Quick Mask Color"),
                                           _("_Mask opacity:"),
                                           FALSE,
                                           NULL,
                                           color,
                                           FALSE,
                                           CLAY_COLOR_TAG_NONE,
                                           FALSE,
                                           FALSE,
                                           FALSE,
                                           quick_mask_configure_callback,
                                           NULL);

      item_options_dialog_set_switches_visible (dialog, FALSE);

      dialogs_attach_dialog (G_OBJECT (image), CONFIGURE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
quick_mask_configure_callback (GtkWidget     *dialog,
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
  GeglColor *old_color;

  old_color = clay_image_get_quick_mask_color (image);

  if (! clay_color_is_perceptually_identical (old_color, channel_color))
    {
      clay_image_set_quick_mask_color (image, channel_color);
      clay_image_flush (image);
    }

  gtk_widget_destroy (dialog);
}
