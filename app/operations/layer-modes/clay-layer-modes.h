/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clay-layer-modes.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *                    Øyvind Kolås <pippin@gimp.org>
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


void                       clay_layer_modes_init                      (void);
void                       clay_layer_modes_exit                      (void);

gboolean                   clay_layer_mode_is_legacy                  (ClayLayerMode           mode);

gboolean                   clay_layer_mode_is_blend_space_mutable     (ClayLayerMode           mode);
gboolean                   clay_layer_mode_is_composite_space_mutable (ClayLayerMode           mode);
gboolean                   clay_layer_mode_is_composite_mode_mutable  (ClayLayerMode           mode);

gboolean                   clay_layer_mode_is_subtractive             (ClayLayerMode           mode);
gboolean                   clay_layer_mode_is_alpha_only              (ClayLayerMode           mode);
gboolean                   clay_layer_mode_is_trivial                 (ClayLayerMode           mode);

ClayLayerColorSpace        clay_layer_mode_get_blend_space            (ClayLayerMode           mode);
ClayLayerColorSpace        clay_layer_mode_get_composite_space        (ClayLayerMode           mode);
ClayLayerCompositeMode     clay_layer_mode_get_composite_mode         (ClayLayerMode           mode);
ClayLayerCompositeMode     clay_layer_mode_get_paint_composite_mode   (ClayLayerMode           mode);

const gchar              * clay_layer_mode_get_operation_name         (ClayLayerMode           mode);
GeglOperation            * clay_layer_mode_get_operation              (ClayLayerMode           mode);

ClayLayerModeFunc          clay_layer_mode_get_function               (ClayLayerMode           mode);
ClayLayerModeBlendFunc     clay_layer_mode_get_blend_function         (ClayLayerMode           mode);

ClayLayerModeContext       clay_layer_mode_get_context                (ClayLayerMode           mode);

ClayLayerMode            * clay_layer_mode_get_context_array          (ClayLayerMode           mode,
                                                                       ClayLayerModeContext    context,
                                                                       gint                   *n_modes);

ClayLayerModeGroup         clay_layer_mode_get_group                  (ClayLayerMode           mode);

const ClayLayerMode      * clay_layer_mode_get_group_array            (ClayLayerModeGroup      group,
                                                                       gint                   *n_modes);

gboolean                   clay_layer_mode_get_for_group              (ClayLayerMode           old_mode,
                                                                       ClayLayerModeGroup      new_group,
                                                                       ClayLayerMode          *new_mode);

const Babl               * clay_layer_mode_get_format                 (ClayLayerMode           mode,
                                                                       ClayLayerColorSpace     blend_space,
                                                                       ClayLayerColorSpace     composite_space,
                                                                       ClayLayerCompositeMode  composite_mode,
                                                                       const Babl             *preferred_format);

ClayLayerCompositeRegion   clay_layer_mode_get_included_region        (ClayLayerMode           mode,
                                                                       ClayLayerCompositeMode  composite_mode);

