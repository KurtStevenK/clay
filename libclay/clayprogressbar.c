/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayprogressbar.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "clayuitypes.h"

#include "clay.h"

#include "clayprogressbar.h"


/**
 * SECTION: clayprogressbar
 * @title: ClayProgressBar
 * @short_description: A widget providing a progress bar.
 *
 * A widget providing a progress bar that automatically redirects any
 * progress calls to itself.
 **/


struct _ClayProgressBar
{
  GtkProgressBar  parent_instance;

  const gchar    *progress_callback;
  GBytes         *window_handle;
};


static void     clay_progress_bar_dispose           (GObject     *object);

static void     clay_progress_bar_start             (const gchar *message,
                                                     gboolean     cancelable,
                                                     gpointer     user_data);
static void     clay_progress_bar_end               (gpointer     user_data);
static void     clay_progress_bar_set_text          (const gchar *message,
                                                     gpointer     user_data);
static void     clay_progress_bar_set_value         (gdouble      percentage,
                                                     gpointer     user_data);
static void     clay_progress_bar_pulse             (gpointer     user_data);
static GBytes * clay_progress_bar_get_window_handle (gpointer     user_data);


G_DEFINE_TYPE (ClayProgressBar, clay_progress_bar, GTK_TYPE_PROGRESS_BAR)

#define parent_class clay_progress_bar_parent_class


static void
clay_progress_bar_class_init (ClayProgressBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clay_progress_bar_dispose;
}

static void
clay_progress_bar_init (ClayProgressBar *bar)
{
  ClayProgressVtable vtable = { 0, };

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");
  gtk_progress_bar_set_ellipsize (GTK_PROGRESS_BAR (bar), PANGO_ELLIPSIZE_END);

  vtable.start             = clay_progress_bar_start;
  vtable.end               = clay_progress_bar_end;
  vtable.set_text          = clay_progress_bar_set_text;
  vtable.set_value         = clay_progress_bar_set_value;
  vtable.pulse             = clay_progress_bar_pulse;
  vtable.get_window_handle = clay_progress_bar_get_window_handle;

  bar->progress_callback = clay_progress_install_vtable (&vtable, bar, NULL);

  clay_widget_set_native_handle (GTK_WIDGET (bar), &bar->window_handle);
}

static void
clay_progress_bar_dispose (GObject *object)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (object);

  if (bar->progress_callback)
    {
      clay_progress_uninstall (bar->progress_callback);
      bar->progress_callback = NULL;
    }

  clay_widget_free_native_handle (GTK_WIDGET (bar), &bar->window_handle);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_progress_bar_start (const gchar *message,
                         gboolean     cancelable,
                         gpointer     user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
clay_progress_bar_end (gpointer user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), " ");
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), 0.0);

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
clay_progress_bar_set_text (const gchar *message,
                            gpointer     user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  gtk_progress_bar_set_text (GTK_PROGRESS_BAR (bar), message ? message : " ");

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
clay_progress_bar_set_value (gdouble  percentage,
                             gpointer user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  if (percentage >= 0.0)
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (bar), percentage);
  else
    gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static void
clay_progress_bar_pulse (gpointer user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (bar));

  if (gtk_widget_is_drawable (GTK_WIDGET (bar)))
    while (g_main_context_pending (NULL))
      g_main_context_iteration (NULL, TRUE);
}

static GBytes *
clay_progress_bar_get_window_handle (gpointer user_data)
{
  ClayProgressBar *bar = CLAY_PROGRESS_BAR (user_data);

  return g_bytes_ref (bar->window_handle);
}

/**
 * clay_progress_bar_new:
 *
 * Creates a new #ClayProgressBar widget.
 *
 * Returns: the new widget.
 *
 * Since: 2.2
 **/
GtkWidget *
clay_progress_bar_new (void)
{
  return g_object_new (CLAY_TYPE_PROGRESS_BAR, NULL);
}
