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

#include "libclaywidgets/claywidgets.h"

#define CLAY_ENABLE_CONTROLLER_UNDER_CONSTRUCTION
#include "libclaywidgets/claycontroller.h"

#include "widgets-types.h"

#include "claycontrollertype.h"


/**
 * ClayControllerType:
 *
 * A helper object to list types/categories of controllers
 */

struct _ClayControllerType
{
  ClayViewable parent_instance;

  GType        gtype;
};

G_DEFINE_TYPE (ClayControllerType, clay_controller_type,
               CLAY_TYPE_VIEWABLE)


static void
clay_controller_type_init (ClayControllerType *self)
{
}

static void
clay_controller_type_class_init (ClayControllerTypeClass *klass)
{
}

ClayControllerType *
clay_controller_type_new (GType gtype)
{
  ClayControllerClass *controller_class;
  ClayControllerType  *type;

  g_return_val_if_fail (g_type_is_a (gtype, CLAY_TYPE_CONTROLLER), NULL);

  controller_class = g_type_class_ref (gtype);

  type = g_object_new (CLAY_TYPE_CONTROLLER_TYPE,
                       "name",      controller_class->name,
                       "icon-name", controller_class->icon_name,
                       NULL);

  type->gtype = gtype;

  g_type_class_unref (controller_class);

  return type;
}

GType
clay_controller_type_get_gtype (ClayControllerType *self)
{
  g_return_val_if_fail (CLAY_IS_CONTROLLER_TYPE (self), G_TYPE_INVALID);

  return self->gtype;
}

GListModel *
clay_controller_type_get_model (void)
{
  GListStore *types;
  GType      *controller_types;
  guint       n_controller_types;
  gint        i;

  types = g_list_store_new (CLAY_TYPE_CONTROLLER_TYPE);

  controller_types = g_type_children (CLAY_TYPE_CONTROLLER,
                                      &n_controller_types);

  for (i = 0; i < n_controller_types; i++)
    {
      ClayControllerType *type;

      type = clay_controller_type_new (controller_types[i]);
      g_list_store_append (G_LIST_STORE (types), type);
      g_object_unref (type);
    }

  g_free (controller_types);

  g_list_store_sort (types, (GCompareDataFunc) clay_object_name_collate,
                     NULL);

  return G_LIST_MODEL (types);
}
