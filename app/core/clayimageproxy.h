/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayimageproxy.h
 * Copyright (C) 2019 Ell
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

#include "clayviewable.h"


#define CLAY_TYPE_IMAGE_PROXY            (clay_image_proxy_get_type ())
#define CLAY_IMAGE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_IMAGE_PROXY, ClayImageProxy))
#define CLAY_IMAGE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_IMAGE_PROXY, ClayImageProxyClass))
#define CLAY_IS_IMAGE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_IMAGE_PROXY))
#define CLAY_IS_IMAGE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_IMAGE_PROXY))
#define CLAY_IMAGE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_IMAGE_PROXY, ClayImageProxyClass))


typedef struct _ClayImageProxyPrivate ClayImageProxyPrivate;
typedef struct _ClayImageProxyClass   ClayImageProxyClass;

struct _ClayImageProxy
{
  ClayViewable           parent_instance;

  ClayImageProxyPrivate *priv;
};

struct _ClayImageProxyClass
{
  ClayViewableClass  parent_class;
};


GType            clay_image_proxy_get_type         (void);

ClayImageProxy * clay_image_proxy_new              (ClayImage      *image);

ClayImage      * clay_image_proxy_get_image        (ClayImageProxy *image_proxy);

void             clay_image_proxy_set_show_all     (ClayImageProxy *image_proxy,
                                                    gboolean        show_all);
gboolean         clay_image_proxy_get_show_all     (ClayImageProxy *image_proxy);

GeglRectangle    clay_image_proxy_get_bounding_box (ClayImageProxy *image_proxy);
