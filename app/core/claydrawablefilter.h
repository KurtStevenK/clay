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

#include "clayfilter.h"


#define CLAY_TYPE_DRAWABLE_FILTER            (clay_drawable_filter_get_type ())
#define CLAY_DRAWABLE_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_FILTER, ClayDrawableFilter))
#define CLAY_DRAWABLE_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE_FILTER, ClayDrawableFilterClass))
#define CLAY_IS_DRAWABLE_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_FILTER))
#define CLAY_IS_DRAWABLE_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE_FILTER))
#define CLAY_DRAWABLE_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE_FILTER, ClayDrawableFilterClass))


typedef struct _ClayDrawableFilterClass  ClayDrawableFilterClass;

struct _ClayDrawableFilterClass
{
  ClayFilterClass  parent_class;

  void (* flush) (ClayDrawableFilter *filter);
};


/*  Drawable Filter functions  */

/*  Successive apply() functions can be called, but eventually MUST be
 *  followed with an commit() or an abort() call, both of which will
 *  remove the live filter from the drawable.
 */

GType      clay_drawable_filter_get_type       (void);

ClayDrawableFilter *
           clay_drawable_filter_new            (ClayDrawable            *drawable,
                                                const gchar             *undo_desc,
                                                GeglNode                *operation,
                                                const gchar             *icon_name);
ClayDrawableFilter *
           clay_drawable_filter_duplicate      (ClayDrawable            *drawable,
                                                ClayDrawableFilter      *prior_filter);

gint       clay_drawable_filter_get_id         (ClayDrawableFilter      *item);
ClayDrawableFilter *
           clay_drawable_filter_get_by_id      (Clay                    *clay,
                                                gint                     filter_id);

ClayDrawable *
           clay_drawable_filter_get_drawable   (ClayDrawableFilter      *filter);
GeglNode * clay_drawable_filter_get_operation  (ClayDrawableFilter      *filter);
ClayDrawableFilterMask *
           clay_drawable_filter_get_mask       (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_temporary  (ClayDrawableFilter      *filter,
                                                gboolean                 temporary);
gboolean   clay_drawable_filter_get_temporary  (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_opacity    (ClayDrawableFilter      *filter,
                                                gdouble                  opacity);
gdouble    clay_drawable_filter_get_opacity    (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_mode       (ClayDrawableFilter      *filter,
                                                ClayLayerMode            paint_mode,
                                                ClayLayerColorSpace      blend_space,
                                                ClayLayerColorSpace      composite_space,
                                                ClayLayerCompositeMode   composite_mode);
ClayLayerMode
           clay_drawable_filter_get_paint_mode (ClayDrawableFilter      *filter);
ClayLayerColorSpace
           clay_drawable_filter_get_blend_space
                                               (ClayDrawableFilter      *filter);
ClayLayerColorSpace
           clay_drawable_filter_get_composite_space
                                               (ClayDrawableFilter      *filter);
ClayLayerCompositeMode
           clay_drawable_filter_get_composite_mode
                                               (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_clip       (ClayDrawableFilter      *filter,
                                                gboolean                 clip);
gboolean   clay_drawable_filter_get_clip       (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_region     (ClayDrawableFilter      *filter,
                                                ClayFilterRegion         region);
ClayFilterRegion
           clay_drawable_filter_get_region     (ClayDrawableFilter      *filter);

void       clay_drawable_filter_set_crop       (ClayDrawableFilter      *filter,
                                                const GeglRectangle     *rect,
                                                gboolean                 update);

void       clay_drawable_filter_set_preview    (ClayDrawableFilter      *filter,
                                                gboolean                 enabled);
void       clay_drawable_filter_set_preview_split
                                               (ClayDrawableFilter      *filter,
                                                gboolean                 enabled,
                                                ClayAlignmentType        alignment,
                                                gint                     split_position);

gboolean   clay_drawable_filter_update         (ClayDrawableFilter      *filter,
                                                const gchar            **propnames,
                                                const ClayValueArray    *values,
                                                gdouble                  opacity,
                                                ClayLayerMode            paint_mode,
                                                ClayLayerColorSpace      blend_space,
                                                ClayLayerColorSpace      composite_space,
                                                ClayLayerCompositeMode   composite_mode,
                                                const gchar            **auxinputnames,
                                                ClayDrawable           **auxinputs,
                                                GError                 **error);
gboolean   clay_drawable_filter_get_add_alpha  (ClayDrawableFilter      *filter);
void       clay_drawable_filter_set_add_alpha  (ClayDrawableFilter      *filter,
                                                gboolean                 add_alpha);

void       clay_drawable_filter_set_override_constraints
                                               (ClayDrawableFilter      *filter,
                                                gboolean                 override_constraints);

const Babl *
           clay_drawable_filter_get_format     (ClayDrawableFilter      *filter);

void       clay_drawable_filter_apply          (ClayDrawableFilter      *filter,
                                                const GeglRectangle     *area);

void       clay_drawable_filter_apply_with_mask
                                               (ClayDrawableFilter      *filter,
                                                ClayChannel             *mask,
                                                const GeglRectangle     *area);

gboolean   clay_drawable_filter_commit         (ClayDrawableFilter      *filter,
                                                gboolean                 non_destructive,
                                                ClayProgress            *progress,
                                                gboolean                 cancellable);
void       clay_drawable_filter_abort          (ClayDrawableFilter      *filter);

void       clay_drawable_filter_layer_mask_freeze
                                               (ClayDrawableFilter      *filter);
void       clay_drawable_filter_refresh_crop   (ClayDrawableFilter      *filter,
                                                GeglRectangle           *rect);
