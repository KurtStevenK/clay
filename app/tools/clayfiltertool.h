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

#include "claycolortool.h"


#define CLAY_TYPE_FILTER_TOOL            (clay_filter_tool_get_type ())
#define CLAY_FILTER_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_FILTER_TOOL, ClayFilterTool))
#define CLAY_FILTER_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_FILTER_TOOL, ClayFilterToolClass))
#define CLAY_IS_FILTER_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_FILTER_TOOL))
#define CLAY_IS_FILTER_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_FILTER_TOOL))
#define CLAY_FILTER_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_FILTER_TOOL, ClayFilterToolClass))

#define CLAY_FILTER_TOOL_GET_OPTIONS(t)  (CLAY_FILTER_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayFilterToolClass ClayFilterToolClass;

struct _ClayFilterTool
{
  ClayColorTool       parent_instance;

  GeglNode           *operation;
  ClayDrawableFilter *existing_filter;
  GObject            *config;
  GObject            *default_config;
  ClayContainer      *settings;

  gchar              *description;

  gboolean            has_settings;

  ClayDrawableFilter *filter;

  ClayGuide          *preview_guide;

  gpointer            pick_identifier;
  gboolean            pick_abyss;

  /* dialog */
  gboolean            overlay;
  ClayToolGui        *gui;
  GtkWidget          *settings_box;
  GtkWidget          *controller_toggle;
  GtkWidget          *operation_settings_box;
  GtkWidget          *clip_combo;
  GtkWidget          *region_combo;
  GtkWidget          *active_picker;

  /* widget */
  ClayToolWidget     *widget;
  ClayToolWidget     *grab_widget;
};

struct _ClayFilterToolClass
{
  ClayColorToolClass  parent_class;

  /* virtual functions */
  gchar     * (* get_operation)   (ClayFilterTool    *filter_tool,
                                   gchar            **description);
  void        (* dialog)          (ClayFilterTool    *filter_tool);
  void        (* reset)           (ClayFilterTool    *filter_tool);
  void        (* set_config)      (ClayFilterTool    *filter_tool,
                                   ClayConfig        *config);
  void        (* config_notify)   (ClayFilterTool    *filter_tool,
                                   ClayConfig        *config,
                                   const GParamSpec  *pspec);

  gboolean    (* settings_import) (ClayFilterTool    *filter_tool,
                                   GInputStream      *input,
                                   GError           **error);
  gboolean    (* settings_export) (ClayFilterTool    *filter_tool,
                                   GOutputStream     *output,
                                   GError           **error);

  void        (* region_changed)  (ClayFilterTool    *filter_tool);
  void        (* color_picked)    (ClayFilterTool    *filter_tool,
                                   gpointer           identifier,
                                   gdouble            x,
                                   gdouble            y,
                                   const Babl        *sample_format,
                                   GeglColor         *color);
};


GType       clay_filter_tool_get_type              (void);

void        clay_filter_tool_get_operation         (ClayFilterTool     *filter_tool,
                                                    ClayDrawableFilter *existing_filter);

void        clay_filter_tool_set_config            (ClayFilterTool     *filter_tool,
                                                    ClayConfig         *config);

void        clay_filter_tool_edit_as               (ClayFilterTool     *filter_tool,
                                                    const gchar        *new_tool_id,
                                                    ClayConfig         *config);

gboolean    clay_filter_tool_on_guide              (ClayFilterTool     *filter_tool,
                                                    const ClayCoords   *coords,
                                                    ClayDisplay        *display);

GtkWidget * clay_filter_tool_dialog_get_vbox       (ClayFilterTool     *filter_tool);

void        clay_filter_tool_enable_color_picking  (ClayFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    gboolean            pick_abyss);
void        clay_filter_tool_disable_color_picking (ClayFilterTool     *filter_tool);

GtkWidget * clay_filter_tool_add_color_picker      (ClayFilterTool     *filter_tool,
                                                    gpointer            identifier,
                                                    const gchar        *icon_name,
                                                    const gchar        *tooltip,
                                                    gboolean            pick_abyss,
                                                    ClayPickerCallback  callback,
                                                    gpointer            callback_data);
GCallback   clay_filter_tool_add_controller        (ClayFilterTool     *filter_tool,
                                                    ClayControllerType  controller_type,
                                                    const gchar        *status_title,
                                                    GCallback           callback,
                                                    gpointer            callback_data,
                                                    gpointer           *set_func_data);

void        clay_filter_tool_set_widget            (ClayFilterTool     *filter_tool,
                                                    ClayToolWidget     *widget);

gboolean    clay_filter_tool_get_drawable_area     (ClayFilterTool     *filter_tool,
                                                    gint               *drawable_offset_x,
                                                    gint               *drawable_offset_y,
                                                    GeglRectangle      *drawable_area);
