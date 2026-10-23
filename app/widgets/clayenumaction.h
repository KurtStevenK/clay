/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayenumaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "clayactionimpl.h"


#define CLAY_TYPE_ENUM_ACTION            (clay_enum_action_get_type ())
#define CLAY_ENUM_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ENUM_ACTION, ClayEnumAction))
#define CLAY_ENUM_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ENUM_ACTION, ClayEnumActionClass))
#define CLAY_IS_ENUM_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ENUM_ACTION))
#define CLAY_IS_ENUM_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), CLAY_TYPE_ENUM_ACTION))
#define CLAY_ENUM_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CLAY_TYPE_ENUM_ACTION, ClayEnumActionClass))


typedef struct _ClayEnumActionClass ClayEnumActionClass;

struct _ClayEnumAction
{
  ClayActionImpl parent_instance;

  gint           value;
  gboolean       value_variable;
};

struct _ClayEnumActionClass
{
  ClayActionImplClass parent_class;
};


GType            clay_enum_action_get_type (void);

ClayEnumAction * clay_enum_action_new      (const gchar *name,
                                            const gchar *label,
                                            const gchar *short_label,
                                            const gchar *tooltip,
                                            const gchar *icon_name,
                                            const gchar *help_id,
                                            gint         value,
                                            gboolean     value_variable,
                                            ClayContext *context);
