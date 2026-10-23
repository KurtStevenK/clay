/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererbrush.h
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

#pragma once

#include "clayviewrenderer.h"


#define CLAY_TYPE_VIEW_RENDERER_BRUSH            (clay_view_renderer_brush_get_type ())
#define CLAY_VIEW_RENDERER_BRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW_RENDERER_BRUSH, ClayViewRendererBrush))
#define CLAY_VIEW_RENDERER_BRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW_RENDERER_BRUSH, ClayViewRendererBrushClass))
#define CLAY_IS_VIEW_RENDERER_BRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW_RENDERER_BRUSH))
#define CLAY_IS_VIEW_RENDERER_BRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW_RENDERER_BRUSH))
#define CLAY_VIEW_RENDERER_BRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW_RENDERER_BRUSH, ClayViewRendererBrushClass))


typedef struct _ClayViewRendererBrushClass  ClayViewRendererBrushClass;

struct _ClayViewRendererBrush
{
  ClayViewRenderer parent_instance;

  guint            pipe_timeout_id;
  gint             pipe_animation_index;

  /* for the pipe render animation timeout */
  GtkWidget       *widget;
};

struct _ClayViewRendererBrushClass
{
  ClayViewRendererClass  parent_class;
};


GType   clay_view_renderer_brush_get_type (void);
