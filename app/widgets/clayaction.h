/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayaction.h
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


#define CLAY_TYPE_ACTION (clay_action_get_type ())
G_DECLARE_INTERFACE (ClayAction,
                     clay_action,
                     CLAY, ACTION,
                     ClayObject)


enum
{
  CLAY_ACTION_PROP_0,
  CLAY_ACTION_PROP_CONTEXT,
  CLAY_ACTION_PROP_SENSITIVE,
  CLAY_ACTION_PROP_VISIBLE,
  CLAY_ACTION_PROP_LABEL,
  CLAY_ACTION_PROP_SHORT_LABEL,
  CLAY_ACTION_PROP_TOOLTIP,
  CLAY_ACTION_PROP_ICON_NAME,
  CLAY_ACTION_PROP_ICON,

  CLAY_ACTION_PROP_COLOR,
  CLAY_ACTION_PROP_VIEWABLE,
  CLAY_ACTION_PROP_ELLIPSIZE,
  CLAY_ACTION_PROP_MAX_WIDTH_CHARS,

  CLAY_ACTION_PROP_LAST = CLAY_ACTION_PROP_MAX_WIDTH_CHARS,
};

struct _ClayActionInterface
{
  GTypeInterface base_interface;

  /* Signals */
  void          (* activate)           (ClayAction   *action,
                                        GVariant     *value);
  void          (* change_state)       (ClayAction   *action,
                                        GVariant     *value);
  void          (* accels_changed)     (ClayAction   *action,
                                        const gchar **accels);
};


void             clay_action_init                (ClayAction       *action);

void             clay_action_emit_activate       (ClayAction       *action,
                                                  GVariant         *value);
void             clay_action_emit_change_state   (ClayAction       *action,
                                                  GVariant         *value);

const gchar     * clay_action_get_name            (ClayAction      *action);
ClayActionGroup * clay_action_get_group           (ClayAction      *action);

void              clay_action_set_label           (ClayAction      *action,
                                                   const gchar     *label);
const gchar     * clay_action_get_label           (ClayAction      *action);

void              clay_action_set_short_label     (ClayAction      *action,
                                                   const gchar     *label);
const gchar     * clay_action_get_short_label     (ClayAction      *action);

void              clay_action_set_tooltip         (ClayAction      *action,
                                                   const gchar     *tooltip);
const gchar     * clay_action_get_tooltip         (ClayAction      *action);

void              clay_action_set_icon_name       (ClayAction      *action,
                                                   const gchar     *icon_name);
const gchar     * clay_action_get_icon_name       (ClayAction      *action);

void              clay_action_set_gicon           (ClayAction      *action,
                                                   GIcon           *icon);
GIcon           * clay_action_get_gicon           (ClayAction      *action);

void              clay_action_set_help_id         (ClayAction      *action,
                                                   const gchar     *help_id);
const gchar     * clay_action_get_help_id         (ClayAction      *action);

void              clay_action_set_visible         (ClayAction      *action,
                                                   gboolean         visible);
gboolean          clay_action_get_visible         (ClayAction      *action);
gboolean          clay_action_is_visible          (ClayAction      *action);

void              clay_action_set_sensitive       (ClayAction      *action,
                                                   gboolean         sensitive,
                                                   const gchar     *reason);
gboolean          clay_action_get_sensitive       (ClayAction      *action,
                                                   const gchar    **reason);
gboolean          clay_action_is_sensitive        (ClayAction      *action,
                                                   const gchar    **reason);

void              clay_action_set_accels          (ClayAction      *action,
                                                   const gchar    **accels);
const gchar    ** clay_action_get_default_accels  (ClayAction      *action);
const gchar    ** clay_action_get_accels          (ClayAction      *action);
gchar **          clay_action_get_display_accels  (ClayAction      *action);
gboolean          clay_action_use_default_accels  (ClayAction      *action);
gboolean          clay_action_is_default_accel    (ClayAction      *action,
                                                   const gchar     *accel);

const gchar     * clay_action_get_menu_path       (ClayAction      *action);

void              clay_action_activate            (ClayAction      *action);

gint              clay_action_name_compare        (ClayAction      *action1,
                                                   ClayAction      *action2);

gboolean          clay_action_is_gui_blacklisted  (const gchar     *action_name);
gboolean          clay_action_is_action_search_blacklisted
                                                  (const gchar     *action_name);

ClayContext     * clay_action_get_context         (ClayAction      *action);
ClayViewable    * clay_action_get_viewable        (ClayAction      *action);


/* Protected functions. */

void              clay_action_install_properties  (GObjectClass    *klass);
void              clay_action_get_property        (GObject         *object,
                                                   guint            property_id,
                                                   GValue          *value,
                                                   GParamSpec      *pspec);
void              clay_action_set_property        (GObject         *object,
                                                   guint            property_id,
                                                   const GValue    *value,
                                                   GParamSpec      *pspec);

void              clay_action_set_proxy           (ClayAction      *action,
                                                   GtkWidget       *proxy);


/* Friend functions. */

void              clay_action_set_group           (ClayAction      *action,
                                                   ClayActionGroup *group);
void              clay_action_set_default_accels  (ClayAction      *action,
                                                   const gchar    **accels);
void              clay_action_set_menu_path       (ClayAction      *action,
                                                   const gchar     *menu_path);
