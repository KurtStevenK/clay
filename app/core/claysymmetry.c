/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysymmetry.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-gegl-nodes.h"

#include "claydrawable.h"
#include "clayimage.h"
#include "clayimage-symmetry.h"
#include "clayitem.h"
#include "claysymmetry.h"

#include "clay-intl.h"


enum
{
  STROKES_UPDATED,
  GUI_PARAM_CHANGED,
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_ACTIVE,
  PROP_VERSION,
};


/* Local function prototypes */

static void       clay_symmetry_finalize            (GObject      *object);
static void       clay_symmetry_set_property        (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void       clay_symmetry_get_property        (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void       clay_symmetry_real_update_strokes (ClaySymmetry *sym,
                                                     ClayDrawable *drawable,
                                                     ClayCoords   *origin);
static void       clay_symmetry_real_get_transform  (ClaySymmetry *sym,
                                                     gint          stroke,
                                                     gdouble      *angle,
                                                     gboolean     *reflect);
static gboolean   clay_symmetry_real_update_version (ClaySymmetry *sym);


G_DEFINE_TYPE_WITH_CODE (ClaySymmetry, clay_symmetry, CLAY_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG, NULL))

#define parent_class clay_symmetry_parent_class

static guint clay_symmetry_signals[LAST_SIGNAL] = { 0 };


static void
clay_symmetry_class_init (ClaySymmetryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /* This signal should likely be emitted at the end of
   * update_strokes() if stroke coordinates were changed.
   */
  clay_symmetry_signals[STROKES_UPDATED] =
    g_signal_new ("strokes-updated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, CLAY_TYPE_IMAGE);

  /* This signal should be emitted when you request a change in the
   * settings UI. For instance adding some settings (therefore having
   * a dynamic UI), or changing scale min/max extremes, etc.
   */
  clay_symmetry_signals[GUI_PARAM_CHANGED] =
    g_signal_new ("gui-param-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1, CLAY_TYPE_IMAGE);

  clay_symmetry_signals[ACTIVE_CHANGED] =
    g_signal_new ("active-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClaySymmetryClass, active_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize     = clay_symmetry_finalize;
  object_class->set_property = clay_symmetry_set_property;
  object_class->get_property = clay_symmetry_get_property;

  klass->label               = _("None");
  klass->update_strokes      = clay_symmetry_real_update_strokes;
  klass->get_transform       = clay_symmetry_real_get_transform;
  klass->active_changed      = NULL;
  klass->update_version      = clay_symmetry_real_update_version;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image",
                                                        NULL, NULL,
                                                        CLAY_TYPE_IMAGE,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ACTIVE,
                            "active",
                            _("Active"),
                            _("Activate symmetry painting"),
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_INT (object_class, PROP_VERSION,
                        "version",
                        "Symmetry version",
                        "Version of the symmetry object",
                        -1, G_MAXINT, 0,
                        CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_symmetry_init (ClaySymmetry *sym)
{
}

static void
clay_symmetry_finalize (GObject *object)
{
  ClaySymmetry *sym = CLAY_SYMMETRY (object);

  clay_symmetry_clear_origin (sym);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_symmetry_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  ClaySymmetry *sym = CLAY_SYMMETRY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      sym->image = g_value_get_object (value);
      break;
    case PROP_ACTIVE:
      sym->active = g_value_get_boolean (value);
      g_signal_emit (sym, clay_symmetry_signals[ACTIVE_CHANGED], 0,
                     sym->active);
      break;
    case PROP_VERSION:
      sym->version = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_symmetry_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClaySymmetry *sym = CLAY_SYMMETRY (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, sym->image);
      break;
    case PROP_ACTIVE:
      g_value_set_boolean (value, sym->active);
      break;
    case PROP_VERSION:
      g_value_set_int (value, sym->version);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_symmetry_real_update_strokes (ClaySymmetry *sym,
                                   ClayDrawable *drawable,
                                   ClayCoords   *origin)
{
  /* The basic symmetry just uses the origin as is. */
  sym->strokes = g_list_prepend (sym->strokes,
                                 g_memdup2 (origin, sizeof (ClayCoords)));
}

static void
clay_symmetry_real_get_transform (ClaySymmetry *sym,
                                  gint          stroke,
                                  gdouble      *angle,
                                  gboolean     *reflect)
{
  /* The basic symmetry does nothing, since no transformation of the
   * brush painting happen. */
}

static gboolean
clay_symmetry_real_update_version (ClaySymmetry *symmetry)
{
  /* Currently all symmetries are at version 0. So all this check has to
   * do is verify that we are at version 0.
   * If one of the child symmetry bumps its version, it will have to
   * override the update_version() virtual function and do any necessary
   * update there (for instance new properties, modified properties, or
   * whatnot).
   */
  gint version;

  g_object_get (symmetry,
                "version", &version,
                NULL);

  return (version == 0);
}

/***** Public Functions *****/

/**
 * clay_symmetry_set_stateful:
 * @sym:      the #ClaySymmetry
 * @stateful: whether the symmetry should be stateful or stateless.
 *
 * By default, symmetry is made stateless, which means in particular
 * that the size of points can change from one stroke to the next, and
 * in particular you cannot map the coordinates from a stroke to the
 * next. I.e. stroke N at time T+1 is not necessarily the continuation
 * of stroke N at time T.
 * To obtain corresponding strokes, stateful tools, such as MyPaint
 * brushes or the ink tool, need to run this function. They should reset
 * to stateless behavior once finished painting.
 *
 * One of the first consequence of being stateful is that the number of
 * strokes cannot be changed, so more strokes than possible on canvas
 * may be computed, and oppositely it will be possible to end up in
 * cases with missing strokes (e.g. a tiling, theoretically infinite,
 * won't be for the ink tool if one draws too far out of canvas).
 **/
void
clay_symmetry_set_stateful (ClaySymmetry *symmetry,
                            gboolean      stateful)
{
  symmetry->stateful = stateful;
}

/**
 * clay_symmetry_set_origin:
 * @sym:      the #ClaySymmetry
 * @drawable: the #ClayDrawable where painting will happen
 * @origin:   new base coordinates.
 *
 * Set the symmetry to new origin coordinates and drawable.
 **/
void
clay_symmetry_set_origin (ClaySymmetry *sym,
                          ClayDrawable *drawable,
                          ClayCoords   *origin)
{
  g_return_if_fail (CLAY_IS_SYMMETRY (sym));
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_get_image (CLAY_ITEM (drawable)) == sym->image);

  if (drawable != sym->drawable)
    {
      if (sym->drawable)
        g_object_unref (sym->drawable);
      sym->drawable = g_object_ref (drawable);
    }

  if (origin != sym->origin)
    {
      g_free (sym->origin);
      sym->origin = g_memdup2 (origin, sizeof (ClayCoords));
    }

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;

  CLAY_SYMMETRY_GET_CLASS (sym)->update_strokes (sym, drawable, origin);
}

/**
 * clay_symmetry_clear_origin:
 * @sym: the #ClaySymmetry
 *
 * Clear the symmetry's origin coordinates and drawable.
 **/
void
clay_symmetry_clear_origin (ClaySymmetry *sym)
{
  g_return_if_fail (CLAY_IS_SYMMETRY (sym));

  g_clear_object (&sym->drawable);

  g_clear_pointer (&sym->origin, g_free);

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;
}

/**
 * clay_symmetry_get_origin:
 * @sym: the #ClaySymmetry
 *
 * Returns: the origin stroke coordinates.
 * The returned value is owned by the #ClaySymmetry and must not be freed.
 **/
ClayCoords *
clay_symmetry_get_origin (ClaySymmetry *sym)
{
  g_return_val_if_fail (CLAY_IS_SYMMETRY (sym), NULL);

  return sym->origin;
}

/**
 * clay_symmetry_get_size:
 * @sym: the #ClaySymmetry
 *
 * Returns: the total number of strokes.
 **/
gint
clay_symmetry_get_size (ClaySymmetry *sym)
{
  g_return_val_if_fail (CLAY_IS_SYMMETRY (sym), 0);

  return g_list_length (sym->strokes);
}

/**
 * clay_symmetry_get_coords:
 * @sym:    the #ClaySymmetry
 * @stroke: the stroke number
 *
 * Returns: the coordinates of the stroke number @stroke.
 * The returned value is owned by the #ClaySymmetry and must not be freed.
 **/
ClayCoords *
clay_symmetry_get_coords (ClaySymmetry *sym,
                          gint          stroke)
{
  g_return_val_if_fail (CLAY_IS_SYMMETRY (sym), NULL);

  return g_list_nth_data (sym->strokes, stroke);
}

/**
 * clay_symmetry_get_transform:
 * @sym:     the #ClaySymmetry
 * @stroke:  the stroke number
 * @angle: (out): output pointer to the transformation rotation angle,
 *           in degrees (ccw)
 * @reflect: (out): output pointer to the transformation reflection flag
 *
 * Returns: the transformation to apply to the paint content for stroke
 * number @stroke.  The transformation is comprised of rotation, possibly
 * followed by horizontal reflection, around the stroke coordinates.
 **/
void
clay_symmetry_get_transform (ClaySymmetry *sym,
                             gint          stroke,
                             gdouble      *angle,
                             gboolean     *reflect)
{
  g_return_if_fail (CLAY_IS_SYMMETRY (sym));
  g_return_if_fail (angle != NULL);
  g_return_if_fail (reflect != NULL);

  *angle   = 0.0;
  *reflect = FALSE;

  CLAY_SYMMETRY_GET_CLASS (sym)->get_transform (sym,
                                                stroke,
                                                angle,
                                                reflect);
}

/**
 * clay_symmetry_get_matrix:
 * @sym:     the #ClaySymmetry
 * @stroke:  the stroke number
 * @matrix:  output pointer to the transformation matrix
 *
 * Returns: the transformation matrix to apply to the paint content for stroke
 * number @stroke.
 **/
void
clay_symmetry_get_matrix (ClaySymmetry *sym,
                          gint          stroke,
                          ClayMatrix3  *matrix)
{
  gdouble  angle;
  gboolean reflect;

  g_return_if_fail (CLAY_IS_SYMMETRY (sym));
  g_return_if_fail (matrix != NULL);

  clay_symmetry_get_transform (sym, stroke, &angle, &reflect);

  clay_matrix3_identity (matrix);
  clay_matrix3_rotate (matrix, -clay_deg_to_rad (angle));
  if (reflect)
    clay_matrix3_scale (matrix, -1.0, 1.0);
}

/**
 * clay_symmetry_get_operation:
 * @sym:          the #ClaySymmetry
 * @stroke:       the stroke number
 *
 * Returns: the transformation operation to apply to the paint content for
 * stroke number @stroke, or NULL for the identity transformation.
 *
 * The returned #GeglNode should be freed by the caller.
 **/
GeglNode *
clay_symmetry_get_operation (ClaySymmetry *sym,
                             gint          stroke)
{
  ClayMatrix3 matrix;

  g_return_val_if_fail (CLAY_IS_SYMMETRY (sym), NULL);

  clay_symmetry_get_matrix (sym, stroke, &matrix);

  if (clay_matrix3_is_identity (&matrix))
    return NULL;

  return clay_gegl_create_transform_node (&matrix);
}

/*
 * clay_symmetry_parasite_name:
 * @type: the #ClaySymmetry's #GType
 *
 * Returns: a newly allocated string.
 */
gchar *
clay_symmetry_parasite_name (GType type)
{
  return g_strconcat ("clay-image-symmetry:", g_type_name (type), NULL);
}

ClayParasite *
clay_symmetry_to_parasite (const ClaySymmetry *sym)
{
  ClayParasite *parasite;
  gchar        *parasite_name;

  g_return_val_if_fail (CLAY_IS_SYMMETRY (sym), NULL);

  parasite_name = clay_symmetry_parasite_name (G_TYPE_FROM_INSTANCE (sym));

  parasite = clay_config_serialize_to_parasite ((ClayConfig *) sym,
                                                parasite_name,
                                                CLAY_PARASITE_PERSISTENT,
                                                NULL);

  g_free (parasite_name);

  return parasite;
}

ClaySymmetry *
clay_symmetry_from_parasite (const ClayParasite *parasite,
                             ClayImage          *image,
                             GType               type)
{
  ClaySymmetry *symmetry;
  gchar        *parasite_name;
  gchar        *parasite_contents;
  guint32       parasite_size;
  GError       *error = NULL;

  parasite_name = clay_symmetry_parasite_name (type);

  g_return_val_if_fail (parasite != NULL, NULL);
  g_return_val_if_fail (strcmp (clay_parasite_get_name (parasite),
                                parasite_name) == 0,
                        NULL);

  parasite_contents = (gchar *) clay_parasite_get_data (parasite, &parasite_size);
  if (! parasite_contents)
    {
      g_warning ("Empty symmetry parasite \"%s\"", parasite_name);

      return NULL;
    }

  symmetry = clay_image_symmetry_new (image, type);

  g_object_set (symmetry,
                "version", -1,
                NULL);

  if (! clay_config_deserialize_parasite (CLAY_CONFIG (symmetry),
                                          parasite,
                                          NULL,
                                          &error))
    {
      g_printerr ("Failed to deserialize symmetry parasite: %s\n"
                  "\t- parasite name: %s\n\t- parasite data: %.*s\n",
                  error->message, parasite_name,
                  parasite_size, parasite_contents);
      g_error_free (error);

      g_object_unref (symmetry);
      symmetry = NULL;
    }
  g_free (parasite_name);

  if (symmetry)
    {
      gint version;

      g_object_get (symmetry,
                    "version", &version,
                    NULL);
      if (version == -1)
        {
          /* If version has not been updated, let's assume this parasite was
           * not representing symmetry settings.
           */
          g_object_unref (symmetry);
          symmetry = NULL;
        }
      else if (CLAY_SYMMETRY_GET_CLASS (symmetry)->update_version (symmetry) &&
               ! CLAY_SYMMETRY_GET_CLASS (symmetry)->update_version (symmetry))
        {
          g_object_unref (symmetry);
          symmetry = NULL;
        }
    }

  return symmetry;
}
