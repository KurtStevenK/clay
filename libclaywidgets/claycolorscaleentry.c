/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycolorscaleentry.c
 * Copyright (C) 2020 Jehan
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
 * SECTION: claycolorscaleentry
 * @title: ClayColorScaleEntry
 * @short_description: Widget containing a color scale, a spin button
 *                     and a label.
 *
 * This widget is a subclass of #ClayScaleEntry showing a
 * #ClayColorScale instead of a #GtkScale.
 **/

struct _ClayColorScaleEntry
{
  ClayScaleEntry parent_instance;
};

static GtkWidget * clay_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment);

G_DEFINE_TYPE (ClayColorScaleEntry, clay_color_scale_entry, CLAY_TYPE_SCALE_ENTRY)

#define parent_class clay_color_scale_entry_parent_class


static void
clay_color_scale_entry_class_init (ClayColorScaleEntryClass *klass)
{
  ClayScaleEntryClass *entry_class = CLAY_SCALE_ENTRY_CLASS (klass);

  entry_class->new_range_widget = clay_color_scale_entry_new_range_widget;
}

static void
clay_color_scale_entry_init (ClayColorScaleEntry *entry)
{
}

static GtkWidget *
clay_color_scale_entry_new_range_widget (GtkAdjustment  *adjustment)
{
  GtkWidget *scale;

  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), NULL);

  scale = clay_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                CLAY_COLOR_SELECTOR_VALUE);

  gtk_range_set_adjustment (GTK_RANGE (scale), adjustment);

  return scale;
}

/**
 * clay_color_scale_entry_new:
 * @text:           The text for the #GtkLabel.
 * @value:          The initial value.
 * @lower:          The lower boundary.
 * @upper:          The upper boundary.
 * @digits:         The number of decimal digits.
 *
 * Returns: (transfer full): The new #ClayColorScale widget.
 **/
GtkWidget *
clay_color_scale_entry_new (const gchar *text,
                            gdouble      value,
                            gdouble      lower,
                            gdouble      upper,
                            guint        digits)
{
  GtkWidget *entry;

  entry = g_object_new (CLAY_TYPE_COLOR_SCALE_ENTRY,
                        "label",          text,
                        "value",          value,
                        "lower",          lower,
                        "upper",          upper,
                        "digits",         digits,
                        NULL);

  return entry;
}
