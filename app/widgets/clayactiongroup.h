/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayactiongroup.h
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

/*  leave this here  */
#ifndef __CLAY_ACTION_GROUP_H__
#define __CLAY_ACTION_GROUP_H__
#endif

#include "core/clayobject.h"

#define CLAY_TYPE_ACTION_GROUP              (clay_action_group_get_type ())
#define CLAY_ACTION_GROUP(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ACTION_GROUP, ClayActionGroup))
#define CLAY_ACTION_GROUP_CLASS(vtable)     (G_TYPE_CHECK_CLASS_CAST ((vtable), CLAY_TYPE_ACTION_GROUP, ClayActionGroupClass))
#define CLAY_IS_ACTION_GROUP(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ACTION_GROUP))
#define CLAY_IS_ACTION_GROUP_CLASS(vtable)  (G_TYPE_CHECK_CLASS_TYPE ((vtable), CLAY_TYPE_ACTION_GROUP))
#define CLAY_ACTION_GROUP_GET_CLASS(inst)   (G_TYPE_INSTANCE_GET_CLASS ((inst), CLAY_TYPE_ACTION_GROUP, ClayActionGroupClass))


typedef struct _ClayActionGroupClass ClayActionGroupClass;

struct _ClayActionGroup
{
  ClayObject                 parent_instance;

  Clay                      *clay;
  gchar                     *label;
  gchar                     *icon_name;

  gpointer                   user_data;

  ClayActionGroupUpdateFunc  update_func;

  GList                     *actions;
};

struct _ClayActionGroupClass
{
  ClayObjectClass      parent_class;

  GHashTable          *groups;
};


typedef void (* ClayActionCallback) (ClayAction *action,
                                     GVariant   *value,
                                     gpointer    data);

struct _ClayActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *short_label;
  const gchar        *accelerator[4];
  const gchar        *tooltip;
  ClayActionCallback  callback;

  const gchar        *help_id;
};

struct _ClayToggleActionEntry
{
  const gchar        *name;
  const gchar        *icon_name;
  const gchar        *label;
  const gchar        *short_label;
  const gchar        *accelerator[4];
  const gchar        *tooltip;
  ClayActionCallback  callback;
  gboolean            is_active;

  const gchar        *help_id;
};

struct _ClayRadioActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  gint         value;

  const gchar *help_id;
};

struct _ClayEnumActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  gint         value;
  gboolean     value_variable;

  const gchar *help_id;
};

struct _ClayStringActionEntry
{
  const gchar *name;
  const gchar *icon_name;
  const gchar *label;
  const gchar *short_label;
  const gchar *accelerator[4];
  const gchar *tooltip;
  const gchar *value;

  const gchar *help_id;
};

struct _ClayDoubleActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *short_label;
  const gchar   *accelerator[4];
  const gchar   *tooltip;
  const gdouble  value;

  const gchar   *help_id;
};

struct _ClayProcedureActionEntry
{
  const gchar   *name;
  const gchar   *icon_name;
  const gchar   *label;
  const gchar   *accelerator;
  const gchar   *tooltip;
  ClayProcedure *procedure;

  const gchar   *help_id;
};


GType            clay_action_group_get_type   (void);

ClayActionGroup *clay_action_group_new        (Clay                  *clay,
                                               const gchar           *name,
                                               const gchar           *label,
                                               const gchar           *icon_name,
                                               gpointer               user_data,
                                               ClayActionGroupUpdateFunc update_func);

GList *clay_action_groups_from_name           (const gchar           *name);

const gchar * clay_action_group_get_name       (ClayActionGroup       *group);

void  clay_action_group_add_action            (ClayActionGroup *action_group,
                                               ClayAction      *action);
void  clay_action_group_add_action_with_accel (ClayActionGroup *action_group,
                                               ClayAction      *action,
                                               const gchar * const*accelerators);
void  clay_action_group_remove_action         (ClayActionGroup *action_group,
                                               ClayAction      *action);

ClayAction  * clay_action_group_get_action     (ClayActionGroup       *group,
                                                const gchar           *action_name);
GList       * clay_action_group_list_actions   (ClayActionGroup       *group);

void   clay_action_group_update               (ClayActionGroup       *group,
                                               gpointer               update_data);

void   clay_action_group_add_actions          (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayActionEntry       *entries,
                                               guint                        n_entries);
void   clay_action_group_add_toggle_actions   (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayToggleActionEntry *entries,
                                               guint                        n_entries);
GSList *clay_action_group_add_radio_actions   (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayRadioActionEntry  *entries,
                                               guint                        n_entries,
                                               GSList                      *radio_group,
                                               gint                         value,
                                               ClayActionCallback           callback);
void   clay_action_group_add_enum_actions     (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayEnumActionEntry   *entries,
                                               guint                        n_entries,
                                               ClayActionCallback           callback);
void   clay_action_group_add_string_actions   (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayStringActionEntry *entries,
                                               guint                        n_entries,
                                               ClayActionCallback           callback);
void   clay_action_group_add_double_actions   (ClayActionGroup             *group,
                                               const gchar                 *msg_context,
                                               const ClayDoubleActionEntry *entries,
                                               guint                        n_entries,
                                               ClayActionCallback           callback);
void   clay_action_group_add_procedure_actions(ClayActionGroup             *group,
                                               const ClayProcedureActionEntry *entries,
                                               guint                        n_entries,
                                               ClayActionCallback           callback);

void   clay_action_group_remove_action_and_accel (ClayActionGroup          *group,
                                                  ClayAction               *action);

void          clay_action_group_set_action_visible    (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         visible);
void          clay_action_group_set_action_sensitive  (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         sensitive,
                                                       const gchar     *reason);
void          clay_action_group_set_action_active     (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         active);
void          clay_action_group_set_action_label      (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *label);
void          clay_action_group_set_action_pixbuf     (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       GdkPixbuf       *pixbuf);
void          clay_action_group_set_action_tooltip    (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       const gchar     *tooltip);
const gchar * clay_action_group_get_action_tooltip    (ClayActionGroup *group,
                                                       const gchar     *action_name);
void          clay_action_group_set_action_color      (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       GeglColor       *color,
                                                       gboolean         set_label);
void          clay_action_group_set_action_viewable   (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       ClayViewable    *viewable);
void          clay_action_group_set_action_hide_empty (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         hide_empty);
void   clay_action_group_set_action_always_show_image (ClayActionGroup *group,
                                                       const gchar     *action_name,
                                                       gboolean         always_show_image);
