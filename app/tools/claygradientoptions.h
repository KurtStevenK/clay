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

#ifndef  __CLAY_GRADIENT_OPTIONS_H__
#define  __CLAY_GRADIENT_OPTIONS_H__


#include "paint/claypaintoptions.h"


#define CLAY_TYPE_GRADIENT_OPTIONS            (clay_gradient_options_get_type ())
#define CLAY_GRADIENT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_GRADIENT_OPTIONS, ClayGradientOptions))
#define CLAY_GRADIENT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_GRADIENT_OPTIONS, ClayGradientOptionsClass))
#define CLAY_IS_GRADIENT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_GRADIENT_OPTIONS))
#define CLAY_IS_GRADIENT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_GRADIENT_OPTIONS))
#define CLAY_GRADIENT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_GRADIENT_OPTIONS, ClayGradientOptionsClass))


typedef struct _ClayGradientOptions   ClayGradientOptions;
typedef struct _ClayPaintOptionsClass ClayGradientOptionsClass;

struct _ClayGradientOptions
{
  ClayPaintOptions   paint_options;

  gdouble            offset;
  ClayGradientType   gradient_type;
  GeglDistanceMetric distance_metric;

  gboolean           supersample;
  gint               supersample_depth;
  gdouble            supersample_threshold;

  gboolean           dither;

  gboolean           instant;
  gboolean           modify_active;

  gboolean           editable_gradient;

  /*  options gui  */
  GtkWidget         *instant_toggle;
  GtkWidget         *editable_gradient_toggle;
  GtkWidget         *modify_active_frame;
  GtkWidget         *modify_active_hint;
};


GType       clay_gradient_options_get_type (void);

GtkWidget * clay_gradient_options_gui      (ClayToolOptions *tool_options);


#endif  /*  __CLAY_GRADIENT_OPTIONS_H__  */
