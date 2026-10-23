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


GList       * clay_image_merge_visible_layers  (ClayImage      *image,
                                                ClayContext    *context,
                                                ClayMergeType   merge_type,
                                                gboolean        merge_active_group,
                                                gboolean        discard_invisible,
                                                ClayProgress   *progress);
GList       * clay_image_merge_down            (ClayImage      *image,
                                                GList          *layers,
                                                ClayContext    *context,
                                                ClayMergeType   merge_type,
                                                ClayProgress   *progress,
                                                ClayItem      **blink_item,
                                                GError        **error);
ClayLayer   * clay_image_merge_group_layer     (ClayImage      *image,
                                                ClayGroupLayer *group);

ClayLayer   * clay_image_flatten               (ClayImage      *image,
                                                ClayContext    *context,
                                                ClayProgress   *progress,
                                                GError        **error);

ClayPath    * clay_image_merge_visible_paths   (ClayImage      *image,
                                                GError        **error);
