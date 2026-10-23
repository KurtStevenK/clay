/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypivotselector.c
 * Copyright (C) 2019 Ell
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

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "claypivotselector.h"


#define EPSILON 1e-6


enum
{
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LEFT,
  PROP_TOP,
  PROP_RIGHT,
  PROP_BOTTOM,
  PROP_X,
  PROP_Y
};


struct _ClayPivotSelectorPrivate
{
  gdouble    left;
  gdouble    top;
  gdouble    right;
  gdouble    bottom;

  gdouble    x;
  gdouble    y;

  GtkWidget *buttons[9];
  GtkWidget *active_button;
};


/*  local function prototypes  */

static void        clay_pivot_selector_set_property         (GObject           *object,
                                                             guint              property_id,
                                                             const GValue      *value,
                                                             GParamSpec        *pspec);
static void        clay_pivot_selector_get_property         (GObject           *object,
                                                             guint              property_id,
                                                             GValue            *value,
                                                             GParamSpec        *pspec);

static void        clay_pivot_selector_button_toggled       (GtkToggleButton   *button,
                                                             ClayPivotSelector *selector);

static GtkWidget * clay_pivot_selector_position_to_button   (ClayPivotSelector *selector,
                                                             gdouble            x,
                                                             gdouble            y);
static void        clay_pivot_selector_button_to_position   (ClayPivotSelector *selector,
                                                             GtkWidget         *button,
                                                             gdouble           *x,
                                                             gdouble           *y);

static void        clay_pivot_selector_update_active_button (ClayPivotSelector *selector);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPivotSelector, clay_pivot_selector, GTK_TYPE_GRID)

#define parent_class clay_pivot_selector_parent_class

static guint pivot_selector_signals[LAST_SIGNAL];


/*  private functions  */


static void
clay_pivot_selector_class_init (ClayPivotSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  pivot_selector_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayPivotSelectorClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->get_property = clay_pivot_selector_get_property;
  object_class->set_property = clay_pivot_selector_set_property;

  g_object_class_install_property (object_class, PROP_LEFT,
                                   g_param_spec_double ("left",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TOP,
                                   g_param_spec_double ("top",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RIGHT,
                                   g_param_spec_double ("right",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BOTTOM,
                                   g_param_spec_double ("bottom",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y",
                                                        NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "ClayPivotSelector");
}

static void
clay_pivot_selector_init (ClayPivotSelector *selector)
{
  GtkWidget *widget = GTK_WIDGET (selector);
  GtkGrid   *grid   = GTK_GRID (selector);
  gint       i;

  selector->priv = clay_pivot_selector_get_instance_private (selector);

  gtk_widget_set_halign (widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);

  gtk_grid_set_row_homogeneous    (grid, TRUE);
  gtk_grid_set_column_homogeneous (grid, TRUE);

  for (i = 0; i < 9; i++)
    {
      static const gchar *icon_names[9] = {
        CLAY_ICON_PIVOT_NORTH_WEST,
        CLAY_ICON_PIVOT_NORTH,
        CLAY_ICON_PIVOT_NORTH_EAST,

        CLAY_ICON_PIVOT_WEST,
        CLAY_ICON_PIVOT_CENTER,
        CLAY_ICON_PIVOT_EAST,

        CLAY_ICON_PIVOT_SOUTH_WEST,
        CLAY_ICON_PIVOT_SOUTH,
        CLAY_ICON_PIVOT_SOUTH_EAST
      };

      GtkWidget *button;
      GtkWidget *image;
      gint       x, y;

      x = i % 3;
      y = i / 3;

      button = gtk_toggle_button_new ();
      gtk_widget_set_can_focus (button, FALSE);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_grid_attach (grid, button, x, y, 1, 1);
      gtk_widget_set_visible (button, TRUE);

      selector->priv->buttons[i] = button;

      g_signal_connect (button, "toggled",
                        G_CALLBACK (clay_pivot_selector_button_toggled),
                        selector);

      image = gtk_image_new_from_icon_name (icon_names[i], GTK_ICON_SIZE_MENU);
      gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
      gtk_container_add (GTK_CONTAINER (button), image);
      gtk_widget_set_visible (image, TRUE);
    }
}

static void
clay_pivot_selector_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayPivotSelector *selector = CLAY_PIVOT_SELECTOR (object);

  switch (property_id)
    {
    case PROP_LEFT:
      clay_pivot_selector_set_bounds (selector,
                                      g_value_get_double (value),
                                      selector->priv->top,
                                      selector->priv->right,
                                      selector->priv->bottom);
      break;
    case PROP_TOP:
      clay_pivot_selector_set_bounds (selector,
                                      selector->priv->left,
                                      g_value_get_double (value),
                                      selector->priv->right,
                                      selector->priv->bottom);
      break;
    case PROP_RIGHT:
      clay_pivot_selector_set_bounds (selector,
                                      selector->priv->left,
                                      selector->priv->top,
                                      g_value_get_double (value),
                                      selector->priv->bottom);
      break;
    case PROP_BOTTOM:
      clay_pivot_selector_set_bounds (selector,
                                      selector->priv->left,
                                      selector->priv->top,
                                      selector->priv->right,
                                      g_value_get_double (value));
      break;

    case PROP_X:
      clay_pivot_selector_set_position (selector,
                                        g_value_get_double (value),
                                        selector->priv->y);
      break;
    case PROP_Y:
      clay_pivot_selector_set_position (selector,
                                        selector->priv->x,
                                        g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pivot_selector_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayPivotSelector *selector = CLAY_PIVOT_SELECTOR (object);

  switch (property_id)
    {
    case PROP_LEFT:
      g_value_set_double (value, selector->priv->left);
      break;
    case PROP_TOP:
      g_value_set_double (value, selector->priv->top);
      break;
    case PROP_RIGHT:
      g_value_set_double (value, selector->priv->right);
      break;
    case PROP_BOTTOM:
      g_value_set_double (value, selector->priv->bottom);
      break;

    case PROP_X:
      g_value_set_double (value, selector->priv->x);
      break;
    case PROP_Y:
      g_value_set_double (value, selector->priv->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pivot_selector_button_toggled (GtkToggleButton   *button,
                                    ClayPivotSelector *selector)
{
  if (GTK_WIDGET (button) == selector->priv->active_button)
    {
      gtk_toggle_button_set_active (button, TRUE);
    }
  else
    {
      gdouble x, y;

      clay_pivot_selector_button_to_position (selector, GTK_WIDGET (button),
                                              &x, &y);

      clay_pivot_selector_set_position (selector, x, y);
    }
}

static GtkWidget *
clay_pivot_selector_position_to_button (ClayPivotSelector *selector,
                                        gdouble            x,
                                        gdouble            y)
{
  gint ix;
  gint iy;

  if (selector->priv->left == selector->priv->right ||
      selector->priv->top  == selector->priv->bottom)
    {
      return NULL;
    }

  x = 2.0 * (x                      - selector->priv->left) /
            (selector->priv->right  - selector->priv->left);
  y = 2.0 * (y                      - selector->priv->top)  /
            (selector->priv->bottom - selector->priv->top);

  ix = RINT (x);
  iy = RINT (y);

  if (fabs (x - ix) > EPSILON || fabs (y - iy) > EPSILON)
    return NULL;

  if (ix < 0 || ix > 2 || iy < 0 || iy > 2)
    return NULL;

  return selector->priv->buttons[3 * iy + ix];
}

static void
clay_pivot_selector_button_to_position (ClayPivotSelector *selector,
                                        GtkWidget         *button,
                                        gdouble           *x,
                                        gdouble           *y)
{
  gint i;

  for (i = 0; selector->priv->buttons[i] != button; i++);

  *x = selector->priv->left +
       (selector->priv->right  - selector->priv->left) * (i % 3) / 2.0;
  *y = selector->priv->top +
       (selector->priv->bottom - selector->priv->top)  * (i / 3) / 2.0;
}

static void
clay_pivot_selector_update_active_button (ClayPivotSelector *selector)
{
  GtkWidget *button;

  button = clay_pivot_selector_position_to_button (selector,
                                                   selector->priv->x,
                                                   selector->priv->y);

  if (button != selector->priv->active_button)
    {
      if (selector->priv->active_button)
        {
          g_signal_handlers_block_by_func (
            selector->priv->active_button,
            clay_pivot_selector_button_toggled,
            selector);

          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (selector->priv->active_button), FALSE);

          g_signal_handlers_unblock_by_func (
            selector->priv->active_button,
            clay_pivot_selector_button_toggled,
            selector);
        }

      selector->priv->active_button = button;

      if (selector->priv->active_button)
        {
          g_signal_handlers_block_by_func (
            selector->priv->active_button,
            clay_pivot_selector_button_toggled,
            selector);

          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (selector->priv->active_button), TRUE);

          g_signal_handlers_unblock_by_func (
            selector->priv->active_button,
            clay_pivot_selector_button_toggled,
            selector);
        }
    }
}


/*  public functions  */


GtkWidget *
clay_pivot_selector_new (gdouble left,
                         gdouble top,
                         gdouble right,
                         gdouble bottom)
{
  return g_object_new (CLAY_TYPE_PIVOT_SELECTOR,

                       "left",   left,
                       "top",    top,
                       "right",  right,
                       "bottom", bottom,

                       "x",      (left + right)  / 2.0,
                       "y",      (top  + bottom) / 2.0,

                       NULL);
}

void
clay_pivot_selector_set_bounds (ClayPivotSelector *selector,
                                gdouble            left,
                                gdouble            top,
                                gdouble            right,
                                gdouble            bottom)
{
  g_return_if_fail (CLAY_IS_PIVOT_SELECTOR (selector));

  if (left  != selector->priv->left  || top    != selector->priv->top ||
      right != selector->priv->right || bottom != selector->priv->bottom)
    {
      g_object_freeze_notify (G_OBJECT (selector));

      selector->priv->left   = left;
      selector->priv->top    = top;
      selector->priv->right  = right;
      selector->priv->bottom = bottom;

      clay_pivot_selector_update_active_button (selector);

      if (left != selector->priv->left)
        g_object_notify (G_OBJECT (selector), "left");
      if (top != selector->priv->top)
        g_object_notify (G_OBJECT (selector), "top");
      if (right != selector->priv->right)
        g_object_notify (G_OBJECT (selector), "right");
      if (left != selector->priv->bottom)
        g_object_notify (G_OBJECT (selector), "bottom");

      g_object_thaw_notify (G_OBJECT (selector));
    }
}

void
clay_pivot_selector_get_bounds (ClayPivotSelector *selector,
                                gdouble           *left,
                                gdouble           *top,
                                gdouble           *right,
                                gdouble           *bottom)
{
  g_return_if_fail (CLAY_IS_PIVOT_SELECTOR (selector));

  if (left)   *left   = selector->priv->left;
  if (top)    *top    = selector->priv->top;
  if (right)  *right  = selector->priv->right;
  if (bottom) *bottom = selector->priv->bottom;
}

void
clay_pivot_selector_set_position (ClayPivotSelector *selector,
                                  gdouble            x,
                                  gdouble            y)
{
  g_return_if_fail (CLAY_IS_PIVOT_SELECTOR (selector));

  if (x != selector->priv->x || y != selector->priv->y)
    {
      g_object_freeze_notify (G_OBJECT (selector));

      selector->priv->x = x;
      selector->priv->y = y;

      clay_pivot_selector_update_active_button (selector);

      g_signal_emit (selector, pivot_selector_signals[CHANGED], 0);

      if (x != selector->priv->x)
        g_object_notify (G_OBJECT (selector), "x");
      if (y != selector->priv->y)
        g_object_notify (G_OBJECT (selector), "y");

      g_object_thaw_notify (G_OBJECT (selector));
    }
}

void
clay_pivot_selector_get_position (ClayPivotSelector *selector,
                                  gdouble           *x,
                                  gdouble           *y)
{
  g_return_if_fail (CLAY_IS_PIVOT_SELECTOR (selector));

  if (x) *x = selector->priv->x;
  if (y) *y = selector->priv->y;
}
