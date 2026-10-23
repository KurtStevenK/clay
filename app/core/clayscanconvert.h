/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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


ClayScanConvert *
          clay_scan_convert_new               (void);

ClayScanConvert *
          clay_scan_convert_new_from_boundary (const ClayBoundSeg *bound_segs,
                                               gint                n_bound_segs,
                                               gint                offset_x,
                                               gint                offset_y);

void      clay_scan_convert_free               (ClayScanConvert   *sc);
void      clay_scan_convert_set_pixel_ratio    (ClayScanConvert   *sc,
                                                gdouble            ratio_xy);
void      clay_scan_convert_set_clip_rectangle (ClayScanConvert   *sc,
                                                gint               x,
                                                gint               y,
                                                gint               width,
                                                gint               height);
void      clay_scan_convert_add_polyline       (ClayScanConvert   *sc,
                                                guint              n_points,
                                                const ClayVector2 *points,
                                                gboolean           closed);
void      clay_scan_convert_add_bezier         (ClayScanConvert      *sc,
                                                const ClayBezierDesc *bezier);
void      clay_scan_convert_stroke             (ClayScanConvert   *sc,
                                                gdouble            width,
                                                ClayJoinStyle      join,
                                                ClayCapStyle       cap,
                                                gdouble            miter,
                                                gdouble            dash_offset,
                                                GArray            *dash_info);
void      clay_scan_convert_render_full        (ClayScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gboolean           replace,
                                                gboolean           antialias,
                                                gdouble            value);

void      clay_scan_convert_render             (ClayScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gboolean           antialias);
void      clay_scan_convert_render_value       (ClayScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gdouble            value);
void      clay_scan_convert_compose            (ClayScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y);
void      clay_scan_convert_compose_value      (ClayScanConvert   *sc,
                                                GeglBuffer        *buffer,
                                                gint               off_x,
                                                gint               off_y,
                                                gdouble            value);
