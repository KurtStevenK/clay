/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayDisplayConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include "config/claycoreconfig.h"


#define CLAY_CONFIG_DEFAULT_IMAGE_TITLE_FORMAT  "%D*%f-%p.%i (%t, %o, %L) %wx%h"
#define CLAY_CONFIG_DEFAULT_IMAGE_STATUS_FORMAT "%n (%m)"


#define CLAY_TYPE_DISPLAY_CONFIG            (clay_display_config_get_type ())
#define CLAY_DISPLAY_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DISPLAY_CONFIG, ClayDisplayConfig))
#define CLAY_DISPLAY_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DISPLAY_CONFIG, ClayDisplayConfigClass))
#define CLAY_IS_DISPLAY_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DISPLAY_CONFIG))
#define CLAY_IS_DISPLAY_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DISPLAY_CONFIG))


typedef struct _ClayDisplayConfigClass ClayDisplayConfigClass;

struct _ClayDisplayConfig
{
  ClayCoreConfig      parent_instance;

  ClayCheckSize       transparency_size;
  ClayCheckType       transparency_type;
  GeglColor          *transparency_custom_color1;
  GeglColor          *transparency_custom_color2;
  gint                snap_distance;
  gint                marching_ants_speed;
  gboolean            resize_windows_on_zoom;
  gboolean            resize_windows_on_resize;
  gboolean            default_show_all;
  gboolean            default_dot_for_dot;
  gboolean            initial_zoom_to_fit;
  ClayDragZoomMode    drag_zoom_mode;
  gboolean            drag_zoom_speed;
  ClayCursorMode      cursor_mode;
  gboolean            cursor_updating;
  gboolean            show_brush_outline;
  gboolean            snap_brush_outline;
  gboolean            show_paint_tool_cursor;
  gchar              *image_title_format;
  gchar              *image_status_format;
  gdouble             monitor_xres;
  gdouble             monitor_yres;
  gboolean            monitor_res_from_gdk;
  ClayViewSize        nav_preview_size;
  ClayDisplayOptions *default_view;
  ClayDisplayOptions *default_fullscreen_view;
  gboolean            activate_on_focus;
  ClaySpaceBarAction  space_bar_action;
  ClayZoomQuality     zoom_quality;
  gboolean            use_event_history;

  GObject            *modifiers_manager;
};

struct _ClayDisplayConfigClass
{
  ClayCoreConfigClass  parent_class;
};


GType  clay_display_config_get_type (void);
