/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claysessioninfo.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_SESSION_INFO            (clay_session_info_get_type ())
#define CLAY_SESSION_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_SESSION_INFO, ClaySessionInfo))
#define CLAY_SESSION_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_SESSION_INFO, ClaySessionInfoClass))
#define CLAY_IS_SESSION_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_SESSION_INFO))
#define CLAY_IS_SESSION_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_SESSION_INFO))
#define CLAY_SESSION_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_SESSION_INFO, ClaySessionInfoClass))


typedef struct _ClaySessionInfoPrivate  ClaySessionInfoPrivate;
typedef struct _ClaySessionInfoClass    ClaySessionInfoClass;

/**
 * ClaySessionInfo:
 *
 * Contains session info for one toplevel window in the interface such
 * as a dock, the empty-image-window, or the open/save dialog.
 */
struct _ClaySessionInfo
{
  ClayObject  parent_instance;

  ClaySessionInfoPrivate *p;
};

struct _ClaySessionInfoClass
{
  ClayObjectClass  parent_class;
};


GType             clay_session_info_get_type                (void);

ClaySessionInfo * clay_session_info_new                     (void);

void              clay_session_info_restore                 (ClaySessionInfo        *info,
                                                             ClayDialogFactory      *factory,
                                                             GdkMonitor             *monitor);
void              clay_session_info_apply_geometry          (ClaySessionInfo        *info,
                                                             GdkMonitor             *current_monitor,
                                                             gboolean                apply_stored_monitor);
void              clay_session_info_read_geometry           (ClaySessionInfo        *info,
                                                             GdkEventConfigure      *cevent);
void              clay_session_info_get_info                (ClaySessionInfo        *info);
void              clay_session_info_get_info_with_widget    (ClaySessionInfo        *info,
                                                             GtkWidget              *widget);
void              clay_session_info_clear_info              (ClaySessionInfo        *info);
gboolean          clay_session_info_is_singleton            (ClaySessionInfo        *info);
gboolean          clay_session_info_is_session_managed      (ClaySessionInfo        *info);
gboolean          clay_session_info_get_remember_size       (ClaySessionInfo        *info);
gboolean          clay_session_info_get_remember_if_open    (ClaySessionInfo        *info);
GtkWidget       * clay_session_info_get_widget              (ClaySessionInfo        *info);
void              clay_session_info_set_widget              (ClaySessionInfo        *info,
                                                             GtkWidget              *widget);
ClayDialogFactoryEntry *
                  clay_session_info_get_factory_entry       (ClaySessionInfo        *info);
void              clay_session_info_set_factory_entry       (ClaySessionInfo        *info,
                                                             ClayDialogFactoryEntry *entry);
gboolean          clay_session_info_get_open                (ClaySessionInfo        *info);
void              clay_session_info_append_book             (ClaySessionInfo        *info,
                                                             ClaySessionInfoBook    *book);
gint              clay_session_info_get_x                   (ClaySessionInfo        *info);
gint              clay_session_info_get_y                   (ClaySessionInfo        *info);
gint              clay_session_info_get_width               (ClaySessionInfo        *info);
gint              clay_session_info_get_height              (ClaySessionInfo        *info);

void              clay_session_info_set_position_accuracy   (gint                    accuracy);
gint              clay_session_info_apply_position_accuracy (gint                    position);
