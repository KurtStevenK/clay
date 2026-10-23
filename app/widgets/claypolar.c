/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypolar.c
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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
#include "libclaycolor/claycolor.h"
#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "core/clay-cairo.h"

#include "claypolar.h"

/* round n to the nearest multiple of m */
#define SNAP(n, m) (RINT((n) / (m)) * (m))

enum
{
  PROP_0,
  PROP_ANGLE,
  PROP_RADIUS
};

typedef enum
{
  POLAR_TARGET_NONE   = 0,
  POLAR_TARGET_CIRCLE = 1 << 0
} PolarTarget;


typedef struct _ClayPolarPrivate ClayPolarPrivate;

struct _ClayPolarPrivate
{
  gdouble      angle;
  gdouble      radius;

  PolarTarget  target;
};

#define GET_PRIVATE(obj)                                                \
  ((ClayPolarPrivate *) clay_polar_get_instance_private ((ClayPolar *) obj))


static void        clay_polar_set_property         (GObject            *object,
                                                    guint               property_id,
                                                    const GValue       *value,
                                                    GParamSpec         *pspec);
static void        clay_polar_get_property         (GObject            *object,
                                                    guint               property_id,
                                                    GValue             *value,
                                                    GParamSpec         *pspec);

static gboolean    clay_polar_draw                 (GtkWidget          *widget,
                                                    cairo_t            *cr);
static gboolean    clay_polar_button_press_event   (GtkWidget          *widget,
                                                    GdkEventButton     *bevent);
static gboolean    clay_polar_motion_notify_event  (GtkWidget          *widget,
                                                    GdkEventMotion     *mevent);

static void        clay_polar_reset_target         (ClayCircle         *circle);

static void        clay_polar_set_target           (ClayPolar           *polar,
                                                    PolarTarget          target);

static void        clay_polar_draw_circle          (cairo_t            *cr,
                                                    gint                size,
                                                    gdouble             angle,
                                                    gdouble             radius,
                                                    PolarTarget         highlight);

static gdouble     clay_polar_normalize_angle      (gdouble             angle);
static gdouble     clay_polar_get_angle_distance   (gdouble             alpha,
                                                    gdouble             beta);


G_DEFINE_TYPE_WITH_PRIVATE (ClayPolar, clay_polar, CLAY_TYPE_CIRCLE)

#define parent_class clay_polar_parent_class


static void
clay_polar_class_init (ClayPolarClass *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS (klass);
  ClayCircleClass *circle_class = CLAY_CIRCLE_CLASS (klass);

  object_class->get_property         = clay_polar_get_property;
  object_class->set_property         = clay_polar_set_property;

  widget_class->draw                 = clay_polar_draw;
  widget_class->button_press_event   = clay_polar_button_press_event;
  widget_class->motion_notify_event  = clay_polar_motion_notify_event;

  circle_class->reset_target         = clay_polar_reset_target;

  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("angle",
                                                        NULL, NULL,
                                                        0.0, 2 * G_PI, 0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RADIUS,
                                   g_param_spec_double ("radius",
                                                        NULL, NULL,
                                                        0.0, 1.0, 0.0,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_polar_init (ClayPolar *polar)
{
}

static void
clay_polar_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ClayPolarPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ANGLE:
      priv->angle = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;

    case PROP_RADIUS:
      priv->radius = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (object));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_polar_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayPolarPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ANGLE:
      g_value_set_double (value, priv->angle);
      break;

    case PROP_RADIUS:
      g_value_set_double (value, priv->radius);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_polar_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  ClayPolarPrivate *priv = GET_PRIVATE (widget);
  GtkAllocation     allocation;
  gint              size;

  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  g_object_get (widget,
                "size", &size,
                NULL);

  gtk_widget_get_allocation (widget, &allocation);

  cairo_save (cr);

  cairo_translate (cr,
                   (allocation.width  - size) / 2.0,
                   (allocation.height - size) / 2.0);

  clay_polar_draw_circle (cr, size,
                          priv->angle, priv->radius,
                          priv->target);

  cairo_restore (cr);

  return FALSE;
}

static gboolean
clay_polar_button_press_event (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  ClayPolarPrivate *priv = GET_PRIVATE (widget);

  if (gdk_event_get_event_type ((GdkEvent *) bevent) == GDK_BUTTON_PRESS &&
      bevent->button == 1                                                &&
      priv->target != POLAR_TARGET_NONE)
    {
      gdouble angle;
      gdouble radius;

      GTK_WIDGET_CLASS (parent_class)->button_press_event (widget, bevent);

      angle = _clay_circle_get_angle_and_distance (CLAY_CIRCLE (widget),
                                                   bevent->x, bevent->y,
                                                   &radius);
      if (bevent->state & GDK_SHIFT_MASK)
        angle = SNAP (angle, G_PI / 12.0);

      radius = MIN (radius, 1.0);

      g_object_set (widget,
                    "angle",  angle,
                    "radius", radius,
                    NULL);
    }

  return FALSE;
}

static gboolean
clay_polar_motion_notify_event (GtkWidget      *widget,
                                GdkEventMotion *mevent)
{
  ClayPolarPrivate *priv = GET_PRIVATE (widget);
  gdouble           angle;
  gdouble           radius;

  angle = _clay_circle_get_angle_and_distance (CLAY_CIRCLE (widget),
                                               mevent->x, mevent->y,
                                               &radius);

  if (_clay_circle_has_grab (CLAY_CIRCLE (widget)))
    {
      radius = MIN (radius, 1.0);

      if (mevent->state & GDK_SHIFT_MASK)
        angle = SNAP (angle, G_PI / 12.0);

      g_object_set (widget,
                    "angle",  angle,
                    "radius", radius,
                    NULL);
    }
  else
    {
      PolarTarget target;
      gdouble     dist_angle;
      gdouble     dist_radius;

      dist_angle  = clay_polar_get_angle_distance (priv->angle, angle);
      dist_radius = ABS (priv->radius - radius);

      if ((radius < 0.2 && priv->radius < 0.2) ||
          (dist_angle  < (G_PI / 12) && dist_radius < 0.2))
        {
          target = POLAR_TARGET_CIRCLE;
        }
      else
        {
          target = POLAR_TARGET_NONE;
        }

      clay_polar_set_target (CLAY_POLAR (widget), target);
    }

  gdk_event_request_motions (mevent);

  return FALSE;
}

static void
clay_polar_reset_target (ClayCircle *circle)
{
  clay_polar_set_target (CLAY_POLAR (circle), POLAR_TARGET_NONE);
}


/*  public functions  */

GtkWidget *
clay_polar_new (void)
{
  return g_object_new (CLAY_TYPE_POLAR, NULL);
}


/*  private functions  */

static void
clay_polar_set_target (ClayPolar   *polar,
                       PolarTarget  target)
{
  ClayPolarPrivate *priv = GET_PRIVATE (polar);

  if (target != priv->target)
    {
      priv->target = target;
      gtk_widget_queue_draw (GTK_WIDGET (polar));
    }
}

static void
clay_polar_draw_circle (cairo_t     *cr,
                        gint         size,
                        gdouble      angle,
                        gdouble      radius,
                        PolarTarget  highlight)
{
  gdouble r = size / 2.0 - 2.0; /* half the broad line with and half a px */
  gdouble x = r + r * radius * cos (angle);
  gdouble y = r - r * radius * sin (angle);

  cairo_save (cr);

  cairo_translate (cr, 2.0, 2.0); /* half the broad line width and half a px*/

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  cairo_arc (cr, x, y, 3, 0, 2 * G_PI);

  if (highlight == POLAR_TARGET_NONE)
    {
      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.8);
      cairo_stroke (cr);
    }
  else
    {
      cairo_set_line_width (cr, 3.0);
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.6);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0);
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.8);
      cairo_stroke (cr);
    }

  cairo_restore (cr);
}

static gdouble
clay_polar_normalize_angle (gdouble angle)
{
  if (angle < 0)
    return angle + 2 * G_PI;
  else if (angle > 2 * G_PI)
    return angle - 2 * G_PI;
  else
    return angle;
}

static gdouble
clay_polar_get_angle_distance (gdouble alpha,
                               gdouble beta)
{
  return ABS (MIN (clay_polar_normalize_angle (alpha - beta),
                   2 * G_PI - clay_polar_normalize_angle (alpha - beta)));
}
