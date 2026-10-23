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

#include "clayitemundo.h"


#define CLAY_TYPE_CHANNEL_UNDO            (clay_channel_undo_get_type ())
#define CLAY_CHANNEL_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_CHANNEL_UNDO, ClayChannelUndo))
#define CLAY_CHANNEL_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_CHANNEL_UNDO, ClayChannelUndoClass))
#define CLAY_IS_CHANNEL_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_CHANNEL_UNDO))
#define CLAY_IS_CHANNEL_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_CHANNEL_UNDO))
#define CLAY_CHANNEL_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_CHANNEL_UNDO, ClayChannelUndoClass))


typedef struct _ClayChannelUndo      ClayChannelUndo;
typedef struct _ClayChannelUndoClass ClayChannelUndoClass;

struct _ClayChannelUndo
{
  ClayItemUndo  parent_instance;

  ClayChannel  *prev_parent;
  gint          prev_position;   /*  former position in list     */
  GList        *prev_channels;   /*  previous selected channels  */
};

struct _ClayChannelUndoClass
{
  ClayItemUndoClass  parent_class;
};


GType   clay_channel_undo_get_type (void);
