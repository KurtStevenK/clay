/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushclipboard.h
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#include "claybrush.h"


#define CLAY_TYPE_BRUSH_CLIPBOARD            (clay_brush_clipboard_get_type ())
#define CLAY_BRUSH_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_CLIPBOARD, ClayBrushClipboard))
#define CLAY_BRUSH_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_CLIPBOARD, ClayBrushClipboardClass))
#define CLAY_IS_BRUSH_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_CLIPBOARD))
#define CLAY_IS_BRUSH_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_CLIPBOARD))
#define CLAY_BRUSH_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_CLIPBOARD, ClayBrushClipboardClass))


typedef struct _ClayBrushClipboardClass ClayBrushClipboardClass;

struct _ClayBrushClipboard
{
  ClayBrush  parent_instance;

  Clay      *clay;
  gboolean   mask_only;
};

struct _ClayBrushClipboardClass
{
  ClayBrushClass  parent_class;
};


GType      clay_brush_clipboard_get_type (void);

ClayData * clay_brush_clipboard_new      (Clay     *clay,
                                          gboolean  mask_only);
