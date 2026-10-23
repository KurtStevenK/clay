/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claylabelcolor.c
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
#include "claywidgets-private.h"


/**
 * SECTION: claylabelcolor
 * @title: ClayLabelColor
 * @short_description: Widget containing a color widget and a label.
 *
 * This widget is a subclass of #ClayLabeled with a #GtkColor.
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
  PROP_EDITABLE,
  N_PROPS
};
static GParamSpec *object_props[N_PROPS] = { NULL, };


struct _ClayLabelColor
{
  ClayLabeled  parent_instance;

  GtkWidget   *area;
  gboolean     editable;
};

static void        clay_label_color_constructed       (GObject       *object);
static void        clay_label_color_set_property      (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void        clay_label_color_get_property      (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);

static GtkWidget * clay_label_color_populate          (ClayLabeled   *color,
                                                       gint          *x,
                                                       gint          *y,
                                                       gint          *width,
                                                       gint          *height);


G_DEFINE_TYPE (ClayLabelColor, clay_label_color, CLAY_TYPE_LABELED)

#define parent_class clay_label_color_parent_class

static guint clay_label_color_signals[LAST_SIGNAL] = { 0 };

static void
clay_label_color_class_init (ClayLabelColorClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  ClayLabeledClass *labeled_class = CLAY_LABELED_CLASS (klass);

  clay_label_color_signals[VALUE_CHANGED] =
    g_signal_new ("value-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed  = clay_label_color_constructed;
  object_class->set_property = clay_label_color_set_property;
  object_class->get_property = clay_label_color_get_property;

  labeled_class->populate    = clay_label_color_populate;

  babl_init ();
  /**
   * ClayLabelColor:value:
   *
   * The currently set value.
   *
   * Since: 3.0
   **/
  object_props[PROP_VALUE] = clay_param_spec_color_from_string ("value",
                                                                "Color",
                                                                "The displayed color",
                                                                TRUE, "black",
                                                                CLAY_PARAM_READWRITE    |
                                                                G_PARAM_CONSTRUCT);

  /**
   * ClayLabelColor:editable:
   *
   * Whether the color can be edited.
   *
   * Since: 3.0
   **/
  object_props[PROP_EDITABLE] = g_param_spec_boolean ("editable",
                                                      "Whether the color can be edited",
                                                      "Whether the color can be edited",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, object_props);
}

static void
clay_label_color_init (ClayLabelColor *color)
{
  GeglColor *black = gegl_color_new ("black");

  color->editable = FALSE;
  color->area = clay_color_area_new (black, CLAY_COLOR_AREA_SMALL_CHECKS,
                                    GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);

  /* Typically for a labelled color area, a small square next to your
   * label is probably what you want to display.
   */
  gtk_widget_set_size_request (color->area, 20, 20);

  g_object_unref (black);
}

static void
clay_label_color_constructed (GObject *object)
{
  ClayLabelColor *color = CLAY_LABEL_COLOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  /* This is important to make this object into a property widget. It
   * will allow config object to bind the "value" property of this
   * widget, and therefore be updated automatically.
   */
  g_object_bind_property (G_OBJECT (color->area), "color",
                          G_OBJECT (color),       "value",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
clay_label_color_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayLabelColor *lcolor = CLAY_LABEL_COLOR (object);

  switch (property_id)
    {
    case PROP_VALUE:
        {
          GeglColor *new_color;
          GeglColor *color;

          new_color = g_value_get_object (value);

          g_object_get (lcolor->area,
                        "color", &color,
                        NULL);

          /* Avoid looping forever since we have bound this widget's
           * "value" property with the color button "value" property.
           */
          if (! clay_color_is_perceptually_identical (color, new_color))
            {
              g_object_set (lcolor->area, "color", new_color, NULL);
              g_signal_emit (object, clay_label_color_signals[VALUE_CHANGED], 0);
            }
          g_object_unref (color);
        }
      break;
    case PROP_EDITABLE:
      if (lcolor->editable != g_value_get_boolean (value))
        {
          const gchar       *dialog_title;
          ClayLabeled       *labeled;
          GeglColor         *color;
          ClayColorAreaType  type;
          gboolean           attached;

          labeled = CLAY_LABELED (lcolor);
          /* Reuse the label contents (without mnemonics) as dialog
           * title for the color selection. This is why the "editable"
           * property must not be a G_PARAM_CONSTRUCT.
           */
          dialog_title = gtk_label_get_text (GTK_LABEL (clay_labeled_get_label (labeled)));

          attached = (gtk_widget_get_parent (lcolor->area) != NULL);
          g_object_get (lcolor->area,
                        "type",  &type,
                        "color", &color,
                        NULL);

          gtk_widget_destroy (lcolor->area);

          lcolor->editable = g_value_get_boolean (value);

          if (lcolor->editable)
            lcolor->area = clay_color_button_new (dialog_title,
                                                20, 20, color, type);
          else
            lcolor->area = clay_color_area_new (color, type,
                                              GDK_BUTTON1_MASK | GDK_BUTTON2_MASK);
          g_object_unref (color);

          gtk_widget_set_size_request (lcolor->area, 20, 20);
          g_object_bind_property (G_OBJECT (lcolor->area), "color",
                                  G_OBJECT (lcolor),     "value",
                                  G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

          if (attached)
            {
              gtk_grid_attach (GTK_GRID (lcolor), lcolor->area, 1, 0, 1, 1);
              gtk_widget_set_visible (lcolor->area, TRUE);
              g_signal_emit_by_name (object, "mnemonic-widget-changed", lcolor->area);
            }
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_label_color_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayLabelColor *lcolor = CLAY_LABEL_COLOR (object);

  switch (property_id)
    {
    case PROP_VALUE:
      {
        GeglColor *color;

        g_object_get (lcolor->area,
                      "color", &color,
                      NULL);
        g_value_take_object (value, color);
      }
      break;
    case PROP_EDITABLE:
      g_value_set_boolean (value, lcolor->editable);
      break;


    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
clay_label_color_populate (ClayLabeled *labeled,
                           gint        *x,
                           gint        *y,
                           gint        *width,
                           gint        *height)
{
  ClayLabelColor *color = CLAY_LABEL_COLOR (labeled);

  gtk_grid_attach (GTK_GRID (color), color->area, 1, 0, 1, 1);
  /* Make sure the label and color won't be glued next to each other's. */
  gtk_grid_set_column_spacing (GTK_GRID (color),
                               4 * gtk_widget_get_scale_factor (GTK_WIDGET (color)));
  gtk_widget_set_visible (color->area, TRUE);

  return color->area;
}


/* Public Functions */


/**
 * clay_label_color_new:
 * @label:  The text for the #GtkLabel.
 * @color:  The color displayed.
 *
 * Creates a #ClayLabelColor which contains a widget and displays a
 * color area. By default, the color area is of type
 * %CLAY_COLOR_AREA_SMALL_CHECKS, which means transparency of @color
 * will be shown.
 *
 * Moreover in the non-editable case, the color is draggable to other
 * widgets accepting color drops with buttons 1 and 2.
 * In the editable case, the @label is reused as the color chooser's
 * dialog title.
 *
 * If you wish to customize any of these default behaviors, get the
 * #ClayColorArea or #ClayColorButton with clay_label_color_get_color_widget().
 *
 * Returns: (transfer full): The new #ClayLabelColor widget.
 **/
GtkWidget *
clay_label_color_new (const gchar *label,
                      GeglColor   *color,
                      gboolean     editable)
{
  GtkWidget *labeled;

  labeled = g_object_new (CLAY_TYPE_LABEL_COLOR,
                          "label",    label,
                          "value",    color,
                          "editable", editable,
                          NULL);

  return labeled;
}

/**
 * clay_label_color_set_value:
 * @color: The #GtkLabelColor.
 * @value: A new value.
 *
 * This function sets the value in the #GtkColor inside @color.
 **/
void
clay_label_color_set_value (ClayLabelColor *color,
                            GeglColor      *value)
{
  g_return_if_fail (CLAY_IS_LABEL_COLOR (color));
  g_return_if_fail (GEGL_IS_COLOR (value));

  g_object_set (color,
                "value", value,
                NULL);
}

/**
 * clay_label_color_get_value:
 * @color: The #GtkLabelColor.
 *
 * This function returns the value shown by @color.
 *
 * Returns: (transfer full): a copy of the [class@Gegl.Color] used by the widget.
 **/
GeglColor *
clay_label_color_get_value (ClayLabelColor *color)
{
  GeglColor *value = NULL;
  GeglColor *retval;

  g_return_val_if_fail (CLAY_IS_LABEL_COLOR (color), NULL);

  g_object_get (color->area,
                "color", &value,
                NULL);
  retval = gegl_color_duplicate (value);
  g_clear_object (&value);

  return retval;
}

/**
 * clay_label_color_set_editable:
 * @color:    The #GtkLabelColor.
 * @editable: Whether the color should be editable.
 *
 * Changes the editability of the color.
 **/
void
clay_label_color_set_editable (ClayLabelColor *color,
                               gboolean        editable)
{
  g_return_if_fail (CLAY_IS_LABEL_COLOR (color));

  g_object_set (color, "editable", editable, NULL);
}

/**
 * clay_label_color_is_editable:
 * @color: The #GtkLabelColor.
 *
 * This function tells whether the color widget allows to edit the
 * color.
 * Returns: %TRUE if the color is editable.
 **/
gboolean
clay_label_color_is_editable (ClayLabelColor *color)
{
  g_return_val_if_fail (CLAY_IS_LABEL_COLOR (color), FALSE);

  return CLAY_IS_COLOR_SELECT (color->area);
}

/**
 * clay_label_color_get_color_widget:
 * @color: The #ClayLabelColor
 *
 * This function returns the color widget packed in @color, which can be
 * either a #ClayColorButton (if the @color is editable) or a
 * #ClayColorArea otherwise.
 *
 * Returns: (transfer none): The color widget packed in @color.
 **/
GtkWidget *
clay_label_color_get_color_widget (ClayLabelColor *color)
{
  g_return_val_if_fail (CLAY_IS_LABEL_COLOR (color), NULL);

  return color->area;
}
