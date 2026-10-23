/* Clay - The GNU Image Manipulation Program
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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "clay-memsize.h"
#include "clayitem.h"
#include "clayitemtree.h"
#include "clayitempropundo.h"
#include "clayparasitelist.h"


enum
{
  PROP_0,
  PROP_PARASITE_NAME
};


static void     clay_item_prop_undo_constructed  (GObject             *object);
static void     clay_item_prop_undo_set_property (GObject             *object,
                                                  guint                property_id,
                                                  const GValue        *value,
                                                  GParamSpec          *pspec);
static void     clay_item_prop_undo_get_property (GObject             *object,
                                                  guint                property_id,
                                                  GValue              *value,
                                                  GParamSpec          *pspec);

static gint64   clay_item_prop_undo_get_memsize  (ClayObject          *object,
                                                  gint64              *gui_size);

static void     clay_item_prop_undo_pop          (ClayUndo            *undo,
                                                  ClayUndoMode         undo_mode,
                                                  ClayUndoAccumulator *accum);
static void     clay_item_prop_undo_free         (ClayUndo            *undo,
                                                  ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayItemPropUndo, clay_item_prop_undo, CLAY_TYPE_ITEM_UNDO)

#define parent_class clay_item_prop_undo_parent_class


static void
clay_item_prop_undo_class_init (ClayItemPropUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed      = clay_item_prop_undo_constructed;
  object_class->set_property     = clay_item_prop_undo_set_property;
  object_class->get_property     = clay_item_prop_undo_get_property;

  clay_object_class->get_memsize = clay_item_prop_undo_get_memsize;

  undo_class->pop                = clay_item_prop_undo_pop;
  undo_class->free               = clay_item_prop_undo_free;

  g_object_class_install_property (object_class, PROP_PARASITE_NAME,
                                   g_param_spec_string ("parasite-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_item_prop_undo_init (ClayItemPropUndo *undo)
{
}

static void
clay_item_prop_undo_constructed (GObject *object)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (object);
  ClayItem         *item;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  item = CLAY_ITEM_UNDO (object)->item;

  switch (CLAY_UNDO (object)->undo_type)
    {
    case CLAY_UNDO_ITEM_REORDER:
      item_prop_undo->parent   = clay_item_get_parent (item);
      item_prop_undo->position = clay_item_get_index (item);
      break;

    case CLAY_UNDO_ITEM_RENAME:
      item_prop_undo->name = g_strdup (clay_object_get_name (item));
      break;

    case CLAY_UNDO_ITEM_DISPLACE:
      clay_item_get_offset (item,
                            &item_prop_undo->offset_x,
                            &item_prop_undo->offset_y);
      break;

    case CLAY_UNDO_ITEM_VISIBILITY:
      item_prop_undo->visible = clay_item_get_visible (item);
      break;

    case CLAY_UNDO_ITEM_COLOR_TAG:
      item_prop_undo->color_tag = clay_item_get_color_tag (item);
      break;

    case CLAY_UNDO_ITEM_LOCK_CONTENT:
      item_prop_undo->lock_content = clay_item_get_lock_content (item);
      break;

    case CLAY_UNDO_ITEM_LOCK_POSITION:
      item_prop_undo->lock_position = clay_item_get_lock_position (item);
      break;

    case CLAY_UNDO_ITEM_LOCK_VISIBILITY:
      item_prop_undo->lock_visibility = clay_item_get_lock_visibility (item);
      break;

    case CLAY_UNDO_PARASITE_ATTACH:
    case CLAY_UNDO_PARASITE_REMOVE:
      clay_assert (item_prop_undo->parasite_name != NULL);

      item_prop_undo->parasite = clay_parasite_copy
        (clay_item_parasite_find (item, item_prop_undo->parasite_name));
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_item_prop_undo_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (object);

  switch (property_id)
    {
    case PROP_PARASITE_NAME:
      item_prop_undo->parasite_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_item_prop_undo_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (object);

  switch (property_id)
    {
    case PROP_PARASITE_NAME:
      g_value_set_string (value, item_prop_undo->parasite_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_item_prop_undo_get_memsize (ClayObject *object,
                                 gint64     *gui_size)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (object);
  gint64            memsize        = 0;

  memsize += clay_string_get_memsize (item_prop_undo->name);
  memsize += clay_string_get_memsize (item_prop_undo->parasite_name);
  memsize += clay_parasite_get_memsize (item_prop_undo->parasite, NULL);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_item_prop_undo_pop (ClayUndo            *undo,
                         ClayUndoMode         undo_mode,
                         ClayUndoAccumulator *accum)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (undo);
  ClayItem         *item           = CLAY_ITEM_UNDO (undo)->item;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  switch (undo->undo_type)
    {
    case CLAY_UNDO_ITEM_REORDER:
      {
        ClayItem *parent;
        gint      position;

        parent   = clay_item_get_parent (item);
        position = clay_item_get_index (item);

        clay_item_tree_reorder_item (clay_item_get_tree (item), item,
                                     item_prop_undo->parent,
                                     item_prop_undo->position,
                                     FALSE, NULL);

        item_prop_undo->parent   = parent;
        item_prop_undo->position = position;
      }
      break;

    case CLAY_UNDO_ITEM_RENAME:
      {
        gchar *name;

        name = g_strdup (clay_object_get_name (item));

        clay_item_tree_rename_item (clay_item_get_tree (item), item,
                                    item_prop_undo->name,
                                    FALSE, NULL);

        g_free (item_prop_undo->name);
        item_prop_undo->name = name;
      }
      break;

    case CLAY_UNDO_ITEM_DISPLACE:
      {
        gint offset_x;
        gint offset_y;

        clay_item_get_offset (item, &offset_x, &offset_y);

        clay_item_translate (item,
                             item_prop_undo->offset_x - offset_x,
                             item_prop_undo->offset_y - offset_y,
                             FALSE);

        item_prop_undo->offset_x = offset_x;
        item_prop_undo->offset_y = offset_y;
      }
      break;

    case CLAY_UNDO_ITEM_VISIBILITY:
      {
        gboolean visible;

        visible = clay_item_get_visible (item);
        clay_item_set_visible (item, item_prop_undo->visible, FALSE);
        item_prop_undo->visible = visible;
      }
      break;

    case CLAY_UNDO_ITEM_COLOR_TAG:
      {
        ClayColorTag color_tag;

        color_tag = clay_item_get_color_tag (item);
        clay_item_set_color_tag (item, item_prop_undo->color_tag, FALSE);
        item_prop_undo->color_tag = color_tag;
      }
      break;

    case CLAY_UNDO_ITEM_LOCK_CONTENT:
      {
        gboolean lock_content;

        lock_content = clay_item_get_lock_content (item);
        clay_item_set_lock_content (item, item_prop_undo->lock_content, FALSE);
        item_prop_undo->lock_content = lock_content;
      }
      break;

    case CLAY_UNDO_ITEM_LOCK_POSITION:
      {
        gboolean lock_position;

        lock_position = clay_item_get_lock_position (item);
        clay_item_set_lock_position (item, item_prop_undo->lock_position, FALSE);
        item_prop_undo->lock_position = lock_position;
      }
      break;

    case CLAY_UNDO_ITEM_LOCK_VISIBILITY:
      {
        gboolean lock_visibility;

        lock_visibility = clay_item_get_lock_visibility (item);
        clay_item_set_lock_visibility (item, item_prop_undo->lock_visibility, FALSE);
        item_prop_undo->lock_visibility = lock_visibility;
      }
      break;

    case CLAY_UNDO_PARASITE_ATTACH:
    case CLAY_UNDO_PARASITE_REMOVE:
      {
        ClayParasite *parasite;

        parasite = item_prop_undo->parasite;

        item_prop_undo->parasite = clay_parasite_copy
          (clay_item_parasite_find (item, item_prop_undo->parasite_name));

        if (parasite)
          clay_item_parasite_attach (item, parasite, FALSE);
        else
          clay_item_parasite_detach (item, item_prop_undo->parasite_name, FALSE);

        if (parasite)
          clay_parasite_free (parasite);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

static void
clay_item_prop_undo_free (ClayUndo     *undo,
                          ClayUndoMode  undo_mode)
{
  ClayItemPropUndo *item_prop_undo = CLAY_ITEM_PROP_UNDO (undo);

  g_clear_pointer (&item_prop_undo->name,          g_free);
  g_clear_pointer (&item_prop_undo->parasite_name, g_free);
  g_clear_pointer (&item_prop_undo->parasite,      clay_parasite_free);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
