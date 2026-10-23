/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claysizeentry.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
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

#include "libclaybase/claybase.h"

#include "claywidgets.h"

#include "clayeevl-private.h"
#include "claysizeentry.h"


/**
 * SECTION: claysizeentry
 * @title: ClaySizeEntry
 * @short_description: Widget for entering pixel values and resolutions.
 * @see_also: #ClayUnit, #ClayUnitComboBox, clay_coordinates_new()
 *
 * This widget is used to enter pixel distances/sizes and resolutions.
 *
 * You can specify the number of fields the widget should provide. For
 * each field automatic mappings are performed between the field's
 * "reference value" and its "value".
 *
 * There is a #ClayUnitComboBox right of the entry fields which lets
 * you specify the #ClayUnit of the displayed values.
 *
 * For each field, there can be one or two #GtkSpinButton's to enter
 * "value" and "reference value". If you specify @show_refval as
 * %FALSE in clay_size_entry_new() there will be only one
 * #GtkSpinButton and the #ClayUnitComboBox will contain an item for
 * selecting CLAY_UNIT_PIXEL.
 *
 * The "reference value" is either of CLAY_UNIT_PIXEL or dpi,
 * depending on which #ClaySizeEntryUpdatePolicy you specify in
 * clay_size_entry_new().  The "value" is either the size in pixels
 * mapped to the size in a real-world-unit (see #ClayUnit) or the dpi
 * value mapped to pixels per real-world-unit.
 **/


#define SIZE_MAX_VALUE 500000.0

#define CLAY_SIZE_ENTRY_DIGITS(unit) (MIN (clay_unit_get_digits (unit), 5) + 1)


enum
{
  VALUE_CHANGED,
  REFVAL_CHANGED,
  UNIT_CHANGED,
  LAST_SIGNAL
};


typedef struct _ClaySizeEntryField
{
  ClaySizeEntry *gse;

  gdouble        resolution;
  gdouble        lower;
  gdouble        upper;

  GtkAdjustment *value_adjustment;
  GtkWidget     *value_spinbutton;
  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  GtkAdjustment *refval_adjustment;
  GtkWidget     *refval_spinbutton;
  gdouble        refval;
  gdouble        min_refval;
  gdouble        max_refval;
  gint           refval_digits;

  gint           stop_recursion;
} ClaySizeEntryField;


struct _ClaySizeEntry
{
  GtkGrid                    parent_instance;

  GSList                    *fields;
  gint                       number_of_fields;

  GtkWidget                 *unit_combo;
  ClayUnit                  *unit;
  gboolean                   menu_show_pixels;
  gboolean                   menu_show_percent;

  gboolean                   show_refval;
  ClaySizeEntryUpdatePolicy  update_policy;
};


static void      clay_size_entry_finalize            (GObject            *object);
static void      clay_size_entry_update_value        (ClaySizeEntryField *gsef,
                                                      gdouble             value);
static void      clay_size_entry_value_callback      (GtkAdjustment      *adjustment,
                                                      gpointer            data);
static void      clay_size_entry_update_refval       (ClaySizeEntryField *gsef,
                                                      gdouble             refval);
static void      clay_size_entry_refval_callback     (GtkAdjustment      *adjustment,
                                                      gpointer            data);
static void      clay_size_entry_update_unit         (ClaySizeEntry      *gse,
                                                      ClayUnit           *unit);
static void      clay_size_entry_unit_callback       (GtkWidget          *widget,
                                                      ClaySizeEntry      *sizeentry);
static void      clay_size_entry_attach_eevl         (GtkSpinButton      *spin_button,
                                                      ClaySizeEntryField *gsef);
static gint      clay_size_entry_eevl_input_callback (GtkSpinButton      *spinner,
                                                      gdouble            *return_val,
                                                      gpointer           *data);
static gboolean  clay_size_entry_eevl_unit_resolver  (const gchar        *ident,
                                                      ClayEevlQuantity   *factor,
                                                      gdouble            *offset,
                                                      gpointer            data);


G_DEFINE_TYPE (ClaySizeEntry, clay_size_entry, GTK_TYPE_GRID)

#define parent_class clay_size_entry_parent_class

static guint clay_size_entry_signals[LAST_SIGNAL] = { 0 };


static void
clay_size_entry_class_init (ClaySizeEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  clay_size_entry_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_size_entry_signals[REFVAL_CHANGED] =
    g_signal_new ("refval-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_size_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize = clay_size_entry_finalize;
}

static void
clay_size_entry_init (ClaySizeEntry *gse)
{
  gse->unit              = clay_unit_pixel ();
  gse->menu_show_pixels  = TRUE;
  gse->menu_show_percent = TRUE;
  gse->show_refval       = FALSE;
  gse->update_policy     = CLAY_SIZE_ENTRY_UPDATE_NONE;
}

static void
clay_size_entry_finalize (GObject *object)
{
  ClaySizeEntry *gse = CLAY_SIZE_ENTRY (object);

  if (gse->fields)
    {
      GSList *list;

      for (list = gse->fields; list; list = list->next)
        g_slice_free (ClaySizeEntryField, list->data);

      g_slist_free (gse->fields);
      gse->fields = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * clay_size_entry_new:
 * @number_of_fields:  The number of input fields.
 * @unit:              The initial unit.
 * @unit_format:       A printf-like unit-format string as is used with
 *                     clay_unit_menu_new().
 * @menu_show_pixels:  %TRUE if the unit menu should contain an item for
 *                     CLAY_UNIT_PIXEL (ignored if the @update_policy is not
 *                     CLAY_SIZE_ENTRY_UPDATE_NONE).
 * @menu_show_percent: %TRUE if the unit menu should contain an item for
 *                     CLAY_UNIT_PERCENT.
 * @show_refval:       %TRUE if you want an extra "reference value"
 *                     spinbutton per input field.
 * @spinbutton_width:  The minimal horizontal size of the #GtkSpinButton's.
 * @update_policy:     How the automatic pixel <-> real-world-unit
 *                     calculations should be done.
 *
 * Creates a new #ClaySizeEntry widget.
 *
 * To have all automatic calculations performed correctly, set up the
 * widget in the following order:
 *
 * 1. clay_size_entry_new()
 *
 * 2. (for each additional input field) clay_size_entry_add_field()
 *
 * 3. clay_size_entry_set_unit()
 *
 * For each input field:
 *
 * 4. clay_size_entry_set_resolution()
 *
 * 5. clay_size_entry_set_refval_boundaries()
 *    (or clay_size_entry_set_value_boundaries())
 *
 * 6. clay_size_entry_set_size()
 *
 * 7. clay_size_entry_set_refval() (or clay_size_entry_set_value())
 *
 * The #ClaySizeEntry is derived from #GtkGrid and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #ClayUnitComboBox to allow the caller to add labels or a
 * #ClayChainButton.
 *
 * Returns: A Pointer to the new #ClaySizeEntry widget.
 **/
GtkWidget *
clay_size_entry_new (gint                       number_of_fields,
                     ClayUnit                  *unit,
                     const gchar               *unit_format,
                     gboolean                   menu_show_pixels,
                     gboolean                   menu_show_percent,
                     gboolean                   show_refval,
                     gint                       spinbutton_width,
                     ClaySizeEntryUpdatePolicy  update_policy)
{
  ClaySizeEntry *gse;
  ClayUnitStore *store;
  gint           i;

  g_return_val_if_fail (CLAY_IS_UNIT (unit), NULL);
  g_return_val_if_fail ((number_of_fields >= 0) && (number_of_fields <= 16),
                        NULL);

  gse = g_object_new (CLAY_TYPE_SIZE_ENTRY, NULL);

  gse->number_of_fields = number_of_fields;
  gse->unit             = unit;
  gse->show_refval      = show_refval;
  gse->update_policy    = update_policy;

  /*  show the 'pixels' menu entry only if we are a 'size' sizeentry and
   *  don't have the reference value spinbutton
   */
  if ((update_policy == CLAY_SIZE_ENTRY_UPDATE_RESOLUTION) ||
      (show_refval == TRUE))
    gse->menu_show_pixels = FALSE;
  else
    gse->menu_show_pixels = menu_show_pixels;

  /*  show the 'percent' menu entry only if we are a 'size' sizeentry
   */
  if (update_policy == CLAY_SIZE_ENTRY_UPDATE_RESOLUTION)
    gse->menu_show_percent = FALSE;
  else
    gse->menu_show_percent = menu_show_percent;

  for (i = 0; i < number_of_fields; i++)
    {
      ClaySizeEntryField *gsef = g_slice_new0 (ClaySizeEntryField);
      gint                digits;

      gse->fields = g_slist_append (gse->fields, gsef);

      gsef->gse               = gse;
      gsef->resolution        = 1.0; /*  just to avoid division by zero  */
      gsef->lower             = 0.0;
      gsef->upper             = 100.0;
      gsef->value             = 0;
      gsef->min_value         = 0;
      gsef->max_value         = SIZE_MAX_VALUE;
      gsef->refval_adjustment = NULL;
      gsef->value_adjustment  = NULL;
      gsef->refval            = 0;
      gsef->min_refval        = 0;
      gsef->max_refval        = SIZE_MAX_VALUE;
      gsef->refval_digits     =
        (update_policy == CLAY_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
      gsef->stop_recursion    = 0;

      digits = ((unit == clay_unit_pixel ()) ?
                gsef->refval_digits : ((unit == clay_unit_percent ()) ?
                                       2 : CLAY_SIZE_ENTRY_DIGITS (unit)));

      gsef->value_adjustment = gtk_adjustment_new (gsef->value,
                                                   gsef->min_value,
                                                   gsef->max_value,
                                                   1.0, 10.0, 0.0);
      gsef->value_spinbutton = clay_spin_button_new (gsef->value_adjustment,
                                                     1.0, digits);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                   TRUE);

      clay_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                   gsef);

      if (spinbutton_width > 0)
        {
          if (spinbutton_width < 17)
            gtk_entry_set_width_chars (GTK_ENTRY (gsef->value_spinbutton),
                                       spinbutton_width);
          else
            gtk_widget_set_size_request (gsef->value_spinbutton,
                                         spinbutton_width, -1);
        }

      gtk_grid_attach (GTK_GRID (gse), gsef->value_spinbutton,
                       i+1, gse->show_refval+1, 1, 1);
      g_signal_connect (gsef->value_adjustment, "value-changed",
                        G_CALLBACK (clay_size_entry_value_callback),
                        gsef);

      gtk_widget_set_visible (gsef->value_spinbutton, TRUE);

      if (gse->show_refval)
        {
          gsef->refval_adjustment = gtk_adjustment_new (gsef->refval,
                                                        gsef->min_refval,
                                                        gsef->max_refval,
                                                        1.0, 10.0, 0.0);
          gsef->refval_spinbutton = clay_spin_button_new (gsef->refval_adjustment,
                                                          1.0,
                                                          gsef->refval_digits);
          gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                       TRUE);

          gtk_widget_set_size_request (gsef->refval_spinbutton,
                                       spinbutton_width, -1);
          gtk_grid_attach (GTK_GRID (gse), gsef->refval_spinbutton,
                           i + 1, 1, 1, 1);
          g_signal_connect (gsef->refval_adjustment,
                            "value-changed",
                            G_CALLBACK (clay_size_entry_refval_callback),
                            gsef);

          clay_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                       gsef);

          gtk_widget_set_visible (gsef->refval_spinbutton, TRUE);
        }

      if (gse->menu_show_pixels && (unit == clay_unit_pixel ()) &&
          ! gse->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }

  store = clay_unit_store_new (gse->number_of_fields);
  clay_unit_store_set_has_pixels (store, gse->menu_show_pixels);
  clay_unit_store_set_has_percent (store, gse->menu_show_percent);

  if (unit_format)
    {
      gchar *short_format = g_strdup (unit_format);
      gchar *p;

      while ((p = strstr (short_format, "%n")))
#ifndef _UCRT
        strcpy (p, "%a");
#else
        strcpy_s (p, strlen (p) + 1, "%a");
#endif

      g_object_set (store,
                    "short-format", short_format,
                    "long-format",  unit_format,
                    NULL);

      g_free (short_format);
    }

  gse->unit_combo = clay_unit_combo_box_new_with_model (store);
  g_object_unref (store);

  clay_unit_combo_box_set_active (CLAY_UNIT_COMBO_BOX (gse->unit_combo), unit);

  gtk_grid_attach (GTK_GRID (gse), gse->unit_combo,
                    i+2, gse->show_refval+1, 1, 1);
  g_signal_connect (gse->unit_combo, "changed",
                    G_CALLBACK (clay_size_entry_unit_callback),
                    gse);
  gtk_widget_set_visible (gse->unit_combo, TRUE);

  return GTK_WIDGET (gse);
}


/**
 * clay_size_entry_add_field:
 * @gse:               The sizeentry you want to add a field to.
 * @value_spinbutton:  The spinbutton to display the field's value.
 * @refval_spinbutton: (nullable): The spinbutton to display the field's reference value.
 *
 * Adds an input field to the #ClaySizeEntry.
 *
 * The new input field will have the index 0. If you specified @show_refval
 * as %TRUE in clay_size_entry_new() you have to pass an additional
 * #GtkSpinButton to hold the reference value. If @show_refval was %FALSE,
 * @refval_spinbutton will be ignored.
 **/
void
clay_size_entry_add_field  (ClaySizeEntry *gse,
                            GtkSpinButton *value_spinbutton,
                            GtkSpinButton *refval_spinbutton)
{
  ClaySizeEntryField   *gsef;
  gint                  digits;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));
  g_return_if_fail (GTK_IS_SPIN_BUTTON (value_spinbutton));

  if (gse->show_refval)
    {
      g_return_if_fail (GTK_IS_SPIN_BUTTON (refval_spinbutton));
    }

  gsef = g_slice_new0 (ClaySizeEntryField);

  gse->fields = g_slist_prepend (gse->fields, gsef);
  gse->number_of_fields++;

  gsef->gse            = gse;
  gsef->resolution     = 1.0; /*  just to avoid division by zero  */
  gsef->lower          = 0.0;
  gsef->upper          = 100.0;
  gsef->value          = 0;
  gsef->min_value      = 0;
  gsef->max_value      = SIZE_MAX_VALUE;
  gsef->refval         = 0;
  gsef->min_refval     = 0;
  gsef->max_refval     = SIZE_MAX_VALUE;
  gsef->refval_digits  =
    (gse->update_policy == CLAY_SIZE_ENTRY_UPDATE_SIZE) ? 0 : 3;
  gsef->stop_recursion = 0;

  gsef->value_adjustment = gtk_spin_button_get_adjustment (value_spinbutton);
  gsef->value_spinbutton = GTK_WIDGET (value_spinbutton);
  g_signal_connect (gsef->value_adjustment, "value-changed",
                    G_CALLBACK (clay_size_entry_value_callback),
                    gsef);

  clay_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                               gsef);

  if (gse->show_refval)
    {
      gsef->refval_adjustment = gtk_spin_button_get_adjustment (refval_spinbutton);
      gsef->refval_spinbutton = GTK_WIDGET (refval_spinbutton);
      g_signal_connect (gsef->refval_adjustment, "value-changed",
                        G_CALLBACK (clay_size_entry_refval_callback),
                        gsef);

      clay_size_entry_attach_eevl (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                   gsef);
    }

  digits = ((gse->unit == clay_unit_pixel ()) ? gsef->refval_digits :
            (gse->unit == clay_unit_percent ()) ? 2 :
            CLAY_SIZE_ENTRY_DIGITS (gse->unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (value_spinbutton), digits);

  if (gse->menu_show_pixels &&
      !gse->show_refval &&
      (gse->unit == clay_unit_pixel ()))
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                  gsef->refval_digits);
    }
}

ClaySizeEntryUpdatePolicy
clay_size_entry_get_update_policy (ClaySizeEntry *gse)
{
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), CLAY_SIZE_ENTRY_UPDATE_SIZE);

  return gse->update_policy;
}

gint
clay_size_entry_get_n_fields (ClaySizeEntry *gse)
{
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), 0);

  return gse->number_of_fields;
}

/**
 * clay_size_entry_get_unit_combo:
 * @gse: a #ClaySizeEntry.
 *
 * Returns: (transfer none) (type ClayUnitComboBox): the size entry's #ClayUnitComboBox.
 **/
GtkWidget *
clay_size_entry_get_unit_combo (ClaySizeEntry *gse)
{
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), NULL);

  return gse->unit_combo;
}

/**
 * clay_size_entry_attach_label:
 * @gse:       The sizeentry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #ClaySizeEntry (which is a #GtkGrid).
 *
 * Returns: (transfer none): A pointer to the new #GtkLabel widget.
 **/
GtkWidget *
clay_size_entry_attach_label (ClaySizeEntry *gse,
                              const gchar   *text,
                              gint           row,
                              gint           column,
                              gfloat         alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GList *children;
      GList *list;

      children = gtk_container_get_children (GTK_CONTAINER (gse));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;
          gint       left_attach;
          gint       top_attach;

          gtk_container_child_get (GTK_CONTAINER (gse), child,
                                   "left-attach", &left_attach,
                                   "top-attach",  &top_attach,
                                   NULL);

          if (left_attach == 1 && top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label), child);
              break;
            }
        }

      g_list_free (children);
    }

  gtk_label_set_xalign (GTK_LABEL (label), alignment);

  gtk_grid_attach (GTK_GRID (gse), label, column, row, 1, 1);
  gtk_widget_set_visible (label, TRUE);

  return label;
}


/**
 * clay_size_entry_set_resolution:
 * @gse:        The sizeentry you want to set a resolution for.
 * @field:      The index of the field you want to set the resolution for.
 * @resolution: The new resolution (in dpi) for the chosen @field.
 * @keep_size:  %TRUE if the @field's size in pixels should stay the same.
 *              %FALSE if the @field's size in units should stay the same.
 *
 * Sets the resolution (in dpi) for field # @field of the #ClaySizeEntry.
 *
 * The @resolution passed will be clamped to fit in
 * [#CLAY_MIN_RESOLUTION..#CLAY_MAX_RESOLUTION].
 *
 * This function does nothing if the #ClaySizeEntryUpdatePolicy specified in
 * clay_size_entry_new() doesn't equal to #CLAY_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
clay_size_entry_set_resolution (ClaySizeEntry *gse,
                                gint           field,
                                gdouble        resolution,
                                gboolean       keep_size)
{
  ClaySizeEntryField *gsef;
  gfloat              val;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  resolution = CLAMP (resolution, CLAY_MIN_RESOLUTION, CLAY_MAX_RESOLUTION);

  gsef = (ClaySizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->resolution = resolution;

  val = gsef->value;

  gsef->stop_recursion = 0;
  clay_size_entry_set_refval_boundaries (gse, field,
                                         gsef->min_refval, gsef->max_refval);

  if (! keep_size)
    clay_size_entry_set_value (gse, field, val);
}


/**
 * clay_size_entry_set_size:
 * @gse:   The sizeentry you want to set a size for.
 * @field: The index of the field you want to set the size for.
 * @lower: The reference value which will be treated as 0%.
 * @upper: The reference value which will be treated as 100%.
 *
 * Sets the pixel values for field # @field of the #ClaySizeEntry
 * which will be treated as 0% and 100%.
 *
 * These values will be used if you specified @menu_show_percent as %TRUE
 * in clay_size_entry_new() and the user has selected CLAY_UNIT_PERCENT in
 * the #ClaySizeEntry's #ClayUnitComboBox.
 *
 * This function does nothing if the #ClaySizeEntryUpdatePolicy specified in
 * clay_size_entry_new() doesn't equal to CLAY_SIZE_ENTRY_UPDATE_SIZE.
 **/
void
clay_size_entry_set_size (ClaySizeEntry *gse,
                          gint           field,
                          gdouble        lower,
                          gdouble        upper)
{
  ClaySizeEntryField *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (ClaySizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->lower = lower;
  gsef->upper = upper;

  clay_size_entry_set_refval (gse, field, gsef->refval);
}


/**
 * clay_size_entry_set_value_boundaries:
 * @gse:   The sizeentry you want to set value boundaries for.
 * @field: The index of the field you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the chosen @field.
 * @upper: The new upper boundary of the value of the chosen @field.
 *
 * Limits the range of possible values which can be entered in field # @field
 * of the #ClaySizeEntry.
 *
 * The current value of the @field will be clamped to fit in the @field's
 * new boundaries.
 *
 * NOTE: In most cases you won't be interested in this function because the
 *       #ClaySizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use clay_size_entry_set_refval_boundaries() instead.
 *       Whatever you do, don't mix these calls. A size entry should either
 *       be clamped by the value or the reference value.
 **/
void
clay_size_entry_set_value_boundaries (ClaySizeEntry *gse,
                                      gint           field,
                                      gdouble        lower,
                                      gdouble        upper)
{
  ClaySizeEntryField *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (ClaySizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->min_value        = lower;
  gsef->max_value        = upper;

  g_object_freeze_notify (G_OBJECT (gsef->value_adjustment));

  gtk_adjustment_set_lower (gsef->value_adjustment, gsef->min_value);
  gtk_adjustment_set_upper (gsef->value_adjustment, gsef->max_value);

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    {
      g_object_thaw_notify (G_OBJECT (gsef->value_adjustment));
      return;
    }

  gsef->stop_recursion++;
  switch (gse->update_policy)
    {
    case CLAY_SIZE_ENTRY_UPDATE_NONE:
      break;

    case CLAY_SIZE_ENTRY_UPDATE_SIZE:
      if (gse->unit == clay_unit_pixel ())
        clay_size_entry_set_refval_boundaries (gse, field,
                                               gsef->min_value,
                                               gsef->max_value);
      else if (gse->unit == clay_unit_percent ())
        clay_size_entry_set_refval_boundaries (gse, field,
                                               gsef->lower +
                                               (gsef->upper - gsef->lower) *
                                               gsef->min_value / 100,
                                               gsef->lower +
                                               (gsef->upper - gsef->lower) *
                                               gsef->max_value / 100);
      else
        clay_size_entry_set_refval_boundaries (gse, field,
                                               gsef->min_value *
                                               gsef->resolution /
                                               clay_unit_get_factor (gse->unit),
                                               gsef->max_value *
                                               gsef->resolution /
                                               clay_unit_get_factor (gse->unit));
      break;

    case CLAY_SIZE_ENTRY_UPDATE_RESOLUTION:
      clay_size_entry_set_refval_boundaries (gse, field,
                                             gsef->min_value *
                                             clay_unit_get_factor (gse->unit),
                                             gsef->max_value *
                                             clay_unit_get_factor (gse->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  clay_size_entry_set_value (gse, field, gsef->value);

  g_object_thaw_notify (G_OBJECT (gsef->value_adjustment));
}

/**
 * clay_size_entry_get_value:
 * @gse:   The sizeentry you want to know a value of.
 * @field: The index of the field you want to know the value of.
 *
 * Returns the value of field # @field of the #ClaySizeEntry.
 *
 * The @value returned is a distance or resolution
 * in the #ClayUnit the user has selected in the #ClaySizeEntry's
 * #ClayUnitComboBox.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #ClaySizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use clay_size_entry_get_refval() instead.
 *
 * Returns: The value of the chosen @field.
 **/
gdouble
clay_size_entry_get_value (ClaySizeEntry *gse,
                           gint           field)
{
  ClaySizeEntryField *gsef;

  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), 0);

  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 0);

  gsef = (ClaySizeEntryField *) g_slist_nth_data (gse->fields, field);

  return gsef->value;
}

static void
clay_size_entry_update_value (ClaySizeEntryField *gsef,
                              gdouble             value)
{
  if (gsef->stop_recursion > 1)
    return;

  gsef->value = value;

  switch (gsef->gse->update_policy)
    {
    case CLAY_SIZE_ENTRY_UPDATE_NONE:
      break;

    case CLAY_SIZE_ENTRY_UPDATE_SIZE:
      if (gsef->gse->unit == clay_unit_pixel ())
        gsef->refval = value;
      else if (gsef->gse->unit == clay_unit_percent ())
        gsef->refval = CLAMP (gsef->lower + (gsef->upper - gsef->lower) * value / 100,
                              gsef->min_refval, gsef->max_refval);
      else
        gsef->refval = CLAMP (value * gsef->resolution /
                              clay_unit_get_factor (gsef->gse->unit),
                              gsef->min_refval, gsef->max_refval);

      if (gsef->gse->show_refval)
        gtk_adjustment_set_value (gsef->refval_adjustment, gsef->refval);
      break;

    case CLAY_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->refval =
        CLAMP (value * clay_unit_get_factor (gsef->gse->unit),
               gsef->min_refval, gsef->max_refval);
      if (gsef->gse->show_refval)
        gtk_adjustment_set_value (gsef->refval_adjustment, gsef->refval);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, clay_size_entry_signals[VALUE_CHANGED], 0);
}

/**
 * clay_size_entry_set_value:
 * @gse:   The sizeentry you want to set a value for.
 * @field: The index of the field you want to set a value for.
 * @value: The new value for @field.
 *
 * Sets the value for field # @field of the #ClaySizeEntry.
 *
 * The @value passed is treated to be a distance or resolution
 * in the #ClayUnit the user has selected in the #ClaySizeEntry's
 * #ClayUnitComboBox.
 *
 * NOTE: In most cases you won't be interested in this value because the
 *       #ClaySizeEntry's purpose is to shield the programmer from unit
 *       calculations. Use clay_size_entry_set_refval() instead.
 **/
void
clay_size_entry_set_value (ClaySizeEntry *gse,
                           gint           field,
                           gdouble        value)
{
  ClaySizeEntryField *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (ClaySizeEntryField *) g_slist_nth_data (gse->fields, field);

  value = CLAMP (value, gsef->min_value, gsef->max_value);
  gtk_adjustment_set_value (gsef->value_adjustment, value);
  clay_size_entry_update_value (gsef, value);
}


static void
clay_size_entry_value_callback (GtkAdjustment *adjustment,
                                gpointer       data)
{
  ClaySizeEntryField *gsef;
  gdouble             new_value;

  gsef = (ClaySizeEntryField *) data;

  new_value = gtk_adjustment_get_value (adjustment);

  if (gsef->value != new_value)
    clay_size_entry_update_value (gsef, new_value);
}


/**
 * clay_size_entry_set_refval_boundaries:
 * @gse:   The sizeentry you want to set the reference value boundaries for.
 * @field: The index of the field you want to set the reference value
 *         boundaries for.
 * @lower: The new lower boundary of the reference value of the chosen @field.
 * @upper: The new upper boundary of the reference value of the chosen @field.
 *
 * Limits the range of possible reference values which can be entered in
 * field # @field of the #ClaySizeEntry.
 *
 * The current reference value of the @field will be clamped to fit in the
 * @field's new boundaries.
 **/
void
clay_size_entry_set_refval_boundaries (ClaySizeEntry *gse,
                                       gint           field,
                                       gdouble        lower,
                                       gdouble        upper)
{
  ClaySizeEntryField   *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail (lower <= upper);

  gsef = (ClaySizeEntryField *) g_slist_nth_data (gse->fields, field);
  gsef->min_refval = lower;
  gsef->max_refval = upper;

  if (gse->show_refval)
    {
      g_object_freeze_notify (G_OBJECT (gsef->refval_adjustment));

      gtk_adjustment_set_lower (gsef->refval_adjustment, gsef->min_refval);
      gtk_adjustment_set_upper (gsef->refval_adjustment, gsef->max_refval);
    }

  if (gsef->stop_recursion) /* this is a hack (but useful ;-) */
    {
      if (gse->show_refval)
        g_object_thaw_notify (G_OBJECT (gsef->refval_adjustment));

      return;
    }

  gsef->stop_recursion++;
  switch (gse->update_policy)
    {
    case CLAY_SIZE_ENTRY_UPDATE_NONE:
      break;

    case CLAY_SIZE_ENTRY_UPDATE_SIZE:
      if (gse->unit == clay_unit_pixel ())
        clay_size_entry_set_value_boundaries (gse, field,
                                              gsef->min_refval,
                                              gsef->max_refval);
      else if (gse->unit == clay_unit_percent ())
        clay_size_entry_set_value_boundaries (gse, field,
                                              100 * (gsef->min_refval -
                                                     gsef->lower) /
                                              (gsef->upper - gsef->lower),
                                              100 * (gsef->max_refval -
                                                     gsef->lower) /
                                              (gsef->upper - gsef->lower));
      else
        clay_size_entry_set_value_boundaries (gse, field,
                                              gsef->min_refval *
                                              clay_unit_get_factor (gse->unit) /
                                              gsef->resolution,
                                              gsef->max_refval *
                                              clay_unit_get_factor (gse->unit) /
                                              gsef->resolution);
      break;

    case CLAY_SIZE_ENTRY_UPDATE_RESOLUTION:
      clay_size_entry_set_value_boundaries (gse, field,
                                            gsef->min_refval /
                                            clay_unit_get_factor (gse->unit),
                                            gsef->max_refval /
                                            clay_unit_get_factor (gse->unit));
      break;

    default:
      break;
    }
  gsef->stop_recursion--;

  clay_size_entry_set_refval (gse, field, gsef->refval);

  if (gse->show_refval)
    g_object_thaw_notify (G_OBJECT (gsef->refval_adjustment));
}

/**
 * clay_size_entry_set_refval_digits:
 * @gse:    The sizeentry you want to set the reference value digits for.
 * @field:  The index of the field you want to set the reference value for.
 * @digits: The new number of decimal digits for the #GtkSpinButton which
 *          displays @field's reference value.
 *
 * Sets the decimal digits of field # @field of the #ClaySizeEntry to
 * @digits.
 *
 * If you don't specify this value explicitly, the reference value's number
 * of digits will equal to 0 for #CLAY_SIZE_ENTRY_UPDATE_SIZE and to 2 for
 * #CLAY_SIZE_ENTRY_UPDATE_RESOLUTION.
 **/
void
clay_size_entry_set_refval_digits (ClaySizeEntry *gse,
                                   gint           field,
                                   gint           digits)
{
  ClaySizeEntryField *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));
  g_return_if_fail ((digits >= 0) && (digits <= 6));

  gsef = (ClaySizeEntryField*) g_slist_nth_data (gse->fields, field);
  gsef->refval_digits = digits;

  if (gse->update_policy == CLAY_SIZE_ENTRY_UPDATE_SIZE)
    {
      if (gse->show_refval)
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->refval_spinbutton),
                                    gsef->refval_digits);
      else if (gse->unit == clay_unit_pixel ())
        gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                    gsef->refval_digits);
    }
}

/**
 * clay_size_entry_get_refval:
 * @gse:   The sizeentry you want to know a reference value of.
 * @field: The index of the field you want to know the reference value of.
 *
 * Returns the reference value for field # @field of the #ClaySizeEntry.
 *
 * The reference value is either a distance in pixels or a resolution
 * in dpi, depending on which #ClaySizeEntryUpdatePolicy you chose in
 * clay_size_entry_new().
 *
 * Returns: The reference value of the chosen @field.
 **/
gdouble
clay_size_entry_get_refval (ClaySizeEntry *gse,
                            gint           field)
{
  ClaySizeEntryField *gsef;

  /*  return 1.0 to avoid division by zero  */
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), 1.0);

  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), 1.0);

  gsef = (ClaySizeEntryField*) g_slist_nth_data (gse->fields, field);

  return gsef->refval;
}

static void
clay_size_entry_update_refval (ClaySizeEntryField *gsef,
                               gdouble             refval)
{
  if (gsef->stop_recursion > 1)
    return;

  gsef->refval = refval;

  switch (gsef->gse->update_policy)
    {
    case CLAY_SIZE_ENTRY_UPDATE_NONE:
      break;

    case CLAY_SIZE_ENTRY_UPDATE_SIZE:
      if (gsef->gse->unit == clay_unit_pixel ())
        gsef->value = refval;
      else if (gsef->gse->unit == clay_unit_percent ())
        gsef->value = CLAMP (100 * (refval - gsef->lower) / (gsef->upper - gsef->lower),
                             gsef->min_value, gsef->max_value);
      else
        gsef->value = CLAMP (refval * clay_unit_get_factor (gsef->gse->unit) /
                             gsef->resolution,
                             gsef->min_value, gsef->max_value);

      gtk_adjustment_set_value (gsef->value_adjustment, gsef->value);
      break;

    case CLAY_SIZE_ENTRY_UPDATE_RESOLUTION:
      gsef->value =
        CLAMP (refval / clay_unit_get_factor (gsef->gse->unit),
               gsef->min_value, gsef->max_value);
      gtk_adjustment_set_value (gsef->value_adjustment, gsef->value);
      break;

    default:
      break;
    }

  g_signal_emit (gsef->gse, clay_size_entry_signals[REFVAL_CHANGED], 0);
}

/**
 * clay_size_entry_set_refval:
 * @gse:    The sizeentry you want to set a reference value for.
 * @field:  The index of the field you want to set the reference value for.
 * @refval: The new reference value for @field.
 *
 * Sets the reference value for field # @field of the #ClaySizeEntry.
 *
 * The @refval passed is either a distance in pixels or a resolution in dpi,
 * depending on which #ClaySizeEntryUpdatePolicy you chose in
 * clay_size_entry_new().
 **/
void
clay_size_entry_set_refval (ClaySizeEntry *gse,
                            gint           field,
                            gdouble        refval)
{
  ClaySizeEntryField *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail ((field >= 0) && (field < gse->number_of_fields));

  gsef = (ClaySizeEntryField *) g_slist_nth_data (gse->fields, field);

  refval = CLAMP (refval, gsef->min_refval, gsef->max_refval);

  if (gse->show_refval)
    gtk_adjustment_set_value (gsef->refval_adjustment, refval);

  clay_size_entry_update_refval (gsef, refval);
}

static void
clay_size_entry_refval_callback (GtkAdjustment *adjustment,
                                 gpointer       data)
{
  ClaySizeEntryField *gsef;
  gdouble             new_refval;

  gsef = (ClaySizeEntryField *) data;

  new_refval = gtk_adjustment_get_value (adjustment);

  if (gsef->refval != new_refval)
    clay_size_entry_update_refval (gsef, new_refval);
}


/**
 * clay_size_entry_get_unit:
 * @gse: The sizeentry you want to know the unit of.
 *
 * Returns the #ClayUnit the user has selected in the #ClaySizeEntry's
 * #ClayUnitComboBox.
 *
 * Returns: (transfer none): The sizeentry's unit.
 **/
ClayUnit *
clay_size_entry_get_unit (ClaySizeEntry *gse)
{
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), clay_unit_inch ());

  return gse->unit;
}

static void
clay_size_entry_update_unit (ClaySizeEntry *gse,
                             ClayUnit      *unit)
{
  ClaySizeEntryField *gsef;
  gint                i;
  gint                digits;

  gse->unit = unit;

  digits = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (gse),
                                               "clay-pixel-digits"));

  for (i = 0; i < gse->number_of_fields; i++)
    {
      gsef = (ClaySizeEntryField *) g_slist_nth_data (gse->fields, i);

      if (gse->update_policy == CLAY_SIZE_ENTRY_UPDATE_SIZE)
        {
          if (unit == clay_unit_pixel ())
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        gsef->refval_digits + digits);
          else if (unit == clay_unit_percent ())
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        2 + digits);
          else
            gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                        CLAY_SIZE_ENTRY_DIGITS (unit) + digits);
        }
      else if (gse->update_policy == CLAY_SIZE_ENTRY_UPDATE_RESOLUTION)
        {
          digits = (clay_unit_get_digits (clay_unit_inch ()) - clay_unit_get_digits (unit));
          gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gsef->value_spinbutton),
                                      MAX (3 + digits, 3));
        }

      gsef->stop_recursion = 0; /* hack !!! */

      clay_size_entry_set_refval_boundaries (gse, i,
                                             gsef->min_refval,
                                             gsef->max_refval);
    }

  g_signal_emit (gse, clay_size_entry_signals[UNIT_CHANGED], 0);
}


/**
 * clay_size_entry_set_unit:
 * @gse:  The sizeentry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #ClaySizeEntry's unit. The reference value for all fields will
 * stay the same but the value in units or pixels per unit will change
 * according to which #ClaySizeEntryUpdatePolicy you chose in
 * clay_size_entry_new().
 **/
void
clay_size_entry_set_unit (ClaySizeEntry *gse,
                          ClayUnit      *unit)
{
  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  g_return_if_fail (gse->menu_show_pixels || (unit != clay_unit_pixel ()));
  g_return_if_fail (gse->menu_show_percent || (unit != clay_unit_percent ()));

  clay_unit_combo_box_set_active (CLAY_UNIT_COMBO_BOX (gse->unit_combo), unit);
  clay_size_entry_update_unit (gse, unit);
}

static void
clay_size_entry_unit_callback (GtkWidget     *widget,
                               ClaySizeEntry *gse)
{
  ClayUnit *new_unit;

  new_unit = clay_unit_combo_box_get_active (CLAY_UNIT_COMBO_BOX (widget));

  if (gse->unit != new_unit)
    clay_size_entry_update_unit (gse, new_unit);
}

/**
 * clay_size_entry_attach_eevl:
 * @spin_button: one of the size_entry's spinbuttons.
 * @gsef:        a size entry field.
 *
 * Hooks in the ClayEevl unit expression parser into the
 * #GtkSpinButton of the #ClaySizeEntryField.
 **/
static void
clay_size_entry_attach_eevl (GtkSpinButton      *spin_button,
                             ClaySizeEntryField *gsef)
{
  gtk_spin_button_set_numeric (spin_button, FALSE);
  gtk_spin_button_set_update_policy (spin_button, GTK_UPDATE_IF_VALID);

  g_signal_connect_after (spin_button, "input",
                          G_CALLBACK (clay_size_entry_eevl_input_callback),
                          gsef);
}

static gint
clay_size_entry_eevl_input_callback (GtkSpinButton *spinner,
                                     gdouble       *return_val,
                                     gpointer      *data)
{
  ClaySizeEntryField *gsef      = (ClaySizeEntryField *) data;
  ClayEevlOptions     options   = CLAY_EEVL_OPTIONS_INIT;
  gboolean            success   = FALSE;
  const gchar        *error_pos = NULL;
  GError             *error     = NULL;
  ClayEevlQuantity    result;

  g_return_val_if_fail (GTK_IS_SPIN_BUTTON (spinner), FALSE);
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gsef->gse), FALSE);

  options.unit_resolver_proc = clay_size_entry_eevl_unit_resolver;
  options.data               = data;

  /* enable ratio expressions when there are two fields */
  if (gsef->gse->number_of_fields == 2)
    {
      ClaySizeEntryField *other_gsef;
      ClayEevlQuantity    default_unit_factor;
      gdouble             default_unit_offset;

      options.ratio_expressions = TRUE;

      if (gsef == gsef->gse->fields->data)
        {
          other_gsef = gsef->gse->fields->next->data;

          options.ratio_invert = FALSE;
        }
      else
        {
          other_gsef = gsef->gse->fields->data;

          options.ratio_invert = TRUE;
        }

      options.unit_resolver_proc (NULL,
                                  &default_unit_factor, &default_unit_offset,
                                  options.data);

      options.ratio_quantity.value     = other_gsef->value /
                                         default_unit_factor.value;
      options.ratio_quantity.dimension = default_unit_factor.dimension;
    }

  success = clay_eevl_evaluate (gtk_entry_get_text (GTK_ENTRY (spinner)),
                                &options,
                                &result,
                                &error_pos,
                                &error);
  if (! success)
    {
      if (error && error_pos)
        {
          g_printerr ("ERROR: %s at '%s'\n",
                      error->message,
                      *error_pos ? error_pos : "<End of input>");
        }
      else
        {
          g_printerr ("ERROR: Expression evaluation failed without error.\n");
        }
      g_clear_error (&error);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else if (result.dimension != 1 && gsef->gse->unit != clay_unit_percent ())
    {
      g_printerr ("ERROR: result has wrong dimension (expected 1, got %d)\n", result.dimension);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else if (result.dimension != 0 && gsef->gse->unit == clay_unit_percent ())
    {
      g_printerr ("ERROR: result has wrong dimension (expected 0, got %d)\n", result.dimension);

      gtk_widget_error_bell (GTK_WIDGET (spinner));
      return GTK_INPUT_ERROR;
    }
  else
    {
      /* transform back to UI-unit */
      ClayEevlQuantity  ui_unit;
      GtkAdjustment    *adj;
      gdouble           val;

      if (gsef->gse->unit == clay_unit_pixel ())
        {
          ui_unit.value     = gsef->resolution;
          ui_unit.dimension = 1;
        }
      else if (gsef->gse->unit == clay_unit_percent ())
        {
          ui_unit.value     = 1.0;
          ui_unit.dimension = 0;
        }
      else
        {
          ui_unit.value     = clay_unit_get_factor(gsef->gse->unit);
          ui_unit.dimension = 1;
        }

      *return_val = result.value * ui_unit.value;

      /*  CLAMP() to adjustment bounds, or too large/small values
       *  will make the validation machinery revert to the old value.
       *  See bug #694477.
       */
      adj = gtk_spin_button_get_adjustment (spinner);

      val = CLAMP (*return_val,
                   gtk_adjustment_get_lower (adj),
                   gtk_adjustment_get_upper (adj));

      if (val != *return_val)
        {
          gtk_widget_error_bell (GTK_WIDGET (spinner));
          *return_val = val;
        }

      return TRUE;
    }
}

static gboolean
clay_size_entry_eevl_unit_resolver (const gchar      *identifier,
                                    ClayEevlQuantity *factor,
                                    gdouble          *offset,
                                    gpointer          data)
{
  ClaySizeEntryField *gsef                 = (ClaySizeEntryField *) data;
  gboolean            resolve_default_unit = (identifier == NULL);
  ClayUnit           *unit                 = clay_unit_pixel ();
  gint                i                    = 1;

  g_return_val_if_fail (gsef, FALSE);
  g_return_val_if_fail (factor != NULL, FALSE);
  g_return_val_if_fail (offset != NULL, FALSE);
  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gsef->gse), FALSE);

  *offset = 0.0;

  while (unit != NULL)
    {
      if ((resolve_default_unit && unit == gsef->gse->unit) ||
          (identifier &&
           (strcmp (clay_unit_get_symbol (unit),       identifier) == 0 ||
            strcmp (clay_unit_get_abbreviation (unit), identifier) == 0)))
        {
          if (unit == clay_unit_percent ())
            {
              if (gsef->gse->unit == clay_unit_percent ())
                {
                  factor->value = 1;
                  factor->dimension = 0;
                }
              else
                {
                  /* gsef->upper contains the '100%'-value */
                  factor->value = 100*gsef->resolution/(gsef->upper - gsef->lower);
                  /* gsef->lower contains the '0%'-value */
                  *offset       = gsef->lower/gsef->resolution;
                  factor->dimension = 1;
                }
              /* return here, don't perform percentage conversion */
              return TRUE;
            }
          else if (unit == clay_unit_pixel ())
            {
              factor->value = gsef->resolution;
            }
          else
            {
              factor->value = clay_unit_get_factor (unit);
            }

          if (gsef->gse->unit == clay_unit_percent ())
            {
              /* map non-percentages onto percent */
              factor->value = gsef->upper/(100*gsef->resolution);
              factor->dimension = 0;
            }
          else
            {
              factor->dimension = 1;
            }

          /* We are done */
          return TRUE;
        }

      /* Hack to handle percent within the loop */
      if (unit == clay_unit_percent ())
        break;

      unit = clay_unit_get_by_id (i++);

      if (unit == NULL)
        unit = clay_unit_percent ();
    }

  return FALSE;
}

/**
 * clay_size_entry_show_unit_menu:
 * @gse: a #ClaySizeEntry
 * @show: Boolean
 *
 * Controls whether a unit menu is shown in the size entry.  If
 * @show is %TRUE, the menu is shown; otherwise it is hidden.
 *
 * Since: 2.4
 **/
void
clay_size_entry_show_unit_menu (ClaySizeEntry *gse,
                                gboolean       show)
{
  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  gtk_widget_set_visible (gse->unit_combo, show);
}


/**
 * clay_size_entry_set_pixel_digits:
 * @gse: a #ClaySizeEntry
 * @digits: the number of digits to display for a pixel size
 *
 * This function allows you set up a #ClaySizeEntry so that sub-pixel
 * sizes can be entered.
 **/
void
clay_size_entry_set_pixel_digits (ClaySizeEntry *gse,
                                  gint           digits)
{
  ClayUnitComboBox *combo;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  combo = CLAY_UNIT_COMBO_BOX (gse->unit_combo);

  g_object_set_data (G_OBJECT (gse), "clay-pixel-digits",
                     GINT_TO_POINTER (digits));
  clay_size_entry_update_unit (gse, clay_unit_combo_box_get_active (combo));
}


/**
 * clay_size_entry_grab_focus:
 * @gse: The sizeentry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkGrid (is this actually true after the Table->Grid conversion?).
 **/
void
clay_size_entry_grab_focus (ClaySizeEntry *gse)
{
  ClaySizeEntryField   *gsef;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  gsef = gse->fields->data;
  if (gsef)
    gtk_widget_grab_focus (gse->show_refval ?
                           gsef->refval_spinbutton : gsef->value_spinbutton);
}

/**
 * clay_size_entry_set_activates_default:
 * @gse:     A #ClaySizeEntry
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * Iterates over all entries in the #ClaySizeEntry and calls
 * gtk_entry_set_activates_default() on them.
 *
 * Since: 2.4
 **/
void
clay_size_entry_set_activates_default (ClaySizeEntry *gse,
                                       gboolean       setting)
{
  GSList *list;

  g_return_if_fail (CLAY_IS_SIZE_ENTRY (gse));

  for (list = gse->fields; list; list = g_slist_next (list))
    {
      ClaySizeEntryField *gsef = list->data;

      if (gsef->value_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->value_spinbutton),
                                         setting);

      if (gsef->refval_spinbutton)
        gtk_entry_set_activates_default (GTK_ENTRY (gsef->refval_spinbutton),
                                         setting);
    }
}

/**
 * clay_size_entry_get_help_widget:
 * @gse: a #ClaySizeEntry
 * @field: the index of the widget you want to get a pointer to
 *
 * You shouldn't fiddle with the internals of a #ClaySizeEntry but
 * if you want to set tooltips using clay_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Returns: (transfer none): a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
clay_size_entry_get_help_widget (ClaySizeEntry *gse,
                                 gint           field)
{
  ClaySizeEntryField *gsef;

  g_return_val_if_fail (CLAY_IS_SIZE_ENTRY (gse), NULL);

  g_return_val_if_fail ((field >= 0) && (field < gse->number_of_fields), NULL);

  gsef = g_slist_nth_data (gse->fields, field);
  if (!gsef)
    return NULL;

  return (gsef->refval_spinbutton ?
          gsef->refval_spinbutton : gsef->value_spinbutton);
}
