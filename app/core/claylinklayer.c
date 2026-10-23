/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayLinkLayer
 * Copyright (C) 2019 Jehan
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

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "claycontext.h"
#include "clayimage.h"
#include "clayimage-color-profile.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylink.h"
#include "claylinklayer.h"
#include "clayobjectqueue.h"
#include "clayprogress.h"
#include "clayrasterizable.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "clay-transform-utils.h"

#include "clay-intl.h"

#define EPSILON 1e-10


enum
{
  LINK_LAYER_XCF_NONE              = 0,
  LINK_LAYER_XCF_DONT_AUTO_RENAME  = 1 << 0,
  LINK_LAYER_XCF_MODIFIED          = 1 << 1
};

enum
{
  PROP_0,
  PROP_LINK,
  PROP_SCALED_ONLY,
  N_PROPS
};

struct _ClayLinkLayerPrivate
{
  ClayLink              *link;
  gboolean               scaled_only;

  ClayMatrix3            matrix;
  gint                   offset_x;
  gint                   offset_y;
  ClayInterpolationType  interpolation;

  /* A transient value only useful to know when to drop monitoring after
   * a buffer update.
   */
  gboolean               keep_monitoring;
};

static void       clay_link_layer_rasterizable_iface_init
                                                 (ClayRasterizableInterface *iface);
static void       clay_link_layer_savable_iface_init
                                                 (ClaySavableInterface      *iface);

static void       clay_link_layer_finalize       (GObject           *object);
static void       clay_link_layer_get_property   (GObject           *object,
                                                  guint              property_id,
                                                  GValue            *value,
                                                  GParamSpec        *pspec);
static void       clay_link_layer_set_property   (GObject           *object,
                                                  guint              property_id,
                                                  const GValue      *value,
                                                  GParamSpec        *pspec);

static gint64     clay_link_layer_get_memsize    (ClayObject        *object,
                                                  gint64            *gui_size);

static ClayItem * clay_link_layer_duplicate      (ClayItem          *item,
                                                  GType              new_type);

static void       clay_link_layer_translate      (ClayItem              *item,
                                                  gdouble                offset_x,
                                                  gdouble                offset_y,
                                                  gboolean               push_undo);
static void       clay_link_layer_flip           (ClayItem              *item,
                                                  ClayContext           *context,
                                                  ClayOrientationType    flip_type,
                                                  gdouble                axis,
                                                  gboolean               clip_result);
static void       clay_link_layer_scale          (ClayItem              *item,
                                                  gint                   new_width,
                                                  gint                   new_height,
                                                  gint                   new_offset_x,
                                                  gint                   new_offset_y,
                                                  ClayInterpolationType  interpolation_type,
                                                  ClayProgress          *progress);
static void       clay_link_layer_transform      (ClayItem               *item,
                                                  ClayContext            *context,
                                                  const ClayMatrix3      *matrix,
                                                  ClayTransformDirection  direction,
                                                  ClayInterpolationType   interpolation_type,
                                                  ClayTransformResize     clip_result,
                                                  ClayProgress           *progress,
                                                  gboolean                push_undo);

static void       clay_link_layer_set_buffer     (ClayDrawable      *drawable,
                                                  gboolean           push_undo,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  const GeglRectangle *bounds);
static void       clay_link_layer_push_undo      (ClayDrawable      *drawable,
                                                  const gchar       *undo_desc,
                                                  GeglBuffer        *buffer,
                                                  gint               x,
                                                  gint               y,
                                                  gint               width,
                                                  gint               height);

static void       clay_link_layer_set_rasterized (ClayRasterizable  *rasterizable,
                                                  gboolean           rasterized);

static void       clay_link_layer_save           (ClaySavable       *savable,
                                                  ClaySaveState     *state);
static gboolean   clay_link_layer_load_enter     (ClayLoadState     *state,
                                                  const gchar      **attribute_names,
                                                  const gchar      **attribute_values,
                                                  gpointer           user_data,
                                                  GError           **error);
static gboolean   clay_link_layer_load_exit      (ClayLoadState     *state,
                                                  const gchar       *text,
                                                  gsize              len,
                                                  gpointer           user_data,
                                                  GError           **error);

static void       clay_link_layer_convert_type   (ClayLayer         *layer,
                                                  ClayImage         *dest_image,
                                                  const Babl        *new_format,
                                                  ClayColorProfile  *src_profile,
                                                  ClayColorProfile  *dest_profile,
                                                  GeglDitherMethod   layer_dither_type,
                                                  GeglDitherMethod   mask_dither_type,
                                                  gboolean           push_undo,
                                                  ClayProgress      *progress);

static void       clay_link_layer_render_full    (ClayLinkLayer     *layer);
static gboolean   clay_link_layer_render_link    (ClayLinkLayer     *layer);

static gboolean
               clay_link_layer_is_scaling_matrix (ClayLinkLayer     *layer,
                                                  const ClayMatrix3 *matrix,
                                                  gint              *new_width,
                                                  gint              *new_height,
                                                  gint              *new_offset_x,
                                                  gint              *new_offset_y);


G_DEFINE_TYPE_WITH_CODE (ClayLinkLayer, clay_link_layer, CLAY_TYPE_LAYER,
                         G_ADD_PRIVATE (ClayLinkLayer)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_RASTERIZABLE,
                                                clay_link_layer_rasterizable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_link_layer_savable_iface_init))

#define parent_class clay_link_layer_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;

static GParamSpec *link_layer_props[N_PROPS] = { NULL, };


static void
clay_link_layer_class_init (ClayLinkLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);
  ClayLayerClass    *layer_class       = CLAY_LAYER_CLASS (klass);

  object_class->finalize            = clay_link_layer_finalize;
  object_class->get_property        = clay_link_layer_get_property;
  object_class->set_property        = clay_link_layer_set_property;

  clay_object_class->get_memsize    = clay_link_layer_get_memsize;

  /* TODO: make a custom icon "clay-link-layer". */
  viewable_class->default_icon_name = "emblem-symbolic-link";
  viewable_class->default_name      = _("Link Layer");

  item_class->duplicate             = clay_link_layer_duplicate;
  item_class->rename                = clay_rasterizable_rename;
  item_class->translate             = clay_link_layer_translate;
  item_class->flip                  = clay_link_layer_flip;
  item_class->scale                 = clay_link_layer_scale;
  item_class->transform             = clay_link_layer_transform;

  item_class->rename_desc           = _("Rename Link Layer");
  item_class->translate_desc        = _("Move Link Layer");
  item_class->scale_desc            = _("Scale Link Layer");
  item_class->resize_desc           = _("Resize Link Layer");
  item_class->flip_desc             = _("Flip Link Layer");
  item_class->rotate_desc           = _("Rotate Link Layer");
  item_class->transform_desc        = _("Transform Link Layer");

  drawable_class->set_buffer        = clay_link_layer_set_buffer;
  drawable_class->push_undo         = clay_link_layer_push_undo;

  layer_class->convert_type         = clay_link_layer_convert_type;

  link_layer_props[PROP_LINK]        = g_param_spec_object ("link",
                                                            NULL, NULL,
                                                            CLAY_TYPE_LINK,
                                                            CLAY_PARAM_READWRITE |
                                                            CLAY_PARAM_STATIC_STRINGS);

  link_layer_props[PROP_SCALED_ONLY] = g_param_spec_boolean ("scaled-only",
                                                             NULL, NULL,
                                                             FALSE,
                                                             CLAY_PARAM_READWRITE |
                                                             CLAY_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, link_layer_props);
}

static void
clay_link_layer_init (ClayLinkLayer *layer)
{
  layer->p         = clay_link_layer_get_instance_private (layer);
  layer->p->link   = NULL;
  clay_matrix3_identity (&layer->p->matrix);

  layer->p->scaled_only   = FALSE;
  layer->p->offset_x      = 0;
  layer->p->offset_y      = 0;
  layer->p->interpolation = CLAY_INTERPOLATION_NONE;

  layer->p->keep_monitoring = FALSE;
}

static void
clay_link_layer_rasterizable_iface_init (ClayRasterizableInterface *iface)
{
  iface->set_rasterized = clay_link_layer_set_rasterized;
}

static void
clay_link_layer_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "link-layer";
  iface->save       = clay_link_layer_save;
  iface->load_enter = clay_link_layer_load_enter;
  iface->load_exit  = clay_link_layer_load_exit;
}

static void
clay_link_layer_finalize (GObject *object)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (object);

  g_clear_object (&layer->p->link);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_link_layer_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (object);

  switch (property_id)
    {
    case PROP_LINK:
      g_value_set_object (value, layer->p->link);
      break;
    case PROP_SCALED_ONLY:
      g_value_set_boolean (value, layer->p->scaled_only);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_link_layer_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (object);

  switch (property_id)
    {
    case PROP_LINK:
      clay_link_layer_set_link (layer, g_value_get_object (value), FALSE);
      break;
    case PROP_SCALED_ONLY:
      layer->p->scaled_only = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_link_layer_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayLinkLayer *layer   = CLAY_LINK_LAYER (object);
  gint64         memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (layer->p->link),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static ClayItem *
clay_link_layer_duplicate (ClayItem *item,
                           GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_LINK_LAYER (new_item))
    {
      ClayLinkLayer *layer     = CLAY_LINK_LAYER (item);
      ClayLinkLayer *new_layer = CLAY_LINK_LAYER (new_item);
      ClayLink      *link      = NULL;
      GeglBuffer    *buffer;
      gint           width;
      gint           height;

      clay_config_sync (G_OBJECT (layer), G_OBJECT (new_layer), 0);

      if (layer->p->link)
        {
          link = clay_link_duplicate (layer->p->link);
          clay_link_layer_set_link (new_layer, link, FALSE);
        }

      buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
      width  = gegl_buffer_get_width (buffer);
      height = gegl_buffer_get_height (buffer);

      if (width  != clay_item_get_width  (CLAY_ITEM (new_layer)) ||
          height != clay_item_get_height (CLAY_ITEM (new_layer)))
        {
          GeglBuffer    *new_buffer;
          GeglRectangle  bounds;

          clay_item_get_offset (CLAY_ITEM (new_layer), &bounds.x, &bounds.y);
          bounds.width  = 0;
          bounds.height = 0;

          new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                        clay_drawable_get_format (CLAY_DRAWABLE (new_layer)));
          CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (CLAY_DRAWABLE (new_layer),
                                                          FALSE, NULL,
                                                          new_buffer, &bounds);
          g_object_unref (new_buffer);
        }

      clay_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                             clay_drawable_get_buffer (CLAY_DRAWABLE (new_layer)), NULL);

      new_layer->p->scaled_only   = layer->p->scaled_only;
      clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (new_layer),
                                         clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (layer)));
      new_layer->p->matrix        = layer->p->matrix;
      new_layer->p->offset_x      = layer->p->offset_x;
      new_layer->p->offset_y      = layer->p->offset_y;
      new_layer->p->interpolation = layer->p->interpolation;

      new_layer->p->keep_monitoring = FALSE;

      if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
        clay_rasterizable_set_undo_rasterized (CLAY_RASTERIZABLE (new_layer), TRUE);

      g_clear_object (&link);
    }

  return new_item;
}

static void
clay_link_layer_translate (ClayItem *item,
                           gdouble   offset_x,
                           gdouble   offset_y,
                           gboolean  push_undo)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (item);

  if (! clay_matrix3_is_identity (&layer->p->matrix))
    clay_matrix3_translate (&layer->p->matrix, offset_x, offset_y);

  CLAY_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y, push_undo);
}

static void
clay_link_layer_flip (ClayItem            *item,
                      ClayContext         *context,
                      ClayOrientationType  flip_type,
                      gdouble              axis,
                      gboolean             clip_result)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (item);

  if (clay_link_is_monitored (layer->p->link))
    {
      ClayMatrix3 matrix = layer->p->matrix;

      clay_transform_matrix_flip (&matrix, flip_type, axis);
      clay_link_layer_set_transform (layer, &matrix, layer->p->interpolation, TRUE);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis, clip_result);
    }
}

static void
clay_link_layer_scale (ClayItem              *item,
                       gint                   new_width,
                       gint                   new_height,
                       gint                   new_offset_x,
                       gint                   new_offset_y,
                       ClayInterpolationType  interpolation_type,
                       ClayProgress          *progress)
{
  ClayLinkLayer   *link_layer = CLAY_LINK_LAYER (item);
  ClayLayer       *layer      = CLAY_LAYER (item);
  ClayObjectQueue *queue      = NULL;

  if (progress && layer->mask)
    {
      ClayLayerMask *mask;

      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      /* temporarily set layer->mask to NULL, so that its size won't be counted
       * when pushing the layer to the queue.
       */
      mask        = layer->mask;
      layer->mask = NULL;

      clay_object_queue_push (queue, layer);
      clay_object_queue_push (queue, mask);

      layer->mask = mask;
    }

  if (queue)
    clay_object_queue_pop (queue);

  if (clay_link_is_vector (link_layer->p->link)    &&
      clay_link_is_monitored (link_layer->p->link) &&
      clay_matrix3_is_identity (&link_layer->p->matrix))
    {
      /* Non-modified vector images are always recomputed from the
       * source file and therefore are always sharp.
       */
      clay_link_set_size (link_layer->p->link, new_width, new_height, FALSE);
      clay_item_set_offset (item, new_offset_x, new_offset_y);
      clay_link_layer_render_link (link_layer);
    }
  else if (! clay_matrix3_is_identity (&link_layer->p->matrix))
    {
      ClayMatrix3 matrix = link_layer->p->matrix;
      gdouble     x_scale_factor;
      gdouble     y_scale_factor;
      gint        offset_x;
      gint        offset_y;

      x_scale_factor = (gdouble) new_width / clay_item_get_width (item);
      y_scale_factor = (gdouble) new_height / clay_item_get_height (item);

      clay_matrix3_scale (&matrix, x_scale_factor, y_scale_factor);

      clay_link_layer_set_transform (link_layer, &matrix, interpolation_type, TRUE);

      /* Unfortunately we can't know the proper translation offset to
       * set before we replay the full matrix (the offsets may have been
       * changed in previous transformations).
       * To avoid re-loading the source image twice though, I don't call
       * clay_link_layer_set_transform() again but directly edit the
       * matrix and set the offset.
       */
      clay_item_get_offset (item, &offset_x, &offset_y);
      clay_matrix3_translate (&link_layer->p->matrix,
                              new_offset_x - offset_x,
                              new_offset_y - offset_y);
      clay_item_set_offset (item, new_offset_x, new_offset_y);
      clay_drawable_update_all (CLAY_DRAWABLE (layer));
    }
  else if (clay_link_is_monitored (link_layer->p->link))
    {
      ClayMatrix3 matrix = link_layer->p->matrix;
      gdouble     x_scale_factor;
      gdouble     y_scale_factor;
      gint        offset_x;
      gint        offset_y;

      x_scale_factor = (gdouble) new_width / clay_item_get_width (item);
      y_scale_factor = (gdouble) new_height / clay_item_get_height (item);

      clay_matrix3_scale (&matrix, x_scale_factor, y_scale_factor);
      clay_item_get_offset (item, &offset_x, &offset_y);
      clay_matrix3_translate (&matrix,
                              new_offset_x - offset_x,
                              new_offset_y - offset_y);
      clay_link_layer_set_transform (link_layer, &matrix, interpolation_type, TRUE);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->scale (CLAY_ITEM (layer),
                                             new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
    }

  if (layer->mask)
    {
      if (queue)
        clay_object_queue_pop (queue);

      clay_item_scale (CLAY_ITEM (layer->mask),
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation_type, progress);
    }

  g_clear_object (&queue);
}

static void
clay_link_layer_transform (ClayItem               *item,
                           ClayContext            *context,
                           const ClayMatrix3      *matrix,
                           ClayTransformDirection  direction,
                           ClayInterpolationType   interpolation_type,
                           ClayTransformResize     clip_result,
                           ClayProgress           *progress,
                           gboolean                push_undo)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (item);
  gboolean       keep_monitoring;

  if (clay_matrix3_is_identity (matrix))
    return;

  if (push_undo)
    clay_image_undo_push_link_layer (clay_item_get_image (CLAY_ITEM (layer)),
                                     _("Transform Link Layer"), layer);

  if (clay_link_is_vector (layer->p->link)    &&
      clay_link_is_monitored (layer->p->link) &&
      clay_matrix3_is_identity (&layer->p->matrix))
    {
      gint new_width;
      gint new_height;
      gint new_offset_x;
      gint new_offset_y;

      if (clay_link_layer_is_scaling_matrix (layer, matrix,
                                             &new_width, &new_height,
                                             &new_offset_x, &new_offset_y))
        {
          /* Scaling when no other transformation has happened yet is
           * special-case for vector links, because we can do even
           * better by reloading from source.
           */
          clay_link_layer_scale (item, new_width, new_height,
                                 new_offset_x, new_offset_y,
                                 interpolation_type, progress);
          return;
        }
    }

  /* Clipping would produce different results when applied on a single
   * step of multiple transformations. So only "adjust" transformations
   * are stored non-destructively. Any clip/crop transformation triggers
   * a destructive transform.
   *
   * Note: we could also store non-destructively a single clipped
   * transformation, but that would be harder to document and explain.
   */
  keep_monitoring = (clay_link_is_monitored (layer->p->link) &&
                     clip_result == CLAY_TRANSFORM_RESIZE_ADJUST);

  if (keep_monitoring)
    {
      ClayMatrix3 left  = *matrix;
      ClayMatrix3 right = layer->p->matrix;

      if (direction == CLAY_TRANSFORM_BACKWARD)
        clay_matrix3_invert (&left);

      clay_matrix3_mult (&left, &right);
      clay_link_layer_set_transform (layer, &right, interpolation_type, TRUE);
    }
  else
    {
      /* Reset the transformation matrix. */
      clay_matrix3_identity (&layer->p->matrix);

      CLAY_ITEM_CLASS (parent_class)->transform (item, context,
                                                 matrix, direction,
                                                 interpolation_type,
                                                 clip_result, progress,
                                                 push_undo);
    }
}

static void
clay_link_layer_set_buffer (ClayDrawable        *drawable,
                            gboolean             push_undo,
                            const gchar         *undo_desc,
                            GeglBuffer          *buffer,
                            const GeglRectangle *bounds)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (drawable);
  ClayImage     *image = clay_item_get_image (CLAY_ITEM (layer));

  if (push_undo)
    {
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE_MOD, undo_desc);
      clay_image_undo_push_link_layer (image, NULL, layer);
    }

  CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  FALSE, undo_desc,
                                                  buffer,
                                                  bounds);

  if (push_undo)
    {
      if (layer->p->link                          &&
          clay_link_is_monitored (layer->p->link) &&
          ! layer->p->keep_monitoring)
        clay_link_freeze (layer->p->link);

      clay_image_undo_group_end (image);
    }
}

static void
clay_link_layer_push_undo (ClayDrawable *drawable,
                           const gchar  *undo_desc,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (drawable);
  ClayImage     *image = clay_item_get_image (CLAY_ITEM (layer));
  gboolean       monitored;

  monitored = clay_link_is_monitored (layer->p->link);

  if (monitored)
    {
      clay_image_undo_group_start (image, CLAY_UNDO_GROUP_DRAWABLE, undo_desc);

      clay_image_undo_push_link_layer (image, NULL, layer);
      clay_link_freeze (layer->p->link);
    }

  CLAY_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (monitored)
    {
      clay_image_undo_group_end (image);
    }
}

static void
clay_link_layer_set_rasterized (ClayRasterizable *rasterizable,
                                gboolean          rasterized)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (rasterizable);

  if (rasterized)
    {
      clay_link_freeze (layer->p->link);
    }
  else
    {
      clay_link_thaw (layer->p->link);
      clay_matrix3_identity (&layer->p->matrix);
      clay_link_layer_render_link (layer);
    }
}

static void
clay_link_layer_save (ClaySavable   *savable,
                      ClaySaveState *state)
{
  ClayLinkLayer *layer = CLAY_LINK_LAYER (savable);

  clay_savable_print_element_start (state, "link-layer",
                                    "name", "%s", clay_object_get_name (CLAY_OBJECT (layer)),
                                    NULL);

  parent_savable_iface->save (savable, state);

  clay_savable_save (CLAY_SAVABLE (layer->p->link), state);

  if (! clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (layer)))
    clay_savable_print_element (state, "auto-rename", NULL, NULL, "disabled", "%b", TRUE, NULL);

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (layer)))
    clay_savable_print_element (state, "rasterized", NULL, NULL, NULL);

  if (! clay_matrix3_is_identity (&layer->p->matrix))
    {
      clay_savable_matrix3_save (&layer->p->matrix, state);
      clay_savable_print_element (state, "transform-offsets", NULL, NULL,
                                  "x", "%d", layer->p->offset_x,
                                  "y", "%d", layer->p->offset_y,
                                  NULL);
      clay_savable_print_element (state, "interpolation",
                                  "%[ClayInterpolationType]", layer->p->interpolation,
                                  NULL);
    }

  clay_savable_print_element_end (state, "link-layer");
}

static gboolean
clay_link_layer_load_enter (ClayLoadState  *state,
                            const gchar   **attribute_names,
                            const gchar   **attribute_values,
                            gpointer        user_data,
                            GError        **error)
{
  ClayLinkLayer *layer;
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

  layer = CLAY_LINK_LAYER (clay_drawable_new (CLAY_TYPE_LINK_LAYER,
                                              state->image, NULL,
                                              0, 0, 1, 1,
                                              clay_image_get_layer_format (state->image,
                                                                           TRUE)));

  clay_object_set_name (CLAY_OBJECT (layer), name);

  clay_savable_load (CLAY_TYPE_LINK, state);
  clay_savable_load_add_simple_handler (state, "auto-rename", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        "disabled", G_TYPE_BOOLEAN,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "rasterized", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);

  clay_savable_load_add_handlers (state, "matrix",
                                  clay_savable_enter_matrix,
                                  clay_savable_exit_matrix,
                                  NULL, NULL);
  clay_savable_load_add_simple_handler (state, "interpolation", CLAY_TYPE_INTERPOLATION_TYPE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "transform-offsets", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "x", G_TYPE_INT,
                                        "y", G_TYPE_INT,
                                        NULL);
  clay_savable_load_push_active_object (state, G_OBJECT (layer));

  return TRUE;
}

static gboolean
clay_link_layer_load_exit (ClayLoadState  *state,
                           const gchar    *text,
                           gsize           len,
                           gpointer        user_data,
                           GError        **error)
{
  ClayLinkLayer         *layer;
  ClayLink              *link           = NULL;
  ClayMatrix3           *matrix         = NULL;
  ClayInterpolationType  interpolation  = CLAY_INTERPOLATION_NONE;
  gboolean               rasterized     = FALSE;
  gboolean               disable_rename = FALSE;
  gint                   offset_x       = 0;
  gint                   offset_y       = 0;

  g_return_val_if_fail (CLAY_IS_LINK_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  /* Order matters because parent's load_exit() will pop the layer. */
  layer = CLAY_LINK_LAYER (clay_savable_load_peek_active_object (state));
  parent_savable_iface->load_exit (state, text, len, user_data, error);

  if (! clay_savable_load_get_values (state, "link", &link, NULL))
    {
      CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                               "%s: missing or invalid <link/> element.",
                               G_STRFUNC);
      return TRUE;
    }

  clay_savable_load_get_values (state,
                                "rasterized",           &rasterized,
                                "auto-rename:disabled", &disable_rename,
                                "matrix3",              &matrix,
                                "interpolation",        &interpolation,
                                "transform-offsets:x",  &offset_x,
                                "transform-offsets:y",  &offset_y,
                                NULL);

  clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (layer), ! disable_rename);

  if (rasterized && clay_link_is_monitored (link))
    clay_link_freeze (link);
  else if (! rasterized && ! clay_link_is_monitored (link))
    clay_link_thaw (link);

  if (matrix)
    clay_item_set_offset (CLAY_ITEM (layer), offset_x, offset_y);
  clay_link_layer_set_link_with_matrix (layer, link, matrix, interpolation,
                                        offset_x, offset_y, FALSE);

  return TRUE;
}

static void
clay_link_layer_convert_type (ClayLayer         *layer,
                              ClayImage         *dest_image,
                              const Babl        *new_format,
                              ClayColorProfile  *src_profile,
                              ClayColorProfile  *dest_profile,
                              GeglDitherMethod   layer_dither_type,
                              GeglDitherMethod   mask_dither_type,
                              gboolean           push_undo,
                              ClayProgress      *progress)
{
  ClayLinkLayer *link_layer = CLAY_LINK_LAYER (layer);
  ClayImage     *image      = clay_item_get_image (CLAY_ITEM (link_layer));

  CLAY_LAYER_CLASS (parent_class)->convert_type (layer, dest_image,
                                                 new_format,
                                                 src_profile,
                                                 dest_profile,
                                                 layer_dither_type,
                                                 mask_dither_type,
                                                 push_undo,
                                                 progress);

  if (link_layer->p->link && clay_link_is_monitored (link_layer->p->link))
    {
      if (push_undo)
        clay_image_undo_push_link_layer (image, NULL, link_layer);

      clay_link_layer_render_link (link_layer);
    }
}


/*  public functions  */

/**
 * clay_link_layer_new:
 * @image: the #ClayImage the layer should belong to
 * @text: a #ClayText object
 *
 * Creates a new link layer.
 *
 * Return value: a new #ClayLinkLayer or %NULL in case of a problem
 **/
ClayLayer *
clay_link_layer_new (ClayImage *image,
                     ClayLink  *link)
{
  ClayLinkLayer *layer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  layer =
    CLAY_LINK_LAYER (clay_drawable_new (CLAY_TYPE_LINK_LAYER,
                                        image, NULL,
                                        0, 0, 1, 1,
                                        clay_image_get_layer_format (image,
                                                                     TRUE)));
  clay_layer_set_mode (CLAY_LAYER (layer),
                       clay_image_get_default_new_layer_mode (image),
                       FALSE);

  if (! clay_link_layer_set_link (layer, link, FALSE))
    g_clear_object (&layer);

  return CLAY_LAYER (layer);
}

ClayLink *
clay_link_layer_get_link (ClayLinkLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), NULL);

  return layer->p->link;
}

gboolean
clay_link_layer_set_link (ClayLinkLayer *layer,
                          ClayLink      *link,
                          gboolean       push_undo)
{
  return clay_link_layer_set_link_with_matrix (layer, link, NULL,
                                               CLAY_INTERPOLATION_NONE,
                                               0, 0, push_undo);
}

gboolean
clay_link_layer_set_link_with_matrix (ClayLinkLayer         *layer,
                                      ClayLink              *link,
                                      ClayMatrix3           *matrix,
                                      ClayInterpolationType  interpolation_type,
                                      gint                   offset_x,
                                      gint                   offset_y,
                                      gboolean               push_undo)
{
  gboolean rendered = FALSE;

  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), FALSE);
  g_return_val_if_fail (CLAY_IS_LINK (link), FALSE);

  if (clay_item_is_attached (CLAY_ITEM (layer)) && push_undo)
    clay_image_undo_push_link_layer (clay_item_get_image (CLAY_ITEM (layer)),
                                     _("Set layer link"), layer);

  if (layer->p->link)
    {
      g_signal_handlers_disconnect_by_func (layer->p->link,
                                            G_CALLBACK (clay_link_layer_render_full),
                                            layer);

    }

  g_set_object (&layer->p->link, link);

  if (link)
    {
      g_signal_connect_object (link, "changed",
                               G_CALLBACK (clay_link_layer_render_full),
                               layer, G_CONNECT_SWAPPED);

      if (clay_link_is_monitored (link))
        {
          if (matrix == NULL)
            {
              clay_matrix3_identity (&layer->p->matrix);
              rendered = clay_link_layer_render_link (layer);
            }
          else
            {
              if (! clay_matrix3_is_identity (matrix))
                {
                  layer->p->offset_x = offset_x;
                  layer->p->offset_y = offset_y;
                }
              rendered = clay_link_layer_set_transform (layer, matrix, interpolation_type, push_undo);
            }
        }
    }

  g_object_notify (G_OBJECT (layer), "link");
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (layer));

  return rendered;
}

gboolean
clay_link_layer_is_monitored (ClayLinkLayer *layer)
{
  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), FALSE);

  return (CLAY_LINK_LAYER (layer)->p->link &&
          clay_link_is_monitored (CLAY_LINK_LAYER (layer)->p->link));
}

gboolean
clay_link_layer_get_transform (ClayLinkLayer         *layer,
                               ClayMatrix3           *matrix,
                               gint                  *offset_x,
                               gint                  *offset_y,
                               ClayInterpolationType *interpolation)
{
  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), FALSE);

  if (matrix)
    *matrix = layer->p->matrix;
  if (offset_x)
    *offset_x = layer->p->offset_x;
  if (offset_y)
    *offset_y = layer->p->offset_y;
  if (interpolation)
    *interpolation = layer->p->interpolation;

  return ! clay_matrix3_is_identity (&layer->p->matrix);
}

gboolean
clay_link_layer_set_transform (ClayLinkLayer         *layer,
                               ClayMatrix3           *matrix,
                               ClayInterpolationType  interpolation_type,
                               gboolean               push_undo)
{
  Clay     *clay;
  gboolean  rendered;

  g_return_val_if_fail (CLAY_IS_LINK_LAYER (layer), FALSE);
  g_return_val_if_fail (clay_link_is_monitored (layer->p->link), FALSE);

  clay = (clay_item_get_image (CLAY_ITEM (layer)))->clay;

  if (clay_matrix3_is_identity (&layer->p->matrix))
    /* First transformation: store the initial offset. */
    clay_item_get_offset (CLAY_ITEM (layer), &layer->p->offset_x, &layer->p->offset_y);

  clay_item_set_offset (CLAY_ITEM (layer), layer->p->offset_x, layer->p->offset_y);

  rendered = clay_link_layer_render_link (layer);
  if (! clay_matrix3_is_identity (matrix))
    {
      layer->p->keep_monitoring = TRUE;
      CLAY_ITEM_CLASS (parent_class)->transform (CLAY_ITEM (layer),
                                                 clay_get_user_context (clay),
                                                 matrix,
                                                 CLAY_TRANSFORM_FORWARD,
                                                 interpolation_type,
                                                 CLAY_TRANSFORM_RESIZE_ADJUST,
                                                 NULL, push_undo);
      layer->p->keep_monitoring = FALSE;
    }

  layer->p->matrix = *matrix;

  /* Interpolations are used to obtain reasonable quality. Considering
   * that doing a single transform will always be better than several
   * transforms in a row, it's OK to just drop the interpolation
   * algorithm of previous transforms. We just store the last one.
   */
  layer->p->interpolation = interpolation_type;

  return rendered;
}

gboolean
clay_item_is_link_layer (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return (CLAY_IS_LINK_LAYER (item) &&
          clay_link_layer_is_monitored (CLAY_LINK_LAYER (item)));
}

void
clay_link_layer_set_xcf_flags (ClayLinkLayer *layer,
                               guint32        flags)
{
  g_return_if_fail (CLAY_IS_LINK_LAYER (layer));

  clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (layer),
                                     (flags & LINK_LAYER_XCF_DONT_AUTO_RENAME) == 0);

  if ((flags & LINK_LAYER_XCF_MODIFIED) != 0)
    {
      clay_link_freeze (layer->p->link);
      clay_rasterizable_set_undo_rasterized (CLAY_RASTERIZABLE (layer), TRUE);
    }
}

guint32
clay_link_layer_get_xcf_flags (ClayLinkLayer *link_layer)
{
  guint flags = 0;

  g_return_val_if_fail (CLAY_IS_LINK_LAYER (link_layer), 0);

  if (! clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (link_layer)))
    flags |= LINK_LAYER_XCF_DONT_AUTO_RENAME;

  if (! clay_link_is_monitored (link_layer->p->link))
    flags |= LINK_LAYER_XCF_MODIFIED;

  return flags;
}


/*  private functions  */

static void
clay_link_layer_render_full (ClayLinkLayer *layer)
{
  clay_link_layer_render_link (layer);
  if (! clay_matrix3_is_identity (&layer->p->matrix))
    {
      Clay *clay = (clay_item_get_image (CLAY_ITEM (layer)))->clay;

      clay_item_set_offset (CLAY_ITEM (layer), layer->p->offset_x, layer->p->offset_y);

      layer->p->keep_monitoring = TRUE;
      CLAY_ITEM_CLASS (parent_class)->transform (CLAY_ITEM (layer),
                                                 clay_get_user_context (clay),
                                                 &layer->p->matrix,
                                                 CLAY_TRANSFORM_FORWARD,
                                                 layer->p->interpolation,
                                                 CLAY_TRANSFORM_RESIZE_ADJUST,
                                                 NULL, FALSE);
      layer->p->keep_monitoring = FALSE;
    }
}

static gboolean
clay_link_layer_render_link (ClayLinkLayer *layer)
{
  ClayDrawable   *drawable;
  ClayItem       *item;
  ClayImage      *image;
  GeglBuffer     *buffer;
  gdouble         xres;
  gdouble         yres;
  gint            width;
  gint            height;

  if (! layer->p->link)
    return FALSE;

  drawable = CLAY_DRAWABLE (layer);
  item     = CLAY_ITEM (layer);
  image    = clay_item_get_image (item);

  clay_image_get_resolution (image, &xres, &yres);
  /* TODO: I could imagine a ClayBusyBox (to be made as ClayProgress) in
   * the later list showing layer update.
   */
  buffer = clay_link_get_buffer (layer->p->link);

  if (! buffer)
    return FALSE;

  width  = gegl_buffer_get_width (buffer);
  height = gegl_buffer_get_height (buffer);

  g_object_freeze_notify (G_OBJECT (drawable));

  if ((width  != clay_item_get_width  (item) ||
       height != clay_item_get_height (item)))
    {
      GeglBuffer    *new_buffer;
      GeglRectangle  bounds;

      clay_item_get_offset (CLAY_ITEM (drawable),
                            &bounds.x, &bounds.y);
      bounds.width  = 0;
      bounds.height = 0;

      new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                    clay_drawable_get_format (drawable));
      CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                      FALSE, NULL,
                                                      new_buffer, &bounds);
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

  clay_rasterizable_auto_rename (CLAY_RASTERIZABLE (layer),
                                 CLAY_OBJECT (layer->p->link),
                                 NULL);

  clay_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE,
                         clay_drawable_get_buffer (drawable), NULL);

  g_object_thaw_notify (G_OBJECT (drawable));

  clay_drawable_update (drawable, 0, 0, width, height);
  clay_image_flush (image);

  return (width > 0 && height > 0);
}

static gboolean
clay_link_layer_is_scaling_matrix (ClayLinkLayer     *layer,
                                   const ClayMatrix3 *matrix,
                                   gint              *new_width,
                                   gint              *new_height,
                                   gint              *new_offset_x,
                                   gint              *new_offset_y)
{
  gboolean is_scaling;

  /* Scaling 3x3 matrix on a 2D plane (with optional translation). */
  is_scaling = (matrix->coeff[0][0] > 0.0 && matrix->coeff[0][1] == 0.0 &&
                matrix->coeff[1][0] < EPSILON && matrix->coeff[1][1] > 0.0 &&
                matrix->coeff[2][0] < EPSILON && matrix->coeff[2][1] < EPSILON && matrix->coeff[2][2] == 1.0);

  if (is_scaling)
    {
      gint width;
      gint height;
      gint offset_x;
      gint offset_y;

      width  = clay_item_get_width (CLAY_ITEM (layer));
      height = clay_item_get_height (CLAY_ITEM (layer));
      clay_item_get_offset (CLAY_ITEM (layer), &offset_x, &offset_y);

      *new_width  = (gint) (width * matrix->coeff[0][0]);
      *new_height = (gint) (height * matrix->coeff[1][1]);

      *new_offset_x = (gint) (offset_x * matrix->coeff[0][0] + matrix->coeff[0][2]);
      *new_offset_y = (gint) (offset_y * matrix->coeff[1][1] + matrix->coeff[1][2]);
    }

  return is_scaling;
}
