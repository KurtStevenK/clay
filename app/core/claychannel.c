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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclaycolor/claycolor.h"

#include "core-types.h"

#include "paint/claypaintcore-stroke.h"
#include "paint/claypaintoptions.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-mask.h"
#include "gegl/clay-gegl-nodes.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "clay-utils.h"
#include "clayboundary.h"
#include "claychannel.h"
#include "claychannel-select.h"
#include "claycontainer.h"
#include "claycontext.h"
#include "claydrawable-fill.h"
#include "claydrawable-stroke.h"
#include "claydrawablefilter.h"
#include "clayerror.h"
#include "clayimage.h"
#include "clayimage-quick-mask.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayer.h"
#include "claylayermask.h"
#include "claypaintinfo.h"
#include "claypickable.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "claystrokeoptions.h"

#include "clay-intl.h"


#define RGBA_EPSILON 1e-6

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};


static void clay_channel_pickable_iface_init (ClayPickableInterface *iface);
static void clay_channel_savable_iface_init  (ClaySavableInterface  *iface);

static void       clay_channel_finalize      (GObject           *object);

static gint64     clay_channel_get_memsize   (ClayObject        *object,
                                              gint64            *gui_size);

static gchar  * clay_channel_get_description (ClayViewable      *viewable,
                                              gchar            **tooltip);

static GeglNode * clay_channel_get_node      (ClayFilter        *filter);

static gboolean   clay_channel_is_attached   (ClayItem          *item);
static ClayItemTree * clay_channel_get_tree  (ClayItem          *item);
static gboolean   clay_channel_bounds        (ClayItem          *item,
                                              gdouble           *x,
                                              gdouble           *y,
                                              gdouble           *width,
                                              gdouble           *height);
static ClayItem * clay_channel_duplicate     (ClayItem          *item,
                                              GType              new_type);
static void       clay_channel_convert       (ClayItem          *item,
                                              ClayImage         *dest_image,
                                              GType              old_type);
static void       clay_channel_translate     (ClayItem          *item,
                                              gdouble            off_x,
                                              gdouble            off_y,
                                              gboolean           push_undo);
static void       clay_channel_scale         (ClayItem          *item,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               new_offset_x,
                                              gint               new_offset_y,
                                              ClayInterpolationType interp_type,
                                              ClayProgress      *progress);
static void       clay_channel_resize        (ClayItem          *item,
                                              ClayContext       *context,
                                              ClayFillType       fill_type,
                                              gint               new_width,
                                              gint               new_height,
                                              gint               offset_x,
                                              gint               offset_y);
static ClayTransformResize
                  clay_channel_get_clip      (ClayItem          *item,
                                              ClayTransformResize clip_result);
static gboolean   clay_channel_fill          (ClayItem          *item,
                                              ClayDrawable      *drawable,
                                              ClayFillOptions   *fill_options,
                                              gboolean           push_undo,
                                              ClayProgress      *progress,
                                              GError           **error);
static gboolean   clay_channel_stroke        (ClayItem          *item,
                                              ClayDrawable      *drawable,
                                              ClayStrokeOptions *stroke_options,
                                              gboolean           push_undo,
                                              ClayProgress      *progress,
                                              GError           **error);
static void       clay_channel_to_selection  (ClayItem          *item,
                                              ClayChannelOps     op,
                                              gboolean           antialias,
                                              gboolean           feather,
                                              gdouble            feather_radius_x,
                                              gdouble            feather_radius_y);

static void       clay_channel_convert_type  (ClayDrawable      *drawable,
                                              ClayImage         *dest_image,
                                              const Babl        *new_format,
                                              ClayColorProfile  *src_profile,
                                              ClayColorProfile  *dest_profile,
                                              GeglDitherMethod   layer_dither_type,
                                              GeglDitherMethod   mask_dither_type,
                                              gboolean           push_undo,
                                              ClayProgress      *progress);
static void clay_channel_invalidate_boundary   (ClayDrawable       *drawable);

static void      clay_channel_set_buffer     (ClayDrawable        *drawable,
                                              gboolean             push_undo,
                                              const gchar         *undo_desc,
                                              GeglBuffer          *buffer,
                                              const GeglRectangle *bounds);

static gdouble   clay_channel_get_opacity_at (ClayPickable        *pickable,
                                              gint                 x,
                                              gint                 y);


static void       clay_channel_save          (ClaySavable         *savable,
                                              ClaySaveState       *state);
static gboolean   clay_channel_load_enter    (ClayLoadState       *state,
                                              const gchar        **attribute_names,
                                              const gchar        **attribute_values,
                                              gpointer             user_data,
                                              GError             **error);
static gboolean   clay_channel_load_exit     (ClayLoadState       *state,
                                              const gchar         *text,
                                              gsize                len,
                                              gpointer             user_data,
                                              GError             **error);


static gboolean   clay_channel_real_boundary (ClayChannel         *channel,
                                              const ClayBoundSeg **segs_in,
                                              const ClayBoundSeg **segs_out,
                                              gint                *num_segs_in,
                                              gint                *num_segs_out,
                                              gint                 x1,
                                              gint                 y1,
                                              gint                 x2,
                                              gint                 y2);
static gboolean   clay_channel_real_is_empty (ClayChannel         *channel);
static gboolean   clay_channel_real_is_full  (ClayChannel         *channel);

static void       clay_channel_real_feather  (ClayChannel         *channel,
                                              gdouble              radius_x,
                                              gdouble              radius_y,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       clay_channel_real_sharpen  (ClayChannel         *channel,
                                              gboolean             push_undo);
static void       clay_channel_real_clear    (ClayChannel         *channel,
                                              const gchar         *undo_desc,
                                              gboolean             push_undo);
static void       clay_channel_real_all      (ClayChannel         *channel,
                                              gboolean             push_undo);
static void       clay_channel_real_invert   (ClayChannel         *channel,
                                              gboolean             push_undo);
static void       clay_channel_real_border   (ClayChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              ClayChannelBorderStyle style,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       clay_channel_real_grow     (ClayChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             push_undo);
static void       clay_channel_real_shrink   (ClayChannel         *channel,
                                              gint                 radius_x,
                                              gint                 radius_y,
                                              gboolean             edge_lock,
                                              gboolean             push_undo);
static void       clay_channel_real_flood    (ClayChannel         *channel,
                                              gboolean             push_undo);


static void      clay_channel_buffer_changed (GeglBuffer          *buffer,
                                              const GeglRectangle *rect,
                                              ClayChannel         *channel);


G_DEFINE_TYPE_WITH_CODE (ClayChannel, clay_channel, CLAY_TYPE_DRAWABLE,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_channel_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_channel_savable_iface_init))

#define parent_class clay_channel_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;

static guint channel_signals[LAST_SIGNAL] = { 0 };


static void
clay_channel_class_init (ClayChannelClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayFilterClass   *filter_class      = CLAY_FILTER_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);

  channel_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayChannelClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize            = clay_channel_finalize;

  clay_object_class->get_memsize    = clay_channel_get_memsize;

  viewable_class->default_icon_name = "clay-channel";
  viewable_class->default_name      = _("Channel");
  viewable_class->get_description   = clay_channel_get_description;

  filter_class->get_node            = clay_channel_get_node;

  item_class->is_attached          = clay_channel_is_attached;
  item_class->get_tree             = clay_channel_get_tree;
  item_class->bounds               = clay_channel_bounds;
  item_class->duplicate            = clay_channel_duplicate;
  item_class->convert              = clay_channel_convert;
  item_class->translate            = clay_channel_translate;
  item_class->scale                = clay_channel_scale;
  item_class->resize               = clay_channel_resize;
  item_class->get_clip             = clay_channel_get_clip;
  item_class->fill                 = clay_channel_fill;
  item_class->stroke               = clay_channel_stroke;
  item_class->to_selection         = clay_channel_to_selection;
  item_class->rename_desc          = C_("undo-type", "Rename Channel");
  item_class->translate_desc       = C_("undo-type", "Move Channel");
  item_class->scale_desc           = C_("undo-type", "Scale Channel");
  item_class->resize_desc          = C_("undo-type", "Resize Channel");
  item_class->flip_desc            = C_("undo-type", "Flip Channel");
  item_class->rotate_desc          = C_("undo-type", "Rotate Channel");
  item_class->transform_desc       = C_("undo-type", "Transform Channel");
  item_class->fill_desc            = C_("undo-type", "Fill Channel");
  item_class->stroke_desc          = C_("undo-type", "Stroke Channel");
  item_class->to_selection_desc    = C_("undo-type", "Channel to Selection");
  item_class->reorder_desc         = C_("undo-type", "Reorder Channel");
  item_class->raise_desc           = C_("undo-type", "Raise Channel");
  item_class->raise_to_top_desc    = C_("undo-type", "Raise Channel to Top");
  item_class->lower_desc           = C_("undo-type", "Lower Channel");
  item_class->lower_to_bottom_desc = C_("undo-type", "Lower Channel to Bottom");
  item_class->raise_failed         = _("Channel cannot be raised higher.");
  item_class->lower_failed         = _("Channel cannot be lowered more.");

  drawable_class->convert_type          = clay_channel_convert_type;
  drawable_class->invalidate_boundary   = clay_channel_invalidate_boundary;
  drawable_class->set_buffer            = clay_channel_set_buffer;

  klass->boundary       = clay_channel_real_boundary;
  klass->is_empty       = clay_channel_real_is_empty;
  klass->is_full        = clay_channel_real_is_full;
  klass->feather        = clay_channel_real_feather;
  klass->sharpen        = clay_channel_real_sharpen;
  klass->clear          = clay_channel_real_clear;
  klass->all            = clay_channel_real_all;
  klass->invert         = clay_channel_real_invert;
  klass->border         = clay_channel_real_border;
  klass->grow           = clay_channel_real_grow;
  klass->shrink         = clay_channel_real_shrink;
  klass->flood          = clay_channel_real_flood;

  klass->feather_desc   = C_("undo-type", "Feather Channel");
  klass->sharpen_desc   = C_("undo-type", "Sharpen Channel");
  klass->clear_desc     = C_("undo-type", "Clear Channel");
  klass->all_desc       = C_("undo-type", "Fill Channel");
  klass->invert_desc    = C_("undo-type", "Invert Channel");
  klass->border_desc    = C_("undo-type", "Border Channel");
  klass->grow_desc      = C_("undo-type", "Grow Channel");
  klass->shrink_desc    = C_("undo-type", "Shrink Channel");
  klass->flood_desc     = C_("undo-type", "Flood Channel");
}

static void
clay_channel_init (ClayChannel *channel)
{
  channel->color = gegl_color_new ("black");

  channel->show_masked    = FALSE;

  /*  Selection mask variables  */
  channel->boundary_known = FALSE;
  channel->segs_in        = NULL;
  channel->segs_out       = NULL;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->empty          = FALSE;
  channel->full           = FALSE;
  channel->bounds_known   = FALSE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = 0;
  channel->y2             = 0;
}

static void
clay_channel_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->get_opacity_at = clay_channel_get_opacity_at;
}

static void
clay_channel_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "channel";
  iface->save       = clay_channel_save;
  iface->load_enter = clay_channel_load_enter;
  iface->load_exit  = clay_channel_load_exit;
}

static void
clay_channel_finalize (GObject *object)
{
  ClayChannel *channel = CLAY_CHANNEL (object);

  g_clear_pointer (&channel->segs_in,  g_free);
  g_clear_pointer (&channel->segs_out, g_free);
  g_clear_object (&channel->color);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_channel_get_memsize (ClayObject *object,
                          gint64     *gui_size)
{
  ClayChannel *channel = CLAY_CHANNEL (object);

  *gui_size += channel->num_segs_in  * sizeof (ClayBoundSeg);
  *gui_size += channel->num_segs_out * sizeof (ClayBoundSeg);

  return CLAY_OBJECT_CLASS (parent_class)->get_memsize (object, gui_size);
}

static gchar *
clay_channel_get_description (ClayViewable  *viewable,
                              gchar        **tooltip)
{
  if (! strcmp (CLAY_IMAGE_QUICK_MASK_NAME,
                clay_object_get_name (viewable)))
    {
      return g_strdup (_("Quick Mask"));
    }

  return CLAY_VIEWABLE_CLASS (parent_class)->get_description (viewable,
                                                              tooltip);
}

static GeglNode *
clay_channel_get_node (ClayFilter *filter)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (filter);
  ClayChannel  *channel  = CLAY_CHANNEL (filter);
  GeglNode     *node;
  GeglNode     *source;
  GeglNode     *mode_node;
  const Babl   *color_format;

  node = CLAY_FILTER_CLASS (parent_class)->get_node (filter);

  source = clay_drawable_get_source_node (drawable);
  gegl_node_add_child (node, source);

  g_warn_if_fail (channel->color_node == NULL);

  color_format =
    clay_babl_format (CLAY_RGB,
                      clay_babl_precision (CLAY_COMPONENT_TYPE_FLOAT,
                                           clay_drawable_get_trc (drawable)),
                      TRUE, NULL);

  channel->color_node = gegl_node_new_child (node,
                                             "operation", "gegl:color",
                                             "format",    color_format,
                                             NULL);
  clay_gegl_node_set_color (channel->color_node, channel->color);

  g_warn_if_fail (channel->mask_node == NULL);

  channel->mask_node = gegl_node_new_child (node,
                                            "operation", "gegl:opacity",
                                            NULL);
  gegl_node_link (channel->color_node, channel->mask_node);

  g_warn_if_fail (channel->invert_node == NULL);

  channel->invert_node = gegl_node_new_child (node,
                                              "operation", "gegl:invert-linear",
                                              NULL);

  if (channel->show_masked)
    {
      gegl_node_link (source, channel->invert_node);
      gegl_node_connect (channel->invert_node, "output",
                         channel->mask_node,   "aux");
    }
  else
    {
      gegl_node_connect (source,             "output",
                         channel->mask_node, "aux");
    }

  mode_node = clay_drawable_get_mode_node (drawable);

  gegl_node_connect (channel->mask_node, "output",
                     mode_node,          "aux");

  return node;
}

static gboolean
clay_channel_is_attached (ClayItem *item)
{
  ClayImage *image = clay_item_get_image (item);

  return (CLAY_IS_IMAGE (image) &&
          clay_container_have (clay_image_get_channels (image),
                               CLAY_OBJECT (item)));
}

static ClayItemTree *
clay_channel_get_tree (ClayItem *item)
{
  if (clay_item_is_attached (item))
    {
      ClayImage *image = clay_item_get_image (item);

      return clay_image_get_channel_tree (image);
    }

  return NULL;
}

static gboolean
clay_channel_bounds (ClayItem *item,
                     gdouble  *x,
                     gdouble  *y,
                     gdouble  *width,
                     gdouble  *height)
{
  ClayChannel *channel = CLAY_CHANNEL (item);

  if (! channel->bounds_known)
    {
      GeglBuffer *buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

      channel->empty = ! clay_gegl_mask_bounds (buffer,
                                                &channel->x1,
                                                &channel->y1,
                                                &channel->x2,
                                                &channel->y2);
      channel->full         = FALSE;
      channel->bounds_known = TRUE;
    }

  *x      = channel->x1;
  *y      = channel->y1;
  *width  = channel->x2 - channel->x1;
  *height = channel->y2 - channel->y1;

  return ! channel->empty;
}

static ClayItem *
clay_channel_duplicate (ClayItem *item,
                        GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_CHANNEL (new_item))
    {
      ClayChannel *channel     = CLAY_CHANNEL (item);
      ClayChannel *new_channel = CLAY_CHANNEL (new_item);

      g_clear_object (&new_channel->color);
      new_channel->color        = gegl_color_duplicate (channel->color);
      new_channel->show_masked  = channel->show_masked;

      /*  selection mask variables  */
      new_channel->bounds_known = channel->bounds_known;
      new_channel->empty        = channel->empty;
      new_channel->full         = channel->full;
      new_channel->x1           = channel->x1;
      new_channel->y1           = channel->y1;
      new_channel->x2           = channel->x2;
      new_channel->y2           = channel->y2;

      if (new_type == CLAY_TYPE_CHANNEL)
        {
          /*  8-bit channel hack: make sure pixels between all sorts
           *  of channels of an image is always copied without any
           *  gamma conversion
           */
          ClayDrawable *new_drawable = CLAY_DRAWABLE (new_item);
          ClayImage    *image        = clay_item_get_image (item);
          const Babl   *format       = clay_image_get_channel_format (image);

          if (format != clay_drawable_get_format (new_drawable))
            {
              GeglBuffer *new_buffer;

              new_buffer =
                clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                      clay_item_get_width  (new_item),
                                                      clay_item_get_height (new_item)),
                                      format, new_drawable);

              gegl_buffer_set_format (new_buffer,
                                      clay_drawable_get_format (new_drawable));
              clay_gegl_buffer_copy (clay_drawable_get_buffer (new_drawable),
                                     NULL, GEGL_ABYSS_NONE,
                                     new_buffer, NULL);
              gegl_buffer_set_format (new_buffer, NULL);

              clay_drawable_set_buffer (new_drawable, FALSE, NULL, new_buffer);
              g_object_unref (new_buffer);
            }
        }
    }

  return new_item;
}

static void
clay_channel_convert (ClayItem  *item,
                      ClayImage *dest_image,
                      GType      old_type)
{
  ClayChannel  *channel  = CLAY_CHANNEL (item);
  ClayDrawable *drawable = CLAY_DRAWABLE (item);

  if (! clay_drawable_is_gray (drawable))
    {
      clay_drawable_convert_type (drawable, dest_image,
                                  CLAY_GRAY,
                                  clay_image_get_precision (dest_image),
                                  clay_drawable_has_alpha (drawable),
                                  NULL, NULL,
                                  GEGL_DITHER_NONE, GEGL_DITHER_NONE,
                                  FALSE, NULL);
    }

  if (clay_drawable_has_alpha (drawable))
    {
      GeglBuffer *new_buffer;
      const Babl *format;
      GeglColor  *background = gegl_color_new ("transparent");

      format = clay_drawable_get_format_without_alpha (drawable);

      new_buffer =
        clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                              clay_item_get_width (item),
                                              clay_item_get_height (item)),
                              format, drawable);

      clay_gegl_apply_flatten (clay_drawable_get_buffer (drawable),
                               NULL, NULL,
                               new_buffer, background,
                               CLAY_LAYER_COLOR_SPACE_RGB_LINEAR);

      clay_drawable_set_buffer_full (drawable, FALSE, NULL,
                                     new_buffer,
                                     GEGL_RECTANGLE (
                                       clay_item_get_offset_x (item),
                                       clay_item_get_offset_y (item),
                                       0, 0),
                                     TRUE);
      g_object_unref (new_buffer);
      g_object_unref (background);
    }

  if (G_TYPE_FROM_INSTANCE (channel) == CLAY_TYPE_CHANNEL)
    {
      gint width  = clay_image_get_width  (dest_image);
      gint height = clay_image_get_height (dest_image);

      clay_item_set_offset (item, 0, 0);

      if (clay_item_get_width  (item) != width ||
          clay_item_get_height (item) != height)
        {
          clay_item_resize (item, clay_get_user_context (dest_image->clay),
                            CLAY_FILL_TRANSPARENT,
                            width, height, 0, 0);
        }
    }

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
clay_channel_translate (ClayItem *item,
                        gdouble   off_x,
                        gdouble   off_y,
                        gboolean  push_undo)
{
  ClayChannel *channel = CLAY_CHANNEL (item);
  gint         x, y, width, height;

  clay_item_bounds (CLAY_ITEM (channel), &x, &y, &width, &height);

  /*  update the old area  */
  clay_drawable_update (CLAY_DRAWABLE (item), x, y, width, height);

  if (push_undo)
    clay_channel_push_undo (channel, NULL);

  if (clay_rectangle_intersect (x + SIGNED_ROUND (off_x),
                                y + SIGNED_ROUND (off_y),
                                width, height,
                                0, 0,
                                clay_item_get_width  (CLAY_ITEM (channel)),
                                clay_item_get_height (CLAY_ITEM (channel)),
                                &x, &y, &width, &height))
    {
      /*  copy the portion of the mask we will keep to a temporary
       *  buffer
       */
      GeglBuffer *tmp_buffer =
        gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                         clay_drawable_get_format (CLAY_DRAWABLE (channel)));

      clay_gegl_buffer_copy (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                             GEGL_RECTANGLE (x - SIGNED_ROUND (off_x),
                                             y - SIGNED_ROUND (off_y),
                                             width, height),
                             GEGL_ABYSS_NONE,
                             tmp_buffer,
                             GEGL_RECTANGLE (0, 0, 0, 0));

      /*  clear the mask  */
      gegl_buffer_clear (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                         NULL);

      /*  copy the temp mask back to the mask  */
      clay_gegl_buffer_copy (tmp_buffer, NULL, GEGL_ABYSS_NONE,
                             clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                             GEGL_RECTANGLE (x, y, 0, 0));

      /*  free the temporary mask  */
      g_object_unref (tmp_buffer);

      channel->x1 = x;
      channel->y1 = y;
      channel->x2 = x + width;
      channel->y2 = y + height;
    }
  else
    {
      /*  clear the mask  */
      gegl_buffer_clear (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                         NULL);

      channel->empty = TRUE;
      channel->x1    = 0;
      channel->y1    = 0;
      channel->x2    = clay_item_get_width  (CLAY_ITEM (channel));
      channel->y2    = clay_item_get_height (CLAY_ITEM (channel));
    }

  /*  update the new area  */
  clay_drawable_update (CLAY_DRAWABLE (item),
                        channel->x1, channel->y1,
                        channel->x2 - channel->x1,
                        channel->y2 - channel->y1);
}

static void
clay_channel_scale (ClayItem              *item,
                    gint                   new_width,
                    gint                   new_height,
                    gint                   new_offset_x,
                    gint                   new_offset_y,
                    ClayInterpolationType  interpolation_type,
                    ClayProgress          *progress)
{
  ClayChannel *channel = CLAY_CHANNEL (item);

  if (G_TYPE_FROM_INSTANCE (item) == CLAY_TYPE_CHANNEL)
    {
      new_offset_x = 0;
      new_offset_y = 0;
    }

  /*  don't waste CPU cycles scaling an empty or full channel  */
  if (channel->bounds_known && (channel->empty || channel->full))
    {
      ClayDrawable *drawable = CLAY_DRAWABLE (item);
      GeglBuffer   *new_buffer;

      new_buffer =
        clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, new_width, new_height),
                              clay_drawable_get_format (drawable),
                              drawable);

      clay_drawable_set_buffer_full (drawable,
                                     clay_item_is_attached (item), NULL,
                                     new_buffer,
                                     GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                     0,            0),
                                     TRUE);
      g_object_unref (new_buffer);

      if (channel->empty)
        clay_channel_clear (CLAY_CHANNEL (item), NULL, FALSE);
      else
        clay_channel_all (CLAY_CHANNEL (item), FALSE);
    }
  else
    {
      CLAY_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interpolation_type, progress);
    }
}

static void
clay_channel_resize (ClayItem     *item,
                     ClayContext  *context,
                     ClayFillType  fill_type,
                     gint          new_width,
                     gint          new_height,
                     gint          offset_x,
                     gint          offset_y)
{
  CLAY_ITEM_CLASS (parent_class)->resize (item, context, fill_type,
                                          new_width, new_height,
                                          offset_x, offset_y);

  if (G_TYPE_FROM_INSTANCE (item) == CLAY_TYPE_CHANNEL)
    {
      clay_item_set_offset (item, 0, 0);
    }
}

static ClayTransformResize
clay_channel_get_clip (ClayItem            *item,
                       ClayTransformResize  clip_result)
{
  return CLAY_TRANSFORM_RESIZE_CLIP;
}

static gboolean
clay_channel_fill (ClayItem         *item,
                   ClayDrawable     *drawable,
                   ClayFillOptions  *fill_options,
                   gboolean          push_undo,
                   ClayProgress     *progress,
                   GError          **error)
{
  ClayChannel        *channel = CLAY_CHANNEL (item);
  const ClayBoundSeg *segs_in;
  const ClayBoundSeg *segs_out;
  gint                n_segs_in;
  gint                n_segs_out;
  gint                offset_x, offset_y;

  if (! clay_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot fill empty channel."));
      return FALSE;
    }

  clay_item_get_offset (item, &offset_x, &offset_y);

  clay_drawable_fill_boundary (drawable,
                               fill_options,
                               segs_in, n_segs_in,
                               offset_x, offset_y,
                               push_undo);

  return TRUE;
}

static gboolean
clay_channel_stroke (ClayItem           *item,
                     ClayDrawable       *drawable,
                     ClayStrokeOptions  *stroke_options,
                     gboolean            push_undo,
                     ClayProgress       *progress,
                     GError            **error)
{
  ClayChannel        *channel = CLAY_CHANNEL (item);
  const ClayBoundSeg *segs_in;
  const ClayBoundSeg *segs_out;
  gint                n_segs_in;
  gint                n_segs_out;
  gboolean            retval = FALSE;
  gint                offset_x, offset_y;

  if (! clay_channel_boundary (channel, &segs_in, &segs_out,
                               &n_segs_in, &n_segs_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, CLAY_ERROR, CLAY_FAILED,
                           _("Cannot stroke empty channel."));
      return FALSE;
    }

  clay_item_get_offset (item, &offset_x, &offset_y);

  switch (clay_stroke_options_get_method (stroke_options))
    {
    case CLAY_STROKE_LINE:
      clay_drawable_stroke_boundary (drawable,
                                     stroke_options,
                                     n_segs_in > 0 ? segs_in   : segs_out,
                                     n_segs_in > 0 ? n_segs_in : n_segs_out,
                                     offset_x, offset_y,
                                     push_undo);
      retval = TRUE;
      break;

    case CLAY_STROKE_PAINT_METHOD:
      {
        ClayPaintInfo    *paint_info;
        ClayPaintCore    *core;
        ClayPaintOptions *paint_options;
        gboolean          emulate_dynamics;

        paint_info = clay_context_get_paint_info (CLAY_CONTEXT (stroke_options));

        core = g_object_new (paint_info->paint_type, NULL);

        paint_options = clay_stroke_options_get_paint_options (stroke_options);
        emulate_dynamics = clay_stroke_options_get_emulate_dynamics (stroke_options);

        retval = clay_paint_core_stroke_boundary (core, drawable,
                                                  paint_options,
                                                  emulate_dynamics,
                                                  n_segs_in > 0 ? segs_in   : segs_out,
                                                  n_segs_in > 0 ? n_segs_in : n_segs_out,
                                                  offset_x, offset_y,
                                                  push_undo, error);

        g_object_unref (core);
      }
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return retval;
}

static void
clay_channel_to_selection (ClayItem       *item,
                           ClayChannelOps  op,
                           gboolean        antialias,
                           gboolean        feather,
                           gdouble         feather_radius_x,
                           gdouble         feather_radius_y)
{
  ClayChannel *channel = CLAY_CHANNEL (item);
  ClayImage   *image   = clay_item_get_image (item);
  gint         off_x, off_y;

  clay_item_get_offset (item, &off_x, &off_y);

  clay_channel_select_channel (clay_image_get_mask (image),
                               CLAY_ITEM_GET_CLASS (item)->to_selection_desc,
                               channel, off_x, off_y,
                               op,
                               feather, feather_radius_x, feather_radius_x);
}

static void
clay_channel_convert_type (ClayDrawable     *drawable,
                           ClayImage        *dest_image,
                           const Babl       *new_format,
                           ClayColorProfile *src_profile,
                           ClayColorProfile *dest_profile,
                           GeglDitherMethod  layer_dither_type,
                           GeglDitherMethod  mask_dither_type,
                           gboolean          push_undo,
                           ClayProgress     *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                          clay_item_get_width  (CLAY_ITEM (drawable)),
                                          clay_item_get_height (CLAY_ITEM (drawable))),
                          new_format, drawable);

  if (mask_dither_type == GEGL_DITHER_NONE)
    {
      clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                             GEGL_ABYSS_NONE,
                             dest_buffer, NULL);
    }
  else
    {
      gint bits;

      bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

      clay_gegl_apply_dither (clay_drawable_get_buffer (drawable),
                              NULL, NULL,
                              dest_buffer, 1 << bits, mask_dither_type);
    }

  clay_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

static void
clay_channel_invalidate_boundary (ClayDrawable *drawable)
{
  ClayChannel *channel = CLAY_CHANNEL (drawable);

  channel->boundary_known = FALSE;
  channel->bounds_known   = FALSE;
}

static void
clay_channel_set_buffer (ClayDrawable        *drawable,
                         gboolean             push_undo,
                         const gchar         *undo_desc,
                         GeglBuffer          *buffer,
                         const GeglRectangle *bounds)
{
  ClayChannel *channel    = CLAY_CHANNEL (drawable);
  GeglBuffer  *old_buffer = clay_drawable_get_buffer (drawable);

  if (old_buffer)
    {
      g_signal_handlers_disconnect_by_func (old_buffer,
                                            clay_channel_buffer_changed,
                                            channel);
    }

  CLAY_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  buffer = clay_drawable_get_buffer (drawable);
  gegl_buffer_signal_connect (buffer, "changed",
                              G_CALLBACK (clay_channel_buffer_changed),
                              channel);

  if (clay_filter_peek_node (CLAY_FILTER (channel)))
    {
      const Babl *color_format =
        clay_babl_format (CLAY_RGB,
                          clay_babl_precision (CLAY_COMPONENT_TYPE_FLOAT,
                                               clay_drawable_get_trc (drawable)),
                          TRUE, NULL);

      gegl_node_set (channel->color_node,
                     "format", color_format,
                     NULL);
    }
}

static gdouble
clay_channel_get_opacity_at (ClayPickable *pickable,
                             gint          x,
                             gint          y)
{
  ClayChannel *channel = CLAY_CHANNEL (pickable);
  gdouble      value   = CLAY_OPACITY_TRANSPARENT;

  if (x >= 0 && x < clay_item_get_width  (CLAY_ITEM (channel)) &&
      y >= 0 && y < clay_item_get_height (CLAY_ITEM (channel)))
    {
      if (! channel->bounds_known ||
          (! channel->empty &&
           x >= channel->x1 &&
           x <  channel->x2 &&
           y >= channel->y1 &&
           y <  channel->y2))
        {
          gegl_buffer_sample (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                              x, y, NULL, &value, babl_format ("Y double"),
                              GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
        }
    }

  return value;
}

static void
clay_channel_save (ClaySavable   *savable,
                   ClaySaveState *state)
{
  ClayChannel *channel = CLAY_CHANNEL (savable);

  clay_savable_print_element_start (state, "channel",
                                    "show-masked", "%b", clay_channel_get_show_masked (channel),
                                    NULL);

  clay_savable_color_save (channel->color, NULL, NULL, state);
  clay_savable_print_element (state, "opacity", "%f",
                              clay_channel_get_opacity (channel), NULL);

  parent_savable_iface->save (savable, state);

  clay_savable_print_element_end (state, "channel");
}

static gboolean
clay_channel_load_enter (ClayLoadState  *state,
                         const gchar   **attribute_names,
                         const gchar   **attribute_values,
                         gpointer        user_data,
                         GError        **error)
{
  ClayChannel *channel;

  while (*attribute_names)
    {
      if (g_strcmp0 (*attribute_names, "show-masked") == 0 ||
          g_strcmp0 (*attribute_names, "apply") == 0       ||
          g_strcmp0 (*attribute_names, "edit") == 0        ||
          g_strcmp0 (*attribute_names, "show") == 0)
        {
          clay_savable_load_store_from_string (state, error,
                                               *attribute_names, G_TYPE_BOOLEAN, *attribute_values,
                                               NULL);
        }
      else
        {
          g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                       "%s: unexpected attribute: '%s'",
                       G_STRFUNC, *attribute_names);
        }

      attribute_names++;
      attribute_values++;
    }

  parent_savable_iface->load_enter (state, attribute_names, attribute_values,
                                    user_data, error);

  clay_savable_load_add_handlers (state, "color",
                                  clay_savable_enter_color,
                                  clay_savable_exit_color,
                                  NULL, NULL);
  clay_savable_load_add_simple_handler (state, "opacity", G_TYPE_DOUBLE,
                                        NULL, NULL, NULL,
                                        FALSE, FALSE, NULL);

  /* Default channel size is bogus. */
  if (! clay_savable_load_peek_active_object (state) ||
      CLAY_IS_DRAWABLE_FILTER (clay_savable_load_peek_active_object (state)))
    {
      channel = clay_channel_new (state->image, 1, 1, NULL, NULL);
    }
  else if (CLAY_IS_LAYER (clay_savable_load_peek_active_object (state)))
    {
      GeglColor *black = gegl_color_new ("black");
      channel = CLAY_CHANNEL (clay_layer_mask_new (state->image, 1, 1, NULL, black));
      g_object_unref (black);
    }
  else
    {
      g_return_val_if_reached (FALSE);
    }
  clay_savable_load_push_active_object (state, G_OBJECT (channel));

  return TRUE;
}

static gboolean
clay_channel_load_exit (ClayLoadState  *state,
                        const gchar    *text,
                        gsize           len,
                        gpointer        user_data,
                        GError        **error)
{
  ClayChannel  *channel;
  gboolean      show_masked = TRUE;
  GeglColor    *color       = NULL;
  gdouble       opacity     = -1.0;

  gboolean      apply       = TRUE;
  gboolean      edit        = FALSE;
  gboolean      show        = FALSE;

  g_return_val_if_fail (CLAY_IS_CHANNEL (clay_savable_load_peek_active_object (state)), FALSE);

  parent_savable_iface->load_exit (state, text, len, user_data, error);

  channel = CLAY_CHANNEL (clay_savable_load_pop_active_object (state));

  clay_savable_load_get_values (state,
                                "show-masked",     &show_masked,
                                "color",           &color,
                                "opacity",         &opacity,

                                "apply",           &apply,
                                "edit",            &edit,
                                "show",            &show,
                                NULL);


  if (clay_item_get_width (CLAY_ITEM (channel)) == 0 ||
      clay_item_get_height (CLAY_ITEM (channel)) == 0)
    {
      /* This may happen for missing or corrupted buffer files. */
      CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                               "Discarding empty channel \"%s\".",
                               clay_object_get_name (CLAY_ITEM (channel)));
      g_object_unref (channel);
    }
  else if (! clay_savable_load_peek_active_object (state))
    {
      /* Specific ClayChannel attributes. */
      clay_channel_set_show_masked (channel, show_masked);
      if (color != NULL)
        clay_channel_set_color (channel, color, FALSE);
      if (opacity >= CLAY_OPACITY_TRANSPARENT && opacity <= CLAY_OPACITY_OPAQUE)
        clay_channel_set_opacity (channel, opacity, FALSE);

      clay_image_add_channel (state->image, channel, NULL,
                              clay_container_get_n_children (clay_image_get_channels (state->image)),
                              FALSE);
    }
  else if (CLAY_IS_DRAWABLE_FILTER (clay_savable_load_peek_active_object (state)))
    {
      ClayDrawableFilter *filter = CLAY_DRAWABLE_FILTER (clay_savable_load_peek_active_object (state));

      g_object_set (filter, "mask", channel, NULL);
      g_object_unref (channel);
    }
  else if (CLAY_IS_LAYER (clay_savable_load_peek_active_object (state)))
    {
      /* We cannot add the mask directly because if the layer dimensions
       * may change while loading it (e.g. group layers if <mask/>
       * appears before children list), then the mask will be truncated.
       * If in the future, layer's and mask's dimensions get
       * disconnected, then we could reorder (or not) the action to add
       * a mask.
       */
      clay_savable_load_store_value (state, "layer-mask", channel, g_object_unref);
      clay_savable_load_store_from_string (state, NULL,
                                           "layer-mask:apply", G_TYPE_BOOLEAN, apply ? "true" : "false",
                                           "layer-mask:edit",  G_TYPE_BOOLEAN, edit ? "true" : "false",
                                           "layer-mask:show",  G_TYPE_BOOLEAN, show ? "true" : "false",
                                           NULL);
      clay_savable_load_bubble_up (state, "layer-mask");
      clay_savable_load_bubble_up (state, "layer-mask:apply");
      clay_savable_load_bubble_up (state, "layer-mask:edit");
      clay_savable_load_bubble_up (state, "layer-mask:show");
    }
  else
    {
      g_object_unref (channel);
      g_return_val_if_reached (FALSE);
    }

  return TRUE;
}

static gboolean
clay_channel_real_boundary (ClayChannel         *channel,
                            const ClayBoundSeg **segs_in,
                            const ClayBoundSeg **segs_out,
                            gint                *num_segs_in,
                            gint                *num_segs_out,
                            gint                 x1,
                            gint                 y1,
                            gint                 x2,
                            gint                 y2)
{
  if (! channel->boundary_known)
    {
      gint x3, y3, x4, y4;

      /* free the out of date boundary segments */
      g_free (channel->segs_in);
      g_free (channel->segs_out);

      if (clay_item_bounds (CLAY_ITEM (channel), &x3, &y3, &x4, &y4))
        {
          GeglBuffer    *buffer;
          GeglRectangle  rect = { x3, y3, x4, y4 };

          x4 += x3;
          y4 += y3;

          buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

          channel->segs_out = clay_boundary_find (buffer, &rect,
                                                  babl_format ("Y float"),
                                                  CLAY_BOUNDARY_IGNORE_BOUNDS,
                                                  x1, y1, x2, y2,
                                                  CLAY_BOUNDARY_HALF_WAY,
                                                  &channel->num_segs_out);
          x1 = MAX (x1, x3);
          y1 = MAX (y1, y3);
          x2 = MIN (x2, x4);
          y2 = MIN (y2, y4);

          if (x2 > x1 && y2 > y1)
            {
              channel->segs_in = clay_boundary_find (buffer, NULL,
                                                     babl_format ("Y float"),
                                                     CLAY_BOUNDARY_WITHIN_BOUNDS,
                                                     x1, y1, x2, y2,
                                                     CLAY_BOUNDARY_HALF_WAY,
                                                     &channel->num_segs_in);
            }
          else
            {
              channel->segs_in     = NULL;
              channel->num_segs_in = 0;
            }
        }
      else
        {
          channel->segs_in      = NULL;
          channel->segs_out     = NULL;
          channel->num_segs_in  = 0;
          channel->num_segs_out = 0;
        }

      channel->boundary_known = TRUE;
    }

  *segs_in      = channel->segs_in;
  *segs_out     = channel->segs_out;
  *num_segs_in  = channel->num_segs_in;
  *num_segs_out = channel->num_segs_out;

  return (! channel->empty);
}

static gboolean
clay_channel_real_is_empty (ClayChannel *channel)
{
  GeglBuffer *buffer;

  if (channel->bounds_known)
    return channel->empty;

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

  if (! clay_gegl_mask_is_empty (buffer))
    return FALSE;

  /*  The mask is empty, meaning we can set the bounds as known  */
  g_clear_pointer (&channel->segs_in,  g_free);
  g_clear_pointer (&channel->segs_out, g_free);

  channel->empty          = TRUE;
  channel->full           = FALSE;
  channel->num_segs_in    = 0;
  channel->num_segs_out   = 0;
  channel->bounds_known   = TRUE;
  channel->boundary_known = TRUE;
  channel->x1             = 0;
  channel->y1             = 0;
  channel->x2             = clay_item_get_width  (CLAY_ITEM (channel));
  channel->y2             = clay_item_get_height (CLAY_ITEM (channel));

  return TRUE;
}

static gboolean
clay_channel_real_is_full (ClayChannel *channel)
{
  if (channel->bounds_known)
    return channel->full;

  return FALSE;
}

static void
clay_channel_real_feather (ClayChannel *channel,
                           gdouble      radius_x,
                           gdouble      radius_y,
                           gboolean     edge_lock,
                           gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x <= 0.0 && radius_y <= 0.0)
    return;

  if (! clay_item_bounds (CLAY_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (clay_channel_is_empty (channel))
    return;

  x1 = MAX (0, x1 - ceil (radius_x));
  y1 = MAX (0, y1 - ceil (radius_y));

  x2 = MIN (clay_item_get_width  (CLAY_ITEM (channel)), x2 + ceil (radius_x));
  y2 = MIN (clay_item_get_height (CLAY_ITEM (channel)), y2 + ceil (radius_y));

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->feather_desc);

  clay_gegl_apply_feather (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                           NULL, NULL,
                           clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                           GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                           radius_x,
                           radius_y,
                           edge_lock);

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_sharpen (ClayChannel *channel,
                           gboolean     push_undo)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (channel);

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->sharpen_desc);

  clay_gegl_apply_threshold (clay_drawable_get_buffer (drawable),
                             NULL, NULL,
                             clay_drawable_get_buffer (drawable),
                             0.5);

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_clear (ClayChannel *channel,
                         const gchar *undo_desc,
                         gboolean     push_undo)
{
  GeglBuffer    *buffer;
  GeglRectangle  rect;
  GeglRectangle  aligned_rect;

  if (channel->bounds_known && channel->empty)
    return;

  if (push_undo)
    {
      if (! undo_desc)
        undo_desc = CLAY_CHANNEL_GET_CLASS (channel)->clear_desc;

      clay_channel_push_undo (channel, undo_desc);
    }

  buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

  if (channel->bounds_known)
    {
      rect.x      = channel->x1;
      rect.y      = channel->y1;
      rect.width  = channel->x2 - channel->x1;
      rect.height = channel->y2 - channel->y1;
    }
  else
    {
      rect.x      = 0;
      rect.y      = 0;
      rect.width  = clay_item_get_width  (CLAY_ITEM (channel));
      rect.height = clay_item_get_height (CLAY_ITEM (channel));
    }

  gegl_rectangle_align_to_buffer (&aligned_rect, &rect, buffer,
                                  GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

  gegl_buffer_clear (buffer, &aligned_rect);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = TRUE;
  channel->full         = FALSE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = clay_item_get_width  (CLAY_ITEM (channel));
  channel->y2           = clay_item_get_height (CLAY_ITEM (channel));

  clay_drawable_update (CLAY_DRAWABLE (channel),
                        rect.x, rect.y, rect.width, rect.height);
}

static void
clay_channel_real_all (ClayChannel *channel,
                       gboolean     push_undo)
{
  GeglColor *color;

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->all_desc);

  /*  clear the channel  */
  color = gegl_color_new ("#fff");
  gegl_buffer_set_color (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                         NULL, color);
  g_object_unref (color);

  /*  we know the bounds  */
  channel->bounds_known = TRUE;
  channel->empty        = FALSE;
  channel->full         = TRUE;
  channel->x1           = 0;
  channel->y1           = 0;
  channel->x2           = clay_item_get_width  (CLAY_ITEM (channel));
  channel->y2           = clay_item_get_height (CLAY_ITEM (channel));

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_invert (ClayChannel *channel,
                          gboolean     push_undo)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (channel);

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->invert_desc);

  if (channel->bounds_known && channel->empty)
    {
      clay_channel_all (channel, FALSE);
    }
  else if (channel->bounds_known && channel->full)
    {
      clay_channel_clear (channel, NULL, FALSE);
    }
  else
    {
      clay_gegl_apply_invert_linear (clay_drawable_get_buffer (drawable),
                                     NULL, NULL,
                                     clay_drawable_get_buffer (drawable));

      clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
    }
}

static void
clay_channel_real_border (ClayChannel            *channel,
                          gint                    radius_x,
                          gint                    radius_y,
                          ClayChannelBorderStyle  style,
                          gboolean                edge_lock,
                          gboolean                push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    {
      /* The relevant GEGL operations require radius_x and radius_y to be > 0.
       * When both are 0 (currently can only be achieved by the user through
       * PDB), the effect should be to clear the channel.
       */
      clay_channel_clear (channel,
                          CLAY_CHANNEL_GET_CLASS (channel)->border_desc,
                          push_undo);
      return;
    }
  else if (radius_x <= 0 || radius_y <= 0)
    {
      /* FIXME: Implement the case where only one of radius_x and radius_y is 0.
       * Currently, should never happen.
       */
      g_return_if_reached();
    }

  if (! clay_item_bounds (CLAY_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (clay_channel_is_empty (channel))
    return;

  if (x1 - radius_x < 0)
    x1 = 0;
  else
    x1 -= radius_x;

  if (x2 + radius_x > clay_item_get_width (CLAY_ITEM (channel)))
    x2 = clay_item_get_width (CLAY_ITEM (channel));
  else
    x2 += radius_x;

  if (y1 - radius_y < 0)
    y1 = 0;
  else
    y1 -= radius_y;

  if (y2 + radius_y > clay_item_get_height (CLAY_ITEM (channel)))
    y2 = clay_item_get_height (CLAY_ITEM (channel));
  else
    y2 += radius_y;

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->border_desc);

  clay_gegl_apply_border (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                          NULL, NULL,
                          clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, style, edge_lock);

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_grow (ClayChannel *channel,
                        gint         radius_x,
                        gint         radius_y,
                        gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      clay_channel_shrink (channel, -radius_x, -radius_y, FALSE, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! clay_item_bounds (CLAY_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (clay_channel_is_empty (channel) ||
      clay_channel_is_full (channel))
    return;

  if (x1 - radius_x > 0)
    x1 = x1 - radius_x;
  else
    x1 = 0;

  if (y1 - radius_y > 0)
    y1 = y1 - radius_y;
  else
    y1 = 0;

  if (x2 + radius_x < clay_item_get_width (CLAY_ITEM (channel)))
    x2 = x2 + radius_x;
  else
    x2 = clay_item_get_width (CLAY_ITEM (channel));

  if (y2 + radius_y < clay_item_get_height (CLAY_ITEM (channel)))
    y2 = y2 + radius_y;
  else
    y2 = clay_item_get_height (CLAY_ITEM (channel));

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->grow_desc);

  clay_gegl_apply_grow (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                        NULL, NULL,
                        clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                        GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                        radius_x, radius_y);

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_shrink (ClayChannel *channel,
                          gint         radius_x,
                          gint         radius_y,
                          gboolean     edge_lock,
                          gboolean     push_undo)
{
  gint x1, y1, x2, y2;

  if (radius_x == 0 && radius_y == 0)
    return;

  if (radius_x <= 0 && radius_y <= 0)
    {
      clay_channel_grow (channel, -radius_x, -radius_y, push_undo);
      return;
    }

  if (radius_x < 0 || radius_y < 0)
    return;

  if (! clay_item_bounds (CLAY_ITEM (channel), &x1, &y1, &x2, &y2))
    return;

  x2 += x1;
  y2 += y1;

  if (clay_channel_is_empty (channel))
    return;

  if (x1 > 0)
    x1--;
  if (y1 > 0)
    y1--;
  if (x2 < clay_item_get_width (CLAY_ITEM (channel)))
    x2++;
  if (y2 < clay_item_get_height (CLAY_ITEM (channel)))
    y2++;

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->shrink_desc);

  clay_gegl_apply_shrink (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                          NULL, NULL,
                          clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                          GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                          radius_x, radius_y, edge_lock);

  clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
}

static void
clay_channel_real_flood (ClayChannel *channel,
                         gboolean     push_undo)
{
  gint x, y, width, height;

  if (! clay_item_bounds (CLAY_ITEM (channel), &x, &y, &width, &height))
    return;

  if (clay_channel_is_empty (channel) ||
      clay_channel_is_full (channel))
    return;

  if (push_undo)
    clay_channel_push_undo (channel,
                            CLAY_CHANNEL_GET_CLASS (channel)->flood_desc);

  clay_gegl_apply_flood (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                         NULL, NULL,
                         clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                         GEGL_RECTANGLE (x, y, width, height));

  clay_drawable_update (CLAY_DRAWABLE (channel), x, y, width, height);
}

static void
clay_channel_buffer_changed (GeglBuffer          *buffer,
                             const GeglRectangle *rect,
                             ClayChannel         *channel)
{
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (channel));
}


/*  public functions  */

ClayChannel *
clay_channel_new (ClayImage   *image,
                  gint         width,
                  gint         height,
                  const gchar *name,
                  GeglColor   *color)
{
  ClayChannel *channel;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  channel =
    CLAY_CHANNEL (clay_drawable_new (CLAY_TYPE_CHANNEL,
                                     image, name,
                                     0, 0, width, height,
                                     clay_image_get_channel_format (image)));

  if (color)
    {
      g_clear_object (&channel->color);
      channel->color = gegl_color_duplicate (color);
    }

  channel->show_masked = TRUE;

  /*  selection mask variables  */
  channel->x2          = width;
  channel->y2          = height;

  return channel;
}

ClayChannel *
clay_channel_new_from_buffer (ClayImage   *image,
                              GeglBuffer  *buffer,
                              const gchar *name,
                              GeglColor   *color)
{
  ClayChannel *channel;
  GeglBuffer  *dest;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  channel = clay_channel_new (image,
                              gegl_buffer_get_width  (buffer),
                              gegl_buffer_get_height (buffer),
                              name, color);

  dest = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));
  clay_gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, dest, NULL);

  return channel;
}

ClayChannel *
clay_channel_new_from_alpha (ClayImage    *image,
                             ClayDrawable *drawable,
                             const gchar  *name,
                             GeglColor    *color)
{
  ClayChannel *channel;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_drawable_has_alpha (drawable), NULL);

  width  = clay_item_get_width  (CLAY_ITEM (drawable));
  height = clay_item_get_height (CLAY_ITEM (drawable));

  channel = clay_channel_new (image, width, height, name, color);

  clay_channel_clear (channel, NULL, FALSE);

  dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer,
                          clay_drawable_get_component_format (drawable,
                                                              CLAY_CHANNEL_ALPHA));
  clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                         GEGL_ABYSS_NONE,
                         dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

  return channel;
}

ClayChannel *
clay_channel_new_from_component (ClayImage       *image,
                                 ClayChannelType  type,
                                 const gchar     *name,
                                 GeglColor       *color)
{
  ClayChannel *channel;
  GeglBuffer  *src_buffer;
  GeglBuffer  *dest_buffer;
  gint         width;
  gint         height;
  const Babl  *format;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  format = clay_image_get_component_format (image, type);

  g_return_val_if_fail (format != NULL, NULL);

  clay_pickable_flush (CLAY_PICKABLE (image));

  src_buffer = clay_pickable_get_buffer (CLAY_PICKABLE (image));
  width  = gegl_buffer_get_width  (src_buffer);
  height = gegl_buffer_get_height (src_buffer);

  channel = clay_channel_new (image, width, height, name, color);

  dest_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (channel));

  gegl_buffer_set_format (dest_buffer, format);
  clay_gegl_buffer_copy (src_buffer, NULL, GEGL_ABYSS_NONE, dest_buffer, NULL);
  gegl_buffer_set_format (dest_buffer, NULL);

  return channel;
}

ClayChannel *
clay_channel_get_parent (ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), NULL);

  return CLAY_CHANNEL (clay_viewable_get_parent (CLAY_VIEWABLE (channel)));
}

void
clay_channel_set_color (ClayChannel *channel,
                        GeglColor   *color,
                        gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (GEGL_IS_COLOR (color));

  if (! clay_color_is_perceptually_identical (channel->color, (GeglColor *) color))
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (channel)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (channel));

          clay_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Color"),
                                              channel);
        }

      g_clear_object (&channel->color);
      channel->color = gegl_color_duplicate (color);

      if (clay_filter_peek_node (CLAY_FILTER (channel)))
        clay_gegl_node_set_color (channel->color_node, channel->color);

      clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

GeglColor *
clay_channel_get_color (ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), NULL);

  return channel->color;
}

gdouble
clay_channel_get_opacity (ClayChannel *channel)
{
  gdouble opacity;

  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), CLAY_OPACITY_TRANSPARENT);

  gegl_color_get_rgba (channel->color, NULL, NULL, NULL, &opacity);

  return opacity;
}

void
clay_channel_set_opacity (ClayChannel *channel,
                          gdouble      opacity,
                          gboolean     push_undo)
{
  gdouble current_opacity;

  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  gegl_color_get_rgba (channel->color, NULL, NULL, NULL, &current_opacity);
  opacity = CLAMP (opacity, CLAY_OPACITY_TRANSPARENT, CLAY_OPACITY_OPAQUE);

  if (current_opacity != opacity)
    {
      if (push_undo && clay_item_is_attached (CLAY_ITEM (channel)))
        {
          ClayImage *image = clay_item_get_image (CLAY_ITEM (channel));

          clay_image_undo_push_channel_color (image, C_("undo-type", "Set Channel Opacity"),
                                              channel);
        }

      clay_color_set_alpha (channel->color, opacity);

      if (clay_filter_peek_node (CLAY_FILTER (channel)))
        clay_gegl_node_set_color (channel->color_node, channel->color);

      clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);

      g_signal_emit (channel, channel_signals[COLOR_CHANGED], 0);
    }
}

gboolean
clay_channel_get_show_masked (ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), FALSE);

  return channel->show_masked;
}

void
clay_channel_set_show_masked (ClayChannel *channel,
                              gboolean     show_masked)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (show_masked != channel->show_masked)
    {
      channel->show_masked = show_masked ? TRUE : FALSE;

      if (channel->invert_node)
        {
          GeglNode *source;

          source = clay_drawable_get_source_node (CLAY_DRAWABLE (channel));

          if (channel->show_masked)
            {
              gegl_node_link (source, channel->invert_node);
              gegl_node_connect (channel->invert_node, "output",
                                 channel->mask_node,   "aux");
            }
          else
            {
              gegl_node_disconnect (channel->invert_node, "input");

              gegl_node_connect (source,             "output",
                                 channel->mask_node, "aux");
            }
        }

      clay_drawable_update (CLAY_DRAWABLE (channel), 0, 0, -1, -1);
    }
}

void
clay_channel_push_undo (ClayChannel *channel,
                        const gchar *undo_desc)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (channel)));

  clay_image_undo_push_mask (clay_item_get_image (CLAY_ITEM (channel)),
                             undo_desc, channel);
}


/******************************/
/*  selection mask functions  */
/******************************/

ClayChannel *
clay_channel_new_mask (ClayImage *image,
                       gint       width,
                       gint       height)
{
  ClayChannel *channel;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  channel =
    CLAY_CHANNEL (clay_drawable_new (CLAY_TYPE_CHANNEL,
                                     image, _("Selection Mask"),
                                     0, 0, width, height,
                                     clay_image_get_mask_format (image)));

  channel->show_masked = TRUE;
  channel->x2          = width;
  channel->y2          = height;

  gegl_buffer_clear (clay_drawable_get_buffer (CLAY_DRAWABLE (channel)),
                     NULL);

  return channel;
}

gboolean
clay_channel_boundary (ClayChannel         *channel,
                       const ClayBoundSeg **segs_in,
                       const ClayBoundSeg **segs_out,
                       gint                *num_segs_in,
                       gint                *num_segs_out,
                       gint                 x1,
                       gint                 y1,
                       gint                 x2,
                       gint                 y2)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), FALSE);
  g_return_val_if_fail (segs_in != NULL, FALSE);
  g_return_val_if_fail (segs_out != NULL, FALSE);
  g_return_val_if_fail (num_segs_in != NULL, FALSE);
  g_return_val_if_fail (num_segs_out != NULL, FALSE);

  return CLAY_CHANNEL_GET_CLASS (channel)->boundary (channel,
                                                     segs_in, segs_out,
                                                     num_segs_in, num_segs_out,
                                                     x1, y1,
                                                     x2, y2);
}

gboolean
clay_channel_is_empty (ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), TRUE);

  return CLAY_CHANNEL_GET_CLASS (channel)->is_empty (channel);
}

gboolean
clay_channel_is_full (ClayChannel *channel)
{
  g_return_val_if_fail (CLAY_IS_CHANNEL (channel), FALSE);

  return CLAY_CHANNEL_GET_CLASS (channel)->is_full (channel);
}

void
clay_channel_feather (ClayChannel *channel,
                      gdouble      radius_x,
                      gdouble      radius_y,
                      gboolean     edge_lock,
                      gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->feather (channel, radius_x, radius_y,
                                             edge_lock, push_undo);
}

void
clay_channel_sharpen (ClayChannel *channel,
                      gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->sharpen (channel, push_undo);
}

void
clay_channel_clear (ClayChannel *channel,
                    const gchar *undo_desc,
                    gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->clear (channel, undo_desc, push_undo);
}

void
clay_channel_all (ClayChannel *channel,
                  gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->all (channel, push_undo);
}

void
clay_channel_invert (ClayChannel *channel,
                     gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->invert (channel, push_undo);
}

void
clay_channel_border (ClayChannel            *channel,
                     gint                    radius_x,
                     gint                    radius_y,
                     ClayChannelBorderStyle  style,
                     gboolean                edge_lock,
                     gboolean                push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->border (channel,
                                            radius_x, radius_y, style, edge_lock,
                                            push_undo);
}

void
clay_channel_grow (ClayChannel *channel,
                   gint         radius_x,
                   gint         radius_y,
                   gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->grow (channel, radius_x, radius_y,
                                          push_undo);
}

void
clay_channel_shrink (ClayChannel  *channel,
                     gint          radius_x,
                     gint          radius_y,
                     gboolean      edge_lock,
                     gboolean      push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->shrink (channel, radius_x, radius_y,
                                            edge_lock, push_undo);
}

void
clay_channel_flood (ClayChannel *channel,
                    gboolean     push_undo)
{
  g_return_if_fail (CLAY_IS_CHANNEL (channel));

  if (! clay_item_is_attached (CLAY_ITEM (channel)))
    push_undo = FALSE;

  CLAY_CHANNEL_GET_CLASS (channel)->flood (channel, push_undo);
}
