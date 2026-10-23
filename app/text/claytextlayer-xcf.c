/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayText
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <cairo.h>

#include "libclaybase/claybase.h"
#include "text-types.h"

#include "core/clay.h"
#include "core/claydrawable-private.h" /* eek */
#include "core/clayimage.h"
#include "core/claylayer-xcf.h"
#include "core/clayparasitelist.h"
#include "core/clayrasterizable.h"

#include "claytext.h"
#include "claytext-parasite.h"
#include "claytextlayer.h"
#include "claytextlayer-xcf.h"

#include "clay-intl.h"


enum
{
  TEXT_LAYER_XCF_NONE              = 0,
  TEXT_LAYER_XCF_DONT_AUTO_RENAME  = 1 << 0,
  TEXT_LAYER_XCF_MODIFIED          = 1 << 1
};


gboolean
clay_text_layer_xcf_load_hack (ClayLayer **layer)
{
  const gchar        *name;
  ClayText           *text = NULL;
  const ClayParasite *parasite;
  gboolean            before_xcf_v19 = FALSE;

  g_return_val_if_fail (layer != NULL, FALSE);
  g_return_val_if_fail (CLAY_IS_LAYER (*layer), FALSE);

  name = clay_text_parasite_name ();
  parasite = clay_item_parasite_find (CLAY_ITEM (*layer), name);

  if (parasite)
    {
      GError *error = NULL;

      text = clay_text_from_parasite (parasite,
                                      clay_item_get_image (CLAY_ITEM (*layer))->clay,
                                      &before_xcf_v19,
                                      &error);

      if (error)
        {
          clay_message (clay_item_get_image (CLAY_ITEM (*layer))->clay, NULL,
                        CLAY_MESSAGE_ERROR,
                        _("Problems parsing the text parasite for layer '%s':\n"
                          "%s\n\n"
                          "Some text properties may be wrong. "
                          "Unless you want to edit the text layer, "
                          "you don't need to worry about this."),
                        clay_object_get_name (*layer),
                        error->message);
          g_clear_error (&error);
        }
    }
  else
    {
      name = clay_text_gdyntext_parasite_name ();

      parasite = clay_item_parasite_find (CLAY_ITEM (*layer), name);

      if (parasite)
        {
          text = clay_text_from_gdyntext_parasite (clay_item_get_image (CLAY_ITEM (*layer))->clay,
                                                   parasite);
          before_xcf_v19 = TRUE;
        }
    }

  if (text)
    {
      clay_layer_from_layer (layer, CLAY_TYPE_TEXT_LAYER,
                             "image", clay_item_get_image (CLAY_ITEM (*layer)),
                             NULL);
      clay_text_layer_set_text (CLAY_TEXT_LAYER (*layer), text);
      g_object_unref (text);

      /*  let the text layer knows what parasite was used to create it  */
      CLAY_TEXT_LAYER (*layer)->text_parasite        = name;
      CLAY_TEXT_LAYER (*layer)->text_parasite_is_old = before_xcf_v19;
    }

  return (text != NULL);
}

void
clay_text_layer_xcf_save_prepare (ClayTextLayer *layer)
{
  ClayText *text;

  g_return_if_fail (CLAY_IS_TEXT_LAYER (layer));

  /*  If the layer has a text parasite already, it wasn't changed and we
   *  can simply save the original parasite back which is still attached.
   */
  if (layer->text_parasite)
    return;

  text = clay_text_layer_get_text (layer);
  if (text)
    {
      ClayParasite *parasite = clay_text_to_parasite (text);

      /*  Don't push an undo because the parasite only exists temporarily
       *  while the text layer is saved to XCF.
       */
      clay_item_parasite_attach (CLAY_ITEM (layer), parasite, FALSE);

      clay_parasite_free (parasite);
    }
}

guint32
clay_text_layer_get_xcf_flags (ClayTextLayer *text_layer)
{
  guint flags = 0;

  g_return_val_if_fail (CLAY_IS_TEXT_LAYER (text_layer), 0);

  if (! clay_rasterizable_get_auto_rename (CLAY_RASTERIZABLE (text_layer)))
    flags |= TEXT_LAYER_XCF_DONT_AUTO_RENAME;

  if (clay_rasterizable_is_rasterized (CLAY_RASTERIZABLE (text_layer)))
    flags |= TEXT_LAYER_XCF_MODIFIED;

  return flags;
}

void
clay_text_layer_set_xcf_flags (ClayTextLayer *text_layer,
                               guint32        flags)
{
  g_return_if_fail (CLAY_IS_TEXT_LAYER (text_layer));

  clay_rasterizable_set_auto_rename (CLAY_RASTERIZABLE (text_layer),
                                     (flags & TEXT_LAYER_XCF_DONT_AUTO_RENAME) == 0);

  if ((flags & TEXT_LAYER_XCF_MODIFIED) != 0)
    clay_rasterizable_rasterize (CLAY_RASTERIZABLE (text_layer), FALSE);
}
