/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytemplateview.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_TEMPLATE_VIEW            (clay_template_view_get_type ())
#define CLAY_TEMPLATE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEMPLATE_VIEW, ClayTemplateView))
#define CLAY_TEMPLATE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEMPLATE_VIEW, ClayTemplateViewClass))
#define CLAY_IS_TEMPLATE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEMPLATE_VIEW))
#define CLAY_IS_TEMPLATE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEMPLATE_VIEW))
#define CLAY_TEMPLATE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_TEMPLATE_VIEW, ClayTemplateViewClass))


typedef struct _ClayTemplateViewClass  ClayTemplateViewClass;

struct _ClayTemplateView
{
  ClayContainerEditor  parent_instance;

  GtkWidget           *create_button;
  GtkWidget           *new_button;
  GtkWidget           *duplicate_button;
  GtkWidget           *edit_button;
  GtkWidget           *delete_button;
};

struct _ClayTemplateViewClass
{
  ClayContainerEditorClass  parent_class;
};


GType       clay_template_view_get_type (void);

GtkWidget * clay_template_view_new      (ClayViewType     view_type,
                                         ClayContainer   *container,
                                         ClayContext     *context,
                                         gint             view_size,
                                         gint             view_border_width,
                                         ClayMenuFactory *menu_factory);
