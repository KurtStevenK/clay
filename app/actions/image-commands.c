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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "config/claydialogconfig.h"

#include "gegl/clay-babl.h"

#include "core/core-enums.h"
#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"
#include "core/clayimage-convert-indexed.h"
#include "core/clayimage-convert-precision.h"
#include "core/clayimage-convert-type.h"
#include "core/clayimage-crop.h"
#include "core/clayimage-duplicate.h"
#include "core/clayimage-flip.h"
#include "core/clayimage-merge.h"
#include "core/clayimage-resize.h"
#include "core/clayimage-rotate.h"
#include "core/clayimage-scale.h"
#include "core/clayimage-undo.h"
#include "core/clayitem.h"
#include "core/claypickable.h"
#include "core/claypickable-auto-shrink.h"
#include "core/clayprogress.h"

#include "widgets/claydialogfactory.h"
#include "widgets/claydock.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "display/claydisplay.h"
#include "display/claydisplayshell.h"

#include "dialogs/dialogs.h"
#include "dialogs/color-profile-dialog.h"
#include "dialogs/convert-indexed-dialog.h"
#include "dialogs/convert-precision-dialog.h"
#include "dialogs/grid-dialog.h"
#include "dialogs/image-merge-layers-dialog.h"
#include "dialogs/image-new-dialog.h"
#include "dialogs/image-properties-dialog.h"
#include "dialogs/image-scale-dialog.h"
#include "dialogs/print-size-dialog.h"
#include "dialogs/resize-dialog.h"

#include "actions.h"
#include "image-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   image_convert_rgb_callback       (GtkWidget                *dialog,
                                                ClayImage                *image,
                                                ClayColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                ClayColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_gray_callback      (GtkWidget                *dialog,
                                                ClayImage                *image,
                                                ClayColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                ClayColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_convert_indexed_callback   (GtkWidget              *dialog,
                                                ClayImage              *image,
                                                ClayConvertPaletteType  palette_type,
                                                gint                    max_colors,
                                                gboolean                remove_duplicates,
                                                ClayConvertDitherType   dither_type,
                                                gboolean                dither_alpha,
                                                gboolean                dither_text_layers,
                                                ClayPalette            *custom_palette,
                                                gpointer                user_data);

static void   image_convert_precision_callback (GtkWidget              *dialog,
                                                ClayImage              *image,
                                                ClayPrecision           precision,
                                                GeglDitherMethod        layer_dither_method,
                                                GeglDitherMethod        text_layer_dither_method,
                                                GeglDitherMethod        mask_dither_method,
                                                gpointer                user_data);

static void   image_profile_assign_callback    (GtkWidget                *dialog,
                                                ClayImage                *image,
                                                ClayColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                ClayColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_profile_convert_callback   (GtkWidget                *dialog,
                                                ClayImage                *image,
                                                ClayColorProfile         *new_profile,
                                                GFile                    *new_file,
                                                ClayColorRenderingIntent  intent,
                                                gboolean                  bpc,
                                                gpointer                  user_data);

static void   image_resize_callback            (GtkWidget              *dialog,
                                                ClayViewable           *viewable,
                                                ClayContext            *context,
                                                gint                    width,
                                                gint                    height,
                                                ClayUnit               *unit,
                                                gint                    offset_x,
                                                gint                    offset_y,
                                                gdouble                 xres,
                                                gdouble                 yres,
                                                ClayUnit               *res_unit,
                                                ClayFillType            fill_type,
                                                ClayItemSet             layer_set,
                                                gboolean                resize_text_layers,
                                                gpointer                user_data);

static void   image_print_size_callback        (GtkWidget              *dialog,
                                                ClayImage              *image,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                ClayUnit               *resolution_unit,
                                                gpointer                user_data);

static void   image_scale_callback             (GtkWidget              *dialog,
                                                ClayViewable           *viewable,
                                                gint                    width,
                                                gint                    height,
                                                ClayUnit               *unit,
                                                ClayInterpolationType   interpolation,
                                                gdouble                 xresolution,
                                                gdouble                 yresolution,
                                                ClayUnit               *resolution_unit,
                                                gpointer                user_data);

static void   image_merge_layers_callback      (GtkWidget              *dialog,
                                                ClayImage              *image,
                                                ClayContext            *context,
                                                ClayMergeType           merge_type,
                                                gboolean                merge_active_group,
                                                gboolean                discard_invisible,
                                                gpointer                user_data);

static void   image_softproof_profile_callback  (GtkWidget                *dialog,
                                                 ClayImage                *image,
                                                 ClayColorProfile         *new_profile,
                                                 GFile                    *new_file,
                                                 ClayColorRenderingIntent  intent,
                                                 gboolean                  bpc,
                                                 gpointer                  user_data);



/*  private variables  */

static ClayUnit              *image_resize_unit  = NULL;
static ClayUnit              *image_scale_unit   = NULL;
static ClayInterpolationType  image_scale_interp = -1;
static ClayPalette           *image_convert_indexed_custom_palette = NULL;


/*  public functions  */

void
image_new_cmd_callback (ClayAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  GtkWidget *widget;
  GtkWidget *dialog;
  return_if_no_widget (widget, data);

  dialog = clay_dialog_factory_dialog_new (clay_dialog_factory_get_singleton (),
                                           clay_widget_get_monitor (widget),
                                           NULL /*ui_manager*/,
                                           widget,
                                           "clay-image-new-dialog", -1, FALSE);

  if (dialog)
    {
      ClayImage *image = action_data_get_image (data);

      image_new_dialog_set (dialog, image, NULL);

      gtk_window_present (GTK_WINDOW (dialog));
    }
}

void
image_duplicate_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  ClayDisplay      *display;
  ClayImage        *image;
  ClayDisplayShell *shell;
  ClayImage        *new_image;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);
  shell = clay_display_get_shell (display);

  new_image = clay_image_duplicate (image);

  clay_create_display (new_image->clay, new_image, shell->unit,
                       clay_zoom_model_get_factor (shell->zoom),
                       G_OBJECT (clay_widget_get_monitor (GTK_WIDGET (shell))));

  g_object_unref (new_image);
}

void
image_convert_base_type_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage         *image;
  ClayDisplay       *display;
  GtkWidget         *widget;
  ClayDialogConfig  *config;
  GtkWidget         *dialog;
  ClayImageBaseType  base_type;
  GError            *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  base_type = (ClayImageBaseType) g_variant_get_int32 (value);

  if (base_type == clay_image_get_base_type (image))
    return;

#define CONVERT_TYPE_DIALOG_KEY "clay-convert-type-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_TYPE_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  switch (base_type)
    {
    case CLAY_RGB:
    case CLAY_GRAY:
      if (clay_image_get_color_profile (image))
        {
          ColorProfileDialogType    dialog_type;
          ClayColorProfileCallback  callback;
          ClayColorProfile         *current_profile;
          ClayColorProfile         *default_profile;
          ClayTRCType               trc;

          current_profile =
            clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));

          trc = clay_babl_trc (clay_image_get_precision (image));

          if (base_type == CLAY_RGB)
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_RGB;
              callback    = image_convert_rgb_callback;

              default_profile = clay_babl_get_builtin_color_profile (CLAY_RGB,
                                                                     trc);
            }
          else
            {
              dialog_type = COLOR_PROFILE_DIALOG_CONVERT_TO_GRAY;
              callback    = image_convert_gray_callback;

              default_profile = clay_babl_get_builtin_color_profile (CLAY_GRAY,
                                                                     trc);
            }

          dialog = color_profile_dialog_new (dialog_type,
                                             image,
                                             action_data_get_context (data),
                                             widget,
                                             current_profile,
                                             default_profile,
                                             0, 0,
                                             callback,
                                             display);
        }
      else if (! clay_image_convert_type (image, base_type, NULL, NULL, &error))
        {
          clay_message_literal (image->clay,
                                G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                                error->message);
          g_clear_error (&error);
        }
      break;

    case CLAY_INDEXED:
      dialog = convert_indexed_dialog_new (image,
                                           action_data_get_context (data),
                                           widget,
                                           config->image_convert_indexed_palette_type,
                                           config->image_convert_indexed_max_colors,
                                           config->image_convert_indexed_remove_duplicates,
                                           config->image_convert_indexed_dither_type,
                                           config->image_convert_indexed_dither_alpha,
                                           config->image_convert_indexed_dither_text_layers,
                                           image_convert_indexed_custom_palette,
                                           image_convert_indexed_callback,
                                           display);
      break;
    }

  if (dialog)
    {
      dialogs_attach_dialog (G_OBJECT (image),
                             CONVERT_TYPE_DIALOG_KEY, dialog);
      gtk_window_present (GTK_WINDOW (dialog));
    }

  /*  always flush, also when only the indexed dialog was shown, so
   *  the menu items get updated back to the current image type
   */
  clay_image_flush (image);
}

void
image_convert_precision_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage         *image;
  ClayDisplay       *display;
  GtkWidget         *widget;
  ClayDialogConfig  *config;
  GtkWidget         *dialog;
  ClayComponentType  component_type;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  component_type = (ClayComponentType) g_variant_get_int32 (value);

  if (component_type == clay_image_get_component_type (image))
    return;

#define CONVERT_PRECISION_DIALOG_KEY "clay-convert-precision-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), CONVERT_PRECISION_DIALOG_KEY);

  if (dialog)
    {
      gtk_widget_destroy (dialog);
      dialog = NULL;
    }

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  dialog = convert_precision_dialog_new (image,
                                         action_data_get_context (data),
                                         widget,
                                         component_type,
                                         config->image_convert_precision_layer_dither_method,
                                         config->image_convert_precision_text_layer_dither_method,
                                         config->image_convert_precision_channel_dither_method,
                                         image_convert_precision_callback,
                                         display);

  dialogs_attach_dialog (G_OBJECT (image),
                         CONVERT_PRECISION_DIALOG_KEY, dialog);

  gtk_window_present (GTK_WINDOW (dialog));

  /*  see comment above  */
  clay_image_flush (image);
}

void
image_convert_trc_cmd_callback (ClayAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  ClayImage     *image;
  ClayDisplay   *display;
  ClayTRCType    trc_type;
  ClayPrecision  precision;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  trc_type = (ClayTRCType) g_variant_get_int32 (value);

  if (trc_type == clay_babl_format_get_trc (clay_image_get_layer_format (image,
                                                                         FALSE)))
    return;

  precision = clay_babl_precision (clay_image_get_component_type (image),
                                   trc_type);

  clay_image_convert_precision (image, precision,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                GEGL_DITHER_NONE,
                                CLAY_PROGRESS (display));
  clay_image_flush (image);
}

void
image_color_profile_use_srgb_cmd_callback (ClayAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  ClayImage *image;
  gboolean   use_srgb;
  return_if_no_image (image, data);

  use_srgb = g_variant_get_boolean (value);

  if (use_srgb != clay_image_get_use_srgb_profile (image, NULL))
    {
      clay_image_set_use_srgb_profile (image, use_srgb);
      clay_image_flush (image);
    }
}

void
image_color_profile_assign_cmd_callback (ClayAction *action,
                                         GVariant   *value,
                                         gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_ASSIGN_DIALOG_KEY "clay-profile-assign-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_ASSIGN_DIALOG_KEY);

  if (! dialog)
    {
      ClayColorProfile *current_profile;
      ClayColorProfile *default_profile;

      current_profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));
      default_profile = clay_image_get_builtin_color_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_ASSIGN_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         current_profile,
                                         default_profile,
                                         0, 0,
                                         image_profile_assign_callback,
                                         display);

      dialogs_attach_dialog (G_OBJECT (image),
                             PROFILE_ASSIGN_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_color_profile_convert_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_CONVERT_DIALOG_KEY "clay-profile-convert-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_CONVERT_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      ClayColorProfile *current_profile;
      ClayColorProfile *default_profile;

      current_profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));
      default_profile = clay_image_get_builtin_color_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_CONVERT_TO_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         widget,
                                         current_profile,
                                         default_profile,
                                         config->image_convert_profile_intent,
                                         config->image_convert_profile_bpc,
                                         image_profile_convert_callback,
                                         display);

      dialogs_attach_dialog (G_OBJECT (image),
                             PROFILE_CONVERT_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_color_profile_discard_cmd_callback (ClayAction *action,
                                          GVariant   *value,
                                          gpointer    data)
{
  ClayImage *image;
  return_if_no_image (image, data);

  clay_image_assign_color_profile (image, NULL, NULL, NULL);
  clay_image_flush (image);
}

static void
image_profile_save_dialog_response (GtkWidget *dialog,
                                    gint       response_id,
                                    ClayImage *image)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    {
      ClayColorProfile *profile;
      GFile            *file;
      GError           *error = NULL;

      profile = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));
      file    = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));

      if (! file)
        return;

      if (! clay_color_profile_save_to_file (profile, file, &error))
        {
          clay_message (image->clay, NULL,
                        CLAY_MESSAGE_WARNING,
                        _("Saving color profile failed: %s"),
                        error->message);
          g_clear_error (&error);
          g_object_unref (file);
          return;
        }

      g_object_unref (file);
    }

  gtk_widget_destroy (dialog);
}

void
image_color_profile_save_cmd_callback (ClayAction *action,
                                       GVariant   *value,
                                       gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define PROFILE_SAVE_DIALOG_KEY "clay-profile-save-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROFILE_SAVE_DIALOG_KEY);

  if (! dialog)
    {
      GtkWindow        *toplevel;
      ClayColorProfile *profile;
      gchar            *basename;

      toplevel = GTK_WINDOW (gtk_widget_get_toplevel (widget));
      profile  = clay_color_managed_get_color_profile (CLAY_COLOR_MANAGED (image));

      dialog =
        clay_color_profile_chooser_dialog_new (_("Save Color Profile"),
                                               toplevel,
                                               GTK_FILE_CHOOSER_ACTION_SAVE);

      clay_color_profile_chooser_dialog_connect_path (dialog,
                                                      G_OBJECT (image->clay->config),
                                                      "color-profile-path");

      basename = g_strconcat (clay_color_profile_get_label (profile),
                              ".icc", NULL);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);
      g_free (basename);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (image_profile_save_dialog_response),
                        image);

      dialogs_attach_dialog (G_OBJECT (image), PROFILE_SAVE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_resize_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayImage   *image;
  GtkWidget   *widget;
  ClayDisplay *display;
  GtkWidget   *dialog;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);
  return_if_no_display (display, data);

#define RESIZE_DIALOG_KEY "clay-resize-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), RESIZE_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      if (image_resize_unit != clay_unit_percent ())
        image_resize_unit = clay_display_get_shell (display)->unit;

      dialog = resize_dialog_new (CLAY_VIEWABLE (image),
                                  action_data_get_context (data),
                                  _("Set Image Canvas Size"),
                                  "clay-image-resize",
                                  widget,
                                  clay_standard_help_func,
                                  CLAY_HELP_IMAGE_RESIZE,
                                  image_resize_unit,
                                  config->image_resize_fill_type,
                                  config->image_resize_layer_set,
                                  config->image_resize_resize_text_layers,
                                  image_resize_callback,
                                  display);

      dialogs_attach_dialog (G_OBJECT (image), RESIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_resize_to_layers_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayDisplay  *display;
  ClayImage    *image;
  ClayProgress *progress;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                  _("Resizing"));

  clay_image_resize_to_layers (image,
                               action_data_get_context (data),
                               NULL, NULL, NULL, NULL, progress);

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);
}

void
image_resize_to_selection_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayDisplay  *display;
  ClayImage    *image;
  ClayProgress *progress;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

  progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                  _("Resizing"));

  clay_image_resize_to_selection (image,
                                  action_data_get_context (data),
                                  progress);

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);
}

void
image_print_size_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = clay_display_get_image (display);

#define PRINT_SIZE_DIALOG_KEY "gimp-print-size-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY);

  if (! dialog)
    {
      dialog = print_size_dialog_new (image,
                                      action_data_get_context (data),
                                      _("Set Image Print Resolution"),
                                      "clay-image-print-size",
                                      widget,
                                      clay_standard_help_func,
                                      CLAY_HELP_IMAGE_PRINT_SIZE,
                                      image_print_size_callback,
                                      NULL);

      dialogs_attach_dialog (G_OBJECT (image), PRINT_SIZE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_scale_cmd_callback (ClayAction *action,
                          GVariant   *value,
                          gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *widget;
  GtkWidget   *dialog;
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  image = clay_display_get_image (display);

#define SCALE_DIALOG_KEY "clay-scale-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), SCALE_DIALOG_KEY);

  if (! dialog)
    {
      if (image_scale_unit != clay_unit_percent ())
        image_scale_unit = clay_display_get_shell (display)->unit;

      if (image_scale_interp == -1)
        image_scale_interp = display->clay->config->interpolation_type;

      dialog = image_scale_dialog_new (image,
                                       action_data_get_context (data),
                                       widget,
                                       image_scale_unit,
                                       image_scale_interp,
                                       image_scale_callback,
                                       display);

      dialogs_attach_dialog (G_OBJECT (image), SCALE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_flip_cmd_callback (ClayAction *action,
                         GVariant   *value,
                         gpointer    data)
{
  ClayDisplay         *display;
  ClayImage           *image;
  ClayProgress        *progress;
  ClayOrientationType  orientation;
  return_if_no_display (display, data);

  orientation = (ClayOrientationType) g_variant_get_int32 (value);

  image = clay_display_get_image (display);

  progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                  _("Flipping"));

  clay_image_flip (image, action_data_get_context (data),
                   orientation, progress);

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);
}

void
image_rotate_cmd_callback (ClayAction *action,
                           GVariant   *value,
                           gpointer    data)
{
  ClayDisplay      *display;
  ClayImage        *image;
  ClayProgress     *progress;
  ClayRotationType  rotation;
  return_if_no_display (display, data);

  rotation = (ClayRotationType) g_variant_get_int32 (value);

  image = clay_display_get_image (display);

  progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                  _("Rotating"));

  clay_image_rotate (image, action_data_get_context (data),
                     rotation, progress);

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);
}

void
image_crop_to_selection_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  if (! clay_item_bounds (CLAY_ITEM (clay_image_get_mask (image)),
                          &x, &y, &width, &height))
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            _("Cannot crop because the current selection "
                              "is empty."));
      return;
    }

  clay_image_crop (image,
                   action_data_get_context (data), CLAY_FILL_TRANSPARENT,
                   x, y, width, height, TRUE);
  clay_image_flush (image);
}

void
image_crop_to_content_cmd_callback (ClayAction *action,
                                    GVariant   *value,
                                    gpointer    data)
{
  ClayImage *image;
  GtkWidget *widget;
  gint       x, y;
  gint       width, height;
  return_if_no_image (image, data);
  return_if_no_widget (widget, data);

  switch (clay_pickable_auto_shrink (CLAY_PICKABLE (image),
                                     0, 0,
                                     clay_image_get_width  (image),
                                     clay_image_get_height (image),
                                     &x, &y, &width, &height))
    {
    case CLAY_AUTO_SHRINK_SHRINK:
      clay_image_crop (image,
                       action_data_get_context (data), CLAY_FILL_TRANSPARENT,
                       x, y, width, height, TRUE);
      clay_image_flush (image);
      break;

    case CLAY_AUTO_SHRINK_EMPTY:
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_INFO,
                            _("Cannot crop because the image has no content."));
      break;

    case CLAY_AUTO_SHRINK_UNSHRINKABLE:
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_INFO,
                            _("Cannot crop because the image is already "
                              "cropped to its content."));
      break;
    }
}

void
image_merge_layers_cmd_callback (ClayAction *action,
                                 GVariant   *value,
                                 gpointer    data)
{
  GtkWidget   *dialog;
  ClayImage   *image;
  ClayDisplay *display;
  GtkWidget   *widget;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

#define MERGE_LAYERS_DIALOG_KEY "clay-merge-layers-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY);

  if (! dialog)
    {
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);

      dialog = image_merge_layers_dialog_new (image,
                                              action_data_get_context (data),
                                              widget,
                                              config->layer_merge_type,
                                              config->layer_merge_active_group_only,
                                              config->layer_merge_discard_invisible,
                                              image_merge_layers_callback,
                                              display);

      dialogs_attach_dialog (G_OBJECT (image), MERGE_LAYERS_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_merge_layers_last_vals_cmd_callback (ClayAction *action,
                                           GVariant   *value,
                                           gpointer    data)
{
  ClayImage        *image;
  ClayDisplay      *display;
  ClayDialogConfig *config;
  return_if_no_image (image, data);
  return_if_no_display (display, data);

  config = CLAY_DIALOG_CONFIG (image->clay->config);

  image_merge_layers_callback (NULL,
                               image,
                               action_data_get_context (data),
                               config->layer_merge_type,
                               config->layer_merge_active_group_only,
                               config->layer_merge_discard_invisible,
                               display);
}

void
image_flatten_image_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage   *image;
  ClayDisplay *display;
  GtkWidget   *widget;
  GError      *error = NULL;
  return_if_no_image (image, data);
  return_if_no_display (display, data);
  return_if_no_widget (widget, data);

  if (! clay_image_flatten (image, action_data_get_context (data),
                            CLAY_PROGRESS (display), &error))
    {
      clay_message_literal (image->clay,
                            G_OBJECT (widget), CLAY_MESSAGE_WARNING,
                            error->message);
      g_clear_error (&error);
      return;
    }

  clay_image_flush (image);
}

void
image_configure_grid_cmd_callback (ClayAction *action,
                                   GVariant   *value,
                                   gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define GRID_DIALOG_KEY "clay-grid-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), GRID_DIALOG_KEY);

  if (! dialog)
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);

      dialog = grid_dialog_new (image,
                                action_data_get_context (data),
                                gtk_widget_get_toplevel (GTK_WIDGET (shell)));

      dialogs_attach_dialog (G_OBJECT (image), GRID_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

void
image_properties_cmd_callback (ClayAction *action,
                               GVariant   *value,
                               gpointer    data)
{
  ClayDisplay *display;
  ClayImage   *image;
  GtkWidget   *dialog;
  return_if_no_display (display, data);

  image = clay_display_get_image (display);

#define PROPERTIES_DIALOG_KEY "clay-image-properties-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (image), PROPERTIES_DIALOG_KEY);

  if (! dialog)
    {
      ClayDisplayShell *shell = clay_display_get_shell (display);

      dialog = image_properties_dialog_new (image,
                                            action_data_get_context (data),
                                            gtk_widget_get_toplevel (GTK_WIDGET (shell)));

      dialogs_attach_dialog (G_OBJECT (image), PROPERTIES_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}


/*  private functions  */

static void
image_convert_rgb_callback (GtkWidget                *dialog,
                            ClayImage                *image,
                            ClayColorProfile         *new_profile,
                            GFile                    *new_file,
                            ClayColorRenderingIntent  intent,
                            gboolean                  bpc,
                            gpointer                  user_data)
{
  ClayProgress *progress = user_data;
  GError       *error    = NULL;

  progress = clay_progress_start (progress, FALSE,
                                  _("Converting to RGB (%s)"),
                                  clay_color_profile_get_label (new_profile));

  if (! clay_image_convert_type (image, CLAY_RGB, new_profile,
                                 progress, &error))
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        clay_progress_end (progress);

      return;
    }

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);

 gtk_widget_destroy (dialog);
}

static void
image_convert_gray_callback (GtkWidget                *dialog,
                             ClayImage                *image,
                             ClayColorProfile         *new_profile,
                             GFile                    *new_file,
                             ClayColorRenderingIntent  intent,
                             gboolean                  bpc,
                             gpointer                  user_data)
{
  ClayProgress *progress = user_data;
  GError       *error    = NULL;

  progress = clay_progress_start (progress, FALSE,
                                  _("Converting to grayscale (%s)"),
                                  clay_color_profile_get_label (new_profile));

  if (! clay_image_convert_type (image, CLAY_GRAY, new_profile,
                                 progress, &error))
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        clay_progress_end (progress);

      return;
    }

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_indexed_callback (GtkWidget              *dialog,
                                ClayImage              *image,
                                ClayConvertPaletteType  palette_type,
                                gint                    max_colors,
                                gboolean                remove_duplicates,
                                ClayConvertDitherType   dither_type,
                                gboolean                dither_alpha,
                                gboolean                dither_text_layers,
                                ClayPalette            *custom_palette,
                                gpointer                user_data)
{
  ClayDialogConfig *config  = CLAY_DIALOG_CONFIG (image->clay->config);
  ClayDisplay      *display = user_data;
  ClayProgress     *progress;
  GError           *error   = NULL;

  g_object_set (config,
                "image-convert-indexed-palette-type",       palette_type,
                "image-convert-indexed-max-colors",         max_colors,
                "image-convert-indexed-remove-duplicates",  remove_duplicates,
                "image-convert-indexed-dither-type",        dither_type,
                "image-convert-indexed-dither-alpha",       dither_alpha,
                "image-convert-indexed-dither-text-layers", dither_text_layers,
                NULL);

  g_set_weak_pointer (&image_convert_indexed_custom_palette, custom_palette);

  progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                  _("Converting to indexed colors"));

  if (! clay_image_convert_indexed (image,
                                    config->image_convert_indexed_palette_type,
                                    config->image_convert_indexed_max_colors,
                                    config->image_convert_indexed_remove_duplicates,
                                    config->image_convert_indexed_dither_type,
                                    config->image_convert_indexed_dither_alpha,
                                    config->image_convert_indexed_dither_text_layers,
                                    image_convert_indexed_custom_palette,
                                    progress,
                                    &error))
    {
      clay_message_literal (image->clay, G_OBJECT (display),
                            CLAY_MESSAGE_WARNING, error->message);
      g_clear_error (&error);

      if (progress)
        clay_progress_end (progress);

      /* Conversion failed, make the dialog active so the user can try again. */
      gtk_widget_set_sensitive (dialog, TRUE);

      return;
    }

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_convert_precision_callback (GtkWidget        *dialog,
                                  ClayImage        *image,
                                  ClayPrecision     precision,
                                  GeglDitherMethod  layer_dither_method,
                                  GeglDitherMethod  text_layer_dither_method,
                                  GeglDitherMethod  channel_dither_method,
                                  gpointer          user_data)
{
  ClayDialogConfig *config   = CLAY_DIALOG_CONFIG (image->clay->config);
  ClayProgress     *progress = user_data;
  const gchar      *enum_desc;
  const Babl       *old_format;
  const Babl       *new_format;
  gint              old_bits;
  gint              new_bits;

  g_object_set (config,
                "image-convert-precision-layer-dither-method",
                layer_dither_method,
                "image-convert-precision-text-layer-dither-method",
                text_layer_dither_method,
                "image-convert-precision-channel-dither-method",
                channel_dither_method,
                NULL);

  /*  we do the same dither method checks here *and* in the dialog,
   *  because the dialog leaves the passed dither methods untouched if
   *  dithering is disabled and passes the original values to the
   *  callback, in order not to change the values saved in
   *  ClayDialogConfig.
   */

  /* random formats with the right precision */
  old_format = clay_image_get_layer_format (image, FALSE);
  new_format = clay_babl_format (CLAY_RGB, precision, FALSE, NULL);

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  if (new_bits >= old_bits ||
      new_bits >  CONVERT_PRECISION_DIALOG_MAX_DITHER_BITS)
    {
      /*  don't dither if we are converting to a higher bit depth,
       *  or to more than MAX_DITHER_BITS.
       */
      layer_dither_method      = GEGL_DITHER_NONE;
      text_layer_dither_method = GEGL_DITHER_NONE;
      channel_dither_method    = GEGL_DITHER_NONE;
    }

  clay_enum_get_value (CLAY_TYPE_PRECISION, precision,
                       NULL, NULL, &enum_desc, NULL);

  progress = clay_progress_start (progress, FALSE,
                                  _("Converting image to %s"),
                                  enum_desc);

  clay_image_convert_precision (image,
                                precision,
                                layer_dither_method,
                                text_layer_dither_method,
                                channel_dither_method,
                                progress);

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_assign_callback (GtkWidget                *dialog,
                               ClayImage                *image,
                               ClayColorProfile         *new_profile,
                               GFile                    *new_file,
                               ClayColorRenderingIntent  intent,
                               gboolean                  bpc,
                               gpointer                  user_data)
{
  GError *error = NULL;

  if (! clay_image_assign_color_profile (image, new_profile, NULL, &error))
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      return;
    }

  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_profile_convert_callback (GtkWidget                *dialog,
                                ClayImage                *image,
                                ClayColorProfile         *new_profile,
                                GFile                    *new_file,
                                ClayColorRenderingIntent  intent,
                                gboolean                  bpc,
                                gpointer                  user_data)
{
  ClayDialogConfig *config   = CLAY_DIALOG_CONFIG (image->clay->config);
  ClayProgress     *progress = user_data;
  GError           *error    = NULL;

  g_object_set (config,
                "image-convert-profile-intent",                   intent,
                "image-convert-profile-black-point-compensation", bpc,
                NULL);

  progress = clay_progress_start (progress, FALSE,
                                  _("Converting to '%s'"),
                                  clay_color_profile_get_label (new_profile));

  if (! clay_image_convert_color_profile (image, new_profile,
                                          config->image_convert_profile_intent,
                                          config->image_convert_profile_bpc,
                                          progress, &error))
    {
      clay_message (image->clay, G_OBJECT (dialog),
                    CLAY_MESSAGE_ERROR,
                    "%s", error->message);
      g_clear_error (&error);

      if (progress)
        clay_progress_end (progress);

      return;
    }

  if (progress)
    clay_progress_end (progress);

  clay_image_flush (image);

  gtk_widget_destroy (dialog);
}

static void
image_resize_callback (GtkWidget    *dialog,
                       ClayViewable *viewable,
                       ClayContext  *context,
                       gint          width,
                       gint          height,
                       ClayUnit     *unit,
                       gint          offset_x,
                       gint          offset_y,
                       gdouble       xres,
                       gdouble       yres,
                       ClayUnit     *res_unit,
                       ClayFillType  fill_type,
                       ClayItemSet   layer_set,
                       gboolean      resize_text_layers,
                       gpointer      user_data)
{
  ClayDisplay *display = user_data;

  image_resize_unit = unit;

  if (width > 0 && height > 0)
    {
      ClayImage        *image  = CLAY_IMAGE (viewable);
      ClayDialogConfig *config = CLAY_DIALOG_CONFIG (image->clay->config);
      ClayProgress     *progress;
      gdouble           old_xres;
      gdouble           old_yres;
      ClayUnit         *old_res_unit;
      gboolean          update_resolution;

      g_object_set (config,
                    "image-resize-fill-type",          fill_type,
                    "image-resize-layer-set",          layer_set,
                    "image-resize-resize-text-layers", resize_text_layers,
                    NULL);

      gtk_widget_destroy (dialog);

      if (width  == clay_image_get_width  (image) &&
          height == clay_image_get_height (image))
        return;

      progress = clay_progress_start (CLAY_PROGRESS (display), FALSE,
                                      _("Resizing"));

      clay_image_get_resolution (image, &old_xres, &old_yres);
      old_res_unit = clay_image_get_unit (image);

      update_resolution = xres     != old_xres ||
                          yres     != old_yres ||
                          res_unit != old_res_unit;

      if (update_resolution)
        {
          clay_image_undo_group_start (image,
                                       CLAY_UNDO_GROUP_IMAGE_SCALE,
                                       _("Change Canvas Size"));
          clay_image_set_resolution (image, xres, yres);
          clay_image_set_unit (image, res_unit);
        }

      clay_image_resize_with_layers (image,
                                     context, fill_type,
                                     width, height,
                                     offset_x, offset_y,
                                     layer_set,
                                     resize_text_layers,
                                     progress);

      if (progress)
        clay_progress_end (progress);

      if (update_resolution)
        clay_image_undo_group_end (image);

      clay_image_flush (image);
    }
  else
    {
      g_warning ("Resize Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_print_size_callback (GtkWidget *dialog,
                           ClayImage *image,
                           gdouble    xresolution,
                           gdouble    yresolution,
                           ClayUnit  *resolution_unit,
                           gpointer   data)
{
  gdouble xres;
  gdouble yres;

  gtk_widget_destroy (dialog);

  clay_image_get_resolution (image, &xres, &yres);

  if (xresolution     == xres &&
      yresolution     == yres &&
      resolution_unit == clay_image_get_unit (image))
    return;

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_SCALE,
                               _("Change Print Size"));

  clay_image_set_resolution (image, xresolution, yresolution);
  clay_image_set_unit (image, resolution_unit);

  clay_image_undo_group_end (image);

  clay_image_flush (image);
}

static void
image_scale_callback (GtkWidget              *dialog,
                      ClayViewable           *viewable,
                      gint                    width,
                      gint                    height,
                      ClayUnit               *unit,
                      ClayInterpolationType   interpolation,
                      gdouble                 xresolution,
                      gdouble                 yresolution,
                      ClayUnit               *resolution_unit,
                      gpointer                user_data)
{
  ClayProgress *progress = user_data;
  ClayImage    *image    = CLAY_IMAGE (viewable);
  gdouble       xres;
  gdouble       yres;

  image_scale_unit   = unit;
  image_scale_interp = interpolation;

  clay_image_get_resolution (image, &xres, &yres);

  if (width > 0 && height > 0)
    {
      gtk_widget_destroy (dialog);

      if (width           == clay_image_get_width  (image) &&
          height          == clay_image_get_height (image) &&
          xresolution     == xres                          &&
          yresolution     == yres                          &&
          resolution_unit == clay_image_get_unit (image))
        return;

      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_IMAGE_SCALE,
                                   _("Scale Image"));

      clay_image_set_resolution (image, xresolution, yresolution);
      clay_image_set_unit (image, resolution_unit);

      if (width  != clay_image_get_width  (image) ||
          height != clay_image_get_height (image))
        {
          progress = clay_progress_start (progress, FALSE,
                                          _("Scaling"));

          clay_image_scale (image, width, height, interpolation, progress);

          if (progress)
            clay_progress_end (progress);
        }

      clay_image_undo_group_end (image);

      clay_image_flush (image);
    }
  else
    {
      g_warning ("Scale Error: "
                 "Both width and height must be greater than zero.");
    }
}

static void
image_merge_layers_callback (GtkWidget     *dialog,
                             ClayImage     *image,
                             ClayContext   *context,
                             ClayMergeType  merge_type,
                             gboolean       merge_active_group,
                             gboolean       discard_invisible,
                             gpointer       user_data)
{
  ClayDialogConfig *config  = CLAY_DIALOG_CONFIG (image->clay->config);
  ClayDisplay      *display = user_data;

  g_object_set (config,
                "layer-merge-type",              merge_type,
                "layer-merge-active-group-only", merge_active_group,
                "layer-merge-discard-invisible", discard_invisible,
                NULL);

  clay_image_merge_visible_layers (image,
                                   context,
                                   config->layer_merge_type,
                                   config->layer_merge_active_group_only,
                                   config->layer_merge_discard_invisible,
                                   CLAY_PROGRESS (display));

  clay_image_flush (image);

  g_clear_pointer (&dialog, gtk_widget_destroy);
}

void
image_softproof_profile_cmd_callback (ClayAction *action,
                                      GVariant   *value,
                                      gpointer    data)
{
  ClayImage        *image;
  ClayDisplayShell *shell;
  GtkWidget        *dialog;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

#define SOFTPROOF_PROFILE_DIALOG_KEY "clay-softproof-profile-dialog"

  dialog = dialogs_get_dialog (G_OBJECT (shell), SOFTPROOF_PROFILE_DIALOG_KEY);

  if (! dialog)
    {
      ClayColorProfile *current_profile;

      current_profile = clay_image_get_simulation_profile (image);

      dialog = color_profile_dialog_new (COLOR_PROFILE_DIALOG_SELECT_SOFTPROOF_PROFILE,
                                         image,
                                         action_data_get_context (data),
                                         GTK_WIDGET (shell),
                                         current_profile,
                                         NULL,
                                         0, 0,
                                         image_softproof_profile_callback,
                                         shell);

      dialogs_attach_dialog (G_OBJECT (shell),
                             SOFTPROOF_PROFILE_DIALOG_KEY, dialog);
    }

  gtk_window_present (GTK_WINDOW (dialog));
}

static void
image_softproof_profile_callback (GtkWidget                *dialog,
                                  ClayImage                *image,
                                  ClayColorProfile         *new_profile,
                                  GFile                    *new_file,
                                  ClayColorRenderingIntent  intent,
                                  gboolean                  bpc,
                                  gpointer                  user_data)
{
  ClayDisplayShell *shell = user_data;

  /* Update image's simulation profile */
  clay_image_set_simulation_profile (image, new_profile);
  clay_color_managed_simulation_profile_changed (CLAY_COLOR_MANAGED (shell));

  gtk_widget_destroy (dialog);
}

void
image_softproof_intent_cmd_callback (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data)
{
  ClayImage                 *image;
  ClayDisplayShell          *shell;
  ClayColorRenderingIntent   intent;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  intent = (ClayColorRenderingIntent) g_variant_get_int32 (value);

  if (intent != clay_image_get_simulation_intent (image))
    {
      clay_image_set_simulation_intent (image, intent);
      shell->color_config_set = TRUE;
      clay_color_managed_simulation_intent_changed (CLAY_COLOR_MANAGED (shell));
    }
}

void
image_softproof_bpc_cmd_callback (ClayAction *action,
                                  GVariant   *value,
                                  gpointer    data)
{
  ClayImage                 *image;
  ClayDisplayShell          *shell;
  gboolean                   bpc;
  return_if_no_image (image, data);
  return_if_no_shell (shell, data);

  bpc = g_variant_get_boolean (value);

  if (bpc != clay_image_get_simulation_bpc (image))
    {
      clay_image_set_simulation_bpc (image, bpc);
      shell->color_config_set = TRUE;
      clay_color_managed_simulation_bpc_changed (CLAY_COLOR_MANAGED (shell));
    }
}
