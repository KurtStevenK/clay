/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationnormal.h
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include "clayoperationlayermode.h"


#define CLAY_TYPE_OPERATION_NORMAL            (clay_operation_normal_get_type ())
#define CLAY_OPERATION_NORMAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_NORMAL, ClayOperationNormal))
#define CLAY_OPERATION_NORMAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_NORMAL, ClayOperationNormalClass))
#define CLAY_IS_OPERATION_NORMAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_NORMAL))
#define CLAY_IS_OPERATION_NORMAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_NORMAL))
#define CLAY_OPERATION_NORMAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_NORMAL, ClayOperationNormalClass))


typedef struct _ClayOperationNormal      ClayOperationNormal;
typedef struct _ClayOperationNormalClass ClayOperationNormalClass;

struct _ClayOperationNormal
{
  ClayOperationLayerMode  parent_instance;
};

struct _ClayOperationNormalClass
{
  ClayOperationLayerModeClass  parent_class;
};


GType      clay_operation_normal_get_type     (void);


/*  protected  */

gboolean   clay_operation_normal_process      (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#if COMPILE_SSE2_INTRINISICS

gboolean   clay_operation_normal_process_sse2 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE2_INTRINISICS */

#if COMPILE_SSE4_1_INTRINISICS

gboolean   clay_operation_normal_process_sse4 (GeglOperation       *op,
                                               void                *in,
                                               void                *layer,
                                               void                *mask,
                                               void                *out,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);

#endif /* COMPILE_SSE4_1_INTRINISICS */
