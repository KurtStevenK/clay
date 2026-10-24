/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayextensiondetails.c
 * Copyright (C) 2018 Jehan <jehan@gimp.org>
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

#include "core/clayextension.h"
#include "core/clayextensionmanager.h"

#include "clayextensiondetails.h"

#include "clay-intl.h"

struct _ClayExtensionDetailsPrivate
{
  ClayExtension  *extension;
  ClayStoreEntry *entry;

  gint            screenshot_width;
  gint            screenshot_height;
};

static void clay_extension_details_size_allocate (ClayExtensionDetails *details,
                                                  GdkRectangle         *allocation,
                                                  GtkGrid              *data);

G_DEFINE_TYPE_WITH_PRIVATE (ClayExtensionDetails, clay_extension_details,
                            GTK_TYPE_FRAME)

#define parent_class clay_extension_details_parent_class


static void
clay_extension_details_class_init (ClayExtensionDetailsClass *klass)
{
}

static void
clay_extension_details_init (ClayExtensionDetails *details)
{
  gtk_frame_set_label_align (GTK_FRAME (details), 0.5, 1.0);
  details->p = clay_extension_details_get_instance_private (details);
}

GtkWidget *
clay_extension_details_new (void)
{
  return g_object_new (CLAY_TYPE_EXTENSION_DETAILS, NULL);
}

void
clay_extension_details_set (ClayExtensionDetails *details,
                            ClayExtension        *extension)
{
  GtkWidget     *grid;
  GtkWidget     *widget;
  GtkTextBuffer *buffer;

  g_return_if_fail (CLAY_IS_EXTENSION (extension));

  if (details->p->extension)
    g_object_unref (details->p->extension);
  details->p->extension  = g_object_ref (extension);
  details->p->entry = NULL;
  details->p->screenshot_width = details->p->screenshot_height = 0;

  gtk_container_foreach (GTK_CONTAINER (details),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gtk_frame_set_label (GTK_FRAME (details),
                       clay_extension_get_name (extension));

  grid = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (grid), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (grid), FALSE);
  gtk_container_add (GTK_CONTAINER (details), grid);
  gtk_widget_set_visible (grid, TRUE);

  if (gtk_widget_get_realized (GTK_WIDGET (details)))
    {
      GtkAllocation allocation;
      gint          baseline   = 0;

      gtk_widget_get_allocated_size (GTK_WIDGET (details),
                                     &allocation, &baseline);
      clay_extension_details_size_allocate (details, &allocation, GTK_GRID (grid));
    }
  g_signal_connect (details, "size-allocate",
                    G_CALLBACK (clay_extension_details_size_allocate),
                    grid);

  {
    gchar       *desc;
    GtkTextIter  iter;

    desc = clay_extension_get_markup_description (extension);
    widget = gtk_text_view_new ();
    gtk_widget_set_vexpand (widget, TRUE);
    gtk_widget_set_hexpand (widget, TRUE);
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
    gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (widget), FALSE);
    gtk_text_view_set_justification (GTK_TEXT_VIEW (widget), GTK_JUSTIFY_FILL);
    gtk_grid_attach (GTK_GRID (grid), widget, 0, 1, 3, 3);
    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
    gtk_text_buffer_get_start_iter (buffer, &iter);
    if (desc)
      {
        gtk_text_buffer_insert_markup (buffer, &iter, desc, -1);
        g_free (desc);
      }
    gtk_widget_set_visible (widget, TRUE);
  }
}

void
clay_extension_details_set_store (ClayExtensionDetails *details,
                                  ClayStoreEntry       *entry)
{
  GtkWidget *grid;
  GtkWidget *label;
  gchar     *markup;

  g_return_if_fail (entry != NULL);

  if (details->p->extension)
    {
      g_object_unref (details->p->extension);
      details->p->extension = NULL;
    }
  details->p->entry = entry;

  gtk_container_foreach (GTK_CONTAINER (details),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  gtk_frame_set_label (GTK_FRAME (details),
                       entry->name ? entry->name : entry->id);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 8);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_container_add (GTK_CONTAINER (details), grid);
  gtk_widget_set_visible (grid, TRUE);

  markup = g_markup_printf_escaped ("<span size='x-large'><b>%s</b></span>",
                                    entry->name ? entry->name : entry->id);
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 2, 1);
  gtk_widget_set_visible (label, TRUE);
  g_free (markup);

  markup = g_strdup_printf ("%s%s%s%s%s",
                            entry->author ? entry->author : "",
                            entry->author && entry->version ? " · " : "",
                            entry->version ? entry->version : "",
                            (entry->author || entry->version) && entry->category ? " · " : "",
                            entry->category ? entry->category : "");
  label = gtk_label_new (markup);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 2, 1);
  gtk_widget_set_visible (label, TRUE);
  g_free (markup);

  if (entry->menu_hint)
    {
      markup = g_markup_printf_escaped (_("<b>Find it in Clay:</b> %s"),
                                        entry->menu_hint);
      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 2, 1);
      gtk_widget_set_visible (label, TRUE);
      g_free (markup);
    }

  if (entry->description)
    {
      GtkWidget *view = gtk_text_view_new ();
      GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_WORD_CHAR);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
      gtk_text_buffer_set_text (buffer, entry->description, -1);
      gtk_widget_set_vexpand (view, TRUE);
      gtk_widget_set_hexpand (view, TRUE);
      gtk_grid_attach (GTK_GRID (grid), view, 0, 3, 2, 1);
      gtk_widget_set_visible (view, TRUE);
    }
}

static void
clay_extension_details_size_allocate (ClayExtensionDetails *details,
                                      GdkRectangle         *allocation,
                                      GtkGrid              *grid)
{
  GtkWidget *widget;

  if (! details->p->extension)
    return;

  if (allocation->width > 0 && allocation->height > 0 &&
      allocation->width * 2 / 3 != details->p->screenshot_width &&
      allocation->height * 2 / 3 != details->p->screenshot_height)
    {
      GdkPixbuf *pixbuf;

      details->p->screenshot_width  = allocation->width * 2 / 3;
      details->p->screenshot_height = allocation->height * 2 / 3;

      pixbuf = clay_extension_get_screenshot (details->p->extension,
                                              details->p->screenshot_width, details->p->screenshot_height,
                                              NULL);
      if (pixbuf)
        {
          widget = gtk_image_new_from_pixbuf (pixbuf);
          gtk_widget_set_vexpand (widget, FALSE);
          gtk_widget_set_hexpand (widget, FALSE);
          gtk_grid_attach (grid, widget, 1, 0, 1, 1);
          gtk_widget_set_visible (widget, TRUE);

          g_object_unref (pixbuf);
        }
    }
}
