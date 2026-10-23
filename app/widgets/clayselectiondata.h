/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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


/*  uri list  */

void            clay_selection_data_set_uri_list  (GtkSelectionData *selection,
                                                   GList            *uris);
GList         * clay_selection_data_get_uri_list  (GtkSelectionData *selection);


/*  color  */

void            clay_selection_data_set_color     (GtkSelectionData *selection,
                                                   GeglColor        *color);
GeglColor     * clay_selection_data_get_color     (GtkSelectionData *selection);


/*  image (xcf)  */

void            clay_selection_data_set_xcf       (GtkSelectionData *selection,
                                                   ClayImage        *image);
ClayImage     * clay_selection_data_get_xcf       (GtkSelectionData *selection,
                                                   Clay             *clay);


/*  stream (svg/png)  */

void            clay_selection_data_set_stream    (GtkSelectionData *selection,
                                                   const guchar     *stream,
                                                   gsize             stream_length);
const guchar  * clay_selection_data_get_stream    (GtkSelectionData *selection,
                                                   gsize            *stream_length);


/*  curve  */

void            clay_selection_data_set_curve     (GtkSelectionData *selection,
                                                   ClayCurve        *curve);
ClayCurve     * clay_selection_data_get_curve     (GtkSelectionData *selection);


/*  image  */

void            clay_selection_data_set_image     (GtkSelectionData *selection,
                                                   ClayImage        *image);
ClayImage     * clay_selection_data_get_image     (GtkSelectionData *selection,
                                                   Clay             *clay);


/*  component  */

void            clay_selection_data_set_component (GtkSelectionData *selection,
                                                   ClayImage        *image,
                                                   ClayChannelType   channel);
ClayImage     * clay_selection_data_get_component (GtkSelectionData *selection,
                                                   Clay             *clay,
                                                   ClayChannelType  *channel);


/*  item  */

void            clay_selection_data_set_item      (GtkSelectionData *selection,
                                                   ClayItem         *item);
ClayItem      * clay_selection_data_get_item      (GtkSelectionData *selection,
                                                   Clay             *clay);


/*  item list  */

void            clay_selection_data_set_item_list (GtkSelectionData *selection,
                                                   GList            *items);
GList         * clay_selection_data_get_item_list (GtkSelectionData *selection,
                                                   Clay             *clay);


/*  various data  */

void            clay_selection_data_set_object    (GtkSelectionData *selection,
                                                   ClayObject       *object);

ClayBrush     * clay_selection_data_get_brush     (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayPattern   * clay_selection_data_get_pattern   (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayGradient  * clay_selection_data_get_gradient  (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayPalette   * clay_selection_data_get_palette   (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayFont      * clay_selection_data_get_font      (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayBuffer    * clay_selection_data_get_buffer    (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayImagefile * clay_selection_data_get_imagefile (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayTemplate  * clay_selection_data_get_template  (GtkSelectionData *selection,
                                                   Clay             *clay);
ClayToolItem  * clay_selection_data_get_tool_item (GtkSelectionData *selection,
                                                   Clay             *clay);
