/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayactionimpl.h
 * Copyright (C) 2004-2019 Michael Natterer <mitch@gimp.org>
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

#include "core/clayobject.h"


#define CLAY_TYPE_ACTION_IMPL            (clay_action_impl_get_type ())
#define CLAY_ACTION_IMPL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ACTION_IMPL, ClayActionImpl))
#define CLAY_ACTION_IMPL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ACTION_IMPL, ClayActionImplClass))
#define CLAY_IS_ACTION_IMPL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ACTION_IMPL))
#define CLAY_IS_ACTION_IMPL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), CLAY_TYPE_ACTION_IMPL))
#define CLAY_ACTION_IMPL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CLAY_TYPE_ACTION_IMPL, ClayActionImplClass))

typedef struct _ClayActionImplClass   ClayActionImplClass;
typedef struct _ClayActionImplPrivate ClayActionImplPrivate;

struct _ClayActionImpl
{
  ClayObject             parent_instance;

  ClayActionImplPrivate *priv;
};

struct _ClayActionImplClass
{
  ClayObjectClass   parent_class;

  /*  signals  */
  void (* change_state) (ClayActionImpl *impl,
                         GVariant       *value);
};

GType        clay_action_impl_get_type (void);

ClayAction * clay_action_impl_new      (const gchar *name,
                                        const gchar *label,
                                        const gchar *short_label,
                                        const gchar *tooltip,
                                        const gchar *icon_name,
                                        const gchar *help_id,
                                        ClayContext *context);
