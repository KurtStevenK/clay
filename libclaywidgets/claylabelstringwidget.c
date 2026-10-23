/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelstringwidget.c
 * Copyright (C) 2023 Jehan
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

#include "claylabelstringwidget.h"


/**
 * SECTION: claylabelstringwidget
 * @title: ClayLabelStringWidget
 * @short_description: Widget containing a label and a widget with a
 *                     string "value" property.
 *
 * This widget is a subclass of #ClayLabeled.
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
  PROP_WIDGET,
};

struct _ClayLabelStringWidget
{
  ClayLabeled    parent_instance;

  GtkWidget     *widget;
  gchar         *value;
};

static void        clay_label_string_widget_constructed       (GObject       *object);
static void        clay_label_string_widget_finalize          (GObject       *object);
static void        clay_label_string_widget_set_property      (GObject       *object,
                                                               guint          property_id,
                                                               const GValue  *value,
                                                               GParamSpec    *pspec);
static void        clay_label_string_widget_get_property      (GObject       *object,
                                                               guint          property_id,
                                                               GValue        *value,
                                                               GParamSpec    *pspec);

static GtkWidget * clay_label_string_widget_populate          (ClayLabeled   *widget,
                                                               gint          *x,
                                                               gint          *y,
                                                               gint          *width,
                                                               gint          *height);

G_DEFINE_TYPE (ClayLabelStringWidget, clay_label_string_widget, CLAY_TYPE_LABELED)

#define parent_class clay_label_string_widget_parent_class

static guint clay_label_string_widget_signals[LAST_SIGNAL] = { 0 };

static void
clay_label_string_widget_class_init (ClayLabelStringWidgetClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  ClayLabeledClass *labeled_class = CLAY_LABELED_CLASS (klass);

  clay_label_string_widget_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = clay_label_string_widget_constructed;
  object_class->finalize     = clay_label_string_widget_finalize;
  object_class->set_property = clay_label_string_widget_set_property;
  object_class->get_property = clay_label_string_widget_get_property;

  labeled_class->populate    = clay_label_string_widget_populate;

  /**
   * ClayLabelStringWidget:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_string ("value", NULL,
                                                        "Current value",
                                                        NULL,
                                                        CLAY_PARAM_READWRITE));

  /**
   * ClayLabelStringWidget:widget:
   *
   * The widget holding a string property named "value".
   *
   * Since: 3.0
   **/
  g_object_class_install_property (object_class, PROP_WIDGET,
                                   g_param_spec_object ("widget", NULL,
                                                        "String widget",
                                                        GTK_TYPE_WIDGET,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_label_string_widget_init (ClayLabelStringWidget *widget)
{
}

static void
clay_label_string_widget_constructed (GObject *object)
{
  ClayLabelStringWidget *widget = CLAY_LABEL_STRING_WIDGET (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_grid_set_column_spacing (GTK_GRID (widget), 6);
  gtk_grid_set_row_spacing (GTK_GRID (widget), 6);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (widget->widget), "value",
                          object,                  "value",
                          G_BINDING_BIDIRECTIONAL |
                          G_BINDING_SYNC_CREATE);
}

static void
clay_label_string_widget_finalize (GObject *object)
{
  ClayLabelStringWidget *widget = CLAY_LABEL_STRING_WIDGET (object);

  g_free (widget->value);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_label_string_widget_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayLabelStringWidget *widget = CLAY_LABEL_STRING_WIDGET (object);

  switch (property_id)
    {
    case PROP_VALUE:
      if (g_strcmp0 (widget->value, g_value_get_string (value)) != 0)
        {
          g_free (widget->value);
          widget->value = g_value_dup_string (value);
          g_signal_emit (object, clay_label_string_widget_signals[VALUE_CHANGED], 0);
        }
      break;
    case PROP_WIDGET:
      widget->widget = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_label_string_widget_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayLabelStringWidget *widget = CLAY_LABEL_STRING_WIDGET (object);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_string (value, widget->value);
      break;
    case PROP_WIDGET:
      g_value_set_object (value, widget->widget);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_label_string_widget_populate (ClayLabeled *labeled,
                                   gint        *x,
                                   gint        *y,
                                   gint        *width,
                                   gint        *height)
{
  ClayLabelStringWidget *widget = CLAY_LABEL_STRING_WIDGET (labeled);

  gtk_grid_attach (GTK_GRID (widget), widget->widget, 1, 0, 1, 1);
  gtk_widget_set_visible (widget->widget, TRUE);

  return widget->widget;
}

/* Public Functions */

/**
 * clay_label_string_widget_new:
 * @text:   The text for the #GtkLabel.
 * @widget: (transfer full): The #GtkWidget to use.
 *
 * Creates a new #ClayLabelStringWidget whose "value" property is bound to
 * that of @widget (which must therefore have such a string property).
 *
 * Returns: (transfer full): The new #ClayLabelStringWidget widget.
 **/
GtkWidget *
clay_label_string_widget_new (const gchar *text,
                              GtkWidget   *widget)
{
  GtkWidget  *string_widget;
  GParamSpec *pspec;

  g_return_val_if_fail ((pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (widget),
                                                               "value")) &&
                        (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
                         G_PARAM_SPEC_TYPE (pspec) == CLAY_TYPE_PARAM_CHOICE),
                        NULL);

  string_widget = g_object_new (CLAY_TYPE_LABEL_STRING_WIDGET,
                                "label",  text,
                                "widget", widget,
                                NULL);

  return string_widget;
}

/**
 * clay_label_string_widget_get_widget:
 * @widget: the #ClayLabelStringWidget.
 *
 * Returns: (transfer none): The new #GtkWidget packed next to the label.
 **/
GtkWidget *
clay_label_string_widget_get_widget (ClayLabelStringWidget *widget)
{
  g_return_val_if_fail (CLAY_IS_LABEL_STRING_WIDGET (widget), NULL);

  return widget->widget;
}
