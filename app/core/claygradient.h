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

#include "claydata.h"


#define CLAY_GRADIENT_DEFAULT_SAMPLE_SIZE 40


struct _ClayGradientSegment
{
  gdouble                  left, middle, right;

  ClayGradientColor        left_color_type;
  GeglColor               *left_color;
  ClayGradientColor        right_color_type;
  GeglColor               *right_color;

  ClayGradientSegmentType  type;          /*  Segment's blending function  */
  ClayGradientSegmentColor color;         /*  Segment's coloring type      */

  ClayGradientSegment     *prev;
  ClayGradientSegment     *next;
};


#define CLAY_TYPE_GRADIENT            (clay_gradient_get_type ())
#define CLAY_GRADIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRADIENT, ClayGradient))
#define CLAY_GRADIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRADIENT, ClayGradientClass))
#define CLAY_IS_GRADIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRADIENT))
#define CLAY_IS_GRADIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRADIENT))
#define CLAY_GRADIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRADIENT, ClayGradientClass))


typedef struct _ClayGradientClass ClayGradientClass;

struct _ClayGradient
{
  ClayData             parent_instance;

  ClayGradientSegment *segments;
};

struct _ClayGradientClass
{
  ClayDataClass  parent_class;
};


GType                 clay_gradient_get_type       (void);

ClayData            * clay_gradient_new            (ClayContext   *context,
                                                    const gchar   *name);
ClayData            * clay_gradient_get_standard   (ClayContext   *context);

ClayGradientSegment * clay_gradient_get_color_at   (ClayGradient                 *gradient,
                                                    ClayContext                  *context,
                                                    ClayGradientSegment          *seg,
                                                    gdouble                       pos,
                                                    gboolean                      reverse,
                                                    ClayGradientBlendColorSpace   blend_color_space,
                                                    GeglColor                   **color);
ClayGradientSegment * clay_gradient_get_segment_at (ClayGradient                 *grad,
                                                    gdouble                       pos);
void                  clay_gradient_split_at       (ClayGradient                 *gradient,
                                                    ClayContext                  *context,
                                                    ClayGradientSegment          *seg,
                                                    gdouble                       pos,
                                                    ClayGradientBlendColorSpace   blend_color_space,
                                                    ClayGradientSegment         **newl,
                                                    ClayGradientSegment         **newr);

gboolean          clay_gradient_has_fg_bg_segments (ClayGradient  *gradient);
ClayGradient    * clay_gradient_flatten            (ClayGradient  *gradient,
                                                    ClayContext   *context);


/*  gradient segment functions  */

ClayGradientSegment * clay_gradient_segment_new       (void);
ClayGradientSegment * clay_gradient_segment_get_last  (ClayGradientSegment *seg);
ClayGradientSegment * clay_gradient_segment_get_first (ClayGradientSegment *seg);
ClayGradientSegment * clay_gradient_segment_get_nth   (ClayGradientSegment *seg,
                                                       gint                 index);

void                  clay_gradient_segment_free      (ClayGradientSegment *seg);
void                  clay_gradient_segments_free     (ClayGradientSegment *seg);

void                  clay_gradient_segment_split_midpoint  (ClayGradient         *gradient,
                                                             ClayContext          *context,
                                                             ClayGradientSegment  *lseg,
                                                             ClayGradientBlendColorSpace blend_color_space,
                                                             ClayGradientSegment **newl,
                                                             ClayGradientSegment **newr);
void                  clay_gradient_segment_split_uniform   (ClayGradient         *gradient,
                                                             ClayContext          *context,
                                                             ClayGradientSegment  *lseg,
                                                             gint                  parts,
                                                             ClayGradientBlendColorSpace blend_color_space,
                                                             ClayGradientSegment **newl,
                                                             ClayGradientSegment **newr);

/* Colors Setting/Getting Routines */
GeglColor           * clay_gradient_segment_get_left_color  (ClayGradient         *gradient,
                                                             ClayGradientSegment  *seg);

void                  clay_gradient_segment_set_left_color  (ClayGradient         *gradient,
                                                             ClayGradientSegment  *seg,
                                                             GeglColor            *color);


GeglColor           * clay_gradient_segment_get_right_color (ClayGradient         *gradient,
                                                             ClayGradientSegment  *seg);

void                  clay_gradient_segment_set_right_color (ClayGradient         *gradient,
                                                             ClayGradientSegment  *seg,
                                                             GeglColor            *color);


ClayGradientColor
clay_gradient_segment_get_left_color_type     (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);

void
clay_gradient_segment_set_left_color_type     (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg,
                                               ClayGradientColor     color_type);


ClayGradientColor
clay_gradient_segment_get_right_color_type    (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);

void
clay_gradient_segment_set_right_color_type    (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg,
                                               ClayGradientColor     color_type);


GeglColor *
clay_gradient_segment_get_left_flat_color     (ClayGradient         *gradient,
                                               ClayContext          *context,
                                               ClayGradientSegment  *seg);


GeglColor *
clay_gradient_segment_get_right_flat_color    (ClayGradient         *gradient,
                                               ClayContext          *context,
                                               ClayGradientSegment  *seg);


/* Position Setting/Getting Routines */
/* (Setters return the position after it was set) */
gdouble clay_gradient_segment_get_left_pos    (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);
gdouble clay_gradient_segment_set_left_pos    (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg,
                                               gdouble               pos);

gdouble clay_gradient_segment_get_right_pos   (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);
gdouble clay_gradient_segment_set_right_pos   (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg,
                                               gdouble               pos);

gdouble clay_gradient_segment_get_middle_pos  (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);
gdouble clay_gradient_segment_set_middle_pos  (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg,
                                               gdouble               pos);

/* Getting/Setting the Blending Function/Coloring Type */
ClayGradientSegmentType
clay_gradient_segment_get_blending_function   (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);
ClayGradientSegmentColor
clay_gradient_segment_get_coloring_type       (ClayGradient         *gradient,
                                               ClayGradientSegment  *seg);

/*
 * If the second segment is NULL, these functions will process
 * until the end of the string.
 * */
gint    clay_gradient_segment_range_get_n_segments
                                              (ClayGradient         *gradient,
                                               ClayGradientSegment  *range_l,
                                               ClayGradientSegment  *range_r);

void    clay_gradient_segment_range_compress  (ClayGradient         *gradient,
                                               ClayGradientSegment  *range_l,
                                               ClayGradientSegment  *range_r,
                                               gdouble               new_l,
                                               gdouble               new_r);
void    clay_gradient_segment_range_blend     (ClayGradient         *gradient,
                                               ClayGradientSegment  *lseg,
                                               ClayGradientSegment  *rseg,
                                               GeglColor            *color1,
                                               GeglColor            *color2,
                                               gboolean              blend_colors,
                                               gboolean              blend_opacity);

void    clay_gradient_segment_range_set_blending_function
                                              (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientSegmentType new_type);

void    clay_gradient_segment_range_set_coloring_type
                                              (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientSegmentColor new_color);

void    clay_gradient_segment_range_flip      (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_replicate (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               gint                  replicate_times,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_split_midpoint
                                              (ClayGradient         *gradient,
                                               ClayContext          *context,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientBlendColorSpace blend_color_space,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_split_uniform
                                              (ClayGradient         *gradient,
                                               ClayContext          *context,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               gint                  parts,
                                               ClayGradientBlendColorSpace blend_color_space,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_delete    (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_merge     (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg,
                                               ClayGradientSegment **final_start_seg,
                                               ClayGradientSegment **final_end_seg);

void    clay_gradient_segment_range_recenter_handles
                                              (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg);
void    clay_gradient_segment_range_redistribute_handles
                                              (ClayGradient         *gradient,
                                               ClayGradientSegment  *start_seg,
                                               ClayGradientSegment  *end_seg);

gdouble clay_gradient_segment_range_move      (ClayGradient         *gradient,
                                               ClayGradientSegment  *range_l,
                                               ClayGradientSegment  *range_r,
                                               gdouble               delta,
                                               gboolean              control_compress);
