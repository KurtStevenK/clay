/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claybrushfactoryview.h
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

#include "claydatafactoryview.h"


#define CLAY_TYPE_BRUSH_FACTORY_VIEW            (clay_brush_factory_view_get_type ())
#define CLAY_BRUSH_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BRUSH_FACTORY_VIEW, ClayBrushFactoryView))
#define CLAY_BRUSH_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BRUSH_FACTORY_VIEW, ClayBrushFactoryViewClass))
#define CLAY_IS_BRUSH_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BRUSH_FACTORY_VIEW))
#define CLAY_IS_BRUSH_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BRUSH_FACTORY_VIEW))
#define CLAY_BRUSH_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BRUSH_FACTORY_VIEW, ClayBrushFactoryViewClass))


typedef struct _ClayBrushFactoryViewClass  ClayBrushFactoryViewClass;

struct _ClayBrushFactoryView
{
  ClayDataFactoryView  parent_instance;

  GtkWidget           *spacing_scale;
  GtkAdjustment       *spacing_adjustment;

  gboolean             change_brush_spacing;
  GQuark               spacing_changed_handler_id;
};

struct _ClayBrushFactoryViewClass
{
  ClayDataFactoryViewClass  parent_class;

  /* Signals */

  void (* spacing_changed) (ClayBrushFactoryView *view);
};


GType       clay_brush_factory_view_get_type (void);

GtkWidget * clay_brush_factory_view_new      (ClayViewType     view_type,
                                              ClayDataFactory *factory,
                                              ClayContext     *context,
                                              gboolean         change_brush_spacing,
                                              gint             view_size,
                                              gint             view_border_width,
                                              ClayMenuFactory *menu_factory);
