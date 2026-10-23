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


typedef void (* ClayConvertIndexedCallback) (GtkWidget              *dialog,
                                             ClayImage              *image,
                                             ClayConvertPaletteType  palette_type,
                                             gint                    max_colors,
                                             gboolean                remove_duplicates,
                                             ClayConvertDitherType   dither_type,
                                             gboolean                dither_alpha,
                                             gboolean                dither_text_layers,
                                             ClayPalette            *custom_palette,
                                             gpointer                user_data);


GtkWidget * convert_indexed_dialog_new (ClayImage                  *image,
                                        ClayContext                *context,
                                        GtkWidget                  *parent,
                                        ClayConvertPaletteType      palette_type,
                                        gint                        max_colors,
                                        gboolean                    remove_duplicates,
                                        ClayConvertDitherType       dither_type,
                                        gboolean                    dither_alpha,
                                        gboolean                    dither_text_layers,
                                        ClayPalette                *custom_palette,
                                        ClayConvertIndexedCallback  callback,
                                        gpointer                    user_data);
