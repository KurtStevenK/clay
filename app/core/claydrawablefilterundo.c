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

#include "libclaybase/claybase.h"

#include "core-types.h"

#include "gegl/clay-gegl-utils.h"

#include "claychannel.h"
#include "claycontainer.h"
#include "claydrawable-filters.h"
#include "claydrawablefilter.h"
#include "claydrawablefilterundo.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayitem.h"
#include "clayundostack.h"


enum
{
  PROP_0,
  PROP_FILTER,
};


static void    clay_drawable_filter_undo_constructed  (GObject             *object);
static void    clay_drawable_filter_undo_set_property (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void    clay_drawable_filter_undo_get_property (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);
static gint64  clay_drawable_filter_undo_get_memsize  (ClayObject          *object,
                                                       gint64              *gui_size);
static void    clay_drawable_filter_undo_pop          (ClayUndo            *undo,
                                                       ClayUndoMode         undo_mode,
                                                       ClayUndoAccumulator *accum);
static void    clay_drawable_filter_undo_free         (ClayUndo            *undo,
                                                       ClayUndoMode         undo_mode);


G_DEFINE_TYPE (ClayDrawableFilterUndo, clay_drawable_filter_undo, CLAY_TYPE_UNDO)

#define parent_class clay_drawable_filter_undo_parent_class


static void
clay_drawable_filter_undo_class_init (ClayDrawableFilterUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayUndoClass   *undo_class        = CLAY_UNDO_CLASS (klass);

  object_class->constructed          = clay_drawable_filter_undo_constructed;
  object_class->set_property         = clay_drawable_filter_undo_set_property;
  object_class->get_property         = clay_drawable_filter_undo_get_property;

  clay_object_class->get_memsize     = clay_drawable_filter_undo_get_memsize;

  undo_class->pop                    = clay_drawable_filter_undo_pop;
  undo_class->free                   = clay_drawable_filter_undo_free;

  g_object_class_install_property (object_class, PROP_FILTER,
                                   g_param_spec_object ("filter", NULL, NULL,
                                                        CLAY_TYPE_DRAWABLE_FILTER,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_drawable_filter_undo_init (ClayDrawableFilterUndo *undo)
{
}

static void
clay_drawable_filter_undo_constructed (GObject *object)
{
  ClayDrawableFilterUndo *df_undo = CLAY_DRAWABLE_FILTER_UNDO (object);
  ClayDrawable           *drawable;
  ClayContainer          *filter_stack;
  GeglNode               *op;
  gchar                  *op_name;
  GParamSpec             **pspecs;
  guint                    n_pspecs;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_DRAWABLE_FILTER (df_undo->filter) &&
               ! clay_drawable_filter_get_temporary (df_undo->filter));

  drawable = clay_drawable_filter_get_drawable (df_undo->filter);
  if (drawable)
    {
      filter_stack = clay_drawable_get_filters (drawable);

      df_undo->row_index =
        clay_container_get_child_index (filter_stack,
                                        CLAY_OBJECT (df_undo->filter));
    }

  if (CLAY_UNDO (object)->undo_type != CLAY_UNDO_FILTER_VISIBILITY)
    {
      op = clay_drawable_filter_get_operation (df_undo->filter);
      gegl_node_get (op, "operation", &op_name, NULL);

      df_undo->node = gegl_node_new ();
      gegl_node_set (df_undo->node, "operation", op_name, NULL);

      pspecs = gegl_operation_list_properties (op_name, &n_pspecs);
      for (gint i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec = pspecs[i];
          GValue      value = G_VALUE_INIT;

          g_value_init (&value, pspec->value_type);
          gegl_node_get_property (op, pspec->name,
                                  &value);

          gegl_node_set_property (df_undo->node, pspec->name,
                                  &value);
          g_value_unset (&value);
        }

      g_free (pspecs);
      g_free (op_name);
    }

  df_undo->active          = clay_filter_get_active (CLAY_FILTER (df_undo->filter));
  df_undo->opacity         = clay_drawable_filter_get_opacity (df_undo->filter);
  df_undo->paint_mode      = clay_drawable_filter_get_paint_mode (df_undo->filter);
  df_undo->blend_space     = clay_drawable_filter_get_blend_space (df_undo->filter);
  df_undo->composite_space = clay_drawable_filter_get_composite_space (df_undo->filter);
  df_undo->composite_mode  = clay_drawable_filter_get_composite_mode (df_undo->filter);
  df_undo->region          = clay_drawable_filter_get_region (df_undo->filter);
}

static void
clay_drawable_filter_undo_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  ClayDrawableFilterUndo *drawable_filter_undo = CLAY_DRAWABLE_FILTER_UNDO (object);

  switch (property_id)
    {
    case PROP_FILTER:
      drawable_filter_undo->filter = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_drawable_filter_undo_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  ClayDrawableFilterUndo *drawable_filter_undo = CLAY_DRAWABLE_FILTER_UNDO (object);

  switch (property_id)
    {
    case PROP_FILTER:
      g_value_set_object (value, drawable_filter_undo->filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_drawable_filter_undo_get_memsize (ClayObject *object,
                                       gint64     *gui_size)
{
  ClayDrawableFilterUndo *drawable_filter_undo = CLAY_DRAWABLE_FILTER_UNDO (object);
  gint64                  memsize              = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (drawable_filter_undo->filter),
                                      NULL);
  memsize += sizeof (guint32);     /* Row Index */
  memsize += sizeof (gdouble);     /* Opacity */
  memsize += sizeof (guint32) * 5; /* Composite and Region options */

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_drawable_filter_undo_pop (ClayUndo            *undo,
                               ClayUndoMode         undo_mode,
                               ClayUndoAccumulator *accum)
{
  ClayDrawableFilterUndo *df_undo      = CLAY_DRAWABLE_FILTER_UNDO (undo);
  ClayDrawableFilter     *filter       = CLAY_DRAWABLE_FILTER (df_undo->filter);
  ClayDrawable           *drawable     = clay_drawable_filter_get_drawable (filter);
  ClayContainer          *filter_stack = clay_drawable_get_filters (drawable);
  ClayImage              *image        = NULL;
  ClayChannel            *selection    = NULL;

  CLAY_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (drawable)
    image = clay_item_get_image (CLAY_ITEM (drawable));

  if (image)
    {
      /* Temporarily remove selection so filters can be applied correctly */
      selection =
        CLAY_CHANNEL (clay_item_duplicate (CLAY_ITEM (clay_image_get_mask (image)),
                                           CLAY_TYPE_CHANNEL));
      clay_channel_clear (clay_image_get_mask (image), NULL, FALSE);
    }

  if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
       undo->undo_type == CLAY_UNDO_FILTER_ADD) ||
      (undo_mode       == CLAY_UNDO_MODE_REDO &&
       undo->undo_type == CLAY_UNDO_FILTER_REMOVE) )
    {
      if (drawable)
        {
          clay_drawable_remove_filter (drawable, CLAY_FILTER (filter));

          clay_drawable_update (drawable, 0, 0, -1, -1);
        }
    }
  if ((undo_mode       == CLAY_UNDO_MODE_UNDO &&
       undo->undo_type == CLAY_UNDO_FILTER_REMOVE) ||
      (undo_mode       == CLAY_UNDO_MODE_REDO &&
       undo->undo_type == CLAY_UNDO_FILTER_ADD) )
    {
      if (drawable)
        {
          clay_drawable_filter_apply (filter, NULL);
          clay_container_reorder (filter_stack, CLAY_OBJECT (filter),
                                  df_undo->row_index);

          if (clay_viewable_preview_is_frozen (CLAY_VIEWABLE (drawable)))
            clay_viewable_preview_thaw (CLAY_VIEWABLE (drawable));
        }
    }
  else if (undo->undo_type == CLAY_UNDO_FILTER_REORDER)
    {
      clay_container_reorder (filter_stack, CLAY_OBJECT (filter),
                              df_undo->row_index);
      clay_drawable_update (drawable, 0, 0, -1, -1);
    }
  else if (undo->undo_type == CLAY_UNDO_FILTER_VISIBILITY)
    {
      ClayFilter *gfilter;
      gboolean    active;

      gfilter = CLAY_FILTER (filter);
      active  = clay_filter_get_active (gfilter);

      clay_filter_set_active (gfilter, df_undo->active);
      clay_drawable_filter_apply (filter, NULL);

      df_undo->active = active;
    }
  else if (undo->undo_type == CLAY_UNDO_FILTER_MODIFIED)
    {
      GeglNode                *op;
      GParamSpec             **pspecs;
      guint                    n_pspecs;
      gchar                   *op_name = NULL;
      gdouble                  opacity;
      ClayLayerMode            paint_mode;
      ClayLayerColorSpace      blend_space;
      ClayLayerColorSpace      composite_space;
      ClayLayerCompositeMode   composite_mode;
      ClayFilterRegion         region;

      /* Swap values */
      opacity         = clay_drawable_filter_get_opacity (filter);
      paint_mode      = clay_drawable_filter_get_paint_mode (filter);
      blend_space     = clay_drawable_filter_get_blend_space (filter);
      composite_space = clay_drawable_filter_get_composite_space (filter);
      composite_mode  = clay_drawable_filter_get_composite_mode (filter);
      region          = clay_drawable_filter_get_region (filter);

      clay_drawable_filter_set_opacity (filter, df_undo->opacity);
      clay_drawable_filter_set_mode (filter, df_undo->paint_mode,
                                     df_undo->blend_space,
                                     df_undo->composite_space,
                                     df_undo->composite_mode);
      clay_drawable_filter_set_region (filter, df_undo->region);

      df_undo->opacity         = opacity;
      df_undo->paint_mode      = paint_mode;
      df_undo->blend_space     = blend_space;
      df_undo->composite_space = composite_space;
      df_undo->composite_mode  = composite_mode;
      df_undo->region          = region;

      op = clay_drawable_filter_get_operation (df_undo->filter);
      gegl_node_get (op, "operation", &op_name, NULL);

      pspecs = gegl_operation_list_properties (op_name, &n_pspecs);

      for (gint i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec  = pspecs[i];
          GValue      value1 = G_VALUE_INIT;
          GValue      value2 = G_VALUE_INIT;

          g_value_init (&value1, pspec->value_type);
          g_value_init (&value2, pspec->value_type);

          gegl_node_get_property (op, pspec->name, &value1);

          gegl_node_get_property (df_undo->node, pspec->name, &value2);
          gegl_node_set_property (op, pspec->name, &value2);

          gegl_node_set_property (df_undo->node, pspec->name, &value1);

          g_value_unset (&value1);
          g_value_unset (&value2);
        }

      g_free (pspecs);
      g_free (op_name);

      clay_drawable_filter_apply (filter, NULL);
    }

  if (image)
    {
      GeglBuffer *buffer = NULL;

      /* Restore selection after undo or redo */
      buffer = clay_gegl_buffer_dup (clay_drawable_get_buffer (CLAY_DRAWABLE (selection)),
                                     CLAY_DRAWABLE (clay_image_get_mask (image)));
      clay_drawable_set_buffer (CLAY_DRAWABLE (clay_image_get_mask (image)),
                                FALSE, NULL, buffer);
      g_object_unref (buffer);
      g_object_unref (selection);
    }
}

static void
clay_drawable_filter_undo_free (ClayUndo     *undo,
                                ClayUndoMode  undo_mode)
{
  ClayDrawableFilterUndo *drawable_filter_undo = CLAY_DRAWABLE_FILTER_UNDO (undo);

  g_clear_object (&drawable_filter_undo->filter);
  g_clear_object (&drawable_filter_undo->node);

  CLAY_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}

/**
 * Checks whether the undo step of toggling all filters in @filter_list can be
 * compressed. It will be compressed if:
 *    - The previous undo is a group of filter visibility changes
 *    - The group has changes for every filters in the list
 *    - The group only has changes for filters from the list
 */
ClayUndo *
clay_drawable_filter_undo_can_compress_visibility (ClayImage *image,
                                                   GList     *filter_list)
{
  ClayUndo      *undo;
  ClayUndoStack *undo_stack;
  GList         *iter;
  gint           n_items;
  gint           n_drawable_filters = 0;

  g_return_val_if_fail (image != NULL, NULL);
  g_return_val_if_fail (filter_list != NULL, NULL);

  undo = clay_image_undo_can_compress (image,
                                       CLAY_TYPE_UNDO_STACK,
                                       CLAY_UNDO_GROUP_FILTER_VISIBILITY);
  if (undo == NULL)
    return NULL;

  undo_stack = CLAY_UNDO_STACK (undo);
  n_items    = clay_container_get_n_children (undo_stack->undos);

  for (iter = filter_list; iter; iter = g_list_next (iter))
    if (CLAY_IS_DRAWABLE_FILTER (iter->data))
      n_drawable_filters++;

  if (n_items != n_drawable_filters)
    return NULL;

  for (gint i = 0; i < n_items; i++)
    {
      const ClayObject             *sub_undo;
      const ClayDrawableFilterUndo *sub_filter_undo;

      sub_undo = clay_container_get_child_by_index (undo_stack->undos, i);
      sub_filter_undo = CLAY_DRAWABLE_FILTER_UNDO (sub_undo);

      if (sub_filter_undo == NULL ||
          CLAY_UNDO (sub_undo)->undo_type != CLAY_UNDO_FILTER_VISIBILITY ||
          ! g_list_find (filter_list, sub_filter_undo->filter))
        {
          return NULL;
        }
    }

  return undo;
}
