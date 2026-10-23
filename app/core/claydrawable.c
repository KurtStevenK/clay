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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-apply-operation.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"
#include "gegl/claytilehandlervalidate.h"

#include "clay.h"
#include "clay-memsize.h"
#include "clay-utils.h"
#include "claychannel.h"
#include "claycontext.h"
#include "claydrawable-combine.h"
#include "claydrawable-fill.h"
#include "claydrawable-filters.h"
#include "claydrawable-floating-selection.h"
#include "claydrawable-preview.h"
#include "claydrawable-private.h"
#include "claydrawable-shadow.h"
#include "claydrawable-transform.h"
#include "claydrawablefilter.h"
#include "claydrawablefiltermask.h"
#include "clayfilterstack.h"
#include "claygrouplayer.h"
#include "clayidtable.h"
#include "clayimage.h"
#include "clayimage-colormap.h"
#include "clayimage-undo-push.h"
#include "claymarshal.h"
#include "claypickable.h"
#include "clayprogress.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-log.h"

#include "clay-intl.h"


#define PAINT_UPDATE_CHUNK_WIDTH  32
#define PAINT_UPDATE_CHUNK_HEIGHT 32


enum
{
  UPDATE,
  FORMAT_CHANGED,
  ALPHA_CHANGED,
  BOUNDING_BOX_CHANGED,
  FILTERS_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void       clay_color_managed_iface_init    (ClayColorManagedInterface *iface);
static void       clay_pickable_iface_init         (ClayPickableInterface     *iface);
static void       clay_savable_iface_init          (ClaySavableInterface     *iface);

static void       clay_drawable_dispose            (GObject           *object);
static void       clay_drawable_finalize           (GObject           *object);
static void       clay_drawable_set_property       (GObject           *object,
                                                    guint              property_id,
                                                    const GValue      *value,
                                                    GParamSpec        *pspec);
static void       clay_drawable_get_property       (GObject           *object,
                                                    guint              property_id,
                                                    GValue            *value,
                                                    GParamSpec        *pspec);

static gint64     clay_drawable_get_memsize        (ClayObject        *object,
                                                    gint64            *gui_size);

static void       clay_drawable_size_changed       (ClayViewable      *viewable);
static gboolean   clay_drawable_get_size           (ClayViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void       clay_drawable_preview_freeze     (ClayViewable      *viewable);
static void       clay_drawable_preview_thaw       (ClayViewable      *viewable);

static GeglNode * clay_drawable_get_node           (ClayFilter        *filter);

static void       clay_drawable_removed            (ClayItem          *item);
static ClayItem * clay_drawable_duplicate          (ClayItem          *item,
                                                    GType              new_type);
static void       clay_drawable_convert            (ClayItem          *item,
                                                    ClayImage         *dest_image,
                                                    GType              old_type);
static void       clay_drawable_scale              (ClayItem          *item,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               new_offset_x,
                                                    gint               new_offset_y,
                                                    ClayInterpolationType interp_type,
                                                    ClayProgress      *progress);
static void       clay_drawable_resize             (ClayItem          *item,
                                                    ClayContext       *context,
                                                    ClayFillType       fill_type,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               offset_x,
                                                    gint               offset_y);
static void       clay_drawable_flip               (ClayItem          *item,
                                                    ClayContext       *context,
                                                    ClayOrientationType  flip_type,
                                                    gdouble            axis,
                                                    gboolean           clip_result);
static void       clay_drawable_rotate             (ClayItem          *item,
                                                    ClayContext       *context,
                                                    ClayRotationType   rotate_type,
                                                    gdouble            center_x,
                                                    gdouble            center_y,
                                                    gboolean           clip_result);
static void       clay_drawable_transform          (ClayItem          *item,
                                                    ClayContext       *context,
                                                    const ClayMatrix3 *matrix,
                                                    ClayTransformDirection direction,
                                                    ClayInterpolationType interpolation_type,
                                                    ClayTransformResize clip_result,
                                                    ClayProgress      *progress,
                                                    gboolean           push_undo);

static const guint8 *
                  clay_drawable_get_icc_profile    (ClayColorManaged  *managed,
                                                    gsize             *len);
static ClayColorProfile *
                  clay_drawable_get_color_profile  (ClayColorManaged  *managed);
static void       clay_drawable_profile_changed    (ClayColorManaged  *managed);

static gboolean   clay_drawable_get_pixel_at       (ClayPickable      *pickable,
                                                    gint               x,
                                                    gint               y,
                                                    const Babl        *format,
                                                    gpointer           pixel);
static void       clay_drawable_get_pixel_average  (ClayPickable      *pickable,
                                                    const GeglRectangle *rect,
                                                    const Babl        *format,
                                                    gpointer           pixel);

static void       clay_drawable_save               (ClaySavable       *savable,
                                                    ClaySaveState     *state);
static gboolean   clay_drawable_load_enter         (ClayLoadState     *state,
                                                    const gchar      **attribute_names,
                                                    const gchar      **attribute_values,
                                                    gpointer           user_data,
                                                    GError           **error);
static gboolean   clay_drawable_load_exit          (ClayLoadState     *state,
                                                    const gchar       *text,
                                                    gsize              len,
                                                    gpointer           user_data,
                                                    GError           **error);

static void       clay_drawable_real_update        (ClayDrawable      *drawable,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

static void    clay_drawable_real_filters_changed  (ClayDrawable      *drawable);
static gint64  clay_drawable_real_estimate_memsize (ClayDrawable      *drawable,
                                                    ClayComponentType  component_type,
                                                    gint               width,
                                                    gint               height);

static void       clay_drawable_real_update_all    (ClayDrawable      *drawable);

static ClayComponentMask
                clay_drawable_real_get_active_mask (ClayDrawable      *drawable);

static gboolean   clay_drawable_real_supports_alpha
                                                   (ClayDrawable     *drawable);

static void       clay_drawable_real_convert_type  (ClayDrawable      *drawable,
                                                    ClayImage         *dest_image,
                                                    const Babl        *new_format,
                                                    ClayColorProfile  *src_profile,
                                                    ClayColorProfile  *dest_profile,
                                                    GeglDitherMethod   layer_dither_type,
                                                    GeglDitherMethod   mask_dither_type,
                                                    gboolean           push_undo,
                                                    ClayProgress      *progress);

static GeglBuffer * clay_drawable_real_get_buffer  (ClayDrawable      *drawable);
static void       clay_drawable_real_set_buffer    (ClayDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    const GeglRectangle *bounds);
static void       clay_drawable_buffer_changed     (GeglBuffer          *buffer,
                                                    const GeglRectangle *rect,
                                                    ClayDrawable        *drawable);

static GeglRectangle clay_drawable_real_get_bounding_box
                                                   (ClayDrawable      *drawable);

static void       clay_drawable_real_push_undo     (ClayDrawable      *drawable,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);
static void       clay_drawable_real_swap_pixels   (ClayDrawable      *drawable,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y);
static GeglNode * clay_drawable_real_get_source_node (ClayDrawable    *drawable);

static void       clay_drawable_format_changed     (ClayDrawable      *drawable);
static void       clay_drawable_alpha_changed      (ClayDrawable      *drawable);

static gchar       * clay_drawable_get_cache_path     (ClayDrawable    *drawable);

static gboolean      clay_drawable_enter_filters      (ClayLoadState   *state,
                                                       const gchar    **attribute_names,
                                                       const gchar    **attribute_values,
                                                       gpointer         user_data,
                                                       GError         **error);


G_DEFINE_ABSTRACT_TYPE_WITH_CODE (ClayDrawable, clay_drawable, CLAY_TYPE_ITEM,
                                  G_ADD_PRIVATE (ClayDrawable)
                                  G_IMPLEMENT_INTERFACE (CLAY_TYPE_COLOR_MANAGED,
                                                         clay_color_managed_iface_init)
                                  G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                         clay_pickable_iface_init)
                                  G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                         clay_savable_iface_init))

#define parent_class clay_drawable_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;

static guint clay_drawable_signals[LAST_SIGNAL] = { 0 };


static void
clay_drawable_class_init (ClayDrawableClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayFilterClass   *filter_class      = CLAY_FILTER_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);

  clay_drawable_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableClass, update),
                  NULL, NULL,
                  clay_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  clay_drawable_signals[FORMAT_CHANGED] =
    g_signal_new ("format-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableClass, format_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_drawable_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_drawable_signals[BOUNDING_BOX_CHANGED] =
    g_signal_new ("bounding-box-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableClass, bounding_box_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_drawable_signals[FILTERS_CHANGED] =
    g_signal_new ("filters-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayDrawableClass, filters_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose           = clay_drawable_dispose;
  object_class->finalize          = clay_drawable_finalize;
  object_class->set_property      = clay_drawable_set_property;
  object_class->get_property      = clay_drawable_get_property;

  clay_object_class->get_memsize  = clay_drawable_get_memsize;

  viewable_class->size_changed    = clay_drawable_size_changed;
  viewable_class->get_size        = clay_drawable_get_size;
  viewable_class->get_new_preview = clay_drawable_get_new_preview;
  viewable_class->get_new_pixbuf  = clay_drawable_get_new_pixbuf;
  viewable_class->preview_freeze  = clay_drawable_preview_freeze;
  viewable_class->preview_thaw    = clay_drawable_preview_thaw;

  filter_class->get_node          = clay_drawable_get_node;

  item_class->removed             = clay_drawable_removed;
  item_class->duplicate           = clay_drawable_duplicate;
  item_class->convert             = clay_drawable_convert;
  item_class->scale               = clay_drawable_scale;
  item_class->resize              = clay_drawable_resize;
  item_class->flip                = clay_drawable_flip;
  item_class->rotate              = clay_drawable_rotate;
  item_class->transform           = clay_drawable_transform;

  klass->update                   = clay_drawable_real_update;
  klass->format_changed           = NULL;
  klass->alpha_changed            = NULL;
  klass->bounding_box_changed     = NULL;
  klass->filters_changed          = clay_drawable_real_filters_changed;
  klass->estimate_memsize         = clay_drawable_real_estimate_memsize;
  klass->update_all               = clay_drawable_real_update_all;
  klass->invalidate_boundary      = NULL;
  klass->get_active_mask          = clay_drawable_real_get_active_mask;
  klass->supports_alpha           = clay_drawable_real_supports_alpha;
  klass->convert_type             = clay_drawable_real_convert_type;
  klass->apply_buffer             = clay_drawable_real_apply_buffer;
  klass->get_buffer               = clay_drawable_real_get_buffer;
  klass->set_buffer               = clay_drawable_real_set_buffer;
  klass->get_bounding_box         = clay_drawable_real_get_bounding_box;
  klass->push_undo                = clay_drawable_real_push_undo;
  klass->swap_pixels              = clay_drawable_real_swap_pixels;
  klass->get_source_node          = clay_drawable_real_get_source_node;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
clay_drawable_init (ClayDrawable *drawable)
{
  drawable->private = clay_drawable_get_instance_private (drawable);

  drawable->private->cache_outdated = TRUE;

  _clay_drawable_filters_init (drawable);
}

/* sorry for the evil casts */

static void
clay_color_managed_iface_init (ClayColorManagedInterface *iface)
{
  iface->get_icc_profile   = clay_drawable_get_icc_profile;
  iface->get_color_profile = clay_drawable_get_color_profile;
  iface->profile_changed   = clay_drawable_profile_changed;
}

static void
clay_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->get_image               = (ClayImage     * (*) (ClayPickable *pickable)) clay_item_get_image;
  iface->get_format              = (const Babl    * (*) (ClayPickable *pickable)) clay_drawable_get_format;
  iface->get_format_with_alpha   = (const Babl    * (*) (ClayPickable *pickable)) clay_drawable_get_format_with_alpha;
  iface->get_buffer              = (GeglBuffer    * (*) (ClayPickable *pickable)) clay_drawable_get_buffer;
  iface->get_buffer_with_effects = (GeglBuffer    * (*) (ClayPickable *pickable)) clay_drawable_get_buffer_with_effects;
  iface->get_pixel_at            = clay_drawable_get_pixel_at;
  iface->get_pixel_average       = clay_drawable_get_pixel_average;
}

static void
clay_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = NULL;
  iface->save       = clay_drawable_save;
  iface->load_enter = clay_drawable_load_enter;
  iface->load_exit  = clay_drawable_load_exit;
}

static void
clay_drawable_dispose (GObject *object)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (object);

  if (clay_drawable_get_floating_sel (drawable))
    clay_drawable_detach_floating_sel (drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
clay_drawable_finalize (GObject *object)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (object);

  while (drawable->private->paint_count)
    clay_drawable_end_paint (drawable);

  g_clear_object (&drawable->private->buffer);
  g_clear_object (&drawable->private->format_profile);

  clay_drawable_free_shadow_buffer (drawable);

  g_clear_object (&drawable->private->source_node);
  g_clear_object (&drawable->private->buffer_source_node);

  _clay_drawable_filters_finalize (drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_drawable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, drawable->private->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_drawable_get_memsize (ClayObject *object,
                           gint64     *gui_size)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (object);
  gint64        memsize  = 0;

  memsize += clay_gegl_buffer_get_memsize (clay_drawable_get_buffer (drawable));
  memsize += clay_gegl_buffer_get_memsize (drawable->private->shadow);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_drawable_size_changed (ClayViewable *viewable)
{
  GList *list;
  gint   width;
  gint   height;

  if (CLAY_VIEWABLE_CLASS (parent_class)->size_changed)
    CLAY_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  width  = clay_item_get_width (CLAY_ITEM (viewable));
  height = clay_item_get_height (CLAY_ITEM (viewable));

  for (list = CLAY_LIST (CLAY_DRAWABLE (viewable)->private->filter_stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      if (CLAY_IS_DRAWABLE_FILTER (list->data))
        {
          ClayDrawableFilter *filter = list->data;
          GeglRectangle       rect   = { 0, 0, width, height };

          clay_drawable_filter_refresh_crop (filter, &rect);
        }
    }
}

static gboolean
clay_drawable_get_size (ClayViewable *viewable,
                        gint         *width,
                        gint         *height)
{
  ClayItem *item = CLAY_ITEM (viewable);

  *width  = clay_item_get_width  (item);
  *height = clay_item_get_height (item);

  return TRUE;
}

static void
clay_drawable_preview_freeze (ClayViewable *viewable)
{
  ClayViewable *parent = clay_viewable_get_parent (viewable);

  if (! parent && clay_item_is_attached (CLAY_ITEM (viewable)))
    parent = CLAY_VIEWABLE (clay_item_get_image (CLAY_ITEM (viewable)));

  if (parent)
    clay_viewable_preview_freeze (parent);
}

static void
clay_drawable_preview_thaw (ClayViewable *viewable)
{
  ClayViewable *parent = clay_viewable_get_parent (viewable);

  if (! parent && clay_item_is_attached (CLAY_ITEM (viewable)))
    parent = CLAY_VIEWABLE (clay_item_get_image (CLAY_ITEM (viewable)));

  if (parent)
    clay_viewable_preview_thaw (parent);
}

static GeglNode *
clay_drawable_get_node (ClayFilter *filter)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *output;

  node = CLAY_FILTER_CLASS (parent_class)->get_node (filter);

  g_warn_if_fail (drawable->private->mode_node == NULL);

  drawable->private->mode_node =
    gegl_node_new_child (node,
                         "operation", "clay:normal",
                         NULL);

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  gegl_node_link (input, drawable->private->mode_node);
  gegl_node_link (drawable->private->mode_node, output);

  return node;
}

static void
clay_drawable_removed (ClayItem *item)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (item);

  clay_drawable_free_shadow_buffer (drawable);

  if (CLAY_ITEM_CLASS (parent_class)->removed)
    CLAY_ITEM_CLASS (parent_class)->removed (item);
}

static ClayItem *
clay_drawable_duplicate (ClayItem *item,
                         GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_DRAWABLE (new_item))
    {
      ClayDrawable  *drawable     = CLAY_DRAWABLE (item);
      ClayDrawable  *new_drawable = CLAY_DRAWABLE (new_item);
      GeglBuffer    *new_buffer;
      ClayContainer *filters;
      GList         *list;

      new_buffer = clay_gegl_buffer_dup (clay_drawable_get_buffer (drawable),
                                         new_drawable);

      clay_drawable_set_buffer (new_drawable, FALSE, NULL, new_buffer);
      g_object_unref (new_buffer);

      filters = clay_drawable_get_filters (drawable);

      for (list = CLAY_LIST (filters)->queue->tail;
           list;
           list = g_list_previous (list))
        {
          ClayDrawableFilter *filter = list->data;

          if (CLAY_IS_DRAWABLE_FILTER (filter))
            {
              ClayDrawableFilter *new_filter;

              new_filter = clay_drawable_filter_duplicate (new_drawable,
                                                           filter);
              if (new_filter)
                {
                  clay_drawable_filter_apply (new_filter, NULL);
                  clay_drawable_filter_commit (new_filter, TRUE, NULL, FALSE);

                  clay_drawable_filter_layer_mask_freeze (new_filter);
                  g_object_unref (new_filter);
                }
            }
        }
    }

  return new_item;
}

static void
clay_drawable_convert (ClayItem  *item,
                       ClayImage *dest_image,
                       GType      old_type)
{
  ClayContainer *filters;

  filters = clay_drawable_get_filters (CLAY_DRAWABLE (item));
  if (clay_container_get_n_children (filters) > 0)
    {
      GList *filter_list;

      for (filter_list = CLAY_LIST (filters)->queue->tail;
           filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (CLAY_IS_DRAWABLE_FILTER (filter_list->data))
            {
              ClayDrawableFilter     *filter = filter_list->data;
              ClayDrawableFilterMask *mask;

              mask = clay_drawable_filter_get_mask (filter);

              if (mask)
                clay_drawable_convert (CLAY_ITEM (mask), dest_image,
                                       CLAY_TYPE_DRAWABLE_FILTER_MASK);
            }
        }
    }

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
clay_drawable_scale (ClayItem              *item,
                     gint                   new_width,
                     gint                   new_height,
                     gint                   new_offset_x,
                     gint                   new_offset_y,
                     ClayInterpolationType  interpolation_type,
                     ClayProgress          *progress)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (item);
  GeglBuffer   *new_buffer;

  new_buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     new_width, new_height),
                                     clay_drawable_get_format (drawable),
                                     drawable);

  clay_gegl_apply_scale (clay_drawable_get_buffer (drawable),
                         progress, C_("undo-type", "Scale"),
                         new_buffer,
                         interpolation_type,
                         ((gdouble) new_width /
                          clay_item_get_width  (item)),
                         ((gdouble) new_height /
                          clay_item_get_height (item)));

  clay_drawable_set_buffer_full (drawable, clay_item_is_attached (item), NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);
}

static void
clay_drawable_resize (ClayItem     *item,
                      ClayContext  *context,
                      ClayFillType  fill_type,
                      gint          new_width,
                      gint          new_height,
                      gint          offset_x,
                      gint          offset_y)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (item);
  GeglBuffer   *new_buffer;
  GList        *list;
  gint          new_offset_x;
  gint          new_offset_y;
  gint          copy_x, copy_y;
  gint          copy_width, copy_height;
  gboolean      intersect;

  /*  if the size doesn't change, this is a nop  */
  if (new_width  == clay_item_get_width  (item) &&
      new_height == clay_item_get_height (item) &&
      offset_x   == 0                           &&
      offset_y   == 0)
    return;

  new_offset_x = clay_item_get_offset_x (item) - offset_x;
  new_offset_y = clay_item_get_offset_y (item) - offset_y;

  intersect = clay_rectangle_intersect (clay_item_get_offset_x (item),
                                        clay_item_get_offset_y (item),
                                        clay_item_get_width (item),
                                        clay_item_get_height (item),
                                        new_offset_x,
                                        new_offset_y,
                                        new_width,
                                        new_height,
                                        &copy_x,
                                        &copy_y,
                                        &copy_width,
                                        &copy_height);

  new_buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     new_width, new_height),
                                     clay_drawable_get_format (drawable),
                                     drawable);

  if (! intersect              ||
      copy_width  != new_width ||
      copy_height != new_height)
    {
      /*  Clear the new buffer if needed  */

      GeglColor   *color;
      ClayPattern *pattern;

      if (clay_get_fill_params (context, fill_type, &color, &pattern, NULL))
        clay_drawable_fill_buffer (drawable, new_buffer,
                                   color, pattern, 0, 0);

      g_clear_object (&color);
    }

  if (intersect && copy_width && copy_height)
    {
      /*  Copy the pixels in the intersection  */
      clay_gegl_buffer_copy (
        clay_drawable_get_buffer (drawable),
        GEGL_RECTANGLE (copy_x - clay_item_get_offset_x (item),
                        copy_y - clay_item_get_offset_y (item),
                        copy_width,
                        copy_height), GEGL_ABYSS_NONE,
        new_buffer,
        GEGL_RECTANGLE (copy_x - new_offset_x,
                        copy_y - new_offset_y, 0, 0));
    }

  clay_drawable_set_buffer_full (drawable,
                                 clay_item_is_attached (item) &&
                                 drawable->private->push_resize_undo,
                                 NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);

  /* Since filter masks are channels, resizing the drawable will cause them
   * to move based on the new top-left coordinate. We need to correct the offset
   * after resizing. */
  for (list = CLAY_LIST (CLAY_DRAWABLE (item)->private->filter_stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      if (CLAY_IS_DRAWABLE_FILTER (list->data))
        {
          ClayDrawableFilter *filter = list->data;
          ClayChannel        *mask;
          gboolean            push_undo;

          push_undo = (clay_item_is_attached (item) &&
                       drawable->private->push_resize_undo);

          mask = CLAY_CHANNEL (clay_drawable_filter_get_mask (filter));
          if (! clay_channel_is_empty (mask))
            clay_item_translate (CLAY_ITEM (mask),
                                 offset_x,
                                 offset_y,
                                 push_undo);
        }
    }
}

static void
clay_drawable_flip (ClayItem            *item,
                    ClayContext         *context,
                    ClayOrientationType  flip_type,
                    gdouble              axis,
                    gboolean             clip_result)
{
  ClayDrawable     *drawable = CLAY_DRAWABLE (item);
  GeglBuffer       *buffer;
  ClayColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  clay_item_get_offset (item, &off_x, &off_y);

  buffer = clay_drawable_transform_buffer_flip (drawable, context,
                                                clay_drawable_get_buffer (drawable),
                                                off_x, off_y,
                                                flip_type, axis,
                                                clip_result,
                                                &buffer_profile,
                                                &new_off_x, &new_off_y);

  if (buffer)
    {
      clay_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, TRUE);
      g_object_unref (buffer);
    }
}

static void
clay_drawable_rotate (ClayItem         *item,
                      ClayContext      *context,
                      ClayRotationType  rotate_type,
                      gdouble           center_x,
                      gdouble           center_y,
                      gboolean          clip_result)
{
  ClayDrawable     *drawable = CLAY_DRAWABLE (item);
  GeglBuffer       *buffer;
  ClayColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  clay_item_get_offset (item, &off_x, &off_y);

  buffer = clay_drawable_transform_buffer_rotate (drawable, context,
                                                  clay_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  rotate_type, center_x, center_y,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y);

  if (buffer)
    {
      clay_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, TRUE);
      g_object_unref (buffer);
    }
}

static void
clay_drawable_transform (ClayItem               *item,
                         ClayContext            *context,
                         const ClayMatrix3      *matrix,
                         ClayTransformDirection  direction,
                         ClayInterpolationType   interpolation_type,
                         ClayTransformResize     clip_result,
                         ClayProgress           *progress,
                         gboolean                push_undo)
{
  ClayDrawable     *drawable = CLAY_DRAWABLE (item);
  GeglBuffer       *buffer;
  ClayColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  clay_item_get_offset (item, &off_x, &off_y);

  buffer = clay_drawable_transform_buffer_affine (drawable, context,
                                                  clay_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  matrix, direction,
                                                  interpolation_type,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y,
                                                  progress);

  if (buffer)
    {
      clay_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, push_undo);
      g_object_unref (buffer);
    }
}

static const guint8 *
clay_drawable_get_icc_profile (ClayColorManaged *managed,
                               gsize            *len)
{
  ClayColorProfile *profile = clay_color_managed_get_color_profile (managed);

  return clay_color_profile_get_icc_profile (profile, len);
}

static ClayColorProfile *
clay_drawable_get_color_profile (ClayColorManaged *managed)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (managed);
  const Babl   *format   = clay_drawable_get_format (drawable);

  if (! drawable->private->format_profile)
    drawable->private->format_profile =
      clay_babl_format_get_color_profile (format);

  return drawable->private->format_profile;
}

static void
clay_drawable_profile_changed (ClayColorManaged *managed)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (managed));
}

static gboolean
clay_drawable_get_pixel_at (ClayPickable *pickable,
                            gint          x,
                            gint          y,
                            const Babl   *format,
                            gpointer      pixel)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (pickable);

  /* do not make this a g_return_if_fail() */
  if (x < 0 || x >= clay_item_get_width  (CLAY_ITEM (drawable)) ||
      y < 0 || y >= clay_item_get_height (CLAY_ITEM (drawable)))
    return FALSE;

  gegl_buffer_sample (clay_drawable_get_buffer (drawable),
                      x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static void
clay_drawable_get_pixel_average (ClayPickable        *pickable,
                                 const GeglRectangle *rect,
                                 const Babl          *format,
                                 gpointer             pixel)
{
  ClayDrawable *drawable = CLAY_DRAWABLE (pickable);

  return clay_gegl_average_color (clay_drawable_get_buffer (drawable),
                                  rect, TRUE, GEGL_ABYSS_NONE, format, pixel);
}

static void
clay_drawable_save (ClaySavable   *savable,
                    ClaySaveState *state)
{
  ClayDrawable  *drawable = CLAY_DRAWABLE (savable);
  ClayContainer *filters;
  GList         *iter;
  gint           num_effects = 0;
  const Babl    *format;

  parent_savable_iface->save (savable, state);

  clay_savable_print_element (state, "dimensions", NULL, NULL,
                              "width",  "%d", clay_item_get_width (CLAY_ITEM (drawable)),
                              "height", "%d", clay_item_get_height (CLAY_ITEM (drawable)),
                              NULL);

  format = clay_drawable_get_format (drawable);
  clay_savable_format_save (format, state);

  if (! CLAY_IS_GROUP_LAYER (drawable))
    {
      gchar *path;

      if ((path = clay_drawable_get_cache_path (drawable)))
        {
          gchar *filename;

          if (drawable->private->cache_outdated)
            {
              gegl_buffer_flush (CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable));
              drawable->private->cache_outdated = FALSE;
            }
          filename = g_path_get_basename (path);
          clay_savable_print_element (state, "buffer", NULL, NULL, "file", "%s", filename, NULL);
          g_free (filename);
        }

      g_free (path);
    }

  /* Get filter information */
  filters = clay_drawable_get_filters (drawable);
  /* Since floating selections are also stored in the filter stack,
   * we need to verify what's in there to get the correct count */
  for (iter = CLAY_LIST (filters)->queue->tail; iter; iter = g_list_previous (iter))
    {
      if (CLAY_IS_DRAWABLE_FILTER (iter->data) &&
          ! clay_drawable_filter_get_temporary (iter->data))
        {
          ClayDrawableFilter     *filter  = iter->data;
          ClayDrawableFilterMask *mask    = NULL;
          GeglNode               *op_node = NULL;

          mask    = clay_drawable_filter_get_mask (filter);
          op_node = clay_drawable_filter_get_operation (filter);

          /* For now, prevent tool-based filters from being saved */
          if (mask    != NULL &&
              op_node != NULL &&
              strcmp (gegl_node_get_operation (op_node), "GraphNode") != 0)
            num_effects++;
        }
    }

  if (num_effects > 0)
    {
      clay_savable_print_element_start (state, "filters", NULL, NULL, NULL);

      for (iter = CLAY_LIST (filters)->queue->head; iter; iter = iter->next)
        {
          if (CLAY_IS_DRAWABLE_FILTER (iter->data) &&
              ! clay_drawable_filter_get_temporary (iter->data))
            {
              ClayDrawableFilter     *filter  = iter->data;
              ClayDrawableFilterMask *mask    = NULL;
              GeglNode               *op_node = NULL;

              mask    = clay_drawable_filter_get_mask (filter);
              op_node = clay_drawable_filter_get_operation (filter);

               /* For now, prevent tool-based filters from being saved */
              if (mask    != NULL &&
                  op_node != NULL &&
                  strcmp (gegl_node_get_operation (op_node), "GraphNode") != 0)
                clay_savable_save (CLAY_SAVABLE (filter), state);
            }
        }

      clay_savable_print_element_end (state, "filters");
    }
}

static gboolean
clay_drawable_load_enter (ClayLoadState  *state,
                          const gchar   **attribute_names,
                          const gchar   **attribute_values,
                          gpointer        user_data,
                          GError        **error)
{
  parent_savable_iface->load_enter (state, attribute_names, attribute_values, user_data, error);

  clay_savable_load_add_handlers (state, "format",
                                  clay_savable_enter_format,
                                  clay_savable_exit_format,
                                  NULL, NULL);

  clay_savable_load_add_simple_handler (state, "dimensions", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "width",  G_TYPE_INT,
                                        "height", G_TYPE_INT,
                                        NULL);
  clay_savable_load_add_simple_handler (state, "buffer", G_TYPE_NONE,
                                        NULL, NULL, NULL,
                                        TRUE, FALSE,
                                        "file", G_TYPE_STRING,
                                        NULL);

  clay_savable_load_add_handlers (state, "filters",
                                  clay_drawable_enter_filters,
                                  NULL, NULL, NULL);

  return TRUE;
}

static gboolean
clay_drawable_load_exit (ClayLoadState  *state,
                         const gchar    *text,
                         gsize           len,
                         gpointer        user_data,
                         GError        **error)
{
  ClayDrawable *drawable;
  gint          width           = 0;
  gint          height          = 0;
  const gchar  *buffer_filename = NULL;
  const Babl   *format          = NULL;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (clay_savable_load_peek_active_object (state)), FALSE);
  drawable = CLAY_DRAWABLE (clay_savable_load_peek_active_object (state));

  parent_savable_iface->load_exit (state, text, len, user_data, error);

  if (clay_savable_load_get_values (state,
                                    "dimensions:width",  &width,
                                    "dimensions:height", &height,
                                    NULL))
    /* Do we actually need to store the dimensions? Isn't the buffer
     * enough for raster drawables, or dimensions may be given through
     * other data (text, paths, etc.).
     */
    clay_item_set_size (CLAY_ITEM (drawable), width, height);

  if (clay_savable_load_get_values (state,
                                    "buffer:file", &buffer_filename,
                                    "format",      &format,
                                    NULL))
    {
      GFile      *buffers_dir;
      GFile      *buffer_file;
      GeglBuffer *buffer      = NULL;
      gint64      id          = 0;
      gboolean    is_ancestor = FALSE;

      buffers_dir = g_file_get_child (state->subdir, "buffers");
      buffer_file = g_file_get_child (buffers_dir, buffer_filename);

      /* Prevent malicious files trying to exit the allowed directory to
       * read external files on the FS.
       */
      if (clay_file_is_ancestor (buffers_dir, buffer_file))
        is_ancestor = TRUE;
      else
        CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                 "%s: unauthorized buffer file '%s': %s",
                                 G_STRFUNC,
                                 clay_object_get_name (CLAY_OBJECT (drawable)),
                                 buffer_filename);

      if (*error == NULL)
        buffer = g_object_new (GEGL_TYPE_BUFFER,
                               "format", format,
                               "path",   g_file_peek_path (buffer_file),
                               NULL);

      if (buffer == NULL)
        CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                 "%s: invalid buffer file content for drawable '%s': %s",
                                 G_STRFUNC,
                                 clay_object_get_name (CLAY_OBJECT (drawable)),
                                 buffer_filename);

      if (*error == NULL)
        {
          gchar      *pattern;
          GRegex     *regex;
          GMatchInfo *matches = NULL;
          GType       gtype;

          gtype   = G_TYPE_FROM_INSTANCE (drawable);
          pattern = g_strdup_printf ("^%s-([1-9][0-9]*)-[0-9]*\\.buffer$", g_type_name (gtype));
          regex   = g_regex_new (pattern, G_REGEX_DEFAULT, G_REGEX_MATCH_DEFAULT, NULL);
          if (g_regex_match (regex, buffer_filename, G_REGEX_MATCH_DEFAULT, &matches))
            {
              gchar  *id_str;

              id_str = g_match_info_fetch (matches, 1);
              id     = g_ascii_strtoll (id_str, NULL, 10);
              if (errno == ERANGE || (gint64) (gint) id != id              ||
                  id < CLAY_ID_TABLE_START_ID || id > CLAY_ID_TABLE_END_ID ||
                  (clay_item_get_id (CLAY_ITEM (drawable)) != id &&
                   ! clay_id_table_reserve (state->clay->item_table, (gint) id)))
                id = 0;

              g_free (id_str);
            }
          else
            {
              CLAY_SAVABLE_LOAD_ERROR (error, CLAY_WLBR_ERROR_DATA, ;,
                                       "%s: invalid buffer file name pattern for drawable '%s': %s",
                                       G_STRFUNC,
                                       clay_object_get_name (CLAY_OBJECT (drawable)),
                                       buffer_filename);
            }

          if (id > 0 && clay_item_get_id (CLAY_ITEM (drawable)) != id)
            clay_item_set_id (CLAY_ITEM (drawable), (gint) id);

          clay_drawable_set_buffer (CLAY_DRAWABLE (drawable), FALSE, NULL, buffer);

          /* We are trying to reserve the same ID so that we can just
           * reuse the buffer file as-is. This may fail though because
           * every run is different. In such a case, the original buffer
           * file will still be used for initialization, but the
           * drawable will end up syncing to a new file. So let's delete
           * the old file once we are done.
           */
          if (id == 0 && is_ancestor)
            g_file_delete (buffer_file, NULL, NULL);

          g_clear_pointer (&matches, g_match_info_free);
          g_regex_unref (regex);
          g_free (pattern);
        }
      /* TODO: test further what happens with invalid buffer file or metadata. */

      g_object_unref (buffers_dir);
      g_object_unref (buffer_file);
      g_clear_object (&buffer);
    }

  return TRUE;
}

static void
clay_drawable_real_update (ClayDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  clay_viewable_invalidate_preview (CLAY_VIEWABLE (drawable));
}

static void
clay_drawable_real_filters_changed (ClayDrawable *drawable)
{
  clay_drawable_update_bounding_box (drawable);
}

static gint64
clay_drawable_real_estimate_memsize (ClayDrawable      *drawable,
                                     ClayComponentType  component_type,
                                     gint               width,
                                     gint               height)
{
  ClayImage   *image = clay_item_get_image (CLAY_ITEM (drawable));
  ClayTRCType  trc   = clay_drawable_get_trc (drawable);
  const Babl  *format;

  format = clay_image_get_format (image,
                                  clay_drawable_get_base_type (drawable),
                                  clay_babl_precision (component_type, trc),
                                  clay_drawable_has_alpha (drawable),
                                  NULL);

  return (gint64) babl_format_get_bytes_per_pixel (format) * width * height;
}

static void
clay_drawable_real_update_all (ClayDrawable *drawable)
{
  clay_drawable_update (drawable, 0, 0, -1, -1);
}

static ClayComponentMask
clay_drawable_real_get_active_mask (ClayDrawable *drawable)
{
  /*  Return all, because that skips the component mask op when painting  */
  return CLAY_COMPONENT_MASK_ALL;
}

static gboolean
clay_drawable_real_supports_alpha (ClayDrawable *drawable)
{
  return FALSE;
}

/* FIXME: this default impl is currently unused because no subclass
 * chains up. the goal is to handle the almost identical subclass code
 * here again.
 */
static void
clay_drawable_real_convert_type (ClayDrawable      *drawable,
                                 ClayImage         *dest_image,
                                 const Babl        *new_format,
                                 ClayColorProfile  *src_profile,
                                 ClayColorProfile  *dest_profile,
                                 GeglDitherMethod   layer_dither_type,
                                 GeglDitherMethod   mask_dither_type,
                                 gboolean           push_undo,
                                 ClayProgress      *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                          clay_item_get_width  (CLAY_ITEM (drawable)),
                                          clay_item_get_height (CLAY_ITEM (drawable))),
                          new_format, drawable);

  clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable), NULL,
                         GEGL_ABYSS_NONE,
                         dest_buffer, NULL);

  clay_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

static GeglBuffer *
clay_drawable_real_get_buffer (ClayDrawable *drawable)
{
  return drawable->private->buffer;
}

static void
clay_drawable_real_set_buffer (ClayDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds)
{
  ClayItem            *item          = CLAY_ITEM (drawable);
  const Babl          *old_format    = NULL;
  gint                 old_has_alpha = -1;
  const GeglRectangle *extent        = gegl_buffer_get_extent (buffer);
  gboolean             free_buffer   = FALSE;

  g_object_freeze_notify (G_OBJECT (drawable));

  clay_drawable_invalidate_boundary (drawable);

  if (push_undo)
    clay_image_undo_push_drawable_mod (clay_item_get_image (item), undo_desc,
                                       drawable, FALSE);

  if (drawable->private->buffer)
    {
      old_format    = clay_drawable_get_format (drawable);
      old_has_alpha = clay_drawable_has_alpha (drawable);

      g_signal_handlers_disconnect_by_func (drawable->private->buffer,
                                            clay_drawable_buffer_changed,
                                            drawable);
    }

  if (! CLAY_IS_GROUP_LAYER (drawable) && (extent->x != 0 || extent->y != 0))
    {
      /* Drawable buffers are always stored with a (0, 0) origin. When
       * setting a buffer with a different origin, we will assume we
       * instead want to update the offset a bit.
       * This may happen for instance when merging filters which may
       * render in negative coordinates.
       *
       * The only exception to this is group layers. Group layer's
       * boundaries will be determined by children layers, but the
       * render may extend outside the boundaries (if children have
       * effects).
       */
      buffer = g_object_new (GEGL_TYPE_BUFFER,
                             "source",  buffer,
                             "shift-x", extent->x,
                             "shift-y", extent->y,
                             NULL);
      free_buffer = TRUE;
    }

  g_set_object (&drawable->private->buffer, buffer);

  if (clay_drawable_is_painting (drawable))
    g_set_object (&drawable->private->paint_buffer, buffer);

  g_clear_object (&drawable->private->format_profile);

  if (drawable->private->buffer_source_node)
    gegl_node_set (drawable->private->buffer_source_node,
                   "buffer", clay_drawable_get_buffer (drawable),
                   NULL);

  if (CLAY_IS_GROUP_LAYER (drawable))
    clay_item_set_offset (item, bounds->x, bounds->y);
  else
    clay_item_set_offset (item, bounds->x + extent->x, bounds->y + extent->y);
  clay_item_set_size (item,
                      bounds->width  ? bounds->width :
                                       gegl_buffer_get_width (buffer),
                      bounds->height ? bounds->height :
                                       gegl_buffer_get_height (buffer));

  clay_drawable_update_bounding_box (drawable);

  if (clay_drawable_get_format (drawable) != old_format)
    clay_drawable_format_changed (drawable);

  if (clay_drawable_has_alpha (drawable) != old_has_alpha)
    clay_drawable_alpha_changed (drawable);

  g_object_notify (G_OBJECT (drawable), "buffer");

  g_object_thaw_notify (G_OBJECT (drawable));

  if (free_buffer)
    g_object_unref (buffer);

  gegl_buffer_signal_connect (drawable->private->buffer,
                              "changed",
                              G_CALLBACK (clay_drawable_buffer_changed),
                              drawable);

  drawable->private->cache_outdated = TRUE;
}

static void
clay_drawable_buffer_changed (GeglBuffer          *buffer,
                              const GeglRectangle *rect,
                              ClayDrawable        *drawable)
{
  drawable->private->cache_outdated = TRUE;
}

static GeglRectangle
clay_drawable_real_get_bounding_box (ClayDrawable *drawable)
{
  return gegl_node_get_bounding_box (clay_drawable_get_source_node (drawable));
}

static void
clay_drawable_real_push_undo (ClayDrawable *drawable,
                              const gchar  *undo_desc,
                              GeglBuffer   *buffer,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height)
{
  ClayImage *image;

  if (! buffer)
    {
      GeglBuffer    *drawable_buffer = clay_drawable_get_buffer (drawable);
      GeglRectangle  drawable_rect;

      gegl_rectangle_align_to_buffer (
        &drawable_rect,
        GEGL_RECTANGLE (x, y, width, height),
        drawable_buffer,
        GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      x      = drawable_rect.x;
      y      = drawable_rect.y;
      width  = drawable_rect.width;
      height = drawable_rect.height;

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                clay_drawable_get_format (drawable));

      clay_gegl_buffer_copy (
        drawable_buffer,
        &drawable_rect, GEGL_ABYSS_NONE,
        buffer,
        GEGL_RECTANGLE (0, 0, 0, 0));
    }
  else
    {
      g_object_ref (buffer);
    }

  image = clay_item_get_image (CLAY_ITEM (drawable));

  clay_image_undo_push_drawable (image,
                                 undo_desc, drawable,
                                 buffer, x, y);

  g_object_unref (buffer);
}

static void
clay_drawable_real_swap_pixels (ClayDrawable *drawable,
                                GeglBuffer   *buffer,
                                gint          x,
                                gint          y)
{
  GeglBuffer *tmp;
  gint        width  = gegl_buffer_get_width (buffer);
  gint        height = gegl_buffer_get_height (buffer);

  tmp = clay_gegl_buffer_dup (buffer, drawable);

  clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, width, height), GEGL_ABYSS_NONE,
                         buffer,
                         GEGL_RECTANGLE (0, 0, 0, 0));
  clay_gegl_buffer_copy (tmp,
                         GEGL_RECTANGLE (0, 0, width, height), GEGL_ABYSS_NONE,
                         clay_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, 0, 0));

  g_object_unref (tmp);

  clay_drawable_update (drawable, x, y, width, height);
}

static GeglNode *
clay_drawable_real_get_source_node (ClayDrawable *drawable)
{
  g_warn_if_fail (drawable->private->buffer_source_node == NULL);

  drawable->private->buffer_source_node =
    gegl_node_new_child (NULL,
                         "operation", "clay:buffer-source-validate",
                         "buffer",    clay_drawable_get_buffer (drawable),
                         NULL);

  return g_object_ref (drawable->private->buffer_source_node);
}

static void
clay_drawable_format_changed (ClayDrawable *drawable)
{
  g_signal_emit (drawable, clay_drawable_signals[FORMAT_CHANGED], 0);
}

static void
clay_drawable_alpha_changed (ClayDrawable *drawable)
{
  g_signal_emit (drawable, clay_drawable_signals[ALPHA_CHANGED], 0);
}

static gchar *
clay_drawable_get_cache_path (ClayDrawable *drawable)
{
  GeglBuffer *buffer;
  gchar      *path = NULL;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  buffer = CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);
  if (buffer)
    g_object_get (buffer,
                  "path", &path,
                  NULL);
  return path;
}

static gboolean
clay_drawable_enter_filters (ClayLoadState  *state,
                             const gchar   **attribute_names,
                             const gchar   **attribute_values,
                             gpointer        user_data,
                             GError        **error)
{
  clay_savable_load (CLAY_TYPE_DRAWABLE_FILTER, state);

  return TRUE;
}

/*  public functions  */

ClayDrawable *
clay_drawable_new (GType          type,
                   ClayImage     *image,
                   const gchar   *name,
                   gint           offset_x,
                   gint           offset_y,
                   gint           width,
                   gint           height,
                   const Babl    *format)
{
  ClayDrawable *drawable;
  GeglBuffer   *buffer;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (type, CLAY_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  drawable = CLAY_DRAWABLE (clay_item_new (type,
                                           image, name,
                                           offset_x, offset_y,
                                           width, height));

  buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height), format,
                                 drawable);

  clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  clay_drawable_enable_resize_undo (drawable);

  return drawable;
}

gint64
clay_drawable_estimate_memsize (ClayDrawable      *drawable,
                                ClayComponentType  component_type,
                                gint               width,
                                gint               height)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), 0);

  return CLAY_DRAWABLE_GET_CLASS (drawable)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

void
clay_drawable_update (ClayDrawable *drawable,
                      gint          x,
                      gint          y,
                      gint          width,
                      gint          height)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  if (width < 0 || height < 0)
    {
      GeglRectangle bounding_box;

      bounding_box = clay_drawable_get_bounding_box (drawable);

      if (width < 0)
        {
          x     = bounding_box.x;
          width = bounding_box.width;
        }

      if (height < 0)
        {
          y      = bounding_box.y;
          height = bounding_box.height;
        }
    }

  if (drawable->private->paint_count == 0)
    {
      g_signal_emit (drawable, clay_drawable_signals[UPDATE], 0,
                     x, y, width, height);
    }
  else
    {
      GeglRectangle rect;

      if (gegl_rectangle_intersect (
            &rect,
            GEGL_RECTANGLE (x, y, width, height),
            GEGL_RECTANGLE (0, 0,
                            clay_item_get_width  (CLAY_ITEM (drawable)),
                            clay_item_get_height (CLAY_ITEM (drawable)))))
        {
          GeglRectangle aligned_rect;

          gegl_rectangle_align_to_buffer (&aligned_rect, &rect,
                                          clay_drawable_get_buffer (drawable),
                                          GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          if (drawable->private->paint_copy_region)
            {
              cairo_region_union_rectangle (
                drawable->private->paint_copy_region,
                (const cairo_rectangle_int_t *) &aligned_rect);
            }
          else
            {
              drawable->private->paint_copy_region =
                cairo_region_create_rectangle (
                  (const cairo_rectangle_int_t *) &aligned_rect);
            }

          gegl_rectangle_align (&aligned_rect, &rect,
                                GEGL_RECTANGLE (0, 0,
                                                PAINT_UPDATE_CHUNK_WIDTH,
                                                PAINT_UPDATE_CHUNK_HEIGHT),
                                GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          if (drawable->private->paint_update_region)
            {
              cairo_region_union_rectangle (
                drawable->private->paint_update_region,
                (const cairo_rectangle_int_t *) &aligned_rect);
            }
          else
            {
              drawable->private->paint_update_region =
                cairo_region_create_rectangle (
                  (const cairo_rectangle_int_t *) &aligned_rect);
            }
        }
    }
}

void
clay_drawable_update_all (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  CLAY_DRAWABLE_GET_CLASS (drawable)->update_all (drawable);
}

void
clay_drawable_filters_changed (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  g_signal_emit (drawable, clay_drawable_signals[FILTERS_CHANGED], 0);
}

void
clay_drawable_invalidate_boundary (ClayDrawable *drawable)
{
  ClayDrawableClass *drawable_class;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  drawable_class = CLAY_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->invalidate_boundary)
    drawable_class->invalidate_boundary (drawable);
}

ClayComponentMask
clay_drawable_get_active_mask (ClayDrawable *drawable)
{
  ClayComponentMask mask;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), 0);

  mask = CLAY_DRAWABLE_GET_CLASS (drawable)->get_active_mask (drawable);

  /* if the drawable doesn't have an alpha channel, the value of the mask's
   * alpha-bit doesn't matter, however, we'd like to have a fully-clear or
   * fully-set mask whenever possible, since it allows us to skip component
   * masking altogether.  we therefore set or clear the alpha bit, depending on
   * the state of the other bits, so that it never gets in the way of a uniform
   * mask.
   */
  if (! clay_drawable_has_alpha (drawable))
    {
      if (mask & ~CLAY_COMPONENT_MASK_ALPHA)
        mask |= CLAY_COMPONENT_MASK_ALPHA;
      else
        mask &= ~CLAY_COMPONENT_MASK_ALPHA;
    }

  return mask;
}

gboolean
clay_drawable_supports_alpha (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  return CLAY_DRAWABLE_GET_CLASS (drawable)->supports_alpha (drawable);
}

void
clay_drawable_convert_type (ClayDrawable      *drawable,
                            ClayImage         *dest_image,
                            ClayImageBaseType  new_base_type,
                            ClayPrecision      new_precision,
                            gboolean           new_has_alpha,
                            ClayColorProfile  *src_profile,
                            ClayColorProfile  *dest_profile,
                            GeglDitherMethod   layer_dither_type,
                            GeglDitherMethod   mask_dither_type,
                            gboolean           push_undo,
                            ClayProgress      *progress)
{
  const Babl    *old_format;
  const Babl    *new_format;
  gint           old_bits;
  gint           new_bits;
  ClayContainer *filters;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (CLAY_IS_IMAGE (dest_image));
  g_return_if_fail (new_base_type != clay_drawable_get_base_type (drawable) ||
                    new_precision != clay_drawable_get_precision (drawable) ||
                    new_has_alpha != clay_drawable_has_alpha (drawable)     ||
                    dest_profile);
  g_return_if_fail (src_profile == NULL || CLAY_IS_COLOR_PROFILE (src_profile));
  g_return_if_fail (dest_profile == NULL || CLAY_IS_COLOR_PROFILE (dest_profile));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  if (! clay_item_is_attached (CLAY_ITEM (drawable)))
    push_undo = FALSE;

  old_format = clay_drawable_get_format (drawable);
  new_format = clay_image_get_format (dest_image,
                                      new_base_type,
                                      new_precision,
                                      new_has_alpha,
                                      NULL /* handled by layer */);

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  if (old_bits <= new_bits || new_bits > 16)
    {
      /*  don't dither if we are converting to a higher bit depth,
       *  or to more than 16 bits (gegl:dither only does
       *  16 bits).
       */
      layer_dither_type = GEGL_DITHER_NONE;
      mask_dither_type  = GEGL_DITHER_NONE;
    }

  CLAY_DRAWABLE_GET_CLASS (drawable)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);

  /* Update the masks of any filters */
  /* TODO: Move to clay_drawable_real_convert_type () once it's updated
   * to run for all ClayDrawable child classes */
  filters = clay_drawable_get_filters (drawable);
  if (clay_container_get_n_children (filters) > 0)
    {
      const Babl    *mask_format;
      GList         *filter_list;
      ClayPrecision  new_mask_precision;

      mask_format        = clay_image_get_mask_format (dest_image);
      new_mask_precision = clay_babl_format_get_precision (mask_format);

      for (filter_list = CLAY_LIST (filters)->queue->tail;
           filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (CLAY_IS_DRAWABLE_FILTER (filter_list->data))
            {
              ClayDrawableFilter     *filter = filter_list->data;
              ClayDrawableFilterMask *mask;

              mask = clay_drawable_filter_get_mask (filter);

              if (new_mask_precision == clay_drawable_get_precision (CLAY_DRAWABLE (mask)))
                /* The only change which may happen for masks is the bit-depth. */
                break;

              clay_drawable_convert_type (CLAY_DRAWABLE (mask), dest_image,
                                          CLAY_GRAY, new_mask_precision,
                                          FALSE,
                                          NULL, NULL,
                                          layer_dither_type, mask_dither_type,
                                          push_undo, progress);
            }
        }
    }

  if (progress)
    clay_progress_set_value (progress, 1.0);
}

void
clay_drawable_apply_buffer (ClayDrawable           *drawable,
                            GeglBuffer             *buffer,
                            const GeglRectangle    *buffer_region,
                            gboolean                push_undo,
                            const gchar            *undo_desc,
                            gdouble                 opacity,
                            ClayLayerMode           mode,
                            ClayLayerColorSpace     blend_space,
                            ClayLayerColorSpace     composite_space,
                            ClayLayerCompositeMode  composite_mode,
                            GeglBuffer             *base_buffer,
                            gint                    base_x,
                            gint                    base_y)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_region != NULL);
  g_return_if_fail (base_buffer == NULL || GEGL_IS_BUFFER (base_buffer));

  CLAY_DRAWABLE_GET_CLASS (drawable)->apply_buffer (drawable, buffer,
                                                    buffer_region,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    blend_space,
                                                    composite_space,
                                                    composite_mode,
                                                    base_buffer,
                                                    base_x, base_y);
}

GeglBuffer *
clay_drawable_get_buffer (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->paint_count == 0)
    return CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);
  else
    return drawable->private->paint_buffer;
}

void
clay_drawable_set_buffer (ClayDrawable *drawable,
                          gboolean      push_undo,
                          const gchar  *undo_desc,
                          GeglBuffer   *buffer)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (! clay_item_is_attached (CLAY_ITEM (drawable)))
    push_undo = FALSE;

  clay_drawable_set_buffer_full (drawable, push_undo, undo_desc, buffer, NULL,
                                 TRUE);
}

void
clay_drawable_set_buffer_full (ClayDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds,
                               gboolean             update)
{
  ClayItem      *item;
  GeglRectangle  curr_bounds;
  gboolean       recreate_buffer = (! CLAY_IS_GROUP_LAYER (drawable));

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  item = CLAY_ITEM (drawable);

  if (recreate_buffer)
    {
      ClayImage *image = clay_item_get_image (item);
      gchar     *path  = NULL;

      g_return_if_fail (image != NULL);

      g_object_get (buffer, "path", &path, NULL);
      if (path)
        {
          const gchar *folder;
          gchar       *dirname;
          gchar       *basename;
          gchar       *prefix;

          folder   = clay_image_get_buffers_folder (image);
          dirname  = g_path_get_dirname (path);
          basename = g_path_get_basename (path);
          prefix   = g_strdup_printf ("%s-%d-",
                                      g_type_name (G_TYPE_FROM_INSTANCE (drawable)),
                                      clay_item_get_id (CLAY_ITEM (drawable)));

          if (g_strcmp0 (folder, dirname) == 0 && g_str_has_prefix (basename, prefix))
            recreate_buffer = FALSE;

          g_free (dirname);
          g_free (basename);
          g_free (prefix);
          g_free (path);
        }


      if (recreate_buffer)
        {
          GeglBuffer *file_buffer;

          file_buffer = clay_gegl_buffer_new (gegl_buffer_get_extent (buffer),
                                              gegl_buffer_get_format (buffer),
                                              drawable);
          clay_gegl_buffer_copy (buffer, NULL,
                                 GEGL_ABYSS_NONE,
                                 file_buffer, NULL);
          buffer = file_buffer;
        }
    }

  if (! clay_item_is_attached (CLAY_ITEM (drawable)))
    push_undo = FALSE;

  if (! bounds)
    {
      clay_item_get_offset (CLAY_ITEM (drawable),
                            &curr_bounds.x, &curr_bounds.y);

      curr_bounds.width  = 0;
      curr_bounds.height = 0;

      bounds = &curr_bounds;
    }

  if (update && clay_drawable_get_buffer (drawable))
    {
      GeglBuffer    *old_buffer = clay_drawable_get_buffer (drawable);
      GeglRectangle  old_extent;
      GeglRectangle  new_extent;

      old_extent = *gegl_buffer_get_extent (old_buffer);
      old_extent.x += clay_item_get_offset_x (item);
      old_extent.y += clay_item_get_offset_x (item);

      new_extent = *gegl_buffer_get_extent (buffer);
      new_extent.x += bounds->x;
      new_extent.y += bounds->y;

      if (! gegl_rectangle_equal (&old_extent, &new_extent))
        clay_drawable_update (drawable, 0, 0, -1, -1);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  CLAY_DRAWABLE_GET_CLASS (drawable)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  g_object_thaw_notify (G_OBJECT (drawable));

  if (update)
    clay_drawable_update (drawable, 0, 0, -1, -1);

  if (recreate_buffer)
    g_object_unref (buffer);
}

GeglBuffer *
clay_drawable_get_buffer_with_effects (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->paint_count == 0)
    {
      if (clay_drawable_has_visible_filters (drawable))
        {
          GeglNode                *source = NULL;
          GeglBuffer              *buffer;
          ClayTileHandlerValidate *validate;

          source = clay_drawable_get_source_node (drawable);
          buffer = CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);

          if (source)
            {
              buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                                        gegl_buffer_get_format (buffer));

              validate =
                CLAY_TILE_HANDLER_VALIDATE (clay_tile_handler_validate_new (source));

              clay_tile_handler_validate_assign (validate, buffer);

              g_object_unref (validate);

              clay_tile_handler_validate_invalidate (validate,
                                                     gegl_buffer_get_extent (buffer));

              return buffer;
            }
          else
            {
              return g_object_ref (buffer);
            }
        }
      else
        {
          return g_object_ref (CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable));
        }
    }
  else
    {
      return g_object_ref (drawable->private->paint_buffer);
    }
}

void
clay_drawable_steal_buffer (ClayDrawable  *drawable,
                            ClayDrawable **src_drawable)
{
  GeglBuffer *buffer;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (src_drawable && CLAY_IS_DRAWABLE (*src_drawable));

  buffer = clay_drawable_get_buffer (*src_drawable);

  g_return_if_fail (buffer != NULL);

  g_object_ref (buffer);

  clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);

  g_object_unref (buffer);
  g_clear_object (src_drawable);
}

void
clay_drawable_set_format (ClayDrawable *drawable,
                          const Babl   *format,
                          gboolean      copy_buffer,
                          gboolean      push_undo)
{
  ClayItem   *item;
  GeglBuffer *buffer;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (format != NULL);
  g_return_if_fail (format != clay_drawable_get_format (drawable));
  g_return_if_fail (clay_babl_format_get_base_type (format) ==
                    clay_drawable_get_base_type (drawable));
  g_return_if_fail (clay_babl_format_get_component_type (format) ==
                    clay_drawable_get_component_type (drawable));
  g_return_if_fail (babl_format_has_alpha (format) ==
                    clay_drawable_has_alpha (drawable));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  item = CLAY_ITEM (drawable);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_drawable_format (clay_item_get_image (item),
                                          NULL, drawable);

  buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 clay_item_get_width  (item),
                                                 clay_item_get_height (item)),
                                 format, drawable);

  if (copy_buffer)
    {
      gegl_buffer_set_format (buffer, clay_drawable_get_format (drawable));

      clay_gegl_buffer_copy (clay_drawable_get_buffer (drawable),
                             NULL, GEGL_ABYSS_NONE,
                             buffer, NULL);

      gegl_buffer_set_format (buffer, NULL);
    }

  clay_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);
}

GeglNode *
clay_drawable_get_source_node (ClayDrawable *drawable)
{
  GeglNode *input;
  GeglNode *source;
  GeglNode *filter;
  GeglNode *output;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->source_node)
    return drawable->private->source_node;

  drawable->private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (drawable->private->source_node, "input");

  source = CLAY_DRAWABLE_GET_CLASS (drawable)->get_source_node (drawable);

  gegl_node_add_child (drawable->private->source_node, source);

  g_object_unref (source);

  if (gegl_node_has_pad (source, "input"))
    {
      gegl_node_link (input, source);
    }

  filter = clay_filter_stack_get_graph (CLAY_FILTER_STACK (drawable->private->filter_stack));

  gegl_node_add_child (drawable->private->source_node, filter);

  gegl_node_link (source, filter);

  output = gegl_node_get_output_proxy (drawable->private->source_node, "output");

  gegl_node_link (filter, output);

  if (clay_drawable_get_floating_sel (drawable))
    _clay_drawable_add_floating_sel_filter (drawable);

  return drawable->private->source_node;
}

GeglNode *
clay_drawable_get_mode_node (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  if (! drawable->private->mode_node)
    clay_filter_get_node (CLAY_FILTER (drawable));

  return drawable->private->mode_node;
}

GeglRectangle
clay_drawable_get_bounding_box (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  if (gegl_rectangle_is_empty (&drawable->private->bounding_box))
    clay_drawable_update_bounding_box (drawable);

  return drawable->private->bounding_box;
}

gboolean
clay_drawable_update_bounding_box (ClayDrawable *drawable)
{
  GeglRectangle bounding_box;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  bounding_box =
    CLAY_DRAWABLE_GET_CLASS (drawable)->get_bounding_box (drawable);

  if (! gegl_rectangle_equal (&bounding_box, &drawable->private->bounding_box))
    {
      GeglRectangle old_bounding_box = drawable->private->bounding_box;
      GeglRectangle diff_rects[4];
      gint          n_diff_rects;
      gint          i;

      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &old_bounding_box,
                                              &bounding_box);

      for (i = 0; i < n_diff_rects; i++)
        {
          clay_drawable_update (drawable,
                                diff_rects[i].x,
                                diff_rects[i].y,
                                diff_rects[i].width,
                                diff_rects[i].height);
        }

      drawable->private->bounding_box = bounding_box;

      g_signal_emit (drawable, clay_drawable_signals[BOUNDING_BOX_CHANGED], 0);

      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &bounding_box,
                                              &old_bounding_box);

      for (i = 0; i < n_diff_rects; i++)
        {
          clay_drawable_update (drawable,
                                diff_rects[i].x,
                                diff_rects[i].y,
                                diff_rects[i].width,
                                diff_rects[i].height);
        }

      return TRUE;
    }

  return FALSE;
}

void
clay_drawable_swap_pixels (ClayDrawable *drawable,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  CLAY_DRAWABLE_GET_CLASS (drawable)->swap_pixels (drawable, buffer, x, y);
}

void
clay_drawable_push_undo (ClayDrawable *drawable,
                         const gchar  *undo_desc,
                         GeglBuffer   *buffer,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height)
{
  ClayItem *item;

  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));
  g_return_if_fail (buffer == NULL || GEGL_IS_BUFFER (buffer));

  item = CLAY_ITEM (drawable);

  g_return_if_fail (clay_item_is_attached (item));

  if (! buffer &&
      ! clay_rectangle_intersect (x, y,
                                  width, height,
                                  0, 0,
                                  clay_item_get_width (item),
                                  clay_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      g_warning ("%s: tried to push empty region", G_STRFUNC);
      return;
    }

  CLAY_DRAWABLE_GET_CLASS (drawable)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);
}

void
clay_drawable_disable_resize_undo (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  drawable->private->push_resize_undo = FALSE;
}

void
clay_drawable_enable_resize_undo (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  drawable->private->push_resize_undo = TRUE;
}

const Babl *
clay_drawable_get_space (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return babl_format_get_space (clay_drawable_get_format (drawable));
}

const Babl *
clay_drawable_get_format (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return gegl_buffer_get_format (drawable->private->buffer);
}

const Babl *
clay_drawable_get_format_with_alpha (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return clay_image_get_format (clay_item_get_image (CLAY_ITEM (drawable)),
                                clay_drawable_get_base_type (drawable),
                                clay_drawable_get_precision (drawable),
                                TRUE,
                                clay_drawable_get_space (drawable));
}

const Babl *
clay_drawable_get_format_without_alpha (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  return clay_image_get_format (clay_item_get_image (CLAY_ITEM (drawable)),
                                clay_drawable_get_base_type (drawable),
                                clay_drawable_get_precision (drawable),
                                FALSE,
                                clay_drawable_get_space (drawable));
}

ClayTRCType
clay_drawable_get_trc (ClayDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return clay_babl_format_get_trc (format);
}

gboolean
clay_drawable_has_alpha (ClayDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return babl_format_has_alpha (format);
}

ClayImageBaseType
clay_drawable_get_base_type (ClayDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return clay_babl_format_get_base_type (format);
}

ClayComponentType
clay_drawable_get_component_type (ClayDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return clay_babl_format_get_component_type (format);
}

ClayPrecision
clay_drawable_get_precision (ClayDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return clay_babl_format_get_precision (format);
}

gboolean
clay_drawable_is_rgb (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  return (clay_drawable_get_base_type (drawable) == CLAY_RGB);
}

gboolean
clay_drawable_is_gray (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  return (clay_drawable_get_base_type (drawable) == CLAY_GRAY);
}

gboolean
clay_drawable_is_indexed (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  return (clay_drawable_get_base_type (drawable) == CLAY_INDEXED);
}

const Babl *
clay_drawable_get_component_format (ClayDrawable    *drawable,
                                    ClayChannelType  channel)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  switch (channel)
    {
    case CLAY_CHANNEL_RED:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_drawable_get_precision (drawable),
                                         RED);

    case CLAY_CHANNEL_GREEN:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_drawable_get_precision (drawable),
                                         GREEN);

    case CLAY_CHANNEL_BLUE:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_drawable_get_precision (drawable),
                                         BLUE);

    case CLAY_CHANNEL_ALPHA:
      return clay_babl_component_format (CLAY_RGB,
                                         clay_drawable_get_precision (drawable),
                                         ALPHA);

    case CLAY_CHANNEL_GRAY:
      return clay_babl_component_format (CLAY_GRAY,
                                         clay_drawable_get_precision (drawable),
                                         GRAY);

    case CLAY_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
clay_drawable_get_component_index (ClayDrawable    *drawable,
                                   ClayChannelType  channel)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), -1);

  switch (channel)
    {
    case CLAY_CHANNEL_RED:     return RED;
    case CLAY_CHANNEL_GREEN:   return GREEN;
    case CLAY_CHANNEL_BLUE:    return BLUE;
    case CLAY_CHANNEL_GRAY:    return GRAY;
    case CLAY_CHANNEL_INDEXED: return INDEXED;
    case CLAY_CHANNEL_ALPHA:
      switch (clay_drawable_get_base_type (drawable))
        {
        case CLAY_RGB:     return ALPHA;
        case CLAY_GRAY:    return ALPHA_G;
        case CLAY_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

void
clay_drawable_start_paint (ClayDrawable *drawable)
{
  g_return_if_fail (CLAY_IS_DRAWABLE (drawable));

  if (drawable->private->paint_count == 0)
    {
      GeglBuffer *buffer = clay_drawable_get_buffer (drawable);

      g_return_if_fail (buffer != NULL);
      g_return_if_fail (drawable->private->paint_buffer == NULL);
      g_return_if_fail (drawable->private->paint_copy_region == NULL);
      g_return_if_fail (drawable->private->paint_update_region == NULL);

      drawable->private->paint_buffer = clay_gegl_buffer_dup (buffer, NULL);
    }

  drawable->private->paint_count++;
}

gboolean
clay_drawable_end_paint (ClayDrawable *drawable)
{
  gboolean result = FALSE;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_count == 1)
    {
      result = clay_drawable_flush_paint (drawable);

      g_clear_object (&drawable->private->paint_buffer);
    }

  drawable->private->paint_count--;

  /* Refresh filters after painting */
  if (clay_drawable_has_visible_filters (drawable) &&
      drawable->private->paint_count == 0)
    {
      clay_drawable_update (drawable, 0, 0, -1, -1);
    }

  return result;
}

gboolean
clay_drawable_flush_paint (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_copy_region)
    {
      GeglBuffer *buffer;
      gint        n_rects;
      gint        i;

      buffer = CLAY_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);

      g_return_val_if_fail (buffer != NULL, FALSE);
      g_return_val_if_fail (drawable->private->paint_buffer != NULL, FALSE);

      n_rects = cairo_region_num_rectangles (
        drawable->private->paint_copy_region);

      for (i = 0; i < n_rects; i++)
        {
          GeglRectangle rect;

          cairo_region_get_rectangle (drawable->private->paint_copy_region,
                                      i, (cairo_rectangle_int_t *) &rect);

          clay_gegl_buffer_copy (
            drawable->private->paint_buffer, &rect, GEGL_ABYSS_NONE,
            buffer, NULL);
        }

      g_clear_pointer (&drawable->private->paint_copy_region,
                       cairo_region_destroy);

      n_rects = cairo_region_num_rectangles (
        drawable->private->paint_update_region);

      for (i = 0; i < n_rects; i++)
        {
          GeglRectangle rect;

          cairo_region_get_rectangle (drawable->private->paint_update_region,
                                      i, (cairo_rectangle_int_t *) &rect);

          g_signal_emit (drawable, clay_drawable_signals[UPDATE], 0,
                         rect.x, rect.y, rect.width, rect.height);
        }

      g_clear_pointer (&drawable->private->paint_update_region,
                       cairo_region_destroy);

      return TRUE;
    }

  return FALSE;
}

gboolean
clay_drawable_is_painting (ClayDrawable *drawable)
{
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), FALSE);

  return drawable->private->paint_count > 0;
}

gchar *
clay_drawable_get_new_cache_path (ClayDrawable *drawable)
{
  static gint  count = 0;
  gchar       *path  = NULL;
  ClayImage   *image;
  const gchar *folder;
  gchar       *filename;

  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);

  image = clay_item_get_image (CLAY_ITEM (drawable));

  g_return_val_if_fail (image != NULL, NULL);

  folder   = clay_image_get_buffers_folder (image);
  filename = g_strdup_printf ("%s-%d-%d.buffer",
                              g_type_name (G_TYPE_FROM_INSTANCE (drawable)),
                              clay_item_get_id (CLAY_ITEM (drawable)), count++);
  path     = g_build_filename (folder, filename, NULL);

  g_free (filename);

  return path;
}
