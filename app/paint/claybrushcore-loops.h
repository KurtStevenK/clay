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


const ClayTempBuf * clay_brush_core_subsample_mask  (ClayBrushCore     *core,
                                                     const ClayTempBuf *mask,
                                                     gdouble            x,
                                                     gdouble            y);
const ClayTempBuf * clay_brush_core_pressurize_mask (ClayBrushCore     *core,
                                                     const ClayTempBuf *brush_mask,
                                                     gdouble            x,
                                                     gdouble            y,
                                                     gdouble            pressure);
const ClayTempBuf * clay_brush_core_solidify_mask   (ClayBrushCore     *core,
                                                     const ClayTempBuf *brush_mask,
                                                     gdouble            x,
                                                     gdouble            y);
