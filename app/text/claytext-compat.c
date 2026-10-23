/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <fontconfig/fontconfig.h>

#include "libclaycolor/claycolor.h"

#include "text-types.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claycontext.h"
#include "core/claydatafactory.h"
#include "core/clayimage.h"
#include "core/claydrawable.h"
#include "core/clayimage.h"
#include "core/clayimage-undo.h"
#include "core/claylayer-floating-selection.h"

#include "clayfont.h"
#include "claytext.h"
#include "claytext-compat.h"
#include "claytextlayer.h"

#include "clay-intl.h"


ClayLayer *
text_render (ClayImage    *image,
             ClayDrawable *drawable,
             ClayContext  *context,
             gint          text_x,
             gint          text_y,
             ClayFont     *font,
             gdouble       font_size,
             const gchar  *text,
             gint          border,
             gboolean      antialias)
{
  ClayText  *gtext;
  ClayLayer *layer;
  GeglColor *color;

  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (drawable == NULL || CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (drawable == NULL ||
                        clay_item_is_attached (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (CLAY_IS_FONT (font), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  if (! clay_data_factory_data_wait (image->clay->font_factory))
    return NULL;

  if (border < 0)
    border = 0;

  color = clay_context_get_foreground (context);

  gtext = g_object_new (CLAY_TYPE_TEXT,
                        "text",      text,
                        "font",      font,
                        "font-size", font_size,
                        "antialias", antialias,
                        "border",    border,
                        "color",     color,
                        NULL);

  layer = clay_text_layer_new (image, gtext);

  g_object_unref (gtext);

  if (!layer)
    return NULL;

  /*  Start a group undo  */
  clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TEXT,
                               _("Add Text Layer"));

  /*  Set the layer offsets  */
  clay_item_set_offset (CLAY_ITEM (layer), text_x, text_y);

  /*  If there is a selection mask clear it--
   *  this might not always be desired, but in general,
   *  it seems like the correct behavior.
   */
  if (! clay_channel_is_empty (clay_image_get_mask (image)))
    clay_channel_clear (clay_image_get_mask (image), NULL, TRUE);

  if (drawable == NULL)
    {
      /*  If the drawable is NULL, create a new layer  */
      clay_image_add_layer (image, layer, NULL, -1, TRUE);
    }
  else
    {
      /*  Otherwise, instantiate the text as the new floating selection */
      floating_sel_attach (layer, drawable);
    }

  /*  end the group undo  */
  clay_image_undo_group_end (image);

  return layer;
}

gboolean
text_get_extents (Clay        *clay,
                  ClayFont    *font,
                  gdouble      font_size,
                  const gchar *text,
                  gint        *width,
                  gint        *height,
                  gint        *ascent,
                  gint        *descent)
{
  PangoFontDescription *font_desc;
  PangoContext         *context;
  PangoLayout          *layout;
  PangoFontMap         *fontmap;
  PangoRectangle        rect;
  gchar                *real_fontname;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (CLAY_IS_FONT (font), FALSE);
  g_return_val_if_fail (text != NULL, FALSE);

  if (! clay_data_factory_data_wait (clay->font_factory))
    return FALSE;

  fontmap = pango_cairo_font_map_new_for_font_type (CAIRO_FONT_TYPE_FT);
  if (! fontmap)
    g_error ("You are using a Pango that has been built against a cairo "
             "that lacks the Freetype font backend");

  /* In case a font becomes missing mid-session and is chosen (or is already in use)
   * pango substitutes EVERY font for the default font, to avoid this
   * the FcConfig has to be set everytime a pango fontmap is created
   */
  pango_fc_font_map_set_config (PANGO_FC_FONT_MAP (fontmap), FcConfigGetCurrent ());
  pango_cairo_font_map_set_resolution (PANGO_CAIRO_FONT_MAP (fontmap),
                                       72.0); /* FIXME: resolution */
  context = pango_font_map_create_context (fontmap);
  g_object_unref (fontmap);

  layout = pango_layout_new (context);
  g_object_unref (context);

  real_fontname = g_strdup_printf ("%s %d", clay_font_get_lookup_name (font), (gint) font_size);
  font_desc = pango_font_description_from_string (real_fontname);
  pango_layout_set_font_description (layout, font_desc);
  pango_font_description_free (font_desc);
  g_free (real_fontname);

  pango_layout_set_text (layout, text, -1);

  pango_layout_get_pixel_extents (layout, NULL, &rect);

  if (width)
    *width = rect.width;
  if (height)
    *height = rect.height;

  if (ascent || descent)
    {
      PangoLayoutIter *iter;
      PangoLayoutLine *line;

      iter = pango_layout_get_iter (layout);
      line = pango_layout_iter_get_line_readonly (iter);
      pango_layout_iter_free (iter);

      pango_layout_line_get_pixel_extents (line, NULL, &rect);

      if (ascent)
        *ascent = PANGO_ASCENT (rect);
      if (descent)
        *descent = PANGO_DESCENT (rect);
    }

  g_object_unref (layout);

  return TRUE;
}
