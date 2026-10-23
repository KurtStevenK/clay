/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayparamspecs-body.c
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

/*  this file is included by both
 *
 *  libclay/clayparamspecs.c
 *  app/core/clayparamspecs.c
 */

/*
 * CLAY_TYPE_PARAM_IMAGE
 */

#define CLAY_PARAM_SPEC_IMAGE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_IMAGE, ClayParamSpecImage))

typedef struct _ClayParamSpecImage ClayParamSpecImage;

struct _ClayParamSpecImage
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

static void       clay_param_image_class_init (GParamSpecClass *klass);
static void       clay_param_image_init       (GParamSpec      *pspec);
static gboolean   clay_param_image_validate   (GParamSpec      *pspec,
                                               GValue          *value);

GType
clay_param_image_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_image_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecImage),
        0,
        (GInstanceInitFunc) clay_param_image_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamImage", &info, 0);
    }

  return type;
}

static void
clay_param_image_class_init (GParamSpecClass *klass)
{
  klass->value_type     = CLAY_TYPE_IMAGE;
  klass->value_validate = clay_param_image_validate;
}

static void
clay_param_image_init (GParamSpec *pspec)
{
  ClayParamSpecImage *ispec = CLAY_PARAM_SPEC_IMAGE (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
clay_param_image_validate (GParamSpec *pspec,
                           GValue     *value)
{
  ClayParamSpecImage *ispec = CLAY_PARAM_SPEC_IMAGE (pspec);
  ClayImage          *image = value->data[0].v_pointer;

  if (! ispec->none_ok && image == NULL)
    return TRUE;

  if (image && (! CLAY_IS_IMAGE (image) ||
                ! clay_image_is_valid (image)))
    {
      g_object_unref (image);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_param_spec_image:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecImage specifying a
 * [type@Image] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecImage.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_image (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  ClayParamSpecImage *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_IMAGE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

/**
 * clay_param_spec_image_none_allowed:
 * @pspec: a #GParamSpec to hold a [class@Clay.Image] value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
clay_param_spec_image_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_IMAGE (pspec), FALSE);

  return CLAY_PARAM_SPEC_IMAGE (pspec)->none_ok;
}


/*
 * CLAY_TYPE_PARAM_ITEM
 */

#define CLAY_PARAM_SPEC_ITEM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_ITEM, ClayParamSpecItem))

typedef struct _ClayParamSpecItem ClayParamSpecItem;

struct _ClayParamSpecItem
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

static void       clay_param_item_class_init (GParamSpecClass *klass);
static void       clay_param_item_init       (GParamSpec      *pspec);
static gboolean   clay_param_item_validate   (GParamSpec      *pspec,
                                              GValue          *value);

GType
clay_param_item_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_item_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_item_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamItem", &info, 0);
    }

  return type;
}

static void
clay_param_item_class_init (GParamSpecClass *klass)
{
  klass->value_type     = CLAY_TYPE_ITEM;
  klass->value_validate = clay_param_item_validate;
}

static void
clay_param_item_init (GParamSpec *pspec)
{
  ClayParamSpecItem *ispec = CLAY_PARAM_SPEC_ITEM (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
clay_param_item_validate (GParamSpec *pspec,
                          GValue     *value)
{
  ClayParamSpecItem *ispec = CLAY_PARAM_SPEC_ITEM (pspec);
  ClayItem          *item  = value->data[0].v_pointer;

  if (! ispec->none_ok && item == NULL)
    return TRUE;

  if (item && (! g_type_is_a (G_OBJECT_TYPE (item), pspec->value_type) ||
               ! clay_item_is_valid (item)))
    {
      g_object_unref (item);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_param_spec_item:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecItem specifying a
 * [type@Item] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecItem.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_item (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_ITEM,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok;

  return G_PARAM_SPEC (ispec);
}

/**
 * clay_param_spec_item_none_allowed:
 * @pspec: a #GParamSpec to hold a [class@Clay.Item] value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
clay_param_spec_item_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_ITEM (pspec), FALSE);

  return CLAY_PARAM_SPEC_ITEM (pspec)->none_ok;
}


/*
 * CLAY_TYPE_PARAM_DRAWABLE
 */

static void   clay_param_drawable_class_init (GParamSpecClass *klass);
static void   clay_param_drawable_init       (GParamSpec      *pspec);

GType
clay_param_drawable_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_drawable_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_drawable_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_ITEM,
                                     "ClayParamDrawable", &info, 0);
    }

  return type;
}

static void
clay_param_drawable_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_DRAWABLE;
}

static void
clay_param_drawable_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_drawable:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecDrawable specifying a
 * [type@Drawable] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecDrawable.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_drawable (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_DRAWABLE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_LAYER
 */

static void   clay_param_layer_class_init (GParamSpecClass *klass);
static void   clay_param_layer_init       (GParamSpec      *pspec);

GType
clay_param_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_layer_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_layer_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_DRAWABLE,
                                     "ClayParamLayer", &info, 0);
    }

  return type;
}

static void
clay_param_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_LAYER;
}

static void
clay_param_layer_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecLayer specifying a
 * [type@Layer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_layer (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_TEXT_LAYER
 */

static void   clay_param_text_layer_class_init (GParamSpecClass *klass);
static void   clay_param_text_layer_init       (GParamSpec      *pspec);

GType
clay_param_text_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_text_layer_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_text_layer_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_LAYER,
                                     "ClayParamTextLayer", &info, 0);
    }

  return type;
}

static void
clay_param_text_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_TEXT_LAYER;
}

static void
clay_param_text_layer_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_text_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecTextLayer specifying a
 * [type@TextLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecTextLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_text_layer (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_TEXT_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_VECTOR_LAYER
 */

static void   clay_param_vector_layer_class_init (GParamSpecClass *klass);
static void   clay_param_vector_layer_init       (GParamSpec      *pspec);

GType
clay_param_vector_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_vector_layer_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_vector_layer_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_LAYER,
                                     "ClayParamVectorLayer", &info, 0);
    }

  return type;
}

static void
clay_param_vector_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_VECTOR_LAYER;
}

static void
clay_param_vector_layer_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_vector_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecVectorLayer specifying a
 * [type@VectorLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecVectorLayer.
 *
 * Since: 3.2
 **/
GParamSpec *
clay_param_spec_vector_layer (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              gboolean     none_ok,
                              GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_VECTOR_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_LINK_LAYER
 */

static void   clay_param_link_layer_class_init (GParamSpecClass *klass);
static void   clay_param_link_layer_init       (GParamSpec      *pspec);

GType
clay_param_link_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_link_layer_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_link_layer_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_LAYER,
                                     "ClayParamLinkLayer", &info, 0);
    }

  return type;
}

static void
clay_param_link_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_LINK_LAYER;
}

static void
clay_param_link_layer_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_link_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecLinkLayer specifying a
 * [type@LinkLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecLinkLayer.
 *
 * Since: 3.2
 **/
GParamSpec *
clay_param_spec_link_layer (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_LINK_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_RASTERIZABLE
 */

static void   clay_param_rasterizable_class_init (GParamSpecClass *klass);
static void   clay_param_rasterizable_init       (GParamSpec      *pspec);

GType
clay_param_rasterizable_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_rasterizable_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_rasterizable_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_DRAWABLE,
                                     "ClayParamRasterizable", &info, 0);
    }

  return type;
}

static void
clay_param_rasterizable_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_RASTERIZABLE;
}

static void
clay_param_rasterizable_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_rasterizable:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecRasterizable specifying a
 * [type@Rasterizable] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created
 * #ClayParamSpecRasterizable.
 *
 * Since: 3.2
 **/
GParamSpec *
clay_param_spec_rasterizable (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              gboolean     none_ok,
                              GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_RASTERIZABLE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_GROUP_LAYER
 */

static void   clay_param_group_layer_class_init (GParamSpecClass *klass);
static void   clay_param_group_layer_init       (GParamSpec      *pspec);

GType
clay_param_group_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_group_layer_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_group_layer_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_LAYER,
                                     "ClayParamGroupLayer", &info, 0);
    }

  return type;
}

static void
clay_param_group_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_GROUP_LAYER;
}

static void
clay_param_group_layer_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_group_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecGroupLayer specifying a
 * [type@GroupLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecGroupLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_group_layer (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             gboolean     none_ok,
                             GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_GROUP_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_CHANNEL
 */

static void   clay_param_channel_class_init (GParamSpecClass *klass);
static void   clay_param_channel_init       (GParamSpec      *pspec);

GType
clay_param_channel_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_channel_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_channel_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_DRAWABLE,
                                     "ClayParamChannel", &info, 0);
    }

  return type;
}

static void
clay_param_channel_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_CHANNEL;
}

static void
clay_param_channel_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_channel:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecChannel specifying a
 * [type@Channel] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecChannel.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_channel (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_CHANNEL,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_LAYER_MASK
 */

static void   clay_param_layer_mask_class_init (GParamSpecClass *klass);
static void   clay_param_layer_mask_init       (GParamSpec      *pspec);

GType
clay_param_layer_mask_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_layer_mask_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_layer_mask_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_CHANNEL,
                                     "ClayParamLayerMask", &info, 0);
    }

  return type;
}

static void
clay_param_layer_mask_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_LAYER_MASK;
}

static void
clay_param_layer_mask_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_layer_mask:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecLayerMask specifying a
 * [type@LayerMask] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecLayerMask.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_layer_mask (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_LAYER_MASK,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_SELECTION
 */

static void   clay_param_selection_class_init (GParamSpecClass *klass);
static void   clay_param_selection_init       (GParamSpec      *pspec);

GType
clay_param_selection_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_selection_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_selection_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_CHANNEL,
                                     "ClayParamSelection", &info, 0);
    }

  return type;
}

static void
clay_param_selection_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_SELECTION;
}

static void
clay_param_selection_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_selection:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecSelection specifying a
 * [type@Selection] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecSelection.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_selection (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           gboolean     none_ok,
                           GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_SELECTION,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_PATH
 */

static void   clay_param_path_class_init (GParamSpecClass *klass);
static void   clay_param_path_init       (GParamSpec      *pspec);

GType
clay_param_path_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_path_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecItem),
        0,
        (GInstanceInitFunc) clay_param_path_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_ITEM,
                                     "ClayParamPath", &info, 0);
    }

  return type;
}

static void
clay_param_path_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_PATH;
}

static void
clay_param_path_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_path:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecPath specifying a
 * [type@Path] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecPath.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_path (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  ClayParamSpecItem *ispec;

  ispec = g_param_spec_internal (CLAY_TYPE_PARAM_PATH,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * CLAY_TYPE_PARAM_DRAWABLE_FILTER
 */

#define CLAY_PARAM_SPEC_DRAWABLE_FILTER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_DRAWABLE_FILTER, ClayParamSpecDrawableFilter))

typedef struct _ClayParamSpecDrawableFilter ClayParamSpecDrawableFilter;

struct _ClayParamSpecDrawableFilter
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

static void   clay_param_drawable_filter_class_init (GParamSpecClass *klass);
static void   clay_param_drawable_filter_init       (GParamSpec      *pspec);

GType
clay_param_drawable_filter_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_drawable_filter_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecDrawableFilter),
        0,
        (GInstanceInitFunc) clay_param_drawable_filter_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamDrawableFilter", &info, 0);
    }

  return type;
}

static void
clay_param_drawable_filter_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_DRAWABLE_FILTER;
}

static void
clay_param_drawable_filter_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_drawable_filter:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecDrawableFilter specifying a
 * [type@DrawableFilter] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecPath.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_drawable_filter (const gchar *name,
                                 const gchar *nick,
                                 const gchar *blurb,
                                 gboolean     none_ok,
                                 GParamFlags  flags)
{
  ClayParamSpecDrawableFilter *fspec;

  fspec = g_param_spec_internal (CLAY_TYPE_PARAM_DRAWABLE_FILTER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (fspec, NULL);

  fspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (fspec);
}

/**
 * clay_param_spec_drawable_filter_none_allowed:
 * @pspec: a #GParamSpec to hold a [class@Clay.DrawableFilter] value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
clay_param_spec_drawable_filter_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_DRAWABLE_FILTER (pspec), FALSE);

  return CLAY_PARAM_SPEC_DRAWABLE_FILTER (pspec)->none_ok;
}


/*
 * CLAY_TYPE_PARAM_DISPLAY
 */

#define CLAY_PARAM_SPEC_DISPLAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_DISPLAY, ClayParamSpecDisplay))

typedef struct _ClayParamSpecDisplay ClayParamSpecDisplay;

struct _ClayParamSpecDisplay
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

static void       clay_param_display_class_init (GParamSpecClass *klass);
static void       clay_param_display_init       (GParamSpec      *pspec);
static gboolean   clay_param_display_validate   (GParamSpec      *pspec,
                                                 GValue          *value);

GType
clay_param_display_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_display_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecDisplay),
        0,
        (GInstanceInitFunc) clay_param_display_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamDisplay", &info, 0);
    }

  return type;
}

static void
clay_param_display_class_init (GParamSpecClass *klass)
{
  klass->value_type     = CLAY_TYPE_DISPLAY;
  klass->value_validate = clay_param_display_validate;
}

static void
clay_param_display_init (GParamSpec *pspec)
{
  ClayParamSpecDisplay *dspec = CLAY_PARAM_SPEC_DISPLAY (pspec);

  dspec->none_ok = FALSE;
}

static gboolean
clay_param_display_validate (GParamSpec *pspec,
                             GValue     *value)
{
  ClayParamSpecDisplay *dspec   = CLAY_PARAM_SPEC_DISPLAY (pspec);
  ClayDisplay          *display = value->data[0].v_pointer;

  if (! dspec->none_ok && display == NULL)
   return TRUE;

  if (display && (! CLAY_IS_DISPLAY (display) ||
                  ! clay_display_is_valid (display)))
    {
      g_object_unref (display);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_param_spec_display:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecDisplay specifying a
 * [type@Display] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecDisplay.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_display (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  ClayParamSpecDisplay *dspec;

  dspec = g_param_spec_internal (CLAY_TYPE_PARAM_DISPLAY,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (dspec, NULL);

  dspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (dspec);
}

/**
 * clay_param_spec_display_none_allowed:
 * @pspec: a #GParamSpec to hold a [class@Clay.Display] value.
 *
 * Returns: %TRUE if a %NULL value is allowed.
 *
 * Since: 3.0
 **/
gboolean
clay_param_spec_display_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_DISPLAY (pspec), FALSE);

  return CLAY_PARAM_SPEC_DISPLAY (pspec)->none_ok;
}


/*
 * CLAY_TYPE_PARAM_RESOURCE
 */

#define CLAY_PARAM_SPEC_RESOURCE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_RESOURCE, ClayParamSpecResource))

typedef struct _ClayParamSpecResource ClayParamSpecResource;

struct _ClayParamSpecResource
{
  ClayParamSpecObject  parent_instance;

  gboolean             none_ok;
  gboolean             default_to_context;
};

static void         clay_param_resource_class_init  (ClayParamSpecObjectClass *klass);
static void         clay_param_resource_init        (GParamSpec               *pspec);
static GParamSpec * clay_param_resource_duplicate   (GParamSpec               *pspec);
static GObject    * clay_param_resource_get_default (GParamSpec               *pspec);
static gboolean     clay_param_resource_validate    (GParamSpec               *pspec,
                                                     GValue                   *value);


GType
clay_param_resource_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_resource_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_resource_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_OBJECT, "ClayParamResource", &info, 0);
    }

  return type;
}

static void
clay_param_resource_class_init (ClayParamSpecObjectClass *klass)
{
  GParamSpecClass *pclass = G_PARAM_SPEC_CLASS (klass);

  klass->duplicate       = clay_param_resource_duplicate;
  klass->get_default     = clay_param_resource_get_default;

  pclass->value_type     = CLAY_TYPE_RESOURCE;
  pclass->value_validate = clay_param_resource_validate;
}

static void
clay_param_resource_init (GParamSpec *pspec)
{
  ClayParamSpecResource *rspec = CLAY_PARAM_SPEC_RESOURCE (pspec);

  rspec->none_ok            = FALSE;
  rspec->default_to_context = FALSE;
}

static GParamSpec *
clay_param_resource_duplicate (GParamSpec *pspec)
{
  GParamSpec *duplicate;
  GObject    *default_value = NULL;

  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_RESOURCE (pspec), NULL);

  if (clay_param_spec_object_has_default (pspec) &&
      ! CLAY_PARAM_SPEC_RESOURCE (pspec)->default_to_context)
    default_value = clay_param_spec_object_get_default (pspec);

  duplicate = clay_param_spec_resource (pspec->name,
                                        g_param_spec_get_nick (pspec),
                                        g_param_spec_get_blurb (pspec),
                                        pspec->value_type,
                                        CLAY_PARAM_SPEC_RESOURCE (pspec)->none_ok,
                                        CLAY_RESOURCE (default_value),
                                        CLAY_PARAM_SPEC_RESOURCE (pspec)->default_to_context,
                                        pspec->flags);

  return duplicate;
}

#ifdef LIBCLAY_COMPILATION
#define CLAY_USER_CONTEXT
#else
#define CLAY_USER_CONTEXT clay_get_user_context (clay_core_app_get_clay (CLAY_CORE_APP (g_application_get_default ())))
#endif

static GObject *
clay_param_resource_get_default (GParamSpec *pspec)
{
  ClayParamSpecResource    *rspec;
  ClayParamSpecObjectClass *parent_class = g_type_class_peek (CLAY_TYPE_PARAM_OBJECT);

  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_RESOURCE (pspec), NULL);

  rspec = CLAY_PARAM_SPEC_RESOURCE (pspec);

  if (rspec->default_to_context)
    {
      if (CLAY_IS_PARAM_SPEC_BRUSH (pspec))
        return G_OBJECT (clay_context_get_brush (CLAY_USER_CONTEXT));
      else if (CLAY_IS_PARAM_SPEC_PATTERN (pspec))
        return G_OBJECT (clay_context_get_pattern (CLAY_USER_CONTEXT));
      else if (CLAY_IS_PARAM_SPEC_GRADIENT (pspec))
        return G_OBJECT (clay_context_get_gradient (CLAY_USER_CONTEXT));
      else if (CLAY_IS_PARAM_SPEC_PALETTE (pspec))
        return G_OBJECT (clay_context_get_palette (CLAY_USER_CONTEXT));
      else if (CLAY_IS_PARAM_SPEC_FONT (pspec))
        return G_OBJECT (clay_context_get_font (CLAY_USER_CONTEXT));
      else
        g_return_val_if_reached (NULL);
    }
  else
    {
      return parent_class->get_default (pspec);
    }
}

#undef CLAY_USER_CONTEXT

static gboolean
clay_param_resource_validate (GParamSpec *pspec,
                              GValue     *value)
{
  ClayParamSpecResource *rspec    = CLAY_PARAM_SPEC_RESOURCE (pspec);
  GObject               *resource = value->data[0].v_pointer;

  if (resource == clay_param_resource_get_default (pspec))
    /* This handles in particular the particular case where none_ok is
     * FALSE yet the context's default is NULL. We let this
     * near-inconsistency pass.
     */
    return FALSE;

  if (! rspec->none_ok && resource == NULL)
    {
      g_set_object (&value->data[0].v_pointer, clay_param_resource_get_default (pspec));
      return TRUE;
    }

  if (resource && (! g_type_is_a (G_OBJECT_TYPE (resource), pspec->value_type) ||
                   ! clay_resource_is_valid ((gpointer) resource)))
    {
      g_object_unref (resource);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_param_spec_resource:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @resource_type: a %GType, subtype of [class@Clay.Resource].
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default resource.
 * @default_to_context: Whether the context is the context's resource
 *                      for the given @resource_type.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecResource specifying a [type@Resource] property.
 * See [func@GObject.ParamSpec.internal] for details on property names.
 *
 * @default_to_context cannot be %TRUE for a @resource_type of value
 * [class@Clay.Resource], but only for specific subtypes. If it is
 * %TRUE, @default_value must be %NULL. Instead of a fixed default,
 * whatever is the context's resource for the given type at run time
 * will be used as dynamic default.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecResource.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_resource (const gchar  *name,
                          const gchar  *nick,
                          const gchar  *blurb,
                          GType         resource_type,
                          gboolean      none_ok,
                          ClayResource *default_value,
                          gboolean      default_to_context,
                          GParamFlags   flags)
{
  ClayParamSpecResource *rspec;
  GType                  param_type;

  g_return_val_if_fail (g_type_is_a (resource_type, CLAY_TYPE_RESOURCE) ||
                        g_type_is_a (resource_type, CLAY_TYPE_PARAM_RESOURCE), NULL);

  if (! g_type_is_a (resource_type, CLAY_TYPE_PARAM_RESOURCE))
    {
      if (resource_type == CLAY_TYPE_RESOURCE ||
          resource_type == G_TYPE_NONE)
        param_type = CLAY_TYPE_PARAM_RESOURCE;
      else if (resource_type == CLAY_TYPE_BRUSH)
        param_type = CLAY_TYPE_PARAM_BRUSH;
      else if (resource_type == CLAY_TYPE_PATTERN)
        param_type = CLAY_TYPE_PARAM_PATTERN;
      else if (resource_type == CLAY_TYPE_GRADIENT)
        param_type = CLAY_TYPE_PARAM_GRADIENT;
      else if (resource_type == CLAY_TYPE_PALETTE)
        param_type = CLAY_TYPE_PARAM_PALETTE;
      else if (resource_type == CLAY_TYPE_FONT)
        param_type = CLAY_TYPE_PARAM_FONT;
      else
        g_return_val_if_reached (NULL);
    }
  else
    {
      param_type = resource_type;
    }
  g_return_val_if_fail (param_type != CLAY_TYPE_RESOURCE || ! default_to_context, NULL);

  rspec = g_param_spec_internal (param_type, name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok            = none_ok ? TRUE : FALSE;
  rspec->default_to_context = default_to_context ? TRUE : FALSE;

  if (! rspec->default_to_context)
    clay_param_spec_object_set_default (G_PARAM_SPEC (rspec), G_OBJECT (default_value));

  return G_PARAM_SPEC (rspec);
}

gboolean
clay_param_spec_resource_none_allowed (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_RESOURCE (pspec), FALSE);

  return CLAY_PARAM_SPEC_RESOURCE (pspec)->none_ok;
}

gboolean
clay_param_spec_resource_defaults_to_context (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_RESOURCE (pspec), FALSE);

  return CLAY_PARAM_SPEC_RESOURCE (pspec)->default_to_context;
}


/*
 * CLAY_TYPE_PARAM_BRUSH
 */

static void   clay_param_brush_class_init (GParamSpecClass *klass);
static void   clay_param_brush_init       (GParamSpec      *pspec);

GType
clay_param_brush_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_brush_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_brush_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_RESOURCE,
                                     "ClayParamBrush", &info, 0);
    }

  return type;
}

static void
clay_param_brush_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_BRUSH;
}

static void
clay_param_brush_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_brush:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default brush.
 * @default_to_context: Whether the context is the context's brush.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecBrush specifying a
 * [type@Brush] property. See also [func@Clay.param_spec_resource].
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecBrush.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_brush (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       ClayBrush   *default_value,
                       gboolean     default_to_context,
                       GParamFlags  flags)
{
  return clay_param_spec_resource (name, nick, blurb, CLAY_TYPE_BRUSH,
                                   none_ok, CLAY_RESOURCE (default_value),
                                   default_to_context, flags);
}


/*
 * CLAY_TYPE_PARAM_PATTERN
 */

static void   clay_param_pattern_class_init (GParamSpecClass *klass);
static void   clay_param_pattern_init       (GParamSpec      *pspec);

GType
clay_param_pattern_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_pattern_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_pattern_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_RESOURCE,
                                     "ClayParamPattern", &info, 0);
    }

  return type;
}

static void
clay_param_pattern_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_PATTERN;
}

static void
clay_param_pattern_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_pattern:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default pattern.
 * @default_to_context: Whether the context is the context's pattern.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecPattern specifying a
 * [type@Pattern] property. See also [func@Clay.param_spec_resource].
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecPattern.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_pattern (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         ClayPattern *default_value,
                         gboolean     default_to_context,
                         GParamFlags  flags)
{
  return clay_param_spec_resource (name, nick, blurb, CLAY_TYPE_PATTERN,
                                   none_ok, CLAY_RESOURCE (default_value),
                                   default_to_context, flags);
}


/*
 * CLAY_TYPE_PARAM_GRADIENT
 */

static void   clay_param_gradient_class_init (GParamSpecClass *klass);
static void   clay_param_gradient_init       (GParamSpec      *pspec);

GType
clay_param_gradient_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_gradient_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_gradient_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_RESOURCE,
                                     "ClayParamGradient", &info, 0);
    }

  return type;
}

static void
clay_param_gradient_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_GRADIENT;
}

static void
clay_param_gradient_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_gradient:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default gradient.
 * @default_to_context: Whether the context is the context's gradient.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecGradient specifying a
 * [type@Gradient] property. See also [func@Clay.param_spec_resource].
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecGradient.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_gradient (const gchar  *name,
                          const gchar  *nick,
                          const gchar  *blurb,
                          gboolean      none_ok,
                          ClayGradient *default_value,
                          gboolean      default_to_context,
                          GParamFlags   flags)
{
  return clay_param_spec_resource (name, nick, blurb, CLAY_TYPE_GRADIENT,
                                   none_ok, CLAY_RESOURCE (default_value),
                                   default_to_context, flags);
}


/*
 * CLAY_TYPE_PARAM_PALETTE
 */

static void   clay_param_palette_class_init (GParamSpecClass *klass);
static void   clay_param_palette_init       (GParamSpec      *pspec);

GType
clay_param_palette_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_palette_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_palette_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_RESOURCE,
                                     "ClayParamPalette", &info, 0);
    }

  return type;
}

static void
clay_param_palette_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_PALETTE;
}

static void
clay_param_palette_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_palette:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default palette.
 * @default_to_context: Whether the context is the context's palette.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecPalette specifying a
 * [type@Palette] property. See also [func@Clay.param_spec_resource].
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecPalette.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_palette (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         ClayPalette *default_value,
                         gboolean     default_to_context,
                         GParamFlags  flags)
{
  return clay_param_spec_resource (name, nick, blurb, CLAY_TYPE_PALETTE,
                                   none_ok, CLAY_RESOURCE (default_value),
                                   default_to_context, flags);
}


/*
 * CLAY_TYPE_PARAM_FONT
 */

static void   clay_param_font_class_init (GParamSpecClass *klass);
static void   clay_param_font_init       (GParamSpec      *pspec);

GType
clay_param_font_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (ClayParamSpecObjectClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_font_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecResource),
        0,
        (GInstanceInitFunc) clay_param_font_init
      };

      type = g_type_register_static (CLAY_TYPE_PARAM_RESOURCE,
                                     "ClayParamFont", &info, 0);
    }

  return type;
}

static void
clay_param_font_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_FONT;
}

static void
clay_param_font_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_font:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @default_value: (nullable): Default font.
 * @default_to_context: Whether the context is the context's font.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecFont specifying a
 * [type@Font] property. See also [func@Clay.param_spec_resource].
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecFont.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_font (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      ClayFont    *default_value,
                      gboolean     default_to_context,
                      GParamFlags  flags)
{
  return clay_param_spec_resource (name, nick, blurb, CLAY_TYPE_FONT,
                                   none_ok, CLAY_RESOURCE (default_value),
                                   default_to_context, flags);
}

/*
 * CLAY_TYPE_PARAM_CURVE
 */

#define CLAY_PARAM_SPEC_CURVE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_CURVE, ClayParamSpecCurve))

typedef struct _ClayParamSpecCurve ClayParamSpecCurve;

struct _ClayParamSpecCurve
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

static void   clay_param_curve_class_init (GParamSpecClass *klass);
static void   clay_param_curve_init       (GParamSpec      *pspec);

GType
clay_param_curve_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_curve_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecCurve),
        0,
        (GInstanceInitFunc) clay_param_curve_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "ClayParamCurve", &info, 0);
    }

  return type;
}

static void
clay_param_curve_class_init (GParamSpecClass *klass)
{
  klass->value_type = CLAY_TYPE_CURVE;
}

static void
clay_param_curve_init (GParamSpec *pspec)
{
}

/**
 * clay_param_spec_curve:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #ClayParamSpecCurve specifying a
 * [type@Curve] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecCurve.
 *
 * Since: 3.2
 **/
GParamSpec *
clay_param_spec_curve (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  ClayParamSpecCurve *cspec;

  cspec = g_param_spec_internal (CLAY_TYPE_PARAM_CURVE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (cspec, NULL);

  cspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (cspec);
}
