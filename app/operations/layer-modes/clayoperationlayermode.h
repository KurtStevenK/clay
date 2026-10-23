/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoperationlayermode.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_OPERATION_LAYER_MODE            (clay_operation_layer_mode_get_type ())
#define CLAY_OPERATION_LAYER_MODE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OPERATION_LAYER_MODE, ClayOperationLayerMode))
#define CLAY_OPERATION_LAYER_MODE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_OPERATION_LAYER_MODE, ClayOperationLayerModeClass))
#define CLAY_IS_OPERATION_LAYER_MODE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OPERATION_LAYER_MODE))
#define CLAY_IS_OPERATION_LAYER_MODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_OPERATION_LAYER_MODE))
#define CLAY_OPERATION_LAYER_MODE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_OPERATION_LAYER_MODE, ClayOperationLayerModeClass))


typedef struct _ClayOperationLayerModeClass ClayOperationLayerModeClass;

struct _ClayOperationLayerMode
{
  GeglOperationPointComposer3  parent_instance;

  ClayLayerMode                layer_mode;
  gdouble                      opacity;
  ClayLayerColorSpace          blend_space;
  ClayLayerColorSpace          composite_space;
  ClayLayerCompositeMode       composite_mode;
  const Babl                  *cached_fish_format;
  const Babl                  *space_fish[4 /* from */][4 /* to */];
  GRWLock                      cache_lock;

  gdouble                      prop_opacity;
  ClayLayerCompositeMode       prop_composite_mode;

  ClayLayerModeFunc            function;
  ClayLayerModeBlendFunc       blend_function;
  gboolean                     is_last_node;
  gboolean                     has_mask;
};

struct _ClayOperationLayerModeClass
{
  GeglOperationPointComposer3Class  parent_class;

  /*  virtual functions  */
  gboolean                 (* parent_process)      (GeglOperation          *operation,
                                                    GeglOperationContext   *context,
                                                    const gchar            *output_prop,
                                                    const GeglRectangle    *result,
                                                    gint                    level);
  gboolean                 (* process)             (GeglOperation          *operation,
                                                    void                   *in,
                                                    void                   *aux,
                                                    void                   *mask,
                                                    void                   *out,
                                                    glong                   samples,
                                                    const GeglRectangle    *roi,
                                                    gint                    level);

  /* Returns the composite region (any combination of the layer and the
   * backdrop) that the layer mode affects.  Most modes only affect the
   * overlapping region, and don't need to override this function.
   */
  ClayLayerCompositeRegion (* get_affected_region) (ClayOperationLayerMode *layer_mode);
};


GType                    clay_operation_layer_mode_get_type            (void);

ClayLayerCompositeRegion clay_operation_layer_mode_get_affected_region (ClayOperationLayerMode *layer_mode);
