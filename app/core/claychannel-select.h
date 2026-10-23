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


/*  basic selection functions  */

void   clay_channel_select_rectangle    (ClayChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   clay_channel_select_ellipse      (ClayChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   clay_channel_select_round_rect   (ClayChannel         *channel,
                                         gint                 x,
                                         gint                 y,
                                         gint                 w,
                                         gint                 h,
                                         gdouble              corner_radius_x,
                                         gdouble              corner_radius_y,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);

/*  select by ClayScanConvert functions  */

void   clay_channel_select_scan_convert (ClayChannel         *channel,
                                         const gchar         *undo_desc,
                                         ClayScanConvert     *scan_convert,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   clay_channel_select_polygon      (ClayChannel         *channel,
                                         const gchar         *undo_desc,
                                         gint                 n_points,
                                         const ClayVector2   *points,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   clay_channel_select_path         (ClayChannel         *channel,
                                         const gchar         *undo_desc,
                                         ClayPath            *path,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y,
                                         gboolean             push_undo);
void   clay_channel_select_buffer       (ClayChannel         *channel,
                                         const gchar         *undo_desc,
                                         GeglBuffer          *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);


/*  select by ClayChannel functions  */

void   clay_channel_select_channel      (ClayChannel         *channel,
                                         const gchar         *undo_desc,
                                         ClayChannel         *add_on,
                                         gint                 offset_x,
                                         gint                 offset_y,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   clay_channel_select_alpha        (ClayChannel         *channel,
                                         ClayDrawable        *drawable,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   clay_channel_select_component    (ClayChannel         *channel,
                                         ClayChannelType      component,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   clay_channel_select_fuzzy        (ClayChannel         *channel,
                                         ClayDrawable        *drawable,
                                         gboolean             sample_merged,
                                         gint                 x,
                                         gint                 y,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         ClaySelectCriterion  select_criterion,
                                         gboolean             diagonal_neighbors,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   clay_channel_select_by_color     (ClayChannel         *channel,
                                         GList               *drawables,
                                         gboolean             sample_merged,
                                         GeglColor           *color,
                                         gfloat               threshold,
                                         gboolean             select_transparent,
                                         ClaySelectCriterion  select_criterion,
                                         ClayChannelOps       op,
                                         gboolean             antialias,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
void   clay_channel_select_by_index     (ClayChannel         *channel,
                                         ClayDrawable        *drawable,
                                         gint                 index,
                                         ClayChannelOps       op,
                                         gboolean             feather,
                                         gdouble              feather_radius_x,
                                         gdouble              feather_radius_y);
