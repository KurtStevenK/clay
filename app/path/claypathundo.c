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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "path-types.h"

#include "libclaybase/claybase.h"

#include "core/clayimage.h"

#include "claypath.h"
#include "claypathundo.h"


enum
{
  PROP_0,
  PROP_PREV_PARENT,
  PROP_PREV_POSITION,
  PROP_PREV_PATHS
};


static void     clay_path_undo_constructed  (GObject             *object);
static void     clay_path_undo_finalize     (GObject             *object);
static void     clay_path_undo_set_property (GObject             *object,
                                             guint                property_id,
                                             const GValue        *value,
                                             GParamSpec          *pspec);
static void     clay_path_undo_get_property (GObject             *object,
                                             guint                property_id,
                                             GValue              *value,
                                             GParamSpec          *pspec);

static gint64   clay_path_undo_get_memsize  (ClayObject          *object,
                                             gint64              *gui_size);

static void     clay_path_undo_pop          (ClayUndo            *undo,
                                             ClayUndoMode         undo_mode,
                                             ClayUndoAccumulator *accum);


G_DEFINE_TYPE (ClayPathUndo, clay_path_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_path_undo_parent_class


static void
clay_path_undo_class_init (ClayPathUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_path_undo_constructed;
  object_class->finalize         = clay_path_undo_finalize;
  object_class->set_property     = clay_path_undo_set_property;
  object_class->get_property     = clay_path_undo_get_property;

  clay_object_class->get_memsize = clay_path_undo_get_memsize;

  undo_class->pop                = clay_path_undo_pop;

  g_object_class_install_property (object_class, PROP_PREV_PARENT,
                                   g_param_spec_object ("prev-parent",
                                                        NULL, NULL,
                                                        CLAY_TYPE_PATH,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_POSITION,
                                   g_param_spec_int ("prev-position", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     CLAY_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PREV_PATHS,
                                   g_param_spec_pointer ("prev-paths", NULL, NULL,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_path_undo_init (ClayPathUndo *undo)
{
  undo->prev_paths = NULL;
}

static void
clay_path_undo_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_PATH (CLAY_ITEM_UNDO (object)->item));
}

static void
clay_path_undo_finalize (GObject *object)
{
  ClayPathUndo *undo = CLAY_PATH_UNDO (object);

  g_clear_pointer (&undo->prev_paths, g_list_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_path_undo_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  ClayPathUndo *path_undo = CLAY_PATH_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      path_undo->prev_parent = g_value_get_object (value);
      break;
    case PROP_PREV_POSITION:
      path_undo->prev_position = g_value_get_int (value);
      break;
    case PROP_PREV_PATHS:
      path_undo->prev_paths = g_list_copy (g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_path_undo_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  ClayPathUndo *path_undo = CLAY_PATH_UNDO (object);

  switch (property_id)
    {
    case PROP_PREV_PARENT:
      g_value_set_object (value, path_undo->prev_parent);
      break;
    case PROP_PREV_POSITION:
      g_value_set_int (value, path_undo->prev_position);
      break;
    case PROP_PREV_PATHS:
      g_value_set_pointer (value, path_undo->prev_paths);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_path_undo_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayItemUndo *item_undo = CLAY_ITEM_UNDO (object);
  gint64        memsize   = 0;

  if (! clay_item_is_attached (item_undo->item))
    memsize += clay_object_get_memsize (CLAY_OBJECT (item_undo->item),
                                        gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_path_undo_pop (ClayUndo            *undo,
                    ClayUndoMode         undo_mode,
                    ClayUndoAccumulator *accum)
{
  ClayPathUndo *path_undo = CLAY_PATH_UNDO (undo);
  ClayPath     *path      = CLAY_PATH (CLAY_ITEM_UNDO (undo)->item);

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
       undo->undo_type == CLAY_UNDO_PATH_ADD) ||
      (undo_mode       == CLAY_UNDO_MODE_REDO &&
       undo->undo_type == CLAY_UNDO_PATH_REMOVE))
    {
      /*  remove path  */

      /*  record the current parent and position  */
      path_undo->prev_parent   = clay_path_get_parent (path);
      path_undo->prev_position = clay_item_get_index (CLAY_ITEM (path));

      clay_image_remove_path (undo->image, path, FALSE,
                              path_undo->prev_paths);
    }
  else
    {
      /*  restore path  */

      /*  record the active path  */
      g_clear_pointer (&path_undo->prev_paths, g_list_free);
      path_undo->prev_paths = g_list_copy (clay_image_get_selected_paths (undo->image));

      clay_image_add_path (undo->image, path,
                           path_undo->prev_parent,
                           path_undo->prev_position, FALSE);
    }
}
