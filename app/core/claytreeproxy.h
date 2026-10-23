/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytreeproxy.h
 * Copyright (C) 2020 Ell
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

#include "claylist.h"


#define CLAY_TYPE_TREE_PROXY            (clay_tree_proxy_get_type ())
#define CLAY_TREE_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TREE_PROXY, ClayTreeProxy))
#define CLAY_TREE_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TREE_PROXY, ClayTreeProxyClass))
#define CLAY_IS_TREE_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TREE_PROXY))
#define CLAY_IS_TREE_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TREE_PROXY))
#define CLAY_TREE_PROXY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TREE_PROXY, ClayTreeProxyClass))


typedef struct _ClayTreeProxyPrivate ClayTreeProxyPrivate;
typedef struct _ClayTreeProxyClass   ClayTreeProxyClass;

struct _ClayTreeProxy
{
  ClayList              parent_instance;

  ClayTreeProxyPrivate *priv;
};

struct _ClayTreeProxyClass
{
  ClayListClass  parent_class;
};


GType           clay_tree_proxy_get_type (void);

ClayContainer * clay_tree_proxy_new               (GType          child_type);
ClayContainer * clay_tree_proxy_new_for_container (ClayContainer *container);

void            clay_tree_proxy_set_container     (ClayTreeProxy *tree_proxy,
                                                   ClayContainer *container);
ClayContainer * clay_tree_proxy_get_container     (ClayTreeProxy *tree_proxy);

void            clay_tree_proxy_set_flat          (ClayTreeProxy *tree_proxy,
                                                   gboolean       flat);
gboolean        clay_tree_proxy_get_flat          (ClayTreeProxy *tree_proxy);
