/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationpointfilter.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "operations-types.h"

#include "clayoperationpointfilter.h"


static void   clay_operation_point_filter_finalize (GObject       *object);
static void   clay_operation_point_filter_prepare  (GeglOperation *operation);


G_DEFINE_ABSTRACT_TYPE (ClayOperationPointFilter, clay_operation_point_filter,
                        GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class clay_operation_point_filter_parent_class


static void
clay_operation_point_filter_class_init (ClayOperationPointFilterClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass  *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize = clay_operation_point_filter_finalize;

  operation_class->prepare = clay_operation_point_filter_prepare;
}

static void
clay_operation_point_filter_init (ClayOperationPointFilter *self)
{
  self->trc_binding = NULL;
  self->trc         = CLAY_TRC_LINEAR;
}

static void
clay_operation_point_filter_finalize (GObject *object)
{
  ClayOperationPointFilter *self = CLAY_OPERATION_POINT_FILTER (object);

  g_clear_object (&self->config);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
clay_operation_point_filter_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  ClayOperationPointFilter *self = CLAY_OPERATION_POINT_FILTER (object);

  switch (property_id)
    {
    case CLAY_OPERATION_POINT_FILTER_PROP_TRC:
      g_value_set_enum (value, self->trc);
      break;

    case CLAY_OPERATION_POINT_FILTER_PROP_CONFIG:
      g_value_set_object (value, self->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
clay_operation_point_filter_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  ClayOperationPointFilter *self = CLAY_OPERATION_POINT_FILTER (object);

  switch (property_id)
    {
    case CLAY_OPERATION_POINT_FILTER_PROP_TRC:
      self->trc = g_value_get_enum (value);
      break;

    case CLAY_OPERATION_POINT_FILTER_PROP_CONFIG:
      g_clear_object (&self->trc_binding);
      g_set_object (&self->config, g_value_dup_object (value));
      if (self->config                                                    &&
          g_object_class_find_property (G_OBJECT_GET_CLASS (self), "trc") &&
          g_object_class_find_property (G_OBJECT_GET_CLASS (self->config), "trc"))
        self->trc_binding = g_object_bind_property (self->config, "trc",
                                                    self,         "trc",
                                                    G_BINDING_SYNC_CREATE);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_point_filter_prepare (GeglOperation *operation)
{
  ClayOperationPointFilter *self = CLAY_OPERATION_POINT_FILTER (operation);
  const Babl               *space = gegl_operation_get_source_space (operation,
                                                                     "input");
  const Babl               *format;

  switch (self->trc)
    {
    default:
    case CLAY_TRC_LINEAR:
      format = babl_format_with_space ("RGBA float", space);
      break;

    case CLAY_TRC_NON_LINEAR:
      format = babl_format_with_space ("R'G'B'A float", space);
      break;

    case CLAY_TRC_PERCEPTUAL:
      format = babl_format_with_space ("R~G~B~A float", space);
      break;
    }

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}
