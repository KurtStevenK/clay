/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelentry.c
 * Copyright (C) 2022 Jehan
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"
#include "libclaybase/claybase.h"

#include "claywidgets.h"


/**
 * SECTION: claylabelentry
 * @title: ClayLabelEntry
 * @short_description: Widget containing an entry and a label.
 *
 * This widget is a subclass of #ClayLabeled with a #GtkEntry.
 **/

enum
{
  VALUE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VALUE,
};

struct _ClayLabelEntry
{
  ClayLabeled   parent_class;

  GtkWidget    *entry;
};

static void        clay_label_entry_constructed       (GObject       *object);
static void        clay_label_entry_set_property      (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void        clay_label_entry_get_property      (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);

static GtkWidget * clay_label_entry_populate          (ClayLabeled   *entry,
                                                       gint          *x,
                                                       gint          *y,
                                                       gint          *width,
                                                       gint          *height);

G_DEFINE_TYPE (ClayLabelEntry, clay_label_entry, CLAY_TYPE_LABELED)

#define parent_class clay_label_entry_parent_class

static guint clay_label_entry_signals[LAST_SIGNAL] = { 0 };

static void
clay_label_entry_class_init (ClayLabelEntryClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  ClayLabeledClass *labeled_class = CLAY_LABELED_CLASS (klass);

  clay_label_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = clay_label_entry_constructed;
  object_class->set_property = clay_label_entry_set_property;
  object_class->get_property = clay_label_entry_get_property;

  labeled_class->populate    = clay_label_entry_populate;

  /**
   * ClayLabelEntry:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value",
                                                        "Entry text",
                                                        "The text in the entry",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE));
}

static void
clay_label_entry_init (ClayLabelEntry *entry)
{
  entry->entry = gtk_entry_new ();
}

static void
clay_label_entry_constructed (GObject *object)
{
  ClayLabelEntry *entry  = CLAY_LABEL_ENTRY (object);
  GtkEntryBuffer *buffer = gtk_entry_get_buffer (GTK_ENTRY (entry->entry));

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (buffer), "text",
                          G_OBJECT (entry),  "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
clay_label_entry_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayLabelEntry *entry = CLAY_LABEL_ENTRY (object);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          GtkEntryBuffer *buffer   = gtk_entry_get_buffer (GTK_ENTRY (entry->entry));
          const gchar    *new_text = g_value_get_string (value);
          const gchar    *text     = gtk_entry_buffer_get_text (buffer);

          /* Avoid looping forever since we have bound this widget's
           * "value" property with the entry button "value" property.
           */
          if ((new_text == NULL && g_strcmp0 (text, "") != 0) ||
              (new_text != NULL && g_strcmp0 (text, new_text) != 0))
            gtk_entry_buffer_set_text (buffer, new_text ? new_text : "", -1);

          g_signal_emit (object, clay_label_entry_signals[VALUE_CHANGED], 0);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_label_entry_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayLabelEntry *entry = CLAY_LABEL_ENTRY (object);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          GtkEntryBuffer *buffer = gtk_entry_get_buffer (GTK_ENTRY (entry->entry));

          g_value_set_string (value, gtk_entry_buffer_get_text (buffer));
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_label_entry_populate (ClayLabeled *labeled,
                           gint        *x,
                           gint        *y,
                           gint        *width,
                           gint        *height)
{
  ClayLabelEntry *entry = CLAY_LABEL_ENTRY (labeled);

  gtk_grid_attach (GTK_GRID (entry), entry->entry, 1, 0, 1, 1);
  /* Make sure the label and entry won't be glued next to each other's. */
  gtk_grid_set_column_spacing (GTK_GRID (entry),
                               4 * gtk_widget_get_scale_factor (GTK_WIDGET (entry)));
  gtk_widget_set_visible (entry->entry, TRUE);

  return entry->entry;
}


/* Public Functions */


/**
 * clay_label_entry_new:
 * @label:  The text for the #GtkLabel.
 *
 *
 * Returns: (transfer full): The new #ClayLabelEntry widget.
 **/
GtkWidget *
clay_label_entry_new (const gchar *label)
{
  GtkWidget *labeled;

  labeled = g_object_new (CLAY_TYPE_LABEL_ENTRY,
                          "label",  label,
                          NULL);

  return labeled;
}

/**
 * clay_label_entry_set_value:
 * @entry: The #GtkLabelEntry.
 * @value: A new value.
 *
 * This function sets the value in the #GtkEntry inside @entry.
 **/
void
clay_label_entry_set_value (ClayLabelEntry *entry,
                            const gchar    *value)
{
  g_return_if_fail (CLAY_IS_LABEL_ENTRY (entry));

  g_object_set (entry,
                "value", value,
                NULL);
}

/**
 * clay_label_entry_get_value:
 * @entry: The #GtkLabelEntry.
 *
 * This function returns the value shown by @entry.
 *
 * Returns: (transfer none): The value currently set.
 **/
const gchar *
clay_label_entry_get_value (ClayLabelEntry *entry)
{
  GtkEntryBuffer *buffer;

  g_return_val_if_fail (CLAY_IS_LABEL_ENTRY (entry), NULL);

  buffer = gtk_entry_get_buffer (GTK_ENTRY (entry->entry));

  return gtk_entry_buffer_get_text (buffer);
}

/**
 * clay_label_entry_get_entry:
 * @entry: The #ClayLabelEntry
 *
 * This function returns the #GtkEntry packed in @entry.
 *
 * Returns: (transfer none): The #GtkEntry contained in @entry.
 **/
GtkWidget *
clay_label_entry_get_entry (ClayLabelEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_LABEL_ENTRY (entry), NULL);

  return entry->entry;
}
