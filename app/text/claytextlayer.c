/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextLayer
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangocairo.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"

#include "text-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/clayimage.h"
#include "core/clayimage-color-profile.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/clayparasitelist.h"
#include "core/claypattern.h"
#include "core/clayrasterizable.h"
#include "core/claysavable.h"
#include "core/claysavable-load.h"
#include "core/claytempbuf.h"

#include "claytext.h"
#include "claytextlayer.h"
#include "claytextlayer-transform.h"
#include "claytextlayout.h"
#include "claytextlayout-render.h"

#include "clay-intl.h"


enum
{
  PROP_0,
  PROP_TEXT,
};

struct _ClayTextLayerPrivate
{
  ClayTextDirection base_dir;

  /* on-canvas editor position */
  gboolean          style_overlay_positioned;
  gdouble           style_overlay_x;
  gdouble           style_overlay_y;
  gdouble           style_overlay_offset_x;
  gdouble           style_overlay_offset_y;
};

static void       clay_text_layer_rasterizable_iface_init
                                                 (ClayRasterizableInterface *iface);
static void       clay_text_layer_savable_iface_init
                                                 (ClaySavableInterface      *iface);

static void       clay_text_layer_finalize       (GObject           *object);
static void       clay_text_layer_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void       clay_text_layer_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);

static void       clay_text_layer_set_rasterized (ClayRasterizable  *rasterizable,
                                                  gboolean           rasterized);

static void       clay_text_layer_save           (ClaySavable       *savable,
                                                  ClaySaveState     *state);
static gboolean   clay_text_layer_load_enter     (ClayLoadState     *state,
                                                  const gchar      **attribute_names,
                                                  const gchar      **attribute_values,
                                                  gpointer           user_data,
                                                  GError           **error);
static gboolean   clay_text_layer_load_exit      (ClayLoadState     *state,
                                                  const gchar       *text,
                                                  gsize              len,
                                                  gpointer           user_data,
                                                  GError           **error);

static gint64     clay_text_layer_get_memsize    (ClayObject        *object,
                                                  gint64            *gui_size);

static ClayItem * clay_text_layer_duplicate      (ClayItem          *item,
                                                  GType              new_type);

static void       clay_text_layer_set_buffer     (ClayDrawable      *drawable,
                                                  gboolean           push_undo,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  const GeglRectangle *bounds);
static void       clay_text_layer_push_undo      (ClayDrawable      *drawable,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height);

static void       clay_text_layer_convert_type   (ClayLayer         *layer,
                                                  ClayImage         *dest_image,
                                                  const Babl        *new_format,
                                                  ClayColorProfile  *src_profile,
                                                  ClayColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  ClayProgress      *progress);

static void       clay_text_layer_text_changed   (ClayTextLayer     *layer);
static gboolean   clay_text_layer_render         (ClayTextLayer     *layer);
static void       clay_text_layer_render_layout  (ClayTextLayer     *layer,
                                                  ClayTextLayout    *layout);


G_DEFINE_TYPE_WITH_CODE (ClayTextLayer, clay_text_layer, CLAY_TYPE_LAYER,
                         G_ADD_PRIVATE (ClayTextLayer)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_RASTERIZABLE,
                                                clay_text_layer_rasterizable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_text_layer_savable_iface_init))

#define parent_class clay_text_layer_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;


static void
clay_text_layer_class_init (ClayTextLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);
  ClayLayerClass    *layer_class       = CLAY_LAYER_CLASS (klass);

  object_class->finalize            = clay_text_layer_finalize;
  object_class->get_property        = clay_text_layer_get_property;
  object_class->set_property        = clay_text_layer_set_property;

  clay_object_class->get_memsize    = clay_text_layer_get_memsize;

  viewable_class->default_icon_name = "clay-text-layer";
  viewable_class->default_name      = _("Text Layer");

  item_class->duplicate             = clay_text_layer_duplicate;
  item_class->rename                = clay_rasterizable_rename;

  item_class->scale                 = clay_text_layer_scale;
  item_class->flip                  = clay_text_layer_flip;
  item_class->rotate                = clay_text_layer_rotate;
  item_class->transform             = clay_text_layer_transform;

  item_class->rename_desc           = _("Rename Text Layer");
  item_class->translate_desc        = _("Move Text Layer");
  item_class->scale_desc            = _("Scale Text Layer");
  item_class->resize_desc           = _("Resize Text Layer");
  item_class->flip_desc             = _("Flip Text Layer");
  item_class->rotate_desc           = _("Rotate Text Layer");
  item_class->transform_desc        = _("Transform Text Layer");

  drawable_class->set_buffer        = clay_text_layer_set_buffer;
  drawable_class->push_undo         = clay_text_layer_push_undo;

  layer_class->convert_type         = clay_text_layer_convert_type;

  CLAY_CONFIG_PROP_OBJECT (object_class, PROP_TEXT,
                           "text",
                           NULL, NULL,
                           CLAY_TYPE_TEXT,
                           CLAY_PARAM_STATIC_STRINGS);
}

static void
clay_text_layer_init (ClayTextLayer *layer)
{
  layer->text                 = NULL;
  layer->text_parasite        = NULL;
  layer->text_parasite_is_old = FALSE;
  layer->private              = clay_text_layer_get_instance_private (layer);
}

static void
clay_text_layer_rasterizable_iface_init (ClayRasterizableInterface *iface)
{
  iface->set_rasterized = clay_text_layer_set_rasterized;
}

static void
clay_text_layer_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "text-layer";
  iface->save       = clay_text_layer_save;
  iface->load_enter = clay_text_layer_load_enter;
  iface->load_exit  = clay_text_layer_load_exit;
}

static void
clay_text_layer_finalize (GObject *object)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (object);

  g_clear_object (&layer->text);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_text_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  ClayTextLayer *text_layer = CLAY_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_object (value, text_layer->text);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayTextLayer *text_layer = CLAY_TEXT_LAYER (object);

  switch (property_id)
    {
    case PROP_TEXT:
      clay_text_layer_set_text (text_layer, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_text_layer_set_rasterized (ClayRasterizable *rasterizable,
                                gboolean          rasterized)
{
  if (! rasterized)
    {
      if (! clay_drawable_has_alpha (CLAY_DRAWABLE (rasterizable)))
        clay_layer_add_alpha (CLAY_LAYER (rasterizable));

      clay_text_layer_render (CLAY_TEXT_LAYER (rasterizable));
    }
}

static void
clay_text_layer_save (ClaySavable   *savable,
                      ClaySaveState *state)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (savable);
  const gchar   *layer_name;

  layer_name = clay_object_get_name (CLAY_OBJECT (layer));
  clay_savable_print_element_start (state, "text-layer", "name", "%s", layer_name, NULL);

  parent_savable_iface->save (savable, state);

  clay_savable_config_save (CLAY_CONFIG (layer->text), "text", state);

  if (! clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (layer)))
    clay_savable_print_element (state, "auto-rename", NULL, NULL, "disabled", "%b", TRUE, NULL);

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_savable_print_element (state, "rasterized", NULL, NULL, NULL);

  clay_savable_print_element_end (state, "text-layer");
}

static gboolean
clay_text_layer_load_enter (ClayLoadState  *state,
                            const gchar   **attribute_names,
                            const gchar   **attribute_values,
                            gpointer        user_data,
                            GError        **error)
{
  ClayTextLayer *layer;
  const gchar   *name = NULL;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "name") == 0)
        name = *attribute_values;
      else
        g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                     "%s: unexpected attribute: '%s'",
                     G_STRFUNC, *attribute_names);

      attribute_names++;
      attribute_values++;
    }

  parent_savable_iface->load_enter (state, attribute_names, attribute_values,
                                    user_data, error);

  layer = CLAY_TEXT_LAYER (clay_drawable_new (CLAY_TYPE_TEXT_LAYER,
                                              state->image, NULL,
                                              0, 0, 1, 1,
                                              clay_image_get_layer_format (state->image,
                                                                           TRUE)));
  clay_object_set_name (CLAY_OBJECT (layer), name);
  /* We rasterize as a trick so that any settings update won't override
   * the buffer contents. We will actually set to proper state in
   * load_exit().
   */
  clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), FALSE);

  clay_savable_config_load (CLAY_TYPE_TEXT, "text", state, NULL,
                            "clay", CLAY_TYPE_CLAY, state->clay,
                            NULL);
  clay_savable_load_add_simple_handler (state, "auto-rename", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        "disabled", G_TYPE_BOOLEAN,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "rasterized", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);

  clay_savable_load_push_active_object (state, G_OBJECT (layer));

  return TRUE;
}

static gboolean
clay_text_layer_load_exit (ClayLoadState  *state,
                           const gchar    *text,
                           gsize           len,
                           gpointer        user_data,
                           GError        **error)
{
  ClayTextLayer *layer          = NULL;
  ClayText      *gtext          = NULL;
  gboolean       rasterized     = FALSE;
  gboolean       disable_rename = FALSE;

  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  /* Order matters because parent's load_exit() will pop the layer. */
  layer = CLAY_TEXT_LAYER (clay_savable_load_peek_active_object (state));
  parent_savable_iface->load_exit (state, text, len, user_data, error);

  clay_savable_load_get_values (state,
                                "rasterized",           &rasterized,
                                "auto-rename:disabled", &disable_rename,
                                "text",                 &gtext,
                                NULL);

  clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (layer), ! disable_rename);
  if (gtext)
    {
      clay_text_layer_set_text (layer, gtext);

      g_return_val_if_fail (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)), FALSE);

      if (! rasterized)
        clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));
    }
  else
    {
      CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                               "%s: this text layer failed to render: %s",
                               G_STRFUNC, clay_object_get_name (CLAY_OBJECT (layer)));
      clay_image_remove_layer (state->image,
                               CLAY_LAYER (layer),
                               FALSE, NULL);
    }

  return TRUE;
}

static gint64
clay_text_layer_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayTextLayer *text_layer = CLAY_TEXT_LAYER (object);
  gint64         memsize    = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (text_layer->text),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static ClayItem *
clay_text_layer_duplicate (ClayItem *item,
                           GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_TEXT_LAYER (new_item))
    {
      ClayTextLayer *layer     = CLAY_TEXT_LAYER (item);
      ClayTextLayer *new_layer = CLAY_TEXT_LAYER (new_item);

      clay_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      if (layer->text)
        {
          ClayText *text = clay_config_duplicate (CLAY_CONFIG (layer->text));

          clay_text_layer_set_text (new_layer, text);

          g_object_unref (text);
        }

      /*  this is just the parasite name, not a pointer to the parasite  */
      if (layer->text_parasite)
        {
          new_layer->text_parasite        = layer->text_parasite;
          new_layer->text_parasite_is_old = layer->text_parasite_is_old;
        }

      new_layer->private->base_dir = layer->private->base_dir;
      clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (new_layer),
                                         clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (layer)));

      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
        clay_rasterizable_set_undo_rasterized (CLAY_RASTERIZABLE (new_layer), TRUE);
    }

  return new_item;
}

static void
clay_text_layer_set_buffer (ClayDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (drawable);
  ClayImage     *image = clay_item_get_image (CLAY_ITEM (layer));
  gboolean       is_rasterized;

  is_rasterized = clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer));

  if (push_undo && ! is_rasterized)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! is_rasterized)
    {
      clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), push_undo);
      clay_image_undo_group_end (image);
    }
}

static void
clay_text_layer_push_undo (ClayDrawable *drawable,
                           const gchar  *undo_desc,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  ClayTextLayer *layer = CLAY_TEXT_LAYER (drawable);
  ClayImage     *image = clay_item_get_image (CLAY_ITEM (layer));
  gboolean       is_rasterized;

  is_rasterized = clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer));

  if (! is_rasterized)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE, undo_desc);

  CLAY_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! is_rasterized)
    {
      clay_rasterizable_rasterize (CLAY_RASTERIZABLE (layer), TRUE);
      clay_image_undo_group_end (image);
    }
}

static void
clay_text_layer_convert_type (ClayLayer         *layer,
                              ClayImage         *dest_image,
                              const Babl        *new_format,
                              ClayColorProfile  *src_profile,
                              ClayColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              ClayProgress      *progress)
{
  ClayTextLayer *text_layer = CLAY_TEXT_LAYER (layer);
  ClayImage     *image      = clay_item_get_image (CLAY_ITEM (text_layer));

  if (! text_layer->text                                          ||
      clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)) ||
      layer_dither_type != GEGL_DITHER_NONE)
    {
      CLAY_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
                                                     new_format,
                                                     src_profile,
                                                     dest_profile,
                                                     layer_dither_type,
                                                     mask_dither_type,
                                                     push_undo,
                                                     progress);
    }
  else
    {
      if (push_undo)
        clay_image_undo_push_text_layer_convert (image, NULL, text_layer);

      text_layer->convert_format = new_format;

      clay_text_layer_render (text_layer);

      text_layer->convert_format = NULL;
    }
}


/*  public functions  */

/**
 * clay_text_layer_new:
 * @image: the #ClayImage the layer should belong to
 * @text: a #ClayText object
 *
 * Creates a new text layer.
 *
 * Returns: (nullable): a new #ClayTextLayer or %NULL in case of a problem
 **/
ClayLayer *
clay_text_layer_new (ClayImage *image,
                     ClayText  *text)
{
  ClayTextLayer *layer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_TEXT (text), NULL);

  if (! text->text && ! text->markup)
    return NULL;

  layer =
    CLAY_TEXT_LAYER (clay_drawable_new (CLAY_TYPE_TEXT_LAYER,
                                        image, NULL,
                                        0, 0, 1, 1,
                                        clay_image_get_layer_format (image,
                                                                     TRUE)));

  clay_layer_set_mode (CLAY_LAYER (layer),
                       clay_image_get_default_new_layer_mode (image),
                       FALSE);

  clay_text_layer_set_text (layer, text);

  if (! clay_text_layer_render (layer))
    {
      g_object_unref (layer);
      return NULL;
    }

  return CLAY_LAYER (layer);
}

void
clay_text_layer_set_text (ClayTextLayer *layer,
                          ClayText      *text)
{
  g_return_if_fail (CLAY_IS_TEXT_LAYER (layer));
  g_return_if_fail (text == NULL || CLAY_IS_TEXT (text));

  if (layer->text == text)
    return;

  if (layer->text)
    {
      g_signal_handlers_disconnect_by_func (layer->text,
                                            G_CALLBACK (clay_text_layer_text_changed),
                                            layer);

      g_clear_object (&layer->text);
    }

  if (text)
    {
      layer->text = g_object_ref (text);
      layer->private->base_dir = layer->text->base_dir;

      g_signal_connect_object (text, "changed",
                               G_CALLBACK (clay_text_layer_text_changed),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "text");
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (layer));
}

ClayText *
clay_text_layer_get_text (ClayTextLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (layer), NULL);

  return layer->text;
}

void
clay_text_layer_set (ClayTextLayer *layer,
                     const gchar   *undo_desc,
                     const gchar   *first_property_name,
                     ...)
{
  ClayImage *image;
  ClayText  *text;
  va_list    var_args;

  g_return_if_fail (clay_item_is_text_layer (CLAY_ITEM (layer)));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (layer)));

  text = clay_text_layer_get_text (layer);
  if (! text)
    return;

  image = clay_item_get_image (CLAY_ITEM (layer));

  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TEXT, undo_desc);

  g_object_freeze_notify (G_OBJECT (layer));

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    {
      /*  pass copy_tiles = TRUE so we not only ref the tiles; after
       *  being a text layer again, undo doesn't care about the
       *  layer's pixels any longer because they are generated, so
       *  changing the text would happily overwrite the layer's
       *  pixels, changing the pixels on the undo stack too without
       *  any chance to ever undo again.
       */
      clay_image_undo_push_drawable_mod (image, NULL,
                                         CLAY_DRAWABLE (layer), TRUE);
    }

  clay_image_undo_push_text_layer (image, undo_desc, layer, NULL);

  va_start (var_args, first_property_name);
  g_object_set_valist (G_OBJECT (text), first_property_name, var_args);
  va_end (var_args);

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_rasterizable_restore (CLAY_RASTERIZABLE (layer));

  g_object_thaw_notify (G_OBJECT (layer));

  clay_image_undo_group_end (image);
}

gboolean
clay_item_is_text_layer (ClayItem *item)
{
  return (CLAY_IS_TEXT_LAYER (item) &&
          ! clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (item)));
}


void
clay_text_layer_set_style_overlay_position (ClayTextLayer *layer,
                                            gboolean       positioned,
                                            gdouble        x,
                                            gdouble        y)
{
  ClayTextLayerPrivate *priv;

  g_return_if_fail (CLAY_IS_TEXT_LAYER (layer));

  priv = layer->private;

  priv->style_overlay_positioned = positioned;

  /* We want to set "style_overlay_x" and "style_overlay_y" only
   * when "positioned" is TRUE. Otherwhise, we only want to set
   * "style_overlay_positioned" to FALSE */
  if (positioned)
    {
      priv->style_overlay_x = x;
      priv->style_overlay_y = y;
    }
}

gboolean
clay_text_layer_get_style_overlay_position (ClayTextLayer *layer,
                                            gdouble       *x,
                                            gdouble       *y)
{
  ClayTextLayerPrivate *priv;

  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (layer), FALSE);

  priv = layer->private;

  if (! priv->style_overlay_positioned)
    return FALSE;

  *x = priv->style_overlay_x;
  *y = priv->style_overlay_y;

  return TRUE;
}

gboolean
clay_text_layer_is_style_overlay_positioned (ClayTextLayer *layer)
{
  ClayTextLayerPrivate *priv;

  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (layer), FALSE);

  priv = layer->private;

  return priv->style_overlay_positioned;
}

void
clay_text_layer_set_style_overlay_offset (ClayTextLayer *layer,
                                          gdouble        offset_x,
                                          gdouble        offset_y)
{
  ClayTextLayerPrivate *priv;

  g_return_if_fail (CLAY_IS_TEXT_LAYER (layer));

  priv = layer->private;

  priv->style_overlay_offset_x = offset_x;
  priv->style_overlay_offset_y = offset_y;
}

void
clay_text_layer_get_style_overlay_offset (ClayTextLayer *layer,
                                          gdouble       *offset_x,
                                          gdouble       *offset_y)
{
  ClayTextLayerPrivate *priv;

  g_return_if_fail (CLAY_IS_TEXT_LAYER (layer));

  priv = layer->private;

  *offset_x = priv->style_overlay_offset_x;
  *offset_y = priv->style_overlay_offset_y;
}

ClayTextLayout *
clay_text_layer_get_layout (ClayTextLayer *layer)
{
   ClayTextLayout *layout;
   ClayItem       *item;
   ClayImage      *image;
   ClayContainer  *container;
   gdouble         xres;
   gdouble         yres;
   GError         *error = NULL;

   if (! layer->text)
     return FALSE;

   item      = CLAY_ITEM (layer);
   image     = clay_item_get_image (item);
   container = clay_data_factory_get_container (image->clay->font_factory);

   clay_data_factory_data_wait (image->clay->font_factory);

   if (clay_container_is_empty (container))
     {
        clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR,
                              _("Due to lack of any fonts, "
                                "text functionality is not available."));
        return NULL;
     }

   clay_image_get_resolution (image, &xres, &yres);

   layout = clay_text_layout_new (layer->text, image, xres, yres, &error);

   if (error)
     {
        clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR, error->message);
        g_error_free (error);
        return NULL;
     }

   return layout;
}

/*  private functions  */

static const Babl *
clay_text_layer_get_format (ClayTextLayer *layer)
{
  if (layer->convert_format)
    return layer->convert_format;

  return clay_drawable_get_format (CLAY_DRAWABLE (layer));
}

static void
clay_text_layer_text_changed (ClayTextLayer *layer)
{
  /*  If the text layer was created from a parasite, it's time to
   *  remove that parasite now.
   */
  if (layer->text_parasite)
    {
      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is loaded from XCF.
       */
      clay_item_parasite_detach (CLAY_ITEM (layer), layer->text_parasite,
                                 FALSE);
      layer->text_parasite        = NULL;
      layer->text_parasite_is_old = FALSE;
    }

  if (layer->text->box_mode == CLAY_TEXT_BOX_DYNAMIC)
    {
      gint                old_width;
      gint                new_width;
      ClayItem           *item         = CLAY_ITEM (layer);
      ClayTextDirection   old_base_dir = layer->private->base_dir;
      ClayTextDirection   new_base_dir = layer->text->base_dir;

      old_width = clay_item_get_width (item);
      clay_text_layer_render (layer);
      new_width = clay_item_get_width (item);

      if (old_base_dir != new_base_dir)
        {
          switch (old_base_dir)
            {
            case CLAY_TEXT_DIRECTION_LTR:
            case CLAY_TEXT_DIRECTION_RTL:
            case CLAY_TEXT_DIRECTION_TTB_LTR:
            case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
              switch (new_base_dir)
                {
                case CLAY_TEXT_DIRECTION_TTB_RTL:
                case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  clay_item_translate (item, -new_width, 0, FALSE);
                  break;

                case CLAY_TEXT_DIRECTION_LTR:
                case CLAY_TEXT_DIRECTION_RTL:
                case CLAY_TEXT_DIRECTION_TTB_LTR:
                case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  break;
                }
              break;

            case CLAY_TEXT_DIRECTION_TTB_RTL:
            case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
              switch (new_base_dir)
                {
                case CLAY_TEXT_DIRECTION_LTR:
                case CLAY_TEXT_DIRECTION_RTL:
                case CLAY_TEXT_DIRECTION_TTB_LTR:
                case CLAY_TEXT_DIRECTION_TTB_LTR_UPRIGHT:
                  clay_item_translate (item, old_width, 0, FALSE);
                  break;

                case CLAY_TEXT_DIRECTION_TTB_RTL:
                case CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT:
                  break;
                }
              break;
            }
        }
      else if ((new_base_dir == CLAY_TEXT_DIRECTION_TTB_RTL ||
              new_base_dir == CLAY_TEXT_DIRECTION_TTB_RTL_UPRIGHT))
        {
          if (old_width != new_width)
            clay_item_translate (item, old_width - new_width, 0, FALSE);
        }
    }
  else
    clay_text_layer_render (layer);

  layer->private->base_dir = layer->text->base_dir;
}

static gboolean
clay_text_layer_render (ClayTextLayer *layer)
{
  ClayDrawable   *drawable;
  ClayItem       *item;
  ClayImage      *image;
  ClayContainer  *container;
  ClayTextLayout *layout;
  gchar          *name = NULL;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;
  GError         *error = NULL;

  if (! layer->text)
    return FALSE;

  drawable  = CLAY_DRAWABLE (layer);
  item      = CLAY_ITEM (layer);
  image     = clay_item_get_image (item);
  container = clay_data_factory_get_container (image->clay->font_factory);

  clay_data_factory_data_wait (image->clay->font_factory);

  if (clay_container_is_empty (container))
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR,
                            _("Due to lack of any fonts, "
                              "text functionality is not available."));
      return FALSE;
    }

  clay_image_get_resolution (image, &xres, &yres);

  layout = clay_text_layout_new (layer->text, image, xres, yres, &error);
  if (error)
    {
      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_error_free (error);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  if (clay_text_layout_get_size (layout, &width, &height) &&
      (width  != clay_item_get_width  (item) ||
       height != clay_item_get_height (item) ||
       clay_text_layer_get_format (layer) !=
       clay_drawable_get_format (drawable)))
    {
      GeglBuffer *new_buffer;

      new_buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                         clay_text_layer_get_format (layer),
                                         drawable);
      clay_drawable_set_buffer (drawable, FALSE, NULL, new_buffer);
      g_object_unref (new_buffer);

      if (clay_layer_get_mask (CLAY_LAYER (layer)))
        {
          ClayLayerMask *mask = clay_layer_get_mask (CLAY_LAYER (layer));

          static ClayContext *unused_eek = NULL;

          if (! unused_eek)
            unused_eek = clay_context_new (image->clay, "eek", NULL);

          clay_item_resize (CLAY_ITEM (mask),
                            unused_eek, CLAY_FILL_TRANSPARENT,
                            width, height, 0, 0);
        }
    }

  if (layer->text->text)
    {
      name = clay_utf8_strtrim (layer->text->text, 30);
    }
  else if (layer->text->markup)
    {
      gchar *tmp = clay_markup_extract_text (layer->text->markup);
      name = clay_utf8_strtrim (tmp, 30);
      g_free (tmp);
    }

  clay_rasterizable_auto_rename (CLAY_RASTERIZABLE (layer), NULL, name);
  g_free (name);

  if (width > 0 && height > 0)
    clay_text_layer_render_layout (layer, layout);

  g_object_unref (layout);

  g_object_thaw_notify (G_OBJECT (drawable));

  return (width > 0 && height > 0);
}

static void
clay_text_layer_set_dash_info (cairo_t      *cr,
                               gdouble       width,
                               gdouble       dash_offset,
                               const GArray *dash_info)
{
  if (dash_info && dash_info->len >= 2)
    {
      gint     n_dashes = dash_info->len;
      gdouble *dashes   = g_new (gdouble, dash_info->len);
      gint     i;

      dash_offset = dash_offset * MAX (width, 1.0);

      for (i = 0; i < n_dashes; i++)
        dashes[i] = MAX (width, 1.0) * g_array_index (dash_info, gdouble, i);

      /* correct 0.0 in the first element (starts with a gap) */

      if (dashes[0] == 0.0)
        {
          gdouble first;

          first = dashes[1];

          /* shift the pattern to really starts with a dash and
           * use the offset to skip into it.
           */
          for (i = 0; i < n_dashes - 2; i++)
            {
              dashes[i] = dashes[i + 2];
              dash_offset += dashes[i];
            }

          if (n_dashes % 2 == 1)
            {
              dashes[n_dashes - 2] = first;
              n_dashes--;
            }
          else if (dash_info->len > 2)
            {
              dashes[n_dashes - 3] += first;
              n_dashes -= 2;
            }
        }

      /* correct odd number of dash specifiers */

      if (n_dashes % 2 == 1)
        {
          gdouble last = dashes[n_dashes - 1];

          dashes[0] += last;
          dash_offset += last;
          n_dashes--;
        }

      if (n_dashes >= 2)
        cairo_set_dash (cr,
                        dashes,
                        n_dashes,
                        dash_offset);

      g_free (dashes);
    }
}

static cairo_surface_t *
clay_temp_buf_create_cairo_surface (ClayTempBuf *temp_buf)
{
  cairo_surface_t *surface;
  gboolean         has_alpha;
  const Babl      *format;
  const Babl      *fish = NULL;
  const guchar    *data;
  gint             width;
  gint             height;
  gint             bpp;
  guchar          *pixels;
  gint             rowstride;
  gint             i;

  g_return_val_if_fail (temp_buf != NULL, NULL);

  data      = clay_temp_buf_get_data (temp_buf);
  format    = clay_temp_buf_get_format (temp_buf);
  width     = clay_temp_buf_get_width (temp_buf);
  height    = clay_temp_buf_get_height (temp_buf);
  bpp       = babl_format_get_bytes_per_pixel (format);
  has_alpha = babl_format_has_alpha (format);

  surface = cairo_image_surface_create (has_alpha ?
                                        CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24,
                                        width, height);

  pixels    = cairo_image_surface_get_data (surface);
  rowstride = cairo_image_surface_get_stride (surface);

  if (format != babl_format (has_alpha ? "cairo-ARGB32" : "cairo-RGB24"))
    fish = babl_fish (format, babl_format (has_alpha ? "cairo-ARGB32" : "cairo-RGB24"));

  for (i = 0; i < height; i++)
    {
      if (fish)
        babl_process (fish, data, pixels, width);
      else
        memcpy (pixels, data, width * bpp);

      data += width * bpp;
      pixels += rowstride;
    }

  return surface;
}

static void
clay_text_layer_render_layout (ClayTextLayer  *layer,
                               ClayTextLayout *layout)
{
  ClayDrawable       *drawable = CLAY_DRAWABLE (layer);
  ClayItem           *item     = CLAY_ITEM (layer);
  const Babl         *format;
  GeglBuffer         *buffer;
  cairo_t            *cr;
  cairo_surface_t    *surface;
  gint                width;
  gint                height;
  cairo_status_t      status;

  g_return_if_fail (clay_drawable_has_alpha (drawable));

  width  = clay_item_get_width  (item);
  height = clay_item_get_height (item);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 2)
  surface = cairo_image_surface_create (CAIRO_FORMAT_RGBA128F, width, height);
#else
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
#endif
  status = cairo_surface_status (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      ClayImage *image = clay_item_get_image (item);

      clay_message_literal (image->clay, NULL, CLAY_MESSAGE_ERROR,
                            _("Your text cannot be rendered. It is likely too big. "
                              "Please make it shorter or use a smaller font."));
      cairo_surface_destroy (surface);
      return;
    }

  cr = cairo_create (surface);
  if (layer->text->outline != CLAY_TEXT_OUTLINE_STROKE_ONLY)
    {
      cairo_save (cr);

      clay_text_layout_render (layout, cr, layer->text->base_dir, FALSE);

      cairo_restore (cr);
    }

  if (layer->text->outline != CLAY_TEXT_OUTLINE_NONE)
    {
      ClayText *text = layer->text;

      cairo_save (cr);

      cairo_set_antialias (cr, text->outline_antialias ?
                           CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE);
      cairo_set_line_cap (cr,
                          text->outline_cap_style == CLAY_CAP_BUTT ? CAIRO_LINE_CAP_BUTT :
                          text->outline_cap_style == CLAY_CAP_ROUND ? CAIRO_LINE_CAP_ROUND :
                          CAIRO_LINE_CAP_SQUARE);
      cairo_set_line_join (cr, text->outline_join_style == CLAY_JOIN_MITER ? CAIRO_LINE_JOIN_MITER :
                               text->outline_join_style == CLAY_JOIN_ROUND ? CAIRO_LINE_JOIN_ROUND :
                               CAIRO_LINE_JOIN_BEVEL);
      cairo_set_miter_limit (cr, text->outline_miter_limit);

      if (text->outline_dash_info)
        clay_text_layer_set_dash_info (cr, text->outline_width, text->outline_dash_offset, text->outline_dash_info);

      if (text->outline_style == CLAY_CUSTOM_STYLE_PATTERN && text->outline_pattern)
        {
          ClayTempBuf     *tempbuf = clay_pattern_get_mask (text->outline_pattern);
          cairo_surface_t *surface = clay_temp_buf_create_cairo_surface (tempbuf);

          cairo_set_source_surface (cr, surface, 0.0, 0.0);
          cairo_surface_destroy (surface);

          cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
        }
      else
        {
          GeglColor  *col = text->outline_foreground;
          gdouble     color[3];

          format = clay_text_layout_get_format (layout, "double");
          gegl_color_get_pixel (col, format, color);
          /* Text layout can be either grayscale or RGB without alpha. */
          if (! babl_space_is_gray (babl_format_get_space (format)))
            cairo_set_source_rgba (cr, color[0], color[1], color[2], 1.0);
          else
            cairo_set_source_rgba (cr, color[0], color[0], color[0], 1.0);
        }

      cairo_set_line_width (cr, text->outline_width * 2);

      clay_text_layout_render (layout, cr, text->base_dir, TRUE);

      if (text->outline_direction == CLAY_TEXT_OUTLINE_DIRECTION_INNER)
        cairo_clip_preserve (cr);

      cairo_stroke_preserve (cr);

      /* Clears inner outline if outline direction is outward */
      if (text->outline_direction == CLAY_TEXT_OUTLINE_DIRECTION_OUTER &&
          text->outline == CLAY_TEXT_OUTLINE_STROKE_ONLY)
        {
          cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
          cairo_fill_preserve (cr);
        }

      cairo_restore (cr);

      if (text->outline_direction == CLAY_TEXT_OUTLINE_DIRECTION_OUTER &&
          text->outline != CLAY_TEXT_OUTLINE_STROKE_ONLY)
        {
          cairo_save (cr);

          clay_text_layout_render (layout, cr, layer->text->base_dir, FALSE);

          cairo_restore (cr);
        }
    }

  cairo_destroy (cr);

  cairo_surface_flush (surface);

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 2)
  /* The CAIRO_FORMAT_RGBA128F surface maps to the layout TRC and space. */
  switch (clay_text_layout_get_trc (layout))
    {
    case CLAY_TRC_LINEAR:
      format = babl_format_with_space ("RaGaBaA float", clay_text_layout_get_space (layout));
      break;
    case CLAY_TRC_NON_LINEAR:
      format = babl_format_with_space ("R'aG'aB'aA float", clay_text_layout_get_space (layout));
      break;
    case CLAY_TRC_PERCEPTUAL:
      format = babl_format_with_space ("R~aG~aB~aA float", clay_text_layout_get_space (layout));
      break;
    default:
      g_return_if_reached ();
    }
#else
  format = babl_format_with_space ("cairo-ARGB32", clay_text_layout_get_space (layout));
#endif
  buffer = clay_cairo_surface_get_buffer (surface, format, FALSE);

  clay_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                         clay_drawable_get_buffer (drawable), NULL);

  g_object_unref (buffer);
  cairo_surface_destroy (surface);

  clay_drawable_update (drawable, 0, 0, width, height);
}
