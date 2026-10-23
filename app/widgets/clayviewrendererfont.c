/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererfont.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "clayviewrendererfont.h"


G_DEFINE_TYPE (ClayViewRendererFont,
               clay_view_renderer_font,
               CLAY_TYPE_VIEW_RENDERER)

#define parent_class clay_view_renderer_font_parent_class


static void
clay_view_renderer_font_class_init (ClayViewRendererFontClass *klass)
{
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  renderer_class->default_bg      = CLAY_VIEW_BG_WHITE;
  renderer_class->follow_theme_bg = CLAY_VIEW_BG_STYLE;
}

static void
clay_view_renderer_font_init (ClayViewRendererFont *renderer)
{
}
