/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayoverlaydialog.h
 * Copyright (C) 2009-2010  Michael Natterer <mitch@gimp.org>
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

#include "clayoverlayframe.h"


#define CLAY_RESPONSE_DETACH 100


#define CLAY_TYPE_OVERLAY_DIALOG            (clay_overlay_dialog_get_type ())
#define CLAY_OVERLAY_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_OVERLAY_DIALOG, ClayOverlayDialog))
#define CLAY_OVERLAY_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_OVERLAY_DIALOG, ClayOverlayDialogClass))
#define CLAY_IS_OVERLAY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_OVERLAY_DIALOG))
#define CLAY_IS_OVERLAY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_OVERLAY_DIALOG))
#define CLAY_OVERLAY_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_OVERLAY_DIALOG, ClayOverlayDialogClass))


typedef struct _ClayOverlayDialog      ClayOverlayDialog;
typedef struct _ClayOverlayDialogClass ClayOverlayDialogClass;

struct _ClayOverlayDialog
{
  ClayOverlayFrame  parent_instance;

  gchar            *title;
  gchar            *icon_name;

  GtkWidget        *header;
  GtkWidget        *icon_image;
  GtkWidget        *title_label;
  GtkWidget        *detach_button;
  GtkWidget        *close_button;
  GtkWidget        *action_area;
};

struct _ClayOverlayDialogClass
{
  ClayOverlayFrameClass  parent_class;

  void (* response) (ClayOverlayDialog *overlay,
                     gint               response_id);

  void (* detach)   (ClayOverlayDialog *overlay);
  void (* close)    (ClayOverlayDialog *overlay);
};


GType       clay_overlay_dialog_get_type               (void);

GtkWidget * clay_overlay_dialog_new                    (ClayToolInfo      *tool_info,
                                                        const gchar       *desc,
                                                        ...) G_GNUC_NULL_TERMINATED;

void        clay_overlay_dialog_response               (ClayOverlayDialog *overlay,
                                                        gint               response_id);
void        clay_overlay_dialog_add_buttons_valist     (ClayOverlayDialog *overlay,
                                                        va_list            args);
GtkWidget * clay_overlay_dialog_add_button             (ClayOverlayDialog *overlay,
                                                        const gchar       *button_text,
                                                        gint               response_id);
void        clay_overlay_dialog_set_alternative_button_order
                                                       (ClayOverlayDialog *overlay,
                                                        gint               n_ids,
                                                        gint              *ids);
void        clay_overlay_dialog_set_default_response   (ClayOverlayDialog *overlay,
                                                        gint               response_id);
void        clay_overlay_dialog_set_response_sensitive (ClayOverlayDialog *overlay,
                                                        gint               response_id,
                                                        gboolean           sensitive);
