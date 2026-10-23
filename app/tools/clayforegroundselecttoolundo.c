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

#if 0

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "tools-types.h"

#include "clayforegroundselecttool.h"
#include "clayforegroundselecttoolundo.h"


enum
{
  PROP_0,
  PROP_FOREGROUND_SELECT_TOOL
};


static void   clay_foreground_select_tool_undo_constructed  (GObject             *object);
static void   clay_foreground_select_tool_undo_set_property (GObject             *object,
                                                             guint                property_id,
                                                             const GValue        *value,
                                                             GParamSpec          *pspec);
static void   clay_foreground_select_tool_undo_get_property (GObject             *object,
                                                             guint                property_id,
                                                             GValue              *value,
                                                             GParamSpec          *pspec);

static void   clay_foreground_select_tool_undo_pop          (ClayUndo            *undo,
                                                             ClayUndoMode         undo_mode,
                                                             ClayUndoAccumulator *accum);
static void   clay_foreground_select_tool_undo_free         (ClayUndo            *undo,
                                                             ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayForegroundSelectToolUndo, clay_foreground_select_tool_undo,
               CLAY_TYPE_UNDO)

#define parent_class clay_foreground_select_tool_undo_parent_class


static void
clay_foreground_select_tool_undo_class_init (ClayForegroundSelectToolUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed  = clay_foreground_select_tool_undo_constructed;
  object_class->set_property = clay_foreground_select_tool_undo_set_property;
  object_class->get_property = clay_foreground_select_tool_undo_get_property;

  undo_class->pop            = clay_foreground_select_tool_undo_pop;
  undo_class->free           = clay_foreground_select_tool_undo_free;

  g_object_class_install_property (object_class, PROP_FOREGROUND_SELECT_TOOL,
                                   g_param_spec_object ("foreground-select-tool",
                                                        NULL, NULL,
                                                        CLAY_TYPE_FOREGROUND_SELECT_TOOL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_foreground_select_tool_undo_init (ClayForegroundSelectToolUndo *undo)
{
}

static void
clay_foreground_select_tool_undo_constructed (GObject *object)
{
  ClayForegroundSelectToolUndo *fg_select_tool_undo;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  fg_select_tool_undo = CLAY_FOREGROUND_SELECT_TOOL_UNDO (object);

  clay_assert (CLAY_IS_FOREGROUND_SELECT_TOOL (fg_select_tool_undo->foreground_select_tool));
}

static void
clay_foreground_select_tool_undo_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  ClayForegroundSelectToolUndo *fg_select_tool_undo =
    CLAY_FOREGROUND_SELECT_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_FOREGROUND_SELECT_TOOL:
      g_set_weak_pointer (&fg_select_tool_undo->foreground_select_tool,
                          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_foreground_select_tool_undo_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  ClayForegroundSelectToolUndo *fg_select_tool_undo =
    CLAY_FOREGROUND_SELECT_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_FOREGROUND_SELECT_TOOL:
      g_value_set_object (value, fg_select_tool_undo->foreground_select_tool);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_foreground_select_tool_undo_pop (ClayUndo              *undo,
                                      ClayUndoMode           undo_mode,
                                      ClayUndoAccumulator   *accum)
{
  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);
}

static void
clay_foreground_select_tool_undo_free (ClayUndo     *undo,
                                       ClayUndoMode  undo_mode)
{
  ClayForegroundSelectToolUndo *fg_select_tool_undo = CLAY_FOREGROUND_SELECT_TOOL_UNDO (undo);

  g_clear_weak_pointer (&fg_select_tool_undo->foreground_select_tool);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}

#endif
