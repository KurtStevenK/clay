/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewrendererlayer.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaywidgets/claywidgets.h"

#include "widgets-types.h"

#include "config/claycoreconfig.h"

#include "core/clay.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"
#include "core/claylinklayer.h"

#include "text/claytextlayer.h"

#include "clayviewrendererlayer.h"


static void   clay_view_renderer_layer_render (ClayViewRenderer *renderer,
                                               GtkWidget        *widget);


G_DEFINE_TYPE (ClayViewRendererLayer,
               clay_view_renderer_layer,
               CLAY_TYPE_VIEW_RENDERER_DRAWABLE)

#define parent_class clay_view_renderer_layer_parent_class


static void
clay_view_renderer_layer_class_init (ClayViewRendererLayerClass *klass)
{
  ClayViewRendererClass *renderer_class = CLAY_VIEW_RENDERER_CLASS (klass);

  renderer_class->render = clay_view_renderer_layer_render;
}

static void
clay_view_renderer_layer_init (ClayViewRendererLayer *renderer)
{
}

static void
clay_view_renderer_layer_render (ClayViewRenderer *renderer,
                                 GtkWidget        *widget)
{
  const gchar *icon_name = NULL;

  if (clay_layer_is_floating_sel (CLAY_LAYER (renderer->viewable)))
    {
      icon_name = CLAY_ICON_LAYER_FLOATING_SELECTION;
    }
  else if (clay_item_is_text_layer (CLAY_ITEM (renderer->viewable)) ||
           (CLAY_IS_LINK_LAYER (renderer->viewable) &&
            clay_link_layer_is_monitored (CLAY_LINK_LAYER (renderer->viewable))))
    {
      icon_name = clay_viewable_get_icon_name (renderer->viewable);
    }
  else
    {
      ClayContainer *children = clay_viewable_get_children (renderer->viewable);

      if (children)
        {
          ClayItem  *item  = CLAY_ITEM (renderer->viewable);
          ClayImage *image = clay_item_get_image (item);

          if (clay_container_get_n_children (children) == 0)
            icon_name = "folder";
          else if (image && ! image->clay->config->group_layer_previews)
            icon_name = clay_viewable_get_icon_name (renderer->viewable);
        }
    }

  if (icon_name)
    clay_view_renderer_render_icon (renderer, widget,
                                    icon_name,
                                    gtk_widget_get_scale_factor (widget));
  else
    CLAY_VIEW_RENDERER_CLASS (parent_class)->render (renderer, widget);
}
