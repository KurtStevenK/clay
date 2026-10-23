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
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/clayimage.h"
#include "core/clayselection.h"

#include "widgets/clayhelp-ids.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claywidgets-utils.h"
#include "widgets/claywindowstrategy.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "dialogs/dialogs.h"

#include "actions.h"
#include "items-commands.h"
#include "select-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   select_feather_callback (GtkWidget *widget,
                                       gdouble    size,
                                       ClayUnit  *unit,
                                       gpointer   data);
static void   select_border_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       ClayUnit  *unit,
                                       gpointer   data);
static void   select_grow_callback    (GtkWidget *widget,
                                       gdouble    size,
                                       ClayUnit  *unit,
                                       gpointer   data);
static void   select_shrink_callback  (GtkWidget *widget,
                                       gdouble    size,
                                       ClayUnit  *unit,
                                       gpointer   data);
static void   select_float            (ClayAction *action,
                                       GVariant   *value,
                                       gboolean    cut,
                                       gpointer    data);


/*  public functions  */

void
select_all_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_channel_all (clay_image_get_mask (image), TRUE);
  clay_image_flush (image);
}

void
select_none_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);
  clay_image_flush (image);
}

void
select_invert_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_channel_invert (clay_image_get_mask (image), TRUE);
  clay_image_flush (image);
}

void
select_cut_float_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  select_float (action, value, TRUE, data);
}

void
select_copy_float_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  select_float (action, value, FALSE, data);
}

void
select_feather_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define FEATHER_DIALOG_KEY "clay-selection-feather-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      GtkWidget        *button;
      gdouble           xres;
      gdouble           yres;

      clay_image_get_resolution (image, &xres, &yres);

      dialog = clay_query_size_box (_("Feather Selection"),
                                    GTK_WIDGET (clay_display_get_shell (display)),
                                    clay_standard_help_func,
                                    CLAY_HELP_SELECTION_FEATHER,
                                    _("Feather selection by"),
                                    config->selection_feather_radius, 0, 5250, 3,
                                    clay_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_feather_callback,
                                    image, NULL);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      clay_help_set_help_data (button,
                               _("When feathering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_feather_edge_lock);
      gtk_box_pack_start (GTK_BOX (CLAY_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);

      dialogs_attach_dialog (G_OBJECT (image), FEATHER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_sharpen_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_channel_sharpen (clay_image_get_mask (image), TRUE);
  clay_image_flush (image);
}

void
select_shrink_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define SHRINK_DIALOG_KEY "clay-selection-shrink-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      clay_image_get_resolution (image, &xres, &yres);

      dialog = clay_query_size_box (_("Shrink Selection"),
                                    GTK_WIDGET (clay_display_get_shell (display)),
                                    clay_standard_help_func,
                                    CLAY_HELP_SELECTION_SHRINK,
                                    _("Shrink selection by"),
                                    config->selection_shrink_radius,
                                    1, max_value, 0,
                                    clay_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_shrink_callback,
                                    image, NULL);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      clay_help_set_help_data (button,
                               _("When shrinking, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_shrink_edge_lock);
      gtk_box_pack_start (GTK_BOX (CLAY_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);

      dialogs_attach_dialog (G_OBJECT (image), SHRINK_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_grow_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define GROW_DIALOG_KEY "clay-selection-grow-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GROW_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      width  = clay_image_get_width  (image);
      height = clay_image_get_height (image);
      max_value = MAX (width, height);

      clay_image_get_resolution (image, &xres, &yres);

      dialog = clay_query_size_box (_("Grow Selection"),
                                    GTK_WIDGET (clay_display_get_shell (display)),
                                    clay_standard_help_func,
                                    CLAY_HELP_SELECTION_GROW,
                                    _("Grow selection by"),
                                    config->selection_grow_radius,
                                    1, max_value, 0,
                                    clay_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_grow_callback,
                                    image, NULL);

      dialogs_attach_dialog (G_OBJECT (image), GROW_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_border_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define BORDER_DIALOG_KEY "clay-selection-border-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), BORDER_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      GtkWidget        *combo;
      GtkWidget        *button;
      gint              width;
      gint              height;
      gint              max_value;
      gdouble           xres;
      gdouble           yres;

      clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                        NULL, NULL, &width, &height);
      max_value = MIN (width, height) / 2;

      clay_image_get_resolution (image, &xres, &yres);

      dialog = clay_query_size_box (_("Border Selection"),
                                    GTK_WIDGET (clay_display_get_shell (display)),
                                    clay_standard_help_func,
                                    CLAY_HELP_SELECTION_BORDER,
                                    _("Border selection by"),
                                    config->selection_border_radius,
                                    1, max_value, 0,
                                    clay_display_get_shell (display)->unit,
                                    MIN (xres, yres),
                                    FALSE,
                                    G_OBJECT (image), "disconnect",
                                    select_border_callback,
                                    image, NULL);

      /* Border style combo */
      combo = clay_enum_combo_box_new (CLAY_TYPE_CHANNEL_BORDER_STYLE);
      clay_int_combo_box_set_label (CLAY_INT_COMBO_BOX (combo),
                                    _("Border style"));

      gtk_box_pack_start (GTK_BOX (CLAY_QUERY_BOX_VBOX (dialog)), combo,
                          FALSE, FALSE, 0);

      g_object_set_data (G_OBJECT (dialog), "border-style-combo", combo);
      clay_int_combo_box_set_active (CLAY_INT_COMBO_BOX (combo),
                                     config->selection_border_style);
      gtk_widget_set_visible (combo, TRUE);

      /* Edge lock button */
      button = gtk_check_button_new_with_mnemonic (_("_Selected areas continue outside the image"));
      g_object_set_data (G_OBJECT (dialog), "edge-lock-toggle", button);
      clay_help_set_help_data (button,
                               _("When bordering, act as if selected areas "
                                 "continued outside the image."),
                               NULL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    config->selection_border_edge_lock);
      gtk_box_pack_start (GTK_BOX (CLAY_QUERY_BOX_VBOX (dialog)), button,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (button, TRUE);

      dialogs_attach_dialog (G_OBJECT (image), BORDER_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
select_flood_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_channel_flood (clay_image_get_mask (image), TRUE);
  clay_image_flush (image);
}

void
select_save_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage   *image;
  ClayChannel *channel;
  GtkWidget   *widget;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  channel = CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (clay_image_get_mask (image)),
                                               CLAY_TYPE_CHANNEL));

  /*  saved selections are not visible by default  */
  clay_item_set_visible (CLAY_ITEM (channel), FALSE, FALSE);

  clay_image_add_channel (image, channel,
                          CLAY_IMAGE_ACTIVE_PARENT, -1, TRUE);
  clay_image_flush (image);

  clay_window_strategy_show_dockable_dialog (CLAY_WINDOW_STRATEGY (clay_get_window_strategy (image->clay)),
                                             image->clay,
                                             clay_dialog_factory_get_singleton (),
                                             clay_widget_get_monitor (widget),
                                             "clay-channel-list");
}

void
select_fill_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayImage *image;
  GList     *selection;
  return_if_no_image (image, data);

  selection = g_list_prepend (NULL, clay_image_get_mask (image)),
  items_fill_cmd_callback (action,
                           image, selection,
                           _("Fill Selection Outline"),
                           CLAY_ICON_TOOL_BUCKET_FILL,
                           CLAY_HELP_SELECTION_FILL,
                           data);
  g_list_free (selection);
}

void
select_fill_last_vals_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GList     *selection;
  return_if_no_image (image, data);

  selection = g_list_prepend (NULL, clay_image_get_mask (image)),
  items_fill_last_vals_cmd_callback (action, image, selection, data);
  g_list_free (selection);
}

void
select_stroke_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage *image;
  GList     *selection;
  return_if_no_image (image, data);

  selection = g_list_prepend (NULL, clay_image_get_mask (image)),
  items_stroke_cmd_callback (action, image, selection,
                             _("Stroke Selection"),
                             CLAY_ICON_SELECTION_STROKE,
                             CLAY_HELP_SELECTION_STROKE,
                             data);
  g_list_free (selection);
}

void
select_stroke_last_vals_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage *image;
  GList     *selection;
  return_if_no_image (image, data);

  selection = g_list_prepend (NULL, clay_image_get_mask (image)),
  items_stroke_last_vals_cmd_callback (action, image, selection, data);
  g_list_free (selection);
}


/*  private functions  */

static void
select_feather_callback (GtkWidget *widget,
                         gdouble    size,
                         ClayUnit  *unit,
                         gpointer   data)
{
  ClayImage        *image  = CLAY_IMAGE (data);
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  GtkWidget        *button;
  gdouble           radius_x;
  gdouble           radius_y;

  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  g_object_set (config,
                "selection-feather-radius", size,
                "selection-feather-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = config->selection_feather_radius;
  radius_y = config->selection_feather_radius;

  if (unit != clay_unit_pixel ())
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      clay_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  clay_channel_feather (clay_image_get_mask (image), radius_x, radius_y,
                        config->selection_feather_edge_lock,
                        TRUE);
  clay_image_flush (image);
}

static void
select_border_callback (GtkWidget *widget,
                        gdouble    size,
                        ClayUnit  *unit,
                        gpointer   data)
{
  ClayImage        *image  = CLAY_IMAGE (data);
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  GtkWidget        *combo;
  GtkWidget        *button;
  gdouble           radius_x;
  gdouble           radius_y;
  gint              border_style;

  combo  = g_object_get_data (G_OBJECT (widget), "border-style-combo");
  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  clay_int_combo_box_get_active (CLAY_INT_COMBO_BOX (combo), &border_style);

  g_object_set (config,
                "selection-border-radius", size,
                "selection-border-style",  border_style,
                "selection-border-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = ROUND (config->selection_border_radius);
  radius_y = ROUND (config->selection_border_radius);

  if (unit != clay_unit_pixel ())
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      clay_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  clay_channel_border (clay_image_get_mask (image), radius_x, radius_y,
                       config->selection_border_style,
                       config->selection_border_edge_lock,
                       TRUE);
  clay_image_flush (image);
}

static void
select_grow_callback (GtkWidget *widget,
                      gdouble    size,
                      ClayUnit  *unit,
                      gpointer   data)
{
  ClayImage        *image  = CLAY_IMAGE (data);
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  gdouble           radius_x;
  gdouble           radius_y;

  g_object_set (config,
                "selection-grow-radius", size,
                NULL);

  radius_x = ROUND (config->selection_grow_radius);
  radius_y = ROUND (config->selection_grow_radius);

  if (unit != clay_unit_pixel ())
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      clay_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  clay_channel_grow (clay_image_get_mask (image), radius_x, radius_y, TRUE);
  clay_image_flush (image);
}

static void
select_shrink_callback (GtkWidget *widget,
                        gdouble    size,
                        ClayUnit  *unit,
                        gpointer   data)
{
  ClayImage        *image  = CLAY_IMAGE (data);
  ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
  GtkWidget        *button;
  gint              radius_x;
  gint              radius_y;

  button = g_object_get_data (G_OBJECT (widget), "edge-lock-toggle");

  g_object_set (config,
                "selection-shrink-radius", size,
                "selection-shrink-edge-lock",
                gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)),
                NULL);

  radius_x = ROUND (config->selection_shrink_radius);
  radius_y = ROUND (config->selection_shrink_radius);

  if (unit != clay_unit_pixel ())
    {
      gdouble xres;
      gdouble yres;
      gdouble factor;

      clay_image_get_resolution (image, &xres, &yres);

      factor = (MAX (xres, yres) /
                MIN (xres, yres));

      if (xres == MIN (xres, yres))
        radius_y *= factor;
      else
        radius_x *= factor;
    }

  clay_channel_shrink (clay_image_get_mask (image), radius_x, radius_y,
                       config->selection_shrink_edge_lock,
                       TRUE);
  clay_image_flush (image);
}

static void
select_float (ClayAction *action,
              GVariant   *value,
              gboolean    cut,
              gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  GList     *drawables;
  GError    *error = NULL;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  drawables = clay_image_get_selected_drawables (image);
  if (clay_selection_float (CLAY_SELECTION (clay_image_get_mask (image)),
                            drawables,
                            action_data_get_context (data),
                            cut, 0, 0, &error))
    {
      clay_image_flush (image);
    }
  else
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
    }
  g_list_free (drawables);
}
