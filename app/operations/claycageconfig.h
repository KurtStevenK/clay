/* xyz.kainzmayer.ClayDesign
 *
 * claycageconfig.h
 * Copyright (C) 2010 Michael Muré <batolettre@gmail.com>
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

#include "clayoperationsettings.h"


struct _ClayCagePoint
{
  ClayVector2 src_point;
  ClayVector2 dest_point;
  ClayVector2 edge_normal;
  gdouble     edge_scaling_factor;
  gboolean    selected;
};


#define CLAY_TYPE_CAGE_CONFIG            (clay_cage_config_get_type ())
#define CLAY_CAGE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CAGE_CONFIG, ClayCageConfig))
#define CLAY_CAGE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CAGE_CONFIG, ClayCageConfigClass))
#define CLAY_IS_CAGE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CAGE_CONFIG))
#define CLAY_IS_CAGE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CAGE_CONFIG))
#define CLAY_CAGE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CAGE_CONFIG, ClayCageConfigClass))


typedef struct _ClayCageConfigClass ClayCageConfigClass;

struct _ClayCageConfig
{
  ClayOperationSettings  parent_instance;

  GArray                *cage_points;

  gdouble                displacement_x;
  gdouble                displacement_y;
  ClayCageMode           cage_mode;  /* Cage mode, used to commit displacement */
};

struct _ClayCageConfigClass
{
  ClayOperationSettingsClass  parent_class;
};


GType           clay_cage_config_get_type               (void);

guint           clay_cage_config_get_n_points           (ClayCageConfig  *gcc);
void            clay_cage_config_add_cage_point         (ClayCageConfig  *gcc,
                                                         gdouble          x,
                                                         gdouble          y);
void            clay_cage_config_insert_cage_point      (ClayCageConfig  *gcc,
                                                         gint             point_number,
                                                         gdouble          x,
                                                         gdouble          y);
void            clay_cage_config_remove_last_cage_point (ClayCageConfig  *gcc);
void            clay_cage_config_remove_cage_point      (ClayCageConfig  *gcc,
                                                         gint             point_number);
void            clay_cage_config_remove_selected_points (ClayCageConfig  *gcc);
ClayVector2     clay_cage_config_get_point_coordinate   (ClayCageConfig  *gcc,
                                                         ClayCageMode     mode,
                                                         gint             point_number);
void            clay_cage_config_add_displacement       (ClayCageConfig  *gcc,
                                                         ClayCageMode     mode,
                                                         gdouble          x,
                                                         gdouble          y);
void            clay_cage_config_commit_displacement    (ClayCageConfig  *gcc);
void            clay_cage_config_reset_displacement     (ClayCageConfig  *gcc);
GeglRectangle   clay_cage_config_get_bounding_box       (ClayCageConfig  *gcc);
void            clay_cage_config_reverse_cage_if_needed (ClayCageConfig  *gcc);
void            clay_cage_config_reverse_cage           (ClayCageConfig  *gcc);
gboolean        clay_cage_config_point_inside           (ClayCageConfig  *gcc,
                                                         gfloat           x,
                                                         gfloat           y);
void            clay_cage_config_select_point           (ClayCageConfig  *gcc,
                                                         gint             point_number);
void            clay_cage_config_select_area            (ClayCageConfig  *gcc,
                                                         ClayCageMode     mode,
                                                         GeglRectangle    area);
void            clay_cage_config_select_add_area        (ClayCageConfig  *gcc,
                                                         ClayCageMode     mode,
                                                         GeglRectangle    area);
void            clay_cage_config_toggle_point_selection (ClayCageConfig  *gcc,
                                                         gint             point_number);
void            clay_cage_config_deselect_points        (ClayCageConfig  *gcc);
gboolean        clay_cage_config_point_is_selected      (ClayCageConfig  *gcc,
                                                         gint             point_number);
