/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydrawablefiltermask.h
 * Copyright (C) 2024 Jehan
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

#include "claychannel.h"


#define CLAY_TYPE_DRAWABLE_FILTER_MASK            (clay_drawable_filter_mask_get_type ())
#define CLAY_DRAWABLE_FILTER_MASK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DRAWABLE_FILTER_MASK, ClayDrawableFilterMask))
#define CLAY_DRAWABLE_FILTER_MASK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DRAWABLE_FILTER_MASK, ClayDrawableFilterMaskClass))
#define CLAY_IS_DRAWABLE_FILTER_MASK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DRAWABLE_FILTER_MASK))
#define CLAY_IS_DRAWABLE_FILTER_MASK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DRAWABLE_FILTER_MASK))
#define CLAY_DRAWABLE_FILTER_MASK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DRAWABLE_FILTER_MASK, ClayDrawableFilterMaskClass))


typedef struct _ClayDrawableFilterMaskClass ClayDrawableFilterMaskClass;

struct _ClayDrawableFilterMask
{
  ClayChannel         parent_instance;

  ClayDrawableFilter *filter;
};

struct _ClayDrawableFilterMaskClass
{
  ClayChannelClass parent_class;
};


GType                    clay_drawable_filter_mask_get_type  (void);

ClayDrawableFilterMask * clay_drawable_filter_mask_new        (ClayImage              *image,
                                                               gint                    width,
                                                               gint                    height);
void                     clay_drawable_filter_mask_set_filter (ClayDrawableFilterMask *mask,
                                                               ClayDrawableFilter     *filter);
ClayDrawableFilter     * clay_drawable_filter_mask_get_filter (ClayDrawableFilterMask *mask);
