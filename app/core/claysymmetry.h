/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysymmetry.h
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include "clayobject.h"


/* shift one more than latest CLAY_CONFIG_PARAM_* */
#define CLAY_SYMMETRY_PARAM_GUI (1 << (0 + CLAY_CONFIG_PARAM_FLAG_SHIFT))


#define CLAY_TYPE_SYMMETRY            (clay_symmetry_get_type ())
#define CLAY_SYMMETRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SYMMETRY, ClaySymmetry))
#define CLAY_SYMMETRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SYMMETRY, ClaySymmetryClass))
#define CLAY_IS_SYMMETRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SYMMETRY))
#define CLAY_IS_SYMMETRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SYMMETRY))
#define CLAY_SYMMETRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SYMMETRY, ClaySymmetryClass))


typedef struct _ClaySymmetryClass   ClaySymmetryClass;

struct _ClaySymmetry
{
  ClayObject    parent_instance;

  ClayImage    *image;
  ClayDrawable *drawable;
  ClayCoords   *origin;
  gboolean      active;
  gint          version;

  GList        *strokes;
  gboolean      stateful;
};

struct _ClaySymmetryClass
{
  ClayObjectClass  parent_class;

  const gchar * label;

  /* Virtual functions */
  void       (* update_strokes)             (ClaySymmetry       *symmetry,
                                             ClayDrawable       *drawable,
                                             ClayCoords         *origin);
  void       (* get_transform)              (ClaySymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
  void       (* active_changed)             (ClaySymmetry       *symmetry);

  gboolean   (* update_version)             (ClaySymmetry       *symmetry);
};


GType          clay_symmetry_get_type       (void);

void           clay_symmetry_set_stateful   (ClaySymmetry       *symmetry,
                                             gboolean            stateful);
void           clay_symmetry_set_origin     (ClaySymmetry       *symmetry,
                                             ClayDrawable       *drawable,
                                             ClayCoords         *origin);
void           clay_symmetry_clear_origin   (ClaySymmetry       *symmetry);

ClayCoords   * clay_symmetry_get_origin     (ClaySymmetry       *symmetry);
gint           clay_symmetry_get_size       (ClaySymmetry       *symmetry);
ClayCoords   * clay_symmetry_get_coords     (ClaySymmetry       *symmetry,
                                             gint                stroke);
void           clay_symmetry_get_transform  (ClaySymmetry       *symmetry,
                                             gint                stroke,
                                             gdouble            *angle,
                                             gboolean           *reflect);
void           clay_symmetry_get_matrix     (ClaySymmetry       *symmetry,
                                             gint                stroke,
                                             ClayMatrix3        *matrix);
GeglNode     * clay_symmetry_get_operation  (ClaySymmetry       *symmetry,
                                             gint                stroke);

gchar        * clay_symmetry_parasite_name  (GType               type);
ClayParasite * clay_symmetry_to_parasite    (const ClaySymmetry *symmetry);
ClaySymmetry * clay_symmetry_from_parasite  (const ClayParasite *parasite,
                                             ClayImage          *image,
                                             GType               type);
