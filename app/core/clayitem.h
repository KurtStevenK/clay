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

#include "clayfilter.h"


#define CLAY_TYPE_ITEM (clay_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayItem,
                          clay_item,
                          CLAY, ITEM,
                          ClayFilter)


struct _ClayItemClass
{
  ClayFilterClass  parent_class;

  /*  signals  */
  void            (* removed)               (ClayItem            *item);
  void            (* visibility_changed)    (ClayItem            *item);
  void            (* color_tag_changed)     (ClayItem            *item);
  void            (* lock_content_changed)  (ClayItem            *item);
  void            (* lock_position_changed) (ClayItem            *item);
  void            (* lock_visibility_changed) (ClayItem            *item);

  /*  virtual functions  */
  void            (* unset_removed)      (ClayItem               *item);
  gboolean        (* is_attached)        (ClayItem               *item);
  gboolean        (* is_content_locked)  (ClayItem               *item,
                                          ClayItem              **locked_item,
                                          gboolean                checking_children);
  gboolean        (* is_position_locked) (ClayItem               *item,
                                          ClayItem              **locked_item,
                                          gboolean                checking_children);
  gboolean        (* is_visibility_locked) (ClayItem               *item,
                                            ClayItem              **locked_item);
  ClayItemTree  * (* get_tree)           (ClayItem               *item);
  gboolean        (* bounds)             (ClayItem               *item,
                                          gdouble                *x,
                                          gdouble                *y,
                                          gdouble                *width,
                                          gdouble                *height);
  ClayItem      * (* duplicate)          (ClayItem               *item,
                                          GType                   new_type);
  void            (* convert)            (ClayItem               *item,
                                          ClayImage              *dest_image,
                                          GType                   old_type);
  gboolean        (* rename)             (ClayItem               *item,
                                          const gchar            *new_name,
                                          const gchar            *undo_desc,
                                          GError                **error);
  void            (* start_move)         (ClayItem               *item,
                                          gboolean                push_undo);
  void            (* end_move)           (ClayItem               *item,
                                          gboolean                push_undo);
  void            (* start_transform)    (ClayItem               *item,
                                          gboolean                push_undo);
  void            (* end_transform)      (ClayItem               *item,
                                          gboolean                push_undo);
  void            (* translate)          (ClayItem               *item,
                                          gdouble                 offset_x,
                                          gdouble                 offset_y,
                                          gboolean                push_undo);
  void            (* scale)              (ClayItem               *item,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    new_offset_x,
                                          gint                    new_offset_y,
                                          ClayInterpolationType   interpolation_type,
                                          ClayProgress           *progress);
  void            (* resize)             (ClayItem               *item,
                                          ClayContext            *context,
                                          ClayFillType            fill_type,
                                          gint                    new_width,
                                          gint                    new_height,
                                          gint                    offset_x,
                                          gint                    offset_y);
  void            (* flip)               (ClayItem               *item,
                                          ClayContext            *context,
                                          ClayOrientationType     flip_type,
                                          gdouble                 axis,
                                          gboolean                clip_result);
  void            (* rotate)             (ClayItem               *item,
                                          ClayContext            *context,
                                          ClayRotationType        rotate_type,
                                          gdouble                 center_x,
                                          gdouble                 center_y,
                                          gboolean                clip_result);
  void            (* transform)          (ClayItem               *item,
                                          ClayContext            *context,
                                          const ClayMatrix3      *matrix,
                                          ClayTransformDirection  direction,
                                          ClayInterpolationType   interpolation_type,
                                          ClayTransformResize     clip_result,
                                          ClayProgress           *progress,
                                          gboolean                push_undo);
  ClayTransformResize (* get_clip)       (ClayItem               *item,
                                          ClayTransformResize     clip_result);
  gboolean        (* fill)               (ClayItem               *item,
                                          ClayDrawable           *drawable,
                                          ClayFillOptions        *fill_options,
                                          gboolean                push_undo,
                                          ClayProgress           *progress,
                                          GError                **error);
  gboolean        (* stroke)             (ClayItem               *item,
                                          ClayDrawable           *drawable,
                                          ClayStrokeOptions      *stroke_options,
                                          gboolean                push_undo,
                                          ClayProgress           *progress,
                                          GError                **error);
  void            (* to_selection)       (ClayItem               *item,
                                          ClayChannelOps          op,
                                          gboolean                antialias,
                                          gboolean                feather,
                                          gdouble                 feather_radius_x,
                                          gdouble                 feather_radius_y);

  const gchar *rename_desc;
  const gchar *translate_desc;
  const gchar *scale_desc;
  const gchar *resize_desc;
  const gchar *flip_desc;
  const gchar *rotate_desc;
  const gchar *transform_desc;
  const gchar *to_selection_desc;
  const gchar *fill_desc;
  const gchar *stroke_desc;

  const gchar *reorder_desc;
  const gchar *raise_desc;
  const gchar *raise_to_top_desc;
  const gchar *lower_desc;
  const gchar *lower_to_bottom_desc;

  const gchar *raise_failed;
  const gchar *lower_failed;
};


ClayItem      * clay_item_new                (GType               type,
                                              ClayImage          *image,
                                              const gchar        *name,
                                              gint                offset_x,
                                              gint                offset_y,
                                              gint                width,
                                              gint                height);

void            clay_item_removed            (ClayItem           *item);
gboolean        clay_item_is_removed         (ClayItem           *item);
void            clay_item_unset_removed      (ClayItem           *item);

gboolean        clay_item_is_attached        (ClayItem           *item);

ClayItem      * clay_item_get_parent         (ClayItem           *item);

ClayItemTree  * clay_item_get_tree           (ClayItem           *item);
ClayContainer * clay_item_get_container      (ClayItem           *item);
GList         * clay_item_get_container_iter (ClayItem           *item);
gint            clay_item_get_index          (ClayItem           *item);
GList         * clay_item_get_path           (ClayItem           *item);

gboolean        clay_item_bounds             (ClayItem           *item,
                                              gint               *x,
                                              gint               *y,
                                              gint               *width,
                                              gint               *height);
gboolean        clay_item_bounds_f           (ClayItem           *item,
                                              gdouble            *x,
                                              gdouble            *y,
                                              gdouble            *width,
                                              gdouble            *height);

ClayItem      * clay_item_duplicate          (ClayItem           *item,
                                              GType               new_type);
ClayItem      * clay_item_convert            (ClayItem           *item,
                                              ClayImage          *dest_image,
                                              GType               new_type);

gboolean        clay_item_rename             (ClayItem           *item,
                                              const gchar        *new_name,
                                              GError            **error);

gint            clay_item_get_width          (ClayItem           *item);
gint            clay_item_get_height         (ClayItem           *item);
void            clay_item_set_size           (ClayItem           *item,
                                              gint                width,
                                              gint                height);

void            clay_item_get_offset         (ClayItem           *item,
                                              gint               *offset_x,
                                              gint               *offset_y);
void            clay_item_set_offset         (ClayItem           *item,
                                              gint                offset_x,
                                              gint                offset_y);
gint            clay_item_get_offset_x       (ClayItem           *item);
gint            clay_item_get_offset_y       (ClayItem           *item);

void            clay_item_start_move         (ClayItem           *item,
                                              gboolean            push_undo);
void            clay_item_end_move           (ClayItem           *item,
                                              gboolean            push_undo);

void            clay_item_start_transform    (ClayItem           *item,
                                              gboolean            push_undo);
void            clay_item_end_transform      (ClayItem           *item,
                                              gboolean            push_undo);

void            clay_item_translate          (ClayItem           *item,
                                              gdouble             offset_x,
                                              gdouble             offset_y,
                                              gboolean            push_undo);

gboolean        clay_item_check_scaling      (ClayItem           *item,
                                              gint                new_width,
                                              gint                new_height);
void            clay_item_scale              (ClayItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                new_offset_x,
                                              gint                new_offset_y,
                                              ClayInterpolationType interpolation,
                                              ClayProgress       *progress);
gboolean        clay_item_scale_by_factors   (ClayItem           *item,
                                              gdouble             w_factor,
                                              gdouble             h_factor,
                                              ClayInterpolationType interpolation,
                                              ClayProgress       *progress);
gboolean
      clay_item_scale_by_factors_with_origin (ClayItem           *item,
                                              gdouble             w_factor,
                                              gdouble             h_factor,
                                              gint                origin_x,
                                              gint                origin_y,
                                              gint                new_origin_x,
                                              gint                new_origin_y,
                                              ClayInterpolationType interpolation,
                                              ClayProgress       *progress);
void            clay_item_scale_by_origin    (ClayItem           *item,
                                              gint                new_width,
                                              gint                new_height,
                                              ClayInterpolationType interpolation,
                                              ClayProgress       *progress,
                                              gboolean            local_origin);
void            clay_item_resize             (ClayItem           *item,
                                              ClayContext        *context,
                                              ClayFillType        fill_type,
                                              gint                new_width,
                                              gint                new_height,
                                              gint                offset_x,
                                              gint                offset_y);
void            clay_item_resize_to_image    (ClayItem           *item);

void            clay_item_flip               (ClayItem           *item,
                                              ClayContext        *context,
                                              ClayOrientationType flip_type,
                                              gdouble             axis,
                                              gboolean            clip_result);
void            clay_item_rotate             (ClayItem           *item,
                                              ClayContext        *context,
                                              ClayRotationType    rotate_type,
                                              gdouble             center_x,
                                              gdouble             center_y,
                                              gboolean            clip_result);
void            clay_item_transform          (ClayItem           *item,
                                              ClayContext        *context,
                                              const ClayMatrix3  *matrix,
                                              ClayTransformDirection direction,
                                              ClayInterpolationType interpolation_type,
                                              ClayTransformResize clip_result,
                                              ClayProgress       *progress);
ClayTransformResize   clay_item_get_clip     (ClayItem           *item,
                                              ClayTransformResize clip_result);

gboolean        clay_item_fill               (ClayItem           *item,
                                              GList              *drawables,
                                              ClayFillOptions    *fill_options,
                                              gboolean            push_undo,
                                              ClayProgress       *progress,
                                              GError            **error);
gboolean        clay_item_stroke             (ClayItem           *item,
                                              GList              *drawables,
                                              ClayContext        *context,
                                              ClayStrokeOptions  *stroke_options,
                                              ClayPaintOptions   *paint_options,
                                              gboolean            push_undo,
                                              ClayProgress       *progress,
                                              GError            **error);

void            clay_item_to_selection       (ClayItem           *item,
                                              ClayChannelOps      op,
                                              gboolean            antialias,
                                              gboolean            feather,
                                              gdouble             feather_radius_x,
                                              gdouble             feather_radius_y);

void            clay_item_add_offset_node    (ClayItem           *item,
                                              GeglNode           *node);
void            clay_item_remove_offset_node (ClayItem           *item,
                                              GeglNode           *node);

gint            clay_item_get_id             (ClayItem           *item);
ClayItem      * clay_item_get_by_id          (Clay               *clay,
                                              gint                id);
void            clay_item_set_id             (ClayItem           *item,
                                              gint                id);

ClayTattoo      clay_item_get_tattoo         (ClayItem           *item);
void            clay_item_set_tattoo         (ClayItem           *item,
                                              ClayTattoo          tattoo);

ClayImage     * clay_item_get_image          (ClayItem           *item);
void            clay_item_set_image          (ClayItem           *item,
                                              ClayImage          *image);

void            clay_item_replace_item       (ClayItem           *item,
                                              ClayItem           *replace);

void               clay_item_set_parasites   (ClayItem           *item,
                                              ClayParasiteList   *parasites);
ClayParasiteList * clay_item_get_parasites   (ClayItem           *item);

gboolean        clay_item_parasite_validate  (ClayItem           *item,
                                              const ClayParasite *parasite,
                                              GError            **error);
void            clay_item_parasite_attach    (ClayItem           *item,
                                              const ClayParasite *parasite,
                                              gboolean            push_undo);
void            clay_item_parasite_detach    (ClayItem           *item,
                                              const gchar        *name,
                                              gboolean            push_undo);
const ClayParasite * clay_item_parasite_find (ClayItem           *item,
                                              const gchar        *name);
gchar        ** clay_item_parasite_list      (ClayItem           *item);

gboolean        clay_item_set_visible        (ClayItem           *item,
                                              gboolean            visible,
                                              gboolean            push_undo);
gboolean        clay_item_get_visible        (ClayItem           *item);
gboolean        clay_item_is_visible         (ClayItem           *item);

void        clay_item_bind_visible_to_active (ClayItem           *item,
                                              gboolean            bind);

void            clay_item_set_color_tag      (ClayItem           *item,
                                              ClayColorTag        color_tag,
                                              gboolean            push_undo);
ClayColorTag    clay_item_get_color_tag      (ClayItem           *item);
ClayColorTag  clay_item_get_merged_color_tag (ClayItem           *item);

void            clay_item_set_lock_content   (ClayItem           *item,
                                              gboolean            lock_content,
                                              gboolean            push_undo);
gboolean        clay_item_get_lock_content   (ClayItem           *item);
gboolean        clay_item_can_lock_content   (ClayItem           *item);
gboolean        clay_item_is_content_locked  (ClayItem           *item,
                                              ClayItem          **locked_item);

void            clay_item_set_lock_position  (ClayItem          *item,
                                              gboolean           lock_position,
                                              gboolean           push_undo);
gboolean        clay_item_get_lock_position  (ClayItem          *item);
gboolean        clay_item_can_lock_position  (ClayItem          *item);
gboolean        clay_item_is_position_locked (ClayItem          *item,
                                              ClayItem         **locked_item);

void            clay_item_set_lock_visibility  (ClayItem        *item,
                                                gboolean         lock_visibility,
                                                gboolean         push_undo);
gboolean        clay_item_get_lock_visibility  (ClayItem        *item);
gboolean        clay_item_can_lock_visibility  (ClayItem        *item);
gboolean        clay_item_is_visibility_locked (ClayItem        *item,
                                                ClayItem       **locked_item);

gboolean        clay_item_mask_bounds        (ClayItem           *item,
                                              gint               *x1,
                                              gint               *y1,
                                              gint               *x2,
                                              gint               *y2);
gboolean        clay_item_mask_intersect     (ClayItem           *item,
                                              gint               *x,
                                              gint               *y,
                                              gint               *width,
                                              gint               *height);

gboolean        clay_item_is_in_set          (ClayItem           *item,
                                              ClayItemSet         set);

gboolean        clay_item_is_rasterizable    (ClayItem           *item);
gboolean        clay_item_is_rasterized      (ClayItem           *item);

gboolean        clay_item_is_writable        (ClayItem           *item,
                                              ClayItem          **read_only_item);
