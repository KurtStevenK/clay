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

#include "claybrushcore.h"


#define CLAY_TYPE_PAINTBRUSH            (clay_paintbrush_get_type ())
#define CLAY_PAINTBRUSH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAINTBRUSH, ClayPaintbrush))
#define CLAY_PAINTBRUSH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAINTBRUSH, ClayPaintbrushClass))
#define CLAY_IS_PAINTBRUSH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAINTBRUSH))
#define CLAY_IS_PAINTBRUSH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAINTBRUSH))
#define CLAY_PAINTBRUSH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAINTBRUSH, ClayPaintbrushClass))


typedef struct _ClayPaintbrushClass ClayPaintbrushClass;

struct _ClayPaintbrush
{
  ClayBrushCore      parent_instance;

  GeglBuffer        *paint_buffer;
  const ClayTempBuf *paint_pixmap;
  GeglColor         *paint_color;
};

struct _ClayPaintbrushClass
{
  ClayBrushCoreClass  parent_class;

  /*  virtual functions  */
  gboolean   (* get_color_history_color) (ClayPaintbrush            *paintbrush,
                                          ClayDrawable              *drawable,
                                          ClayPaintOptions          *paint_options,
                                          GeglColor                **color);
  void       (* get_paint_params)        (ClayPaintbrush            *paintbrush,
                                          ClayDrawable              *drawable,
                                          ClayPaintOptions          *paint_options,
                                          ClaySymmetry              *sym,
                                          gdouble                    grad_point,
                                          ClayLayerMode             *paint_mode,
                                          ClayPaintApplicationMode  *paint_appl_mode,
                                          const ClayTempBuf        **paint_pixmap,
                                          GeglColor                **paint_color);
};


void    clay_paintbrush_register (Clay                      *clay,
                                  ClayPaintRegisterCallback  callback);

GType   clay_paintbrush_get_type (void);


/*  protected  */

void    _clay_paintbrush_motion  (ClayPaintCore             *paint_core,
                                  ClayDrawable              *drawable,
                                  ClayPaintOptions          *paint_options,
                                  ClaySymmetry              *sym,
                                  gdouble                    opacity);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayPaintbrush, g_object_unref);
