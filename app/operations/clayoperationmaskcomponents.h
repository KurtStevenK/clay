/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationmaskcomponents.h
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

#include <gegl-plugin.h>


#define CLAY_TYPE_OPERATION_MASK_COMPONENTS            (clay_operation_mask_components_get_type ())
#define CLAY_OPERATION_MASK_COMPONENTS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_MASK_COMPONENTS, ClayOperationMaskComponents))
#define CLAY_OPERATION_MASK_COMPONENTS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_MASK_COMPONENTS, ClayOperationMaskComponentsClass))
#define CLAY_IS_OPERATION_MASK_COMPONENTS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_MASK_COMPONENTS))
#define CLAY_IS_OPERATION_MASK_COMPONENTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_MASK_COMPONENTS))
#define CLAY_OPERATION_MASK_COMPONENTS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_MASK_COMPONENTS, ClayOperationMaskComponentsClass))


typedef struct _ClayOperationMaskComponents      ClayOperationMaskComponents;
typedef struct _ClayOperationMaskComponentsClass ClayOperationMaskComponentsClass;

struct _ClayOperationMaskComponents
{
  GeglOperationPointComposer  parent_instance;

  ClayComponentMask           mask;
  gdouble                     alpha;

  guint32                     alpha_value;
  gpointer                    process;
  const Babl                 *format;
};

struct _ClayOperationMaskComponentsClass
{
  GeglOperationPointComposerClass  parent_class;
};


GType        clay_operation_mask_components_get_type   (void);

const Babl * clay_operation_mask_components_get_format (const Babl        *input_format);

void         clay_operation_mask_components_process    (const Babl        *format,
                                                        gconstpointer      in,
                                                        gconstpointer      aux,
                                                        gpointer           out,
                                                        gint               n,
                                                        ClayComponentMask  mask);
