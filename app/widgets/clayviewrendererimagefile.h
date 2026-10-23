/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererimagefile.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "clayviewrenderer.h"


#define CLAY_TYPE_VIEW_RENDERER_IMAGEFILE (clay_view_renderer_imagefile_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayViewRendererImagefile,
                          clay_view_renderer_imagefile,
                          CLAY, VIEW_RENDERER_IMAGEFILE,
                          ClayViewRenderer)


struct _ClayViewRendererImagefileClass
{
  ClayViewRendererClass parent_class;
};
