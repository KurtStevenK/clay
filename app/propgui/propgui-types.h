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

#include "display/display-enums.h"

#include "widgets/widgets-types.h"


/*  enums, move to propgui-enums.h if we get more  */

typedef enum
{
  CLAY_CONTROLLER_TYPE_LINE,
  CLAY_CONTROLLER_TYPE_SLIDER_LINE,
  CLAY_CONTROLLER_TYPE_TRANSFORM_GRID,
  CLAY_CONTROLLER_TYPE_TRANSFORM_GRIDS,
  CLAY_CONTROLLER_TYPE_GYROSCOPE,
  CLAY_CONTROLLER_TYPE_FOCUS
} ClayControllerType;


/*  structs  */

typedef struct
{
  gdouble        value;           /*  slider value                           */
  gdouble        min;             /*  minimal allowable slider value         */
  gdouble        max;             /*  maximal allowable slider value         */

  guint          visible    : 1;  /*  slider is visible                      */
  guint          selectable : 1;  /*  slider is selectable                   */
  guint          movable    : 1;  /*  slider movable                         */
  guint          removable  : 1;  /*  slider is removable                    */

  guint          autohide   : 1;  /*  whether to autohide the slider         */
  ClayHandleType type;            /*  slider handle type                     */
  gdouble        size;            /*  slider handle size, as a fraction of   *
                                   *  the default size                       */

  gpointer       data;            /*  user data                              */
} ClayControllerSlider;

#define CLAY_CONTROLLER_SLIDER_DEFAULT                                         \
  ((const ClayControllerSlider) {                                              \
    .value      = 0.0,                                                         \
    .min        = 0.0,                                                         \
    .max        = 1.0,                                                         \
                                                                               \
    .visible    = TRUE,                                                        \
    .selectable = TRUE,                                                        \
    .movable    = TRUE,                                                        \
    .removable  = FALSE,                                                       \
                                                                               \
    .autohide   = FALSE,                                                       \
    .type       = CLAY_HANDLE_FILLED_DIAMOND,                                  \
    .size       = 1.0,                                                         \
                                                                               \
    .data       = NULL                                                         \
  })


/*  function types  */

typedef void (* ClayPickerCallback)                   (gpointer                    data,
                                                       gpointer                    identifier,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       const Babl                 *sample_format,
                                                       GeglColor                  *color);

typedef void (* ClayControllerLineCallback)           (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2);
typedef void (* ClayControllerSliderLineCallback)     (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     x1,
                                                       gdouble                     y1,
                                                       gdouble                     x2,
                                                       gdouble                     y2,
                                                       const ClayControllerSlider *sliders,
                                                       gint                        n_sliders);
typedef void (* ClayControllerTransformGridCallback)  (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const ClayMatrix3          *transform);
typedef void (* ClayControllerTransformGridsCallback) (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       const ClayMatrix3          *transforms,
                                                       gint                        n_transforms);
typedef void (* ClayControllerGyroscopeCallback)      (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       gdouble                     yaw,
                                                       gdouble                     pitch,
                                                       gdouble                     roll,
                                                       gdouble                     zoom,
                                                       gboolean                    invert);
typedef void (* ClayControllerFocusCallback)          (gpointer                    data,
                                                       GeglRectangle              *area,
                                                       ClayLimitType               type,
                                                       gdouble                     x,
                                                       gdouble                     y,
                                                       gdouble                     radius,
                                                       gdouble                     aspect_ratio,
                                                       gdouble                     angle,
                                                       gdouble                     inner_limit,
                                                       gdouble                     midpoint);


typedef GtkWidget * (* ClayCreatePickerFunc)          (gpointer                    creator,
                                                       const gchar                *property_name,
                                                       const gchar                *icon_name,
                                                       const gchar                *tooltip,
                                                       gboolean                    pick_abyss,
                                                       ClayPickerCallback          callback,
                                                       gpointer                    callback_data);

typedef GCallback   (* ClayCreateControllerFunc)      (gpointer                    creator,
                                                       ClayControllerType          controller_type,
                                                       const gchar                *status_title,
                                                       GCallback                   callback,
                                                       gpointer                    callback_data,
                                                       gpointer                   *set_func_data);

