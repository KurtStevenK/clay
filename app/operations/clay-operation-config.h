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


void            clay_operation_config_init_start    (Clay          *clay);
void            clay_operation_config_init_end      (Clay          *clay);
void            clay_operation_config_exit          (Clay          *clay);

gboolean        clay_operation_config_is_custom     (Clay          *clay,
                                                     const gchar   *operation);

void            clay_operation_config_register      (Clay          *clay,
                                                     const gchar   *operation,
                                                     GType          config_type);

GType           clay_operation_config_get_type      (Clay          *clay,
                                                     const gchar   *operation,
                                                     const gchar   *icon_name,
                                                     GType          parent_type);

ClayContainer * clay_operation_config_get_container (Clay          *clay,
                                                     GType          config_type,
                                                     GCompareFunc   sort_func);

void            clay_operation_config_serialize     (Clay          *clay,
                                                     ClayContainer *container,
                                                     GFile         *file);
void            clay_operation_config_deserialize   (Clay          *clay,
                                                     ClayContainer *container,
                                                     GFile         *file);

void            clay_operation_config_sync_node     (GObject       *config,
                                                     GeglNode      *node);
void            clay_operation_config_connect_node  (GObject       *config,
                                                     GeglNode      *node);

GParamSpec ** clay_operation_config_list_properties (GObject      *config,
                                                     GType         owner_type,
                                                     GParamFlags   flags,
                                                     guint        *n_pspecs);
