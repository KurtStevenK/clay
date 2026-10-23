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

#pragma once

#include "claydrawable.h"


#define CLAY_TYPE_LAYER            (clay_layer_get_type ())
#define CLAY_LAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_LAYER, ClayLayer))
#define CLAY_LAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_LAYER, ClayLayerClass))
#define CLAY_IS_LAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_LAYER))
#define CLAY_IS_LAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_LAYER))
#define CLAY_LAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_LAYER, ClayLayerClass))


typedef struct _ClayLayerClass ClayLayerClass;

struct _ClayLayer
{
  ClayDrawable            parent_instance;

  gdouble                 opacity;                    /*  layer opacity                     */
  ClayLayerMode           mode;                       /*  layer combination mode            */
  ClayLayerColorSpace     blend_space;                /*  layer blend space                 */
  ClayLayerColorSpace     composite_space;            /*  layer composite space             */
  ClayLayerCompositeMode  composite_mode;             /*  layer composite mode              */
  ClayLayerMode           effective_mode;             /*  layer effective combination mode  */
  ClayLayerColorSpace     effective_blend_space;      /*  layer effective blend space       */
  ClayLayerColorSpace     effective_composite_space;  /*  layer effective composite space   */
  ClayLayerCompositeMode  effective_composite_mode;   /*  layer effective composite mode    */
  gboolean                excludes_backdrop;          /*  layer clips backdrop              */
  gboolean                lock_alpha;                 /*  lock the alpha channel            */

  ClayLayerMask          *mask;                       /*  possible layer mask               */
  gboolean                apply_mask;                 /*  controls mask application         */
  gboolean                edit_mask;                  /*  edit mask or layer?               */
  gboolean                show_mask;                  /*  show mask or layer?               */

  GSList                 *move_stack;                 /*  ancestors affected by move        */

  GeglNode               *layer_offset_node;
  GeglNode               *mask_offset_node;

  /*  Floating selections  */
  struct
  {
    ClayDrawable *drawable;           /*  floating sel is attached to    */
    gboolean      boundary_known;     /*  is the current boundary valid  */
    ClayBoundSeg *segs;               /*  boundary of floating sel       */
    gint          num_segs;           /*  number of segs in boundary     */
  } fs;
};

struct _ClayLayerClass
{
  ClayDrawableClass  parent_class;

  /*  signals  */
  void          (* opacity_changed)           (ClayLayer              *layer);
  void          (* mode_changed)              (ClayLayer              *layer);
  void          (* blend_space_changed)       (ClayLayer              *layer);
  void          (* composite_space_changed)   (ClayLayer              *layer);
  void          (* composite_mode_changed)    (ClayLayer              *layer);
  void          (* effective_mode_changed)    (ClayLayer              *layer);
  void          (* excludes_backdrop_changed) (ClayLayer              *layer);
  void          (* lock_alpha_changed)        (ClayLayer              *layer);
  void          (* mask_changed)              (ClayLayer              *layer);
  void          (* apply_mask_changed)        (ClayLayer              *layer);
  void          (* edit_mask_changed)         (ClayLayer              *layer);
  void          (* show_mask_changed)         (ClayLayer              *layer);

  /*  virtual functions  */
  gboolean      (* is_alpha_locked)           (ClayLayer              *layer,
                                               ClayLayer             **locked_layer);

  void          (* translate)                 (ClayLayer              *layer,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* scale)                     (ClayLayer              *layer,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    new_offset_x,
                                               gint                    new_offset_y,
                                               ClayInterpolationType   interpolation_type,
                                               ClayProgress           *progress);
  void          (* resize)                    (ClayLayer              *layer,
                                               ClayContext            *context,
                                               ClayFillType            fill_type,
                                               gint                    new_width,
                                               gint                    new_height,
                                               gint                    offset_x,
                                               gint                    offset_y);
  void          (* flip)                      (ClayLayer              *layer,
                                               ClayContext            *context,
                                               ClayOrientationType     flip_type,
                                               gdouble                 axis,
                                               gboolean                clip_result);
  void          (* rotate)                    (ClayLayer              *layer,
                                               ClayContext            *context,
                                               ClayRotationType        rotate_type,
                                               gdouble                 center_x,
                                               gdouble                 center_y,
                                               gboolean                clip_result);
  void          (* transform)                 (ClayLayer              *layer,
                                               ClayContext            *context,
                                               const ClayMatrix3      *matrix,
                                               ClayTransformDirection  direction,
                                               ClayInterpolationType   interpolation_type,
                                               ClayTransformResize     clip_result,
                                               ClayProgress           *progress,
                                               gboolean                push_undo);
  void          (* convert_type)              (ClayLayer              *layer,
                                               ClayImage              *dest_image,
                                               const Babl             *new_format,
                                               ClayColorProfile       *src_profile,
                                               ClayColorProfile       *dest_profile,
                                               GeglDitherMethod        layer_dither_type,
                                               GeglDitherMethod        mask_dither_type,
                                               gboolean                push_undo,
                                               ClayProgress           *progress);
  GeglRectangle (* get_bounding_box)          (ClayLayer              *layer);
  void          (* get_effective_mode)        (ClayLayer              *layer,
                                               ClayLayerMode          *mode,
                                               ClayLayerColorSpace    *blend_space,
                                               ClayLayerColorSpace    *composite_space,
                                               ClayLayerCompositeMode *composite_mode);
  gboolean      (* get_excludes_backdrop)     (ClayLayer              *layer);
};


/*  function declarations  */

GType           clay_layer_get_type            (void);

void            clay_layer_fix_format_space    (ClayLayer            *layer,
                                                gboolean              copy_buffer,
                                                gboolean              push_undo);

ClayLayer     * clay_layer_get_parent          (ClayLayer            *layer);

ClayLayerMask * clay_layer_get_mask            (ClayLayer            *layer);
ClayLayerMask * clay_layer_create_mask         (ClayLayer            *layer,
                                                ClayAddMaskType       mask_type,
                                                ClayChannel          *channel);
ClayLayerMask * clay_layer_add_mask            (ClayLayer            *layer,
                                                ClayLayerMask        *mask,
                                                gboolean              edit_mask,
                                                gboolean              push_undo,
                                                GError              **error);
void            clay_layer_apply_mask          (ClayLayer            *layer,
                                                ClayMaskApplyMode     mode,
                                                gboolean              push_undo);

void            clay_layer_set_apply_mask      (ClayLayer           *layer,
                                                gboolean             apply,
                                                gboolean             push_undo);
gboolean        clay_layer_get_apply_mask      (ClayLayer           *layer);

void            clay_layer_set_edit_mask       (ClayLayer           *layer,
                                                gboolean             edit);
gboolean        clay_layer_get_edit_mask       (ClayLayer           *layer);

void            clay_layer_set_show_mask       (ClayLayer           *layer,
                                                gboolean             show,
                                                gboolean             push_undo);
gboolean        clay_layer_get_show_mask       (ClayLayer           *layer);

void            clay_layer_add_alpha           (ClayLayer            *layer);
void            clay_layer_remove_alpha        (ClayLayer            *layer,
                                                ClayContext          *context);

void            clay_layer_resize_to_image     (ClayLayer            *layer,
                                                ClayContext          *context,
                                                ClayFillType          fill_type);

ClayDrawable * clay_layer_get_floating_sel_drawable (ClayLayer       *layer);
void           clay_layer_set_floating_sel_drawable (ClayLayer       *layer,
                                                     ClayDrawable    *drawable);
gboolean        clay_layer_is_floating_sel     (ClayLayer            *layer);

void            clay_layer_set_opacity         (ClayLayer            *layer,
                                                gdouble               opacity,
                                                gboolean              push_undo);
gdouble         clay_layer_get_opacity         (ClayLayer            *layer);

void            clay_layer_set_mode            (ClayLayer            *layer,
                                                ClayLayerMode         mode,
                                                gboolean              push_undo);
ClayLayerMode   clay_layer_get_mode            (ClayLayer            *layer);

void            clay_layer_set_blend_space     (ClayLayer            *layer,
                                                ClayLayerColorSpace   blend_space,
                                                gboolean              push_undo);
ClayLayerColorSpace
                clay_layer_get_blend_space     (ClayLayer            *layer);
ClayLayerColorSpace
               clay_layer_get_real_blend_space (ClayLayer            *layer);

void            clay_layer_set_composite_space (ClayLayer            *layer,
                                                ClayLayerColorSpace   composite_space,
                                                gboolean              push_undo);
ClayLayerColorSpace
                clay_layer_get_composite_space (ClayLayer            *layer);
ClayLayerColorSpace
           clay_layer_get_real_composite_space (ClayLayer            *layer);

void            clay_layer_set_composite_mode  (ClayLayer            *layer,
                                                ClayLayerCompositeMode  composite_mode,
                                                gboolean              push_undo);
ClayLayerCompositeMode
                clay_layer_get_composite_mode  (ClayLayer            *layer);
ClayLayerCompositeMode
           clay_layer_get_real_composite_mode  (ClayLayer            *layer);

void            clay_layer_get_effective_mode  (ClayLayer            *layer,
                                                ClayLayerMode          *mode,
                                                ClayLayerColorSpace    *blend_space,
                                                ClayLayerColorSpace    *composite_space,
                                                ClayLayerCompositeMode *composite_mode);

gboolean      clay_layer_get_excludes_backdrop (ClayLayer            *layer);

void            clay_layer_set_lock_alpha      (ClayLayer            *layer,
                                                gboolean              lock_alpha,
                                                gboolean              push_undo);
gboolean        clay_layer_get_lock_alpha      (ClayLayer            *layer);
gboolean        clay_layer_can_lock_alpha      (ClayLayer            *layer);
gboolean        clay_layer_is_alpha_locked     (ClayLayer            *layer,
                                                ClayLayer           **locked_layer);


/*  protected  */

void          clay_layer_update_effective_mode (ClayLayer            *layer);
void       clay_layer_update_excludes_backdrop (ClayLayer            *layer);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayLayer, g_object_unref);
