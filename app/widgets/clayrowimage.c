/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayrowimage.c
 * Copyright (C) 2026 Michael Natterer <mitch@gimp.org>
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

#include "core/clayimage.h"

#include "claydnd.h"
#include "clayrowimage.h"


static void   clay_row_image_constructed   (GObject      *object);

static void   clay_row_image_set_viewable  (ClayRow      *row,
                                            ClayViewable *viewable);
static void   clay_row_image_set_view_size (ClayRow      *row);


G_DEFINE_TYPE (ClayRowImage,
               clay_row_image,
               CLAY_TYPE_ROW)

#define parent_class clay_row_image_parent_class


static void
clay_row_image_class_init (ClayRowImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClayRowClass *row_class    = CLAY_ROW_CLASS (klass);

  object_class->constructed = clay_row_image_constructed;

  row_class->set_viewable   = clay_row_image_set_viewable;
  row_class->set_view_size  = clay_row_image_set_view_size;
}

static void
clay_row_image_init (ClayRowImage *row)
{
}

static void
clay_row_image_constructed (GObject *object)
{
  GtkWidget *view;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  view = _clay_row_get_view (CLAY_ROW (object));

  if (view)
    {
      gint view_size = clay_row_get_view_size (CLAY_ROW (object), NULL);

      gtk_widget_set_size_request (view, view_size, view_size);
    }
}

static void
clay_row_image_set_viewable (ClayRow      *row,
                             ClayViewable *viewable)
{
  if (clay_row_get_viewable (row))
    {
      clay_dnd_xds_source_remove (GTK_WIDGET (row));
    }

  CLAY_ROW_CLASS (parent_class)->set_viewable (row, viewable);

  if (viewable)
    {
      clay_dnd_xds_source_add (GTK_WIDGET (row),
                               (ClayDndDragViewableFunc)
                               clay_dnd_get_drag_viewable,
                               NULL);
    }
}

static void
clay_row_image_set_view_size (ClayRow *row)
{
  GtkWidget *view;

  CLAY_ROW_CLASS (parent_class)->set_view_size (row);

  view = _clay_row_get_view (row);

  if (view)
    {
      gint view_size = clay_row_get_view_size (row, NULL);

      gtk_widget_set_size_request (view, view_size, view_size);
    }
}
