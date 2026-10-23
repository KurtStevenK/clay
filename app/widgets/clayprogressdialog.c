/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayprogressdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clayprogress.h"

#include "clayprogressbox.h"
#include "clayprogressdialog.h"

#include "clay-intl.h"


#define PROGRESS_DIALOG_WIDTH  400


static void    clay_progress_dialog_progress_iface_init (ClayProgressInterface *iface);

static void     clay_progress_dialog_response           (GtkDialog    *dialog,
                                                         gint          response_id);

static ClayProgress *
                clay_progress_dialog_progress_start     (ClayProgress *progress,
                                                         gboolean      cancellable,
                                                         const gchar  *message);
static void     clay_progress_dialog_progress_end       (ClayProgress *progress);
static gboolean clay_progress_dialog_progress_is_active (ClayProgress *progress);
static void     clay_progress_dialog_progress_set_text  (ClayProgress *progress,
                                                         const gchar  *message);
static void     clay_progress_dialog_progress_set_value (ClayProgress *progress,
                                                         gdouble       percentage);
static gdouble  clay_progress_dialog_progress_get_value (ClayProgress *progress);
static void     clay_progress_dialog_progress_pulse     (ClayProgress *progress);


G_DEFINE_TYPE_WITH_CODE (ClayProgressDialog, clay_progress_dialog,
                         CLAY_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROGRESS,
                                                clay_progress_dialog_progress_iface_init))

#define parent_class clay_progress_dialog_parent_class


static void
clay_progress_dialog_class_init (ClayProgressDialogClass *klass)
{
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  dialog_class->response = clay_progress_dialog_response;
}

static void
clay_progress_dialog_init (ClayProgressDialog *dialog)
{
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  dialog->box = clay_progress_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (dialog->box), 12);
  gtk_box_pack_start (GTK_BOX (content_area), dialog->box, TRUE, TRUE, 0);
  gtk_widget_set_visible (dialog->box, TRUE);

  g_signal_connect (dialog->box, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &dialog->box);

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_Cancel"), GTK_RESPONSE_CANCEL);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

  gtk_widget_set_size_request (GTK_WIDGET (dialog), PROGRESS_DIALOG_WIDTH, -1);
}

static void
clay_progress_dialog_progress_iface_init (ClayProgressInterface *iface)
{
  iface->start     = clay_progress_dialog_progress_start;
  iface->end       = clay_progress_dialog_progress_end;
  iface->is_active = clay_progress_dialog_progress_is_active;
  iface->set_text  = clay_progress_dialog_progress_set_text;
  iface->set_value = clay_progress_dialog_progress_set_value;
  iface->get_value = clay_progress_dialog_progress_get_value;
  iface->pulse     = clay_progress_dialog_progress_pulse;
}

static void
clay_progress_dialog_response (GtkDialog *dialog,
                               gint       response_id)
{
  ClayProgressDialog *progress_dialog = CLAY_PROGRESS_DIALOG (dialog);

  if (CLAY_PROGRESS_BOX (progress_dialog->box)->cancellable)
    clay_progress_cancel (CLAY_PROGRESS (dialog));
}

static ClayProgress *
clay_progress_dialog_progress_start (ClayProgress *progress,
                                     gboolean      cancellable,
                                     const gchar  *message)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return NULL;

  if (clay_progress_start (CLAY_PROGRESS (dialog->box), cancellable,
                           "%s", message))
    {
      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, cancellable);

      gtk_window_present (GTK_WINDOW (dialog));

      return progress;
    }

  return NULL;
}

static void
clay_progress_dialog_progress_end (ClayProgress *progress)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  if (CLAY_PROGRESS_BOX (dialog->box)->active)
    {
      clay_progress_end (CLAY_PROGRESS (dialog->box));

      gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                         GTK_RESPONSE_CANCEL, FALSE);

      gtk_widget_set_visible (GTK_WIDGET (dialog), FALSE);
    }
}

static gboolean
clay_progress_dialog_progress_is_active (ClayProgress *progress)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return FALSE;

  return clay_progress_is_active (CLAY_PROGRESS (dialog->box));
}

static void
clay_progress_dialog_progress_set_text (ClayProgress *progress,
                                        const gchar  *message)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  clay_progress_set_text_literal (CLAY_PROGRESS (dialog->box), message);
}

static void
clay_progress_dialog_progress_set_value (ClayProgress *progress,
                                         gdouble       percentage)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  clay_progress_set_value (CLAY_PROGRESS (dialog->box), percentage);
}

static gdouble
clay_progress_dialog_progress_get_value (ClayProgress *progress)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return 0.0;

  return clay_progress_get_value (CLAY_PROGRESS (dialog->box));
}

static void
clay_progress_dialog_progress_pulse (ClayProgress *progress)
{
  ClayProgressDialog *dialog = CLAY_PROGRESS_DIALOG (progress);

  if (! dialog->box)
    return;

  clay_progress_pulse (CLAY_PROGRESS (dialog->box));
}

GtkWidget *
clay_progress_dialog_new (void)
{
  gboolean use_header_bar;

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  return g_object_new (CLAY_TYPE_PROGRESS_DIALOG,
                       "title",             _("Progress"),
                       "role",              "progress",
                       "skip-taskbar-hint", TRUE,
                       "skip-pager-hint",   TRUE,
                       "resizable",         FALSE,
                       "focus-on-map",      FALSE,
                       "window-position",   GTK_WIN_POS_CENTER,
                       "use-header-bar",    use_header_bar,
                       NULL);
}
