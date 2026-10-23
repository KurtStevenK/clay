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


/*  zoom: functions to transform from image space to unrotated display
 *  space and back, taking into account scroll offset and scale
 */

void  clay_display_shell_zoom_coords                   (ClayDisplayShell   *shell,
                                                        const ClayCoords   *image_coords,
                                                        ClayCoords         *display_coords);
void  clay_display_shell_unzoom_coords                 (ClayDisplayShell   *shell,
                                                        const ClayCoords   *display_coords,
                                                        ClayCoords         *image_coords);

void  clay_display_shell_zoom_xy                       (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  clay_display_shell_unzoom_xy                     (ClayDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny,
                                                        gboolean            round);

void  clay_display_shell_zoom_xy_f                     (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  clay_display_shell_unzoom_xy_f                   (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  clay_display_shell_zoom_segments                 (ClayDisplayShell   *shell,
                                                        const ClayBoundSeg *src_segs,
                                                        ClaySegment        *dest_segs,
                                                        gint                n_segs,
                                                        gdouble             offset_x,
                                                        gdouble             offset_y);


/*  rotate: functions to transform from unrotated and unflipped but
 *  zoomed display space to rotated and flipped display space and back
 */

void  clay_display_shell_rotate_coords                 (ClayDisplayShell   *shell,
                                                        const ClayCoords   *image_coords,
                                                        ClayCoords         *display_coords);
void  clay_display_shell_unrotate_coords               (ClayDisplayShell   *shell,
                                                        const ClayCoords   *display_coords,
                                                        ClayCoords         *image_coords);

void  clay_display_shell_rotate_xy                     (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  clay_display_shell_unrotate_xy                   (ClayDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny);

void  clay_display_shell_rotate_xy_f                   (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  clay_display_shell_unrotate_xy_f                 (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  clay_display_shell_rotate_bounds                 (ClayDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  clay_display_shell_unrotate_bounds               (ClayDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);


/*  transform: functions to transform from image space to rotated
 *  display space and back, taking into account scroll offset, scale,
 *  rotation and flipping
 */

void  clay_display_shell_transform_coords              (ClayDisplayShell   *shell,
                                                        const ClayCoords   *image_coords,
                                                        ClayCoords         *display_coords);
void  clay_display_shell_untransform_coords            (ClayDisplayShell   *shell,
                                                        const ClayCoords   *display_coords,
                                                        ClayCoords         *image_coords);

void  clay_display_shell_transform_xy                  (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gint               *nx,
                                                        gint               *ny);
void  clay_display_shell_untransform_xy                (ClayDisplayShell   *shell,
                                                        gint                x,
                                                        gint                y,
                                                        gint               *nx,
                                                        gint               *ny,
                                                        gboolean            round);

void  clay_display_shell_transform_xy_f                (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);
void  clay_display_shell_untransform_xy_f              (ClayDisplayShell   *shell,
                                                        gdouble             x,
                                                        gdouble             y,
                                                        gdouble            *nx,
                                                        gdouble            *ny);

void  clay_display_shell_transform_bounds              (ClayDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  clay_display_shell_untransform_bounds            (ClayDisplayShell   *shell,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);

void  clay_display_shell_transform_bounds_with_scale   (ClayDisplayShell   *shell,
                                                        gdouble             scale,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);
void  clay_display_shell_untransform_bounds_with_scale (ClayDisplayShell   *shell,
                                                        gdouble             scale,
                                                        gdouble             x1,
                                                        gdouble             y1,
                                                        gdouble             x2,
                                                        gdouble             y2,
                                                        gdouble            *nx1,
                                                        gdouble            *ny1,
                                                        gdouble            *nx2,
                                                        gdouble            *ny2);

void  clay_display_shell_untransform_viewport          (ClayDisplayShell   *shell,
                                                        gboolean            clip,
                                                        gint               *x,
                                                        gint               *y,
                                                        gint               *width,
                                                        gint               *height);
