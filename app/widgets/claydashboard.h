/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydashboard.h
 * Copyright (C) 2017 Ell
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

#include "clayeditor.h"


struct _ClayDashboardLogParams
{
  gint     sample_frequency;
  gboolean backtrace;
  gboolean messages;
  gboolean progressive;
};


#define CLAY_TYPE_DASHBOARD            (clay_dashboard_get_type ())
#define CLAY_DASHBOARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DASHBOARD, ClayDashboard))
#define CLAY_DASHBOARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DASHBOARD, ClayDashboardClass))
#define CLAY_IS_DASHBOARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DASHBOARD))
#define CLAY_IS_DASHBOARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DASHBOARD))
#define CLAY_DASHBOARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DASHBOARD, ClayDashboardClass))


typedef struct _ClayDashboardPrivate ClayDashboardPrivate;
typedef struct _ClayDashboardClass   ClayDashboardClass;

struct _ClayDashboard
{
  ClayEditor            parent_instance;

  ClayDashboardPrivate *priv;
};

struct _ClayDashboardClass
{
  ClayEditorClass  parent_class;
};


GType                          clay_dashboard_get_type                   (void);

GtkWidget                    * clay_dashboard_new                        (Clay                          *clay,
                                                                          ClayMenuFactory               *menu_factory);

gboolean                       clay_dashboard_log_start_recording        (ClayDashboard                 *dashboard,
                                                                          GFile                         *file,
                                                                          const ClayDashboardLogParams  *params,
                                                                          GError                       **error);
gboolean                       clay_dashboard_log_stop_recording         (ClayDashboard                 *dashboard,
                                                                          GError                       **error);
gboolean                       clay_dashboard_log_is_recording           (ClayDashboard                 *dashboard);
const ClayDashboardLogParams * clay_dashboard_log_get_default_params     (ClayDashboard                 *dashboard);
void                           clay_dashboard_log_add_marker             (ClayDashboard                 *dashboard,
                                                                          const gchar                   *description);

void                           clay_dashboard_reset                      (ClayDashboard                 *dashboard);

void                           clay_dashboard_set_update_interval        (ClayDashboard                 *dashboard,
                                                                          ClayDashboardUpdateInteval     update_interval);
ClayDashboardUpdateInteval     clay_dashboard_get_update_interval        (ClayDashboard                 *dashboard);

void                           clay_dashboard_set_history_duration       (ClayDashboard                 *dashboard,
                                                                          ClayDashboardHistoryDuration   history_duration);
ClayDashboardHistoryDuration   clay_dashboard_get_history_duration       (ClayDashboard                 *dashboard);

void                           clay_dashboard_set_low_swap_space_warning (ClayDashboard                 *dashboard,
                                                                          gboolean                       low_swap_space_warning);
gboolean                       clay_dashboard_get_low_swap_space_warning (ClayDashboard                 *dashboard);

void                           clay_dashboard_menu_setup                 (ClayUIManager                 *manager,
                                                                          const gchar                   *ui_path);
