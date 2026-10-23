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

#include "clayitem.h"


#define CLAY_TYPE_DRAWABLE            (clay_drawable_get_type ())
#define CLAY_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE, ClayDrawable))
#define CLAY_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE, ClayDrawableClass))
#define CLAY_IS_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE))
#define CLAY_IS_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE))
#define CLAY_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE, ClayDrawableClass))


typedef struct _ClayDrawablePrivate ClayDrawablePrivate;
typedef struct _ClayDrawableClass   ClayDrawableClass;

struct _ClayDrawable
{
  ClayItem             parent_instance;

  ClayDrawablePrivate *private;
};

struct _ClayDrawableClass
{
  ClayItemClass  parent_class;

  /*  signals  */
  void          (* update)                (ClayDrawable         *drawable,
                                           gint                  x,
                                           gint                  y,
                                           gint                  width,
                                           gint                  height);
  void          (* format_changed)        (ClayDrawable         *drawable);
  void          (* alpha_changed)         (ClayDrawable         *drawable);
  void          (* bounding_box_changed)  (ClayDrawable         *drawable);
  void          (* filters_changed)       (ClayDrawable         *drawable);

  /*  virtual functions  */
  gint64        (* estimate_memsize)        (ClayDrawable         *drawable,
                                             ClayComponentType     component_type,
                                             gint                  width,
                                             gint                  height);
  void          (* update_all)              (ClayDrawable         *drawable);
  void          (* invalidate_boundary)     (ClayDrawable         *drawable);
  ClayComponentMask (* get_active_mask)     (ClayDrawable         *drawable);
  gboolean      (* supports_alpha)          (ClayDrawable         *drawable);
  void          (* convert_type)            (ClayDrawable         *drawable,
                                             ClayImage            *dest_image,
                                             const Babl           *new_format,
                                             ClayColorProfile     *src_profile,
                                             ClayColorProfile     *dest_profile,
                                             GeglDitherMethod      layer_dither_type,
                                             GeglDitherMethod      mask_dither_type,
                                             gboolean              push_undo,
                                             ClayProgress         *progress);
  void          (* apply_buffer)            (ClayDrawable         *drawable,
                                             GeglBuffer           *buffer,
                                             const GeglRectangle  *buffer_region,
                                             gboolean              push_undo,
                                             const gchar          *undo_desc,
                                             gdouble               opacity,
                                             ClayLayerMode         mode,
                                             ClayLayerColorSpace   blend_space,
                                             ClayLayerColorSpace   composite_space,
                                             ClayLayerCompositeMode composite_mode,
                                             GeglBuffer           *base_buffer,
                                             gint                  base_x,
                                             gint                  base_y);
  GeglBuffer  * (* get_buffer)              (ClayDrawable         *drawable);
  void          (* set_buffer)              (ClayDrawable         *drawable,
                                             gboolean              push_undo,
                                             const gchar          *undo_desc,
                                             GeglBuffer           *buffer,
                                             const GeglRectangle  *bounds);
  GeglBuffer  * (* get_buffer_with_effects) (ClayDrawable         *drawable);
  GeglRectangle (* get_bounding_box)        (ClayDrawable         *drawable);
  void          (* push_undo)               (ClayDrawable         *drawable,
                                             const gchar          *undo_desc,
                                             GeglBuffer           *buffer,
                                             gint                  x,
                                             gint                  y,
                                             gint                  width,
                                             gint                  height);
  void          (* swap_pixels)             (ClayDrawable         *drawable,
                                             GeglBuffer           *buffer,
                                             gint                  x,
                                             gint                  y);
  GeglNode    * (* get_source_node)         (ClayDrawable         *drawable);
};


GType           clay_drawable_get_type                (void);

ClayDrawable  * clay_drawable_new                     (GType               type,
                                                       ClayImage          *image,
                                                       const gchar        *name,
                                                       gint                offset_x,
                                                       gint                offset_y,
                                                       gint                width,
                                                       gint                height,
                                                       const Babl         *format);

gint64          clay_drawable_estimate_memsize        (ClayDrawable       *drawable,
                                                       ClayComponentType   component_type,
                                                       gint                width,
                                                       gint                height);

void            clay_drawable_update                  (ClayDrawable       *drawable,
                                                       gint                x,
                                                       gint                y,
                                                       gint                width,
                                                       gint                height);
void            clay_drawable_update_all              (ClayDrawable       *drawable);

void            clay_drawable_filters_changed         (ClayDrawable       *drawable);

void           clay_drawable_invalidate_boundary      (ClayDrawable       *drawable);
ClayComponentMask clay_drawable_get_active_mask       (ClayDrawable       *drawable);

gboolean        clay_drawable_supports_alpha          (ClayDrawable       *drawable);

void            clay_drawable_convert_type            (ClayDrawable       *drawable,
                                                       ClayImage          *dest_image,
                                                       ClayImageBaseType   new_base_type,
                                                       ClayPrecision       new_precision,
                                                       gboolean            new_has_alpha,
                                                       ClayColorProfile   *src_profile,
                                                       ClayColorProfile   *dest_profile,
                                                       GeglDitherMethod    layer_dither_type,
                                                       GeglDitherMethod    mask_dither_type,
                                                       gboolean            push_undo,
                                                       ClayProgress       *progress);

void            clay_drawable_apply_buffer            (ClayDrawable        *drawable,
                                                       GeglBuffer          *buffer,
                                                       const GeglRectangle *buffer_rect,
                                                       gboolean             push_undo,
                                                       const gchar         *undo_desc,
                                                       gdouble              opacity,
                                                       ClayLayerMode        mode,
                                                       ClayLayerColorSpace  blend_space,
                                                       ClayLayerColorSpace  composite_space,
                                                       ClayLayerCompositeMode composite_mode,
                                                       GeglBuffer          *base_buffer,
                                                       gint                 base_x,
                                                       gint                 base_y);

GeglBuffer    * clay_drawable_get_buffer              (ClayDrawable       *drawable);
void            clay_drawable_set_buffer              (ClayDrawable       *drawable,
                                                       gboolean            push_undo,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer);
void            clay_drawable_set_buffer_full         (ClayDrawable       *drawable,
                                                       gboolean            push_undo,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer,
                                                       const GeglRectangle *bounds,
                                                       gboolean            update);
GeglBuffer    * clay_drawable_get_buffer_with_effects (ClayDrawable       *drawable);

void            clay_drawable_steal_buffer            (ClayDrawable       *drawable,
                                                       ClayDrawable      **src_drawable);

void            clay_drawable_set_format              (ClayDrawable       *drawable,
                                                       const Babl         *format,
                                                       gboolean            copy_buffer,
                                                       gboolean            push_undo);

GeglNode      * clay_drawable_get_source_node         (ClayDrawable       *drawable);
GeglNode      * clay_drawable_get_mode_node           (ClayDrawable       *drawable);

GeglRectangle   clay_drawable_get_bounding_box        (ClayDrawable       *drawable);
gboolean        clay_drawable_update_bounding_box
                                                      (ClayDrawable       *drawable);

void            clay_drawable_swap_pixels             (ClayDrawable       *drawable,
                                                       GeglBuffer         *buffer,
                                                       gint                x,
                                                       gint                y);

void            clay_drawable_push_undo               (ClayDrawable       *drawable,
                                                       const gchar        *undo_desc,
                                                       GeglBuffer         *buffer,
                                                       gint                x,
                                                       gint                y,
                                                       gint                width,
                                                       gint                height);

void           clay_drawable_disable_resize_undo      (ClayDrawable       *drawable);
void           clay_drawable_enable_resize_undo       (ClayDrawable       *drawable);

const Babl      * clay_drawable_get_space            (ClayDrawable    *drawable);
const Babl      * clay_drawable_get_format           (ClayDrawable    *drawable);
const Babl      * clay_drawable_get_format_with_alpha(ClayDrawable    *drawable);
const Babl      * clay_drawable_get_format_without_alpha
                                                     (ClayDrawable    *drawable);
ClayTRCType       clay_drawable_get_trc              (ClayDrawable    *drawable);
gboolean          clay_drawable_has_alpha            (ClayDrawable    *drawable);
ClayImageBaseType clay_drawable_get_base_type        (ClayDrawable    *drawable);
ClayComponentType clay_drawable_get_component_type   (ClayDrawable    *drawable);
ClayPrecision     clay_drawable_get_precision        (ClayDrawable    *drawable);
gboolean          clay_drawable_is_rgb               (ClayDrawable    *drawable);
gboolean          clay_drawable_is_gray              (ClayDrawable    *drawable);
gboolean          clay_drawable_is_indexed           (ClayDrawable    *drawable);

const Babl      * clay_drawable_get_component_format (ClayDrawable    *drawable,
                                                      ClayChannelType  channel);
gint              clay_drawable_get_component_index  (ClayDrawable    *drawable,
                                                      ClayChannelType  channel);

void              clay_drawable_start_paint          (ClayDrawable    *drawable);
gboolean          clay_drawable_end_paint            (ClayDrawable    *drawable);
gboolean          clay_drawable_flush_paint          (ClayDrawable    *drawable);
gboolean          clay_drawable_is_painting          (ClayDrawable    *drawable);

gchar           * clay_drawable_get_new_cache_path   (ClayDrawable    *drawable);


G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayDrawable, g_object_unref);
