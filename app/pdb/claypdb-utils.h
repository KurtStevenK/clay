/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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


GList           * clay_pdb_get_resources          (Clay               *clay,
                                                   GType               data_type,
                                                   const gchar        *name,
                                                   ClayPDBDataAccess   access,
                                                   GError            **error);
ClayResource    * clay_pdb_get_resource           (Clay               *clay,
                                                   GType               data_type,
                                                   const gchar        *name,
                                                   ClayPDBDataAccess   access,
                                                   GError            **error);
ClayResource    * clay_pdb_get_resource_by_id     (Clay               *clay,
                                                   GType               data_type,
                                                   const gchar        *name,
                                                   const gchar        *collection,
                                                   gboolean            is_internal,
                                                   ClayPDBDataAccess   access,
                                                   GError            **error);
ClayBrush       * clay_pdb_get_generated_brush    (Clay               *clay,
                                                   const gchar        *name,
                                                   ClayPDBDataAccess   access,
                                                   GError            **error);

ClayBuffer      * clay_pdb_get_buffer             (Clay               *clay,
                                                   const gchar        *name,
                                                   GError            **error);
ClayPaintInfo   * clay_pdb_get_paint_info         (Clay               *clay,
                                                   const gchar        *name,
                                                   GError            **error);

gboolean          clay_pdb_item_is_attached       (ClayItem           *item,
                                                   ClayImage          *image,
                                                   ClayPDBItemModify   modify,
                                                   GError            **error);
gboolean          clay_pdb_item_is_in_tree        (ClayItem           *item,
                                                   ClayImage          *image,
                                                   ClayPDBItemModify   modify,
                                                   GError            **error);
gboolean          clay_pdb_item_is_in_same_tree   (ClayItem           *item,
                                                   ClayItem           *item2,
                                                   ClayImage          *image,
                                                   GError            **error);
gboolean          clay_pdb_item_is_not_ancestor   (ClayItem           *item,
                                                   ClayItem           *not_descendant,
                                                   GError            **error);
gboolean          clay_pdb_item_is_floating       (ClayItem           *item,
                                                   ClayImage          *dest_image,
                                                   GError            **error);
gboolean          clay_pdb_item_is_modifiable     (ClayItem           *item,
                                                   ClayPDBItemModify   modify,
                                                   GError            **error);
gboolean          clay_pdb_item_is_group          (ClayItem           *item,
                                                   GError            **error);
gboolean          clay_pdb_item_is_not_group      (ClayItem           *item,
                                                   GError            **error);

gboolean          clay_pdb_layer_is_text_layer    (ClayLayer          *layer,
                                                   ClayPDBItemModify   modify,
                                                   GError            **error);

gboolean          clay_pdb_image_is_base_type     (ClayImage          *image,
                                                   ClayImageBaseType   type,
                                                   GError            **error);
gboolean          clay_pdb_image_is_not_base_type (ClayImage          *image,
                                                   ClayImageBaseType   type,
                                                   GError            **error);

gboolean          clay_pdb_image_is_precision     (ClayImage          *image,
                                                   ClayPrecision       precision,
                                                   GError            **error);
gboolean          clay_pdb_image_is_not_precision (ClayImage          *image,
                                                   ClayPrecision       precision,
                                                   GError            **error);

ClayGuide       * clay_pdb_image_get_guide        (ClayImage          *image,
                                                   gint                guide_id,
                                                   GError            **error);
ClaySamplePoint * clay_pdb_image_get_sample_point (ClayImage          *image,
                                                   gint                sample_point_id,
                                                   GError            **error);

ClayStroke      * clay_pdb_get_path_stroke        (ClayPath           *path,
                                                   gint                stroke_id,
                                                   ClayPDBItemModify   modify,
                                                   GError            **error);

gboolean          clay_pdb_is_canonical_procedure (const gchar        *procedure_name,
                                                   GError            **error);
