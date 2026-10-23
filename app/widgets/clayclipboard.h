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


void         clay_clipboard_init       (Clay        *clay);
void         clay_clipboard_exit       (Clay        *clay);

gboolean     clay_clipboard_has_image  (Clay        *clay);
gboolean     clay_clipboard_has_buffer (Clay        *clay);
gboolean     clay_clipboard_has_svg    (Clay        *clay);
gboolean     clay_clipboard_has_curve  (Clay        *clay);

ClayObject * clay_clipboard_get_object (Clay        *clay);

ClayImage  * clay_clipboard_get_image  (Clay        *clay);
ClayBuffer * clay_clipboard_get_buffer (Clay        *clay);
gchar      * clay_clipboard_get_svg    (Clay        *clay,
                                        gsize       *svg_length);
ClayCurve  * clay_clipboard_get_curve  (Clay        *clay);

void         clay_clipboard_set_image  (Clay        *clay,
                                        ClayImage   *image);
void         clay_clipboard_set_buffer (Clay        *clay,
                                        ClayBuffer  *buffer);
void         clay_clipboard_set_svg    (Clay        *clay,
                                        const gchar *svg);
void         clay_clipboard_set_text   (Clay        *clay,
                                        const gchar *text);
void         clay_clipboard_set_curve  (Clay        *clay,
                                        ClayCurve   *curve);
