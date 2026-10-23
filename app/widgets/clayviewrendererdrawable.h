/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererdrawable.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_VIEW_RENDERER_DRAWABLE            (clay_view_renderer_drawable_get_type ())
#define CLAY_VIEW_RENDERER_DRAWABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEW_RENDERER_DRAWABLE, ClayViewRendererDrawable))
#define CLAY_VIEW_RENDERER_DRAWABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEW_RENDERER_DRAWABLE, ClayViewRendererDrawableClass))
#define CLAY_IS_VIEW_RENDERER_DRAWABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, CLAY_TYPE_VIEW_RENDERER_DRAWABLE))
#define CLAY_IS_VIEW_RENDERER_DRAWABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEW_RENDERER_DRAWABLE))
#define CLAY_VIEW_RENDERER_DRAWABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEW_RENDERER_DRAWABLE, ClayViewRendererDrawableClass))


typedef struct _ClayViewRendererDrawablePrivate ClayViewRendererDrawablePrivate;
typedef struct _ClayViewRendererDrawableClass   ClayViewRendererDrawableClass;

struct _ClayViewRendererDrawable
{
  ClayViewRenderer                 parent_instance;

  ClayViewRendererDrawablePrivate *priv;
};

struct _ClayViewRendererDrawableClass
{
  ClayViewRendererClass  parent_class;
};


GType   clay_view_renderer_drawable_get_type (void);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayViewRendererDrawable, g_object_unref);
