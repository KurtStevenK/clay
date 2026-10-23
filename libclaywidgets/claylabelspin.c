/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelspin.c
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
 * SECTION: claylabelspin
 * @title: ClayLabelSpin
 * @short_description: Widget containing a spin button and a label.
 *
 * This widget is a subclass of #ClayLabeled with a #ClaySpinButton.
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
  PROP_LOWER,
  PROP_UPPER,
  PROP_DIGITS,
};

typedef struct _ClayLabelSpinPrivate
{
  ClayLabeled    parent_instance;

  GtkWidget     *spinbutton;
  GtkAdjustment *spin_adjustment;

  gint           digits;

  gdouble        value;
} ClayLabelSpinPrivate;

static void        clay_label_spin_constructed       (GObject       *object);
static void        clay_label_spin_set_property      (GObject       *object,
                                                      guint          property_id,
                                                      const GValue  *value,
                                                      GParamSpec    *pspec);
static void        clay_label_spin_get_property      (GObject       *object,
                                                      guint          property_id,
                                                      GValue        *value,
                                                      GParamSpec    *pspec);

static GtkWidget * clay_label_spin_populate          (ClayLabeled   *spin,
                                                      gint          *x,
                                                      gint          *y,
                                                      gint          *width,
                                                      gint          *height);

static void        clay_label_spin_update_spin_width (ClayLabelSpin *spin,
                                                      gdouble        lower,
                                                      gdouble        upper,
                                                      guint          digits);
static void        clay_label_spin_update_settings   (ClayLabelSpin *spin);

G_DEFINE_TYPE_WITH_PRIVATE (ClayLabelSpin, clay_label_spin, CLAY_TYPE_LABELED)

#define parent_class clay_label_spin_parent_class

static guint clay_label_spin_signals[LAST_SIGNAL] = { 0 };

static void
clay_label_spin_class_init (ClayLabelSpinClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  ClayLabeledClass *labeled_class = CLAY_LABELED_CLASS (klass);

  clay_label_spin_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayLabelSpinClass, value_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = clay_label_spin_constructed;
  object_class->set_property = clay_label_spin_set_property;
  object_class->get_property = clay_label_spin_get_property;

  labeled_class->populate    = clay_label_spin_populate;

  /**
   * ClayLabelSpin:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value", NULL,
                                                        "Current value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        CLAY_PARAM_READWRITE));

  /**
   * ClayLabelSpin:lower:
   *
   * The lower bound of the spin button.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_LOWER,
                                   g_param_spec_double ("lower", NULL,
                                                        "Minimum value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  /**
   * ClayLabelSpin:upper:
   *
   * The upper bound of the spin button.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_UPPER,
                                   g_param_spec_double ("upper", NULL,
                                                        "Max value",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 1.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  /**
   * ClayLabelSpin:digits:
   *
   * The number of decimal places to display. If -1, then the number is
   * estimated.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_DIGITS,
                                   g_param_spec_int ("digits", NULL,
                                                     "The number of decimal places to display",
                                                     -1, G_MAXINT, -1,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
clay_label_spin_init (ClayLabelSpin *spin)
{
  ClayLabelSpinPrivate *priv  = clay_label_spin_get_instance_private (spin);

  /* We want the adjustment to exist at init so that construction
   * properties can apply (default values are bogus but should be
   * properly overridden with expected values if the object was created
   * with clay_label_spin_new().
   */
  priv->spin_adjustment = gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 0.0);

  gtk_grid_set_row_spacing (GTK_GRID (spin), 6);
  gtk_grid_set_column_spacing (GTK_GRID (spin), 6);
}

static void
clay_label_spin_constructed (GObject *object)
{
  ClayLabelSpin        *spin = CLAY_LABEL_SPIN (object);
  ClayLabelSpinPrivate *priv  = clay_label_spin_get_instance_private (spin);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (priv->spin_adjustment), "value",
                          G_OBJECT (spin),                  "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);

  clay_label_spin_update_settings (spin);
}

static void
clay_label_spin_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayLabelSpin        *spin = CLAY_LABEL_SPIN (object);
  ClayLabelSpinPrivate *priv  = clay_label_spin_get_instance_private (spin);

  switch (property_id)
    {
    case PROP_VALUE:
      if (priv->value != g_value_get_double (value))
        {
          priv->value = g_value_get_double (value);
          g_signal_emit (object, clay_label_spin_signals[VALUE_CHANGED], 0);
        }
      break;
    case PROP_LOWER:
      gtk_adjustment_set_lower (priv->spin_adjustment,
                                g_value_get_double (value));
      if (priv->spinbutton)
        {
          clay_label_spin_update_settings (spin);
        }
      break;
    case PROP_UPPER:
      gtk_adjustment_set_upper (priv->spin_adjustment,
                                g_value_get_double (value));
      if (priv->spinbutton)
        {
          clay_label_spin_update_settings (spin);
        }
      break;
    case PROP_DIGITS:
      priv->digits = g_value_get_int (value);
      if (priv->spinbutton)
        {
          clay_label_spin_update_settings (spin);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_label_spin_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayLabelSpin        *spin       = CLAY_LABEL_SPIN (object);
  ClayLabelSpinPrivate *priv       = clay_label_spin_get_instance_private (spin);
  GtkSpinButton        *spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, priv->value);
      break;
    case PROP_LOWER:
      g_value_set_double (value, gtk_adjustment_get_lower (priv->spin_adjustment));
      break;
    case PROP_UPPER:
      g_value_set_double (value, gtk_adjustment_get_upper (priv->spin_adjustment));
      break;
    case PROP_DIGITS:
      g_value_set_uint (value, gtk_spin_button_get_digits (spinbutton));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_label_spin_populate (ClayLabeled *labeled,
                          gint        *x,
                          gint        *y,
                          gint        *width,
                          gint        *height)
{
  ClayLabelSpin        *spin = CLAY_LABEL_SPIN (labeled);
  ClayLabelSpinPrivate *priv = clay_label_spin_get_instance_private (spin);

  priv->spinbutton = clay_spin_button_new (priv->spin_adjustment, 2.0, 2.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (priv->spinbutton), TRUE);

  gtk_grid_attach (GTK_GRID (spin), priv->spinbutton, 1, 0, 1, 1);
  gtk_widget_set_visible (priv->spinbutton, TRUE);

  return priv->spinbutton;
}

static void
clay_label_spin_update_spin_width (ClayLabelSpin *spin,
                                   gdouble        lower,
                                   gdouble        upper,
                                   guint          digits)
{
  ClayLabelSpinPrivate *priv = clay_label_spin_get_instance_private (spin);
  gint                  width = 0;

  g_return_if_fail (CLAY_IS_LABEL_SPIN (spin));

  /* Necessary size to display the max/min integer values, with optional
   * negative sign.
   */
  width = (gint) floor (log10 (upper) + 1) + (upper < 0.0 ? 1 : 0);
  width = MAX (width, (gint) floor (log10 (lower) + 1) + (lower < 0.0 ? 1 : 0));

  /* Adding decimal digits and separator. */
  width += (digits > 0 ? 1 + digits : 0);

  /* Overlong spin button are useless. */
  width = MIN (10, width);

  gtk_entry_set_width_chars (GTK_ENTRY (priv->spinbutton), width);
}

static void
clay_label_spin_update_settings (ClayLabelSpin *spin)
{
  ClayLabelSpinPrivate *priv = clay_label_spin_get_instance_private (spin);
  gdouble               lower;
  gdouble               upper;
  gdouble               step;
  gdouble               page;
  gint                  digits = priv->digits;
  gboolean              adjust_step = (digits == 0);

  g_return_if_fail (CLAY_IS_LABEL_SPIN (spin));

  g_object_get (spin,
                "lower",  &lower,
                "upper",  &upper,
                NULL);

  g_return_if_fail (upper >= lower);

  clay_range_estimate_settings (lower, upper, &step, &page,
                                digits < 0 ? &digits: NULL);

  if (adjust_step && digits == 0 && step < 1.0)
    {
      step = 1.0;
      if (step > upper-lower)
        step = upper-lower;
      if (page < step || page > upper-lower)
        page = step;
    }

  clay_label_spin_set_increments (spin, step, page);
  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (priv->spinbutton), (guint) digits);
  clay_label_spin_update_spin_width (spin, lower, upper, (guint) digits);
}


/* Public Functions */


/**
 * clay_label_spin_new:
 * @text:   The text for the #GtkLabel.
 * @value:  The initial value.
 * @lower:  The lower boundary.
 * @upper:  The upper boundary.
 * @digits: The number of decimal digits.
 *
 * Suitable increment values are estimated based on the [@lower, @upper]
 * range.
 * If @digits is -1, then it will also be estimated based on the same
 * range. Digits estimation will always be at least 1, so if you want to
 * show integer values only, set 0 explicitly.
 *
 * Returns: (transfer full): The new #ClayLabelSpin widget.
 **/
GtkWidget *
clay_label_spin_new (const gchar *text,
                     gdouble      value,
                     gdouble      lower,
                     gdouble      upper,
                     gint         digits)
{
  GtkWidget *labeled;

  g_return_val_if_fail (upper >= lower, NULL);
  g_return_val_if_fail (digits >= -1, NULL);

  labeled = g_object_new (CLAY_TYPE_LABEL_SPIN,
                          "label",  text,
                          "value",  value,
                          "lower",  lower,
                          "upper",  upper,
                          "digits", digits,
                          NULL);

  return labeled;
}

/**
 * clay_label_spin_set_value:
 * @spin: The #GtkLabelSpin.
 * @value: A new value.
 *
 * This function sets the value shown by @spin.
 **/
void
clay_label_spin_set_value (ClayLabelSpin *spin,
                           gdouble         value)
{
  g_return_if_fail (CLAY_IS_LABEL_SPIN (spin));

  g_object_set (spin,
                "value", value,
                NULL);
}

/**
 * clay_label_spin_get_value:
 * @spin: The #GtkLabelSpin.
 *
 * This function returns the value shown by @spin.
 *
 * Returns: The value currently set.
 **/
gdouble
clay_label_spin_get_value (ClayLabelSpin *spin)
{
  gdouble value;

  g_return_val_if_fail (CLAY_IS_LABEL_SPIN (spin), 0.0);

  g_object_get (spin,
                "value", &value,
                NULL);
  return value;
}

/**
 * clay_label_spin_set_increments:
 * @spin: the #ClayLabelSpin.
 * @step: the step increment.
 * @page: the page increment.
 *
 * Set the step and page increments of the spin button.
 * By default, these increment values are automatically computed
 * depending on the range based on common usage. So you will likely not
 * need to run this for most case. Yet if you want specific increments
 * (which the widget cannot guess), you can call this function.
 */
void
clay_label_spin_set_increments (ClayLabelSpin *spin,
                                gdouble        step,
                                gdouble        page)
{
  ClayLabelSpinPrivate *priv = clay_label_spin_get_instance_private (spin);
  GtkSpinButton        *spinbutton;
  gdouble               lower;
  gdouble               upper;

  g_return_if_fail (CLAY_IS_LABEL_SPIN (spin));
  g_return_if_fail (step <= page);

  spinbutton = GTK_SPIN_BUTTON (priv->spinbutton);

  gtk_spin_button_get_range (spinbutton, &lower, &upper);
  g_return_if_fail (upper >= lower);
  g_return_if_fail ((upper == lower || step <= upper - lower) &&
                    (upper == lower || page <= upper - lower));

  g_object_freeze_notify (G_OBJECT (spinbutton));
  gtk_adjustment_set_step_increment (gtk_spin_button_get_adjustment (spinbutton), step);
  gtk_adjustment_set_page_increment (gtk_spin_button_get_adjustment (spinbutton), page);
  g_object_thaw_notify (G_OBJECT (spinbutton));

  g_object_set (priv->spinbutton,
                "climb-rate", step,
                NULL);
}

/**
 * clay_label_spin_set_digits:
 * @spin: the #ClayLabelSpin.
 * @digits: the number of decimal places to display.
 *
 * Set the number of decimal place to display in the @spin's entry.
 * If @digits is -1, then it will also be estimated based on @spin's
 * range. Digits estimation will always be at least 1, so if you want to
 * show integer values only, set 0 explicitly.
 */
void
clay_label_spin_set_digits (ClayLabelSpin *spin,
                            gint           digits)
{
  g_return_if_fail (CLAY_IS_LABEL_SPIN (spin));

  g_object_set (spin,
                "digits", digits,
                NULL);
}

/**
 * clay_label_spin_get_spin_button:
 * @spin: The #ClayLabelSpin
 *
 * This function returns the #ClaySpinButton packed in @spin.
 *
 * Returns: (transfer none): The #ClaySpinButton contained in @spin.
 **/
GtkWidget *
clay_label_spin_get_spin_button (ClayLabelSpin *spin)
{
  ClayLabelSpinPrivate *priv = clay_label_spin_get_instance_private (spin);

  g_return_val_if_fail (CLAY_IS_LABEL_SPIN (spin), NULL);

  return priv->spinbutton;
}
