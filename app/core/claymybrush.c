/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymybrush.c
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "clay-memsize.h"
#include "claymybrush.h"
#include "claymybrush-load.h"
#include "claymybrush-private.h"
#include "claytagged.h"

#include "clay-intl.h"


static void          clay_mybrush_tagged_iface_init     (ClayTaggedInterface  *iface);

static void          clay_mybrush_finalize              (GObject              *object);
static void          clay_mybrush_set_property          (GObject              *object,
                                                         guint                 property_id,
                                                         const GValue         *value,
                                                         GParamSpec           *pspec);
static void          clay_mybrush_get_property          (GObject              *object,
                                                         guint                 property_id,
                                                         GValue               *value,
                                                         GParamSpec           *pspec);

static gint64        clay_mybrush_get_memsize           (ClayObject           *object,
                                                         gint64               *gui_size);

static gchar       * clay_mybrush_get_description       (ClayViewable         *viewable,
                                                         gchar               **tooltip);

static void          clay_mybrush_dirty                 (ClayData             *data);
static const gchar * clay_mybrush_get_extension         (ClayData             *data);

static gchar       * clay_mybrush_get_checksum          (ClayTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (ClayMybrush, clay_mybrush, CLAY_TYPE_DATA,
                         G_ADD_PRIVATE (ClayMybrush)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_TAGGED,
                                                clay_mybrush_tagged_iface_init))

#define parent_class clay_mybrush_parent_class


static void
clay_mybrush_class_init (ClayMybrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayDataClass     *data_class        = CLAY_DATA_CLASS (klass);

  object_class->finalize            = clay_mybrush_finalize;
  object_class->get_property        = clay_mybrush_get_property;
  object_class->set_property        = clay_mybrush_set_property;

  clay_object_class->get_memsize    = clay_mybrush_get_memsize;

  viewable_class->default_icon_name = "clay-tool-mypaint-brush";
  viewable_class->get_description   = clay_mybrush_get_description;

  data_class->dirty                 = clay_mybrush_dirty;
  data_class->get_extension         = clay_mybrush_get_extension;
}

static void
clay_mybrush_tagged_iface_init (ClayTaggedInterface *iface)
{
  iface->get_checksum = clay_mybrush_get_checksum;
}

static void
clay_mybrush_init (ClayMybrush *brush)
{
  brush->priv = clay_mybrush_get_instance_private (brush);

  brush->priv->radius        = 1.0;
  brush->priv->opaque        = 1.0;
  brush->priv->hardness      = 1.0;
  brush->priv->gain          = 0.0;
  brush->priv->pigment       = 0.0;
  brush->priv->posterize     = 0.0;
  brush->priv->posterize_num = 1.0;
  brush->priv->eraser        = FALSE;
}

static void
clay_mybrush_finalize (GObject *object)
{
  ClayMybrush *brush = CLAY_MYBRUSH (object);

  g_clear_pointer (&brush->priv->brush_json, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_mybrush_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_mybrush_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_mybrush_get_memsize (ClayObject *object,
                          gint64     *gui_size)
{
  ClayMybrush *brush   = CLAY_MYBRUSH (object);
  gint64       memsize = 0;

  memsize += clay_string_get_memsize (brush->priv->brush_json);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gchar *
clay_mybrush_get_description (ClayViewable  *viewable,
                              gchar        **tooltip)
{
  ClayMybrush *brush = CLAY_MYBRUSH (viewable);

  return g_strdup_printf ("%s",
                          clay_object_get_name (brush));
}

static void
clay_mybrush_dirty (ClayData *data)
{
  CLAY_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
clay_mybrush_get_extension (ClayData *data)
{
  return CLAY_MYBRUSH_FILE_EXTENSION;
}

static gchar *
clay_mybrush_get_checksum (ClayTagged *tagged)
{
  ClayMybrush *brush           = CLAY_MYBRUSH (tagged);
  gchar       *checksum_string = NULL;

  if (brush->priv->brush_json)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum,
                         (const guchar *) brush->priv->brush_json,
                         strlen (brush->priv->brush_json));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }

  return checksum_string;
}

/*  public functions  */

ClayData *
clay_mybrush_new (ClayContext *context,
                  const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (CLAY_TYPE_MYBRUSH,
                       "name",      name,
                       "mime-type", "image/x-clay-myb",
                       NULL);
}

ClayData *
clay_mybrush_get_standard (ClayContext *context)
{
  static ClayData *standard_mybrush = NULL;

  if (! standard_mybrush)
    {
      g_set_weak_pointer (&standard_mybrush,
                          clay_mybrush_new (context, "Standard"));

      clay_data_clean (standard_mybrush);
      clay_data_make_internal (standard_mybrush, "clay-mybrush-standard");
    }

  return standard_mybrush;
}

const gchar *
clay_mybrush_get_brush_json (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), NULL);

  return brush->priv->brush_json;
}

gdouble
clay_mybrush_get_radius (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 1.0);

  return brush->priv->radius;
}

gdouble
clay_mybrush_get_opaque (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 1.0);

  return brush->priv->opaque;
}

gdouble
clay_mybrush_get_hardness (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 1.0);

  return brush->priv->hardness;
}

gdouble
clay_mybrush_get_gain (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 0.0);

  return brush->priv->gain;
}


gdouble
clay_mybrush_get_pigment (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 0.0);

  return brush->priv->pigment;
}

gdouble
clay_mybrush_get_posterize (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 0.0);

  return brush->priv->posterize;
}

gdouble
clay_mybrush_get_posterize_num (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 0.05);

  return brush->priv->posterize_num;
}

gdouble
clay_mybrush_get_offset_by_random (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), 1.0);

  return brush->priv->offset_by_random;
}

gboolean
clay_mybrush_get_is_eraser (ClayMybrush *brush)
{
  g_return_val_if_fail (CLAY_IS_MYBRUSH (brush), FALSE);

  return brush->priv->eraser;
}
