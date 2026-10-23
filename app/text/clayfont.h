/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayfont.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
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

#include "core/claydata.h"


#define CLAY_TYPE_FONT            (clay_font_get_type ())
#define CLAY_FONT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FONT, ClayFont))
#define CLAY_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FONT, ClayFontClass))
#define CLAY_IS_FONT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FONT))
#define CLAY_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FONT))
#define CLAY_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FONT, ClayFontClass))


typedef struct _ClayFontClass ClayFontClass;

GType         clay_font_get_type                 (void);

ClayData    * clay_font_get_standard             (void);
const gchar * clay_font_get_lookup_name          (ClayFont        *font);
void          clay_font_set_lookup_name          (ClayFont        *font,
                                                  gchar           *name);
gboolean      clay_font_match_by_lookup_name     (ClayFont        *font,
                                                  const gchar     *name);
gboolean      clay_font_match_by_description     (ClayFont        *font,
                                                  const gchar     *desc);
gboolean      clay_font_match_by_postscript_name (ClayFont        *font,
                                                  const gchar     *psname);
void          clay_font_set_font_info            (ClayFont        *font,
                                                  gpointer         font_info[]);
void          clay_font_class_set_font_factory   (ClayFontFactory *factory);

enum
{
  /* properties for serialization*/
  PROP_HASH,
  PROP_FULLNAME,
  PROP_FAMILY,
  PROP_STYLE,
  PROP_PSNAME,
  PROP_WEIGHT,
  PROP_WIDTH,
  PROP_INDEX,
  PROP_SLANT,
  PROP_FONTVERSION,

  /*for backward compatibility*/
  PROP_DESC,
  PROP_FILE,

  PROPERTIES_COUNT
};
