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

#include "clayviewable.h"


struct _ClayUndoAccumulator
{
  gboolean mode_changed;
  gboolean precision_changed;

  gboolean size_changed;
  gint     previous_origin_x;
  gint     previous_origin_y;
  gint     previous_width;
  gint     previous_height;

  gboolean resolution_changed;

  gboolean unit_changed;
};


#define CLAY_TYPE_UNDO            (clay_undo_get_type ())
#define CLAY_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_UNDO, ClayUndo))
#define CLAY_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_UNDO, ClayUndoClass))
#define CLAY_IS_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_UNDO))
#define CLAY_IS_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_UNDO))
#define CLAY_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_UNDO, ClayUndoClass))


typedef struct _ClayUndoClass ClayUndoClass;

struct _ClayUndo
{
  ClayViewable      parent_instance;

  ClayImage        *image;          /* the image this undo is part of     */
  guint             time;           /* time of undo step construction     */

  ClayUndoType      undo_type;      /* undo type                          */
  ClayDirtyMask     dirty_mask;     /* affected parts of the image        */

  ClayTempBuf      *preview;
  guint             preview_idle_id;
};

struct _ClayUndoClass
{
  ClayViewableClass  parent_class;

  void (* pop)  (ClayUndo            *undo,
                 ClayUndoMode         undo_mode,
                 ClayUndoAccumulator *accum);
  void (* free) (ClayUndo            *undo,
                 ClayUndoMode         undo_mode);
};


GType         clay_undo_get_type        (void);

void          clay_undo_pop             (ClayUndo            *undo,
                                         ClayUndoMode         undo_mode,
                                         ClayUndoAccumulator *accum);
void          clay_undo_free            (ClayUndo            *undo,
                                         ClayUndoMode         undo_mode);

void          clay_undo_create_preview  (ClayUndo            *undo,
                                         ClayContext         *context,
                                         gboolean             create_now);
void          clay_undo_refresh_preview (ClayUndo            *undo,
                                         ClayContext         *context);

const gchar * clay_undo_type_to_name    (ClayUndoType         type);

gboolean      clay_undo_is_weak         (ClayUndo            *undo);
gint          clay_undo_get_age         (ClayUndo            *undo);
void          clay_undo_reset_age       (ClayUndo            *undo);
