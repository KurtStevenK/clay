/* xyz.kainzmayer.ClayDesign
 *
 * claynpointdeformationoptions.h
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
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

#include "core/claytooloptions.h"


#define CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS            (clay_n_point_deformation_options_get_type ())
#define CLAY_N_POINT_DEFORMATION_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS, ClayNPointDeformationOptions))
#define CLAY_N_POINT_DEFORMATION_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS, ClayNPointDeformationOptionsClass))
#define CLAY_IS_N_POINT_DEFORMATION_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define CLAY_IS_N_POINT_DEFORMATION_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS))
#define CLAY_N_POINT_DEFORMATION_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_N_POINT_DEFORMATION_OPTIONS, ClayNPointDeformationOptionsClass))


typedef struct _ClayNPointDeformationOptions      ClayNPointDeformationOptions;
typedef struct _ClayNPointDeformationOptionsClass ClayNPointDeformationOptionsClass;

struct _ClayNPointDeformationOptions
{
  ClayToolOptions  parent_instance;

  gdouble          square_size;
  gdouble          rigidity;
  gboolean         asap_deformation;
  gboolean         mls_weights;
  gdouble          mls_weights_alpha;
  gboolean         mesh_visible;

  GtkWidget       *scale_square_size;
  GtkWidget       *check_mesh_visible;
};

struct _ClayNPointDeformationOptionsClass
{
  ClayToolOptionsClass  parent_class;
};


GType       clay_n_point_deformation_options_get_type (void);

GtkWidget * clay_n_point_deformation_options_gui      (ClayToolOptions *tool_options);

void        clay_n_point_deformation_options_set_sensitivity (ClayNPointDeformationOptions *npd_options,
                                                              gboolean                      tool_active);
