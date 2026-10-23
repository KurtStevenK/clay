/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claynumberpairentry.c
 * Copyright (C) 2006  Simon Budig       <simon@gimp.org>
 * Copyright (C) 2007  Sven Neumann      <sven@gimp.org>
 * Copyright (C) 2007  Martin Nordholts  <martin@svn.gnome.org>
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

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "claywidgetstypes.h"

#include "clayicons.h"
#include "claynumberpairentry.h"


/**
 * SECTION: claynumberpairentry
 * @title: ClayNumberPairEntry
 * @short_description: A #GtkEntry subclass to enter ratios.
 *
 * A #GtkEntry subclass to enter ratios.
 **/


#define EPSILON 0.000001


enum
{
  NUMBERS_CHANGED,
  RATIO_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LEFT_NUMBER,
  PROP_RIGHT_NUMBER,
  PROP_DEFAULT_LEFT_NUMBER,
  PROP_DEFAULT_RIGHT_NUMBER,
  PROP_USER_OVERRIDE,
  PROP_SEPARATORS,
  PROP_DEFAULT_TEXT,
  PROP_ALLOW_SIMPLIFICATION,
  PROP_MIN_VALID_VALUE,
  PROP_MAX_VALID_VALUE,
  PROP_RATIO,
  PROP_ASPECT
};

typedef enum
{
  PARSE_VALID,
  PARSE_CLEAR,
  PARSE_INVALID
} ParseResult;


struct _ClayNumberPairEntry
{
  GtkEntry     parent_instance;

  /* The current number pair displayed in the widget. */
  gdouble      left_number;
  gdouble      right_number;

  /* What number pair that should be displayed when not in
     user_override mode. */
  gdouble      default_left_number;
  gdouble      default_right_number;

  /* Whether or not the current value in the entry has been explicitly
   * set by the user.
   */
  gboolean     user_override;

  /* Is the font style currently set to ITALIC or NORMAL ? */
  gboolean     font_italic;

  /* What separators that are valid when parsing input, e.g. when the
   * widget is used for aspect ratio, valid separators are typically
   * ':' and '/'.
   */
  gunichar    *separators;
  glong        num_separators;

  /* A string to be shown in the entry when in automatic mode */
  gchar       *default_text;

  /* Whether or to not to divide the numbers with the greatest common
   * divisor when input ends in '='.
   */
  gboolean     allow_simplification;

  /* What range of values considered valid. */
  gdouble      min_valid_value;
  gdouble      max_valid_value;
};


static void         clay_number_pair_entry_finalize          (GObject             *entry);

static gboolean     clay_number_pair_entry_valid_separator   (ClayNumberPairEntry *entry,
                                                              gunichar             candidate);
static void         clay_number_pair_entry_ratio_to_fraction (gdouble              ratio,
                                                              gdouble             *numerator,
                                                              gdouble             *denominator);

static void         clay_number_pair_entry_set_property      (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);
static void         clay_number_pair_entry_get_property      (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);
static void         clay_number_pair_entry_changed           (ClayNumberPairEntry *entry);
static void         clay_number_pair_entry_icon_press        (ClayNumberPairEntry *entry);
static gboolean     clay_number_pair_entry_events            (GtkWidget           *widget,
                                                              GdkEvent            *event);

static void         clay_number_pair_entry_update_text       (ClayNumberPairEntry *entry);

static ParseResult  clay_number_pair_entry_parse_text        (ClayNumberPairEntry *entry,
                                                              const gchar         *text,
                                                              gdouble             *left_value,
                                                              gdouble             *right_value);
static gboolean     clay_number_pair_entry_numbers_in_range  (ClayNumberPairEntry *entry,
                                                              gdouble              left_number,
                                                              gdouble              right_number);

static gchar *      clay_number_pair_entry_strdup_number_pair_string
                                                             (ClayNumberPairEntry *entry,
                                                              gdouble left_number,
                                                              gdouble right_number);



G_DEFINE_TYPE (ClayNumberPairEntry, clay_number_pair_entry, GTK_TYPE_ENTRY)


#define parent_class clay_number_pair_entry_parent_class

/* What the user shall end the input with when simplification is desired. */
#define SIMPLIFICATION_CHAR  ((gunichar) '=')

#define DEFAULT_SEPARATOR    ((gunichar) ',')


static guint entry_signals[LAST_SIGNAL] = { 0 };


static void
clay_number_pair_entry_class_init (ClayNumberPairEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  entry_signals[NUMBERS_CHANGED] =
    g_signal_new ("numbers-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  entry_signals[RATIO_CHANGED] =
    g_signal_new ("ratio-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property = clay_number_pair_entry_set_property;
  object_class->get_property = clay_number_pair_entry_get_property;
  object_class->finalize     = clay_number_pair_entry_finalize;

  g_object_class_install_property (object_class, PROP_LEFT_NUMBER,
                                   g_param_spec_double ("left-number",
                                                        "Left number",
                                                        "The left number",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_RIGHT_NUMBER,
                                   g_param_spec_double ("right-number",
                                                        "Right number",
                                                        "The right number",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEFAULT_LEFT_NUMBER,
                                   g_param_spec_double ("default-left-number",
                                                        "Default left number",
                                                        "The default left number",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEFAULT_RIGHT_NUMBER,
                                   g_param_spec_double ("default-right-number",
                                                        "Default right number",
                                                        "The default right number",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        100.0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_USER_OVERRIDE,
                                   g_param_spec_boolean ("user-override",
                                                         "User override",
                                                         "Whether the widget is in 'user override' mode",
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SEPARATORS,
                                   g_param_spec_string ("separators",
                                                        "Separators",
                                                        "A string of valid separators",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DEFAULT_TEXT,
                                   g_param_spec_string ("default-text",
                                                        "Default text",
                                                        "String to show when in automatic mode",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ALLOW_SIMPLIFICATION,
                                   g_param_spec_boolean ("allow-simplification",
                                                         "Allow simplification",
                                                         "Whether to allow simplification",
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MIN_VALID_VALUE,
                                   g_param_spec_double ("min-valid-value",
                                                        "Min valid value",
                                                        "Minimum value valid when parsing input",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        G_MINDOUBLE,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_MAX_VALID_VALUE,
                                   g_param_spec_double ("max-valid-value",
                                                        "Max valid value",
                                                        "Maximum value valid when parsing input",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_RATIO,
                                   g_param_spec_double ("ratio",
                                                        "Ratio",
                                                        "The value as ratio",
                                                        G_MINDOUBLE, G_MAXDOUBLE,
                                                        1.0,
                                                        CLAY_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ASPECT,
                                   g_param_spec_enum ("aspect",
                                                      "Aspect",
                                                      "The value as aspect",
                                                      CLAY_TYPE_ASPECT_TYPE,
                                                      CLAY_ASPECT_SQUARE,
                                                      CLAY_PARAM_READWRITE));
}

static void
clay_number_pair_entry_init (ClayNumberPairEntry *entry)
{
  entry->left_number          = 1.0;
  entry->right_number         = 1.0;
  entry->default_left_number  = 1.0;
  entry->default_right_number = 1.0;
  entry->user_override        = FALSE;
  entry->font_italic          = FALSE;
  entry->separators           = NULL;
  entry->default_text         = NULL;
  entry->num_separators       = 0;
  entry->allow_simplification = FALSE;
  entry->min_valid_value      = G_MINDOUBLE;
  entry->max_valid_value      = G_MAXDOUBLE;

  g_signal_connect (entry, "changed",
                    G_CALLBACK (clay_number_pair_entry_changed),
                    NULL);
  g_signal_connect (entry, "icon-press",
                    G_CALLBACK (clay_number_pair_entry_icon_press),
                    NULL);
  g_signal_connect (entry, "focus-out-event",
                    G_CALLBACK (clay_number_pair_entry_events),
                    NULL);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (clay_number_pair_entry_events),
                    NULL);

  gtk_widget_set_direction (GTK_WIDGET (entry), GTK_TEXT_DIR_LTR);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     CLAY_ICON_EDIT_CLEAR);
}

static void
clay_number_pair_entry_finalize (GObject *object)
{
  ClayNumberPairEntry *entry = CLAY_NUMBER_PAIR_ENTRY (object);

  g_clear_pointer (&entry->separators, g_free);
  entry->num_separators = 0;

  g_clear_pointer (&entry->default_text, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * clay_number_pair_entry_new:
 * @separators:           The allowed separators.
 * @allow_simplification: Whether to do simplification on the entered term.
 * @min_valid_value:      The minimum allowed result value.
 * @max_valid_value:      The maximum allowed result value.
 *
 * Creates a new #ClayNumberPairEntry widget, which is a GtkEntry that
 * accepts two numbers separated by a separator. Typical input example
 * with a 'x' separator: "377x233".
 *
 * The widget supports simplification of the entered ratio when the
 * input ends in '=', if "allow-simplification" is TRUE.
 *
 * The "separators" property contains a string of characters valid as
 * separators when parsing input. The first separator is used when
 * displaying the current values.
 *
 * It is possible to specify what range of values that shall be
 * considered as valid when parsing user input, by changing
 * "min-valid-value" and "max-valid-value".
 *
 * The first separator of @separators is used to display the current
 * value.
 *
 * Returns: The new #ClayNumberPairEntry widget.
 *
 * Since: 2.4
 **/
GtkWidget *
clay_number_pair_entry_new (const gchar *separators,
                            gboolean     allow_simplification,
                            gdouble      min_valid_value,
                            gdouble      max_valid_value)
{
  return g_object_new (CLAY_TYPE_NUMBER_PAIR_ENTRY,
                       "separators",           separators,
                       "allow-simplification", allow_simplification,
                       "min-valid-value",      min_valid_value,
                       "max-valid-value",      max_valid_value,
                       NULL);
}

static void
clay_number_pair_entry_ratio_to_fraction (gdouble  ratio,
                                          gdouble *numerator,
                                          gdouble *denominator)
{
  gdouble  remainder, next_cf;
  gint     p0, p1, p2;
  gint     q0, q1, q2;

  /* calculate the continued fraction to approximate the desired ratio */

  p0 = 1;
  q0 = 0;
  p1 = floor (ratio);
  q1 = 1;

  remainder = ratio - p1;

  while (fabs (remainder) >= 0.0001 &&
         fabs (((gdouble) p1 / q1) - ratio) > 0.0001)
    {
      remainder = 1.0 / remainder;

      next_cf = floor (remainder);

      p2 = next_cf * p1 + p0;
      q2 = next_cf * q1 + q0;

      /* remember the last two fractions */
      p0 = p1;
      q0 = q1;
      p1 = p2;
      q1 = q2;

      remainder = remainder - next_cf;
    }

  /* only use the calculated fraction if it is "reasonable" */
  if (p1 < 1000 && q1 < 1000)
    {
      *numerator   = p1;
      *denominator = q1;
    }
  else
    {
      *numerator   = ratio;
      *denominator = 1.0;
    }
}

/**
 * clay_number_pair_entry_set_ratio:
 * @entry: A #ClayNumberPairEntry widget.
 * @ratio: Ratio to set in the widget.
 *
 * Sets the numbers of the #ClayNumberPairEntry to have the desired
 * ratio. If the new ratio is different than the previous ratio, the
 * "ratio-changed" signal is emitted.
 *
 * An attempt is made to convert the decimal number into a fraction
 * with left_number and right_number < 1000.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_set_ratio (ClayNumberPairEntry *entry,
                                  gdouble              ratio)
{
  gdouble numerator;
  gdouble denominator;

  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  clay_number_pair_entry_ratio_to_fraction (ratio, &numerator, &denominator);

  clay_number_pair_entry_set_values (entry, numerator, denominator);
}

/**
 * clay_number_pair_entry_get_ratio:
 * @entry: A #ClayNumberPairEntry widget.
 *
 * Retrieves the ratio of the numbers displayed by a #ClayNumberPairEntry.
 *
 * Returns: The ratio value.
 *
 * Since: 2.4
 **/
gdouble
clay_number_pair_entry_get_ratio (ClayNumberPairEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry), 1.0);

  return entry->left_number / entry->right_number;
}

/**
 * clay_number_pair_entry_set_values:
 * @entry: A #ClayNumberPairEntry widget.
 * @left:  Left number in the entry.
 * @right: Right number in the entry.
 *
 * Forces setting the numbers displayed by a #ClayNumberPairEntry,
 * ignoring if the user has set their own value. The state of
 * user-override will not be changed.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_set_values (ClayNumberPairEntry *entry,
                                   gdouble              left,
                                   gdouble              right)
{
  ClayAspectType old_aspect;
  gdouble        old_ratio;
  gdouble        old_left_number;
  gdouble        old_right_number;
  gboolean       numbers_changed = FALSE;
  gboolean       ratio_changed   = FALSE;

  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  /* Store current values */

  old_left_number  = entry->left_number;
  old_right_number = entry->right_number;
  old_ratio        = clay_number_pair_entry_get_ratio (entry);
  old_aspect       = clay_number_pair_entry_get_aspect (entry);


  /* Freeze notification */

  g_object_freeze_notify (G_OBJECT (entry));


  /* Set the new numbers and update the entry */

  entry->left_number  = left;
  entry->right_number = right;

  g_object_notify (G_OBJECT (entry), "left-number");
  g_object_notify (G_OBJECT (entry), "right-number");

  clay_number_pair_entry_update_text (entry);


  /* Find out what has changed */

  if (fabs (old_ratio - clay_number_pair_entry_get_ratio (entry)) > EPSILON)
    {
      g_object_notify (G_OBJECT (entry), "ratio");

      ratio_changed = TRUE;

      if (old_aspect != clay_number_pair_entry_get_aspect (entry))
        g_object_notify (G_OBJECT (entry), "aspect");
    }

  if (old_left_number  != entry->left_number ||
      old_right_number != entry->right_number)
    {
      numbers_changed = TRUE;
    }


  /* Thaw */

  g_object_thaw_notify (G_OBJECT (entry));


  /* Emit relevant signals */

  if (numbers_changed)
    g_signal_emit (entry, entry_signals[NUMBERS_CHANGED], 0);

  if (ratio_changed)
    g_signal_emit (entry, entry_signals[RATIO_CHANGED], 0);
}

/**
 * clay_number_pair_entry_get_values:
 * @entry: A #ClayNumberPairEntry widget.
 * @left:  (out) (optional): Location to store the left number (may be %NULL).
 * @right: (out) (optional): Location to store the right number (may be %NULL).
 *
 * Gets the numbers displayed by a #ClayNumberPairEntry.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_get_values (ClayNumberPairEntry *entry,
                                   gdouble             *left,
                                   gdouble             *right)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  if (left != NULL)
    *left  = entry->left_number;

  if (right != NULL)
    *right = entry->right_number;
}

/**
 * clay_number_pair_entry_set_default_text:
 * @entry:  A #ClayNumberPairEntry widget.
 * @string: Default string.
 *
 * Causes the entry to show a given string when in automatic mode,
 * instead of the default numbers. The only thing this does is making
 * the #ClayNumberPairEntry showing this string, the internal state
 * and API calls are not affected.
 *
 * Set the default string to %NULL to display default values as
 * normal.
 *
 * Since: 2.4
 */
void
clay_number_pair_entry_set_default_text (ClayNumberPairEntry *entry,
                                         const gchar         *string)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  g_free (entry->default_text);
  entry->default_text = g_strdup (string);

  clay_number_pair_entry_update_text (entry);

  g_object_notify (G_OBJECT (entry), "default-text");
}

/**
 * clay_number_pair_entry_get_default_text:
 * @entry:  A #ClayNumberPairEntry widget.
 *
 * Returns: (nullable): the string manually set to be shown,
 *          or %NULL if values are shown in a normal fashion.
 *
 * Since: 2.4
 */
const gchar *
clay_number_pair_entry_get_default_text (ClayNumberPairEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry), NULL);

  return entry->default_text;
}

/**
 * clay_number_pair_entry_set_aspect:
 * @entry:  A #ClayNumberPairEntry widget.
 * @aspect: The new aspect.
 *
 * Sets the aspect of the ratio by swapping the left_number and
 * right_number if necessary (or setting them to 1.0 in case that
 * @aspect is %CLAY_ASPECT_SQUARE).
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_set_aspect (ClayNumberPairEntry *entry,
                                   ClayAspectType       aspect)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  if (clay_number_pair_entry_get_aspect (entry) == aspect)
    return;

  switch (aspect)
    {
    case CLAY_ASPECT_SQUARE:
      clay_number_pair_entry_set_values (entry,
                                         entry->left_number,
                                         entry->left_number);
      break;

    case CLAY_ASPECT_LANDSCAPE:
    case CLAY_ASPECT_PORTRAIT:
      clay_number_pair_entry_set_values (entry,
                                         entry->right_number,
                                         entry->left_number);
      break;
    }
}

/**
 * clay_number_pair_entry_get_aspect:
 * @entry: A #ClayNumberPairEntry widget.
 *
 * Gets the aspect of the ratio displayed by a #ClayNumberPairEntry.
 *
 * Returns: The entry's current aspect.
 *
 * Since: 2.4
 **/
ClayAspectType
clay_number_pair_entry_get_aspect (ClayNumberPairEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry), CLAY_ASPECT_SQUARE);

  if (entry->left_number > entry->right_number)
    {
      return CLAY_ASPECT_LANDSCAPE;
    }
  else if (entry->left_number < entry->right_number)
    {
      return CLAY_ASPECT_PORTRAIT;
    }
  else
    {
      return CLAY_ASPECT_SQUARE;
    }
}

static void
clay_number_pair_entry_modify_font (ClayNumberPairEntry *entry,
                                    gboolean             italic)
{
  GtkStyleContext *style;

  if (entry->font_italic == italic)
    return;

  style = gtk_widget_get_style_context (GTK_WIDGET (entry));

  if (italic)
    gtk_style_context_add_class (style, "italic");
  else
    gtk_style_context_remove_class (style, "italic");

  gtk_entry_set_icon_sensitive (GTK_ENTRY (entry),
                                GTK_ENTRY_ICON_SECONDARY,
                                ! italic);

  entry->font_italic = italic;
}


/**
 * clay_number_pair_entry_set_user_override:
 * @entry:         A #ClayNumberPairEntry widget.
 * @user_override: %TRUE sets the entry in user overridden mode,
 *                 %FALSE disables.
 *
 * When the entry is not in user overridden mode, the values will
 * change when the default values are changed. When in user overridden
 * mode, setting default values will not affect the active values.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_set_user_override (ClayNumberPairEntry *entry,
                                          gboolean             user_override)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  entry->user_override = user_override;

  if (! user_override)
    {
      clay_number_pair_entry_set_default_values (entry,
                                                 entry->default_left_number,
                                                 entry->default_right_number);
    }

  clay_number_pair_entry_modify_font (entry, ! user_override);

  g_object_notify (G_OBJECT (entry), "user-override");
}

/**
 * clay_number_pair_entry_get_user_override:
 * @entry: A #ClayNumberPairEntry widget.
 *
 * Returns: Whether or not the the widget is in user overridden mode.
 *
 * Since: 2.4
 **/
gboolean
clay_number_pair_entry_get_user_override (ClayNumberPairEntry *entry)
{
  g_return_val_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry), FALSE);

  return entry->user_override;
}

static void
clay_number_pair_entry_changed (ClayNumberPairEntry *entry)
{
  clay_number_pair_entry_modify_font (entry, FALSE);
}

static void
clay_number_pair_entry_icon_press (ClayNumberPairEntry *entry)
{
  clay_number_pair_entry_set_user_override (entry, FALSE);

  gtk_editable_set_position (GTK_EDITABLE (entry), -1);
}

static gboolean
clay_number_pair_entry_events (GtkWidget *widget,
                               GdkEvent  *event)
{
  ClayNumberPairEntry *entry = CLAY_NUMBER_PAIR_ENTRY (widget);
  gboolean             force_user_override;

  force_user_override = FALSE;

  switch (event->type)
    {
    case GDK_KEY_PRESS:
      {
        GdkEventKey *kevent = (GdkEventKey *) event;

        if (kevent->keyval != GDK_KEY_Return   &&
            kevent->keyval != GDK_KEY_KP_Enter &&
            kevent->keyval != GDK_KEY_ISO_Enter)
          break;

        /* If parsing was done due to widgets focus being lost, we only change
         * to user-override mode if the values differ from the default ones. If
         * Return was pressed however, we always switch to user-override mode.
         */
        force_user_override = TRUE;
      }
      /* Fall through */

    case GDK_FOCUS_CHANGE:
      {
        const gchar *text;
        ParseResult  parse_result;
        gdouble      left_value;
        gdouble      right_value;

        text = gtk_entry_get_text (GTK_ENTRY (entry));

        parse_result = clay_number_pair_entry_parse_text (entry,
                                                          text,
                                                          &left_value,
                                                          &right_value);
        switch (parse_result)
          {
          case PARSE_VALID:
            {
              if (entry->left_number  != left_value  ||
                  entry->right_number != right_value ||
                  force_user_override)
                {
                  clay_number_pair_entry_set_values (entry,
                                                     left_value,
                                                     right_value);

                  clay_number_pair_entry_set_user_override (entry, TRUE);
                }
            }
            break;

          case PARSE_CLEAR:
            clay_number_pair_entry_set_user_override (entry, FALSE);
            break;

          default:
            break;
          }

        /* Make sure the entry text is up to date */

        clay_number_pair_entry_update_text (entry);

        gtk_editable_set_position (GTK_EDITABLE (entry), -1);
      }
      break;

    default:
      break;
    }

  return FALSE;
}

/**
 * clay_number_pair_entry_strdup_number_pair_string:
 * @entry:
 * @left_number:
 * @right_number:
 *
 * Returns: allocated data, must be g_free:d.
 **/
static gchar *
clay_number_pair_entry_strdup_number_pair_string (ClayNumberPairEntry *entry,
                                                  gdouble              left_number,
                                                  gdouble              right_number)
{
  gchar sep[8];
  gint  len;

  if (entry->num_separators > 0)
    len = g_unichar_to_utf8 (entry->separators[0], sep);
  else
    len = g_unichar_to_utf8 (DEFAULT_SEPARATOR, sep);

  sep[len] = '\0';

  return g_strdup_printf ("%g%s%g", left_number, sep, right_number);
}

static void
clay_number_pair_entry_update_text (ClayNumberPairEntry *entry)
{
  gchar *buffer;

  if (! entry->user_override &&
      entry->default_text != NULL)
    {
      /* Instead of the numbers, show the string explicitly set by a
       * client to show when in automatic mode.
       */
      buffer = g_strdup (entry->default_text);
    }
  else
    {
      buffer = clay_number_pair_entry_strdup_number_pair_string (entry,
                                                                 entry->left_number,
                                                                 entry->right_number);
    }

  g_signal_handlers_block_by_func (entry,
                                   clay_number_pair_entry_changed, NULL);

  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
  g_free (buffer);

  g_signal_handlers_unblock_by_func (entry,
                                     clay_number_pair_entry_changed, NULL);

  clay_number_pair_entry_modify_font (entry, ! entry->user_override);
}

static gboolean
clay_number_pair_entry_valid_separator (ClayNumberPairEntry *entry,
                                        gunichar             candidate)
{
  if (entry->num_separators > 0)
    {
      gint i;

      for (i = 0; i < entry->num_separators; i++)
        if (entry->separators[i] == candidate)
          return TRUE;
    }
  else if (candidate == DEFAULT_SEPARATOR)
    {
      return TRUE;
    }

  return FALSE;
}

static ParseResult
clay_number_pair_entry_parse_text (ClayNumberPairEntry *entry,
                                   const gchar         *text,
                                   gdouble             *left_value,
                                   gdouble             *right_value)
{
  gdouble   new_left_number;
  gdouble   new_right_number;
  gboolean  simplify = FALSE;
  gchar    *end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check if clear */
  if (! *text)
    return PARSE_CLEAR;

  /* try to parse a number */
  new_left_number = strtod (text, &end);

  if (end == text)
    return PARSE_INVALID;
  else
    text = end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for a valid separator */
  if (! clay_number_pair_entry_valid_separator (entry, g_utf8_get_char (text)))
    return PARSE_INVALID;
  else
    text = g_utf8_next_char (text);

  /* try to parse another number */
  new_right_number = strtod (text, &end);

  if (end == text)
    return PARSE_INVALID;
  else
    text = end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for the simplification char */
  if (g_utf8_get_char (text) == SIMPLIFICATION_CHAR)
    {
      simplify = entry->allow_simplification;
      text = g_utf8_next_char (text);
    }

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* check for trailing garbage */
  if (*text)
    return PARSE_INVALID;

  if (! clay_number_pair_entry_numbers_in_range (entry,
                                                 new_left_number,
                                                 new_right_number))
    return PARSE_INVALID;

  if (simplify && new_right_number != 0.0)
    {
      clay_number_pair_entry_ratio_to_fraction (new_left_number /
                                                new_right_number,
                                                left_value,
                                                right_value);
    }
  else
    {
      *left_value = new_left_number;
      *right_value = new_right_number;
    }

  return PARSE_VALID;
}

static void
clay_number_pair_entry_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayNumberPairEntry        *entry = CLAY_NUMBER_PAIR_ENTRY (object);

  switch (property_id)
    {
    case PROP_LEFT_NUMBER:
      clay_number_pair_entry_set_values (entry,
                                         g_value_get_double (value),
                                         entry->right_number);
      break;
    case PROP_RIGHT_NUMBER:
      clay_number_pair_entry_set_values (entry,
                                         entry->left_number,
                                         g_value_get_double (value));
      break;
    case PROP_DEFAULT_LEFT_NUMBER:
      clay_number_pair_entry_set_default_values (entry,
                                                 g_value_get_double (value),
                                                 entry->default_right_number);
      break;
    case PROP_DEFAULT_RIGHT_NUMBER:
      clay_number_pair_entry_set_default_values (entry,
                                                 entry->default_left_number,
                                                 g_value_get_double (value));
      break;
    case PROP_USER_OVERRIDE:
      clay_number_pair_entry_set_user_override (entry,
                                                g_value_get_boolean (value));
      break;
    case PROP_SEPARATORS:
      g_free (entry->separators);
      entry->num_separators = 0;
      if (g_value_get_string (value))
        entry->separators = g_utf8_to_ucs4 (g_value_get_string (value), -1,
                                           NULL, &entry->num_separators, NULL);
      else
        entry->separators = NULL;
      break;
    case PROP_DEFAULT_TEXT:
      clay_number_pair_entry_set_default_text (entry,
                                               g_value_get_string (value));
      break;
    case PROP_ALLOW_SIMPLIFICATION:
      entry->allow_simplification = g_value_get_boolean (value);
      break;
    case PROP_MIN_VALID_VALUE:
      entry->min_valid_value = g_value_get_double (value);
      break;
    case PROP_MAX_VALID_VALUE:
      entry->max_valid_value = g_value_get_double (value);
      break;
    case PROP_RATIO:
      clay_number_pair_entry_set_ratio (entry, g_value_get_double (value));
      break;
    case PROP_ASPECT:
      clay_number_pair_entry_set_aspect (entry, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_number_pair_entry_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayNumberPairEntry *entry = CLAY_NUMBER_PAIR_ENTRY (object);

  switch (property_id)
    {
    case PROP_LEFT_NUMBER:
      g_value_set_double (value, entry->left_number);
      break;
    case PROP_RIGHT_NUMBER:
      g_value_set_double (value, entry->right_number);
      break;
    case PROP_DEFAULT_LEFT_NUMBER:
      g_value_set_double (value, entry->default_left_number);
      break;
    case PROP_DEFAULT_RIGHT_NUMBER:
      g_value_set_double (value, entry->default_right_number);
      break;
    case PROP_USER_OVERRIDE:
      g_value_set_boolean (value, entry->user_override);
      break;
    case PROP_SEPARATORS:
      g_value_take_string (value,
                           g_ucs4_to_utf8 (entry->separators,
                                           entry->num_separators,
                                           NULL, NULL, NULL));
      break;
    case PROP_ALLOW_SIMPLIFICATION:
      g_value_set_boolean (value, entry->allow_simplification);
      break;
    case PROP_DEFAULT_TEXT:
      g_value_set_string (value, entry->default_text);
      break;
    case PROP_MIN_VALID_VALUE:
      g_value_set_double (value, entry->min_valid_value);
      break;
    case PROP_MAX_VALID_VALUE:
      g_value_set_double (value, entry->max_valid_value);
      break;
    case PROP_RATIO:
      g_value_set_double (value, clay_number_pair_entry_get_ratio (entry));
      break;
    case PROP_ASPECT:
      g_value_set_enum (value, clay_number_pair_entry_get_aspect (entry));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * clay_number_pair_entry_set_default_values:
 * @entry: A #ClayNumberPairEntry widget.
 * @left:  Default left value in the entry.
 * @right: Default right value in the entry.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_set_default_values (ClayNumberPairEntry *entry,
                                           gdouble              left,
                                           gdouble              right)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  entry->default_left_number  = left;
  entry->default_right_number = right;

  if (! entry->user_override)
    {
      clay_number_pair_entry_set_values (entry,
                                         entry->default_left_number,
                                         entry->default_right_number);
    }
}

/**
 * clay_number_pair_entry_get_default_values:
 * @entry: A #ClayNumberPairEntry widget.
 * @left:  (out) (optional): Location to put the default left value, or %NULL.
 * @right: (out) (optional): Location to put the default right value, or %NULL.
 *
 * Since: 2.4
 **/
void
clay_number_pair_entry_get_default_values (ClayNumberPairEntry *entry,
                                           gdouble             *left,
                                           gdouble             *right)
{
  g_return_if_fail (CLAY_IS_NUMBER_PAIR_ENTRY (entry));

  if (left != NULL)
    *left = entry->default_left_number;

  if (right != NULL)
    *right = entry->default_right_number;
}

static gboolean
clay_number_pair_entry_numbers_in_range (ClayNumberPairEntry *entry,
                                         gdouble              left_number,
                                         gdouble              right_number)
{
  return (left_number  >= entry->min_valid_value &&
          left_number  <= entry->max_valid_value &&
          right_number >= entry->min_valid_value &&
          right_number <= entry->max_valid_value);
}
