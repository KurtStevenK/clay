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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/claycontainer.h"
#include "core/claycontext.h"

#include "claycontainereditor.h"
#include "claycontainerview.h"
#include "claycontainerview-utils.h"
#include "claydockable.h"


/*  public functions  */

ClayContainerView *
clay_container_view_get_by_dockable (ClayDockable *dockable)
{
  GtkWidget *child;

  g_return_val_if_fail (CLAY_IS_DOCKABLE (dockable), NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    {
      if (CLAY_IS_CONTAINER_EDITOR (child))
        {
          return CLAY_CONTAINER_EDITOR (child)->view;
        }
      else if (CLAY_IS_CONTAINER_VIEW (child))
        {
          return CLAY_CONTAINER_VIEW (child);
        }
    }

  return NULL;
}

void
clay_container_view_remove_active (ClayContainerView *view)
{
  ClayContext   *context;
  ClayContainer *container;

  g_return_if_fail (CLAY_IS_CONTAINER_VIEW (view));

  context   = clay_container_view_get_context (view);
  container = clay_container_view_get_container (view);

  if (context && container)
    {
      GType       child_type;
      ClayObject *active;

      child_type = clay_container_get_child_type (container);

      active = clay_context_get_by_type (context, child_type);

      if (active)
        {
          ClayObject *new;

          new = clay_container_get_neighbor_of (container, active);

          if (new)
            clay_context_set_by_type (context, child_type, new);

          clay_container_remove (container, active);
        }
    }
}
