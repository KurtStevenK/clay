/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayapplicator.h
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

#pragma once


#define CLAY_TYPE_APPLICATOR            (clay_applicator_get_type ())
#define CLAY_APPLICATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_APPLICATOR, ClayApplicator))
#define CLAY_APPLICATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_APPLICATOR, ClayApplicatorClass))
#define CLAY_IS_APPLICATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_APPLICATOR))
#define CLAY_IS_APPLICATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_APPLICATOR))
#define CLAY_APPLICATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_APPLICATOR, ClayApplicatorClass))


typedef struct _ClayApplicatorClass ClayApplicatorClass;

struct _ClayApplicator
{
  GObject                 parent_instance;

  GeglNode               *node;
  GeglNode               *input_node;
  GeglNode               *aux_node;
  GeglNode               *output_node;

  gboolean                active;

  GeglBuffer             *apply_buffer;
  GeglNode               *apply_src_node;

  gint                    apply_offset_x;
  gint                    apply_offset_y;
  GeglNode               *apply_offset_node;

  gdouble                 opacity;
  ClayLayerMode           paint_mode;
  ClayLayerColorSpace     blend_space;
  ClayLayerColorSpace     composite_space;
  ClayLayerCompositeMode  composite_mode;
  GeglNode               *mode_node;

  ClayComponentMask       affect;
  GeglNode               *affect_node;

  const Babl             *output_format;
  GeglNode               *convert_format_node;

  gboolean                cache_enabled;
  GeglNode               *cache_node;

  gboolean                crop_enabled;
  GeglRectangle           crop_rect;
  GeglNode               *crop_node;

  GeglBuffer             *src_buffer;
  GeglNode               *src_node;

  GeglBuffer             *dest_buffer;
  GeglNode               *dest_node;

  GeglBuffer             *mask_buffer;
  GeglNode               *mask_node;

  gint                    mask_offset_x;
  gint                    mask_offset_y;
  GeglNode               *mask_offset_node;
};

struct _ClayApplicatorClass
{
  GObjectClass  parent_class;
};


GType        clay_applicator_get_type          (void);

ClayApplicator * clay_applicator_new           (GeglNode             *parent);

void         clay_applicator_set_active        (ClayApplicator       *applicator,
                                                gboolean              active);

void         clay_applicator_set_src_buffer    (ClayApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);
void         clay_applicator_set_dest_buffer   (ClayApplicator       *applicator,
                                                GeglBuffer           *dest_buffer);

void         clay_applicator_set_mask_buffer   (ClayApplicator       *applicator,
                                                GeglBuffer           *mask_buffer);
void         clay_applicator_set_mask_offset   (ClayApplicator       *applicator,
                                                gint                  mask_offset_x,
                                                gint                  mask_offset_y);

void         clay_applicator_set_apply_buffer  (ClayApplicator       *applicator,
                                                GeglBuffer           *apply_buffer);
void         clay_applicator_set_apply_offset  (ClayApplicator       *applicator,
                                                gint                  apply_offset_x,
                                                gint                  apply_offset_y);

void         clay_applicator_set_opacity       (ClayApplicator       *applicator,
                                                gdouble               opacity);
void         clay_applicator_set_mode          (ClayApplicator       *applicator,
                                                ClayLayerMode         paint_mode,
                                                ClayLayerColorSpace   blend_space,
                                                ClayLayerColorSpace   composite_space,
                                                ClayLayerCompositeMode composite_mode);
void         clay_applicator_set_affect        (ClayApplicator       *applicator,
                                                ClayComponentMask     affect);

gboolean     clay_applicator_set_output_format (ClayApplicator       *applicator,
                                                const Babl           *format);
const Babl * clay_applicator_get_output_format (ClayApplicator       *applicator);

void         clay_applicator_set_cache         (ClayApplicator       *applicator,
                                                gboolean              enable);
gboolean     clay_applicator_get_cache         (ClayApplicator       *applicator);
GeglBuffer * clay_applicator_get_cache_buffer  (ClayApplicator       *applicator,
                                                GeglRectangle       **rectangles,
                                                gint                 *n_rectangles);

void         clay_applicator_set_crop          (ClayApplicator       *applicator,
                                                const GeglRectangle  *rect);
const GeglRectangle * clay_applicator_get_crop (ClayApplicator       *applicator);

void         clay_applicator_blit              (ClayApplicator       *applicator,
                                                const GeglRectangle  *rect);
