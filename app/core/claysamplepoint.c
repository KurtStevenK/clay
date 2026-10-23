/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gio/gio.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay-utils.h"
#include "claysamplepoint.h"
#include "claysavable.h"


enum
{
  PROP_0,
  PROP_POSITION_X,
  PROP_POSITION_Y,
  PROP_PICK_MODE
};


struct _ClaySamplePointPrivate
{
  gint              position_x;
  gint              position_y;
  ClayColorPickMode pick_mode;
};


static void   clay_sample_point_savable_iface_init (ClaySavableInterface *iface);

static void   clay_sample_point_get_property       (GObject              *object,
                                                    guint                 property_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);
static void   clay_sample_point_set_property       (GObject              *object,
                                                    guint                 property_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);

static void   clay_sample_point_savable_save       (ClaySavable          *savable,
                                                    ClaySaveState        *state);


G_DEFINE_TYPE_WITH_CODE (ClaySamplePoint, clay_sample_point, CLAY_TYPE_AUX_ITEM,
                         G_ADD_PRIVATE (ClaySamplePoint)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_sample_point_savable_iface_init))


static void
clay_sample_point_class_init (ClaySamplePointClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = clay_sample_point_get_property;
  object_class->set_property = clay_sample_point_set_property;

  CLAY_CONFIG_PROP_INT (object_class, PROP_POSITION_X,
                        "position-x",
                        NULL, NULL,
                        CLAY_SAMPLE_POINT_POSITION_UNDEFINED,
                        CLAY_MAX_IMAGE_SIZE,
                        CLAY_SAMPLE_POINT_POSITION_UNDEFINED,
                        0);

  CLAY_CONFIG_PROP_INT (object_class, PROP_POSITION_Y,
                        "position-y",
                        NULL, NULL,
                        CLAY_SAMPLE_POINT_POSITION_UNDEFINED,
                        CLAY_MAX_IMAGE_SIZE,
                        CLAY_SAMPLE_POINT_POSITION_UNDEFINED,
                        0);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_PICK_MODE,
                         "pick-mode",
                         NULL, NULL,
                         CLAY_TYPE_COLOR_PICK_MODE,
                         CLAY_COLOR_PICK_MODE_PIXEL,
                         0);
}

static void
clay_sample_point_savable_iface_init (ClaySavableInterface *iface)
{
  iface->save = clay_sample_point_savable_save;
}

static void
clay_sample_point_init (ClaySamplePoint *sample_point)
{
  sample_point->priv = clay_sample_point_get_instance_private (sample_point);
}

static void
clay_sample_point_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  ClaySamplePoint *sample_point = CLAY_SAMPLE_POINT (object);

  switch (property_id)
    {
    case PROP_POSITION_X:
      g_value_set_int (value, sample_point->priv->position_x);
      break;
    case PROP_POSITION_Y:
      g_value_set_int (value, sample_point->priv->position_y);
      break;
    case PROP_PICK_MODE:
      g_value_set_enum (value, sample_point->priv->pick_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_sample_point_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  ClaySamplePoint *sample_point = CLAY_SAMPLE_POINT (object);

  switch (property_id)
    {
    case PROP_POSITION_X:
      sample_point->priv->position_x = g_value_get_int (value);
      break;
    case PROP_POSITION_Y:
      sample_point->priv->position_y = g_value_get_int (value);
      break;
    case PROP_PICK_MODE:
      sample_point->priv->pick_mode = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_sample_point_savable_save (ClaySavable   *savable,
                                ClaySaveState *state)
{
  ClaySamplePoint   *sample_point = CLAY_SAMPLE_POINT (savable);
  ClayColorPickMode  pick_mode;

  /* XXX At this point, I just store the enum because that's what we
   * were already doing in XCF, but the question can be raised whether
   * we'd want a more generic sample points which can display colors in
   * a random model and space in the future?
   */
  pick_mode  = clay_sample_point_get_pick_mode (sample_point);
  clay_savable_print_element (state, "sample-point", NULL, NULL,
                              "pick-mode", "%[ClayColorPickMode]", pick_mode,
                              NULL);
}

ClaySamplePoint *
clay_sample_point_new (guint32 sample_point_ID)
{
  return g_object_new (CLAY_TYPE_SAMPLE_POINT,
                       "id", sample_point_ID,
                       NULL);
}

void
clay_sample_point_get_position (ClaySamplePoint *sample_point,
                                gint            *position_x,
                                gint            *position_y)
{
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));
  g_return_if_fail (position_x != NULL);
  g_return_if_fail (position_y != NULL);

  *position_x = sample_point->priv->position_x;
  *position_y = sample_point->priv->position_y;
}

void
clay_sample_point_set_position (ClaySamplePoint *sample_point,
                                gint             position_x,
                                gint             position_y)
{
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  if (sample_point->priv->position_x != position_x ||
      sample_point->priv->position_y != position_y)
    {
      sample_point->priv->position_x = position_x;
      sample_point->priv->position_y = position_y;

      g_object_freeze_notify (G_OBJECT (sample_point));

      g_object_notify (G_OBJECT (sample_point), "position-x");
      g_object_notify (G_OBJECT (sample_point), "position-y");

      g_object_thaw_notify (G_OBJECT (sample_point));
    }
}

ClayColorPickMode
clay_sample_point_get_pick_mode (ClaySamplePoint *sample_point)
{
  g_return_val_if_fail (CLAY_IS_SAMPLE_POINT (sample_point),
                        CLAY_COLOR_PICK_MODE_PIXEL);

  return sample_point->priv->pick_mode;
}

void
clay_sample_point_set_pick_mode (ClaySamplePoint   *sample_point,
                                 ClayColorPickMode  pick_mode)
{
  g_return_if_fail (CLAY_IS_SAMPLE_POINT (sample_point));

  if (sample_point->priv->pick_mode != pick_mode)
    {
      sample_point->priv->pick_mode = pick_mode;

      g_object_notify (G_OBJECT (sample_point), "pick-mode");
    }
}
