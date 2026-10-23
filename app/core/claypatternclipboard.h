/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypatternclipboard.h
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

#include "claypattern.h"


#define CLAY_TYPE_PATTERN_CLIPBOARD            (clay_pattern_clipboard_get_type ())
#define CLAY_PATTERN_CLIPBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PATTERN_CLIPBOARD, ClayPatternClipboard))
#define CLAY_PATTERN_CLIPBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PATTERN_CLIPBOARD, ClayPatternClipboardClass))
#define CLAY_IS_PATTERN_CLIPBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PATTERN_CLIPBOARD))
#define CLAY_IS_PATTERN_CLIPBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PATTERN_CLIPBOARD))
#define CLAY_PATTERN_CLIPBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PATTERN_CLIPBOARD, ClayPatternClipboardClass))


typedef struct _ClayPatternClipboardClass ClayPatternClipboardClass;

struct _ClayPatternClipboard
{
  ClayPattern  parent_instance;

  Clay        *clay;
};

struct _ClayPatternClipboardClass
{
  ClayPatternClass  parent_class;
};


GType      clay_pattern_clipboard_get_type (void);

ClayData * clay_pattern_clipboard_new      (Clay *clay);
