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

#pragma once


#define CLAY_TYPE_PAD_ACTIONS            (clay_pad_actions_get_type ())
#define CLAY_PAD_ACTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PAD_ACTIONS, ClayPadActions))
#define CLAY_PAD_ACTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PAD_ACTIONS, ClayPadActionsClass))
#define CLAY_IS_PAD_ACTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PAD_ACTIONS))
#define CLAY_IS_PAD_ACTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PAD_ACTIONS))
#define CLAY_PAD_ACTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PAD_ACTIONS, ClayPadActionsClass))


typedef struct _ClayPadActionsPrivate ClayPadActionsPrivate;
typedef struct _ClayPadActionsClass   ClayPadActionsClass;

struct _ClayPadActions
{
  GObject                parent_instance;
  ClayPadActionsPrivate *priv;
};

struct _ClayPadActionsClass
{
  GObjectClass parent_class;
};

typedef enum
{
  CLAY_PAD_ACTION_BUTTON,
  CLAY_PAD_ACTION_RING,
  CLAY_PAD_ACTION_STRIP
} ClayPadActionType;

typedef void (* ClayPadActionForeach) (ClayPadActions    *pad_actions,
                                       ClayPadActionType  action_type,
                                       guint              number,
                                       guint              mode,
                                       const gchar       *action,
                                       gpointer           data);


GType            clay_pad_actions_get_type      (void);

ClayPadActions * clay_pad_actions_new           (void);

void             clay_pad_actions_foreach       (ClayPadActions       *pad_actions,
                                                 ClayPadActionForeach  func,
                                                 gpointer              data);
gint             clay_pad_actions_set_action    (ClayPadActions       *pad_actions,
                                                 ClayPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode,
                                                 const gchar          *action_name);
void             clay_pad_actions_clear_action  (ClayPadActions       *pad_actions,
                                                 ClayPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode);
gint             clay_pad_actions_get_n_actions (ClayPadActions       *pad_actions);
const gchar    * clay_pad_actions_get_action    (ClayPadActions       *pad_actions,
                                                 ClayPadActionType     action_type,
                                                 guint                 number,
                                                 guint                 mode);
