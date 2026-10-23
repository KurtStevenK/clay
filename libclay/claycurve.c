/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycurve.c
 * Copyright (C) 2026 Alx Sa
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "clay.h"

#include "claycurve.h"


enum
{
  PROP_0,
  PROP_CURVE_TYPE,
  PROP_N_SAMPLES,
  N_PROPS
};
static GParamSpec *obj_props[N_PROPS] = { NULL, };

enum
{
  POINTS_CHANGED,
  SAMPLES_CHANGED,
  LAST_SIGNAL
};
static guint clay_curve_signals[LAST_SIGNAL] = { 0 };

typedef struct
{
  gdouble           x;
  gdouble           y;

 ClayCurvePointType type;
} ClayCurvePoint;

struct _ClayCurve
{
  GObject           parent_instance;

  ClayCurveType     curve_type;

  gint              n_points;
  ClayCurvePoint   *points;

  gint              n_samples;
  gdouble          *samples;

  gboolean          identity;   /* whether the curve is an identity mapping */
};

static void          clay_curve_finalize          (GObject          *object);
static void          clay_curve_set_property      (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);
static void          clay_curve_get_property      (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);

static void          clay_curve_build_samples     (ClayCurve        *curve);


G_DEFINE_TYPE (ClayCurve, clay_curve, G_TYPE_OBJECT);

static void  clay_curve_class_init (ClayCurveClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * ClayCurve::points-changed:
   * @curve: the curve emitting the signal
   *
   * Emitted when any points are added, deleted, or edited in @curve.
   *
   * Since: 3.2
   */
  clay_curve_signals[POINTS_CHANGED] =
    g_signal_new ("points-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * ClayCurve::samples-changed:
   * @curve: the curve emitting the signal
   *
   * Emitted when any sample is edited, or if the number of samples
   * changed, in @curve.
   *
   * Since: 3.2
   */
  clay_curve_signals[SAMPLES_CHANGED] =
    g_signal_new ("samples-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize     = clay_curve_finalize;
  object_class->set_property = clay_curve_set_property;
  object_class->get_property = clay_curve_get_property;

  /**
   * ClayCurve:curve-type:
   *
   * The curve type.
   *
   * Since: 3.2
   */
  obj_props[PROP_CURVE_TYPE] =
      g_param_spec_enum ("curve-type",
                         "Curve Type",
                         "The curve type",
                         CLAY_TYPE_CURVE_TYPE,
                         CLAY_CURVE_SMOOTH,
                         CLAY_CONFIG_PARAM_FLAGS |
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * ClayCurve:n-samples:
   *
   * The number of samples this [enum@Clay.CurveType.FREE] curve is
   * split into.
   *
   * Since: 3.2
   */
  obj_props[PROP_N_SAMPLES] =
      g_param_spec_int ("n-samples",
                        "Number of Samples",
                        "The number of samples",
                        256, 256, 256,
                        CLAY_CONFIG_PARAM_FLAGS |
                        G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
clay_curve_init (ClayCurve *curve)
{
  curve->n_points  = 0;
  curve->points    = NULL;
  curve->n_samples = 256;
  curve->samples   = NULL;
  curve->identity  = FALSE;
}

static void
clay_curve_finalize (GObject *object)
{
  ClayCurve *curve = CLAY_CURVE (object);

  g_clear_pointer (&curve->points,  g_free);
  curve->n_points = 0;

  g_clear_pointer (&curve->samples, g_free);
  curve->n_samples = 0;
}

static void
clay_curve_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ClayCurve *curve = CLAY_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      clay_curve_set_curve_type (curve, g_value_get_enum (value));
      break;

    case PROP_N_SAMPLES:
      if (curve->curve_type == CLAY_CURVE_FREE)
        clay_curve_set_n_samples (curve, g_value_get_int (value));
      else
        curve->n_samples = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_curve_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayCurve *curve = CLAY_CURVE (object);

  switch (property_id)
    {
    case PROP_CURVE_TYPE:
      g_value_set_enum (value, curve->curve_type);
      break;

    case PROP_N_SAMPLES:
      g_value_set_int (value, curve->n_samples);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* Public Functions */

/**
 * clay_curve_new:
 *
 * Creates a new #ClayCurve object, of type
 * [enum@Clay.CurveType.SMOOTH], with 0 points initially.
 *
 * Returns: (transfer full): a new curve.
 *
 * Since: 3.2
 */
ClayCurve *
clay_curve_new (void)
{
  return g_object_new (CLAY_TYPE_CURVE, NULL);
}

/**
 * clay_curve_get_curve_type:
 * @curve: the #ClayCurve.
 *
 * Returns: the @curve type.
 *
 * Since: 3.2
 */
ClayCurveType
clay_curve_get_curve_type (ClayCurve *curve)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), CLAY_CURVE_SMOOTH);

  return curve->curve_type;
}

/**
 * clay_curve_set_curve_type:
 * @curve: the #ClayCurve.
 * @curve_type: the new curve type.
 *
 * Sets the curve type of @curve, as follows:
 *
 * - Nothing happens if the curve type is unchanged.
 * - If you change to [enum@Clay.CurveType.SMOOTH], it will create a
 *   non-specified number of points and will approximate their position
 *   along the freehand curve. All default points will be
 *   [enum@Clay.CurvePointType.SMOOTH].
 * - If you change to [enum@Clay.CurveType.FREE], all existing points
 *   will be cleared.
 *
 * Since: 3.2
 */
void
clay_curve_set_curve_type (ClayCurve     *curve,
                           ClayCurveType  curve_type)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));

  if (curve->curve_type != curve_type)
    {
      g_object_freeze_notify (G_OBJECT (curve));

      if (curve_type == CLAY_CURVE_SMOOTH)
        {
          gint i;

          curve->curve_type = curve_type;

          g_free (curve->points);

          /*  pick some points from the curve and make them control
           *  points
           */
          curve->n_points = 9;
          curve->points   = g_new0 (ClayCurvePoint, 9);

          for (i = 0; i < curve->n_points; i++)
            {
              gint sample = i * (curve->n_samples - 1) / (curve->n_points - 1);

              curve->points[i].x    = (gdouble) sample /
                                      (gdouble) (curve->n_samples - 1);
              curve->points[i].y    = curve->samples[sample];
              curve->points[i].type = CLAY_CURVE_POINT_SMOOTH;
            }

          g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);
        }
      else
        {
          clay_curve_clear_points (curve);
          curve->curve_type = curve_type;
          clay_curve_build_samples (curve);
        }

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_CURVE_TYPE]);

      g_object_thaw_notify (G_OBJECT (curve));
    }
}

/**
 * clay_curve_get_n_points:
 * @curve: the #ClayCurve.
 *
 * Gets the number of points in a [enum@Clay.CurveType.SMOOTH] curve. Note that it will always
 * be 0 for a [enum@Clay.CurveType.FREE] curve.
 *
 * This can later be used e.g. in [method@Clay.Curve.get_point] as points
 * are numbered from 0 (included) to the returned number (excluded).
 *
 * Note that the #ClayCurve API is not thread-safe. So be careful that
 * the information on the number of points is still valid when you use
 * it (you may have added or removed points in particular).
 *
 * Returns: the number of points in a smooth curve.
 *
 * Since: 3.2
 */
gint
clay_curve_get_n_points (ClayCurve *curve)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), 0);
  g_return_val_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH, 0);

  return curve->n_points;
}

/**
 * clay_curve_add_point:
 * @curve: the #ClayCurve.
 * @x: the point abscissa on a `[0.0, 1.0]` range.
 * @y: the point ordinate on a `[0.0, 1.0]` range.
 *
 * Add a new point in a [enum@Clay.CurveType.SMOOTH] @curve, with
 * coordinates `(x, y)`. Any value outside the `[0.0, 1.0]` range will
 * be silently clamped.
 *
 * The returned identifier can later be used e.g. in
 * [method@Clay.Curve.get_point] or other functions taking a @point number
 * as argument.
 *
 * Calling this may change identifiers for other points and the total
 * number of points in this @curve. Any such information you currently
 * hold should be considered invalid once the curve is changed.
 *
 * Returns: a point identifier to be used in other functions.
 *
 * Since: 3.2
 */
gint
clay_curve_add_point (ClayCurve *curve,
                      gdouble    x,
                      gdouble    y)
{
  ClayCurvePoint *points;
  gint            point;

  g_return_val_if_fail (CLAY_IS_CURVE (curve), -1);
  g_return_val_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH, -1);

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  for (point = 0; point < curve->n_points; point++)
    {
      if (curve->points[point].x > x)
        break;
    }

  points = g_new0 (ClayCurvePoint, curve->n_points + 1);

  memcpy (points,         curve->points,
          point * sizeof (ClayCurvePoint));
  memcpy (points + point + 1, curve->points + point,
          (curve->n_points - point) * sizeof (ClayCurvePoint));

  points[point].x    = x;
  points[point].y    = y;
  points[point].type = CLAY_CURVE_POINT_SMOOTH;

  g_free (curve->points);

  curve->n_points++;
  curve->points = points;

  g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);

  return point;
}

/**
 * clay_curve_get_point:
 * @curve: the #ClayCurve.
 * @point: a point identifier.
 * @x: (out) (nullable): the point abscissa on a `[0.0, 1.0]` range.
 * @y: (out) (nullable): the point ordinate on a `[0.0, 1.0]` range.
 *
 * Gets the @point coordinates for a [enum@Clay.CurveType.SMOOTH] @curve.
 *
 * The @point identifier must be between 0 and the value returned by
 * [method@Clay.Curve.get_n_points].
 *
 * You may also use a point identifier as returned by
 * [method@Clay.Curve.add_point], which will correspond to the same
 * point, unless you modified the @curve since (e.g. by calling
 * `clay_curve_add_point` again, or by deleting or modifying a point).
 *
 * Since: 3.2
 */
void
clay_curve_get_point (ClayCurve *curve,
                      gint       point,
                      gdouble   *x,
                      gdouble   *y)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH);
  g_return_if_fail (point >= 0 && point < curve->n_points);

  if (x) *x = curve->points[point].x;
  if (y) *y = curve->points[point].y;
}

/**
 * clay_curve_set_point_type:
 * @curve: the #ClayCurve.
 * @point: a point identifier.
 * @type: a point type.
 *
 * Sets the @point type in a [enum@Clay.CurveType.SMOOTH] @curve.
 *
 * Since: 3.2
 */
void
clay_curve_set_point_type (ClayCurve          *curve,
                           gint                point,
                           ClayCurvePointType  type)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH);
  g_return_if_fail (point >= 0 && point < curve->n_points);

  curve->points[point].type = type;

  g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);
}

/**
 * clay_curve_get_point_type:
 * @curve: the #ClayCurve.
 * @point: a point identifier.
 *
 * Returns: the @point type of a [enum@Clay.CurveType.SMOOTH] @curve.
 *
 * Since: 3.2
 */
ClayCurvePointType
clay_curve_get_point_type (ClayCurve *curve,
                           gint       point)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), CLAY_CURVE_POINT_SMOOTH);
  g_return_val_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH, CLAY_CURVE_POINT_SMOOTH);
  g_return_val_if_fail (point >= 0 && point < curve->n_points, CLAY_CURVE_POINT_SMOOTH);

  return curve->points[point].type;
}

/**
 * clay_curve_delete_point:
 * @curve: the #ClayCurve.
 * @point: a point identifier.
 *
 * Deletes a specific @point from a [enum@Clay.CurveType.SMOOTH] @curve.
 *
 * The @point identifier must be between 0 and the value returned by
 * [method@Clay.Curve.get_n_points].
 *
 * You may also use a point identifier as returned by
 * [method@Clay.Curve.add_point], which will correspond to the same
 * point, unless you modified the @curve since (e.g. by calling
 * `clay_curve_add_point` again, or by deleting or modifying a point).
 *
 * Since: 3.2
 */
void
clay_curve_delete_point (ClayCurve *curve,
                         gint       point)
{
  ClayCurvePoint *points;

  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH);
  g_return_if_fail (point >= 0 && point < curve->n_points);

  points = g_new0 (ClayCurvePoint, curve->n_points - 1);

  memcpy (points,         curve->points,
          point * sizeof (ClayCurvePoint));
  memcpy (points + point, curve->points + point + 1,
          (curve->n_points - point - 1) * sizeof (ClayCurvePoint));

  g_free (curve->points);

  curve->n_points--;
  curve->points = points;

  g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);
}

/**
 * clay_curve_set_point:
 * @curve: the #ClayCurve.
 * @point: a point identifier.
 * @x: the point abscissa on a `[0.0, 1.0]` range.
 * @y: the point ordinate on a `[0.0, 1.0]` range.
 *
 * Sets the @point coordinates in a [enum@Clay.CurveType.SMOOTH] @curve.
 * Any value outside the `[0.0, 1.0]` range will be silently clamped.
 *
 * Since: 3.2
 */
void
clay_curve_set_point (ClayCurve *curve,
                      gint       point,
                      gdouble    x,
                      gdouble    y)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH);
  g_return_if_fail (point >= 0 && point < curve->n_points);

  curve->points[point].x = CLAMP (x, 0.0, 1.0);
  curve->points[point].y = CLAMP (y, 0.0, 1.0);

  if (point > 0)
    curve->points[point].x = MAX (x, curve->points[point - 1].x);

  if (point < curve->n_points - 1)
    curve->points[point].x = MIN (x, curve->points[point + 1].x);

  g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);
}

/**
 * clay_curve_clear_points:
 * @curve: the #ClayCurve.
 *
 * Deletes all points from a [enum@Clay.CurveType.SMOOTH] @curve.
 *
 * A subsequent call to [method@Clay.Curve.get_n_points] will return 0.
 *
 * Since: 3.2
 */
void
clay_curve_clear_points (ClayCurve *curve)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_SMOOTH);

  if (curve->points)
    {
      g_clear_pointer (&curve->points, g_free);
      curve->n_points = 0;

      g_signal_emit (G_OBJECT (curve), clay_curve_signals[POINTS_CHANGED], 0);
    }
}

/**
 * clay_curve_get_n_samples:
 * @curve: the #ClayCurve.
 *
 * Gets the number of samples in a [enum@Clay.CurveType.FREE] curve.
 *
 * Returns: the number of samples in a freehand curve.
 *
 * Since: 3.2
 */
gint
clay_curve_get_n_samples (ClayCurve *curve)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), 0);
  g_return_val_if_fail (curve->curve_type == CLAY_CURVE_FREE, 0);

  return curve->n_samples;
}

/**
 * clay_curve_set_n_samples:
 * @curve: the #ClayCurve.
 * @n_samples: the number of samples.
 *
 * Sets the number of sample in a [enum@Clay.CurveType.FREE] @curve.
 *
 * Samples will be positioned on the curve abscissa at regular interval.
 * The more samples, the more your curve will have details. Currently,
 * the value of @n_samples is limited and must be between `2^8` and `2^12`.
 *
 * Note that changing the number of samples will reset the curve to an
 * identity curve.
 *
 * Since: 3.2
 */
void
clay_curve_set_n_samples (ClayCurve *curve,
                          gint       n_samples)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_FREE);
  g_return_if_fail (n_samples >= 256);
  g_return_if_fail (n_samples <= 4096);

  if (n_samples != curve->n_samples)
    {
      g_object_freeze_notify (G_OBJECT (curve));
      curve->n_samples = n_samples;
      clay_curve_build_samples (curve);
      g_object_thaw_notify (G_OBJECT (curve));

      g_object_notify_by_pspec (G_OBJECT (curve), obj_props[PROP_N_SAMPLES]);
      g_signal_emit (G_OBJECT (curve), clay_curve_signals[SAMPLES_CHANGED], 0);
    }
}

/**
 * clay_curve_get_sample:
 * @curve: the #ClayCurve.
 * @x: an abscissa on a `[0.0, 1.0]` range.
 *
 * Gets the ordinate @y value corresponding to the passed @x abscissa
 * value, in a [enum@Clay.CurveType.FREE] @curve.
 *
 * Note that while the @y coordinate will be stored exactly, the @x
 * coordinate will be rounded to the closest curve sample on the
 * abscissa. The more sample was set with
 * [method@Clay.Curve.set_n_samples], the more precise the rounding will
 * be.
 *
 * Since: 3.2
 */
gdouble
clay_curve_get_sample (ClayCurve *curve,
                       gdouble    x)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), 0);
  g_return_val_if_fail (curve->curve_type == CLAY_CURVE_FREE, 0);
  g_return_val_if_fail (x >= 0 && x <= 1.0, 0);

  return curve->samples[ROUND (x * (gdouble) (curve->n_samples - 1))];
}

/**
 * clay_curve_set_sample:
 * @curve: the #ClayCurve.
 * @x: the point abscissa on a `[0.0, 1.0]` range.
 * @y: the point ordinate on a `[0.0, 1.0]` range.
 *
 * Sets a sample in a [enum@Clay.CurveType.FREE] @curve, with
 * coordinates `(x, y)`.
 *
 * Note that while the @y coordinate will be stored exactly, the @x
 * coordinate will be rounded to the closest curve sample on the
 * abscissa. The more sample was set with
 * [method@Clay.Curve.set_n_samples], the more precise the rounding will
 * be.
 *
 * Since: 3.2
 */
void
clay_curve_set_sample (ClayCurve *curve,
                       gdouble    x,
                       gdouble    y)
{
  g_return_if_fail (CLAY_IS_CURVE (curve));
  g_return_if_fail (curve->curve_type == CLAY_CURVE_FREE);
  g_return_if_fail (x >= 0 && x <= 1.0);
  g_return_if_fail (y >= 0 && y <= 1.0);

  curve->samples[ROUND (x * (gdouble) (curve->n_samples - 1))] = y;

  g_signal_emit (G_OBJECT (curve), clay_curve_signals[SAMPLES_CHANGED], 0);
}

/**
 * clay_curve_is_identity:
 * @curve: a #ClayCurve object
 *
 * If this function returns %TRUE, then the curve maps each value to
 * itself. If it returns %FALSE, then this assumption can not be made.
 *
 * Returns: %TRUE if the curve is an identity mapping, %FALSE otherwise.
 *
 * Since: 3.2
 **/
gboolean
clay_curve_is_identity (ClayCurve *curve)
{
  g_return_val_if_fail (CLAY_IS_CURVE (curve), FALSE);

  return curve->identity;
}


/* Private functions */

static void
clay_curve_build_samples (ClayCurve *curve)
{
  curve->samples = g_renew (gdouble, curve->samples, curve->n_samples);

  for (gint i = 0; i < curve->n_samples; i++)
    curve->samples[i] = (gdouble) i / (gdouble) (curve->n_samples - 1);

  if (curve->curve_type == CLAY_CURVE_FREE)
    curve->identity = TRUE;
}
