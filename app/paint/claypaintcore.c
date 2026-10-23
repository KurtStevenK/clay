/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2013 Daniel Sabo
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

#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaycolor/claycolor.h"
#include "libclaymath/claymath.h"

#include "paint-types.h"

#include "operations/layer-modes/clay-layer-modes.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-nodes.h"
#include "gegl/clay-gegl-utils.h"
#include "gegl/clayapplicator.h"

#include "core/clay.h"
#include "core/clay-utils.h"
#include "core/claychannel.h"
#include "core/clayimage.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-symmetry.h"
#include "core/clayimage-undo.h"
#include "core/clayimage-undo-push.h"
#include "core/claylayer.h"
#include "core/claylayermask.h"
#include "core/claypickable.h"
#include "core/clayprojection.h"
#include "core/claysymmetry.h"
#include "core/claytempbuf.h"

#include "claypaintcore.h"
#include "claypaintcoreundo.h"
#include "claypaintcore-loops.h"
#include "claypaintoptions.h"

#include "clayairbrush.h"

#include "clay-intl.h"


#define STROKE_BUFFER_INIT_SIZE 2000

enum
{
  PROP_0,
  PROP_UNDO_DESC
};


/*  local function prototypes  */

static void      clay_paint_core_finalize            (GObject          *object);
static void      clay_paint_core_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      clay_paint_core_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean  clay_paint_core_real_start          (ClayPaintCore    *core,
                                                      GList            *drawables,
                                                      ClayPaintOptions *paint_options,
                                                      const ClayCoords *coords,
                                                      GError          **error);
static gboolean  clay_paint_core_real_pre_paint      (ClayPaintCore    *core,
                                                      GList            *drawables,
                                                      ClayPaintOptions *options,
                                                      ClayPaintState    paint_state,
                                                      guint32           time);
static void      clay_paint_core_real_paint          (ClayPaintCore    *core,
                                                      GList            *drawables,
                                                      ClayPaintOptions *options,
                                                      ClaySymmetry     *sym,
                                                      ClayPaintState    paint_state,
                                                      guint32           time);
static void      clay_paint_core_real_post_paint     (ClayPaintCore    *core,
                                                      GList            *drawables,
                                                      ClayPaintOptions *options,
                                                      ClayPaintState    paint_state,
                                                      guint32           time);
static void      clay_paint_core_real_interpolate    (ClayPaintCore    *core,
                                                      GList            *drawables,
                                                      ClayPaintOptions *options,
                                                      guint32           time);
static GeglBuffer *
               clay_paint_core_real_get_paint_buffer (ClayPaintCore    *core,
                                                      ClayDrawable     *drawable,
                                                      ClayPaintOptions *options,
                                                      ClayLayerMode     paint_mode,
                                                      const ClayCoords *coords,
                                                      gint             *paint_buffer_x,
                                                      gint             *paint_buffer_y,
                                                      gint             *paint_width,
                                                      gint             *paint_height);
static ClayUndo* clay_paint_core_real_push_undo      (ClayPaintCore    *core,
                                                      ClayImage        *image,
                                                      const gchar      *undo_desc);


G_DEFINE_TYPE (ClayPaintCore, clay_paint_core, CLAY_TYPE_OBJECT)

#define parent_class clay_paint_core_parent_class

static gint global_core_ID = 1;


static void
clay_paint_core_class_init (ClayPaintCoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_paint_core_finalize;
  object_class->set_property = clay_paint_core_set_property;
  object_class->get_property = clay_paint_core_get_property;

  klass->start               = clay_paint_core_real_start;
  klass->pre_paint           = clay_paint_core_real_pre_paint;
  klass->paint               = clay_paint_core_real_paint;
  klass->post_paint          = clay_paint_core_real_post_paint;
  klass->interpolate         = clay_paint_core_real_interpolate;
  klass->get_paint_buffer    = clay_paint_core_real_get_paint_buffer;
  klass->push_undo           = clay_paint_core_real_push_undo;

  g_object_class_install_property (object_class, PROP_UNDO_DESC,
                                   g_param_spec_string ("undo-desc", NULL, NULL,
                                                        _("Paint"),
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_paint_core_init (ClayPaintCore *core)
{
  core->ID = global_core_ID++;
  core->undo_buffers = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                              NULL, g_object_unref);
  core->original_bounds = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                 NULL, g_free);
}

static void
clay_paint_core_finalize (GObject *object)
{
  ClayPaintCore *core = CLAY_PAINT_CORE (object);

  clay_paint_core_cleanup (core);

  g_clear_pointer (&core->undo_desc, g_free);
  g_hash_table_unref (core->undo_buffers);
  g_hash_table_unref (core->original_bounds);
  if (core->applicators)
    g_hash_table_unref (core->applicators);

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_paint_core_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ClayPaintCore *core = CLAY_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_set_str (&core->undo_desc, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_paint_core_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayPaintCore *core = CLAY_PAINT_CORE (object);

  switch (property_id)
    {
    case PROP_UNDO_DESC:
      g_value_set_string (value, core->undo_desc);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
clay_paint_core_real_start (ClayPaintCore    *core,
                            GList            *drawables,
                            ClayPaintOptions *paint_options,
                            const ClayCoords *coords,
                            GError          **error)
{
  return TRUE;
}

static gboolean
clay_paint_core_real_pre_paint (ClayPaintCore    *core,
                                GList            *drawables,
                                ClayPaintOptions *paint_options,
                                ClayPaintState    paint_state,
                                guint32           time)
{
  return TRUE;
}

static void
clay_paint_core_real_paint (ClayPaintCore    *core,
                            GList            *drawables,
                            ClayPaintOptions *paint_options,
                            ClaySymmetry     *sym,
                            ClayPaintState    paint_state,
                            guint32           time)
{
}

static void
clay_paint_core_real_post_paint (ClayPaintCore    *core,
                                 GList            *drawables,
                                 ClayPaintOptions *paint_options,
                                 ClayPaintState    paint_state,
                                 guint32           time)
{
}

static void
clay_paint_core_real_interpolate (ClayPaintCore    *core,
                                  GList            *drawables,
                                  ClayPaintOptions *paint_options,
                                  guint32           time)
{
  clay_paint_core_paint (core, drawables, paint_options,
                         CLAY_PAINT_STATE_MOTION, time);

  core->last_coords = core->cur_coords;
}

static GeglBuffer *
clay_paint_core_real_get_paint_buffer (ClayPaintCore    *core,
                                       ClayDrawable     *drawable,
                                       ClayPaintOptions *paint_options,
                                       ClayLayerMode     paint_mode,
                                       const ClayCoords *coords,
                                       gint             *paint_buffer_x,
                                       gint             *paint_buffer_y,
                                       gint             *paint_width,
                                       gint             *paint_height)
{
  return NULL;
}

static ClayUndo *
clay_paint_core_real_push_undo (ClayPaintCore *core,
                                ClayImage     *image,
                                const gchar   *undo_desc)
{
  return clay_image_undo_push (image, CLAY_TYPE_PAINT_CORE_UNDO,
                               CLAY_UNDO_PAINT, undo_desc,
                               0,
                               "paint-core", core,
                               NULL);
}


/*  public functions  */

void
clay_paint_core_paint (ClayPaintCore    *core,
                       GList            *drawables,
                       ClayPaintOptions *paint_options,
                       ClayPaintState    paint_state,
                       guint32           time)
{
  ClayPaintCoreClass *core_class;

  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options));

  core_class = CLAY_PAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawables,
                             paint_options,
                             paint_state, time))
    {
      ClaySymmetry *sym;
      ClayImage    *image;

      image = clay_item_get_image (CLAY_ITEM (drawables->data));

      if (paint_state == CLAY_PAINT_STATE_MOTION)
        {
          /* Save coordinates for clay_paint_core_interpolate() */
          core->last_paint.x = core->cur_coords.x;
          core->last_paint.y = core->cur_coords.y;
        }

      sym = g_object_ref (clay_image_get_active_symmetry (image));
      clay_symmetry_set_origin (sym, drawables->data, &core->cur_coords);

      core_class->paint (core, drawables,
                         paint_options,
                         sym, paint_state, time);

      clay_symmetry_clear_origin (sym);
      g_object_unref (sym);

      core_class->post_paint (core, drawables,
                              paint_options,
                              paint_state, time);
    }
}

gboolean
clay_paint_core_start (ClayPaintCore     *core,
                       GList             *drawables,
                       ClayPaintOptions  *paint_options,
                       const ClayCoords  *coords,
                       GError           **error)
{
  ClayImage     *image;
  ClayChannel   *mask;
  gint           max_width  = 0;
  gint           max_height = 0;
  GeglRectangle *rect;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), FALSE);
  g_return_val_if_fail (g_list_length (drawables) > 0, FALSE);
  g_return_val_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (coords != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (GList *iter = drawables; iter; iter = iter->next)
    g_return_val_if_fail (clay_item_is_attached (iter->data), FALSE);

  image = clay_item_get_image (CLAY_ITEM (drawables->data));

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  core->stroke_buffer = g_array_sized_new (TRUE, TRUE,
                                           sizeof (ClayCoords),
                                           STROKE_BUFFER_INIT_SIZE);

  /* remember the last stroke's endpoint for later undo */
  core->start_coords = core->last_coords;
  core->cur_coords   = *coords;

  if (paint_options->use_applicator)
    core->applicators = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_object_unref);
  else
    core->applicators  = NULL;

  if (! CLAY_PAINT_CORE_GET_CLASS (core)->start (core, drawables,
                                                 paint_options,
                                                 coords, error))
    {
      return FALSE;
    }

  /*  Set the image pickable  */
  if (! core->show_all)
    core->image_pickable = CLAY_PICKABLE (image);
  else
    core->image_pickable = CLAY_PICKABLE (clay_image_get_projection (image));

  /*  Allocate the saved proj structure  */
  g_clear_object (&core->saved_proj_buffer);

  if (core->use_saved_proj)
    {
      GeglBuffer *buffer = clay_pickable_get_buffer (core->image_pickable);

      core->saved_proj_buffer = clay_gegl_buffer_dup (buffer, NULL);
    }

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Allocate the undo structures  */
      rect         = g_new (GeglRectangle, 1);
      rect->width  = clay_item_get_width (CLAY_ITEM (iter->data));
      rect->height = clay_item_get_height (CLAY_ITEM (iter->data));
      clay_item_get_offset (CLAY_ITEM (iter->data), &rect->x, &rect->y);

      g_hash_table_insert (core->original_bounds, iter->data,
                           rect);
      g_hash_table_insert (core->undo_buffers, iter->data,
                           clay_gegl_buffer_dup (clay_drawable_get_buffer (iter->data), NULL));

      max_width  = MAX (max_width, clay_item_get_width (iter->data));
      max_height = MAX (max_height, clay_item_get_height (iter->data));
    }

  /*  Allocate the canvas blocks structure  */
  if (core->canvas_buffer)
    g_object_unref (core->canvas_buffer);

  core->canvas_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0, max_width, max_height),
                     babl_format ("Y float"));

  /*  Get the initial undo extents  */

  core->x1 = core->x2 = core->cur_coords.x;
  core->y1 = core->y2 = core->cur_coords.y;

  core->last_paint.x = -1e6;
  core->last_paint.y = -1e6;

  mask = clay_image_get_mask (image);

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (! clay_channel_is_empty (mask) &&
      (g_list_length (drawables) > 1 || CLAY_DRAWABLE (mask) != drawables->data))
    {
      GeglBuffer *mask_buffer;

      mask_buffer = clay_drawable_get_buffer (CLAY_DRAWABLE (mask));

      core->mask_buffer   = g_object_ref (mask_buffer);
    }
  else
    {
      core->mask_buffer = NULL;
    }

  if (paint_options->use_applicator)
    {
      for (GList *iter = drawables; iter; iter = iter->next)
        {
          ClayApplicator *applicator;

          applicator = clay_applicator_new (NULL);
          g_hash_table_insert (core->applicators, iter->data, applicator);

          if (core->mask_buffer)
            {
              gint offset_x;
              gint offset_y;

              clay_applicator_set_mask_buffer (applicator,
                                               core->mask_buffer);
              clay_item_get_offset (iter->data, &offset_x, &offset_y);
              clay_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
            }

          clay_applicator_set_affect (applicator,
                                      clay_drawable_get_active_mask (iter->data));
          clay_applicator_set_dest_buffer (applicator,
                                           clay_drawable_get_buffer (iter->data));
        }
    }

  /* initialize the lock_blink_state */
  core->lock_blink_state = CLAY_PAINT_LOCK_NOT_BLINKED;

  /*  Freeze the drawable preview so that it isn't constantly updated.  */
  for (GList *iter = drawables; iter; iter = iter->next)
    clay_viewable_preview_freeze (CLAY_VIEWABLE (iter->data));

  return TRUE;
}

void
clay_paint_core_finish (ClayPaintCore *core,
                        GList         *drawables,
                        gboolean       push_undo)
{
  ClayImage *image;
  gboolean   undo_group_started = FALSE;

  g_return_if_fail (CLAY_IS_PAINT_CORE (core));

  if (core->applicators)
    {
      g_hash_table_unref (core->applicators);
      core->applicators = NULL;
    }

  if (core->stroke_buffer)
    {
      g_array_free (core->stroke_buffer, TRUE);
      core->stroke_buffer = NULL;
    }

  g_clear_object (&core->mask_buffer);

  image = clay_item_get_image (CLAY_ITEM (drawables->data));

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      /*  Determine if any part of the image has been altered--
       *  if nothing has, then just go to the next drawable...
       */
      if ((core->x2 == core->x1) || (core->y2 == core->y1))
        {
          clay_viewable_preview_thaw (CLAY_VIEWABLE (iter->data));
          continue;
        }

      if (push_undo)
        {
          GeglBuffer    *undo_buffer;
          GeglBuffer    *buffer;
          GeglBuffer    *drawable_buffer;
          GeglRectangle  rect;
          GeglRectangle  old_rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, clay_object_get_name (iter->data));
              continue;
            }

          if (! undo_group_started)
            {
              clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT,
                                           core->undo_desc);
              undo_group_started = TRUE;
            }

          /* get new and old bounds of drawable */
          old_rect = *(GeglRectangle*) g_hash_table_lookup (core->original_bounds, iter->data);

          clay_item_get_offset (CLAY_ITEM (iter->data), &rect.x, &rect.y);
          rect.width = clay_item_get_width (CLAY_ITEM (iter->data));
          rect.height = clay_item_get_height (CLAY_ITEM (iter->data));

          /* Making copy of entire buffer consumes more memory, so do that only when buffer has resized */
          if (rect.x == old_rect.x         &&
              rect.y == old_rect.y         &&
              rect.width == old_rect.width &&
              rect.height == old_rect.height)
            {
              clay_rectangle_intersect (core->x1, core->y1, core->x2 - core->x1,
                                        core->y2 - core->y1, 0, 0,
                                        clay_item_get_width  (CLAY_ITEM (iter->data)),
                                        clay_item_get_height (CLAY_ITEM (iter->data)),
                                        &rect.x, &rect.y, &rect.width, &rect.height);

              gegl_rectangle_align_to_buffer (&rect, &rect, undo_buffer,
                                              GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

              CLAY_PAINT_CORE_GET_CLASS (core)->push_undo (core, image, NULL);

              buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                        clay_drawable_get_format (iter->data));

              clay_gegl_buffer_copy (undo_buffer,
                                     &rect,
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              clay_drawable_push_undo (iter->data, NULL,
                                       buffer, rect.x, rect.y, rect.width, rect.height);
            }
          else
            {
              /* drawable is expanded only if drawable is layer or layer mask*/
              g_return_if_fail (CLAY_IS_LAYER (iter->data) || CLAY_IS_LAYER_MASK (iter->data));

              /* create a copy of original buffer from undo data */
              buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                             old_rect.width,
                                                             old_rect.height),
                                             clay_drawable_get_format (iter->data),
                                             CLAY_DRAWABLE (iter->data));

              clay_gegl_buffer_copy (undo_buffer,
                                     GEGL_RECTANGLE (old_rect.x - rect.x,
                                                     old_rect.y - rect.y,
                                                     old_rect.width,
                                                     old_rect.height),
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              /* make a backup copy of drawable to restore */
              drawable_buffer = g_object_ref (clay_drawable_get_buffer (CLAY_DRAWABLE (iter->data)));

              if (CLAY_IS_LAYER_MASK (drawables->data) || CLAY_LAYER (drawables->data)->mask)
                {
                  GeglBuffer   *other_new;
                  GeglBuffer   *other_old;
                  ClayDrawable *other_drawable;

                  if (CLAY_IS_LAYER_MASK (drawables->data))
                    other_drawable = CLAY_DRAWABLE ((CLAY_LAYER_MASK (drawables->data))->layer);
                  else
                    other_drawable = CLAY_DRAWABLE (CLAY_LAYER (drawables->data)->mask);

                  /* create a copy of original buffer by taking the required area */
                  other_old = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                                    old_rect.width,
                                                                    old_rect.height),
                                                    clay_drawable_get_format (other_drawable),
                                                    other_drawable);

                  clay_gegl_buffer_copy (clay_drawable_get_buffer (other_drawable),
                                         GEGL_RECTANGLE (old_rect.x - rect.x,
                                                         old_rect.y - rect.y,
                                                         old_rect.width,
                                                         old_rect.height),
                                         GEGL_ABYSS_NONE,
                                         other_old,
                                         GEGL_RECTANGLE (0, 0, 0, 0));

                  /* make a backup copy of drawable to restore */
                  other_new = g_object_ref (clay_drawable_get_buffer (other_drawable));

                  clay_drawable_set_buffer_full (other_drawable, FALSE, NULL,
                                                 other_old, &old_rect,
                                                 FALSE);
                  clay_drawable_set_buffer_full (other_drawable, TRUE, NULL,
                                                 other_new, &rect,
                                                 FALSE);

                  g_object_unref (other_new);
                  g_object_unref (other_old);
                }
              /* Restore drawable to state before painting started */
              clay_drawable_set_buffer_full (iter->data, FALSE, NULL,
                                             buffer, &old_rect,
                                             FALSE);
              /* Change the drawable again but push undo this time */
              clay_drawable_set_buffer_full (iter->data, TRUE, NULL,
                                             drawable_buffer, &rect,
                                             FALSE);

              g_object_unref (drawable_buffer);
            }

          g_object_unref (buffer);
          g_object_unref (undo_buffer);
        }

      clay_viewable_preview_thaw (CLAY_VIEWABLE (iter->data));
    }

  core->image_pickable = NULL;

  g_clear_object (&core->saved_proj_buffer);

  if (undo_group_started)
    clay_image_undo_group_end (image);
}

void
clay_paint_core_cancel (ClayPaintCore *core,
                        GList         *drawables)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (CLAY_IS_PAINT_CORE (core));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  for (GList *iter = drawables; iter; iter = iter->next)
    {
      if (clay_rectangle_intersect (core->x1, core->y1,
                                    core->x2 - core->x1,
                                    core->y2 - core->y1,
                                    0, 0,
                                    clay_item_get_width  (CLAY_ITEM (iter->data)),
                                    clay_item_get_height (CLAY_ITEM (iter->data)),
                                    &x, &y, &width, &height))
        {
          GeglBuffer    *undo_buffer;
          GeglRectangle  new_rect;
          GeglRectangle  old_rect;

          if (! g_hash_table_steal_extended (core->undo_buffers, iter->data,
                                             NULL, (gpointer*) &undo_buffer))
            {
              g_critical ("%s: missing undo buffer for '%s'.",
                          G_STRFUNC, clay_object_get_name (iter->data));
              continue;
            }

          old_rect = *(GeglRectangle*) g_hash_table_lookup (core->original_bounds, iter->data);

          clay_item_get_offset (CLAY_ITEM (iter->data), &new_rect.x, &new_rect.y);
          new_rect.width = clay_item_get_width (CLAY_ITEM (iter->data));
          new_rect.height = clay_item_get_height (CLAY_ITEM (iter->data));

          if (new_rect.x == old_rect.x         &&
              new_rect.y == old_rect.y         &&
              new_rect.width == old_rect.width &&
              new_rect.height == old_rect.height)
            {
              GeglRectangle  rect;

              gegl_rectangle_align_to_buffer (&rect,
                                              GEGL_RECTANGLE (x, y, width, height),
                                              clay_drawable_get_buffer (iter->data),
                                              GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

              clay_gegl_buffer_copy (undo_buffer,
                                     &rect,
                                     GEGL_ABYSS_NONE,
                                     clay_drawable_get_buffer (iter->data),
                                     &rect);

              clay_drawable_update (iter->data, x, y, width, height);
            }
          else
            {
              GeglBuffer    *buffer;
              GeglRectangle  bbox;

              /* drawable is expanded only if drawable is layer or layer mask,
               * so drawable cannot be anything else */
              g_return_if_fail (CLAY_IS_LAYER (iter->data) || CLAY_IS_LAYER_MASK (iter->data));

              /* When canceling painting with drawable expansion, ensure that
               * the drawable display outside the reverted size is not shown. We
               * cannot use clay_drawable_update() because it won't work while
               * painting. Directly emit the "update" signal.
               * */
              bbox = clay_drawable_get_bounding_box (iter->data);
              g_signal_emit_by_name (iter->data, "update", bbox.x, bbox.y, bbox.width, bbox.height);

              /* create a copy of original buffer from undo data */
              buffer = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                             old_rect.width,
                                                             old_rect.height),
                                             clay_drawable_get_format (iter->data),
                                             CLAY_DRAWABLE (iter->data));

              clay_gegl_buffer_copy (undo_buffer,
                                     GEGL_RECTANGLE (old_rect.x - new_rect.x,
                                                     old_rect.y - new_rect.y,
                                                     old_rect.width,
                                                     old_rect.height),
                                     GEGL_ABYSS_NONE,
                                     buffer,
                                     GEGL_RECTANGLE (0, 0, 0, 0));

              if (CLAY_IS_LAYER_MASK (drawables->data) || CLAY_LAYER (drawables->data)->mask)
                {
                  GeglBuffer   *other_old;
                  ClayDrawable *other_drawable;

                  if (CLAY_IS_LAYER_MASK (drawables->data))
                    other_drawable = CLAY_DRAWABLE ((CLAY_LAYER_MASK (drawables->data))->layer);
                  else
                    other_drawable = CLAY_DRAWABLE (CLAY_LAYER (drawables->data)->mask);

                  /* create a copy of original buffer by taking the required area */
                  other_old = clay_gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                                    old_rect.width,
                                                                    old_rect.height),
                                                    clay_drawable_get_format (other_drawable),
                                                    other_drawable);

                  clay_gegl_buffer_copy (clay_drawable_get_buffer (other_drawable),
                                         GEGL_RECTANGLE (old_rect.x - new_rect.x,
                                                         old_rect.y - new_rect.y,
                                                         old_rect.width,
                                                         old_rect.height),
                                         GEGL_ABYSS_NONE,
                                         other_old,
                                         GEGL_RECTANGLE (0, 0, 0, 0));

                  clay_drawable_set_buffer_full (other_drawable, FALSE, NULL,
                                                 other_old, &old_rect,
                                                 FALSE);

                  g_object_unref (other_old);
                }
              /* Restore drawable to state before painting started */
              clay_drawable_set_buffer_full (iter->data, FALSE, NULL,
                                             buffer, &old_rect,
                                             FALSE);

              clay_drawable_update (iter->data, 0, 0, -1, -1);

              g_object_unref (buffer);
            }

          g_object_unref (undo_buffer);
        }

      clay_viewable_preview_thaw (CLAY_VIEWABLE (iter->data));
    }

  g_clear_object (&core->saved_proj_buffer);
}

void
clay_paint_core_cleanup (ClayPaintCore *core)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));

  g_hash_table_remove_all (core->undo_buffers);
  g_hash_table_remove_all (core->original_bounds);

  g_clear_object (&core->saved_proj_buffer);
  g_clear_object (&core->canvas_buffer);
  g_clear_object (&core->paint_buffer);
}

void
clay_paint_core_interpolate (ClayPaintCore    *core,
                             GList            *drawables,
                             ClayPaintOptions *paint_options,
                             const ClayCoords *coords,
                             guint32           time)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (drawables != NULL);
  g_return_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;

  CLAY_PAINT_CORE_GET_CLASS (core)->interpolate (core, drawables,
                                                 paint_options, time);
}

void
clay_paint_core_set_show_all (ClayPaintCore *core,
                              gboolean       show_all)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));

  core->show_all = show_all;
}

gboolean
clay_paint_core_get_show_all (ClayPaintCore *core)
{
  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), FALSE);

  return core->show_all;
}


gboolean
clay_paint_core_expand_drawable (ClayPaintCore    *core,
                                 ClayDrawable     *drawable,
                                 ClayPaintOptions *options,
                                 gint              x1,
                                 gint              x2,
                                 gint              y1,
                                 gint              y2,
                                 gint             *new_off_x,
                                 gint             *new_off_y)
{
  gint          drawable_width, drawable_height;
  gint          drawable_offset_x, drawable_offset_y;
  gint          image_width, image_height;
  gint          new_width, new_height;
  gint          expand_amount;
  gboolean      show_all;
  gboolean      outside_image;
  ClayImage    *image     = clay_item_get_image (CLAY_ITEM (drawable));
  ClayLayer    *layer;

  drawable_width  = clay_item_get_width  (CLAY_ITEM (drawable));
  drawable_height = clay_item_get_height (CLAY_ITEM (drawable));
  clay_item_get_offset (CLAY_ITEM (drawable), &drawable_offset_x, &drawable_offset_y);

  new_width  = drawable_width;
  new_height = drawable_height;
  *new_off_x = 0;
  *new_off_y = 0;

  image_width  = clay_image_get_width (image);
  image_height = clay_image_get_height (image);

  expand_amount = options->expand_amount;
  show_all      = clay_paint_core_get_show_all (core);
  outside_image = x2 < -drawable_offset_x || x1 > image_width - drawable_offset_x ||
                  y2 < -drawable_offset_y || y1 > image_height - drawable_offset_y;

  /* Don't expand if drawable is anything other
   * than layer or layer mask */
  if (CLAY_IS_LAYER_MASK (drawable))
    layer = (CLAY_LAYER_MASK (drawable))->layer;
  else if (CLAY_IS_LAYER (drawable))
    layer = CLAY_LAYER (drawable);
  else
    return FALSE;

  if (!clay_paint_core_get_show_all (core) && outside_image)
    return FALSE;

  if (!options->expand_use)
    return FALSE;

  if (x1 < 0)
    {
      if (show_all)
        {
          new_width += expand_amount - x1;
          *new_off_x += expand_amount - x1;
        }
      else if (drawable_offset_x > 0)
        {
          new_width += expand_amount - x1;
          *new_off_x += expand_amount - x1;
          if (*new_off_x > drawable_offset_x)
            {
              new_width -= *new_off_x - drawable_offset_x;
              *new_off_x = drawable_offset_x;
            }
        }
    }
  if (y1 < 0)
    {
      if (show_all)
        {
          new_height += expand_amount - y1;
          *new_off_y += expand_amount - y1;
        }
      else if (drawable_offset_y > 0)
        {
          new_height += expand_amount - y1;
          *new_off_y += expand_amount - y1;
          if (*new_off_y > drawable_offset_y)
            {
              new_height -= *new_off_y - drawable_offset_y;
              *new_off_y = drawable_offset_y;
            }
        }
    }
  if (x2 > drawable_width)
    {
      if (show_all)
        {
          new_width += x2 - drawable_width + expand_amount;
        }
      else if (drawable_width + drawable_offset_x < image_width)
        {
          new_width += x2 - drawable_width + expand_amount;
          if (new_width + drawable_offset_x - *new_off_x > image_width)
            new_width = image_width + *new_off_x - drawable_offset_x;
        }
    }
  if (y2 > drawable_height)
    {
      if (show_all)
        {
          new_height += y2 - drawable_height + expand_amount;
        }
      else if (drawable_height + drawable_offset_y < image_height)
        {
          new_height += y2 - drawable_height + expand_amount;
          if (new_height + drawable_offset_y - *new_off_y > image_height)
            new_height = image_height + *new_off_y - drawable_offset_y;
        }
    }

  if (new_width != drawable_width   || *new_off_x ||
      new_height != drawable_height || *new_off_y)
    {
      GeglColor    *color;
      ClayPattern  *pattern;
      ClayContext  *context   = CLAY_CONTEXT (options);
      ClayFillType  fill_type = options->expand_fill_type;
      gboolean      context_has_image;
      ClayFillType  mask_fill_type;
      GeglBuffer   *undo_buffer;
      GeglBuffer   *new_buffer;

      if (clay_item_get_lock_position (CLAY_ITEM (layer)))
        {
          if (core->lock_blink_state == CLAY_PAINT_LOCK_NOT_BLINKED)
            core->lock_blink_state = CLAY_PAINT_LOCK_BLINK_PENDING;

          /* Since we are not expanding, set new offset to zero */
          *new_off_x = 0;
          *new_off_y = 0;

          return FALSE;
        }

      mask_fill_type = options->expand_mask_fill_type == CLAY_ADD_MASK_BLACK ?
                         CLAY_FILL_TRANSPARENT :
                         CLAY_FILL_WHITE;

      /* The image field of context is null but
       * is required for Filling with Middle Gray */
      context_has_image = context->image != NULL;
      if (!context_has_image)
        context->image = image;

      /* ensure that every expansion is pushed to undo stack */
      if (core->x2 == core->x1)
        core->x2++;
      if (core->y2 == core->y1)
        core->y2++;

      g_object_freeze_notify (G_OBJECT (layer));

      clay_drawable_disable_resize_undo (CLAY_DRAWABLE (layer));
      CLAY_LAYER_GET_CLASS (layer)->resize (layer, context, fill_type,
                                            new_width, new_height,
                                            *new_off_x, *new_off_y);
      clay_drawable_enable_resize_undo (CLAY_DRAWABLE (layer));

      if (layer->mask)
        {
          g_object_freeze_notify (G_OBJECT (layer->mask));

          clay_drawable_disable_resize_undo (CLAY_DRAWABLE (layer->mask));
          CLAY_ITEM_GET_CLASS (layer->mask)->resize (CLAY_ITEM (layer->mask), context,
                                                     mask_fill_type, new_width, new_height,
                                                     *new_off_x, *new_off_y);
          clay_drawable_enable_resize_undo (CLAY_DRAWABLE (layer->mask));

          g_object_thaw_notify (G_OBJECT (layer->mask));
        }

      g_object_thaw_notify (G_OBJECT (layer));

      clay_image_flush (image);

      if (CLAY_IS_LAYER_MASK (drawable))
        {
          fill_type = mask_fill_type;
        }
      else if (fill_type == CLAY_FILL_TRANSPARENT &&
               ! clay_drawable_has_alpha (drawable))
        {
          fill_type = CLAY_FILL_BACKGROUND;
        }

      new_buffer = clay_gegl_buffer_resize (core->canvas_buffer, new_width, new_height,
                                            -(*new_off_x), -(*new_off_y), NULL, NULL, 0, 0);
      g_object_unref (core->canvas_buffer);
      core->canvas_buffer = new_buffer;

      clay_get_fill_params (context, fill_type, &color, &pattern, NULL);
      if (color && ! clay_drawable_has_alpha (drawable))
        clay_color_set_alpha (color, 1.0);

      undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);
      g_object_ref (undo_buffer);

      new_buffer = clay_gegl_buffer_resize (undo_buffer, new_width, new_height,
                                            -(*new_off_x), -(*new_off_y), color,
                                            pattern, 0, 0);
      g_hash_table_insert (core->undo_buffers, drawable, new_buffer);
      g_object_unref (undo_buffer);
      g_clear_object (&color);

      /* Restore context to its original state */
      if (!context_has_image)
        context->image = NULL;

      return TRUE;
    }
  return FALSE;
}

void
clay_paint_core_set_current_coords (ClayPaintCore    *core,
                                    const ClayCoords *coords)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;
}

void
clay_paint_core_get_current_coords (ClayPaintCore    *core,
                                    ClayCoords       *coords)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->cur_coords;
}

void
clay_paint_core_set_last_coords (ClayPaintCore    *core,
                                 const ClayCoords *coords)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->last_coords = *coords;
}

void
clay_paint_core_get_last_coords (ClayPaintCore *core,
                                 ClayCoords    *coords)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->last_coords;
}

/**
 * clay_paint_core_round_line:
 * @core:                   the #ClayPaintCore
 * @options:                the #ClayPaintOptions to use
 * @constrain_15_degrees:   the modifier state
 * @constrain_offset_angle: the angle by which to offset the lines, in degrees
 * @constrain_xres:         the horizontal resolution
 * @constrain_yres:         the vertical resolution
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artifacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
void
clay_paint_core_round_line (ClayPaintCore    *core,
                            ClayPaintOptions *paint_options,
                            gboolean          constrain_15_degrees,
                            gdouble           constrain_offset_angle,
                            gdouble           constrain_xres,
                            gdouble           constrain_yres)
{
  g_return_if_fail (CLAY_IS_PAINT_CORE (core));
  g_return_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options));

  if (clay_paint_options_get_brush_mode (paint_options) == CLAY_BRUSH_HARD)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  if (constrain_15_degrees)
    clay_constrain_line (core->last_coords.x, core->last_coords.y,
                         &core->cur_coords.x, &core->cur_coords.y,
                         CLAY_CONSTRAIN_LINE_15_DEGREES,
                         constrain_offset_angle,
                         constrain_xres, constrain_yres);
}


/*  protected functions  */

GeglBuffer *
clay_paint_core_get_paint_buffer (ClayPaintCore    *core,
                                  ClayDrawable     *drawable,
                                  ClayPaintOptions *paint_options,
                                  ClayLayerMode     paint_mode,
                                  const ClayCoords *coords,
                                  gint             *paint_buffer_x,
                                  gint             *paint_buffer_y,
                                  gint             *paint_width,
                                  gint             *paint_height)
{
  GeglBuffer *paint_buffer;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (CLAY_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (drawable)), NULL);
  g_return_val_if_fail (CLAY_IS_PAINT_OPTIONS (paint_options), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (paint_buffer_x != NULL, NULL);
  g_return_val_if_fail (paint_buffer_y != NULL, NULL);

  paint_buffer =
    CLAY_PAINT_CORE_GET_CLASS (core)->get_paint_buffer (core, drawable,
                                                        paint_options,
                                                        paint_mode,
                                                        coords,
                                                        paint_buffer_x,
                                                        paint_buffer_y,
                                                        paint_width,
                                                        paint_height);

  core->paint_buffer_x = *paint_buffer_x;
  core->paint_buffer_y = *paint_buffer_y;

  return paint_buffer;
}

ClayPickable *
clay_paint_core_get_image_pickable (ClayPaintCore *core)
{
  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->image_pickable != NULL, NULL);

  return core->image_pickable;
}

GeglBuffer *
clay_paint_core_get_orig_image (ClayPaintCore *core,
                                ClayDrawable  *drawable)
{
  GeglBuffer *undo_buffer;

  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), NULL);

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);
  g_return_val_if_fail (undo_buffer != NULL, NULL);

  return undo_buffer;
}

GeglBuffer *
clay_paint_core_get_orig_proj (ClayPaintCore *core)
{
  g_return_val_if_fail (CLAY_IS_PAINT_CORE (core), NULL);
  g_return_val_if_fail (core->saved_proj_buffer != NULL, NULL);

  return core->saved_proj_buffer;
}

void
clay_paint_core_paste (ClayPaintCore            *core,
                       const ClayTempBuf        *paint_mask,
                       gint                      paint_mask_offset_x,
                       gint                      paint_mask_offset_y,
                       ClayDrawable             *drawable,
                       gdouble                   paint_opacity,
                       gdouble                   image_opacity,
                       ClayLayerMode             paint_mode,
                       ClayPaintApplicationMode  mode)
{
  gint               width  = gegl_buffer_get_width  (core->paint_buffer);
  gint               height = gegl_buffer_get_height (core->paint_buffer);
  ClayComponentMask  affect = clay_drawable_get_active_mask (drawable);
  GeglBuffer        *undo_buffer;

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (! affect)
    return;

  if (core->applicators)
    {
      ClayApplicator *applicator;

      applicator = g_hash_table_lookup (core->applicators, drawable);

      /*  If the mode is CONSTANT:
       *   combine the canvas buffer and the paint mask to the paint buffer
       */
      if (mode == CLAY_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                clay_temp_buf_create_buffer ((ClayTempBuf *) paint_mask);

              clay_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            CLAY_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          clay_gegl_apply_mask (core->canvas_buffer,
                                GEGL_RECTANGLE (core->paint_buffer_x,
                                                core->paint_buffer_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                1.0);

          clay_applicator_set_src_buffer (applicator, undo_buffer);
        }
      /*  Otherwise:
       *   combine the paint mask to the paint buffer directly
       */
      else
        {
          GeglBuffer *paint_mask_buffer =
            clay_temp_buf_create_buffer ((ClayTempBuf *) paint_mask);

          clay_gegl_apply_mask (paint_mask_buffer,
                                GEGL_RECTANGLE (paint_mask_offset_x,
                                                paint_mask_offset_y,
                                                width, height),
                                core->paint_buffer,
                                GEGL_RECTANGLE (0, 0, width, height),
                                paint_opacity);

          g_object_unref (paint_mask_buffer);

          clay_applicator_set_src_buffer (applicator,
                                          clay_drawable_get_buffer (drawable));
        }

      clay_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      clay_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      clay_applicator_set_opacity (applicator, image_opacity);
      clay_applicator_set_mode (applicator, paint_mode,
                                CLAY_LAYER_COLOR_SPACE_AUTO,
                                CLAY_LAYER_COLOR_SPACE_AUTO,
                                clay_layer_mode_get_paint_composite_mode (paint_mode));

      /*  apply the paint area to the image  */
      clay_applicator_blit (applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));
    }
  else
    {
      ClayPaintCoreLoopsParams    params = {};
      ClayPaintCoreLoopsAlgorithm algorithms = CLAY_PAINT_CORE_LOOPS_ALGORITHM_NONE;

      params.paint_buf          = clay_gegl_buffer_get_temp_buf (core->paint_buffer);
      params.paint_buf_offset_x = core->paint_buffer_x;
      params.paint_buf_offset_y = core->paint_buffer_y;

      if (! params.paint_buf)
        return;

      params.dest_buffer = clay_drawable_get_buffer (drawable);

      if (mode == CLAY_PAINT_CONSTANT)
        {
          params.canvas_buffer = core->canvas_buffer;

          /* This step is skipped by the ink tool, which writes
           * directly to canvas_buffer
           */
          if (paint_mask != NULL)
            {
              /* Mix paint mask and canvas_buffer */
              params.paint_mask          = paint_mask;
              params.paint_mask_offset_x = paint_mask_offset_x;
              params.paint_mask_offset_y = paint_mask_offset_y;
              params.stipple             = CLAY_IS_AIRBRUSH (core);
              params.paint_opacity       = paint_opacity;

              algorithms |= CLAY_PAINT_CORE_LOOPS_ALGORITHM_COMBINE_PAINT_MASK_TO_CANVAS_BUFFER;
            }

          /* Write canvas_buffer to paint_buf */
          algorithms |= CLAY_PAINT_CORE_LOOPS_ALGORITHM_CANVAS_BUFFER_TO_COMP_MASK;

          /* undo buf -> paint_buf -> dest_buffer */
          params.src_buffer = undo_buffer;
        }
      else
        {
          g_return_if_fail (paint_mask);

          /* Write paint_mask to paint_buf, does not modify canvas_buffer */
          params.paint_mask          = paint_mask;
          params.paint_mask_offset_x = paint_mask_offset_x;
          params.paint_mask_offset_y = paint_mask_offset_y;
          params.paint_opacity       = paint_opacity;

          algorithms |= CLAY_PAINT_CORE_LOOPS_ALGORITHM_PAINT_MASK_TO_COMP_MASK;

          /* dest_buffer -> paint_buf -> dest_buffer */
          params.src_buffer = params.dest_buffer;
        }

      clay_item_get_offset (CLAY_ITEM (drawable),
                            &params.mask_offset_x, &params.mask_offset_y);
      params.mask_offset_x = -params.mask_offset_x;
      params.mask_offset_y = -params.mask_offset_y;
      params.mask_buffer   = core->mask_buffer;
      params.image_opacity = image_opacity;
      params.paint_mode    = paint_mode;

      algorithms |= CLAY_PAINT_CORE_LOOPS_ALGORITHM_DO_LAYER_BLEND;

      if (affect != CLAY_COMPONENT_MASK_ALL)
        {
          params.affect = affect;

          algorithms |= CLAY_PAINT_CORE_LOOPS_ALGORITHM_MASK_COMPONENTS;
        }

      clay_paint_core_loops_process (&params, algorithms);
    }

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  clay_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/* This works similarly to clay_paint_core_paste. However, instead of
 * combining the canvas to the paint core drawable using one of the
 * combination modes, it uses a "replace" mode (i.e. transparent
 * pixels in the canvas erase the paint core drawable).

 * When not drawing on alpha-enabled images, it just paints using
 * NORMAL mode.
 */
void
clay_paint_core_replace (ClayPaintCore            *core,
                         const ClayTempBuf        *paint_mask,
                         gint                      paint_mask_offset_x,
                         gint                      paint_mask_offset_y,
                         ClayDrawable             *drawable,
                         gdouble                   paint_opacity,
                         gdouble                   image_opacity,
                         ClayPaintApplicationMode  mode)
{
  GeglBuffer        *undo_buffer;
  gint               width, height;
  ClayComponentMask  affect;

  if (! clay_drawable_has_alpha (drawable))
    {
      clay_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             CLAY_LAYER_MODE_NORMAL,
                             mode);
      return;
    }

  width  = gegl_buffer_get_width  (core->paint_buffer);
  height = gegl_buffer_get_height (core->paint_buffer);

  affect = clay_drawable_get_active_mask (drawable);

  if (! affect)
    return;

  undo_buffer = g_hash_table_lookup (core->undo_buffers, drawable);

  if (core->applicators)
    {
      ClayApplicator *applicator;
      GeglRectangle   mask_rect;
      GeglBuffer     *mask_buffer;
      gint            offset_x;
      gint            offset_y;

      applicator = g_hash_table_lookup (core->applicators, drawable);

      /*  If the mode is CONSTANT:
       *   combine the paint mask to the canvas buffer, and use it as the mask
       *   buffer
       */
      if (mode == CLAY_PAINT_CONSTANT)
        {
          /* Some tools (ink) paint the mask to paint_core->canvas_buffer
           * directly. Don't need to copy it in this case.
           */
          if (paint_mask != NULL)
            {
              GeglBuffer *paint_mask_buffer =
                clay_temp_buf_create_buffer ((ClayTempBuf *) paint_mask);

              clay_gegl_combine_mask_weird (paint_mask_buffer,
                                            GEGL_RECTANGLE (paint_mask_offset_x,
                                                            paint_mask_offset_y,
                                                            width, height),
                                            core->canvas_buffer,
                                            GEGL_RECTANGLE (core->paint_buffer_x,
                                                            core->paint_buffer_y,
                                                            width, height),
                                            paint_opacity,
                                            CLAY_IS_AIRBRUSH (core));

              g_object_unref (paint_mask_buffer);
            }

          mask_buffer = g_object_ref (core->canvas_buffer);
          mask_rect   = *GEGL_RECTANGLE (core->paint_buffer_x,
                                         core->paint_buffer_y,
                                         width, height);

          clay_applicator_set_src_buffer (applicator, undo_buffer);
        }
      /*  Otherwise:
       *   use the paint mask as the mask buffer directly
       */
      else
        {
          mask_buffer =
            clay_temp_buf_create_buffer ((ClayTempBuf *) paint_mask);
          mask_rect   = *GEGL_RECTANGLE (paint_mask_offset_x,
                                         paint_mask_offset_y,
                                         width, height);

          clay_applicator_set_src_buffer (applicator,
                                          clay_drawable_get_buffer (drawable));
        }

      clay_item_get_offset (CLAY_ITEM (drawable), &offset_x, &offset_y);
      if (core->mask_buffer)
        {
          GeglBuffer    *combined_mask_buffer;
          GeglRectangle  combined_mask_rect;
          GeglRectangle  aligned_combined_mask_rect;

          combined_mask_rect = *GEGL_RECTANGLE (core->paint_buffer_x,
                                                core->paint_buffer_y,
                                                width, height);

          gegl_rectangle_align_to_buffer (
            &aligned_combined_mask_rect, &combined_mask_rect,
            clay_drawable_get_buffer (drawable),
            GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          combined_mask_buffer = gegl_buffer_new (&aligned_combined_mask_rect,
                                                  babl_format ("Y float"));

          clay_gegl_buffer_copy (
            core->mask_buffer,
            GEGL_RECTANGLE (aligned_combined_mask_rect.x + offset_x,
                            aligned_combined_mask_rect.y + offset_y,
                            aligned_combined_mask_rect.width,
                            aligned_combined_mask_rect.height),
            GEGL_ABYSS_NONE,
            combined_mask_buffer,
            &aligned_combined_mask_rect);

          clay_gegl_combine_mask (mask_buffer,          &mask_rect,
                                  combined_mask_buffer, &combined_mask_rect,
                                  1.0);

          g_object_unref (mask_buffer);

          mask_buffer = combined_mask_buffer;
          mask_rect   = combined_mask_rect;
        }

      clay_applicator_set_mask_buffer (applicator, mask_buffer);
      clay_applicator_set_mask_offset (applicator,
                                       core->paint_buffer_x - mask_rect.x,
                                       core->paint_buffer_y - mask_rect.y);

      clay_applicator_set_apply_buffer (applicator,
                                        core->paint_buffer);
      clay_applicator_set_apply_offset (applicator,
                                        core->paint_buffer_x,
                                        core->paint_buffer_y);

      clay_applicator_set_opacity (applicator, image_opacity);
      clay_applicator_set_mode (applicator, CLAY_LAYER_MODE_REPLACE,
                                CLAY_LAYER_COLOR_SPACE_AUTO,
                                CLAY_LAYER_COLOR_SPACE_AUTO,
                                clay_layer_mode_get_paint_composite_mode (
                                  CLAY_LAYER_MODE_REPLACE));

      /*  apply the paint area to the image  */
      clay_applicator_blit (applicator,
                            GEGL_RECTANGLE (core->paint_buffer_x,
                                            core->paint_buffer_y,
                                            width, height));

      clay_applicator_set_mask_buffer (applicator, core->mask_buffer);
      clay_applicator_set_mask_offset (applicator, -offset_x, -offset_y);

      g_object_unref (mask_buffer);
    }
  else
    {
      clay_paint_core_paste (core, paint_mask,
                             paint_mask_offset_x,
                             paint_mask_offset_y,
                             drawable,
                             paint_opacity,
                             image_opacity,
                             CLAY_LAYER_MODE_REPLACE,
                             mode);
      return;
    }

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->paint_buffer_x);
  core->y1 = MIN (core->y1, core->paint_buffer_y);
  core->x2 = MAX (core->x2, core->paint_buffer_x + width);
  core->y2 = MAX (core->y2, core->paint_buffer_y + height);

  /*  Update the drawable  */
  clay_drawable_update (drawable,
                        core->paint_buffer_x,
                        core->paint_buffer_y,
                        width, height);
}

/**
 * Smooth and store coords in the stroke buffer
 */

void
clay_paint_core_smooth_coords (ClayPaintCore    *core,
                               ClayPaintOptions *paint_options,
                               ClayCoords       *coords)
{
  ClaySmoothingOptions *smoothing_options = paint_options->smoothing_options;
  GArray               *history           = core->stroke_buffer;

  if (core->stroke_buffer == NULL)
    return; /* Paint core has not initialized yet */

  if (smoothing_options->use_smoothing &&
      smoothing_options->smoothing_quality > 0)
    {
      gint       i;
      guint      length;
      gint       min_index;
      gdouble    gaussian_weight  = 0.0;
      gdouble    gaussian_weight2 = SQR (smoothing_options->smoothing_factor);
      gdouble    velocity_sum     = 0.0;
      gdouble    scale_sum        = 0.0;

      g_array_append_val (history, *coords);

      if (history->len < 2)
        return; /* Just don't bother, nothing to do */

      coords->x = coords->y = 0.0;

      length = MIN (smoothing_options->smoothing_quality, history->len);

      min_index = history->len - length;

      if (gaussian_weight2 != 0.0)
        gaussian_weight = 1 / (sqrt (2 * G_PI) * smoothing_options->smoothing_factor);

      for (i = history->len - 1; i >= min_index; i--)
        {
          gdouble     rate        = 0.0;
          ClayCoords *next_coords = &g_array_index (history,
                                                    ClayCoords, i);

          if (gaussian_weight2 != 0.0)
            {
              /* We use gaussian function with velocity as a window function */
              velocity_sum += next_coords->velocity * 100;
              rate = gaussian_weight * exp (-velocity_sum * velocity_sum /
                                            (2 * gaussian_weight2));
            }

          scale_sum += rate;
          coords->x += rate * next_coords->x;
          coords->y += rate * next_coords->y;
        }

      if (scale_sum != 0.0)
        {
          coords->x /= scale_sum;
          coords->y /= scale_sum;
        }
    }
}
