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


/*  image undos  */

ClayUndo * clay_image_undo_push_image_type          (ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_precision     (ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_size          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     gint           previous_origin_x,
                                                     gint           previous_origin_y,
                                                     gint           previous_width,
                                                     gint           prevoius_height);
ClayUndo * clay_image_undo_push_image_resolution    (ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_grid          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayGrid      *grid);
ClayUndo * clay_image_undo_push_image_colormap      (ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_hidden_profile(ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_metadata      (ClayImage     *image,
                                                     const gchar   *undo_desc);
ClayUndo * clay_image_undo_push_image_parasite      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     const ClayParasite *parasite);
ClayUndo * clay_image_undo_push_image_parasite_remove (ClayImage   *image,
                                                     const gchar   *undo_desc,
                                                     const gchar   *name);


/*  guide & sample point undos  */

ClayUndo * clay_image_undo_push_guide               (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayGuide     *guide);
ClayUndo * clay_image_undo_push_sample_point        (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClaySamplePoint *sample_point);


/*  drawable undos  */

ClayUndo * clay_image_undo_push_drawable            (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     GeglBuffer    *buffer,
                                                     gint           x,
                                                     gint           y);
ClayUndo * clay_image_undo_push_drawable_mod        (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     gboolean       copy_buffer);
ClayUndo * clay_image_undo_push_drawable_format     (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable);


/*  drawable filter undos  */

ClayUndo * clay_image_undo_push_filter_add          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     ClayDrawableFilter
                                                                   *filter);

ClayUndo * clay_image_undo_push_filter_remove       (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     ClayDrawableFilter
                                                                   *filter);

ClayUndo * clay_image_undo_push_filter_reorder      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     ClayDrawableFilter
                                                                   *filter);
ClayUndo * clay_image_undo_push_filter_modified     (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     ClayDrawableFilter
                                                                   *filter);
ClayUndo * clay_image_undo_push_filter_visibility   (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayDrawable  *drawable,
                                                     ClayDrawableFilter
                                                                   *filter);

ClayUndo * clay_image_undo_push_filter_visibility_compressible (ClayDrawableFilter
                                                                   *filter,
                                                                ClayContext
                                                                   *context);

/*  mask undos  */

ClayUndo * clay_image_undo_push_mask                (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayChannel   *mask);
ClayUndo * clay_image_undo_push_mask_precision      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayChannel   *mask);


/*  item undos  */

ClayUndo * clay_image_undo_push_item_reorder        (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_rename         (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_displace       (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_visibility     (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_color_tag      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_lock_content   (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_lock_position  (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item);
ClayUndo * clay_image_undo_push_item_lock_visibility (ClayImage    *image,
                                                      const gchar  *undo_desc,
                                                      ClayItem     *item);
ClayUndo * clay_image_undo_push_item_parasite       (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item,
                                                     const ClayParasite *parasite);
ClayUndo * clay_image_undo_push_item_parasite_remove(ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayItem      *item,
                                                     const gchar   *name);


/*  layer undos  */

ClayUndo * clay_image_undo_push_layer_add           (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer,
                                                     GList         *prev_layers);
ClayUndo * clay_image_undo_push_layer_remove        (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer,
                                                     ClayLayer     *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_layers);
ClayUndo * clay_image_undo_push_layer_mode          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer);
ClayUndo * clay_image_undo_push_layer_opacity       (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer);
ClayUndo * clay_image_undo_push_layer_lock_alpha    (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer);


/*  group layer undos  */

ClayUndo *
    clay_image_undo_push_group_layer_suspend_resize (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo *
     clay_image_undo_push_group_layer_resume_resize (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo *
      clay_image_undo_push_group_layer_suspend_mask (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo *
       clay_image_undo_push_group_layer_resume_mask (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo *
   clay_image_undo_push_group_layer_start_transform (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo *
     clay_image_undo_push_group_layer_end_transform (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);
ClayUndo * clay_image_undo_push_group_layer_convert (ClayImage      *image,
                                                     const gchar    *undo_desc,
                                                     ClayGroupLayer *group);

/*  rasterizable undos  */

ClayUndo * clay_image_undo_push_rasterizable        (ClayImage        *image,
                                                     const gchar      *undo_desc,
                                                     ClayRasterizable *rasterizable);

/*  text layer undos  */

ClayUndo * clay_image_undo_push_text_layer          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayTextLayer *layer,
                                                     const GParamSpec *pspec);
ClayUndo * clay_image_undo_push_text_layer_convert  (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayTextLayer *layer);

/*  link layer undos  */

ClayUndo * clay_image_undo_push_link_layer          (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLinkLayer *layer);

/*  vector layer undos  */

ClayUndo * clay_image_undo_push_vector_layer        (ClayImage        *image,
                                                     const gchar      *undo_desc,
                                                     ClayVectorLayer  *layer,
                                                     const GParamSpec *pspec);

/*  layer mask undos  */

ClayUndo * clay_image_undo_push_layer_mask_add      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer,
                                                     ClayLayerMask *mask);
ClayUndo * clay_image_undo_push_layer_mask_remove   (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer,
                                                     ClayLayerMask *mask);
ClayUndo * clay_image_undo_push_layer_mask_apply    (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer);
ClayUndo * clay_image_undo_push_layer_mask_show     (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *layer);


/*  channel undos  */

ClayUndo * clay_image_undo_push_channel_add         (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayChannel   *channel,
                                                     GList         *prev_channels);
ClayUndo * clay_image_undo_push_channel_remove      (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayChannel   *channel,
                                                     ClayChannel   *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_channels);
ClayUndo * clay_image_undo_push_channel_color       (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayChannel   *channel);


/*  path undos  */

ClayUndo * clay_image_undo_push_path_add            (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayPath      *path,
                                                     GList         *prev_paths);
ClayUndo * clay_image_undo_push_path_remove         (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayPath      *path,
                                                     ClayPath      *prev_parent,
                                                     gint           prev_position,
                                                     GList         *prev_paths);
ClayUndo * clay_image_undo_push_path_mod            (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayPath      *path);


/*  floating selection undos  */

ClayUndo * clay_image_undo_push_fs_to_layer         (ClayImage     *image,
                                                     const gchar   *undo_desc,
                                                     ClayLayer     *floating_layer);


/*  EEK undo  */

ClayUndo * clay_image_undo_push_cantundo            (ClayImage     *image,
                                                     const gchar   *undo_desc);
