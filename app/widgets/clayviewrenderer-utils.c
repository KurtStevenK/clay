/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrenderer-utils.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/claybrush.h"
#include "core/claybuffer.h"
#include "core/claygradient.h"
#include "core/clayimage.h"
#include "core/clayimagefile.h"

#include "core/clayimageproxy.h"
#include "core/claylayer.h"
#include "core/claypalette.h"

#include "path/claypath.h"

#include "text/clayfont.h"

#include "clayviewrenderer-utils.h"
#include "clayviewrendererbrush.h"
#include "clayviewrendererbuffer.h"
#include "clayviewrendererfont.h"
#include "clayviewrenderergradient.h"
#include "clayviewrendererimage.h"
#include "clayviewrendererimagefile.h"
#include "clayviewrendererlayer.h"
#include "clayviewrendererpalette.h"
#include "clayviewrendererpath.h"


GType
clay_view_renderer_type_from_viewable_type (GType viewable_type)
{
  GType type = CLAY_TYPE_VIEW_RENDERER;

  g_return_val_if_fail (g_type_is_a (viewable_type, CLAY_TYPE_VIEWABLE),
                        G_TYPE_NONE);

  if (g_type_is_a (viewable_type, CLAY_TYPE_BRUSH))
    {
      type = CLAY_TYPE_VIEW_RENDERER_BRUSH;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_BUFFER))
    {
      type = CLAY_TYPE_VIEW_RENDERER_BUFFER;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_IMAGE) ||
           g_type_is_a (viewable_type, CLAY_TYPE_IMAGE_PROXY))
    {
      type = CLAY_TYPE_VIEW_RENDERER_IMAGE;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_LAYER))
    {
      type = CLAY_TYPE_VIEW_RENDERER_LAYER;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_DRAWABLE))
    {
      type = CLAY_TYPE_VIEW_RENDERER_DRAWABLE;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_GRADIENT))
    {
      type = CLAY_TYPE_VIEW_RENDERER_GRADIENT;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_PATH))
    {
      type = CLAY_TYPE_VIEW_RENDERER_PATH;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_IMAGEFILE))
    {
      type = CLAY_TYPE_VIEW_RENDERER_IMAGEFILE;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_PALETTE))
    {
      type = CLAY_TYPE_VIEW_RENDERER_PALETTE;
    }
  else if (g_type_is_a (viewable_type, CLAY_TYPE_FONT))
    {
      type = CLAY_TYPE_VIEW_RENDERER_FONT;
    }

  return type;
}
