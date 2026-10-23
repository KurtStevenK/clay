/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "claydrawable-filters.h"
#include "claygrouplayer.h"
#include "claygrouplayerundo.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "claylayerstack.h"
#include "claylinklayer.h"
#include "clayobjectqueue.h"
#include "claypickable.h"
#include "clayprogress.h"
#include "clayprojectable.h"
#include "clayprojection.h"
#include "claysavable.h"
#include "claysavable-load.h"

#include "clay-intl.h"


typedef struct _ClayGroupLayerPrivate ClayGroupLayerPrivate;

struct _ClayGroupLayerPrivate
{
  ClayContainer  *children;
  ClayProjection *projection;
  GeglNode       *source_node;
  GeglNode       *parent_source_node;
  GeglNode       *graph;
  GeglNode       *offset_node;
  GeglRectangle   bounding_box;
  gint            suspend_resize;
  gint            suspend_mask;
  GeglBuffer     *suspended_mask_buffer;
  GeglRectangle   suspended_mask_bounds;
  gint            direct_update;
  gint            transforming;
  gboolean        expanded;
  gboolean        pass_through;

  /*  hackish temp states to make the projection/tiles stuff work  */
  const Babl     *convert_format;
  gboolean        reallocate_projection;
};

#define GET_PRIVATE(item) \
  ((ClayGroupLayerPrivate *) clay_group_layer_get_instance_private ((ClayGroupLayer *) (item)))


static void            clay_projectable_iface_init   (ClayProjectableInterface  *iface);
static void            clay_pickable_iface_init      (ClayPickableInterface     *iface);
static void            clay_savable_iface_init       (ClaySavableInterface      *iface);

static void            clay_group_layer_finalize     (GObject         *object);
static void            clay_group_layer_set_property (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void            clay_group_layer_get_property (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);

static gint64          clay_group_layer_get_memsize  (ClayObject      *object,
                                                      gint64          *gui_size);

static void        clay_group_layer_ancestry_changed (ClayViewable    *viewable);
static gboolean        clay_group_layer_get_size     (ClayViewable    *viewable,
                                                      gint            *width,
                                                      gint            *height);
static ClayContainer * clay_group_layer_get_children (ClayViewable    *viewable);
static gboolean        clay_group_layer_get_expanded (ClayViewable    *viewable);
static void            clay_group_layer_set_expanded (ClayViewable    *viewable,
                                                      gboolean         expanded);

static gboolean   clay_group_layer_is_content_locked (ClayItem        *item,
                                                      ClayItem       **locked_item,
                                                      gboolean         check_children);
static gboolean  clay_group_layer_is_position_locked (ClayItem        *item,
                                                      ClayItem       **locked_item,
                                                      gboolean         check_children);
static ClayItem      * clay_group_layer_duplicate    (ClayItem        *item,
                                                      GType            new_type);
static void            clay_group_layer_convert      (ClayItem        *item,
                                                      ClayImage       *dest_image,
                                                      GType            old_type);
static void         clay_group_layer_start_transform (ClayItem        *item,
                                                      gboolean         push_undo);
static void           clay_group_layer_end_transform (ClayItem        *item,
                                                      gboolean         push_undo);
static void            clay_group_layer_resize       (ClayItem        *item,
                                                      ClayContext     *context,
                                                      ClayFillType     fill_type,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             offset_x,
                                                      gint             offset_y);
static ClayTransformResize
                       clay_group_layer_get_clip     (ClayItem        *item,
                                                      ClayTransformResize clip_result);

static gint64      clay_group_layer_estimate_memsize (ClayDrawable      *drawable,
                                                      ClayComponentType  component_type,
                                                      gint               width,
                                                      gint               height);
static void            clay_group_layer_update_all   (ClayDrawable      *drawable);

static void            clay_group_layer_translate    (ClayLayer       *layer,
                                                      gint             offset_x,
                                                      gint             offset_y);
static void            clay_group_layer_scale        (ClayLayer       *layer,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             new_offset_x,
                                                      gint             new_offset_y,
                                                      ClayInterpolationType  interp_type,
                                                      ClayProgress    *progress);
static void            clay_group_layer_flip         (ClayLayer       *layer,
                                                      ClayContext     *context,
                                                      ClayOrientationType flip_type,
                                                      gdouble          axis,
                                                      gboolean         clip_result);
static void            clay_group_layer_rotate       (ClayLayer       *layer,
                                                      ClayContext     *context,
                                                      ClayRotationType rotate_type,
                                                      gdouble          center_x,
                                                      gdouble          center_y,
                                                      gboolean         clip_result);
static void            clay_group_layer_transform    (ClayLayer       *layer,
                                                      ClayContext     *context,
                                                      const ClayMatrix3 *matrix,
                                                      ClayTransformDirection direction,
                                                      ClayInterpolationType  interpolation_type,
                                                      ClayTransformResize clip_result,
                                                      ClayProgress      *progress,
                                                      gboolean           push_undo);
static void            clay_group_layer_convert_type (ClayLayer         *layer,
                                                      ClayImage         *dest_image,
                                                      const Babl        *new_format,
                                                      ClayColorProfile  *src_profile,
                                                      ClayColorProfile  *dest_profile,
                                                      GeglDitherMethod   layer_dither_type,
                                                      GeglDitherMethod   mask_dither_type,
                                                      gboolean           push_undo,
                                                      ClayProgress      *progress);
static GeglNode   * clay_group_layer_get_source_node (ClayDrawable      *drawable);

static void         clay_group_layer_filters_changed (ClayDrawable      *drawable);

static void         clay_group_layer_opacity_changed (ClayLayer         *layer);
static void  clay_group_layer_effective_mode_changed (ClayLayer         *layer);
static void
          clay_group_layer_excludes_backdrop_changed (ClayLayer         *layer);
static GeglRectangle
                   clay_group_layer_get_bounding_box (ClayLayer         *layer);
static void      clay_group_layer_get_effective_mode (ClayLayer         *layer,
                                                      ClayLayerMode          *mode,
                                                      ClayLayerColorSpace    *blend_space,
                                                      ClayLayerColorSpace    *composite_space,
                                                      ClayLayerCompositeMode *composite_mode);
static gboolean
              clay_group_layer_get_excludes_backdrop (ClayLayer         *layer);

static const Babl    * clay_group_layer_get_format   (ClayProjectable *projectable);
static GeglRectangle
       clay_group_layer_projectable_get_bounding_box (ClayProjectable *projectable);
static GeglNode      * clay_group_layer_get_graph    (ClayProjectable *projectable);
static void            clay_group_layer_begin_render (ClayProjectable *projectable);
static void            clay_group_layer_end_render   (ClayProjectable *projectable);

static void          clay_group_layer_pickable_flush (ClayPickable    *pickable);
static gdouble       clay_group_layer_get_opacity_at (ClayPickable    *pickable,
                                                      gint             x,
                                                      gint             y);

static void          clay_group_layer_save           (ClaySavable     *savable,
                                                      ClaySaveState   *state);
static gboolean      clay_group_layer_load_enter     (ClayLoadState   *state,
                                                      const gchar    **attribute_names,
                                                      const gchar    **attribute_values,
                                                      gpointer         user_data,
                                                      GError         **error);


static void            clay_group_layer_child_add    (ClayContainer   *container,
                                                      ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void            clay_group_layer_child_remove (ClayContainer   *container,
                                                      ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void            clay_group_layer_child_move   (ClayLayer       *child,
                                                      GParamSpec      *pspec,
                                                      ClayGroupLayer  *group);
static void            clay_group_layer_child_resize (ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void    clay_group_layer_child_active_changed (ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void
       clay_group_layer_child_effective_mode_changed (ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void
    clay_group_layer_child_excludes_backdrop_changed (ClayLayer       *child,
                                                      ClayGroupLayer  *group);
static void clay_group_layer_child_filters_changed   (ClayLayer       *child,
                                                      ClayGroupLayer  *group);

static void            clay_group_layer_flush        (ClayGroupLayer  *group);
static void            clay_group_layer_update       (ClayGroupLayer  *group);
static void            clay_group_layer_update_size  (ClayGroupLayer  *group);
static void        clay_group_layer_update_mask_size (ClayGroupLayer  *group);
static void      clay_group_layer_update_source_node (ClayGroupLayer  *group);
static void        clay_group_layer_update_mode_node (ClayGroupLayer  *group);

static void            clay_group_layer_stack_update (ClayDrawableStack *stack,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      ClayGroupLayer    *group);
static void            clay_group_layer_proj_update  (ClayProjection    *proj,
                                                      gboolean           now,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      ClayGroupLayer    *group);

static gboolean        clay_group_layer_enter_layers (ClayLoadState      *state,
                                                      const gchar       **attribute_names,
                                                      const gchar       **attribute_values,
                                                      gpointer            user_data,
                                                      GError            **error);


G_DEFINE_TYPE_WITH_CODE (ClayGroupLayer, clay_group_layer, CLAY_TYPE_LAYER,
                         G_ADD_PRIVATE (ClayGroupLayer)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PROJECTABLE,
                                                clay_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_pickable_iface_init)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_savable_iface_init))


#define parent_class clay_group_layer_parent_class

static ClaySavableInterface *parent_savable_iface = NULL;


/* disable pass-through groups strength-reduction to normal groups.
 * see clay_group_layer_get_effective_mode().
 */
static gboolean no_pass_through_strength_reduction = FALSE;


static void
clay_group_layer_class_init (ClayGroupLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);
  ClayItemClass     *item_class        = CLAY_ITEM_CLASS (klass);
  ClayDrawableClass *drawable_class    = CLAY_DRAWABLE_CLASS (klass);
  ClayLayerClass    *layer_class       = CLAY_LAYER_CLASS (klass);

  object_class->set_property             = clay_group_layer_set_property;
  object_class->get_property             = clay_group_layer_get_property;
  object_class->finalize                 = clay_group_layer_finalize;

  clay_object_class->get_memsize         = clay_group_layer_get_memsize;

  viewable_class->default_icon_name      = "clay-group-layer";
  viewable_class->default_name           = _("Layer Group");
  viewable_class->ancestry_changed       = clay_group_layer_ancestry_changed;
  viewable_class->get_size               = clay_group_layer_get_size;
  viewable_class->get_children           = clay_group_layer_get_children;
  viewable_class->set_expanded           = clay_group_layer_set_expanded;
  viewable_class->get_expanded           = clay_group_layer_get_expanded;

  item_class->is_content_locked          = clay_group_layer_is_content_locked;
  item_class->is_position_locked         = clay_group_layer_is_position_locked;
  item_class->duplicate                  = clay_group_layer_duplicate;
  item_class->convert                    = clay_group_layer_convert;
  item_class->start_transform            = clay_group_layer_start_transform;
  item_class->end_transform              = clay_group_layer_end_transform;
  item_class->resize                     = clay_group_layer_resize;
  item_class->get_clip                   = clay_group_layer_get_clip;

  item_class->rename_desc                = C_("undo-type", "Rename Layer Group");
  item_class->translate_desc             = C_("undo-type", "Move Layer Group");
  item_class->scale_desc                 = C_("undo-type", "Scale Layer Group");
  item_class->resize_desc                = C_("undo-type", "Resize Layer Group");
  item_class->flip_desc                  = C_("undo-type", "Flip Layer Group");
  item_class->rotate_desc                = C_("undo-type", "Rotate Layer Group");
  item_class->transform_desc             = C_("undo-type", "Transform Layer Group");

  drawable_class->estimate_memsize       = clay_group_layer_estimate_memsize;
  drawable_class->update_all             = clay_group_layer_update_all;
  drawable_class->get_source_node        = clay_group_layer_get_source_node;
  drawable_class->filters_changed        = clay_group_layer_filters_changed;

  layer_class->opacity_changed           = clay_group_layer_opacity_changed;
  layer_class->effective_mode_changed    = clay_group_layer_effective_mode_changed;
  layer_class->excludes_backdrop_changed = clay_group_layer_excludes_backdrop_changed;
  layer_class->translate                 = clay_group_layer_translate;
  layer_class->scale                     = clay_group_layer_scale;
  layer_class->flip                      = clay_group_layer_flip;
  layer_class->rotate                    = clay_group_layer_rotate;
  layer_class->transform                 = clay_group_layer_transform;
  layer_class->convert_type              = clay_group_layer_convert_type;
  layer_class->get_bounding_box          = clay_group_layer_get_bounding_box;
  layer_class->get_effective_mode        = clay_group_layer_get_effective_mode;
  layer_class->get_excludes_backdrop     = clay_group_layer_get_excludes_backdrop;

  if (g_getenv ("CLAY_NO_PASS_THROUGH_STRENGTH_REDUCTION"))
    no_pass_through_strength_reduction = TRUE;
}

static void
clay_projectable_iface_init (ClayProjectableInterface *iface)
{
  iface->get_image          = (ClayImage * (*) (ClayProjectable *)) clay_item_get_image;
  iface->get_format         = clay_group_layer_get_format;
  iface->get_offset         = (void (*) (ClayProjectable*, gint*, gint*)) clay_item_get_offset;
  iface->get_bounding_box   = clay_group_layer_projectable_get_bounding_box;
  iface->get_graph          = clay_group_layer_get_graph;
  iface->begin_render       = clay_group_layer_begin_render;
  iface->end_render         = clay_group_layer_end_render;
  iface->invalidate_preview = (void (*) (ClayProjectable*)) clay_viewable_invalidate_preview;
}

static void
clay_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->flush          = clay_group_layer_pickable_flush;
  iface->get_opacity_at = clay_group_layer_get_opacity_at;
}

static void
clay_savable_iface_init (ClaySavableInterface *iface)
{
  parent_savable_iface = g_type_interface_peek_parent (iface);

  iface->tag        = "group-layer";
  iface->save       = clay_group_layer_save;
  iface->load_enter = clay_group_layer_load_enter;
}

static void
clay_group_layer_init (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);

  private->children = clay_layer_stack_new (CLAY_TYPE_LAYER);
  private->expanded = TRUE;

  g_signal_connect (private->children, "add",
                    G_CALLBACK (clay_group_layer_child_add),
                    group);
  g_signal_connect (private->children, "remove",
                    G_CALLBACK (clay_group_layer_child_remove),
                    group);

  clay_container_add_handler (private->children, "notify::offset-x",
                              G_CALLBACK (clay_group_layer_child_move),
                              group);
  clay_container_add_handler (private->children, "notify::offset-y",
                              G_CALLBACK (clay_group_layer_child_move),
                              group);
  clay_container_add_handler (private->children, "size-changed",
                              G_CALLBACK (clay_group_layer_child_resize),
                              group);
  clay_container_add_handler (private->children, "bounding-box-changed",
                              G_CALLBACK (clay_group_layer_child_resize),
                              group);
  clay_container_add_handler (private->children, "active-changed",
                              G_CALLBACK (clay_group_layer_child_active_changed),
                              group);
  clay_container_add_handler (private->children, "effective-mode-changed",
                              G_CALLBACK (clay_group_layer_child_effective_mode_changed),
                              group);
  clay_container_add_handler (private->children, "excludes-backdrop-changed",
                              G_CALLBACK (clay_group_layer_child_excludes_backdrop_changed),
                              group);
  clay_container_add_handler (private->children, "filters-changed",
                              G_CALLBACK (clay_group_layer_child_filters_changed),
                              group);


  g_signal_connect (private->children, "update",
                    G_CALLBACK (clay_group_layer_stack_update),
                    group);

  private->projection = clay_projection_new (CLAY_PROJECTABLE (group));
  clay_projection_set_priority (private->projection, 1);

  g_signal_connect (private->projection, "update",
                    G_CALLBACK (clay_group_layer_proj_update),
                    group);
}

static void
clay_group_layer_finalize (GObject *object)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (object);

  if (private->children)
    {
      g_signal_handlers_disconnect_by_func (private->children,
                                            clay_group_layer_child_add,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            clay_group_layer_child_remove,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            clay_group_layer_stack_update,
                                            object);

      /* this is particularly important to avoid reallocating the projection
       * in response to a "bounding-box-changed" signal, which can be emitted
       * during layer destruction.  see issue #4584.
       */
      clay_container_remove_handlers_by_data (private->children, object);

      g_clear_object (&private->children);
    }

  g_clear_object (&private->projection);
  g_clear_object (&private->source_node);
  g_clear_object (&private->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_group_layer_set_property (GObject      *object,
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
clay_group_layer_get_property (GObject    *object,
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
clay_group_layer_get_memsize (ClayObject *object,
                              gint64     *gui_size)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (object);
  gint64                 memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->children), gui_size);
  memsize += clay_object_get_memsize (CLAY_OBJECT (private->projection), gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_group_layer_ancestry_changed (ClayViewable *viewable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (viewable);

  clay_projection_set_priority (private->projection,
                                clay_viewable_get_depth (viewable) + 1);

  CLAY_VIEWABLE_CLASS (parent_class)->ancestry_changed (viewable);
}

static gboolean
clay_group_layer_get_size (ClayViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (viewable);

  /*  return the size only if there are children ...  */
  if (! clay_container_is_empty (private->children))
    {
      return CLAY_VIEWABLE_CLASS (parent_class)->get_size (viewable,
                                                           width, height);
    }

  /*  ... otherwise, return "no content"  */
  return FALSE;
}

static ClayContainer *
clay_group_layer_get_children (ClayViewable *viewable)
{
  return GET_PRIVATE (viewable)->children;
}

static gboolean
clay_group_layer_get_expanded (ClayViewable *viewable)
{
  ClayGroupLayer *group = CLAY_GROUP_LAYER (viewable);

  return GET_PRIVATE (group)->expanded;
}

static void
clay_group_layer_set_expanded (ClayViewable *viewable,
                               gboolean      expanded)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (viewable);

  if (private->expanded != expanded)
    {
      private->expanded = expanded;

      clay_viewable_expanded_changed (viewable);
    }
}

static gboolean
clay_group_layer_is_content_locked (ClayItem  *item,
                                    ClayItem **locked_item,
                                    gboolean   check_children)
{
  /* A content-locked child locks the group too. */
  if (check_children)
    {
      ClayGroupLayerPrivate *private = GET_PRIVATE (item);
      GList                 *list;

      for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          ClayItem *child = list->data;

          if (clay_item_get_lock_content (child))
            {
              if (locked_item)
                *locked_item = child;

              return TRUE;
            }
          else if (CLAY_IS_GROUP_LAYER (child) &&
                   clay_group_layer_is_content_locked (child,
                                                       locked_item,
                                                       TRUE))
            {
              return TRUE;
            }
        }
    }

  /* A locked parent locks the group too! Which is handled by parent
   * implementation of the method.
   */
  return CLAY_ITEM_CLASS (parent_class)->is_content_locked (item,
                                                            locked_item,
                                                            FALSE);
}

static gboolean
clay_group_layer_is_position_locked (ClayItem  *item,
                                     ClayItem **locked_item,
                                     gboolean   check_children)
{
  /* A position-locked child locks the group too. */
  if (check_children)
    {
      ClayGroupLayerPrivate *private = GET_PRIVATE (item);
      GList                 *list;

      for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          ClayItem *child = list->data;

          if (clay_item_get_lock_position (child))
            {
              if (locked_item)
                *locked_item = child;

              return TRUE;
            }
          else if (CLAY_IS_GROUP_LAYER (child) &&
                   clay_group_layer_is_position_locked (child,
                                                        locked_item,
                                                        TRUE))
            {
              return TRUE;
            }
        }
    }

  /* A locked parent locks the group too! Which is handled by parent
   * implementation of the method.
   */
  return CLAY_ITEM_CLASS (parent_class)->is_position_locked (item,
                                                             locked_item,
                                                             FALSE);
}

static ClayItem *
clay_group_layer_duplicate (ClayItem *item,
                            GType     new_type)
{
  ClayItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_DRAWABLE), NULL);

  new_item = CLAY_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (CLAY_IS_GROUP_LAYER (new_item))
    {
      ClayGroupLayerPrivate *private     = GET_PRIVATE (item);
      ClayGroupLayer        *new_group   = CLAY_GROUP_LAYER (new_item);
      ClayGroupLayerPrivate *new_private = GET_PRIVATE (new_item);
      gint                   position    = 0;
      GList                 *list;

      clay_group_layer_suspend_resize (new_group, FALSE);

      for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          ClayItem      *child = list->data;
          ClayItem      *new_child;
          ClayLayerMask *mask;

          new_child = clay_item_duplicate (child, G_TYPE_FROM_INSTANCE (child));

          clay_object_set_name (CLAY_OBJECT (new_child),
                                clay_object_get_name (child));

          mask = clay_layer_get_mask (CLAY_LAYER (child));

          if (mask)
            {
              ClayLayerMask *new_mask;

              new_mask = clay_layer_get_mask (CLAY_LAYER (new_child));

              clay_object_set_name (CLAY_OBJECT (new_mask),
                                    clay_object_get_name (mask));
            }

          clay_viewable_set_parent (CLAY_VIEWABLE (new_child),
                                    CLAY_VIEWABLE (new_group));

          clay_container_insert (new_private->children,
                                 CLAY_OBJECT (new_child),
                                 position++);
        }

      /*  force the projection to reallocate itself  */
      GET_PRIVATE (new_group)->reallocate_projection = TRUE;

      clay_group_layer_resume_resize (new_group, FALSE);
    }

  return new_item;
}

static void
clay_group_layer_convert (ClayItem  *item,
                          ClayImage *dest_image,
                          GType      old_type)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem *child = list->data;

      CLAY_ITEM_GET_CLASS (child)->convert (child, dest_image,
                                            G_TYPE_FROM_INSTANCE (child));
    }

  CLAY_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
clay_group_layer_start_transform (ClayItem *item,
                             gboolean  push_undo)
{
  _clay_group_layer_start_transform (CLAY_GROUP_LAYER (item), push_undo);

  if (CLAY_ITEM_CLASS (parent_class)->start_transform)
    CLAY_ITEM_CLASS (parent_class)->start_transform (item, push_undo);
}

static void
clay_group_layer_end_transform (ClayItem *item,
                           gboolean  push_undo)
{
  if (CLAY_ITEM_CLASS (parent_class)->end_transform)
    CLAY_ITEM_CLASS (parent_class)->end_transform (item, push_undo);

  _clay_group_layer_end_transform (CLAY_GROUP_LAYER (item), push_undo);
}

static void
clay_group_layer_resize (ClayItem     *item,
                         ClayContext  *context,
                         ClayFillType  fill_type,
                         gint          new_width,
                         gint          new_height,
                         gint          offset_x,
                         gint          offset_y)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (item);
  ClayGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;
  gint                   x, y;

  /* we implement ClayItem::resize(), instead of ClayLayer::resize(), so that
   * ClayLayer doesn't resize the mask.  note that clay_item_resize() calls
   * clay_item_{start,end}_move(), and not clay_item_{start,end}_transform(),
   * so that mask resizing is handled by clay_group_layer_update_size().
   */

  x = clay_item_get_offset_x (item) - offset_x;
  y = clay_item_get_offset_y (item) - offset_y;

  clay_group_layer_suspend_resize (group, TRUE);

  list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));

  while (list)
    {
      ClayItem *child = list->data;
      gint      child_width;
      gint      child_height;
      gint      child_x;
      gint      child_y;

      list = g_list_next (list);

      if (clay_rectangle_intersect (x,
                                    y,
                                    new_width,
                                    new_height,
                                    clay_item_get_offset_x (child),
                                    clay_item_get_offset_y (child),
                                    clay_item_get_width  (child),
                                    clay_item_get_height (child),
                                    &child_x,
                                    &child_y,
                                    &child_width,
                                    &child_height))
        {
          gint child_offset_x = clay_item_get_offset_x (child) - child_x;
          gint child_offset_y = clay_item_get_offset_y (child) - child_y;

          clay_item_resize (child, context, fill_type,
                            child_width, child_height,
                            child_offset_x, child_offset_y);
        }
      else if (clay_item_is_attached (item))
        {
          clay_image_remove_layer (clay_item_get_image (item),
                                   CLAY_LAYER (child),
                                   TRUE, NULL);
        }
      else
        {
          clay_container_remove (private->children, CLAY_OBJECT (child));
        }
    }

  clay_group_layer_resume_resize (group, TRUE);
}

static ClayTransformResize
clay_group_layer_get_clip (ClayItem            *item,
                           ClayTransformResize  clip_result)
{
  /* TODO: add clipping support, by clipping all sublayers as a unit, instead
   * of individually.
   */
  return CLAY_TRANSFORM_RESIZE_ADJUST;
}

static gint64
clay_group_layer_estimate_memsize (ClayDrawable      *drawable,
                                   ClayComponentType  component_type,
                                   gint               width,
                                   gint               height)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;
  ClayImageBaseType      base_type;
  gint64                 memsize = 0;

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayDrawable *child = list->data;
      gint          child_width;
      gint          child_height;

      child_width  = (clay_item_get_width (CLAY_ITEM (child)) *
                      width /
                      clay_item_get_width (CLAY_ITEM (drawable)));
      child_height = (clay_item_get_height (CLAY_ITEM (child)) *
                      height /
                      clay_item_get_height (CLAY_ITEM (drawable)));

      memsize += clay_drawable_estimate_memsize (child,
                                                 component_type,
                                                 child_width,
                                                 child_height);
    }

  base_type = clay_drawable_get_base_type (drawable);

  memsize += clay_projection_estimate_memsize (base_type, component_type,
                                               width, height);

  return memsize +
         CLAY_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static void
clay_group_layer_update_all (ClayDrawable *drawable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;

  /*  redirect stack updates to the drawable, rather than to the projection  */
  private->direct_update++;

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayFilter *child = list->data;

      if (clay_filter_get_active (child))
        clay_drawable_update_all (CLAY_DRAWABLE (child));
    }

  /*  redirect stack updates back to the projection  */
  private->direct_update--;
}

static void
clay_group_layer_translate (ClayLayer *layer,
                            gint       offset_x,
                            gint       offset_y)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  gint                   x, y;
  GList                 *list;

  /*  don't use clay_group_layer_suspend_resize(), but rather increment
   *  private->suspend_resize directly, since we're translating the group layer
   *  here, rather than relying on clay_group_layer_update_size() to do it.
   */
  private->suspend_resize++;

  /*  redirect stack updates to the drawable, rather than to the projection  */
  private->direct_update++;

  /*  translate the child layers *before* updating the group's offset, so that,
   *  if this is a nested group, the parent's bounds still reflect the original
   *  layer positions.  This prevents the original area of the child layers,
   *  which is updated as part of their translation, from being clipped to the
   *  post-translation parent bounds (see issue #3484).  The new area of the
   *  child layers, which is likewise updated as part of translation, *may* get
   *  clipped to the old parent bounds, but the corresponding region will be
   *  updated anyway when the parent is resized, once we update the group's
   *  offset.
   */
  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem *child = list->data;

      /*  don't push an undo here because undo will call us again  */
      clay_item_translate (child, offset_x, offset_y, FALSE);
    }

  clay_item_get_offset (CLAY_ITEM (group), &x, &y);

  x += offset_x;
  y += offset_y;

  /*  update the offset node  */
  if (private->offset_node)
    gegl_node_set (private->offset_node,
                   "x", (gdouble) -x,
                   "y", (gdouble) -y,
                   NULL);

  /*  invalidate the selection boundary because of a layer modification  */
  clay_drawable_invalidate_boundary (CLAY_DRAWABLE (layer));

  /*  update the group layer offset  */
  clay_item_set_offset (CLAY_ITEM (group), x, y);

  /*  redirect stack updates back to the projection  */
  private->direct_update--;

  /*  don't use clay_group_layer_resume_resize(), but rather decrement
   *  private->suspend_resize directly, so that clay_group_layer_update_size()
   *  isn't called.
   */
  private->suspend_resize--;
}

static void
clay_group_layer_scale (ClayLayer             *layer,
                        gint                   new_width,
                        gint                   new_height,
                        gint                   new_offset_x,
                        gint                   new_offset_y,
                        ClayInterpolationType  interpolation_type,
                        ClayProgress          *progress)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  ClayItem              *item    = CLAY_ITEM (layer);
  ClayObjectQueue       *queue   = NULL;
  GList                 *list;
  gdouble                width_factor;
  gdouble                height_factor;
  gint                   old_offset_x;
  gint                   old_offset_y;

  width_factor  = (gdouble) new_width  / (gdouble) clay_item_get_width  (item);
  height_factor = (gdouble) new_height / (gdouble) clay_item_get_height (item);

  old_offset_x = clay_item_get_offset_x (item);
  old_offset_y = clay_item_get_offset_y (item);

  if (progress)
    {
      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      clay_object_queue_push_container (queue, private->children);
    }

  clay_group_layer_suspend_resize (group, TRUE);

  list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));

  while (list)
    {
      ClayItem *child = list->data;

      list = g_list_next (list);

      if (queue)
        clay_object_queue_pop (queue);

      if (! clay_item_scale_by_factors_with_origin (child,
                                                    width_factor, height_factor,
                                                    old_offset_x, old_offset_y,
                                                    new_offset_x, new_offset_y,
                                                    interpolation_type,
                                                    progress))
        {
          /* new width or height are 0; remove item */
          if (clay_item_is_attached (item))
            {
              clay_image_remove_layer (clay_item_get_image (item),
                                       CLAY_LAYER (child),
                                       TRUE, NULL);
            }
          else
            {
              clay_container_remove (private->children, CLAY_OBJECT (child));
            }
        }
    }

  clay_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static void
clay_group_layer_flip (ClayLayer           *layer,
                       ClayContext         *context,
                       ClayOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  clay_group_layer_suspend_resize (group, TRUE);

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem *child = list->data;

      clay_item_flip (child, context,
                      flip_type, axis, clip_result);
    }

  clay_group_layer_resume_resize (group, TRUE);
}

static void
clay_group_layer_rotate (ClayLayer        *layer,
                         ClayContext      *context,
                         ClayRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  clay_group_layer_suspend_resize (group, TRUE);

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem *child = list->data;

      clay_item_rotate (child, context,
                        rotate_type, center_x, center_y, clip_result);
    }

  clay_group_layer_resume_resize (group, TRUE);
}

static void
clay_group_layer_transform (ClayLayer              *layer,
                            ClayContext            *context,
                            const ClayMatrix3      *matrix,
                            ClayTransformDirection  direction,
                            ClayInterpolationType   interpolation_type,
                            ClayTransformResize     clip_result,
                            ClayProgress           *progress,
                            gboolean                push_undo)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  ClayObjectQueue       *queue   = NULL;
  GList                 *list;

  if (progress)
    {
      queue    = clay_object_queue_new (progress);
      progress = CLAY_PROGRESS (queue);

      clay_object_queue_push_container (queue, private->children);
    }

  clay_group_layer_suspend_resize (group, TRUE);

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem *child = list->data;

      if (queue)
        clay_object_queue_pop (queue);

      clay_item_transform (child, context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  clay_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static const Babl *
get_projection_format (ClayProjectable   *projectable,
                       ClayImageBaseType  base_type,
                       ClayPrecision      precision)
{
  ClayImage *image = clay_item_get_image (CLAY_ITEM (projectable));

  switch (base_type)
    {
    case CLAY_RGB:
    case CLAY_INDEXED:
      return clay_image_get_format (image, CLAY_RGB, precision, TRUE,
                                    clay_image_get_layer_space (image));

    case CLAY_GRAY:
      return clay_image_get_format (image, CLAY_GRAY, precision, TRUE,
                                    clay_image_get_layer_space (image));
    }

  g_return_val_if_reached (NULL);
}

static void
clay_group_layer_convert_type (ClayLayer        *layer,
                               ClayImage        *dest_image,
                               const Babl       *new_format,
                               ClayColorProfile *src_profile,
                               ClayColorProfile *dest_profile,
                               GeglDitherMethod  layer_dither_type,
                               GeglDitherMethod  mask_dither_type,
                               gboolean          push_undo,
                               ClayProgress     *progress)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);
  GeglBuffer            *buffer;

  if (push_undo)
    {
      ClayImage *image = clay_item_get_image (CLAY_ITEM (group));

      clay_image_undo_push_group_layer_convert (image, NULL, group);
    }

  /*  Need to temporarily set the projectable's format to the new
   *  values so the projection will create its tiles with the right
   *  depth
   */
  private->convert_format =
    get_projection_format (CLAY_PROJECTABLE (group),
                           clay_babl_format_get_base_type (new_format),
                           clay_babl_format_get_precision (new_format));
  clay_projectable_structure_changed (CLAY_PROJECTABLE (group));
  clay_group_layer_flush (group);

  buffer = clay_pickable_get_buffer (CLAY_PICKABLE (private->projection));

  clay_drawable_set_buffer_full (CLAY_DRAWABLE (group),
                                 FALSE, NULL,
                                 buffer, NULL,
                                 TRUE);

  /*  reset, the actual format is right now  */
  private->convert_format = NULL;
}

static GeglNode *
clay_group_layer_get_source_node (ClayDrawable *drawable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GeglNode              *input;

  g_warn_if_fail (private->source_node == NULL);

  private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (private->source_node, "input");

  private->parent_source_node =
    CLAY_DRAWABLE_CLASS (parent_class)->get_source_node (drawable);

  gegl_node_add_child (private->source_node, private->parent_source_node);

  g_object_unref (private->parent_source_node);

  if (gegl_node_has_pad (private->parent_source_node, "input"))
    {
      gegl_node_link (input, private->parent_source_node);
    }

  /* make sure we have a graph */
  (void) clay_group_layer_get_graph (CLAY_PROJECTABLE (drawable));

  gegl_node_add_child (private->source_node, private->graph);

  clay_group_layer_update_source_node (CLAY_GROUP_LAYER (drawable));

  return g_object_ref (private->source_node);
}

static void
clay_group_layer_filters_changed (ClayDrawable *drawable)
{
  clay_layer_update_effective_mode (CLAY_LAYER (drawable));

  if (CLAY_DRAWABLE_CLASS (parent_class)->filters_changed)
    CLAY_DRAWABLE_CLASS (parent_class)->filters_changed (drawable);
}

static void
clay_group_layer_opacity_changed (ClayLayer *layer)
{
  clay_layer_update_effective_mode (layer);

  if (CLAY_LAYER_CLASS (parent_class)->opacity_changed)
    CLAY_LAYER_CLASS (parent_class)->opacity_changed (layer);
}

static void
clay_group_layer_effective_mode_changed (ClayLayer *layer)
{
  ClayGroupLayer        *group               = CLAY_GROUP_LAYER (layer);
  ClayGroupLayerPrivate *private             = GET_PRIVATE (layer);
  ClayLayerMode          mode;
  gboolean               pass_through;
  gboolean               update_bounding_box = FALSE;

  clay_layer_get_effective_mode (layer, &mode, NULL, NULL, NULL);

  pass_through = (mode == CLAY_LAYER_MODE_PASS_THROUGH);

  if (pass_through != private->pass_through)
    {
      if (private->pass_through && ! pass_through)
        {
          /* when switching from pass-through mode to a non-pass-through mode,
           * flush the pickable in order to make sure the projection's buffer
           * gets properly invalidated synchronously, so that it can be used
           * as a source for the rest of the composition.
           */
          clay_pickable_flush (CLAY_PICKABLE (private->projection));
        }

      private->pass_through = pass_through;

      update_bounding_box = TRUE;
    }

  clay_group_layer_update_source_node (group);
  clay_group_layer_update_mode_node (group);

  if (update_bounding_box)
    clay_drawable_update_bounding_box (CLAY_DRAWABLE (group));

  if (CLAY_LAYER_CLASS (parent_class)->effective_mode_changed)
    CLAY_LAYER_CLASS (parent_class)->effective_mode_changed (layer);
}

static void
clay_group_layer_excludes_backdrop_changed (ClayLayer *layer)
{
  ClayGroupLayer *group = CLAY_GROUP_LAYER (layer);

  clay_group_layer_update_source_node (group);
  clay_group_layer_update_mode_node (group);

  if (CLAY_LAYER_CLASS (parent_class)->excludes_backdrop_changed)
    CLAY_LAYER_CLASS (parent_class)->excludes_backdrop_changed (layer);
}

static GeglRectangle
clay_group_layer_get_bounding_box (ClayLayer *layer)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);

  /* With #4634 were reported to us cases where the get_bounding_box()
   * was smaller than the calculated bounding box. But we also had the
   * opposite case, which is that the get_bounding_box() value was
   * bigger than the computed bounding_box (e.g. when having a
   * pass-through group layer with a single transparent layer, smaller
   * than the canvas: applying an effect only affects this layer,
   * whereas the actual render of the layer group contains the below
   * layers). Cf. #15004.
   *
   * XXX It feels like the parent's get_bounding_box() implementation
   * (which gets the source node bounding box) is bugged and should not
   * return what it does. This should likely be looked closer into.
   * For now, I get the union bounding box of both these results.
   *
   * Older explanation for #4634:
   * for pass-through groups, use the group's calculated bounding box, instead
   * of the source-node's bounding box, since we don't update the bounding box
   * on all events that may affect the latter, and since it includes the
   * bounding box of the backdrop.  this means we can't attach filters that may
   * affect the bounding box to a pass-through group (since their effect weon't
   * be reflected by the group's bounding box), but attaching filters to pass-
   * through groups makes little sense anyway.
   */
  if (private->pass_through)
    {
      GeglRectangle rect = CLAY_LAYER_CLASS (parent_class)->get_bounding_box (layer);

      gegl_rectangle_bounding_box (&rect, &private->bounding_box, &rect);

      return rect;
    }
  else
    {
      return CLAY_LAYER_CLASS (parent_class)->get_bounding_box (layer);
    }
}

static void
clay_group_layer_get_effective_mode (ClayLayer              *layer,
                                     ClayLayerMode          *mode,
                                     ClayLayerColorSpace    *blend_space,
                                     ClayLayerColorSpace    *composite_space,
                                     ClayLayerCompositeMode *composite_mode)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);

  /* try to strength-reduce pass-through groups to normal groups, which are
   * cheaper.
   */
  if (clay_layer_get_mode (layer) == CLAY_LAYER_MODE_PASS_THROUGH &&
      ! clay_drawable_has_visible_filters (CLAY_DRAWABLE (layer)) &&
      ! no_pass_through_strength_reduction)
    {
      /* we perform the strength-reduction if:
       *
       *   - the group has no active children;
       *
       *   or,
       *
       *     - the group has a single active child; or,
       *
       *     - the effective mode of all the active children is normal, their
       *       effective composite mode is UNION, and their effective blend and
       *       composite spaces are equal;
       *
       *   - and,
       *
       *     - the group's opacity is 100%, and it has no mask (or the mask
       *       isn't applied); or,
       *
       *     - the group's composite space equals the active children's
       *       composite space.
       *
       *     - none of the active children have any visible filter
       *
       */

      GList    *list;
      gboolean  reduce = TRUE;
      gboolean  first  = TRUE;

      *mode            = CLAY_LAYER_MODE_NORMAL;
      *blend_space     = clay_layer_get_real_blend_space (layer);
      *composite_space = clay_layer_get_real_composite_space (layer);
      *composite_mode  = clay_layer_get_real_composite_mode (layer);

      for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          ClayLayer *child = list->data;

          if (! clay_filter_get_active (CLAY_FILTER (child)))
            continue;

          if (clay_drawable_has_visible_filters (CLAY_DRAWABLE (child)))
            {
              reduce = FALSE;
              break;
            }

          if (first)
            {
              clay_layer_get_effective_mode (child,
                                             mode,
                                             blend_space,
                                             composite_space,
                                             composite_mode);

              if (*mode == CLAY_LAYER_MODE_NORMAL_LEGACY)
                *mode = CLAY_LAYER_MODE_NORMAL;

              first = FALSE;
            }
          else
            {
              ClayLayerMode          other_mode;
              ClayLayerColorSpace    other_blend_space;
              ClayLayerColorSpace    other_composite_space;
              ClayLayerCompositeMode other_composite_mode;

              if (*mode           != CLAY_LAYER_MODE_NORMAL ||
                  *composite_mode != CLAY_LAYER_COMPOSITE_UNION)
                {
                  reduce = FALSE;

                  break;
                }

              clay_layer_get_effective_mode (child,
                                             &other_mode,
                                             &other_blend_space,
                                             &other_composite_space,
                                             &other_composite_mode);

              if (other_mode == CLAY_LAYER_MODE_NORMAL_LEGACY)
                other_mode = CLAY_LAYER_MODE_NORMAL;

              if (other_mode            != *mode            ||
                  other_blend_space     != *blend_space     ||
                  other_composite_space != *composite_space ||
                  other_composite_mode  != *composite_mode)
                {
                  reduce = FALSE;

                  break;
                }
            }
        }

      if (reduce)
        {
          gboolean has_mask;

          has_mask = clay_layer_get_mask (layer) &&
                     clay_layer_get_apply_mask (layer);

          if (first                                                  ||
              (clay_layer_get_opacity (layer) == CLAY_OPACITY_OPAQUE &&
               ! has_mask)                                           ||
              *composite_space == clay_layer_get_real_composite_space (layer))
            {
              /* strength reduction succeeded! */
              return;
            }
        }
    }

  /* strength-reduction failed.  chain up. */
  CLAY_LAYER_CLASS (parent_class)->get_effective_mode (layer,
                                                       mode,
                                                       blend_space,
                                                       composite_space,
                                                       composite_mode);
}

static gboolean
clay_group_layer_get_excludes_backdrop (ClayLayer *layer)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (layer);

  if (private->pass_through)
    {
      GList *list;

      for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          ClayFilter *child = list->data;

          if (clay_filter_get_active (child) &&
              clay_layer_get_excludes_backdrop (CLAY_LAYER (child)))
            return TRUE;
        }

      return FALSE;
    }
  else
    return CLAY_LAYER_CLASS (parent_class)->get_excludes_backdrop (layer);
}

static const Babl *
clay_group_layer_get_format (ClayProjectable *projectable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (projectable);
  ClayImageBaseType      base_type;
  ClayPrecision          precision;

  if (private->convert_format)
    return private->convert_format;

  base_type = clay_drawable_get_base_type (CLAY_DRAWABLE (projectable));
  precision = clay_drawable_get_precision (CLAY_DRAWABLE (projectable));

  return get_projection_format (projectable, base_type, precision);
}

static GeglRectangle
clay_group_layer_projectable_get_bounding_box (ClayProjectable *projectable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (projectable);

  return private->bounding_box;
}

static GeglNode *
clay_group_layer_get_graph (ClayProjectable *projectable)
{
  ClayGroupLayer        *group   = CLAY_GROUP_LAYER (projectable);
  ClayGroupLayerPrivate *private = GET_PRIVATE (projectable);
  GeglNode              *input;
  GeglNode              *layers_node;
  GeglNode              *output;
  gint                   off_x;
  gint                   off_y;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  input = gegl_node_get_input_proxy (private->graph, "input");

  layers_node =
    clay_filter_stack_get_graph (CLAY_FILTER_STACK (private->children));

  gegl_node_add_child (private->graph, layers_node);

  gegl_node_link (input, layers_node);

  clay_item_get_offset (CLAY_ITEM (group), &off_x, &off_y);

  private->offset_node = gegl_node_new_child (private->graph,
                                              "operation", "gegl:translate",
                                              "x",         (gdouble) -off_x,
                                              "y",         (gdouble) -off_y,
                                              NULL);

  gegl_node_link (layers_node, private->offset_node);

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_link (private->offset_node, output);

  return private->graph;
}

static void
clay_group_layer_begin_render (ClayProjectable *projectable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (projectable);

  if (private->source_node == NULL)
    return;

  if (private->pass_through)
    gegl_node_disconnect (private->graph, "input");
}

static void
clay_group_layer_end_render (ClayProjectable *projectable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (projectable);

  if (private->source_node == NULL)
    return;

  if (private->pass_through)
    {
      GeglNode *input;

      input = gegl_node_get_input_proxy (private->source_node, "input");

      gegl_node_link (input, private->graph);
    }
}

static void
clay_group_layer_pickable_flush (ClayPickable *pickable)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (pickable);

  clay_pickable_flush (CLAY_PICKABLE (private->projection));
}

static gdouble
clay_group_layer_get_opacity_at (ClayPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  /* Only consider child layers as having content */

  return CLAY_OPACITY_TRANSPARENT;
}

static void
clay_group_layer_save (ClaySavable   *savable,
                       ClaySaveState *state)
{
  ClayGroupLayer *layer = CLAY_GROUP_LAYER (savable);

  clay_savable_print_element_start (state, "group-layer",
                                    "name", "%s", clay_object_get_name (CLAY_OBJECT (layer)),
                                    NULL);

  parent_savable_iface->save (savable, state);

  if (clay_viewable_get_children (CLAY_VIEWABLE (layer)) != NULL)
    {
      ClayContainer *stack = clay_viewable_get_children (CLAY_VIEWABLE (layer));
      GList         *child;

      clay_savable_print_element_start (state, "layers", NULL);

      child = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (stack));
      for (; child; child = child->next)
        clay_savable_save (CLAY_SAVABLE (child->data), state);

      clay_savable_print_element_end (state, "layers");
    }

  clay_savable_print_element_end (state, "group-layer");
}

static gboolean
clay_group_layer_load_enter (ClayLoadState  *state,
                             const gchar   **attribute_names,
                             const gchar   **attribute_values,
                             gpointer        user_data,
                             GError        **error)
{
  ClayLayer     *layer;
  const gchar   *name   = NULL;
  ClayLayer     *parent = NULL;
  ClayContainer *container;

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

  clay_savable_load_add_handlers (state, "layers",
                                  clay_group_layer_enter_layers,
                                  NULL, NULL, NULL);
  layer = clay_group_layer_new (state->image);

  if (name == NULL)
    g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                 "%s: missing layer group name.",
                 G_STRFUNC);
  else
    clay_object_set_name (CLAY_OBJECT (layer), name);

  /* Insert the group layer immediately on purpose, unlike other types
   * of layers, so that we can add children to it before the element
   * closes.
   */
  if (CLAY_IS_GROUP_LAYER (clay_savable_load_peek_active_object (state)))
    {
      parent    = CLAY_LAYER (clay_savable_load_peek_active_object (state));
      container = clay_viewable_get_children (CLAY_VIEWABLE (parent));
    }
  else
    {
      container = clay_image_get_layers (state->image);
    }

  clay_image_add_layer (state->image, layer, parent,
                        clay_container_get_n_children (container),
                        FALSE);

  clay_savable_load_push_active_object (state, G_OBJECT (layer));

  return TRUE;
}


/*  public functions  */

ClayLayer *
clay_group_layer_new (ClayImage *image)
{
  ClayGroupLayer *group;
  const Babl     *format;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);

  format = clay_image_get_layer_format (image, TRUE);

  group = CLAY_GROUP_LAYER (clay_drawable_new (CLAY_TYPE_GROUP_LAYER,
                                               image, NULL,
                                               0, 0, 1, 1,
                                               format));

  clay_layer_set_mode (CLAY_LAYER (group),
                       clay_image_get_default_new_layer_mode (image),
                       FALSE);

  return CLAY_LAYER (group);
}

ClayProjection *
clay_group_layer_get_projection (ClayGroupLayer *group)
{
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);

  return GET_PRIVATE (group)->projection;
}

void
clay_group_layer_suspend_resize (ClayGroupLayer *group,
                                 gboolean        push_undo)
{
  ClayItem *item;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  item = CLAY_ITEM (group);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_group_layer_suspend_resize (clay_item_get_image (item),
                                                     NULL, group);

  GET_PRIVATE (group)->suspend_resize++;
}

void
clay_group_layer_resume_resize (ClayGroupLayer *group,
                                gboolean        push_undo)
{
  ClayGroupLayerPrivate *private;
  ClayItem              *item;
  ClayItem              *mask = NULL;
  GeglBuffer            *mask_buffer;
  GeglRectangle          mask_bounds;
  ClayUndo              *undo;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_resize > 0);

  item = CLAY_ITEM (group);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      undo =
        clay_image_undo_push_group_layer_resume_resize (clay_item_get_image (item),
                                                        NULL, group);

      /* if there were any {suspend,resume}_mask() calls during the time the
       * group's size was suspended, the resume_mask() calls will not have seen
       * any changes to the mask, and will therefore won't restore the mask
       * during undo.  if the group's bounding box did change while resize was
       * suspended, and if there are no other {suspend,resume}_mask() blocks
       * that will see the resized mask, we have to restore the mask during the
       * resume_resize() undo.
       *
       * we ref the mask buffer here, and compare it to the mask buffer after
       * updating the size.
       */
      if (private->suspend_resize == 1 && private->suspend_mask == 0)
        {
          mask = CLAY_ITEM (clay_layer_get_mask (CLAY_LAYER (group)));

          if (mask)
            {
              mask_buffer =
                g_object_ref (clay_drawable_get_buffer (CLAY_DRAWABLE (mask)));

              mask_bounds.x      = clay_item_get_offset_x (mask);
              mask_bounds.y      = clay_item_get_offset_y (mask);
              mask_bounds.width  = clay_item_get_width    (mask);
              mask_bounds.height = clay_item_get_height   (mask);
            }
        }
    }

  private->suspend_resize--;

  if (private->suspend_resize == 0)
    {
      clay_group_layer_update_size (group);

      if (mask)
        {
          /* if the mask changed, make sure it's restored during undo, as per
           * the comment above.
           */
          if (clay_drawable_get_buffer (CLAY_DRAWABLE (mask)) != mask_buffer)
            {
              g_return_if_fail (undo != NULL);

              CLAY_GROUP_LAYER_UNDO (undo)->mask_buffer = mask_buffer;
              CLAY_GROUP_LAYER_UNDO (undo)->mask_bounds = mask_bounds;
            }
          else
            {
              g_object_unref (mask_buffer);
            }
        }
    }
}

void
clay_group_layer_suspend_mask (ClayGroupLayer *group,
                               gboolean        push_undo)
{
  ClayGroupLayerPrivate *private;
  ClayItem              *item;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = CLAY_ITEM (group);

  /* avoid pushing an undo step if this is a nested suspend_mask() call, since
   * the value of 'push_undo' in nested calls should be the same as that passed
   * to the outermost call, and only pushing an undo step for the outermost
   * call in this case is enough.  we can't support cases where the values of
   * 'push_undo' in nested calls are different in a meaningful way, and
   * avoiding undo steps for nested calls prevents us from storing multiple
   * references to the suspend mask buffer on the undo stack.  while storing
   * multiple references to the buffer doesn't waste any memory (since all the
   * references are to the same buffer), it does cause the undo stack memory-
   * usage estimation to overshoot, potentially resulting in undo steps being
   * dropped unnecessarily.
   */
  if (! clay_item_is_attached (item) || private->suspend_mask > 0)
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_group_layer_suspend_mask (clay_item_get_image (item),
                                                   NULL, group);

  if (private->suspend_mask == 0)
    {
      if (clay_layer_get_mask (CLAY_LAYER (group)))
        {
          ClayItem *mask = CLAY_ITEM (clay_layer_get_mask (CLAY_LAYER (group)));

          private->suspended_mask_buffer =
            g_object_ref (clay_drawable_get_buffer (CLAY_DRAWABLE (mask)));

          private->suspended_mask_bounds.x      = clay_item_get_offset_x (mask);
          private->suspended_mask_bounds.y      = clay_item_get_offset_y (mask);
          private->suspended_mask_bounds.width  = clay_item_get_width    (mask);
          private->suspended_mask_bounds.height = clay_item_get_height   (mask);
        }
      else
        {
          private->suspended_mask_buffer = NULL;
        }
    }

  private->suspend_mask++;
}

void
clay_group_layer_resume_mask (ClayGroupLayer *group,
                              gboolean        push_undo)
{
  ClayGroupLayerPrivate *private;
  ClayItem              *item;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_mask > 0);

  item = CLAY_ITEM (group);

  /* avoid pushing an undo step if this is a nested resume_mask() call.  see
   * the comment in clay_group_layer_suspend_mask().
   */
  if (! clay_item_is_attached (item) || private->suspend_mask > 1)
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_group_layer_resume_mask (clay_item_get_image (item),
                                                  NULL, group);

  private->suspend_mask--;

  if (private->suspend_mask == 0)
    g_clear_object (&private->suspended_mask_buffer);
}


/*  protected functions  */

void
_clay_group_layer_set_suspended_mask (ClayGroupLayer      *group,
                                      GeglBuffer          *buffer,
                                      const GeglRectangle *bounds)
{
  ClayGroupLayerPrivate *private;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (bounds != NULL);

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_mask > 0);

  g_object_ref (buffer);

  g_clear_object (&private->suspended_mask_buffer);

  private->suspended_mask_buffer = buffer;
  private->suspended_mask_bounds = *bounds;
}

GeglBuffer *
_clay_group_layer_get_suspended_mask (ClayGroupLayer *group,
                                      GeglRectangle  *bounds)
{
  ClayGroupLayerPrivate *private;
  ClayLayerMask         *mask;

  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (bounds != NULL, NULL);

  private = GET_PRIVATE (group);
  mask    = clay_layer_get_mask (CLAY_LAYER (group));

  g_return_val_if_fail (private->suspend_mask > 0, NULL);

  if (mask &&
      clay_drawable_get_buffer (CLAY_DRAWABLE (mask)) !=
      private->suspended_mask_buffer)
    {
      *bounds = private->suspended_mask_bounds;

      return private->suspended_mask_buffer;
    }

  return NULL;
}

void
_clay_group_layer_start_transform (ClayGroupLayer *group,
                                   gboolean        push_undo)
{
  ClayGroupLayerPrivate *private;
  ClayItem              *item;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = CLAY_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_group_layer_start_transform (clay_item_get_image (item),
                                                      NULL, group);

  private->transforming++;
}

void
_clay_group_layer_end_transform (ClayGroupLayer *group,
                                 gboolean        push_undo)
{
  ClayGroupLayerPrivate *private;
  ClayItem              *item;

  g_return_if_fail (CLAY_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = CLAY_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);
  g_return_if_fail (private->transforming > 0);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_push_group_layer_end_transform (clay_item_get_image (item),
                                                    NULL, group);

  private->transforming--;

  if (private->transforming == 0)
    clay_group_layer_update_mask_size (CLAY_GROUP_LAYER (item));
}


/*  private functions  */

static void
clay_group_layer_child_add (ClayContainer  *container,
                            ClayLayer      *child,
                            ClayGroupLayer *group)
{
  clay_group_layer_update (group);

  if (clay_filter_get_active (CLAY_FILTER (child)))
    {
      clay_layer_update_effective_mode (CLAY_LAYER (group));

      if (clay_layer_get_excludes_backdrop (child))
        clay_layer_update_excludes_backdrop (CLAY_LAYER (group));
    }
}

static void
clay_group_layer_child_remove (ClayContainer  *container,
                               ClayLayer      *child,
                               ClayGroupLayer *group)
{
  clay_group_layer_update (group);

  if (clay_filter_get_active (CLAY_FILTER (child)))
    {
      clay_layer_update_effective_mode (CLAY_LAYER (group));

      if (clay_layer_get_excludes_backdrop (child))
        clay_layer_update_excludes_backdrop (CLAY_LAYER (group));
    }
}

static void
clay_group_layer_child_move (ClayLayer      *child,
                             GParamSpec     *pspec,
                             ClayGroupLayer *group)
{
  clay_group_layer_update (group);
}

static void
clay_group_layer_child_resize (ClayLayer      *child,
                               ClayGroupLayer *group)
{
  clay_group_layer_update (group);
}

static void
clay_group_layer_child_active_changed (ClayLayer      *child,
                                       ClayGroupLayer *group)
{
  clay_layer_update_effective_mode (CLAY_LAYER (group));

  if (clay_layer_get_excludes_backdrop (child))
    clay_layer_update_excludes_backdrop (CLAY_LAYER (group));
}

static void
clay_group_layer_child_effective_mode_changed (ClayLayer      *child,
                                               ClayGroupLayer *group)
{
  if (clay_filter_get_active (CLAY_FILTER (child)))
    clay_layer_update_effective_mode (CLAY_LAYER (group));
}

static void
clay_group_layer_child_filters_changed (ClayLayer      *child,
                                        ClayGroupLayer *group)
{
  if (clay_filter_get_active (CLAY_FILTER (child)))
    clay_layer_update_effective_mode (CLAY_LAYER (group));
}

static void
clay_group_layer_child_excludes_backdrop_changed (ClayLayer      *child,
                                                  ClayGroupLayer *group)
{
  if (clay_filter_get_active (CLAY_FILTER (child)))
    clay_layer_update_excludes_backdrop (CLAY_LAYER (group));
}

static void
clay_group_layer_flush (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);

  if (private->pass_through)
    {
      /*  flush the projectable, not the pickable, because the source
       *  node of pass-through groups doesn't use the projection's
       *  buffer, hence there's no need to invalidate it synchronously.
       */
      clay_projectable_flush (CLAY_PROJECTABLE (group), TRUE);
    }
  else
    {
      /* make sure we have a buffer, and stop any idle rendering, which is
       * initiated when a new buffer is allocated.  the call to
       * clay_pickable_flush() below causes any pending idle rendering to
       * finish synchronously, so this needs to happen before.
       */
      clay_pickable_get_buffer (CLAY_PICKABLE (private->projection));
      clay_projection_stop_rendering (private->projection);

      /*  flush the pickable not the projectable because flushing the
       *  pickable will finish all invalidation on the projection so it
       *  can be used as source (note that it will still be constructed
       *  when the actual read happens, so this it not a performance
       *  problem)
       */
      clay_pickable_flush (CLAY_PICKABLE (private->projection));
    }
}

static void
clay_group_layer_update (ClayGroupLayer *group)
{
  if (GET_PRIVATE (group)->suspend_resize == 0)
    {
      clay_group_layer_update_size (group);
    }
}

static void
clay_group_layer_update_size (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);
  ClayItem              *item    = CLAY_ITEM (group);
  ClayLayer             *layer   = CLAY_LAYER (group);
  ClayItem              *mask    = CLAY_ITEM (clay_layer_get_mask (layer));
  GeglRectangle          old_bounds;
  GeglRectangle          bounds;
  GeglRectangle          old_bounding_box;
  GeglRectangle          bounding_box;
  gboolean               first   = TRUE;
  gboolean               size_changed;
  gboolean               resize_mask;
  GList                 *list;

  old_bounds.x      = clay_item_get_offset_x (item);
  old_bounds.y      = clay_item_get_offset_y (item);
  old_bounds.width  = clay_item_get_width    (item);
  old_bounds.height = clay_item_get_height   (item);

  bounds.x          = 0;
  bounds.y          = 0;
  bounds.width      = 1;
  bounds.height     = 1;

  old_bounding_box  = private->bounding_box;
  bounding_box      = bounds;

  for (list = clay_item_stack_get_item_iter (CLAY_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      ClayItem      *child = list->data;
      GeglRectangle  child_bounds;
      GeglRectangle  child_bounding_box;

      if (! clay_viewable_get_size (CLAY_VIEWABLE (child),
                                    &child_bounds.width, &child_bounds.height))
        {
          /*  ignore children without content (empty group layers);
           *  see bug 777017
           */
          continue;
        }

      clay_item_get_offset (child, &child_bounds.x, &child_bounds.y);

      child_bounding_box =
        clay_drawable_get_bounding_box (CLAY_DRAWABLE (child));

      child_bounding_box.x += child_bounds.x;
      child_bounding_box.y += child_bounds.y;

      if (first)
        {
          bounds       = child_bounds;
          bounding_box = child_bounding_box;

          first = FALSE;
        }
      else
        {
          gegl_rectangle_bounding_box (&bounds,
                                       &bounds, &child_bounds);
          gegl_rectangle_bounding_box (&bounding_box,
                                       &bounding_box, &child_bounding_box);
        }
    }

  bounding_box.x -= bounds.x;
  bounding_box.y -= bounds.y;

  size_changed = ! (gegl_rectangle_equal (&bounds,       &old_bounds) &&
                    gegl_rectangle_equal (&bounding_box, &old_bounding_box));

  resize_mask = mask && ! gegl_rectangle_equal (&bounds, &old_bounds);

  /* if we show the mask, invalidate the old mask area */
  if (resize_mask && clay_layer_get_show_mask (layer))
    {
      clay_drawable_update (CLAY_DRAWABLE (group),
                            clay_item_get_offset_x (mask) - old_bounds.x,
                            clay_item_get_offset_y (mask) - old_bounds.y,
                            clay_item_get_width    (mask),
                            clay_item_get_height   (mask));
    }

  if (private->reallocate_projection || size_changed)
    {
      GeglBuffer *buffer;

      /* if the graph is already constructed, set the offset node's
       * coordinates first, so the graph is in the right state when
       * the projection is reallocated, see bug #730550.
       */
      if (private->offset_node)
        gegl_node_set (private->offset_node,
                       "x", (gdouble) -bounds.x,
                       "y", (gdouble) -bounds.y,
                       NULL);

      /* update our offset *before* calling clay_pickable_get_buffer(), so
       * that if our graph isn't constructed yet, the offset node picks
       * up the right coordinates in clay_group_layer_get_graph().
       */
      clay_item_set_offset (item, bounds.x, bounds.y);

      /* update the bounding box before updating the projection, so that it
       * picks up the right size.
       */
      private->bounding_box = bounding_box;

      if (private->reallocate_projection)
        {
          private->reallocate_projection = FALSE;

          clay_projectable_structure_changed (CLAY_PROJECTABLE (group));
        }
      else
        {
          /* when there's no need to reallocate the projection, we call
           * clay_projectable_bounds_changed(), rather than structure_chaned(),
           * so that the projection simply copies the old content over to the
           * new buffer with an offset, rather than re-renders the graph.
           */
          clay_projectable_bounds_changed (CLAY_PROJECTABLE (group),
                                           old_bounds.x, old_bounds.y);
        }

      buffer = clay_pickable_get_buffer (CLAY_PICKABLE (private->projection));

      clay_drawable_set_buffer_full (CLAY_DRAWABLE (group),
                                     FALSE, NULL,
                                     buffer, &bounds,
                                     FALSE /* don't update the drawable, the
                                            * flush() below will take care of
                                            * that.
                                            */);

      clay_group_layer_flush (group);
    }

  /* resize the mask if not transforming (in which case, ClayLayer takes care
   * of the mask)
   */
  if (resize_mask && ! private->transforming)
    clay_group_layer_update_mask_size (group);

  /* if we show the mask, invalidate the new mask area */
  if (resize_mask && clay_layer_get_show_mask (layer))
    {
      clay_drawable_update (CLAY_DRAWABLE (group),
                            clay_item_get_offset_x (mask) - bounds.x,
                            clay_item_get_offset_y (mask) - bounds.y,
                            clay_item_get_width    (mask),
                            clay_item_get_height   (mask));
    }
}

static void
clay_group_layer_update_mask_size (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);
  ClayItem              *item    = CLAY_ITEM (group);
  ClayItem              *mask;
  GeglBuffer            *buffer;
  GeglBuffer            *mask_buffer;
  GeglRectangle          bounds;
  GeglRectangle          mask_bounds;
  GeglRectangle          copy_bounds;
  gboolean               intersect;

  mask = CLAY_ITEM (clay_layer_get_mask (CLAY_LAYER (group)));

  if (! mask)
    return;

  bounds.x           = clay_item_get_offset_x (item);
  bounds.y           = clay_item_get_offset_y (item);
  bounds.width       = clay_item_get_width    (item);
  bounds.height      = clay_item_get_height   (item);

  mask_bounds.x      = clay_item_get_offset_x (mask);
  mask_bounds.y      = clay_item_get_offset_y (mask);
  mask_bounds.width  = clay_item_get_width    (mask);
  mask_bounds.height = clay_item_get_height   (mask);

  if (gegl_rectangle_equal (&bounds, &mask_bounds))
    return;

  buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0, bounds.width, bounds.height),
                                 clay_drawable_get_format (CLAY_DRAWABLE (mask)),
                                 CLAY_DRAWABLE (mask));

  if (private->suspended_mask_buffer)
    {
      /* copy the suspended mask into the new mask */
      mask_buffer = private->suspended_mask_buffer;
      mask_bounds = private->suspended_mask_bounds;
    }
  else
    {
      /* copy the old mask into the new mask */
      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));
    }

  intersect = clay_rectangle_intersect (bounds.x,
                                        bounds.y,
                                        bounds.width,
                                        bounds.height,
                                        mask_bounds.x,
                                        mask_bounds.y,
                                        mask_bounds.width,
                                        mask_bounds.height,
                                        &copy_bounds.x,
                                        &copy_bounds.y,
                                        &copy_bounds.width,
                                        &copy_bounds.height);

  if (intersect)
    {
      clay_gegl_buffer_copy (mask_buffer,
                             GEGL_RECTANGLE (copy_bounds.x - mask_bounds.x,
                                             copy_bounds.y - mask_bounds.y,
                                             copy_bounds.width,
                                             copy_bounds.height),
                             GEGL_ABYSS_NONE,
                             buffer,
                             GEGL_RECTANGLE (copy_bounds.x - bounds.x,
                                             copy_bounds.y - bounds.y,
                                             copy_bounds.width,
                                             copy_bounds.height));
    }

  clay_drawable_set_buffer_full (CLAY_DRAWABLE (mask),
                                 FALSE, NULL,
                                 buffer, &bounds,
                                 TRUE);

  g_object_unref (buffer);
}

static void
clay_group_layer_update_source_node (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);
  GeglNode              *input;
  GeglNode              *output;

  if (private->source_node == NULL)
    return;

  input  = gegl_node_get_input_proxy  (private->source_node, "input");
  output = gegl_node_get_output_proxy (private->source_node, "output");

  if (private->pass_through)
    {
      gegl_node_link (input, private->graph);
      gegl_node_link (private->graph, output);
    }
  else
    {
      gegl_node_disconnect (private->graph, "input");

      gegl_node_link (private->parent_source_node, output);
    }
}

static void
clay_group_layer_update_mode_node (ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);
  GeglNode              *node;
  GeglNode              *input;
  GeglNode              *mode_node;

  node      = clay_filter_get_node (CLAY_FILTER (group));
  input     = gegl_node_get_input_proxy (node, "input");
  mode_node = clay_drawable_get_mode_node (CLAY_DRAWABLE (group));

  if (private->pass_through &&
      clay_layer_get_excludes_backdrop (CLAY_LAYER (group)))
    {
      gegl_node_disconnect (mode_node, "input");
    }
  else
    {
      gegl_node_link (input, mode_node);
    }
}

static void
clay_group_layer_stack_update (ClayDrawableStack *stack,
                               gint               x,
                               gint               y,
                               gint               width,
                               gint               height,
                               ClayGroupLayer    *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, clay_object_get_name (group),
              x, y, width, height);
#endif

  if (! private->direct_update)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  pass to the projection as-is.
       */
      clay_projectable_invalidate (CLAY_PROJECTABLE (group),
                                   x, y, width, height);

      clay_group_layer_flush (group);
    }

  if (private->direct_update || private->pass_through)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  transform to layer coordinates when emitting our own update signal.
       */
      clay_drawable_update (CLAY_DRAWABLE (group),
                            x - clay_item_get_offset_x (CLAY_ITEM (group)),
                            y - clay_item_get_offset_y (CLAY_ITEM (group)),
                            width, height);
    }
}

static void
clay_group_layer_proj_update (ClayProjection *proj,
                              gboolean        now,
                              gint            x,
                              gint            y,
                              gint            width,
                              gint            height,
                              ClayGroupLayer *group)
{
  ClayGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, clay_object_get_name (group),
              x, y, width, height);
#endif

  if (! private->pass_through)
    {
      /* TODO: groups can currently have a gegl:transform op attached as a filter
       * when using a transform tool, in which case the updated region needs
       * undergo the same transformation.  more generally, when a drawable has
       * filters they may influence the area affected by drawable updates.
       *
       * this needs to be addressed much more generally at some point, but for now
       * we just resort to updating the entire group when it has a filter (i.e.,
       * when it's being used with a transform tool).  we restrict this to groups,
       * and don't do this more generally in clay_drawable_update(), because this
       * negatively impacts the performance of the warp tool, which does perform
       * accurate drawable updates while using a filter.
       */
      if (clay_drawable_has_visible_filters (CLAY_DRAWABLE (group)))
        {
          width  = -1;
          height = -1;
        }

      /*  the projection speaks in image coordinates, transform to layer
       *  coordinates when emitting our own update signal.
       */
      clay_drawable_update (CLAY_DRAWABLE (group),
                            x - clay_item_get_offset_x (CLAY_ITEM (group)),
                            y - clay_item_get_offset_y (CLAY_ITEM (group)),
                            width, height);
    }
}

static gboolean
clay_group_layer_enter_layers (ClayLoadState  *state,
                               const gchar   **attribute_names,
                               const gchar   **attribute_values,
                               gpointer        user_data,
                               GError        **error)
{
  g_return_val_if_fail (CLAY_IS_GROUP_LAYER (clay_savable_load_peek_active_object (state)), FALSE);

  clay_savable_load (CLAY_TYPE_LAYER, state);
  clay_savable_load (CLAY_TYPE_GROUP_LAYER, state);
  clay_savable_load (CLAY_TYPE_VECTOR_LAYER, state);
  clay_savable_load (CLAY_TYPE_LINK_LAYER, state);
  clay_savable_load (CLAY_TYPE_TEXT_LAYER, state);

  return TRUE;
}
