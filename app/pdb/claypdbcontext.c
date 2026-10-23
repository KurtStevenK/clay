/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * claypdbcontext.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "pdb-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claylist.h"
#include "core/claypaintinfo.h"
#include "core/claystrokeoptions.h"

#include "paint/claybrushcore.h"
#include "paint/claypaintoptions.h"

#include "claypdbcontext.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS_X,
  PROP_FEATHER_RADIUS_Y,
  PROP_SAMPLE_MERGED,
  PROP_SAMPLE_CRITERION,
  PROP_SAMPLE_THRESHOLD,
  PROP_SAMPLE_TRANSPARENT,
  PROP_DIAGONAL_NEIGHBORS,
  PROP_INTERPOLATION,
  PROP_TRANSFORM_DIRECTION,
  PROP_TRANSFORM_RESIZE,
  PROP_DISTANCE_METRIC
};


static void   clay_pdb_context_iface_init   (ClayConfigInterface *iface);

static void   clay_pdb_context_constructed  (GObject      *object);
static void   clay_pdb_context_finalize     (GObject      *object);
static void   clay_pdb_context_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   clay_pdb_context_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);

static void   clay_pdb_context_reset        (ClayConfig   *config);


G_DEFINE_TYPE_WITH_CODE (ClayPDBContext, clay_pdb_context, CLAY_TYPE_CONTEXT,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CONFIG,
                                                clay_pdb_context_iface_init))

#define parent_class clay_pdb_context_parent_class

static ClayConfigInterface *parent_config_iface = NULL;


static void
clay_pdb_context_class_init (ClayPDBContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = clay_pdb_context_constructed;
  object_class->finalize     = clay_pdb_context_finalize;
  object_class->set_property = clay_pdb_context_set_property;
  object_class->get_property = clay_pdb_context_get_property;

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            _("Smooth edges"),
                            TRUE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS_X,
                           "feather-radius-x",
                           _("Feather radius X"),
                           NULL,
                           0.0, 1000.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS_Y,
                           "feather-radius-y",
                           _("Feather radius Y"),
                           NULL,
                           0.0, 1000.0, 10.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                            "sample-merged",
                            _("Sample merged"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_SAMPLE_CRITERION,
                         "sample-criterion",
                         _("Sample criterion"),
                         NULL,
                         CLAY_TYPE_SELECT_CRITERION,
                         CLAY_SELECT_CRITERION_COMPOSITE,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_DOUBLE (object_class, PROP_SAMPLE_THRESHOLD,
                           "sample-threshold",
                           _("Sample threshold"),
                           NULL,
                           0.0, 1.0, 0.0,
                           CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_SAMPLE_TRANSPARENT,
                            "sample-transparent",
                            _("Sample transparent"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_BOOLEAN (object_class, PROP_DIAGONAL_NEIGHBORS,
                            "diagonal-neighbors",
                            _("Diagonal neighbors"),
                            NULL,
                            FALSE,
                            CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION,
                         "interpolation",
                         _("Interpolation"),
                         NULL,
                         CLAY_TYPE_INTERPOLATION_TYPE,
                         CLAY_INTERPOLATION_CUBIC,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_TRANSFORM_DIRECTION,
                         "transform-direction",
                         _("Transform direction"),
                         NULL,
                         CLAY_TYPE_TRANSFORM_DIRECTION,
                         CLAY_TRANSFORM_FORWARD,
                         CLAY_PARAM_STATIC_STRINGS);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_TRANSFORM_RESIZE,
                         "transform-resize",
                         _("Transform resize"),
                         NULL,
                         CLAY_TYPE_TRANSFORM_RESIZE,
                         CLAY_TRANSFORM_RESIZE_ADJUST,
                         CLAY_PARAM_STATIC_STRINGS);

  /* Legacy blend used "manhattan" metric to compute distance.
   * API needs to stay compatible, hence the default value for this
   * property.
   * Nevertheless Euclidean distance since to render better; for Clay 3
   * API, we might therefore want to change the defaults to
   * GEGL_DISTANCE_METRIC_EUCLIDEAN. FIXME.
   */
  CLAY_CONFIG_PROP_ENUM (object_class, PROP_DISTANCE_METRIC,
                         "distance-metric",
                         _("Distance metric"),
                         NULL,
                         GEGL_TYPE_DISTANCE_METRIC,
                         GEGL_DISTANCE_METRIC_MANHATTAN,
                         CLAY_PARAM_STATIC_STRINGS);

}

static void
clay_pdb_context_iface_init (ClayConfigInterface *iface)
{
  parent_config_iface = g_type_interface_peek_parent (iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (CLAY_TYPE_CONFIG);

  iface->reset = clay_pdb_context_reset;
}

static void
clay_pdb_context_init (ClayPDBContext *context)
{
  context->paint_options_list = clay_list_new (CLAY_TYPE_PAINT_OPTIONS,
                                               FALSE);
}

static void
clay_pdb_context_constructed (GObject *object)
{
  ClayPDBContext        *context = CLAY_PDB_CONTEXT (object);
  ClayInterpolationType  interpolation;
  gint                   threshold;
  GParamSpec            *pspec;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  context->stroke_options = clay_stroke_options_new (CLAY_CONTEXT (context)->clay,
                                                     CLAY_CONTEXT (context),
                                                     TRUE);

  /* keep the stroke options in sync with the context */
  clay_context_define_properties (CLAY_CONTEXT (context->stroke_options),
                                  CLAY_CONTEXT_PROP_MASK_ALL, FALSE);
  clay_context_set_parent (CLAY_CONTEXT (context->stroke_options),
                           CLAY_CONTEXT (context));

  /* preserve the traditional PDB default */
  g_object_set (context->stroke_options,
                "method", CLAY_STROKE_PAINT_METHOD,
                NULL);

  g_object_bind_property (G_OBJECT (context),                 "antialias",
                          G_OBJECT (context->stroke_options), "antialias",
                          G_BINDING_SYNC_CREATE);

  /* get default interpolation from clayrc */

  interpolation = CLAY_CONTEXT (object)->clay->config->interpolation_type;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                        "interpolation");

  if (pspec)
    G_PARAM_SPEC_ENUM (pspec)->default_value = interpolation;

  g_object_set (object, "interpolation", interpolation, NULL);

  /* get default threshold from clayrc */

  threshold = CLAY_CONTEXT (object)->clay->config->default_threshold;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object),
                                        "sample-threshold");

  if (pspec)
    G_PARAM_SPEC_DOUBLE (pspec)->default_value = threshold / 255.0;

  g_object_set (object, "sample-threshold", threshold / 255.0, NULL);
}

static void
clay_pdb_context_finalize (GObject *object)
{
  ClayPDBContext *context = CLAY_PDB_CONTEXT (object);

  g_clear_object (&context->paint_options_list);
  g_clear_object (&context->stroke_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pdb_context_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  ClayPDBContext *options = CLAY_PDB_CONTEXT (object);

  switch (property_id)
    {
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;

    case PROP_FEATHER:
      options->feather = g_value_get_boolean (value);
      break;

    case PROP_FEATHER_RADIUS_X:
      options->feather_radius_x = g_value_get_double (value);
      break;

    case PROP_FEATHER_RADIUS_Y:
      options->feather_radius_y = g_value_get_double (value);
      break;

    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;

    case PROP_SAMPLE_CRITERION:
      options->sample_criterion = g_value_get_enum (value);
      break;

    case PROP_SAMPLE_THRESHOLD:
      options->sample_threshold = g_value_get_double (value);
      break;

    case PROP_SAMPLE_TRANSPARENT:
      options->sample_transparent = g_value_get_boolean (value);
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      options->diagonal_neighbors = g_value_get_boolean (value);
      break;

    case PROP_INTERPOLATION:
      options->interpolation = g_value_get_enum (value);
      break;

    case PROP_TRANSFORM_DIRECTION:
      options->transform_direction = g_value_get_enum (value);
      break;

    case PROP_TRANSFORM_RESIZE:
      options->transform_resize = g_value_get_enum (value);
      break;

    case PROP_DISTANCE_METRIC:
      options->distance_metric = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_context_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  ClayPDBContext *options = CLAY_PDB_CONTEXT (object);

  switch (property_id)
    {
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;

    case PROP_FEATHER:
      g_value_set_boolean (value, options->feather);
      break;

    case PROP_FEATHER_RADIUS_X:
      g_value_set_double (value, options->feather_radius_x);
      break;

    case PROP_FEATHER_RADIUS_Y:
      g_value_set_double (value, options->feather_radius_y);
      break;

    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;

    case PROP_SAMPLE_CRITERION:
      g_value_set_enum (value, options->sample_criterion);
      break;

    case PROP_SAMPLE_THRESHOLD:
      g_value_set_double (value, options->sample_threshold);
      break;

    case PROP_SAMPLE_TRANSPARENT:
      g_value_set_boolean (value, options->sample_transparent);
      break;

    case PROP_DIAGONAL_NEIGHBORS:
      g_value_set_boolean (value, options->diagonal_neighbors);
      break;

    case PROP_INTERPOLATION:
      g_value_set_enum (value, options->interpolation);
      break;

    case PROP_TRANSFORM_DIRECTION:
      g_value_set_enum (value, options->transform_direction);
      break;

    case PROP_TRANSFORM_RESIZE:
      g_value_set_enum (value, options->transform_resize);
      break;

    case PROP_DISTANCE_METRIC:
      g_value_set_enum (value, options->distance_metric);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_context_reset (ClayConfig *config)
{
  ClayPDBContext *context = CLAY_PDB_CONTEXT (config);
  GList          *list;

  for (list = CLAY_LIST (context->paint_options_list)->queue->head;
       list;
       list = g_list_next (list))
    {
      clay_config_reset (list->data);
    }

  clay_config_reset (CLAY_CONFIG (context->stroke_options));

  /* preserve the traditional PDB default */
  g_object_set (context->stroke_options,
                "method", CLAY_STROKE_PAINT_METHOD,
                NULL);

  parent_config_iface->reset (config);

  g_object_notify (G_OBJECT (context), "antialias");
}

ClayContext *
clay_pdb_context_new (Clay        *clay,
                      ClayContext *parent,
                      gboolean     set_parent)
{
  ClayPDBContext *context;
  GList          *list;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (parent), NULL);

  context = g_object_new (CLAY_TYPE_PDB_CONTEXT,
                          "clay", clay,
                          "name", "PDB Context",
                          NULL);

  if (set_parent)
    {
      clay_context_define_properties (CLAY_CONTEXT (context),
                                      CLAY_CONTEXT_PROP_MASK_ALL, FALSE);
      clay_context_set_parent (CLAY_CONTEXT (context), parent);

      for (list = clay_get_paint_info_iter (clay);
           list;
           list = g_list_next (list))
        {
          ClayPaintInfo *info = list->data;

          clay_container_add (context->paint_options_list,
                              CLAY_OBJECT (info->paint_options));
        }
    }
  else
    {
      for (list = CLAY_LIST (CLAY_PDB_CONTEXT (parent)->paint_options_list)->queue->head;
           list;
           list = g_list_next (list))
        {
          ClayPaintOptions *options = clay_config_duplicate (list->data);

          clay_container_add (context->paint_options_list,
                              CLAY_OBJECT (options));
          g_object_unref (options);
        }

      clay_config_copy (CLAY_CONFIG (CLAY_PDB_CONTEXT (parent)->stroke_options),
                        CLAY_CONFIG (context->stroke_options),
                        0);
    }

  /*  copy the context properties last, they might have been
   *  overwritten by the above copying of stroke options, which have
   *  the pdb context as parent
   */
  clay_config_sync (G_OBJECT (parent), G_OBJECT (context), 0);

  /* Reset the proper init name after syncing. */
  g_object_set (G_OBJECT (context),
                "name", "PDB Context",
                NULL);

  return CLAY_CONTEXT (context);
}

ClayContainer *
clay_pdb_context_get_paint_options_list (ClayPDBContext *context)
{
  g_return_val_if_fail (CLAY_IS_PDB_CONTEXT (context), NULL);

  return context->paint_options_list;
}

ClayPaintOptions *
clay_pdb_context_get_paint_options (ClayPDBContext *context,
                                    const gchar    *name)
{
  g_return_val_if_fail (CLAY_IS_PDB_CONTEXT (context), NULL);

  if (! name)
    name = clay_object_get_name (clay_context_get_paint_info (CLAY_CONTEXT (context)));

  return (ClayPaintOptions *)
    clay_container_get_child_by_name (context->paint_options_list, name);
}

GList *
clay_pdb_context_get_brush_options (ClayPDBContext *context)
{
  GList *brush_options = NULL;
  GList *list;

  g_return_val_if_fail (CLAY_IS_PDB_CONTEXT (context), NULL);

  for (list = CLAY_LIST (context->paint_options_list)->queue->head;
       list;
       list = g_list_next (list))
    {
      ClayPaintOptions *options = list->data;

      if (g_type_is_a (options->paint_info->paint_type, CLAY_TYPE_BRUSH_CORE))
        brush_options = g_list_prepend (brush_options, options);
    }

  return g_list_reverse (brush_options);
}

ClayStrokeOptions *
clay_pdb_context_get_stroke_options (ClayPDBContext *context)
{
  g_return_val_if_fail (CLAY_IS_PDB_CONTEXT (context), NULL);

  return context->stroke_options;
}
