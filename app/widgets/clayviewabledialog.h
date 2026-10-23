/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayviewabledialog.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_VIEWABLE_DIALOG            (clay_viewable_dialog_get_type ())
#define CLAY_VIEWABLE_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_VIEWABLE_DIALOG, ClayViewableDialog))
#define CLAY_VIEWABLE_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_VIEWABLE_DIALOG, ClayViewableDialogClass))
#define CLAY_IS_VIEWABLE_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_VIEWABLE_DIALOG))
#define CLAY_IS_VIEWABLE_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_VIEWABLE_DIALOG))
#define CLAY_VIEWABLE_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_VIEWABLE_DIALOG, ClayViewableDialogClass))


typedef struct _ClayViewableDialogClass  ClayViewableDialogClass;

struct _ClayViewableDialog
{
  ClayDialog   parent_instance;

  ClayContext *context;

  GList       *viewables;

  GtkWidget   *icon;
  GtkWidget   *view;
  GtkWidget   *desc_label;
  GtkWidget   *viewable_label;
};

struct _ClayViewableDialogClass
{
  ClayDialogClass  parent_class;
};


GType       clay_viewable_dialog_get_type (void);

GtkWidget * clay_viewable_dialog_new      (GList              *viewables,
                                           ClayContext        *context,
                                           const gchar        *title,
                                           const gchar        *role,
                                           const gchar        *icon_name,
                                           const gchar        *desc,
                                           GtkWidget          *parent,
                                           ClayHelpFunc        help_func,
                                           const gchar        *help_id,
                                           ...) G_GNUC_NULL_TERMINATED;

void    clay_viewable_dialog_set_viewables (ClayViewableDialog *dialog,
                                            GList              *viewables,
                                            ClayContext        *context);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayViewableDialog, g_object_unref);
