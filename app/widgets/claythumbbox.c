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
#include "libclaythumb/claythumb.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimagefile.h"
#include "core/clayprogress.h"
#include "core/claysubprogress.h"

#include "plug-in/claypluginmanager-file.h"

#include "clayfiledialog.h" /* eek */
#include "claythumbbox.h"
#include "clayview.h"
#include "clayviewrenderer-frame.h"
#include "claywidgets-utils.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void     clay_thumb_box_progress_iface_init (ClayProgressInterface *iface);

static void     clay_thumb_box_dispose            (GObject           *object);
static void     clay_thumb_box_finalize           (GObject           *object);

static ClayProgress *
                clay_thumb_box_progress_start     (ClayProgress      *progress,
                                                   gboolean           cancellable,
                                                   const gchar       *message);
static void     clay_thumb_box_progress_end       (ClayProgress      *progress);
static gboolean clay_thumb_box_progress_is_active (ClayProgress      *progress);
static void     clay_thumb_box_progress_set_value (ClayProgress      *progress,
                                                   gdouble            percentage);
static gdouble  clay_thumb_box_progress_get_value (ClayProgress      *progress);
static void     clay_thumb_box_progress_pulse     (ClayProgress      *progress);

static gboolean clay_thumb_box_progress_message   (ClayProgress      *progress,
                                                   Clay              *clay,
                                                   ClayMessageSeverity  severity,
                                                   const gchar       *domain,
                                                   const gchar       *message);

static gboolean clay_thumb_box_ebox_button_press  (GtkWidget         *widget,
                                                   GdkEventButton    *bevent,
                                                   ClayThumbBox      *box);
static void clay_thumb_box_thumbnail_clicked      (GtkWidget         *widget,
                                                   GdkModifierType    state,
                                                   ClayThumbBox      *box);
static void clay_thumb_box_imagefile_info_changed (ClayImagefile     *imagefile,
                                                   ClayThumbBox      *box);
static void clay_thumb_box_thumb_state_notify     (ClayThumbnail     *thumb,
                                                   GParamSpec        *pspec,
                                                   ClayThumbBox      *box);
static void clay_thumb_box_create_thumbnails      (ClayThumbBox      *box,
                                                   gboolean           force);
static void clay_thumb_box_create_thumbnail       (ClayThumbBox      *box,
                                                   GFile             *file,
                                                   ClayThumbnailSize  size,
                                                   gboolean           force,
                                                   ClayProgress      *progress);
static gboolean clay_thumb_box_auto_thumbnail     (ClayThumbBox      *box);


G_DEFINE_TYPE_WITH_CODE (ClayThumbBox, clay_thumb_box, GTK_TYPE_FRAME,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_thumb_box_progress_iface_init))

#define parent_class clay_thumb_box_parent_class


static void
clay_thumb_box_class_init (ClayThumbBoxClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose  = clay_thumb_box_dispose;
  object_class->finalize = clay_thumb_box_finalize;

  gtk_widget_class_set_css_name (widget_class, "treeview");
}

static void
clay_thumb_box_init (ClayThumbBox *box)
{
  gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_IN);

  box->idle_id = 0;
}

static void
clay_thumb_box_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start     = clay_thumb_box_progress_start;
  iface->end       = clay_thumb_box_progress_end;
  iface->is_active = clay_thumb_box_progress_is_active;
  iface->set_value = clay_thumb_box_progress_set_value;
  iface->get_value = clay_thumb_box_progress_get_value;
  iface->pulse     = clay_thumb_box_progress_pulse;
  iface->message   = clay_thumb_box_progress_message;
}

static void
clay_thumb_box_dispose (GObject *object)
{
  ClayThumbBox *box = CLAY_THUMB_BOX (object);

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);

  box->progress = NULL;
}

static void
clay_thumb_box_finalize (GObject *object)
{
  ClayThumbBox *box = CLAY_THUMB_BOX (object);

  clay_thumb_box_take_files (box, NULL);

  g_clear_object (&box->imagefile);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static ClayProgress *
clay_thumb_box_progress_start (ClayProgress *progress,
                               gboolean      cancellable,
                               const gchar  *message)
{
  ClayThumbBox *box = CLAY_THUMB_BOX (progress);

  if (! box->progress)
    return NULL;

  if (! box->progress_active)
    {
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);
      GtkWidget      *toplevel;

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = TRUE;

      toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

      if (CLAY_IS_FILE_DIALOG (toplevel))
        gtk_dialog_set_response_sensitive (GTK_DIALOG (toplevel),
                                           GTK_RESPONSE_CANCEL, cancellable);

      return progress;
    }

  return NULL;
}

static void
clay_thumb_box_progress_end (ClayProgress *progress)
{
  if (clay_thumb_box_progress_is_active (progress))
    {
      ClayThumbBox   *box = CLAY_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, 0.0);

      box->progress_active = FALSE;
    }
}

static gboolean
clay_thumb_box_progress_is_active (ClayProgress *progress)
{
  ClayThumbBox *box = CLAY_THUMB_BOX (progress);

  return (box->progress && box->progress_active);
}

static void
clay_thumb_box_progress_set_value (ClayProgress *progress,
                                   gdouble       percentage)
{
  if (clay_thumb_box_progress_is_active (progress))
    {
      ClayThumbBox   *box = CLAY_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_set_fraction (bar, percentage);
    }
}

static gdouble
clay_thumb_box_progress_get_value (ClayProgress *progress)
{
  if (clay_thumb_box_progress_is_active (progress))
    {
      ClayThumbBox   *box = CLAY_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      return gtk_progress_bar_get_fraction (bar);
    }

  return 0.0;
}

static void
clay_thumb_box_progress_pulse (ClayProgress *progress)
{
  if (clay_thumb_box_progress_is_active (progress))
    {
      ClayThumbBox   *box = CLAY_THUMB_BOX (progress);
      GtkProgressBar *bar = GTK_PROGRESS_BAR (box->progress);

      gtk_progress_bar_pulse (bar);
    }
}

static gboolean
clay_thumb_box_progress_message (ClayProgress        *progress,
                                 Clay                *clay,
                                 ClayMessageSeverity  severity,
                                 const gchar         *domain,
                                 const gchar         *message)
{
  /*  ClayThumbBox never shows any messages  */

  return TRUE;
}


/*  stupid ClayHeader class just so we get a "header" CSS node  */

#define CLAY_TYPE_HEADER (clay_header_get_type ())

typedef struct _GtkBox      ClayHeader;
typedef struct _GtkBoxClass ClayHeaderClass;

static GType clay_header_get_type (void);

G_DEFINE_TYPE (ClayHeader, clay_header, GTK_TYPE_BOX)

static void
clay_header_class_init (ClayHeaderClass *klass)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "header");
}

static void
clay_header_init (ClayHeader *header)
{
  gtk_orientable_set_orientation (GTK_ORIENTABLE (header),
                                  GTK_ORIENTATION_VERTICAL);
}


/*  public functions  */

GtkWidget *
clay_thumb_box_new (ClayContext *context)
{
  ClayThumbBox   *box;
  GtkWidget      *vbox;
  GtkWidget      *vbox2;
  GtkWidget      *ebox;
  GtkWidget      *button;
  GtkWidget      *label;
  gchar          *str;
  gint            h, v;

  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);

  box = g_object_new (CLAY_TYPE_THUMB_BOX, NULL);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (box)),
                               GTK_STYLE_CLASS_VIEW);

  box->context = context;

  ebox = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (box), ebox);
  gtk_widget_set_visible (ebox, TRUE);

  g_signal_connect (ebox, "button-press-event",
                    G_CALLBACK (clay_thumb_box_ebox_button_press),
                    box);

  str = g_strdup_printf (_("Click to update preview\n"
                           "%s-Click to force update even "
                           "if preview is up-to-date"),
                         clay_get_mod_string (clay_get_toggle_behavior_mask ()));

  clay_help_set_help_data (ebox, str, NULL);

  g_free (str);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (ebox), vbox);
  gtk_widget_set_visible (vbox, TRUE);

  vbox2 = g_object_new (CLAY_TYPE_HEADER, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);
  gtk_widget_set_visible (vbox2, TRUE);

  button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_set_visible (button, TRUE);

  label = gtk_label_new_with_mnemonic (_("Pr_eview"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_container_add (GTK_CONTAINER (button), label);
  gtk_widget_set_visible (label, TRUE);

  g_signal_connect (button, "button-press-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "button-release-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "enter-notify-event",
                    G_CALLBACK (gtk_true),
                    NULL);
  g_signal_connect (button, "leave-notify-event",
                    G_CALLBACK (gtk_true),
                    NULL);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 4);
  gtk_box_pack_start (GTK_BOX (vbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_set_visible (vbox2, TRUE);

  box->imagefile = clay_imagefile_new (context->clay, NULL);

  g_signal_connect (box->imagefile, "info-changed",
                    G_CALLBACK (clay_thumb_box_imagefile_info_changed),
                    box);

  g_signal_connect (clay_imagefile_get_thumbnail (box->imagefile),
                    "notify::thumb-state",
                    G_CALLBACK (clay_thumb_box_thumb_state_notify),
                    box);

  clay_view_renderer_get_frame_size (&h, &v);

  box->preview = clay_view_new (context,
                                CLAY_VIEWABLE (box->imagefile),
                                /* add padding for the shadow frame */
                                context->clay->config->thumbnail_size +
                                MAX (h, v),
                                0, FALSE);

  gtk_style_context_add_class (gtk_widget_get_style_context (box->preview),
                               GTK_STYLE_CLASS_VIEW);
  gtk_box_pack_start (GTK_BOX (vbox2), box->preview, FALSE, FALSE, 0);
  gtk_widget_set_visible (box->preview, TRUE);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), box->preview);

  g_signal_connect (box->preview, "clicked",
                    G_CALLBACK (clay_thumb_box_thumbnail_clicked),
                    box);

  box->filename = gtk_label_new (_("No selection"));
  gtk_label_set_max_width_chars (GTK_LABEL (box->filename), 1);
  gtk_label_set_ellipsize (GTK_LABEL (box->filename), PANGO_ELLIPSIZE_MIDDLE);
  gtk_label_set_justify (GTK_LABEL (box->filename), GTK_JUSTIFY_CENTER);
  clay_label_set_attributes (GTK_LABEL (box->filename),
                             PANGO_ATTR_STYLE, PANGO_STYLE_OBLIQUE,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->filename, FALSE, FALSE, 0);
  gtk_widget_set_visible (box->filename, TRUE);

  box->info = gtk_label_new (" \n \n \n ");
  gtk_label_set_justify (GTK_LABEL (box->info), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (box->info), TRUE);
  clay_label_set_attributes (GTK_LABEL (box->info),
                             PANGO_ATTR_SCALE, PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox2), box->info, FALSE, FALSE, 0);
  gtk_widget_set_visible (box->info, TRUE);

  box->progress = gtk_progress_bar_new ();
  gtk_widget_set_halign (box->progress, GTK_ALIGN_FILL);
  gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (box->progress), TRUE);
  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "Fog");
  gtk_box_pack_end (GTK_BOX (vbox2), box->progress, FALSE, FALSE, 0);
  /* don't gtk_widget_set_visible (box->progress, TRUE); */

  gtk_widget_set_size_request (GTK_WIDGET (box),
                               MAX ((gint) CLAY_THUMB_SIZE_NORMAL,
                                    (gint) context->clay->config->thumbnail_size) +
                               2 * MAX (h, v),
                               -1);

  return GTK_WIDGET (box);
}

void
clay_thumb_box_take_file (ClayThumbBox *box,
                          GFile        *file)
{
  g_return_if_fail (CLAY_IS_THUMB_BOX (box));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  if (box->idle_id)
    {
      g_source_remove (box->idle_id);
      box->idle_id = 0;
    }

  clay_imagefile_set_file (box->imagefile, file);

  if (file)
    {
      GFileInfo *info;
      gchar     *basename;

      info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                0, NULL, NULL);

      if (info != NULL)
        basename =
          g_file_info_get_attribute_as_string (info,
                                               G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
      else
        basename = g_path_get_basename (clay_file_get_utf8_name (file));

      gtk_label_set_text (GTK_LABEL (box->filename), basename);
      g_free (basename);
      g_clear_object (&info);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (box->filename), _("No selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (box), file != NULL);
  clay_imagefile_update (box->imagefile);
}

void
clay_thumb_box_take_files (ClayThumbBox *box,
                           GSList       *files)
{
  g_return_if_fail (CLAY_IS_THUMB_BOX (box));

  if (box->files)
    {
      g_slist_free_full (box->files, (GDestroyNotify) g_object_unref);
      box->files = NULL;
    }

  box->files = files;
}


/*  private functions  */

static gboolean
clay_thumb_box_ebox_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent,
                                  ClayThumbBox   *box)
{
  clay_thumb_box_thumbnail_clicked (widget, bevent->state, box);

  return TRUE;
}

static void
clay_thumb_box_thumbnail_clicked (GtkWidget       *widget,
                                  GdkModifierType  state,
                                  ClayThumbBox    *box)
{
  clay_thumb_box_create_thumbnails (box,
                                    (state & clay_get_toggle_behavior_mask ()) ?
                                    TRUE : FALSE);
}

static void
clay_thumb_box_imagefile_info_changed (ClayImagefile *imagefile,
                                       ClayThumbBox  *box)
{
  gtk_label_set_text (GTK_LABEL (box->info),
                      clay_imagefile_get_desc_string (imagefile));
}

static void
clay_thumb_box_thumb_state_notify (ClayThumbnail *thumb,
                                   GParamSpec    *pspec,
                                   ClayThumbBox  *box)
{
  ClayThumbState image_state;
  ClayThumbState thumb_state;

  if (box->idle_id)
    return;

  g_object_get (thumb,
                "image-state", &image_state,
                "thumb-state", &thumb_state,
                NULL);

  if (image_state == CLAY_THUMB_STATE_REMOTE)
    return;

  switch (thumb_state)
    {
    case CLAY_THUMB_STATE_NOT_FOUND:
    case CLAY_THUMB_STATE_OLD:
      box->idle_id =
        g_idle_add_full (G_PRIORITY_LOW,
                         (GSourceFunc) clay_thumb_box_auto_thumbnail,
                         box, NULL);
      break;

    default:
      break;
    }
}

static void
clay_thumb_box_create_thumbnails (ClayThumbBox *box,
                                  gboolean      force)
{
  Clay           *clay     = box->context->clay;
  ClayProgress   *progress = CLAY_PROGRESS (box);
  ClayFileDialog *dialog   = NULL;
  GtkWidget      *toplevel;
  GSList         *list;
  gint            n_files;
  gint            i;

  if (clay->config->thumbnail_size == CLAY_THUMBNAIL_SIZE_NONE)
    return;

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (box));

  if (CLAY_IS_FILE_DIALOG (toplevel))
    dialog = CLAY_FILE_DIALOG (toplevel);

  clay_set_busy (clay);

  if (dialog)
    clay_file_dialog_set_sensitive (dialog, FALSE);
  else
    gtk_widget_set_sensitive (toplevel, FALSE);

  if (box->files)
    {
      gtk_widget_set_visible (box->info, FALSE);
      gtk_widget_set_visible (box->progress, TRUE);
    }

  n_files = g_slist_length (box->files);

  if (n_files > 1)
    {
      gchar *str;

      clay_progress_start (CLAY_PROGRESS (box), TRUE, "%s", "");

      progress = clay_sub_progress_new (CLAY_PROGRESS (box));

      clay_sub_progress_set_step (CLAY_SUB_PROGRESS (progress), 0, n_files);

      for (list = box->files->next, i = 1;
           list;
           list = g_slist_next (list), i++)
        {
          str = g_strdup_printf (_("Thumbnail %d of %d"), i, n_files);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
          g_free (str);

          clay_progress_set_value (progress, 0.0);

          while (g_main_context_pending (NULL))
            g_main_context_iteration (NULL, FALSE);

          clay_thumb_box_create_thumbnail (box,
                                           list->data,
                                           clay->config->thumbnail_size,
                                           force,
                                           progress);

          if (dialog && dialog->canceled)
            goto canceled;

          clay_sub_progress_set_step (CLAY_SUB_PROGRESS (progress), i, n_files);
        }

      str = g_strdup_printf (_("Thumbnail %d of %d"), n_files, n_files);
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), str);
      g_free (str);

      clay_progress_set_value (progress, 0.0);

      while (g_main_context_pending (NULL))
        g_main_context_iteration (NULL, FALSE);
    }

  if (box->files)
    {
      clay_thumb_box_create_thumbnail (box,
                                       box->files->data,
                                       clay->config->thumbnail_size,
                                       force,
                                       progress);

      clay_progress_set_value (progress, 1.0);
    }

 canceled:

  if (n_files > 1)
    {
      g_object_unref (progress);

      clay_progress_end (CLAY_PROGRESS (box));
      gtk_progress_bar_set_text (GTK_PROGRESS_BAR (box->progress), "");
    }

  if (box->files)
    {
      gtk_widget_set_visible (box->progress, FALSE);
      gtk_widget_set_visible (box->info, TRUE);
    }

  if (dialog)
    clay_file_dialog_set_sensitive (dialog, TRUE);
  else
    gtk_widget_set_sensitive (toplevel, TRUE);

  clay_unset_busy (clay);
}

static void
clay_thumb_box_create_thumbnail (ClayThumbBox      *box,
                                 GFile             *file,
                                 ClayThumbnailSize  size,
                                 gboolean           force,
                                 ClayProgress      *progress)
{
  ClayThumbnail *thumb = clay_imagefile_get_thumbnail (box->imagefile);
  GFileInfo     *info;
  gchar         *basename;

  info = g_file_query_info (file,
                                G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                                0, NULL, NULL);

  if (info != NULL)
    basename =
      g_file_info_get_attribute_as_string (info,
                                           G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME);
  else
    basename = g_path_get_basename (clay_file_get_utf8_name (file));

  gtk_label_set_text (GTK_LABEL (box->filename), basename);
  g_free (basename);
  g_clear_object (&info);

  clay_imagefile_set_file (box->imagefile, file);

  if (force ||
      (clay_thumbnail_peek_thumb (thumb, (ClayThumbSize) size) < CLAY_THUMB_STATE_FAILED &&
       ! clay_thumbnail_has_failed (thumb)))
    {
      GError *error = NULL;

      if (! clay_imagefile_create_thumbnail (box->imagefile, box->context,
                                             progress,
                                             size, ! force, &error))
        {
          clay_message_literal (box->context->clay,
                                G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}

static gboolean
clay_thumb_box_auto_thumbnail (ClayThumbBox *box)
{
  Clay          *clay  = box->context->clay;
  ClayThumbnail *thumb = clay_imagefile_get_thumbnail (box->imagefile);
  GFile         *file  = clay_imagefile_get_file (box->imagefile);
  ClayThumbState image_state;
  ClayThumbState thumb_state;
  gint64         image_filesize;

  g_object_get (thumb,
                "image-state",    &image_state,
                "thumb-state",    &thumb_state,
                "image-filesize", &image_filesize,
                NULL);

  box->idle_id = 0;

  if (image_state == CLAY_THUMB_STATE_NOT_FOUND)
    return FALSE;

  switch (thumb_state)
    {
    case CLAY_THUMB_STATE_NOT_FOUND:
    case CLAY_THUMB_STATE_OLD:
      if (image_filesize < clay->config->thumbnail_filesize_limit &&
          ! clay_thumbnail_has_failed (thumb)                            &&
          clay_plug_in_manager_file_procedure_find_by_extension (clay->plug_in_manager,
                                                                 CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                                 file))
        {
          if (image_filesize > 0)
            {
              gchar *size;
              gchar *text;

              size = g_format_size (image_filesize);
              text = g_strdup_printf ("%s\n%s",
                                      size, _("Creating preview..."));

              gtk_label_set_text (GTK_LABEL (box->info), text);

              g_free (text);
              g_free (size);
            }
          else
            {
              gtk_label_set_text (GTK_LABEL (box->info),
                                  _("Creating preview..."));
            }

          clay_imagefile_create_thumbnail_weak (box->imagefile, box->context,
                                                CLAY_PROGRESS (box),
                                                clay->config->thumbnail_size,
                                                TRUE);
        }
      break;

    default:
      break;
    }

  return FALSE;
}
