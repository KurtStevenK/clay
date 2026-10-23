/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGuide
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "clayimage-guides.h"
#include "clayguide.h"
#include "claysavable.h"
#include "claysavable-load.h"


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_POSITION,
  PROP_STYLE
};


struct _ClayGuidePrivate
{
  ClayOrientationType  orientation;
  gint                 position;

  ClayGuideStyle       style;
};


static void       clay_guide_savable_iface_init (ClaySavableInterface  *iface);

static void       clay_guide_get_property       (GObject               *object,
                                                 guint                  property_id,
                                                 GValue                *value,
                                                 GParamSpec            *pspec);
static void       clay_guide_set_property       (GObject               *object,
                                                 guint                  property_id,
                                                 const GValue          *value,
                                                 GParamSpec            *pspec);

static void       clay_guide_save               (ClaySavable           *savable,
                                                 ClaySaveState         *state);
static gboolean   clay_guide_load_enter         (ClayLoadState         *state,
                                                 const gchar          **attribute_names,
                                                 const gchar          **attribute_values,
                                                 gpointer               user_data,
                                                 GError               **error);


G_DEFINE_TYPE_WITH_CODE (ClayGuide, clay_guide, CLAY_TYPE_AUX_ITEM,
                         G_ADD_PRIVATE (ClayGuide)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_guide_savable_iface_init))


static void
clay_guide_class_init (ClayGuideClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = clay_guide_get_property;
  object_class->set_property = clay_guide_set_property;

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_ORIENTATION,
                         "orientation",
                         NULL, NULL,
                         CLAY_TYPE_ORIENTATION_TYPE,
                         CLAY_ORIENTATION_UNKNOWN,
                         0);

  CLAY_CONFIG_PROP_INT (object_class, PROP_POSITION,
                        "position",
                        NULL, NULL,
                        CLAY_GUIDE_POSITION_UNDEFINED,
                        CLAY_MAX_IMAGE_SIZE,
                        CLAY_GUIDE_POSITION_UNDEFINED,
                        0);

  CLAY_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         NULL, NULL,
                         CLAY_TYPE_GUIDE_STYLE,
                         CLAY_GUIDE_STYLE_NONE,
                         0);
}

static void
clay_guide_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = "guide";
  iface->save       = clay_guide_save;
  iface->load_enter = clay_guide_load_enter;
  iface->load_exit  = NULL;
}

static void
clay_guide_init (ClayGuide *guide)
{
  guide->priv = clay_guide_get_instance_private (guide);
}

static void
clay_guide_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  ClayGuide *guide = CLAY_GUIDE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, guide->priv->orientation);
      break;
    case PROP_POSITION:
      g_value_set_int (value, guide->priv->position);
      break;
    case PROP_STYLE:
      g_value_set_enum (value, guide->priv->style);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_guide_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  ClayGuide *guide = CLAY_GUIDE (object);

  switch (property_id)
    {
    case PROP_ORIENTATION:
      guide->priv->orientation = g_value_get_enum (value);
      break;
    case PROP_POSITION:
      guide->priv->position = g_value_get_int (value);
      break;
    case PROP_STYLE:
      guide->priv->style = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_guide_save (ClaySavable   *savable,
                 ClaySaveState *state)
{
  ClayGuide *guide = CLAY_GUIDE (savable);

  clay_savable_print_element (state, "guide", NULL, NULL,
                              "orientation", "%[ClayOrientationType]", guide->priv->orientation,
                              "position",    "%d",                     guide->priv->position,
                              NULL);
}

static gboolean
clay_guide_load_enter (ClayLoadState  *state,
                       const gchar   **attribute_names,
                       const gchar   **attribute_values,
                       gpointer        user_data,
                       GError        **error)
{
  const gchar *orientation = NULL;
  const gchar *position    = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "orientation") == 0)
        orientation = *attribute_values;
      else if (g_strcmp0 (*attribute_names, "position") == 0)
        position = *attribute_values;
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  if ((! orientation || ! position) && *error == NULL)
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: missing orientation and/or position attributes.",
                   G_STRFUNC);
    }
  else
    {
      ClayOrientationType o = CLAY_ORIENTATION_UNKNOWN;
      gint                p = -1;

      clay_savable_load_store_from_string (state, error,
                                           "orientation", CLAY_TYPE_ORIENTATION_TYPE, orientation,
                                           "position",    G_TYPE_INT,                 position,
                                           NULL);
      clay_savable_load_get_values (state,
                                    "orientation", &o,
                                    "position",    &p,
                                    NULL);
      switch (o)
        {
        case CLAY_ORIENTATION_HORIZONTAL:
          clay_image_add_hguide (state->image, p, FALSE);
          break;
        case CLAY_ORIENTATION_VERTICAL:
          clay_image_add_vguide (state->image, p, FALSE);
          break;
        default:
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_DATA,
                       "%s: ignoring guide of orientation '%s' and position %s.\n",
                       G_STRFUNC, orientation, position);
        }
    }

  return TRUE;
}

ClayGuide *
clay_guide_new (ClayOrientationType  orientation,
                guint32              guide_ID)
{
  return g_object_new (CLAY_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       "style",       CLAY_GUIDE_STYLE_NORMAL,
                       NULL);
}

/**
 * clay_guide_custom_new:
 * @orientation:       the #ClayOrientationType
 * @guide_ID:          the unique guide ID
 * @guide_style:       the #ClayGuideStyle
 *
 * This function returns a new guide and will flag it as "custom".
 * Custom guides are used for purpose "other" than the basic guides
 * a user can create oneself, for instance as symmetry guides, to
 * drive GEGL ops, etc.
 * They are not saved in the XCF file. If an op, a symmetry or a plugin
 * wishes to save its state, it has to do it internally.
 * Moreover they don't follow guide snapping settings and never snap.
 *
 * Returns: the custom #ClayGuide.
 **/
ClayGuide *
clay_guide_custom_new (ClayOrientationType  orientation,
                       guint32              guide_ID,
                       ClayGuideStyle       guide_style)
{
  return g_object_new (CLAY_TYPE_GUIDE,
                       "id",          guide_ID,
                       "orientation", orientation,
                       "style",       guide_style,
                       NULL);
}

ClayOrientationType
clay_guide_get_orientation (ClayGuide *guide)
{
  g_return_val_if_fail (CLAY_IS_GUIDE (guide), CLAY_ORIENTATION_UNKNOWN);

  return guide->priv->orientation;
}

void
clay_guide_set_orientation (ClayGuide           *guide,
                            ClayOrientationType  orientation)
{
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  guide->priv->orientation = orientation;

  g_object_notify (G_OBJECT (guide), "orientation");
}

gint
clay_guide_get_position (ClayGuide *guide)
{
  g_return_val_if_fail (CLAY_IS_GUIDE (guide), CLAY_GUIDE_POSITION_UNDEFINED);

  return guide->priv->position;
}

void
clay_guide_set_position (ClayGuide *guide,
                         gint       position)
{
  g_return_if_fail (CLAY_IS_GUIDE (guide));

  guide->priv->position = position;

  g_object_notify (G_OBJECT (guide), "position");
}

ClayGuideStyle
clay_guide_get_style (ClayGuide *guide)
{
  g_return_val_if_fail (CLAY_IS_GUIDE (guide), CLAY_GUIDE_STYLE_NONE);

  return guide->priv->style;
}

gboolean
clay_guide_is_custom (ClayGuide *guide)
{
  g_return_val_if_fail (CLAY_IS_GUIDE (guide), FALSE);

  return guide->priv->style != CLAY_GUIDE_STYLE_NORMAL;
}
