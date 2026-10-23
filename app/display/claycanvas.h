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

#include "widgets/clayoverlaybox.h"


#define CLAY_CANVAS_EVENT_MASK (GDK_EXPOSURE_MASK            | \
                                GDK_POINTER_MOTION_MASK      | \
                                GDK_BUTTON_PRESS_MASK        | \
                                GDK_BUTTON_RELEASE_MASK      | \
                                GDK_SCROLL_MASK              | \
                                GDK_SMOOTH_SCROLL_MASK       | \
                                GDK_TOUCHPAD_GESTURE_MASK    | \
                                GDK_STRUCTURE_MASK           | \
                                GDK_ENTER_NOTIFY_MASK        | \
                                GDK_LEAVE_NOTIFY_MASK        | \
                                GDK_FOCUS_CHANGE_MASK        | \
                                GDK_KEY_PRESS_MASK           | \
                                GDK_KEY_RELEASE_MASK         | \
                                GDK_PROXIMITY_OUT_MASK)


#define CLAY_TYPE_CANVAS            (clay_canvas_get_type ())
#define CLAY_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CANVAS, ClayCanvas))
#define CLAY_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CANVAS, ClayCanvasClass))
#define CLAY_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CANVAS))
#define CLAY_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CANVAS))
#define CLAY_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CANVAS, ClayCanvasClass))


typedef struct _ClayCanvasClass ClayCanvasClass;

struct _ClayCanvas
{
  ClayOverlayBox         parent_instance;

  ClayDisplayConfig     *config;
  PangoLayout           *layout;

  ClayCanvasPaddingMode  padding_mode;
  GeglColor             *padding_color;
};

struct _ClayCanvasClass
{
  ClayOverlayBoxClass  parent_class;
};


GType         clay_canvas_get_type     (void);

GtkWidget   * clay_canvas_new          (ClayDisplayConfig     *config);

PangoLayout * clay_canvas_get_layout   (ClayCanvas            *canvas,
                                        const gchar           *format,
                                        ...) G_GNUC_PRINTF (2, 3);

void          clay_canvas_set_padding  (ClayCanvas            *canvas,
                                        ClayCanvasPaddingMode  padding_mode,
                                        GeglColor             *padding_color);
