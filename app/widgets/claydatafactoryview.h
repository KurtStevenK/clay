/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydatafactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "claycontainereditor.h"


#define CLAY_TYPE_DATA_FACTORY_VIEW            (clay_data_factory_view_get_type ())
#define CLAY_DATA_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DATA_FACTORY_VIEW, ClayDataFactoryView))
#define CLAY_DATA_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DATA_FACTORY_VIEW, ClayDataFactoryViewClass))
#define CLAY_IS_DATA_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DATA_FACTORY_VIEW))
#define CLAY_IS_DATA_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DATA_FACTORY_VIEW))
#define CLAY_DATA_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DATA_FACTORY_VIEW, ClayDataFactoryViewClass))


typedef struct _ClayDataFactoryViewClass   ClayDataFactoryViewClass;
typedef struct _ClayDataFactoryViewPrivate ClayDataFactoryViewPrivate;

struct _ClayDataFactoryView
{
  ClayContainerEditor         parent_instance;

  ClayDataFactoryViewPrivate *priv;
};

struct _ClayDataFactoryViewClass
{
  ClayContainerEditorClass  parent_class;
};


GType             clay_data_factory_view_get_type                 (void);

GtkWidget *       clay_data_factory_view_new                      (ClayViewType         view_type,
                                                                   ClayDataFactory     *factory,
                                                                   ClayContext         *context,
                                                                   gint                 view_size,
                                                                   gint                 view_border_width,
                                                                   ClayMenuFactory     *menu_factory,
                                                                   const gchar         *menu_identifier,
                                                                   const gchar         *ui_path,
                                                                   const gchar         *action_group);


void              clay_data_factory_view_show_follow_theme_toggle (ClayDataFactoryView *view,
                                                                   gboolean             show);

GtkWidget       * clay_data_factory_view_get_edit_button          (ClayDataFactoryView *factory_view);
GtkWidget       * clay_data_factory_view_get_duplicate_button     (ClayDataFactoryView *factory_view);
ClayDataFactory * clay_data_factory_view_get_data_factory         (ClayDataFactoryView *factory_view);
GType             clay_data_factory_view_get_child_type           (ClayDataFactoryView *factory_view);
gboolean          clay_data_factory_view_has_data_new_func        (ClayDataFactoryView *factory_view);
gboolean          clay_data_factory_view_have                     (ClayDataFactoryView *factory_view,
                                                                   ClayObject          *object);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ClayDataFactoryView, g_object_unref);
