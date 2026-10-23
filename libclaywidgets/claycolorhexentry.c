/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorhexentry.c
 * Copyright (C) 2004  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaycolor/claycolor.h"

#include "claywidgetstypes.h"

#include "claycellrenderercolor.h"
#include "claycolorhexentry.h"
#include "clayhelpui.h"

#include "libclay/libclay-intl.h"


/**
 * SECTION: claycolorhexentry
 * @title: ClayColorHexEntry
 * @short_description: Widget for entering a color's hex triplet.
 *
 * Widget for entering a color's hex triplet. The syntax follows CSS and
 * SVG specifications, which means that only sRGB colors are supported.
 **/


enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  COLUMN_NAME,
  COLUMN_COLOR,
  NUM_COLUMNS
};


struct _ClayColorHexEntry
{
  GtkEntry   parent_instance;

  GeglColor *color;
};


static void      clay_color_hex_entry_constructed (GObject            *object);
static void      clay_color_hex_entry_finalize    (GObject            *object);

static gboolean  clay_color_hex_entry_events      (GtkWidget          *widget,
                                                   GdkEvent           *event);

static gboolean  clay_color_hex_entry_matched     (GtkEntryCompletion *completion,
                                                   GtkTreeModel       *model,
                                                   GtkTreeIter        *iter,
                                                   ClayColorHexEntry  *entry);


G_DEFINE_TYPE (ClayColorHexEntry, clay_color_hex_entry, GTK_TYPE_ENTRY)

#define parent_class clay_color_hex_entry_parent_class

static guint entry_signals[LAST_SIGNAL] = { 0 };


static void
clay_color_hex_entry_class_init (ClayColorHexEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  entry_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed = clay_color_hex_entry_constructed;
  object_class->finalize    = clay_color_hex_entry_finalize;
}

static void
clay_color_hex_entry_init (ClayColorHexEntry *entry)
{
  GtkEntryCompletion  *completion;
  GtkCellRenderer     *cell;
  GtkListStore        *store;
  GeglColor          **colors;
  const gchar        **names;
  gint                 i;

  /* GtkEntry's minimum size is way too large, set a reasonable one
   * for our use case
   */
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);

  clay_help_set_help_data (GTK_WIDGET (entry),
                           _("Hexadecimal color notation as used in HTML and "
                             "CSS.  This entry also accepts CSS color names."),
                           NULL);

  entry->color = gegl_color_new ("black");

  store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, GEGL_TYPE_COLOR);

  names = clay_color_list_names (&colors);

  for (i = 0; names[i] != NULL; i++)
    {
      GtkTreeIter  iter;
      GeglColor   *named_color = colors[i];

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          COLUMN_NAME,  names[i],
                          COLUMN_COLOR, named_color,
                          -1);
    }

  clay_color_array_free (colors);
  g_free (names);

  completion = g_object_new (GTK_TYPE_ENTRY_COMPLETION,
                             "model", store,
                             NULL);
  g_object_unref (store);

  cell = clay_cell_renderer_color_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), cell,
                                  "color", COLUMN_COLOR,
                                  NULL);

  gtk_entry_completion_set_text_column (completion, COLUMN_NAME);

  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  g_object_unref (completion);

  g_signal_connect_object (entry, "focus-out-event",
                           G_CALLBACK (clay_color_hex_entry_events),
                           NULL, 0);
  g_signal_connect_object (entry, "key-press-event",
                           G_CALLBACK (clay_color_hex_entry_events),
                           NULL, 0);
  g_signal_connect_object (entry, "key-release-event",
                           G_CALLBACK (clay_color_hex_entry_events),
                           NULL, 0);

  g_signal_connect_object (completion, "match-selected",
                           G_CALLBACK (clay_color_hex_entry_matched),
                           entry, 0);
}

static void
clay_color_hex_entry_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_entry_set_text (GTK_ENTRY (object), "000000");
}

static void
clay_color_hex_entry_finalize (GObject *object)
{
  ClayColorHexEntry *entry = CLAY_COLOR_HEX_ENTRY (object);

  g_object_unref (entry->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * clay_color_hex_entry_new:
 *
 * Returns: a new #ClayColorHexEntry widget
 *
 * Since: 2.2
 **/
GtkWidget *
clay_color_hex_entry_new (void)
{
  return g_object_new (CLAY_TYPE_COLOR_HEX_ENTRY, NULL);
}

/**
 * clay_color_hex_entry_set_color:
 * @entry: a #ClayColorHexEntry widget
 * @color: the color to set.
 *
 * Sets the color displayed by a #ClayColorHexEntry. If the new color
 * is different to the previously set color, the "color-changed"
 * signal is emitted.
 *
 * Since: 2.2
 **/
void
clay_color_hex_entry_set_color (ClayColorHexEntry *entry,
                                GeglColor         *color)
{
  gchar  buffer[8];
  guchar rgb[3];

  g_return_if_fail (CLAY_IS_COLOR_HEX_ENTRY (entry));
  g_return_if_fail (GEGL_IS_COLOR (color));

  g_object_unref (entry->color);
  entry->color = gegl_color_duplicate (color);

  gegl_color_get_pixel (color, babl_format ("R'G'B' u8"), rgb);
  g_snprintf (buffer, sizeof (buffer), "%.2x%.2x%.2x", rgb[0], rgb[1], rgb[2]);

  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  /* move cursor to the end */
  gtk_editable_set_position (GTK_EDITABLE (entry), -1);

  g_signal_emit (entry, entry_signals[COLOR_CHANGED], 0);
}

/**
 * clay_color_hex_entry_get_color:
 * @entry: a #ClayColorHexEntry widget
 *
 * Retrieves the color value displayed by a #ClayColorHexEntry.
 *
 * Returns: (transfer full): the color stored in @entry.
 *
 * Since: 2.2
 **/
GeglColor *
clay_color_hex_entry_get_color (ClayColorHexEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_COLOR_HEX_ENTRY (entry), NULL);

  return gegl_color_duplicate (entry->color);
}

static gboolean
clay_color_hex_entry_events (GtkWidget *widget,
                             GdkEvent  *event)
{
  ClayColorHexEntry *entry       = CLAY_COLOR_HEX_ENTRY (widget);
  gboolean           check_color = FALSE;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval == GDK_KEY_Return   ||
            kevent->keyval == GDK_KEY_KP_Enter ||
            kevent->keyval == GDK_KEY_ISO_Enter)
          check_color = TRUE;
      }

    case GDK_KEY_RELEASE:
    case GDK_FOCUS_CHANGE:
      {
        const gchar *text;
        gchar        buffer[8];
        guchar       rgb[3];
        gsize        len;

        text = gtk_entry_get_text (GTK_ENTRY (widget));
        len  = strlen (text);

        if (len >= 6 || check_color)
          {
            gegl_color_get_pixel (entry->color, babl_format ("R'G'B' u8"),
                                  rgb);
            g_snprintf (buffer, sizeof (buffer), "%.2x%.2x%.2x",
                        rgb[0], rgb[1], rgb[2]);

            if (g_ascii_strcasecmp (buffer, text) != 0)
              {
                GeglColor *color = NULL;
                gint       position;

                position = gtk_editable_get_position (GTK_EDITABLE (widget));

                if (len > 0 &&
                    ((color = clay_color_parse_hex_substring (text, len)) ||
                     (color = clay_color_parse_name (text))))
                  {
                    clay_color_hex_entry_set_color (entry, color);
                    g_object_unref (color);

                    if (! check_color)
                      gtk_editable_set_position (GTK_EDITABLE (entry),
                                                 position);
                  }
                else if (gdk_event_get_event_type (event) == GDK_FOCUS_CHANGE || check_color)
                  {
                    gtk_entry_set_text (GTK_ENTRY (entry), buffer);
                  }
              }
          }
      }
      break;

    default:
      /*  do nothing  */
      break;
    }

  return FALSE;
}

static gboolean
clay_color_hex_entry_matched (GtkEntryCompletion *completion,
                              GtkTreeModel       *model,
                              GtkTreeIter        *iter,
                              ClayColorHexEntry  *entry)
{
  gchar     *name  = NULL;
  GeglColor *color = NULL;

  gtk_tree_model_get (model, iter,
                      COLUMN_NAME, &name,
                      -1);

  if ((color = clay_color_parse_name (name)))
    clay_color_hex_entry_set_color (entry, color);

  g_free (name);
  g_clear_object (&color);

  return TRUE;
}
