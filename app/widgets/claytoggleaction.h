/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytoggleaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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


#define CLAY_TYPE_TOGGLE_ACTION            (clay_toggle_action_get_type ())
#define CLAY_TOGGLE_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TOGGLE_ACTION, ClayToggleAction))
#define CLAY_TOGGLE_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TOGGLE_ACTION, ClayToggleActionClass))
#define CLAY_IS_TOGGLE_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TOGGLE_ACTION))
#define CLAY_IS_TOGGLE_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), CLAY_TYPE_TOGGLE_ACTION))
#define CLAY_TOGGLE_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CLAY_TYPE_TOGGLE_ACTION, ClayToggleActionClass))


typedef struct _ClayToggleActionClass   ClayToggleActionClass;
typedef struct _ClayToggleActionPrivate ClayToggleActionPrivate;

struct _ClayToggleAction
{
  ClayActionImpl           parent_instance;

  ClayToggleActionPrivate *priv;
};

struct _ClayToggleActionClass
{
  ClayActionImplClass  parent_class;

  /* Signals */

  void     (* toggled) (ClayToggleAction *action);

  /* Private methods */

  gboolean (* toggle)  (ClayToggleAction *action);
};


GType             clay_toggle_action_get_type  (void);

ClayAction      * clay_toggle_action_new       (const gchar *name,
                                                const gchar *label,
                                                const gchar *short_label,
                                                const gchar *tooltip,
                                                const gchar *icon_name,
                                                const gchar *help_id,
                                                ClayContext *context);

void              clay_toggle_action_set_active (ClayToggleAction *action,
                                                 gboolean          active);
gboolean          clay_toggle_action_get_active (ClayToggleAction *action);


/* Protected functions */

void             _clay_toggle_action_set_active (ClayToggleAction *action,
                                                 gboolean          active);
