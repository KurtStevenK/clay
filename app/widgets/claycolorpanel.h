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


#define CLAY_TYPE_COLOR_PANEL            (clay_color_panel_get_type ())
#define CLAY_COLOR_PANEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_COLOR_PANEL, ClayColorPanel))
#define CLAY_COLOR_PANEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_COLOR_PANEL, ClayColorPanelClass))
#define CLAY_IS_COLOR_PANEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_COLOR_PANEL))
#define CLAY_IS_COLOR_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_COLOR_PANEL))
#define CLAY_COLOR_PANEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_COLOR_PANEL, ClayColorPanelClass))


typedef struct _ClayColorPanelClass ClayColorPanelClass;

struct _ClayColorPanel
{
  ClayColorButton  parent_instance;

  ClayContext     *context;
  GtkWidget       *color_dialog;

  gboolean         user_context_aware;
};

struct _ClayColorPanelClass
{
  ClayColorButtonClass  parent_class;

  /*  signals  */
  void (* response) (ClayColorPanel       *panel,
                     ClayColorDialogState  state);
};


GType       clay_color_panel_get_type        (void);

GtkWidget * clay_color_panel_new             (const gchar          *title,
                                              GeglColor            *color,
                                              ClayColorAreaType     type,
                                              gint                  width,
                                              gint                  height);

void        clay_color_panel_set_context     (ClayColorPanel       *panel,
                                              ClayContext          *context);

void        clay_color_panel_set_user_context_aware
                                             (ClayColorPanel       *panel,
                                              gboolean              user_context_aware);

void        clay_color_panel_dialog_response (ClayColorPanel       *panel,
                                              ClayColorDialogState  state);
