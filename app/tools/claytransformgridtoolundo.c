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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "tools-types.h"

#include "claytoolcontrol.h"
#include "claytransformgridtool.h"
#include "claytransformgridtoolundo.h"


enum
{
  PROP_0,
  PROP_TRANSFORM_TOOL
};


static void   clay_transform_grid_tool_undo_constructed  (GObject             *object);
static void   clay_transform_grid_tool_undo_set_property (GObject             *object,
                                                          guint                property_id,
                                                          const GValue        *value,
                                                          GParamSpec          *pspec);
static void   clay_transform_grid_tool_undo_get_property (GObject             *object,
                                                          guint                property_id,
                                                          GValue              *value,
                                                          GParamSpec          *pspec);

static void   clay_transform_grid_tool_undo_pop          (ClayUndo            *undo,
                                                          ClayUndoMode         undo_mode,
                                                          ClayUndoAccumulator *accum);
static void   clay_transform_grid_tool_undo_free         (ClayUndo            *undo,
                                                          ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayTransformGridToolUndo, clay_transform_grid_tool_undo, CLAY_TYPE_UNDO)

#define parent_class clay_transform_grid_tool_undo_parent_class


static void
clay_transform_grid_tool_undo_class_init (ClayTransformGridToolUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayUndoClass *undo_class   = CLAY_UNDO_CLASS (klass);

  object_class->constructed  = clay_transform_grid_tool_undo_constructed;
  object_class->set_property = clay_transform_grid_tool_undo_set_property;
  object_class->get_property = clay_transform_grid_tool_undo_get_property;

  undo_class->pop            = clay_transform_grid_tool_undo_pop;
  undo_class->free           = clay_transform_grid_tool_undo_free;

  g_object_class_install_property (object_class, PROP_TRANSFORM_TOOL,
                                   g_param_spec_object ("transform-tool",
                                                        NULL, NULL,
                                                        CLAY_TYPE_TRANSFORM_GRID_TOOL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_transform_grid_tool_undo_init (ClayTransformGridToolUndo *undo)
{
}

static void
clay_transform_grid_tool_undo_constructed (GObject *object)
{
  ClayTransformGridToolUndo *tg_tool_undo = CLAY_TRANSFORM_GRID_TOOL_UNDO (object);
  ClayTransformGridTool     *tg_tool;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_TRANSFORM_GRID_TOOL (tg_tool_undo->tg_tool));

  tg_tool = tg_tool_undo->tg_tool;

  memcpy (tg_tool_undo->trans_infos[CLAY_TRANSFORM_FORWARD],
          tg_tool->init_trans_info, sizeof (TransInfo));
  memcpy (tg_tool_undo->trans_infos[CLAY_TRANSFORM_BACKWARD],
          tg_tool->init_trans_info, sizeof (TransInfo));

#if 0
  if (tg_tool->original)
    tg_tool_undo->original = tile_manager_ref (tg_tool->original);
#endif
}

static void
clay_transform_grid_tool_undo_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  ClayTransformGridToolUndo *tg_tool_undo = CLAY_TRANSFORM_GRID_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_TRANSFORM_TOOL:
      g_set_weak_pointer (&tg_tool_undo->tg_tool,
                          g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_transform_grid_tool_undo_get_property (GObject    *object,
                                            guint       property_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  ClayTransformGridToolUndo *tg_tool_undo = CLAY_TRANSFORM_GRID_TOOL_UNDO (object);

  switch (property_id)
    {
    case PROP_TRANSFORM_TOOL:
      g_value_set_object (value, tg_tool_undo->tg_tool);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_transform_grid_tool_undo_pop (ClayUndo            *undo,
                                   ClayUndoMode         undo_mode,
                                   ClayUndoAccumulator *accum)
{
  ClayTransformGridToolUndo *tg_tool_undo = CLAY_TRANSFORM_GRID_TOOL_UNDO (undo);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (tg_tool_undo->tg_tool)
    {
      ClayTransformGridTool *tg_tool;
#if 0
      TileManager           *temp;
#endif
      TransInfo              temp_trans_infos[2];

      tg_tool = tg_tool_undo->tg_tool;

      /*  swap the transformation information00 arrays  */
      memcpy (temp_trans_infos, tg_tool_undo->trans_infos,
              sizeof (tg_tool->trans_infos));
      memcpy (tg_tool_undo->trans_infos, tg_tool->trans_infos,
              sizeof (tg_tool->trans_infos));
      memcpy (tg_tool->trans_infos, temp_trans_infos,
              sizeof (tg_tool->trans_infos));

#if 0
      /*  swap the original buffer--the source buffer for repeated transform_grids
       */
      temp                   = tg_tool_undo->original;
      tg_tool_undo->original = tg_tool->original;
      tg_tool->original      = temp;
#endif

#if 0
      /*  If we're re-implementing the first transform_grid, reactivate tool  */
      if (undo_mode == CLAY_UNDO_MODE_REDO && tg_tool->original)
        {
          clay_tool_control_activate (CLAY_TOOL (tg_tool)->control);

          clay_draw_tool_resume (CLAY_DRAW_TOOL (tg_tool));
        }
#endif
    }
 }

static void
clay_transform_grid_tool_undo_free (ClayUndo     *undo,
                                    ClayUndoMode  undo_mode)
{
  ClayTransformGridToolUndo *tg_tool_undo = CLAY_TRANSFORM_GRID_TOOL_UNDO (undo);

  g_clear_weak_pointer (&tg_tool_undo->tg_tool);

#if 0
  if (tg_tool_undo->original)
    {
      tile_manager_unref (tg_tool_undo->original);
      tg_tool_undo->original = NULL;
    }
#endif

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
