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

#include <stdlib.h>
#include <string.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "gegl/clay-babl.h"
#include "gegl/clay-gegl-loops.h"
#include "gegl/clay-gegl-utils.h"

#include "clay.h"
#include "clay-memsize.h"
#include "claychunkiterator.h"
#include "clayimage.h"
#include "claymarshal.h"
#include "claypickable.h"
#include "clayprojectable.h"
#include "clayprojection.h"
#include "claytilehandlerprojectable.h"

#include "clay-log.h"
#include "clay-priorities.h"


/*  chunk size for area updates  */
#define CLAY_PROJECTION_UPDATE_CHUNK_WIDTH  32
#define CLAY_PROJECTION_UPDATE_CHUNK_HEIGHT 32


enum
{
  UPDATE,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


struct _ClayProjectionPrivate
{
  ClayProjectable           *projectable;

  GeglBuffer                *buffer;
  ClayTileHandlerValidate   *validate_handler;

  gint                       priority;

  cairo_region_t            *update_region;
  GeglRectangle              priority_rect;
  ClayChunkIterator         *iter;
  guint                      idle_id;

  gboolean                   invalidate_preview;
};


/*  local function prototypes  */

static void   clay_projection_pickable_iface_init (ClayPickableInterface  *iface);

static void        clay_projection_finalize              (GObject         *object);
static void        clay_projection_set_property          (GObject         *object,
                                                          guint            property_id,
                                                          const GValue    *value,
                                                          GParamSpec      *pspec);
static void        clay_projection_get_property          (GObject         *object,
                                                          guint            property_id,
                                                          GValue          *value,
                                                          GParamSpec      *pspec);

static gint64      clay_projection_get_memsize           (ClayObject      *object,
                                                          gint64          *gui_size);

static void        clay_projection_pickable_flush        (ClayPickable    *pickable);
static ClayImage * clay_projection_get_image             (ClayPickable    *pickable);
static const Babl * clay_projection_get_format           (ClayPickable    *pickable);
static GeglBuffer * clay_projection_get_buffer           (ClayPickable    *pickable);
static gboolean    clay_projection_get_pixel_at          (ClayPickable    *pickable,
                                                          gint             x,
                                                          gint             y,
                                                          const Babl      *format,
                                                          gpointer         pixel);
static gdouble     clay_projection_get_opacity_at        (ClayPickable    *pickable,
                                                          gint             x,
                                                          gint             y);
static void        clay_projection_get_pixel_average     (ClayPickable    *pickable,
                                                          const GeglRectangle *rect,
                                                          const Babl      *format,
                                                          gpointer         pixel);

static void        clay_projection_allocate_buffer       (ClayProjection  *proj);
static void        clay_projection_free_buffer           (ClayProjection  *proj);
static void        clay_projection_add_update_area       (ClayProjection  *proj,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);
static void        clay_projection_update_priority_rect  (ClayProjection  *proj);
static gboolean    clay_projection_chunk_render_start    (GWeakRef        *proj_ref);
static void        clay_projection_chunk_render_stop     (ClayProjection  *proj,
                                                          gboolean         merge);
static gboolean    clay_projection_chunk_render_callback (ClayProjection  *proj);
static gboolean    clay_projection_chunk_render_iteration(ClayProjection  *proj);
static void        clay_projection_paint_area            (ClayProjection  *proj,
                                                          gboolean         now,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h);

static void        clay_projection_projectable_invalidate(ClayProjectable *projectable,
                                                          gint             x,
                                                          gint             y,
                                                          gint             w,
                                                          gint             h,
                                                          ClayProjection  *proj);
static void        clay_projection_projectable_flush     (ClayProjectable *projectable,
                                                          gboolean         invalidate_preview,
                                                          ClayProjection  *proj);
static void
           clay_projection_projectable_structure_changed (ClayProjectable *projectable,
                                                          ClayProjection  *proj);
static void   clay_projection_projectable_bounds_changed (ClayProjectable *projectable,
                                                          gint             old_x,
                                                          gint             old_y,
                                                          ClayProjection  *proj);


G_DEFINE_TYPE_WITH_CODE (ClayProjection, clay_projection, CLAY_TYPE_OBJECT,
                         G_ADD_PRIVATE (ClayProjection)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_PICKABLE,
                                                clay_projection_pickable_iface_init))

#define parent_class clay_projection_parent_class

static guint projection_signals[LAST_SIGNAL] = { 0 };


static void
clay_projection_class_init (ClayProjectionClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  projection_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayProjectionClass, update),
                  NULL, NULL,
                  clay_marshal_VOID__BOOLEAN_INT_INT_INT_INT,
                  G_TYPE_NONE, 5,
                  G_TYPE_BOOLEAN,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  object_class->finalize         = clay_projection_finalize;
  object_class->set_property     = clay_projection_set_property;
  object_class->get_property     = clay_projection_get_property;

  clay_object_class->get_memsize = clay_projection_get_memsize;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
clay_projection_init (ClayProjection *proj)
{
  proj->priv = clay_projection_get_instance_private (proj);
}

static void
clay_projection_pickable_iface_init (ClayPickableInterface *iface)
{
  iface->flush                 = clay_projection_pickable_flush;
  iface->get_image             = clay_projection_get_image;
  iface->get_format            = clay_projection_get_format;
  iface->get_format_with_alpha = clay_projection_get_format; /* sic */
  iface->get_buffer            = clay_projection_get_buffer;
  iface->get_pixel_at          = clay_projection_get_pixel_at;
  iface->get_opacity_at        = clay_projection_get_opacity_at;
  iface->get_pixel_average     = clay_projection_get_pixel_average;
}

static void
clay_projection_finalize (GObject *object)
{
  ClayProjection *proj = CLAY_PROJECTION (object);

  clay_projection_free_buffer (proj);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_projection_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_projection_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ClayProjection *projection = CLAY_PROJECTION (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, projection->priv->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_projection_get_memsize (ClayObject *object,
                             gint64     *gui_size)
{
  ClayProjection *projection = CLAY_PROJECTION (object);
  gint64          memsize    = 0;

  memsize += clay_gegl_pyramid_get_memsize (projection->priv->buffer);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * clay_projection_estimate_memsize:
 * @type:           the projectable's base type
 * @component_type: the projectable's component type
 * @width:          projection width
 * @height:         projection height
 *
 * Calculates a rough estimate of the memory that is required for the
 * projection of an image with the given @width and @height.
 *
 * Returns: a rough estimate of the memory requirements.
 **/
gint64
clay_projection_estimate_memsize (ClayImageBaseType type,
                                  ClayComponentType component_type,
                                  gint              width,
                                  gint              height)
{
  const Babl *format;
  gint64      bytes;

  if (type == CLAY_INDEXED)
    type = CLAY_RGB;

  format = clay_babl_format (type,
                             clay_babl_precision (component_type, FALSE),
                             TRUE, NULL);
  bytes  = babl_format_get_bytes_per_pixel (format);

  /* The pyramid levels constitute a geometric sum with a ratio of 1/4. */
  return bytes * (gint64) width * (gint64) height * 1.33;
}

gboolean
clay_projection_has_alpha (ClayProjection *proj)
{
  GeglBuffer *buffer;

  clay_projection_pickable_flush (CLAY_PICKABLE (proj));
  buffer = clay_projection_get_buffer (CLAY_PICKABLE (proj));

  return clay_gegl_has_alpha (buffer, NULL, TRUE, GEGL_ABYSS_NONE);
}


static void
clay_projection_pickable_flush (ClayPickable *pickable)
{
  ClayProjection *proj = CLAY_PROJECTION (pickable);

  /* create the buffer if it doesn't exist */
  clay_projection_get_buffer (pickable);

  clay_projection_finish_draw (proj);
  clay_projection_flush_now (proj, FALSE);

  if (proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      clay_projectable_invalidate_preview (proj->priv->projectable);
    }
}

static ClayImage *
clay_projection_get_image (ClayPickable *pickable)
{
  ClayProjection *proj = CLAY_PROJECTION (pickable);

  return clay_projectable_get_image (proj->priv->projectable);
}

static const Babl *
clay_projection_get_format (ClayPickable *pickable)
{
  ClayProjection *proj = CLAY_PROJECTION (pickable);

  return clay_projectable_get_format (proj->priv->projectable);
}

static GeglBuffer *
clay_projection_get_buffer (ClayPickable *pickable)
{
  ClayProjection *proj = CLAY_PROJECTION (pickable);

  if (! proj->priv->buffer)
    {
      GeglRectangle bounding_box;

      bounding_box =
        clay_projectable_get_bounding_box (proj->priv->projectable);

      clay_projection_allocate_buffer (proj);

      /*  This used to call clay_tile_handler_validate_invalidate()
       *  which forced the entire projection to be constructed in one
       *  go for new images, causing a potentially huge delay. Now we
       *  initially validate stuff the normal way, which makes the
       *  image appear incrementally, but it keeps everything
       *  responsive.
       */
      clay_projection_add_update_area (proj,
                                       bounding_box.x,     bounding_box.y,
                                       bounding_box.width, bounding_box.height);
      proj->priv->invalidate_preview = TRUE;
      clay_projection_flush (proj);
    }

  return proj->priv->buffer;
}

static gboolean
clay_projection_get_pixel_at (ClayPickable *pickable,
                              gint          x,
                              gint          y,
                              const Babl   *format,
                              gpointer      pixel)
{
  ClayProjection *proj   = CLAY_PROJECTION (pickable);
  GeglBuffer     *buffer = clay_projection_get_buffer (pickable);
  GeglRectangle   bounding_box;

  bounding_box = clay_projectable_get_bounding_box (proj->priv->projectable);

  if (x <  bounding_box.x                      ||
      y <  bounding_box.y                      ||
      x >= bounding_box.x + bounding_box.width ||
      y >= bounding_box.y + bounding_box.height)
    {
      return FALSE;
    }

  gegl_buffer_sample (buffer, x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static gdouble
clay_projection_get_opacity_at (ClayPickable *pickable,
                                gint          x,
                                gint          y)
{
  return CLAY_OPACITY_OPAQUE;
}

static void
clay_projection_get_pixel_average (ClayPickable        *pickable,
                                   const GeglRectangle *rect,
                                   const Babl          *format,
                                   gpointer             pixel)
{
  GeglBuffer *buffer = clay_projection_get_buffer (pickable);

  return clay_gegl_average_color (buffer, rect, TRUE, GEGL_ABYSS_NONE, format,
                                  pixel);
}


/*  public functions  */

ClayProjection *
clay_projection_new (ClayProjectable *projectable)
{
  ClayProjection *proj;

  g_return_val_if_fail (CLAY_IS_PROJECTABLE (projectable), NULL);

  proj = g_object_new (CLAY_TYPE_PROJECTION, NULL);

  proj->priv->projectable = projectable;

  g_signal_connect_object (projectable, "invalidate",
                           G_CALLBACK (clay_projection_projectable_invalidate),
                           proj, 0);
  g_signal_connect_object (projectable, "flush",
                           G_CALLBACK (clay_projection_projectable_flush),
                           proj, 0);
  g_signal_connect_object (projectable, "structure-changed",
                           G_CALLBACK (clay_projection_projectable_structure_changed),
                           proj, 0);
  g_signal_connect_object (projectable, "bounds-changed",
                           G_CALLBACK (clay_projection_projectable_bounds_changed),
                           proj, 0);

  return proj;
}

void
clay_projection_set_priority (ClayProjection *proj,
                              gint            priority)
{
  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  proj->priv->priority = priority;
}

gint
clay_projection_get_priority (ClayProjection *proj)
{
  g_return_val_if_fail (CLAY_IS_PROJECTION (proj), 0);

  return proj->priv->priority;
}

void
clay_projection_set_priority_rect (ClayProjection *proj,
                                   gint            x,
                                   gint            y,
                                   gint            w,
                                   gint            h)
{
  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  proj->priv->priority_rect = *GEGL_RECTANGLE (x, y, w, h);

  clay_projection_update_priority_rect (proj);
}

void
clay_projection_stop_rendering (ClayProjection *proj)
{
  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  clay_projection_chunk_render_stop (proj, TRUE);
}

/**
 * clay_projection_flush:
 * @proj:
 *
 * This requests to render the projection. This function is thread-safe
 * and can be called in any thread.
 *
 * The actual projection painting will happen in the main thread.
 */
void
clay_projection_flush (ClayProjection *proj)
{
  GWeakRef *proj_ref = g_new (GWeakRef, 1);;

  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  g_weak_ref_init (proj_ref, proj);
  /* Construct in chunks - asynchronously in the main thread */
  g_idle_add_full (G_PRIORITY_HIGH_IDLE,
                   (GSourceFunc) clay_projection_chunk_render_start,
                   proj_ref, NULL);
}

/**
 * clay_projection_flush_now:
 * @proj:
 * @direct:
 *
 * This renders the projection immediately. You can only call this from
 * the main thread.
 */
void
clay_projection_flush_now (ClayProjection *proj,
                           gboolean        direct)
{
  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  /* Construct NOW - synchronously */
  if (proj->priv->update_region)
    {
      gint n_rects = cairo_region_num_rectangles (proj->priv->update_region);
      gint i;

      /* Make sure we have a buffer */
      clay_projection_allocate_buffer (proj);

      for (i = 0; i < n_rects; i++)
        {
          cairo_rectangle_int_t rect;

          cairo_region_get_rectangle (proj->priv->update_region,
                                      i, &rect);

          clay_projection_paint_area (proj,
                                      direct,
                                      rect.x,
                                      rect.y,
                                      rect.width,
                                      rect.height);
        }

      /*  Free the update region  */
      g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);
    }
}

void
clay_projection_finish_draw (ClayProjection *proj)
{
  g_return_if_fail (CLAY_IS_PROJECTION (proj));

  if (proj->priv->iter)
    {
      clay_chunk_iterator_set_priority_rect (proj->priv->iter, NULL);

      clay_tile_handler_validate_begin_validate (proj->priv->validate_handler);

      while (clay_projection_chunk_render_iteration (proj));

      clay_tile_handler_validate_end_validate (proj->priv->validate_handler);

      clay_projection_chunk_render_stop (proj, FALSE);
    }
}


/*  private functions  */

static void
clay_projection_allocate_buffer (ClayProjection *proj)
{
  const Babl    *format;
  GeglRectangle  bounding_box;

  if (proj->priv->buffer)
    return;

  format       = clay_projection_get_format (CLAY_PICKABLE (proj));
  bounding_box =
    clay_projectable_get_bounding_box (proj->priv->projectable);

  proj->priv->buffer = gegl_buffer_new (&bounding_box, format);

  proj->priv->validate_handler =
    CLAY_TILE_HANDLER_VALIDATE (
      clay_tile_handler_projectable_new (proj->priv->projectable));

  clay_tile_handler_validate_assign (proj->priv->validate_handler,
                                     proj->priv->buffer);

  g_object_notify (G_OBJECT (proj), "buffer");
}

static void
clay_projection_free_buffer (ClayProjection  *proj)
{
  clay_projection_chunk_render_stop (proj, FALSE);

  g_clear_pointer (&proj->priv->update_region, cairo_region_destroy);

  if (proj->priv->buffer)
    {
      clay_tile_handler_validate_unassign (proj->priv->validate_handler,
                                           proj->priv->buffer);

      g_clear_object (&proj->priv->buffer);
      g_clear_object (&proj->priv->validate_handler);

      g_object_notify (G_OBJECT (proj), "buffer");
    }
}

static void
clay_projection_add_update_area (ClayProjection *proj,
                                 gint            x,
                                 gint            y,
                                 gint            w,
                                 gint            h)
{
  cairo_rectangle_int_t rect;
  GeglRectangle         bounding_box;

  bounding_box = clay_projectable_get_bounding_box (proj->priv->projectable);

  /*  align the rectangle to the UPDATE_CHUNK_WIDTH x UPDATE_CHUNK_HEIGHT grid,
   *  to decrease the complexity of the update area.
   */
  w = ceil  ((gdouble) (x + w) / CLAY_PROJECTION_UPDATE_CHUNK_WIDTH ) * CLAY_PROJECTION_UPDATE_CHUNK_WIDTH;
  h = ceil  ((gdouble) (y + h) / CLAY_PROJECTION_UPDATE_CHUNK_HEIGHT) * CLAY_PROJECTION_UPDATE_CHUNK_HEIGHT;
  x = floor ((gdouble) x       / CLAY_PROJECTION_UPDATE_CHUNK_WIDTH ) * CLAY_PROJECTION_UPDATE_CHUNK_WIDTH;
  y = floor ((gdouble) y       / CLAY_PROJECTION_UPDATE_CHUNK_HEIGHT) * CLAY_PROJECTION_UPDATE_CHUNK_HEIGHT;

  w -= x;
  h -= y;

  if (gegl_rectangle_intersect ((GeglRectangle *) &rect,
                                GEGL_RECTANGLE (x, y, w, h), &bounding_box))
    {
      if (proj->priv->update_region)
        cairo_region_union_rectangle (proj->priv->update_region, &rect);
      else
        proj->priv->update_region = cairo_region_create_rectangle (&rect);
    }
}

static void
clay_projection_update_priority_rect (ClayProjection *proj)
{
  if (proj->priv->iter)
    {
      GeglRectangle rect;
      GeglRectangle bounding_box;
      gint          off_x, off_y;

      rect = proj->priv->priority_rect;

      clay_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
      bounding_box = clay_projectable_get_bounding_box (proj->priv->projectable);

      /*  subtract the projectable's offsets because the list of update
       *  areas is in tile-pyramid coordinates, but our external API is
       *  always in terms of image coordinates.
       */
      rect.x -= off_x;
      rect.y -= off_y;

      gegl_rectangle_intersect (&rect, &rect, &bounding_box);

      clay_chunk_iterator_set_priority_rect (proj->priv->iter, &rect);
    }
}

static gboolean
clay_projection_chunk_render_start (GWeakRef *proj_ref)
{
  ClayProjection *proj;

  proj = g_weak_ref_get (proj_ref);
  g_weak_ref_clear (proj_ref);
  g_free (proj_ref);

  if (proj == NULL)
    return G_SOURCE_REMOVE;

  if (proj->priv->update_region)
    {
      cairo_region_t *region             = proj->priv->update_region;
      gboolean        invalidate_preview = FALSE;

      /* Make sure we have a buffer */
      clay_projection_allocate_buffer (proj);

      if (proj->priv->iter)
        {
          region = clay_chunk_iterator_stop (proj->priv->iter, FALSE);

          proj->priv->iter = NULL;

          if (cairo_region_is_empty (region))
            invalidate_preview = proj->priv->invalidate_preview;

          if (proj->priv->update_region)
            {
              cairo_region_union (region, proj->priv->update_region);

              cairo_region_destroy (proj->priv->update_region);
            }
        }

      proj->priv->update_region = NULL;

      if (region && ! cairo_region_is_empty (region))
        {
          proj->priv->iter = clay_chunk_iterator_new (region);

          clay_projection_update_priority_rect (proj);

          if (! proj->priv->idle_id)
            {
              proj->priv->idle_id = g_idle_add_full (CLAY_PRIORITY_PROJECTION_IDLE + proj->priv->priority,
                                                     (GSourceFunc) clay_projection_chunk_render_callback,
                                                     proj, NULL);
            }
        }
      else
        {
          if (region)
            cairo_region_destroy (region);

          if (proj->priv->idle_id)
            {
              g_source_remove (proj->priv->idle_id);
              proj->priv->idle_id = 0;
            }

          if (invalidate_preview)
            {
              /* invalidate the preview here since it is constructed from
               * the projection
               */
              proj->priv->invalidate_preview = FALSE;

              clay_projectable_invalidate_preview (proj->priv->projectable);
            }
        }
    }
  else if (! proj->priv->iter && proj->priv->invalidate_preview)
    {
      /* invalidate the preview here since it is constructed from
       * the projection
       */
      proj->priv->invalidate_preview = FALSE;

      clay_projectable_invalidate_preview (proj->priv->projectable);
    }

  g_object_unref (proj);

  return G_SOURCE_REMOVE;
}

static void
clay_projection_chunk_render_stop (ClayProjection *proj,
                                   gboolean        merge)
{
  if (proj->priv->idle_id)
    {
      g_source_remove (proj->priv->idle_id);
      proj->priv->idle_id = 0;
    }

  if (proj->priv->iter)
    {
      if (merge)
        {
          cairo_region_t *region;

          region = clay_chunk_iterator_stop (proj->priv->iter, FALSE);

          if (proj->priv->update_region)
            {
              cairo_region_union (proj->priv->update_region, region);

              cairo_region_destroy (region);
            }
          else
            {
              proj->priv->update_region = region;
            }
        }
      else
        {
          clay_chunk_iterator_stop (proj->priv->iter, TRUE);
        }

      proj->priv->iter = NULL;
    }
}

static gboolean
clay_projection_chunk_render_callback (ClayProjection *proj)
{
  if (clay_projection_chunk_render_iteration (proj))
    {
      return G_SOURCE_CONTINUE;
    }
  else
    {
      proj->priv->idle_id = 0;

      return G_SOURCE_REMOVE;
    }
}

static gboolean
clay_projection_chunk_render_iteration (ClayProjection *proj)
{
  if (clay_chunk_iterator_next (proj->priv->iter))
    {
      GeglRectangle rect;

      clay_tile_handler_validate_begin_validate (proj->priv->validate_handler);

      while (clay_chunk_iterator_get_rect (proj->priv->iter, &rect))
        {
          clay_projection_paint_area (proj, TRUE,
                                      rect.x, rect.y, rect.width, rect.height);
        }

      clay_tile_handler_validate_end_validate (proj->priv->validate_handler);

      /* Still work to do. */
      return TRUE;
    }
  else
    {
      proj->priv->iter = NULL;

      if (proj->priv->invalidate_preview)
        {
          /* invalidate the preview here since it is constructed from
           * the projection
           */
          proj->priv->invalidate_preview = FALSE;

          clay_projectable_invalidate_preview (proj->priv->projectable);
        }

      /* FINISHED */
      return FALSE;
    }
}

static void
clay_projection_paint_area (ClayProjection *proj,
                            gboolean        now,
                            gint            x,
                            gint            y,
                            gint            w,
                            gint            h)
{
  gint          off_x, off_y;
  GeglRectangle bounding_box;
  GeglRectangle rect;

  clay_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);
  bounding_box = clay_projectable_get_bounding_box (proj->priv->projectable);

  if (gegl_rectangle_intersect (&rect,
                                GEGL_RECTANGLE (x, y, w, h), &bounding_box))
    {
      if (now)
        {
          clay_tile_handler_validate_validate (
            proj->priv->validate_handler,
            proj->priv->buffer,
            &rect,
            FALSE, FALSE);
        }
      else
        {
          clay_tile_handler_validate_invalidate (
            proj->priv->validate_handler,
            &rect);
        }

      /*  add the projectable's offsets because the list of update areas
       *  is in tile-pyramid coordinates, but our external API is always
       *  in terms of image coordinates.
       */
      g_signal_emit (proj, projection_signals[UPDATE], 0,
                     now,
                     rect.x + off_x,
                     rect.y + off_y,
                     rect.width,
                     rect.height);
    }
}


/*  image callbacks  */

static void
clay_projection_projectable_invalidate (ClayProjectable *projectable,
                                        gint             x,
                                        gint             y,
                                        gint             w,
                                        gint             h,
                                        ClayProjection  *proj)
{
  gint off_x, off_y;

  clay_projectable_get_offset (proj->priv->projectable, &off_x, &off_y);

  /*  subtract the projectable's offsets because the list of update
   *  areas is in tile-pyramid coordinates, but our external API is
   *  always in terms of image coordinates.
   */
  x -= off_x;
  y -= off_y;

  clay_projection_add_update_area (proj, x, y, w, h);
}

static void
clay_projection_projectable_flush (ClayProjectable *projectable,
                                   gboolean         invalidate_preview,
                                   ClayProjection  *proj)
{
  if (invalidate_preview)
    proj->priv->invalidate_preview = TRUE;

  clay_projection_flush (proj);
}

static void
clay_projection_projectable_structure_changed (ClayProjectable *projectable,
                                               ClayProjection  *proj)
{
  GeglRectangle bounding_box;

  clay_projection_free_buffer (proj);

  bounding_box = clay_projectable_get_bounding_box (projectable);

  clay_projection_add_update_area (proj,
                                   bounding_box.x,     bounding_box.y,
                                   bounding_box.width, bounding_box.height);
}

static void
clay_projection_projectable_bounds_changed (ClayProjectable *projectable,
                                            gint             old_x,
                                            gint             old_y,
                                            ClayProjection  *proj)
{
  GeglBuffer              *old_buffer = proj->priv->buffer;
  ClayTileHandlerValidate *old_validate_handler;
  GeglRectangle            old_bounding_box;
  GeglRectangle            bounding_box;
  GeglRectangle            old_bounds;
  GeglRectangle            bounds;
  GeglRectangle            int_bounds;
  gint                     x, y;
  gint                     dx, dy;

  if (! old_buffer)
    {
      clay_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  old_bounding_box = *gegl_buffer_get_extent (old_buffer);

  clay_projectable_get_offset (projectable, &x, &y);
  bounding_box = clay_projectable_get_bounding_box (projectable);

  if (x == old_x && y == old_y &&
      gegl_rectangle_equal (&bounding_box, &old_bounding_box))
    {
      return;
    }

  old_bounds    = old_bounding_box;
  old_bounds.x += old_x;
  old_bounds.y += old_y;

  bounds        = bounding_box;
  bounds.x     += x;
  bounds.y     += y;

  if (! gegl_rectangle_intersect (&int_bounds, &bounds, &old_bounds))
    {
      clay_projection_projectable_structure_changed (projectable, proj);

      return;
    }

  dx = x - old_x;
  dy = y - old_y;

#if 1
  /* FIXME:  when there's an offset between the new bounds and the old bounds,
   * use clay_projection_projectable_structure_changed(), instead of copying a
   * shifted version of the old buffer, since the synchronous copy can take a
   * notable amount of time for big buffers, when the offset is such that tiles
   * are not COW-ed.  while clay_projection_projectable_structure_changed()
   * causes the projection to be re-rendered, which is overall slower, it's
   * done asynchronously.
   *
   * this needs to be improved.
   */
  if (dx || dy)
    {
      clay_projection_projectable_structure_changed (projectable, proj);

      return;
    }
#endif

  /* reallocate the buffer, and copy the old buffer to the corresponding
   * region of the new buffer.
   */

  clay_projection_chunk_render_stop (proj, TRUE);

  if (dx == 0 && dy == 0)
    {
      clay_tile_handler_validate_buffer_set_extent (old_buffer, &bounding_box);
    }
  else
    {
      old_validate_handler = proj->priv->validate_handler;

      proj->priv->buffer           = NULL;
      proj->priv->validate_handler = NULL;

      clay_projection_allocate_buffer (proj);

      clay_tile_handler_validate_buffer_copy (
        old_buffer,
        GEGL_RECTANGLE (int_bounds.x - old_x,
                        int_bounds.y - old_y,
                        int_bounds.width,
                        int_bounds.height),
        proj->priv->buffer,
        GEGL_RECTANGLE (int_bounds.x - x,
                        int_bounds.y - y,
                        int_bounds.width,
                        int_bounds.height));

      clay_tile_handler_validate_unassign (old_validate_handler,
                                           old_buffer);

      g_object_unref (old_validate_handler);
      g_object_unref (old_buffer);
    }

  if (proj->priv->update_region)
    {
      cairo_region_translate (proj->priv->update_region, dx, dy);
      cairo_region_intersect_rectangle (
        proj->priv->update_region,
        (const cairo_rectangle_int_t *) &bounding_box);
    }

  int_bounds.x -= x;
  int_bounds.y -= y;

  if (int_bounds.x > bounding_box.x)
    {
      clay_projection_add_update_area (proj,
                                       bounding_box.x,
                                       bounding_box.y,
                                       int_bounds.x - bounding_box.x,
                                       bounding_box.height);
    }
  if (int_bounds.y > bounding_box.y)
    {
      clay_projection_add_update_area (proj,
                                       bounding_box.x,
                                       bounding_box.y,
                                       bounding_box.width,
                                       int_bounds.y - bounding_box.y);
    }
  if (int_bounds.x + int_bounds.width < bounding_box.x + bounding_box.width)
    {
      clay_projection_add_update_area (proj,
                                       int_bounds.x + int_bounds.width,
                                       bounding_box.y,
                                       bounding_box.x + bounding_box.width -
                                       (int_bounds.x  + int_bounds.width),
                                       bounding_box.height);
    }
  if (int_bounds.y + int_bounds.height < bounding_box.y + bounding_box.height)
    {
      clay_projection_add_update_area (proj,
                                       bounding_box.x,
                                       int_bounds.y + int_bounds.height,
                                       bounding_box.width,
                                       bounding_box.y + bounding_box.height -
                                       (int_bounds.y  + int_bounds.height));
    }

  proj->priv->invalidate_preview = TRUE;
}
