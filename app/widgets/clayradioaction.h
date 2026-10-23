/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayradioaction.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
 * Copyright (C) 2023 Jehan
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

#include "claytoggleaction.h"


#define CLAY_TYPE_RADIO_ACTION            (clay_radio_action_get_type ())
#define CLAY_RADIO_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_RADIO_ACTION, ClayRadioAction))
#define CLAY_RADIO_ACTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_RADIO_ACTION, ClayRadioActionClass))
#define CLAY_IS_RADIO_ACTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_RADIO_ACTION))
#define CLAY_IS_RADIO_ACTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), CLAY_TYPE_RADIO_ACTION))
#define CLAY_RADIO_ACTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), CLAY_TYPE_RADIO_ACTION, ClayRadioActionClass))


typedef struct _ClayRadioActionClass   ClayRadioActionClass;
typedef struct _ClayRadioActionPrivate ClayRadioActionPrivate;

struct _ClayRadioAction
{
  ClayToggleAction        parent_instance;

  ClayRadioActionPrivate *priv;
};

struct _ClayRadioActionClass
{
  ClayToggleActionClass  parent_class;
};


GType         clay_radio_action_get_type          (void);

ClayAction  * clay_radio_action_new               (const gchar *name,
                                                   const gchar *label,
                                                   const gchar *short_label,
                                                   const gchar *tooltip,
                                                   const gchar *icon_name,
                                                   const gchar *help_id,
                                                   gint         value,
                                                   ClayContext *context);

GSList      * clay_radio_action_get_group         (ClayRadioAction *action);
void          clay_radio_action_set_group         (ClayRadioAction *action,
                                                   GSList          *group);
void          clay_radio_action_set_group_label   (ClayRadioAction *action,
                                                   const gchar     *label);
const gchar * clay_radio_action_get_group_label   (ClayRadioAction *action);

gint          clay_radio_action_get_current_value (ClayRadioAction *action);
void          clay_radio_action_set_current_value (ClayRadioAction *action,
                                                   gint             current_value);
