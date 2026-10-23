/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-3d-transform-utils.h
 * Copyright (C) 2019 Ell
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


gdouble       clay_transform_3d_angle_of_view_to_focal_length   (gdouble            angle_of_view,
                                                                 gdouble            width,
                                                                 gdouble            height);
gdouble       clay_transform_3d_focal_length_to_angle_of_view   (gdouble            focal_length,
                                                                 gdouble            width,
                                                                 gdouble            height);

gint          clay_transform_3d_permutation_to_rotation_order   (const gint         permutation[3]);
void          clay_transform_3d_rotation_order_to_permutation   (gint               rotation_order,
                                                                 gint               permutation[3]);
gint          clay_transform_3d_rotation_order_reverse          (gint               rotation_order);

void          clay_transform_3d_vector3_rotate                  (ClayVector3       *vector,
                                                                 const ClayVector3 *axis);
ClayVector3   clay_transform_3d_vector3_rotate_val              (ClayVector3        vector,
                                                                 ClayVector3        axis);

void          clay_transform_3d_matrix3_to_matrix4              (const ClayMatrix3 *matrix3,
                                                                 ClayMatrix4       *matrix4,
                                                                 gint               axis);
void          clay_transform_3d_matrix4_to_matrix3              (const ClayMatrix4 *matrix4,
                                                                 ClayMatrix3       *matrix3,
                                                                 gint               axis);

void          clay_transform_3d_matrix4_translate               (ClayMatrix4       *matrix,
                                                                 gdouble            x,
                                                                 gdouble            y,
                                                                 gdouble            z);

void          clay_transform_3d_matrix4_rotate                  (ClayMatrix4       *matrix,
                                                                 const ClayVector3 *axis);
void          clay_transform_3d_matrix4_rotate_standard         (ClayMatrix4       *matrix,
                                                                 gint               axis,
                                                                 gdouble            angle);

void          clay_transform_3d_matrix4_rotate_euler            (ClayMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble            angle_x,
                                                                 gdouble            angle_y,
                                                                 gdouble            angle_z,
                                                                 gdouble            pivot_x,
                                                                 gdouble            pivot_y,
                                                                 gdouble            pivot_z);
void          clay_transform_3d_matrix4_rotate_euler_decompose  (ClayMatrix4       *matrix,
                                                                 gint               rotation_order,
                                                                 gdouble           *angle_x,
                                                                 gdouble           *angle_y,
                                                                 gdouble           *angle_z);

void          clay_transform_3d_matrix4_perspective             (ClayMatrix4       *matrix,
                                                                 gdouble            camera_x,
                                                                 gdouble            camera_y,
                                                                 gdouble            camera_z);

void          clay_transform_3d_matrix                          (ClayMatrix3       *matrix,
                                                                 gdouble            camera_x,
                                                                 gdouble            camera_y,
                                                                 gdouble            camera_z,
                                                                 gdouble            offset_x,
                                                                 gdouble            offset_y,
                                                                 gdouble            offset_z,
                                                                 gint               rotation_order,
                                                                 gdouble            angle_x,
                                                                 gdouble            angle_y,
                                                                 gdouble            angle_z,
                                                                 gdouble            pivot_x,
                                                                 gdouble            pivot_y,
                                                                 gdouble            pivot_z);
