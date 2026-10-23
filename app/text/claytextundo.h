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

#include "core/clayitemundo.h"


#define CLAY_TYPE_TEXT_UNDO            (clay_text_undo_get_type ())
#define CLAY_TEXT_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_UNDO, ClayTextUndo))
#define CLAY_TEXT_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_UNDO, ClayTextUndoClass))
#define CLAY_IS_TEXT_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_UNDO))
#define CLAY_IS_TEXT_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_UNDO))
#define CLAY_TEXT_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEXT_UNDO, ClayTextUndoClass))


typedef struct _ClayTextUndoClass ClayTextUndoClass;

struct _ClayTextUndo
{
  ClayItemUndo      parent_instance;

  ClayText         *text;
  const GParamSpec *pspec;
  GValue           *value;
  const Babl       *format;
};

struct _ClayTextUndoClass
{
  ClayItemClass  parent_class;
};


GType      clay_text_undo_get_type (void);
