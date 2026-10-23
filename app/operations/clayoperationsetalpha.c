/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationsetalpha.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "clayoperationsetalpha.h"


enum
{
  PROP_0,
  PROP_VALUE
};


static void       clay_operation_set_alpha_get_property (GObject             *object,
                                                         guint                property_id,
                                                         GValue              *value,
                                                         GParamSpec          *pspec);
static void       clay_operation_set_alpha_set_property (GObject             *object,
                                                         guint                property_id,
                                                         const GValue        *value,
                                                         GParamSpec          *pspec);

static void       clay_operation_set_alpha_prepare      (GeglOperation       *operation);
static gboolean   clay_operation_set_alpha_process      (GeglOperation       *operation,
                                                         void                *in_buf,
                                                         void                *aux_buf,
                                                         void                *out_buf,
                                                         glong                samples,
                                                         const GeglRectangle *roi,
                                                         gint                 level);


G_DEFINE_TYPE (ClayOperationSetAlpha, clay_operation_set_alpha,
               GEGL_TYPE_OPERATION_POINT_COMPOSER)

#define parent_class clay_operation_set_alpha_parent_class


static void
clay_operation_set_alpha_class_init (ClayOperationSetAlphaClass *klass)
{
  GObjectClass                    *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  object_class->set_property = clay_operation_set_alpha_set_property;
  object_class->get_property = clay_operation_set_alpha_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "clay:set-alpha",
                                 "categories",  "color",
                                 "description", "Set a buffer's alpha channel to a value",
                                 NULL);

  operation_class->prepare = clay_operation_set_alpha_prepare;

  point_class->process     = clay_operation_set_alpha_process;

  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_double ("value",
                                                        "Value",
                                                        "The alpha value",
                                                        0.0, 1.0, 1.0,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
clay_operation_set_alpha_init (ClayOperationSetAlpha *self)
{
}

static void
clay_operation_set_alpha_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  ClayOperationSetAlpha *self = CLAY_OPERATION_SET_ALPHA (object);

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, self->value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_set_alpha_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  ClayOperationSetAlpha *self = CLAY_OPERATION_SET_ALPHA (object);

  switch (property_id)
    {
    case PROP_VALUE:
      self->value = g_value_get_double (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_operation_set_alpha_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "aux",    babl_format_with_space ("Y float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("RGBA float", space));
}

static gboolean
clay_operation_set_alpha_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *aux_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  ClayOperationSetAlpha *self = CLAY_OPERATION_SET_ALPHA (operation);
  gfloat                *src  = in_buf;
  gfloat                *aux  = aux_buf;
  gfloat                *dest = out_buf;

  if (aux)
    {
      while (samples--)
        {
          dest[RED]   = src[RED];
          dest[GREEN] = src[GREEN];
          dest[BLUE]  = src[BLUE];
          dest[ALPHA] = self->value * *aux;

          src  += 4;
          aux  += 1;
          dest += 4;
        }
    }
  else
    {
      while (samples--)
        {
          dest[RED]   = src[RED];
          dest[GREEN] = src[GREEN];
          dest[BLUE]  = src[BLUE];
          dest[ALPHA] = self->value;

          src  += 4;
          dest += 4;
        }
    }

  return TRUE;
}
