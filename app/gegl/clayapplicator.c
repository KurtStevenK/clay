/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayapplicator.c
 * Copyright (C) 2012-2013 Michael Natterer <mitch@gimp.org>
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

#include "clay-gegl-types.h"

#include "clay-gegl-nodes.h"
#include "clayapplicator.h"


static void   clay_applicator_finalize     (GObject      *object);
static void   clay_applicator_set_property (GObject      *object,
                                            guint         property_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void   clay_applicator_get_property (GObject      *object,
                                            guint         property_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);


G_DEFINE_TYPE (ClayApplicator, clay_applicator, G_TYPE_OBJECT)

#define parent_class clay_applicator_parent_class


static void
clay_applicator_class_init (ClayApplicatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = clay_applicator_finalize;
  object_class->set_property = clay_applicator_set_property;
  object_class->get_property = clay_applicator_get_property;
}

static void
clay_applicator_init (ClayApplicator *applicator)
{
  applicator->active          = TRUE;
  applicator->opacity         = 1.0;
  applicator->paint_mode      = CLAY_LAYER_MODE_NORMAL;
  applicator->blend_space     = CLAY_LAYER_COLOR_SPACE_AUTO;
  applicator->composite_space = CLAY_LAYER_COLOR_SPACE_AUTO;
  applicator->composite_mode  = CLAY_LAYER_COMPOSITE_AUTO;
  applicator->affect          = CLAY_COMPONENT_MASK_ALL;
}

static void
clay_applicator_finalize (GObject *object)
{
  ClayApplicator *applicator = CLAY_APPLICATOR (object);

  g_clear_object (&applicator->node);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_applicator_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_applicator_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

ClayApplicator *
clay_applicator_new (GeglNode *parent)
{
  ClayApplicator *applicator;

  g_return_val_if_fail (parent == NULL || GEGL_IS_NODE (parent), NULL);

  applicator = g_object_new (CLAY_TYPE_APPLICATOR, NULL);

  if (parent)
    applicator->node = g_object_ref (parent);
  else
    applicator->node = gegl_node_new ();

  applicator->input_node =
    gegl_node_get_input_proxy  (applicator->node, "input");

  applicator->aux_node =
    gegl_node_get_input_proxy  (applicator->node, "aux");

  applicator->output_node =
    gegl_node_get_output_proxy (applicator->node, "output");

  applicator->mode_node = gegl_node_new_child (applicator->node,
                                               "operation", "clay:normal",
                                               NULL);

  clay_gegl_mode_node_set_mode (applicator->mode_node,
                                applicator->paint_mode,
                                applicator->blend_space,
                                applicator->composite_space,
                                applicator->composite_mode);
  clay_gegl_mode_node_set_opacity (applicator->mode_node,
                                   applicator->opacity);

  gegl_node_link (applicator->input_node, applicator->mode_node);

  applicator->apply_offset_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:translate",
                         NULL);

  gegl_node_link_many (applicator->aux_node,
                       applicator->apply_offset_node,
                       NULL);

  gegl_node_connect (applicator->apply_offset_node, "output",
                     applicator->mode_node,         "aux");

  applicator->mask_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:buffer-source",
                         NULL);

  applicator->mask_offset_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:translate",
                         NULL);

  gegl_node_link (applicator->mask_node, applicator->mask_offset_node);
  /* don't connect the the mask offset node to mode's aux2 yet */

  applicator->affect_node =
    gegl_node_new_child (applicator->node,
                         "operation", "clay:mask-components",
                         "mask",      applicator->affect,
                         NULL);

  applicator->convert_format_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:nop",
                         NULL);

  applicator->cache_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:nop",
                         NULL);

  applicator->crop_node =
    gegl_node_new_child (applicator->node,
                         "operation", "gegl:nop",
                         NULL);

  gegl_node_link_many (applicator->input_node,
                       applicator->affect_node,
                       applicator->convert_format_node,
                       applicator->cache_node,
                       applicator->crop_node,
                       applicator->output_node,
                       NULL);

  gegl_node_connect (applicator->mode_node,   "output",
                     applicator->affect_node, "aux");

  return applicator;
}

void
clay_applicator_set_active (ClayApplicator *applicator,
                            gboolean        active)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (active != applicator->active)
    {
      applicator->active = active;

      if (active)
        gegl_node_link (applicator->crop_node, applicator->output_node);
      else
        gegl_node_link (applicator->input_node, applicator->output_node);
    }
}

void
clay_applicator_set_src_buffer (ClayApplicator *applicator,
                                GeglBuffer     *src_buffer)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));
  g_return_if_fail (src_buffer == NULL || GEGL_IS_BUFFER (src_buffer));

  if (src_buffer == applicator->src_buffer)
    return;

  if (src_buffer)
    {
      if (! applicator->src_node)
        {
          applicator->src_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:buffer-source",
                                 "buffer",    src_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->src_node,
                         "buffer", src_buffer,
                         NULL);
        }

      if (! applicator->src_buffer)
        gegl_node_link (applicator->src_node, applicator->input_node);
    }
  else if (applicator->src_buffer)
    {
      gegl_node_disconnect (applicator->input_node, "input");

      gegl_node_set (applicator->src_node,
                     "buffer", NULL,
                     NULL);
    }

  applicator->src_buffer = src_buffer;
}

void
clay_applicator_set_dest_buffer (ClayApplicator *applicator,
                                 GeglBuffer     *dest_buffer)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));
  g_return_if_fail (dest_buffer == NULL || GEGL_IS_BUFFER (dest_buffer));

  if (dest_buffer == applicator->dest_buffer)
    return;

  if (dest_buffer)
    {
      if (! applicator->dest_node)
        {
          applicator->dest_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:write-buffer",
                                 "buffer",    dest_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->dest_node,
                         "buffer", dest_buffer,
                         NULL);
        }

      if (! applicator->dest_buffer)
        gegl_node_link (applicator->affect_node, applicator->dest_node);
    }
  else if (applicator->dest_buffer)
    {
      gegl_node_disconnect (applicator->dest_node, "input");

      gegl_node_set (applicator->dest_node,
                     "buffer", NULL,
                     NULL);
    }

  applicator->dest_buffer = dest_buffer;
}

void
clay_applicator_set_mask_buffer (ClayApplicator *applicator,
                                 GeglBuffer     *mask_buffer)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));
  g_return_if_fail (mask_buffer == NULL || GEGL_IS_BUFFER (mask_buffer));

  if (applicator->mask_buffer == mask_buffer)
    return;

  gegl_node_set (applicator->mask_node,
                 "buffer", mask_buffer,
                 NULL);

  if (mask_buffer)
    {
      gegl_node_connect (applicator->mask_offset_node, "output",
                         applicator->mode_node,        "aux2");
    }
  else
    {
      gegl_node_disconnect (applicator->mode_node, "aux2");
    }

  applicator->mask_buffer = mask_buffer;
}

void
clay_applicator_set_mask_offset (ClayApplicator *applicator,
                                 gint            mask_offset_x,
                                 gint            mask_offset_y)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->mask_offset_x != mask_offset_x ||
      applicator->mask_offset_y != mask_offset_y)
    {
      applicator->mask_offset_x = mask_offset_x;
      applicator->mask_offset_y = mask_offset_y;

      gegl_node_set (applicator->mask_offset_node,
                     "x", (gdouble) mask_offset_x,
                     "y", (gdouble) mask_offset_y,
                     NULL);
    }
}

void
clay_applicator_set_apply_buffer (ClayApplicator *applicator,
                                  GeglBuffer     *apply_buffer)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));
  g_return_if_fail (apply_buffer == NULL || GEGL_IS_BUFFER (apply_buffer));

  if (apply_buffer == applicator->apply_buffer)
    return;

  if (apply_buffer)
    {
      if (! applicator->apply_src_node)
        {
          applicator->apply_src_node =
            gegl_node_new_child (applicator->node,
                                 "operation", "gegl:buffer-source",
                                 "buffer",    apply_buffer,
                                 NULL);
        }
      else
        {
          gegl_node_set (applicator->apply_src_node,
                         "buffer", apply_buffer,
                         NULL);
        }

      if (! applicator->apply_buffer)
        {
          gegl_node_connect (applicator->apply_src_node,    "output",
                             applicator->apply_offset_node, "input");
        }
    }
  else if (applicator->apply_buffer)
    {
      gegl_node_link (applicator->aux_node, applicator->apply_offset_node);
    }

  applicator->apply_buffer = apply_buffer;
}

void
clay_applicator_set_apply_offset (ClayApplicator *applicator,
                                  gint            apply_offset_x,
                                  gint            apply_offset_y)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->apply_offset_x != apply_offset_x ||
      applicator->apply_offset_y != apply_offset_y)
    {
      applicator->apply_offset_x = apply_offset_x;
      applicator->apply_offset_y = apply_offset_y;

      gegl_node_set (applicator->apply_offset_node,
                     "x", (gdouble) apply_offset_x,
                     "y", (gdouble) apply_offset_y,
                     NULL);
    }
}

void
clay_applicator_set_opacity (ClayApplicator *applicator,
                             gdouble         opacity)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->opacity != opacity)
    {
      applicator->opacity = opacity;

      clay_gegl_mode_node_set_opacity (applicator->mode_node,
                                       opacity);
    }
}

void
clay_applicator_set_mode (ClayApplicator         *applicator,
                          ClayLayerMode           paint_mode,
                          ClayLayerColorSpace     blend_space,
                          ClayLayerColorSpace     composite_space,
                          ClayLayerCompositeMode  composite_mode)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->paint_mode      != paint_mode      ||
      applicator->blend_space     != blend_space     ||
      applicator->composite_space != composite_space ||
      applicator->composite_mode  != composite_mode)
    {
      applicator->paint_mode      = paint_mode;
      applicator->blend_space     = blend_space;
      applicator->composite_space = composite_space;
      applicator->composite_mode  = composite_mode;

      clay_gegl_mode_node_set_mode (applicator->mode_node,
                                    paint_mode, blend_space,
                                    composite_space, composite_mode);
    }
}

void
clay_applicator_set_affect (ClayApplicator    *applicator,
                            ClayComponentMask  affect)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->affect != affect)
    {
      applicator->affect = affect;

      gegl_node_set (applicator->affect_node,
                     "mask", affect,
                     NULL);
    }
}

gboolean
clay_applicator_set_output_format (ClayApplicator *applicator,
                                   const Babl     *format)
{
  gboolean changed = FALSE;

  g_return_val_if_fail (CLAY_IS_APPLICATOR (applicator), FALSE);

  if (applicator->output_format != format)
    {
      changed = TRUE;

      if (format)
        {
          if (! applicator->output_format)
            {
              gegl_node_set (applicator->convert_format_node,
                             "operation", "gegl:convert-format",
                             "format",    format,
                             NULL);
            }
          else
            {
              gegl_node_set (applicator->convert_format_node,
                             "format", format,
                             NULL);
            }
        }
      else
        {
          gegl_node_set (applicator->convert_format_node,
                         "operation", "gegl:nop",
                         NULL);
        }

      applicator->output_format = format;
    }

  return changed;
}

const Babl *
clay_applicator_get_output_format (ClayApplicator *applicator)
{
  g_return_val_if_fail (CLAY_IS_APPLICATOR (applicator), NULL);

  return applicator->output_format;
}

void
clay_applicator_set_cache (ClayApplicator *applicator,
                           gboolean        enable)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->cache_enabled != enable)
    {
      if (enable)
        {
          gegl_node_set (applicator->cache_node,
                         "operation", "gegl:cache",
                         NULL);
        }
      else
        {
          gegl_node_set (applicator->cache_node,
                         "operation", "gegl:nop",
                         NULL);
        }

      applicator->cache_enabled = enable;
    }
}

gboolean
clay_applicator_get_cache (ClayApplicator *applicator)
{
  g_return_val_if_fail (CLAY_IS_APPLICATOR (applicator), FALSE);

  return applicator->cache_enabled;
}

gboolean gegl_buffer_list_valid_rectangles (GeglBuffer     *buffer,
                                            GeglRectangle **rectangles,
                                            gint           *n_rectangles);

GeglBuffer *
clay_applicator_get_cache_buffer (ClayApplicator  *applicator,
                                  GeglRectangle  **rectangles,
                                  gint            *n_rectangles)
{
  g_return_val_if_fail (CLAY_IS_APPLICATOR (applicator), NULL);
  g_return_val_if_fail (rectangles != NULL, NULL);
  g_return_val_if_fail (n_rectangles != NULL, NULL);

  if (applicator->cache_enabled)
    {
      GeglBuffer *cache;

      gegl_node_get (applicator->cache_node,
                     "cache", &cache,
                     NULL);

      if (cache)
        {
          if (gegl_buffer_list_valid_rectangles (cache,
                                                 rectangles, n_rectangles))
            {
              return cache;
            }

          g_object_unref (cache);
        }
    }

  return NULL;
}

void
clay_applicator_set_crop (ClayApplicator      *applicator,
                          const GeglRectangle *rect)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  if (applicator->crop_enabled != (rect != NULL) ||
      (rect && ! gegl_rectangle_equal (&applicator->crop_rect, rect)))
    {
      if (rect)
        {
          if (! applicator->crop_enabled)
            {
              gegl_node_set (applicator->crop_node,
                             "operation", "clay:compose-crop",
                             "x",         rect->x,
                             "y",         rect->y,
                             "width",     rect->width,
                             "height",    rect->height,
                             NULL);

              gegl_node_connect (applicator->input_node, "output",
                                 applicator->crop_node,  "aux");
            }
          else
            {
              gegl_node_set (applicator->crop_node,
                             "x",      rect->x,
                             "y",      rect->y,
                             "width",  rect->width,
                             "height", rect->height,
                             NULL);
            }

          applicator->crop_enabled = TRUE;
          applicator->crop_rect    = *rect;
        }
      else
        {
          gegl_node_disconnect (applicator->crop_node, "aux");
          gegl_node_set (applicator->crop_node,
                         "operation", "gegl:nop",
                         NULL);

          applicator->crop_enabled = FALSE;
        }
    }
}

const GeglRectangle *
clay_applicator_get_crop (ClayApplicator *applicator)
{
  g_return_val_if_fail (CLAY_IS_APPLICATOR (applicator), NULL);

  if (applicator->crop_enabled)
    return &applicator->crop_rect;

  return NULL;
}

void
clay_applicator_blit (ClayApplicator      *applicator,
                      const GeglRectangle *rect)
{
  g_return_if_fail (CLAY_IS_APPLICATOR (applicator));

  gegl_node_blit (applicator->dest_node, 1.0, rect,
                  NULL, NULL, 0, GEGL_BLIT_DEFAULT);
}
