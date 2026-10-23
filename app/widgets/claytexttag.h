/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextTag
 * Copyright (C) 2010  Michael Natterer <mitch@gimp.org>
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


/*  GtkTextTag property names  */

#define CLAY_TEXT_PROP_NAME_SIZE     "size"
#define CLAY_TEXT_PROP_NAME_BASELINE "rise"
#define CLAY_TEXT_PROP_NAME_KERNING  "rise" /* FIXME */
#define CLAY_TEXT_PROP_NAME_FONT     "font"
#define CLAY_TEXT_PROP_NAME_FG_COLOR "foreground-rgba"
#define CLAY_TEXT_PROP_NAME_BG_COLOR "background-rgba"


gint       clay_text_tag_get_size     (GtkTextTag  *tag);
gint       clay_text_tag_get_baseline (GtkTextTag  *tag);
gint       clay_text_tag_get_kerning  (GtkTextTag  *tag);
gchar    * clay_text_tag_get_font     (GtkTextTag  *tag);
gboolean   clay_text_tag_get_fg_color (GtkTextTag  *tag,
                                       GeglColor  **color);
gboolean   clay_text_tag_get_bg_color (GtkTextTag  *tag,
                                       GeglColor  **color);
