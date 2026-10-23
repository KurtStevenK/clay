/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-3d-transform-utils.c
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

#include "config.h"

#include <glib-object.h>

#include "libclaymath/claymath.h"

#include "core-types.h"

#include "clay-transform-3d-utils.h"


#define MIN_FOCAL_LENGTH 0.01


gdouble
clay_transform_3d_angle_of_view_to_focal_length (gdouble angle_of_view,
                                                 gdouble width,
                                                 gdouble height)
{
  return MAX (width, height) / (2.0 * tan (angle_of_view / 2.0));
}

gdouble
clay_transform_3d_focal_length_to_angle_of_view (gdouble focal_length,
                                                 gdouble width,
                                                 gdouble height)
{
  return 2.0 * atan (MAX (width, height) / (2.0 * focal_length));
}

gint
clay_transform_3d_permutation_to_rotation_order (const gint permutation[3])
{
  if (permutation[1] == (permutation[0] + 1) % 3)
    return permutation[0] << 1;
  else
    return (permutation[2] << 1) + 1;
}

void
clay_transform_3d_rotation_order_to_permutation (gint rotation_order,
                                                 gint permutation[3])
{
  gboolean reverse = rotation_order & 1;
  gint     shift   = rotation_order >> 1;
  gint i;

  for (i = 0; i < 3; i++)
    permutation[reverse ? 2 - i : i] = (i + shift) % 3;
}

gint
clay_transform_3d_rotation_order_reverse (gint rotation_order)
{
  return rotation_order ^ 1;
}

void
clay_transform_3d_vector3_rotate (ClayVector3       *vector,
                                  const ClayVector3 *axis)
{
  ClayVector3 normal;
  ClayVector3 proj;
  ClayVector3 u, v;
  gdouble     angle;

  angle = clay_vector3_length (axis);

  if (angle == 0.0)
    return;

  normal = clay_vector3_mul_val (*axis, 1.0 / angle);

  proj = clay_vector3_mul_val (normal,
                               clay_vector3_inner_product_val (*vector,
                                                               normal));

  u = clay_vector3_sub_val (*vector, proj);
  v = clay_vector3_cross_product_val (u, normal);

  clay_vector3_mul (&u, cos (angle));
  clay_vector3_mul (&v, sin (angle));

  *vector = proj;

  clay_vector3_add (vector, vector, &u);
  clay_vector3_add (vector, vector, &v);
}

ClayVector3
clay_transform_3d_vector3_rotate_val (ClayVector3 vector,
                                      ClayVector3 axis)
{
  clay_transform_3d_vector3_rotate (&vector, &axis);

  return vector;
}

void
clay_transform_3d_matrix3_to_matrix4 (const ClayMatrix3 *matrix3,
                                      ClayMatrix4       *matrix4,
                                      gint               axis)
{
  gint i, j;
  gint k, l;

  for (i = 0; i < 4; i++)
    {
      if (i == axis)
        {
          matrix4->coeff[i][i] = 1.0;
        }
      else
        {
          matrix4->coeff[axis][i] = 0.0;
          matrix4->coeff[i][axis] = 0.0;
        }
    }

  for (i = 0; i < 3; i++)
    {
      k = i + (i >= axis);

      for (j = 0; j < 3; j++)
        {
          l = j + (j >= axis);

          matrix4->coeff[k][l] = matrix3->coeff[i][j];
        }
    }
}

void
clay_transform_3d_matrix4_to_matrix3 (const ClayMatrix4 *matrix4,
                                      ClayMatrix3       *matrix3,
                                      gint               axis)
{
  gint i, j;
  gint k, l;

  for (i = 0; i < 3; i++)
    {
      k = i + (i >= axis);

      for (j = 0; j < 3; j++)
        {
          l = j + (j >= axis);

          matrix3->coeff[i][j] = matrix4->coeff[k][l];
        }
    }
}

void
clay_transform_3d_matrix4_translate (ClayMatrix4 *matrix,
                                     gdouble      x,
                                     gdouble      y,
                                     gdouble      z)
{
  gint i;

  for (i = 0; i < 4; i++)
    matrix->coeff[0][i] += x * matrix->coeff[3][i];

  for (i = 0; i < 4; i++)
    matrix->coeff[1][i] += y * matrix->coeff[3][i];

  for (i = 0; i < 4; i++)
    matrix->coeff[2][i] += z * matrix->coeff[3][i];
}

void
clay_transform_3d_matrix4_rotate (ClayMatrix4       *matrix,
                                  const ClayVector3 *axis)
{
  ClayMatrix4 rotation;
  ClayVector3 v;

  v = clay_transform_3d_vector3_rotate_val ((ClayVector3) {1.0, 0.0, 0.0},
                                            *axis);

  rotation.coeff[0][0] = v.x;
  rotation.coeff[1][0] = v.y;
  rotation.coeff[2][0] = v.z;
  rotation.coeff[3][0] = 0.0;

  v = clay_transform_3d_vector3_rotate_val ((ClayVector3) {0.0, 1.0, 0.0},
                                            *axis);

  rotation.coeff[0][1] = v.x;
  rotation.coeff[1][1] = v.y;
  rotation.coeff[2][1] = v.z;
  rotation.coeff[3][1] = 0.0;

  v = clay_transform_3d_vector3_rotate_val ((ClayVector3) {0.0, 0.0, 1.0},
                                            *axis);

  rotation.coeff[0][2] = v.x;
  rotation.coeff[1][2] = v.y;
  rotation.coeff[2][2] = v.z;
  rotation.coeff[3][2] = 0.0;

  rotation.coeff[0][3] = 0.0;
  rotation.coeff[1][3] = 0.0;
  rotation.coeff[2][3] = 0.0;
  rotation.coeff[3][3] = 1.0;

  clay_matrix4_mult (&rotation, matrix);
}

void
clay_transform_3d_matrix4_rotate_standard (ClayMatrix4 *matrix,
                                           gint         axis,
                                           gdouble      angle)
{
  gdouble v[3] = {};

  v[axis] = angle;

  clay_transform_3d_matrix4_rotate (matrix, &(ClayVector3) {v[0], v[1], v[2]});
}

void
clay_transform_3d_matrix4_rotate_euler (ClayMatrix4 *matrix,
                                        gint         rotation_order,
                                        gdouble      angle_x,
                                        gdouble      angle_y,
                                        gdouble      angle_z,
                                        gdouble      pivot_x,
                                        gdouble      pivot_y,
                                        gdouble      pivot_z)
{
  const gdouble angles[3] = {angle_x, angle_y, angle_z};
  gint          permutation[3];
  gint          i;

  clay_transform_3d_rotation_order_to_permutation (rotation_order, permutation);

  clay_transform_3d_matrix4_translate (matrix, -pivot_x, -pivot_y, -pivot_z);

  for (i = 0; i < 3; i++)
    {
      clay_transform_3d_matrix4_rotate_standard (matrix,
                                                 permutation[i],
                                                 angles[permutation[i]]);
    }

  clay_transform_3d_matrix4_translate (matrix, +pivot_x, +pivot_y, +pivot_z);
}

void
clay_transform_3d_matrix4_rotate_euler_decompose (ClayMatrix4 *matrix,
                                                  gint         rotation_order,
                                                  gdouble     *angle_x,
                                                  gdouble     *angle_y,
                                                  gdouble     *angle_z)
{
  ClayMatrix4     m = *matrix;
  gdouble * const angles[3] = {angle_x, angle_y, angle_z};
  gint            permutation[3];
  gboolean        forward;

  clay_transform_3d_rotation_order_to_permutation (rotation_order, permutation);

  forward = permutation[1] == (permutation[0] + 1) % 3;

  *angles[permutation[2]] = atan2 (m.coeff[permutation[1]][permutation[0]],
                                   m.coeff[permutation[0]][permutation[0]]);

  if (forward)
    *angles[permutation[2]] *= -1.0;

  clay_transform_3d_matrix4_rotate_standard (&m,
                                             permutation[2],
                                             -*angles[permutation[2]]);

  *angles[permutation[1]] = atan2 (m.coeff[permutation[2]][permutation[0]],
                                   m.coeff[permutation[0]][permutation[0]]);

  if (! forward)
    *angles[permutation[1]] *= -1.0;

  clay_transform_3d_matrix4_rotate_standard (&m,
                                             permutation[1],
                                             -*angles[permutation[1]]);

  *angles[permutation[0]] = atan2 (m.coeff[permutation[2]][permutation[1]],
                                   m.coeff[permutation[1]][permutation[1]]);

  if (forward)
    *angles[permutation[0]] *= -1.0;
}

void
clay_transform_3d_matrix4_perspective (ClayMatrix4 *matrix,
                                       gdouble      camera_x,
                                       gdouble      camera_y,
                                       gdouble      camera_z)
{
  gint i;

  camera_z = MIN (camera_z, -MIN_FOCAL_LENGTH);

  clay_transform_3d_matrix4_translate (matrix, -camera_x, -camera_y, 0.0);

  for (i = 0; i < 4; i++)
    matrix->coeff[3][i] += matrix->coeff[2][i] / -camera_z;

  clay_transform_3d_matrix4_translate (matrix, +camera_x, +camera_y, 0.0);
}

void
clay_transform_3d_matrix (ClayMatrix3 *matrix,
                          gdouble      camera_x,
                          gdouble      camera_y,
                          gdouble      camera_z,
                          gdouble      offset_x,
                          gdouble      offset_y,
                          gdouble      offset_z,
                          gint         rotation_order,
                          gdouble      angle_x,
                          gdouble      angle_y,
                          gdouble      angle_z,
                          gdouble      pivot_x,
                          gdouble      pivot_y,
                          gdouble      pivot_z)
{
  ClayMatrix4 m;

  clay_matrix4_identity (&m);
  clay_transform_3d_matrix4_rotate_euler (&m,
                                          rotation_order,
                                          angle_x, angle_y, angle_z,
                                          pivot_x, pivot_y, pivot_z);
  clay_transform_3d_matrix4_translate (&m, offset_x, offset_y, offset_z);
  clay_transform_3d_matrix4_perspective (&m, camera_x, camera_y, camera_z);

  clay_transform_3d_matrix4_to_matrix3 (&m, matrix, 2);
}
