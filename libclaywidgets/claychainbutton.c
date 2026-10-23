/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claychainbutton.c
 * Copyright (C) 1999-2000 Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "claywidgetstypes.h"

#include "claychainbutton.h"
#include "clayicons.h"


/**
 * SECTION: claychainbutton
 * @title: ClayChainButton
 * @short_description: Widget to visually connect two entry widgets.
 * @see_also: You may want to use the convenience function
 *            clay_coordinates_new() to set up two ClaySizeEntries
 *            (see #ClaySizeEntry) linked with a #ClayChainButton.
 *
 * This widget provides a button showing either a linked or a broken
 * chain that can be used to link two entries, spinbuttons, colors or
 * other GUI elements and show that they may be locked. Use it for
 * example to connect X and Y ratios to provide the possibility of a
 * constrained aspect ratio.
 *
 * The #ClayChainButton only gives visual feedback, it does not really
 * connect widgets. You have to take care of locking the values
 * yourself by checking the state of the #ClayChainButton whenever a
 * value changes in one of the connected widgets and adjusting the
 * other value if necessary.
 **/


enum
{
  PROP_0,
  PROP_POSITION,
  PROP_ICON_SIZE,
  PROP_ACTIVE
};

enum
{
  TOGGLED,
  LAST_SIGNAL
};


struct _ClayChainButton
{
  GtkGrid            parent_instance;

  ClayChainPosition  position;
  gboolean           active;

  GtkWidget         *button;
  GtkWidget         *line1;
  GtkWidget         *line2;
  GtkWidget         *image;
};


static void      clay_chain_button_constructed      (GObject         *object);
static void      clay_chain_button_set_property     (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void      clay_chain_button_get_property     (GObject         *object,
                                                     guint            property_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);

static void      clay_chain_button_compute_expand   (GtkWidget       *widget,
                                                     gboolean        *hexpand_p,
                                                     gboolean        *vexpand_p);

static void      clay_chain_button_clicked_callback (GtkWidget       *widget,
                                                     ClayChainButton *button);
static void      clay_chain_button_update_image     (ClayChainButton *button);

static GtkWidget * clay_chain_line_new            (ClayChainPosition  position,
                                                   gint               which);


G_DEFINE_TYPE (ClayChainButton, clay_chain_button, GTK_TYPE_GRID)

#define parent_class clay_chain_button_parent_class

static guint clay_chain_button_signals[LAST_SIGNAL] = { 0 };

static const gchar * const clay_chain_icon_names[] =
{
  CLAY_ICON_CHAIN_HORIZONTAL,
  CLAY_ICON_CHAIN_HORIZONTAL_BROKEN,
  CLAY_ICON_CHAIN_VERTICAL,
  CLAY_ICON_CHAIN_VERTICAL_BROKEN
};


static void
clay_chain_button_class_init (ClayChainButtonClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed    = clay_chain_button_constructed;
  object_class->set_property   = clay_chain_button_set_property;
  object_class->get_property   = clay_chain_button_get_property;

  widget_class->compute_expand = clay_chain_button_compute_expand;

  clay_chain_button_signals[TOGGLED] =
    g_signal_new ("toggled",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * ClayChainButton:position:
   *
   * The position in which the chain button will be used.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_POSITION,
                                   g_param_spec_enum ("position",
                                                      "Position",
                                                      "The chain's position",
                                                      CLAY_TYPE_CHAIN_POSITION,
                                                      CLAY_CHAIN_TOP,
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      CLAY_PARAM_READWRITE));

  /**
   * ClayChainButton:icon-size:
   *
   * The chain button icon size.
   *
   * Since: 2.10.10
   */
  g_object_class_install_property (object_class, PROP_ICON_SIZE,
                                   g_param_spec_enum ("icon-size",
                                                      "Icon Size",
                                                      "The chain's icon size",
                                                      GTK_TYPE_ICON_SIZE,
                                                      GTK_ICON_SIZE_BUTTON,
                                                      G_PARAM_CONSTRUCT |
                                                      CLAY_PARAM_READWRITE));

  /**
   * ClayChainButton:active:
   *
   * The toggled state of the chain button.
   *
   * Since: 2.10.10
   */
  g_object_class_install_property (object_class, PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                         "Active",
                                                         "The chain's toggled state",
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT |
                                                         CLAY_PARAM_READWRITE));
}

static void
clay_chain_button_init (ClayChainButton *button)
{
  button->position = CLAY_CHAIN_TOP;
  button->active   = FALSE;
  button->image    = gtk_image_new ();
  button->button   = gtk_button_new ();

  gtk_button_set_relief (GTK_BUTTON (button->button), GTK_RELIEF_NONE);
  gtk_container_add (GTK_CONTAINER (button->button), button->image);
  gtk_widget_set_visible (button->image, TRUE);

  g_signal_connect (button->button, "clicked",
                    G_CALLBACK (clay_chain_button_clicked_callback),
                    button);
}

static void
clay_chain_button_constructed (GObject *object)
{
  ClayChainButton *button = CLAY_CHAIN_BUTTON (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  button->line1 = clay_chain_line_new (button->position, 1);
  button->line2 = clay_chain_line_new (button->position, -1);

  clay_chain_button_update_image (button);

  if (button->position & CLAY_CHAIN_LEFT) /* are we a vertical chainbutton? */
    {
      gtk_widget_set_vexpand (button->line1, TRUE);
      gtk_widget_set_vexpand (button->line2, TRUE);

      gtk_grid_attach (GTK_GRID (button), button->line1,  0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), button->button, 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (button), button->line2,  0, 2, 1, 1);
    }
  else
    {
      gtk_widget_set_hexpand (button->line1, TRUE);
      gtk_widget_set_hexpand (button->line2, TRUE);

      gtk_grid_attach (GTK_GRID (button), button->line1,  0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), button->button, 1, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (button), button->line2,  2, 0, 1, 1);
    }

  gtk_widget_set_visible (button->button, TRUE);
  gtk_widget_set_visible (button->line1, TRUE);
  gtk_widget_set_visible (button->line2, TRUE);
}

static void
clay_chain_button_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClayChainButton *button = CLAY_CHAIN_BUTTON (object);

  switch (property_id)
    {
    case PROP_POSITION:
      button->position = g_value_get_enum (value);
      break;

    case PROP_ICON_SIZE:
      g_object_set_property (G_OBJECT (button->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      clay_chain_button_set_active (button, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_chain_button_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClayChainButton *button = CLAY_CHAIN_BUTTON (object);

  switch (property_id)
    {
    case PROP_POSITION:
      g_value_set_enum (value, button->position);
      break;

    case PROP_ICON_SIZE:
      g_object_get_property (G_OBJECT (button->image), "icon-size", value);
      break;

    case PROP_ACTIVE:
      g_value_set_boolean (value, clay_chain_button_get_active (button));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_chain_button_compute_expand (GtkWidget *widget,
                                  gboolean  *hexpand_p,
                                  gboolean  *vexpand_p)
{
  /* don't inherit [hv]expand from the chain lines.  see issue #3876. */
  *hexpand_p = FALSE;
  *vexpand_p = FALSE;
}

/**
 * clay_chain_button_new:
 * @position: The position you are going to use for the button
 *            with respect to the widgets you want to chain.
 *
 * Creates a new #ClayChainButton widget.
 *
 * This returns a button showing either a broken or a linked chain and
 * small clamps attached to both sides that visually group the two
 * widgets you want to connect. This widget looks best when attached
 * to a grid taking up two columns (or rows respectively) next to the
 * widgets that it is supposed to connect. It may work for more than
 * two widgets, but the look is optimized for two.
 *
 * Returns: Pointer to the new #ClayChainButton, which is inactive
 *          by default. Use clay_chain_button_set_active() to
 *          change its state.
 */
GtkWidget *
clay_chain_button_new (ClayChainPosition position)
{
  return g_object_new (CLAY_TYPE_CHAIN_BUTTON,
                       "position", position,
                       NULL);
}

/**
 * clay_chain_button_set_icon_size:
 * @button: Pointer to a #ClayChainButton.
 * @size: The new icon size.
 *
 * Sets the icon size of the #ClayChainButton.
 *
 * Since: 2.10.10
 */
void
clay_chain_button_set_icon_size (ClayChainButton *button,
                                 GtkIconSize      size)
{
  g_return_if_fail (CLAY_IS_CHAIN_BUTTON (button));

  g_object_set (button,
                "icon-size", size,
                NULL);
}

/**
 * clay_chain_button_get_icon_size:
 * @button: Pointer to a #ClayChainButton.
 *
 * Gets the icon size of the #ClayChainButton.
 *
 * Returns: The icon size.
 *
 * Since: 2.10.10
 */
GtkIconSize
clay_chain_button_get_icon_size (ClayChainButton *button)
{
  GtkIconSize size;

  g_return_val_if_fail (CLAY_IS_CHAIN_BUTTON (button), GTK_ICON_SIZE_BUTTON);

  g_object_get (button,
                "icon-size", &size,
                NULL);

  return size;
}

/**
 * clay_chain_button_set_active:
 * @button: Pointer to a #ClayChainButton.
 * @active: The new state.
 *
 * Sets the state of the #ClayChainButton to be either locked (%TRUE) or
 * unlocked (%FALSE) and changes the showed pixmap to reflect the new state.
 */
void
clay_chain_button_set_active (ClayChainButton  *button,
                              gboolean          active)
{
  g_return_if_fail (CLAY_IS_CHAIN_BUTTON (button));

  button = clay_chain_button_get_instance_private (button);

  if (button->active != active)
    {
      button->active = active ? TRUE : FALSE;

      clay_chain_button_update_image (button);

      g_signal_emit (button, clay_chain_button_signals[TOGGLED], 0);

      g_object_notify (G_OBJECT (button), "active");
    }
}

/**
 * clay_chain_button_get_active
 * @button: Pointer to a #ClayChainButton.
 *
 * Checks the state of the #ClayChainButton.
 *
 * Returns: %TRUE if the #ClayChainButton is active (locked).
 */
gboolean
clay_chain_button_get_active (ClayChainButton *button)
{
  g_return_val_if_fail (CLAY_IS_CHAIN_BUTTON (button), FALSE);

  button = clay_chain_button_get_instance_private (button);

  return button->active;
}

/**
 * clay_chain_button_get_button
 * @button: A #ClayChainButton.
 *
 * Returns: (transfer none) (type GtkButton): The #ClayChainButton's button.
 *
 * Since: 3.0
 */
GtkWidget *
clay_chain_button_get_button (ClayChainButton *button)
{
  g_return_val_if_fail (CLAY_IS_CHAIN_BUTTON (button), FALSE);

  button = clay_chain_button_get_instance_private (button);

  return button->button;
}


/*  private functions  */

static void
clay_chain_button_clicked_callback (GtkWidget       *widget,
                                    ClayChainButton *button)
{
  clay_chain_button_set_active (button, ! button->active);
}

static void
clay_chain_button_update_image (ClayChainButton *button)
{
  guint i;

  i = ((button->position & CLAY_CHAIN_LEFT) << 1) + (button->active ? 0 : 1);

  gtk_image_set_from_icon_name (GTK_IMAGE (button->image),
                                clay_chain_icon_names[i],
                                clay_chain_button_get_icon_size (button));
}


/* ClayChainLine is a simple no-window widget for drawing the lines.
 *
 * Originally this used to be a GtkDrawingArea but this turned out to
 * be a bad idea. We don't need an extra window to draw on and we also
 * don't need any input events.
 */

static GType     clay_chain_line_get_type (void);
static gboolean  clay_chain_line_draw     (GtkWidget *widget,
                                           cairo_t   *cr);

struct _ClayChainLine
{
  GtkWidget          parent_instance;
  ClayChainPosition  position;
  gint               which;
};

typedef struct _ClayChainLine  ClayChainLine;
typedef GtkWidgetClass         ClayChainLineClass;

G_DEFINE_TYPE (ClayChainLine, clay_chain_line, GTK_TYPE_WIDGET)

static void
clay_chain_line_class_init (ClayChainLineClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->draw = clay_chain_line_draw;
}

static void
clay_chain_line_init (ClayChainLine *line)
{
  gtk_widget_set_has_window (GTK_WIDGET (line), FALSE);
}

static GtkWidget *
clay_chain_line_new (ClayChainPosition  position,
                     gint               which)
{
  ClayChainLine *line = g_object_new (clay_chain_line_get_type (), NULL);

  line->position = position;
  line->which    = which;

  return GTK_WIDGET (line);
}

static gboolean
clay_chain_line_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  GtkStyleContext   *context = gtk_widget_get_style_context (widget);
  ClayChainLine     *line    = ((ClayChainLine *) widget);
  GtkAllocation      allocation;
  GdkPoint           points[3];
  ClayChainPosition  position;
  GdkRGBA            color;

  gtk_widget_get_allocation (widget, &allocation);

#define SHORT_LINE 4
  points[0].x = allocation.width  / 2;
  points[0].y = allocation.height / 2;

  position = line->position;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    {
      switch (position)
        {
        case CLAY_CHAIN_TOP:
        case CLAY_CHAIN_BOTTOM:
          break;

        case CLAY_CHAIN_LEFT:
          position = CLAY_CHAIN_RIGHT;
          break;

        case CLAY_CHAIN_RIGHT:
          position = CLAY_CHAIN_LEFT;
          break;
        }
    }

  switch (position)
    {
    case CLAY_CHAIN_LEFT:
      points[0].x += SHORT_LINE;
      points[1].x = points[0].x - SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case CLAY_CHAIN_RIGHT:
      points[0].x -= SHORT_LINE;
      points[1].x = points[0].x + SHORT_LINE;
      points[1].y = points[0].y;
      points[2].x = points[1].x;
      points[2].y = (line->which == 1 ? allocation.height - 1 : 0);
      break;

    case CLAY_CHAIN_TOP:
      points[0].y += SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y - SHORT_LINE;
      points[2].x = (line->which == 1 ? allocation.width - 1 : 0);
      points[2].y = points[1].y;
      break;

    case CLAY_CHAIN_BOTTOM:
      points[0].y -= SHORT_LINE;
      points[1].x = points[0].x;
      points[1].y = points[0].y + SHORT_LINE;
      points[2].x = (line->which == 1 ? allocation.width - 1 : 0);
      points[2].y = points[1].y;
      break;

    default:
      return FALSE;
    }

  cairo_move_to (cr, points[0].x, points[0].y);
  cairo_line_to (cr, points[1].x, points[1].y);
  cairo_line_to (cr, points[2].x, points[2].y);

  cairo_set_line_width (cr, 2.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget), &color);
  gdk_cairo_set_source_rgba (cr, &color);

  cairo_stroke (cr);

  return TRUE;
}
